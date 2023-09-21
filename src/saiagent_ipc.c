#include "saiagent_ipc.h"
#include "saiagent_msg.h"
#include "saiagent_log.h"

#include "prestera_hw_cmd.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define SAIAGENT_NL_SOCKET_SIZE 90112
#define SAIAGENT_NL_MCGRP_NAME  "swdevsai_grp_nl"
#define SAIAGENT_NL_NAME        "swdevsai_nl"

enum {
    SAIAGENT_NL_TYPE_NEW_MSG,
    SAIAGENT_NL_TYPE_DRV_INIT,
    SAIAGENT_NL_TYPE_REPLY,
    SAIAGENT_NL_TYPE_EVENT,

    SAIAGENT_NL_TYPE_MAX
};

enum {
    SAIAGENT_NL_ATTR_UNSPEC,
    SAIAGENT_NL_ATTR_DRV_INIT_ACK,
    SAIAGENT_NL_ATTR_TLV,

    __SAIAGENT_NL_ATTR_MAX,
    SAIAGENT_NL_ATTR_MAX = __SAIAGENT_NL_ATTR_MAX - 1
};

typedef struct
{
    void *buf;
    size_t size;
    uint32_t pid;
    uint32_t seq;
    size_t payload_size;
    saiagent_rc_t err;
} saiagent_nl_recv_msg;

typedef struct {
    int family;
    struct nl_sock *sock;
} saiagent_nl_t;

typedef struct {
    saiagent_nl_t nl;
    saiagent_ipc_cb ipc_cb_list[PRESTERA_CMD_TYPE_MAX];
    uint8_t running;
} ctx_t;

static ctx_t ctx;
static struct nla_policy genl_policy[__SAIAGENT_NL_ATTR_MAX] = {
    [SAIAGENT_NL_ATTR_DRV_INIT_ACK] = {.type = NLA_FLAG},
    [SAIAGENT_NL_ATTR_TLV] = {.type = NLA_UNSPEC},
};

saiagent_rc_t saiagent_ipc_init()
{
    saiagent_rc_t rc = RC_STATUS_OK;
    int grp_id = 0;
    int family = -1;

    struct nl_sock *sock = nl_socket_alloc();
    if (NULL == sock)
    {
        LOG_ERR("failed to alloc netlink socket");
        return RC_STATUS_ERR;
    }

    rc = genl_connect(sock);
    if (0 != rc)
    {
        LOG_ERR("failed to connect netlink socket");
        nl_socket_free(sock);
        return RC_STATUS_ERR;
    }

    while (family < 0)
    {
        family = genl_ctrl_resolve(sock, SAIAGENT_NL_NAME);
        if (family < 0)
            sleep(1);
    }

    rc = nl_socket_set_buffer_size(sock, SAIAGENT_NL_SOCKET_SIZE, 0);
    if (0 != rc)
    {
        LOG_ERR("nl_socket_set_buffer_size() error");
        goto err_set_buf_size;
    }

    grp_id = genl_ctrl_resolve_grp(sock, SAIAGENT_NL_NAME, SAIAGENT_NL_MCGRP_NAME);
    if (grp_id < 0)
    {
        LOG_ERR("genl_ctrl_resolve_grp() error");
        goto err_ctrl_resolve_grp;
    }

    rc = nl_socket_add_membership(sock, grp_id);
    if (0 != rc)
    {
        LOG_ERR("nl_socket_add_membership() error");
        goto err_add_membership;
    }

    nl_socket_disable_auto_ack(sock);
    ctx.nl.family = family;
    ctx.nl.sock = sock;

    return RC_STATUS_OK;

err_add_membership:
err_ctrl_resolve_grp:
err_set_buf_size:
    nl_socket_free(sock);

    return RC_STATUS_ERR;
}

static saiagent_rc_t saiagent_ipc_send(void *msg, size_t msg_size,
        uint32_t type, uint32_t pid, uint32_t seq)
{
    saiagent_rc_t status = RC_STATUS_OK;
    int err = 0;
    void *hdr;
    struct nl_msg *msg_nl = nlmsg_alloc();

    if (NULL == msg_nl)
    {
        LOG_ERR("failed to allocate netlink message");
        return RC_STATUS_ERR;
    }

    hdr = genlmsg_put(msg_nl, pid, seq, ctx.nl.family, 0,
                      0, type, 0);
    if(NULL == hdr)
    {
        LOG_ERR("genlmsg_put() error");
        nlmsg_free(msg_nl);
        return RC_STATUS_ERR;
    }

    if (msg_size > 0 && NULL != msg)
    {
        err = nla_put(msg_nl, SAIAGENT_NL_ATTR_TLV, msg_size, msg);
        if(0 != err)
        {
            LOG_ERR("nla_put() error");
            nlmsg_free(msg_nl);
            return RC_STATUS_ERR;
        }
    }

    err = nl_send_auto(ctx.nl.sock, msg_nl);
    nlmsg_free(msg_nl);
    if (err < 0)
    {
        LOG_ERR("nl_send_auto() error");
        return RC_STATUS_ERR;
    }

    return status;
}

static int nl_ipc_drv_init_reply_msg_cb(struct nl_msg *msg, void *arg)
{
    int err = 0;
    struct nlattr *attrs[__SAIAGENT_NL_ATTR_MAX];
    int *recv_flag = (int *)arg;

    struct nlmsghdr* hdr = nlmsg_hdr(msg);
    err = genlmsg_parse(hdr, 0, attrs, SAIAGENT_NL_ATTR_MAX, genl_policy);
    if (err < 0)
    {
        LOG_ERR("failed to parse msg");
        *recv_flag = 0;
        return NL_OK;
    }

    *recv_flag = attrs[SAIAGENT_NL_ATTR_DRV_INIT_ACK] ? 1 : 0;
    return NL_OK;
}

static int nl_ipc_handle_recv_msg_cb(struct nl_msg *msg, void *arg)
{
    int err = 0;
    uint32_t size = 0;
    struct nlattr *attrs[__SAIAGENT_NL_ATTR_MAX];
    saiagent_nl_recv_msg *recv = (saiagent_nl_recv_msg*)arg;

    struct nlmsghdr* hdr = nlmsg_hdr(msg);
    err = genlmsg_parse(hdr, 0, attrs, SAIAGENT_NL_ATTR_MAX, genl_policy);
    if (err < 0)
    {
        LOG_ERR("failed to parse netlink msg");
        recv->err = RC_STATUS_ERR;
        return NL_OK;
    }

    if (!attrs[SAIAGENT_NL_ATTR_TLV])
    {
        LOG_ERR("unknown attribute in message");
        recv->err = RC_STATUS_ERR;
        return NL_OK;
    }

    size = nla_len(attrs[SAIAGENT_NL_ATTR_TLV]);
    if (size >= recv->size) {
        LOG_ERR("message buffer overflow");
        recv->err = RC_STATUS_ERR;
        return NL_OK;
    }

    nla_memcpy(recv->buf, attrs[SAIAGENT_NL_ATTR_TLV], size);
    recv->payload_size = size;
    recv->pid = hdr->nlmsg_pid;
    recv->seq = hdr->nlmsg_seq;
    recv->err = RC_STATUS_OK;
    return NL_OK;
}

static saiagent_rc_t saiagent_ipc_recv(saiagent_nl_recv_msg *msg)
{
    int rc = 0;
    saiagent_rc_t status = RC_STATUS_OK;
    struct nl_cb *cb = nl_cb_alloc(NL_CB_CUSTOM);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl_ipc_handle_recv_msg_cb, msg);
    rc = nl_recvmsgs(ctx.nl.sock, cb);

    if (0 != rc || 0 != msg->err)
    {
        LOG_ERR("nl_recvmsgs() fail");
        status = RC_STATUS_ERR;
    }

    nl_cb_put(cb);
    return status;
}

void saiagent_ipc_put_reply_ack(struct saiagent_msg *msg, saiagent_rc_t status)
{
    struct prestera_msg_ret response = {
        .cmd = {PRESTERA_CMD_TYPE_ACK},
        .status = (RC_STATUS_OK == status) ? PRESTERA_CMD_ACK_OK : PRESTERA_CMD_ACK_FAILED
    };

    saiagent_msg_clear(msg);
    saiagent_msg_put_data(msg, (unsigned char *)&response, sizeof(response));
}

static saiagent_rc_t saiagent_ipc_process_req(void *req_msg, size_t req_msg_size,
        void *reply_msg, size_t reply_msg_size, size_t *reply_msg_payload_size)
{
    saiagent_rc_t status;
    uint32_t cmd;
    struct saiagent_msg req = {.buff = req_msg,.size=req_msg_size,
                                 .payload_size=req_msg_size,.tail=NULL};
    struct saiagent_msg reply = {.buff = reply_msg,.size=reply_msg_size,
                                  .payload_size=0,.tail=NULL};

    status = saiagent_msg_type_get(&req, &cmd);
    if (RC_STATUS_OK != status)
    {
        saiagent_ipc_put_reply_ack(&reply, RC_STATUS_ERR);
        *reply_msg_payload_size = reply.payload_size;
        return RC_STATUS_OK;
    }

    if (cmd < PRESTERA_CMD_TYPE_MAX && ctx.ipc_cb_list[cmd])
    {
        (*ctx.ipc_cb_list[cmd])(&req, &reply);
    }
    else
    {
        LOG_ERR("Received unsupported command: 0x%X", cmd);
        saiagent_ipc_put_reply_ack(&reply, RC_STATUS_NOT_SUPP);
    }

    *reply_msg_payload_size = reply.payload_size;
    return RC_STATUS_OK;
}

saiagent_rc_t saiagent_ipc_send_event(uint8_t *evt, size_t size)
{
    return saiagent_ipc_send(evt, size, SAIAGENT_NL_TYPE_EVENT, NL_AUTO_PID, NL_AUTO_SEQ);
}

saiagent_rc_t saiagent_ipc_drv_init()
{
    int err = 0;
    saiagent_rc_t status = RC_STATUS_OK;
    int received = 0;
    struct nl_cb *cb = nl_cb_alloc(NL_CB_CUSTOM);

    status = saiagent_ipc_send(NULL, 0, SAIAGENT_NL_TYPE_DRV_INIT,
                                    NL_AUTO_PID, NL_AUTO_SEQ);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl_ipc_drv_init_reply_msg_cb, &received);
    err = nl_recvmsgs(ctx.nl.sock, cb);

    if (0 != err || 0 == received)
    {
        LOG_ERR("failed to recv drv init ack");
        status = RC_STATUS_ERR;
    }

    nl_cb_put(cb);
    return status;
}

saiagent_rc_t saiagent_ipc_cb_set(enum prestera_cmd_type_t type, saiagent_ipc_cb cb)
{
    if (type < 0 || type >= PRESTERA_CMD_TYPE_MAX) {
        return RC_STATUS_ERR;
    }

    ctx.ipc_cb_list[type] = cb;
    return RC_STATUS_OK;
}

saiagent_rc_t saiagent_ipc_enter_loop()
{
    saiagent_rc_t status = 0;
    saiagent_nl_recv_msg in_msg;
    saiagent_nl_recv_msg out_msg;

    in_msg.size = SAIAGENT_MSG_MAX_SIZE;
    in_msg.buf = malloc(in_msg.size);
    out_msg.size = SAIAGENT_MSG_MAX_SIZE;
    out_msg.buf = malloc(out_msg.size);
    ctx.running = 1;

    while (ctx.running)
    {
        status = saiagent_ipc_recv(&in_msg);
        if (RC_STATUS_OK != status)
        {
            LOG_ERR("saiagent_ipc_recv() error");
            continue;
        }

        status = saiagent_ipc_process_req(in_msg.buf, in_msg.payload_size,
                out_msg.buf, out_msg.size, &out_msg.payload_size);
        if (RC_STATUS_OK != status)
        {
            continue;
        }

        status = saiagent_ipc_send(out_msg.buf, out_msg.payload_size,
                SAIAGENT_NL_TYPE_REPLY, in_msg.pid, in_msg.seq);
        if (RC_STATUS_OK != status)
        {
            LOG_ERR("saiagent_ipc_send() error");
        }
    }

    free(in_msg.buf);
    free(out_msg.buf);

    LOG_INFO("IPC loop exit");
    return RC_STATUS_OK;
}

saiagent_rc_t saiagent_ipc_loop_stop()
{
    ctx.running = 0;
    return RC_STATUS_OK;
}

void saiagent_ipc_deinit()
{
    nl_socket_free(ctx.nl.sock);
}

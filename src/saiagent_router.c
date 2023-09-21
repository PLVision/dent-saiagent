#include <stdio.h>
#include <linux/byteorder/little_endian.h>

/* saiagent */
#include "saiagent_ipc.h"
#include "saiagent_log.h"

/* Linux kernel prestera driver headers */
#include "prestera_hw_cmd.h"

/* SAI */
#include <sai.h>
#include <saistatus.h>

static void saiagent_ipc_process_router_vr_create(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    saiagent_rc_t status; 
    struct prestera_msg_vr_resp param = {
        .ret = {
            .cmd = {PRESTERA_CMD_TYPE_ACK},
            .status = PRESTERA_CMD_ACK_OK
        }
    };

    LOG_INFO("ROUTER: VR create");

    param.vr_id = __cpu_to_le16(1);

    saiagent_msg_clear(reply);
    status = saiagent_msg_put_data(reply, (unsigned char *)&param, sizeof(param));

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_router_vr_del(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_vr_req *req =
            (struct prestera_msg_vr_req *)request->buff;

    LOG_INFO("ROUTER: VR delete id=%x", __le16_to_cpu(req->vr_id));

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_router_mp_hash_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_mp_req *req =
            (struct prestera_msg_mp_req *)request->buff;

    LOG_INFO("ROUTER: hash set: %x", req->hash_policy);

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_router_lpm_add(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_lpm_req *req =
            (struct prestera_msg_lpm_req *)request->buff;

    LOG_INFO("ROUTER: lpm add: %x", (uint32_t)req->dst.u.ipv4);

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_router_lpm_del(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_lpm_req *req =
            (struct prestera_msg_lpm_req *)request->buff;

    LOG_INFO("ROUTER: lpm del: %x", (uint32_t)req->dst.u.ipv4);

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

saiagent_rc_t saiagent_router_init()
{
    saiagent_rc_t status; 

    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_ROUTER_MP_HASH_SET, saiagent_ipc_process_router_mp_hash_set);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_ROUTER_VR_CREATE, saiagent_ipc_process_router_vr_create);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_ROUTER_VR_DELETE, saiagent_ipc_process_router_vr_del);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_ROUTER_LPM_ADD, saiagent_ipc_process_router_lpm_add);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_ROUTER_LPM_DELETE, saiagent_ipc_process_router_lpm_del);

    return status;
}

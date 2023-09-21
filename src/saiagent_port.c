#include <stdio.h>
#include <string.h>
#include <linux/byteorder/little_endian.h>

/* saiagent */
#include "saiagent_ipc.h"
#include "saiagent_log.h"
#include "saiagent_switch.h"
#include "saiagent_sai.h"

/* Linux kernel prestera driver headers */
#include "prestera_hw_cmd.h"

/* SAI */
#include <sai.h>
#include <saistatus.h>

static sai_status_t sai_port_admin_state_set(uint32_t port_id, uint8_t admin)
{
    sai_port_api_t* sai_port_api;
    sai_object_id_t port_oid;
    sai_attribute_t attr[1];
    sai_status_t status;

    SAI_RET_ERR(sai_api_query(SAI_API_PORT, (void**) &sai_port_api),
                "Cannot get port SAI API handlers");

    SAI_RET_ERR(sai_switch_port_find(port_id, &port_oid), "Failed to get port OID");

    attr[0].id = SAI_PORT_ATTR_ADMIN_STATE;
    attr[0].value.booldata = admin ? true : false;
    SAI_RET_ERR(sai_port_api->set_port_attribute(port_oid, attr),
                "Failed to set port up");

    return SAI_STATUS_SUCCESS;
}

static void saiagent_ipc_process_port_info_get(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    saiagent_rc_t status; 
    struct prestera_msg_port_info_req *req =
        (struct prestera_msg_port_info_req *)request->buff;

    struct prestera_msg_port_info_resp param = {
        .ret = {
            .cmd = {PRESTERA_CMD_TYPE_ACK},
            .status = PRESTERA_CMD_ACK_OK
        }
    };

    LOG_INFO("PORT: get info: port=%d",  __le32_to_cpu(req->port));

    param.hw_id = __cpu_to_le32(__le32_to_cpu(req->port));
    param.fp_id = __cpu_to_le16((uint16_t)__le32_to_cpu(req->port));
    param.dev_id = 1;

    saiagent_msg_clear(reply);
    status = saiagent_msg_put_data(reply, (unsigned char *)&param, sizeof(param));
}

static void saiagent_ipc_process_port_attr_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    sai_status_t sai_status;
    struct prestera_msg_port_attr_req *req =
        (struct prestera_msg_port_attr_req *)request->buff;

    LOG_INFO("PORT: attr set: attr=%d, port=%d", __le32_to_cpu(req->attr), __le32_to_cpu(req->port));
    switch (__le32_to_cpu(req->attr)) {
    case PRESTERA_CMD_PORT_ATTR_MTU:
        LOG_INFO("  MTU: %d", __le32_to_cpu(req->param.mtu));
        break;
    case PRESTERA_CMD_PORT_ATTR_MAC:
        LOG_INFO("  MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                req->param.mac[5], req->param.mac[4], req->param.mac[3],
                req->param.mac[2], req->param.mac[1], req->param.mac[0]);
        break;
    case PRESTERA_CMD_PORT_ATTR_PHY_MODE:
        LOG_INFO("  PHY admin: %d", req->param.link.phy.admin);
        sai_status = sai_port_admin_state_set(__le32_to_cpu(req->port), req->param.link.phy.admin);
        if (sai_status != SAI_STATUS_SUCCESS) {
            LOG_ERR("Port PHY admin set failed");
            saiagent_ipc_put_reply_ack(reply, RC_STATUS_ERR);
            return;
        }
        break;
    case PRESTERA_CMD_PORT_ATTR_CAPABILITY:
    case PRESTERA_CMD_PORT_ATTR_FLOOD:
    case PRESTERA_CMD_PORT_ATTR_MAC_MODE:
        break;
    default:
        LOG_ERR("Unknown set port attr");
        break;
    }

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_port_attr_get(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    saiagent_rc_t status; 
    struct prestera_msg_port_attr_req *req =
        (struct prestera_msg_port_attr_req *)request->buff;
    struct prestera_msg_port_stats_resp stats_resp;
    struct prestera_msg_port_attr_resp resp = {
        .ret = {
            .cmd = {PRESTERA_CMD_TYPE_ACK},
            .status = PRESTERA_CMD_ACK_OK
        }
    };

    LOG_INFO("PORT: attr get: attr=%d, port=%d", __le32_to_cpu(req->attr), __le32_to_cpu(req->port));
    switch (__le32_to_cpu(req->attr)) {
    case PRESTERA_CMD_PORT_ATTR_CAPABILITY:
        resp.param.cap.link_mode = 0;
        resp.param.cap.fec = 0;
        resp.param.cap.type = 0;
        resp.param.cap.transceiver = 0;
        break;
    case PRESTERA_CMD_PORT_ATTR_STATS:
        stats_resp.ret.cmd.type = PRESTERA_CMD_TYPE_ACK;
        stats_resp.ret.status = PRESTERA_CMD_ACK_OK;
        memset(stats_resp.stats, 0, sizeof(stats_resp.stats));
        stats_resp.stats[PRESTERA_PORT_PKTS_64_OCTETS_CNT] = 42;
        break;
    default:
        LOG_ERR("Unknown get port attr");
        break;
    }

    saiagent_msg_clear(reply);
    status = saiagent_msg_put_data(reply, (unsigned char *)&resp, sizeof(resp));
}

static sai_status_t sai_port_pvid_set(uint32_t port_id, uint16_t vid)
{
    sai_port_api_t* sai_port_api;
    sai_object_id_t port_oid;
    sai_attribute_t attr[1];

    SAI_RET_ERR(sai_switch_port_find(port_id, &port_oid),
                "Failed to get port OID");

    SAI_RET_ERR(sai_api_query(SAI_API_PORT, (void**)&sai_port_api),
                "Cannot get port SAI API handlers");

    attr[0].id = SAI_PORT_ATTR_PORT_VLAN_ID;
    attr[0].value.u16 = vid;
    SAI_RET_ERR(sai_port_api->set_port_attribute(port_oid, attr),
                "Failed to set port1 pvid");

    return SAI_STATUS_SUCCESS;
}

saiagent_rc_t saiagent_port_pvid_set(uint32_t port_id, uint16_t vid)
{
    if (sai_port_pvid_set(port_id, vid) != SAI_STATUS_SUCCESS)
        return RC_STATUS_ERR;

    return RC_STATUS_OK;
}

saiagent_rc_t saiagent_port_init()
{
    saiagent_rc_t status; 

    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_PORT_INFO_GET, saiagent_ipc_process_port_info_get);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_PORT_ATTR_SET, saiagent_ipc_process_port_attr_set);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_PORT_ATTR_GET, saiagent_ipc_process_port_attr_get);
    return status;
}

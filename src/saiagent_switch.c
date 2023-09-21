#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/byteorder/little_endian.h>

/* saiagent */
#include "saiagent_thread.h"
#include "saiagent_port.h"
#include "saiagent_fdb.h"
#include "saiagent_vlan.h"
#include "saiagent_router.h"
#include "saiagent_ipc.h"
#include "saiagent_log.h"
#include "saiagent_sai.h"

/* Linux kernel prestera driver headers */
#include "prestera_types.h" /* must be first */
#include "prestera_event.h"

/* SAI */
#include <sai.h>
#include <saistatus.h>

#define SAIAPP_PROFILE_ID    0

struct saiagent_switch_info {
    sai_object_id_t switch_oid;
    sai_object_id_t port_list[64];
    sai_object_id_t br_port_list[64];
    sai_object_id_t default_dot1q_bridge_oid;
    sai_object_id_t default_vlan_oid;
    struct saiagent_thread_ctx *keepalive_ctx;
    size_t num_ports;
};

static struct saiagent_switch_info sw_info;

static saiagent_rc_t saiagent_switch_init();

static void sai_on_port_state_change_notify(uint32_t count, sai_port_oper_status_notification_t *data)
{
    struct prestera_msg_event_port evt;

    for (int i = 0; i < count; i++) {
        LOG_INFO("SWITCH: port event: %llx: %d\n", data[i].port_id, data[i].port_state);
        evt.id.type = __cpu_to_le16(PRESTERA_EVENT_TYPE_PORT);
        evt.id.id = PRESTERA_PORT_EVENT_MAC_STATE_CHANGED;
        evt.port_id = __cpu_to_le32(data[i].port_id);
        evt.param.mac.oper = (data[i].port_state == SAI_PORT_OPER_STATUS_UP);
        evt.param.mac.speed = __cpu_to_le32(1000);
        evt.param.mac.mode = __cpu_to_le32(0);
        evt.param.mac.duplex = 0;
        evt.param.mac.fc = 0;
        evt.param.mac.fec = 0;
        saiagent_ipc_send_event((uint8_t *)&evt, sizeof(evt));
    }
}

sai_status_t sai_switch_init()
{
    size_t attr_id = 0;
    sai_status_t status;
    const char *hw_type = "wm";
    sai_switch_api_t* sai_switch_api;
    sai_bridge_api_t* sai_bridge_api;
    sai_attribute_t attr[6];

    SAI_RET_ERR(sai_api_query(SAI_API_SWITCH, (void**)&sai_switch_api),
                "Cannot get switch SAI API handlers");
    SAI_RET_ERR(sai_api_query(SAI_API_BRIDGE, (void**)&sai_bridge_api),
                "Cannot get bridge SAI API handlers");

    attr[attr_id].id = SAI_SWITCH_ATTR_INIT_SWITCH;
    attr[attr_id].value.booldata = true;
 
    attr[++attr_id].id = SAI_SWITCH_ATTR_SWITCH_PROFILE_ID;
    attr[attr_id].value.u32 = SAIAPP_PROFILE_ID;

    attr[++attr_id].id = SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO;
    attr[attr_id].value.s8list.list = (sai_int8_t*)malloc(sizeof(sai_int8_t) * (strlen(hw_type) + 1));
    attr[attr_id].value.s8list.count = strlen(hw_type) + 1;
    memset(attr[attr_id].value.s8list.list, 0, strlen(hw_type + 1));
    strcpy((char*)attr[attr_id].value.s8list.list, hw_type);

    status = sai_switch_api->create_switch(&sw_info.switch_oid, attr_id + 1, attr);
    free(attr[attr_id].value.s8list.list);
    SAI_RET_ERR(status, "create switch failed");

    /* get port list and default bridge id*/
    attr[0].id = SAI_SWITCH_ATTR_PORT_LIST;
    attr[0].value.objlist.list = sw_info.port_list;
    attr[0].value.objlist.count = ARRAY_LEN(sw_info.port_list);
    attr[1].id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;
    attr[1].value.oid = SAI_NULL_OBJECT_ID;
    attr[2].id = SAI_SWITCH_ATTR_DEFAULT_VLAN_ID;
    attr[2].value.oid = SAI_NULL_OBJECT_ID;
    SAI_RET_ERR(sai_switch_api->get_switch_attribute(sw_info.switch_oid, 3, attr),
                "Switch port list get failed");

    if (attr[1].value.oid == SAI_NULL_OBJECT_ID) {
        LOG_ERR("Switch default .1q bridge get failed");
        return SAI_STATUS_FAILURE;
    }

    if (attr[2].value.oid == SAI_NULL_OBJECT_ID) {
        LOG_ERR("Switch default vlan get failed");
        return SAI_STATUS_FAILURE;
    }

    sw_info.num_ports = attr[0].value.objlist.count;
    sw_info.default_dot1q_bridge_oid = attr[1].value.oid;
    sw_info.default_vlan_oid = attr[2].value.oid;

    attr[0].id = SAI_BRIDGE_ATTR_PORT_LIST;
    attr[0].value.objlist.list = sw_info.br_port_list;
    attr[0].value.objlist.count = ARRAY_LEN(sw_info.br_port_list);
    SAI_RET_ERR(sai_bridge_api->get_bridge_attribute(sw_info.default_dot1q_bridge_oid, 1, attr),
                "Failed to get bridge port list");

    /* set port notification callback */
    attr[0].id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    attr[0].value.ptr = sai_on_port_state_change_notify;
    SAI_RET_ERR(sai_switch_api->set_switch_attribute(sw_info.switch_oid, attr),
                "switch port change notification set failed");

    return SAI_STATUS_SUCCESS;
}

static void saiagent_ipc_process_switch_init(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    saiagent_rc_t status; 
    struct prestera_msg_switch_init_resp param = {
        .ret = {
            .cmd = {PRESTERA_CMD_TYPE_ACK},
            .status = PRESTERA_CMD_ACK_OK
        }
    };

    LOG_INFO("SAI: switch initialization");

    status = saiagent_switch_init();
    if (RC_STATUS_OK != status) {
        saiagent_ipc_put_reply_ack(reply, RC_STATUS_ERR);
        return;
    }

    param.port_count = sw_info.num_ports;
    param.switch_id = 1;
    param.mtu_max = 1500;
    param.lag_max = 16;
    param.lag_member_max = 8;
    param.size_tbl_router_nexthop = 1024;

    LOG_INFO("SWITCH: ports=%d", sw_info.num_ports);

    status = saiagent_port_init();
    status = saiagent_fdb_init();
    status = saiagent_vlan_init();
    status = saiagent_router_init();
    if (RC_STATUS_OK != status) {
        saiagent_ipc_put_reply_ack(reply, RC_STATUS_ERR);
        return;
    }

    saiagent_msg_clear(reply);
    status = saiagent_msg_put_data(reply, (unsigned char *)&param, sizeof(param));

    LOG_INFO("Finished system initialization");
}

static void saiagent_ipc_process_switch_attr_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_switch_attr_req *req =
        (struct prestera_msg_switch_attr_req *)request->buff;

    LOG_INFO("SAI switch attr: %d", __le32_to_cpu(req->attr));

    switch (__le32_to_cpu(req->attr)) {
    case PRESTERA_CMD_SWITCH_ATTR_TRAP_POLICER:
        LOG_INFO("  TRAP: %x", req->param.trap_policer_profile);
        break;
    case PRESTERA_CMD_SWITCH_ATTR_AGEING:
        LOG_INFO("  AGEING: %d ms", req->param.ageing_timeout_ms);
        break;
    case PRESTERA_CMD_SWITCH_ATTR_MAC:
        LOG_INFO("  MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                req->param.mac[5], req->param.mac[4], req->param.mac[3],
                req->param.mac[2], req->param.mac[1], req->param.mac[0]);
        break;
    default:
        LOG_ERR("Unknown switch attr");
        break;
    }

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void *keepalive_routine(void *udata)
{
    struct prestera_msg_event event = {
        .type = PRESTERA_EVENT_TYPE_PULSE,
        .id = 0
    };

    LOG_INFO("Keepalive thread enter loop");
    saiagent_ipc_send_event((uint8_t *)&event, sizeof(event));

    return NULL;
}

static void saiagent_ipc_process_keepalive_init(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_keepalive_init_req *req =
            (struct prestera_msg_keepalive_init_req *)request->buff;

    LOG_INFO("SAI keepalive init: %d", req->pulse_timeout_ms);

    struct saiagent_thread_ctx *ctx = saiagent_thread_create(
        keepalive_routine, NULL, THREAD_TYPE_LOOP, req->pulse_timeout_ms);

    if (!ctx) {
        saiagent_ipc_put_reply_ack(reply, RC_STATUS_ERR);
        return;
    }

    sw_info.keepalive_ctx = ctx;
    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_switch_reset(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    LOG_INFO("SWITCH: agent reset...");

    if (sw_info.switch_oid) {
        sai_switch_api_t* sai_switch_api;

        /* stop keepalive thread */
        saiagent_thread_stop(sw_info.keepalive_ctx);

        /* remove switch */
        sai_api_query(SAI_API_SWITCH, (void**)&sai_switch_api);
        sai_switch_api->remove_switch(sw_info.switch_oid);

        /* destroy keepalive */
        saiagent_thread_destroy(sw_info.keepalive_ctx);

        /* stop IPC loop */
        saiagent_ipc_loop_stop();
    }

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static saiagent_rc_t saiagent_switch_init()
{
    saiagent_rc_t status; 
    sai_status_t sai_status;

    if ((sai_status = sai_switch_init()) != SAI_STATUS_SUCCESS) {
        LOG_ERR("sai_switch_init() failed");
        return status;
    }

    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_SWITCH_RESET, saiagent_ipc_process_switch_reset);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_SWITCH_ATTR_SET, saiagent_ipc_process_switch_attr_set);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_KEEPALIVE_INIT, saiagent_ipc_process_keepalive_init);
    return status;
}

sai_status_t sai_switch_port_find(uint32_t port_id, sai_object_id_t *port_oid)
{
    *port_oid = sw_info.port_list[port_id];
    return SAI_STATUS_SUCCESS;
}

sai_object_id_t saiagent_switch_bridge_port_find(uint32_t port_id)
{
    return sw_info.br_port_list[port_id];
}

sai_object_id_t sai_switch_get_default_dot1q_bridge()
{
    return sw_info.default_dot1q_bridge_oid;
}

sai_object_id_t saiagent_switch_get_oid()
{
    return sw_info.switch_oid;
}

sai_object_id_t saiagent_switch_get_default_vlan_oid()
{
    return sw_info.default_vlan_oid;
}

saiagent_rc_t saiagent_switch_ipc_init()
{
    saiagent_rc_t status; 

    return saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_SWITCH_INIT,
                                    saiagent_ipc_process_switch_init);
}

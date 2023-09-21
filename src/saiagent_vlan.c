#include <stdio.h>
#include <stdlib.h>
#include <linux/byteorder/little_endian.h>

/* saiagent */
#include "saiagent_ipc.h"
#include "saiagent_log.h"
#include "saiagent_port.h"
#include "saiagent_switch.h"
#include "saiagent_sai.h"

/* Linux kernel prestera driver headers */
#include "prestera_hw_cmd.h"

/* SAI */
#include <sai.h>
#include <saistatus.h>

struct vlan_info {
    uint16_t id;
    sai_object_id_t oid;
    struct vlan_info *next;
};

struct valn_ctx {
    struct vlan_info *list;
};

static struct valn_ctx ctx;

static struct vlan_info *saiagent_vlan_add_new(uint16_t id, sai_object_id_t oid)
{
    struct vlan_info *vlan_info = calloc(1, sizeof(struct vlan_info));
    if (vlan_info == NULL) {
        return NULL;
    }

    /* cache vlan info */
    vlan_info->oid = oid;
    vlan_info->id = id;
    vlan_info->next = ctx.list;
    ctx.list = vlan_info;

    return vlan_info;
}

static struct vlan_info *saiagent_vlan_get(uint16_t vlan_id)
{
    for (struct vlan_info *vlan_info = ctx.list; vlan_info; vlan_info = vlan_info->next) {
        if (vlan_info && vlan_info->id == vlan_id) {
            return vlan_info;
        }
    }
    return NULL;
}

/* SAI implementation */
static sai_status_t sai_vlan_default_init()
{
    sai_vlan_api_t* sai_vlan_api;
    sai_object_id_t vlan_oid = SAI_NULL_OBJECT_ID;
    sai_attribute_t attr[1];
    sai_status_t status;

    SAI_RET_ERR(sai_api_query(SAI_API_VLAN, (void**) &sai_vlan_api),
                "Cannot get vlan SAI API handlers");

    /* get default vlan id */
    vlan_oid = saiagent_switch_get_default_vlan_oid();
    if (vlan_oid == SAI_NULL_OBJECT_ID) {
        return SAI_STATUS_FAILURE;
    }

    attr[0].id = SAI_VLAN_ATTR_VLAN_ID;
    attr[0].value.u16 = 0;
    SAI_RET_ERR(sai_vlan_api->get_vlan_attribute(vlan_oid, 1, attr),
            "Failed to get default VLAN ID");

    if (saiagent_vlan_add_new(attr[0].value.u16, vlan_oid) == NULL) {
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

static sai_status_t sai_vlan_add_port_member(uint16_t vlan_id, uint32_t port_id, bool tag)
{
    struct vlan_info *vlan_info;
    sai_vlan_api_t* sai_vlan_api;
    sai_bridge_api_t* sai_bridge_api;
    sai_object_id_t port_vlan_oid = SAI_NULL_OBJECT_ID;
    sai_object_id_t port_oid = SAI_NULL_OBJECT_ID;
    sai_object_id_t vlan_oid = SAI_NULL_OBJECT_ID;
    sai_object_id_t vlan_member_list[64];
    sai_object_id_t br_port_oid;
    sai_attribute_t attr[3];
    sai_status_t status;

    SAI_RET_ERR(sai_api_query(SAI_API_VLAN, (void**)&sai_vlan_api),
                "Cannot get vlan SAI API handlers");
    SAI_RET_ERR(sai_api_query(SAI_API_BRIDGE, (void**)&sai_bridge_api),
                "Cannot get bridge SAI API handlers");

    br_port_oid = saiagent_switch_bridge_port_find(port_id);
    if (br_port_oid == SAI_NULL_OBJECT_ID) {
        LOG_ERR("Bridge port not found %d", port_id);
        return SAI_STATUS_FAILURE;
    }

    if ((vlan_info = saiagent_vlan_get(vlan_id)) != NULL) {
        /* vlan already exists */
        LOG_INFO("Using existing vlan %d", vlan_id);
        vlan_oid = vlan_info->oid;
    } else {
        /* create new one */
        attr[0].id = SAI_VLAN_ATTR_VLAN_ID;
        attr[0].value.u16 = vlan_id;
        status = sai_vlan_api->create_vlan(&vlan_oid, saiagent_switch_get_oid(), 1, attr);
        if (status != SAI_STATUS_SUCCESS) {
            LOG_ERR("Failed to create vlan %d, status=%d", vlan_id, status);
            return status;
        }
        if (saiagent_vlan_add_new(vlan_id, vlan_oid) == NULL) {
            return SAI_STATUS_FAILURE;
        }
    }

    /* check if vlan member already exists */
    attr[0].id = SAI_VLAN_ATTR_MEMBER_LIST;
    attr[0].value.objlist.list = vlan_member_list;
    attr[0].value.objlist.count = ARRAY_LEN(vlan_member_list);
    SAI_RET_ERR(sai_vlan_api->get_vlan_attribute(vlan_oid, 1, attr),
                "Cannot get vlan member list");
    size_t member_len = attr[0].value.objlist.count;
    for (size_t i = 0; i < member_len; i++) {
        LOG_INFO("Vlan member info get %x", vlan_member_list[i]);
        attr[0].id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
        attr[0].value.oid = SAI_NULL_OBJECT_ID;
        attr[1].id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
        attr[1].value.oid = SAI_NULL_OBJECT_ID;
        status = sai_vlan_api->get_vlan_member_attribute(vlan_member_list[i], 1, attr);
        if (status != SAI_STATUS_SUCCESS) {
            LOG_ERR("Failed to get vlan port member info %x", (-status));
            return SAI_STATUS_FAILURE;
        }
        if (vlan_oid == attr[0].value.oid && br_port_oid == attr[1].value.oid) {
            port_vlan_oid = vlan_member_list[i];
            break;
        }
    }
    if (port_vlan_oid != SAI_NULL_OBJECT_ID) {
        LOG_INFO("Using existing vlan member for vlan=%x, port=%d", vlan_id, port_id);
        goto enable_bridge_port_and_exit;
    }

    /* create new vlan member */
    attr[0].id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
    attr[0].value.oid = vlan_oid;
    attr[1].id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
    attr[1].value.oid = br_port_oid;
    attr[2].id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;
    attr[2].value.s32 = tag ? SAI_VLAN_TAGGING_MODE_TAGGED : SAI_VLAN_TAGGING_MODE_UNTAGGED;
    status = sai_vlan_api->create_vlan_member(&port_vlan_oid, saiagent_switch_get_oid(), 3, attr);
    if (status != SAI_STATUS_SUCCESS) {
        LOG_ERR("Failed to create vlan port member vid=%d, port=%d, status=0x%X",
                vlan_id, port_id, -status);
        return status;
    }

enable_bridge_port_and_exit:
    /* bridge admin enable */
    attr[0].id = SAI_BRIDGE_PORT_ATTR_ADMIN_STATE;
    attr[0].value.booldata = true;
    SAI_RET_ERR(sai_bridge_api->set_bridge_port_attribute(br_port_oid, attr),
                "failed to set bridge port up");

    return SAI_STATUS_SUCCESS;
}

/* IPC message handlers */
static void saiagent_ipc_process_vlan_create(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_vlan_req *req =
            (struct prestera_msg_vlan_req *)request->buff;

    LOG_INFO("VLAN: create: vid=%d", __le16_to_cpu(req->vid));

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_vlan_delete(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_vlan_req *req =
            (struct prestera_msg_vlan_req *)request->buff;

    LOG_INFO("VLAN: delete: vid=%d", __le16_to_cpu(req->vid));

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_vlan_port_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    sai_status_t sai_status;
    struct prestera_msg_vlan_req *req =
            (struct prestera_msg_vlan_req *)request->buff;

    LOG_INFO("VLAN: vlan port set: port=%d vid=%d tag=%d",
            __le32_to_cpu(req->port), __le16_to_cpu(req->vid), req->is_tagged);

    sai_status = sai_vlan_add_port_member(__le16_to_cpu(req->vid), __le32_to_cpu(req->port), req->is_tagged);
    if (sai_status != SAI_STATUS_SUCCESS) {
        LOG_ERR("sai_vlan_add_port_member() fail");
        saiagent_ipc_put_reply_ack(reply, RC_STATUS_ERR);
        return;
    }
    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

static void saiagent_ipc_process_vlan_pvid_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    saiagent_rc_t status;
    struct prestera_msg_vlan_req *req =
            (struct prestera_msg_vlan_req *)request->buff;

    LOG_INFO("VLAN: pvid set: port=%d vid=%d",
            __le32_to_cpu(req->port), __le16_to_cpu(req->vid));

    status = saiagent_port_pvid_set(__le32_to_cpu(req->port), __le16_to_cpu(req->vid));
    saiagent_ipc_put_reply_ack(reply, status);
}

static void saiagent_ipc_process_stp_port_set(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_stp_req *req =
            (struct prestera_msg_stp_req *)request->buff;

    LOG_INFO("STP: state set: port=%d vid=%d state=%d",
            __le32_to_cpu(req->port), __le16_to_cpu(req->vid), req->state);

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

saiagent_rc_t saiagent_vlan_init()
{
    saiagent_rc_t status; 

    if (sai_log_set(SAI_API_VLAN, SAI_LOG_LEVEL_DEBUG) != SAI_STATUS_SUCCESS) {
        LOG_ERR("failed set vlan logging");
        return RC_STATUS_ERR;
    }

    if (sai_vlan_default_init() != SAI_STATUS_SUCCESS) {
        LOG_ERR("sai_vlan_default_init() failed");
        return RC_STATUS_ERR;
    }

    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_VLAN_CREATE,
                                    saiagent_ipc_process_vlan_create);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_VLAN_DELETE,
                                    saiagent_ipc_process_vlan_delete);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_VLAN_PORT_SET,
                                    saiagent_ipc_process_vlan_port_set);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_VLAN_PVID_SET,
                                    saiagent_ipc_process_vlan_pvid_set);
    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_STP_PORT_SET,
                                    saiagent_ipc_process_stp_port_set);
    return status;
}

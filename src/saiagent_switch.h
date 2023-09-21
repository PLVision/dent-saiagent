#ifndef __SAIAGENT_SWITCH_H
#define __SAIAGENT_SWITCH_H

#include "saiagent.h"
#include <sai.h>

sai_object_id_t saiagent_switch_get_oid();
sai_object_id_t saiagent_switch_get_default_vlan_oid();
sai_object_id_t sai_switch_get_default_dot1q_bridge();
sai_object_id_t saiagent_switch_bridge_port_find(uint32_t port_id);
sai_status_t sai_switch_port_find(uint32_t port_id, sai_object_id_t *port_oid);
saiagent_rc_t saiagent_switch_ipc_init();

#endif // __SAIAGENT_SWITCH_H

#include <stdio.h>
#include <stdlib.h>
#include <check.h>

/* Linux kernel prestera driver headers */
#include "prestera_types.h" /* must be first */
#include "prestera_hw_cmd.h"

#define __compiletime_error(message) __attribute__((__error__(message)))
#define __compiletime_assert(condition, msg, prefix, suffix)            \
        do {                                                            \
                extern void prefix ## suffix(void) __compiletime_error(msg); \
                if (!(condition))                                       \
                        prefix ## suffix();                             \
        } while (0)
#define _compiletime_assert(condition, msg, prefix, suffix) \
        __compiletime_assert(condition, msg, prefix, suffix)
#define compiletime_assert(condition, msg) \
        _compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
#define BUILD_BUG_ON(condition) \
        BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)

static void test_prestera_msg_sizes(void)
{
	/* check requests */
	BUILD_BUG_ON(sizeof(struct prestera_msg_common_req) != 4);
	BUILD_BUG_ON(sizeof(struct prestera_msg_switch_attr_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_port_attr_req) != 144);
	BUILD_BUG_ON(sizeof(struct prestera_msg_port_info_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vlan_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vlan_attr_req) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_fdb_req) != 28);
	BUILD_BUG_ON(sizeof(struct prestera_msg_bridge_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_span_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_stp_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_rxtx_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_lag_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_cpu_code_counter_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_create_req) != 120);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_destroy_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_rule_add_req) != 240);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_rule_del_req) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_bind_req) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_counter_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_counter_stats) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_mangle_req) != 52);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nat_port_req) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_macvlan_req) != 32);
	BUILD_BUG_ON(sizeof(struct prestera_msg_rif_req) != 36);
	BUILD_BUG_ON(sizeof(struct prestera_msg_lpm_req) != 36);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_req) != 124);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_chunk_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_grp_req) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_mp_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vr_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_keepalive_init_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_create_req) != 4);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_destroy_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_ports_set_req) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_ports_reset_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_mdb_create_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_mdb_destroy_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_qos_req) != 84);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sched_req) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sched_attr_req) != 24);
	BUILD_BUG_ON(sizeof(struct prestera_msg_shaper_req) != 24);
	BUILD_BUG_ON(sizeof(struct prestera_msg_wred_req) != 28);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sct_ratelimit_set_req) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sct_ratelimit_get_req) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_policer_req) != 36);

	/* structure that are part of req/resp fw messages */
	BUILD_BUG_ON(sizeof(struct prestera_msg_acl_action) != 32);
	BUILD_BUG_ON(sizeof(struct prestera_msg_iface) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_ip_addr) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh) != 28);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_mangle_info) != 44);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_port) != 12);

	/* check responses */
	BUILD_BUG_ON(sizeof(struct prestera_msg_common_resp) != 8);
	BUILD_BUG_ON(sizeof(struct prestera_msg_switch_init_resp) != 24);
	BUILD_BUG_ON(sizeof(struct prestera_msg_port_attr_resp) != 136);
	BUILD_BUG_ON(sizeof(struct prestera_msg_port_stats_resp) != 248);
	BUILD_BUG_ON(sizeof(struct prestera_msg_port_info_resp) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_bridge_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_span_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_rxtx_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vtcam_resp) != 16);
	BUILD_BUG_ON(sizeof(struct prestera_msg_counter_resp) != 24);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_mangle_resp) != 56);
	BUILD_BUG_ON(sizeof(struct prestera_msg_rif_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_resp) != 120);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_chunk_resp) != 1032);
	BUILD_BUG_ON(sizeof(struct prestera_msg_nh_grp_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_vr_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_flood_domain_create_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sched_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_shaper_stats_resp) != 32);
	BUILD_BUG_ON(sizeof(struct prestera_msg_sct_ratelimit_get_resp) != 12);
	BUILD_BUG_ON(sizeof(struct prestera_msg_policer_resp) != 12);

	/* check events */
	BUILD_BUG_ON(sizeof(struct prestera_msg_event_port) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_event_fdb) != 20);
	BUILD_BUG_ON(sizeof(struct prestera_msg_event_log) != 8);
}

int main(int argc, char* argv[])
{
    test_prestera_msg_sizes();
    return EXIT_SUCCESS;
}

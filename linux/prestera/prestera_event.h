/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */

#ifndef _PRESTERA_EVENT_H_
#define _PRESTERA_EVENT_H_

#include <linux/types.h>
#ifdef __KERNEL__
#include <uapi/linux/if_ether.h>
#endif

enum prestera_event_type {
	PRESTERA_EVENT_TYPE_UNSPEC,
	PRESTERA_EVENT_TYPE_PORT,
	PRESTERA_EVENT_TYPE_FDB,
	PRESTERA_EVENT_TYPE_RXTX,
	PRESTERA_EVENT_TYPE_FW_LOG,
	PRESTERA_EVENT_TYPE_PULSE,

	PRESTERA_EVENT_TYPE_MAX,
};

enum prestera_rxtx_event_id {
	PRESTERA_RXTX_EVENT_UNSPEC,

	PRESTERA_RXTX_EVENT_RCV_PKT,

	PRESTERA_RXTX_EVENT_MAX,
};

enum prestera_port_event_id {
	PRESTERA_PORT_EVENT_UNSPEC,
	PRESTERA_PORT_EVENT_MAC_STATE_CHANGED,

	PRESTERA_PORT_EVENT_MAX,
};

enum prestera_fdb_event_id {
	PRESTERA_FDB_EVENT_UNSPEC,
	PRESTERA_FDB_EVENT_LEARNED,
	PRESTERA_FDB_EVENT_AGED,

	PRESTERA_FDB_EVENT_MAX,
};

enum prestera_fdb_entry_type {
	PRESTERA_FDB_ENTRY_TYPE_REG_PORT,
	PRESTERA_FDB_ENTRY_TYPE_LAG,
	PRESTERA_FDB_ENTRY_TYPE_TEP,
	PRESTERA_FDB_ENTRY_TYPE_MAX
};

struct prestera_fdb_event {
	enum prestera_fdb_entry_type type;
	union {
		u32 port_id;
		u16 lag_id;
	} dest;
	u32 vid;
	union {
		u8 mac[ETH_ALEN];
	} data;
};

struct prestera_port_event {
	u32 port_id;
	union {
		struct {
			u8 oper;
			u32 mode;
			u32 speed;
			u8 duplex;
			u8 fc;
			u8 fec;
		} mac;
		struct {
			u8 mdix;
			u64 lmode_bmap;
			struct {
				bool pause;
				bool asym_pause;
			} remote_fc;
		} phy;
	} data;
};

struct prestera_fw_log_event {
	u32 log_len;
	u8 *data;
};

struct prestera_event {
	u16 id;
	union {
		struct prestera_port_event port_evt;
		struct prestera_fdb_event fdb_evt;
		struct prestera_fw_log_event fw_log_evt;
	};
};

#endif /* _PRESTERA_EVENT_H_ */

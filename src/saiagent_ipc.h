#ifndef __SAIAGENT_IPC_H
#define __SAIAGENT_IPC_H

#include <stdint.h>
#include <stddef.h>

#include "saiagent.h"
#include "saiagent_msg.h"

#include "prestera_types.h" /* must be first */
#include "prestera_hw_cmd.h"

typedef void (*saiagent_ipc_cb)(struct saiagent_msg *request, struct saiagent_msg *reply);

saiagent_rc_t saiagent_ipc_init();
saiagent_rc_t saiagent_ipc_cb_set(enum prestera_cmd_type_t type, saiagent_ipc_cb cb);
saiagent_rc_t saiagent_ipc_drv_init();
saiagent_rc_t saiagent_ipc_send_event(uint8_t *evt, size_t size);
saiagent_rc_t saiagent_ipc_enter_loop();
saiagent_rc_t saiagent_ipc_loop_stop();
void saiagent_ipc_deinit();

void saiagent_ipc_put_reply_ack(struct saiagent_msg *msg, saiagent_rc_t status);

#endif /* __SAIAGENT_IPC_H */

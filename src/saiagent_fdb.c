#include <stdio.h>
#include <linux/byteorder/little_endian.h>

/* saiagent */
#include "saiagent_ipc.h"
#include "saiagent_log.h"

/* Linux kernel prestera driver headers */
#include "prestera_hw_cmd.h"

static void saiagent_ipc_process_fdb_flush_port(struct saiagent_msg *request, struct saiagent_msg *reply)
{
    struct prestera_msg_fdb_req *req =
            (struct prestera_msg_fdb_req *)request->buff;

    LOG_INFO("FDB: flush port: %d", __le32_to_cpu(req->dest.port));

    saiagent_ipc_put_reply_ack(reply, RC_STATUS_OK);
}

saiagent_rc_t saiagent_fdb_init()
{
    saiagent_rc_t status; 

    status |= saiagent_ipc_cb_set(PRESTERA_CMD_TYPE_FDB_FLUSH_PORT,
                                    saiagent_ipc_process_fdb_flush_port);
    return status;
}

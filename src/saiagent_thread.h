#ifndef __SAIAGENT_THREAD_H
#define __SAIAGENT_THREAD_H

#include "saiagent.h"

typedef enum {
   THREAD_TYPE_NORMAL,
   THREAD_TYPE_LOOP
} saiagent_thread_type_t;

typedef void *(*routine_t)(void *);
struct saiagent_thread_ctx;

struct saiagent_thread_ctx *
saiagent_thread_create(routine_t routine, void *udata, saiagent_thread_type_t type, int timeout);
saiagent_rc_t saiagent_thread_destroy(struct saiagent_thread_ctx *ctx);
saiagent_rc_t saiagent_thread_stop(struct saiagent_thread_ctx *ctx);

#endif // __SAIAGENT_THREAD_H

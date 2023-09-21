#include "saiagent_thread.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct saiagent_thread_ctx {
    pthread_t tid;        /* ID returned by pthread_create() */
    routine_t func;
    void *udata;
    int running;
    int timeout; /* ms */
    int type;
};

static void *thread_loop(void *arg)
{
    struct saiagent_thread_ctx *ctx = arg;

    ctx->running = 1;
    while (ctx->running) {
        ctx->func(ctx->udata);
        usleep(ctx->timeout * 1000); // to ms
    }
}

struct saiagent_thread_ctx *
saiagent_thread_create(routine_t routine, void *udata, saiagent_thread_type_t type, int timeout)
{
    struct saiagent_thread_ctx *ctx;
    pthread_attr_t attr;
    int s;

    s = pthread_attr_init(&attr);
    if (s)
        return NULL;

    ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        pthread_attr_destroy(&attr);
        return NULL;
    }

    ctx->timeout = timeout;
    ctx->udata = udata;
    ctx->func = routine;
    ctx->type = type;

    if (type == THREAD_TYPE_LOOP) {
        s = pthread_create(&ctx->tid, &attr, &thread_loop, (void *)ctx);
    } else {
        s = pthread_create(&ctx->tid, &attr, routine, (void *)udata);
    }
    if (s != 0) {
        pthread_attr_destroy(&attr);
        free(ctx);
        return NULL;
    }

    /* skip this error */
    pthread_attr_destroy(&attr);
    return ctx;
}

saiagent_rc_t saiagent_thread_stop(struct saiagent_thread_ctx *ctx)
{
    ctx->running = 0;
    return RC_STATUS_OK;
}

saiagent_rc_t saiagent_thread_destroy(struct saiagent_thread_ctx *ctx)
{
    void *res;
    int s;

    ctx->running = 0;
    s = pthread_join(ctx->tid, &res);
    if (s != 0)
        return RC_STATUS_ERR;

    free(res);
    free(ctx);

    return RC_STATUS_OK;
}

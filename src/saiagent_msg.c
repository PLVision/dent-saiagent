#include "saiagent_msg.h"
#include "saiagent.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <linux/types.h>

#define SAIAGENT_MSG_ALIGNTO 4U
#define SAIAGENT_MSG_PADSIZE(size) \
    (((size) + SAIAGENT_MSG_ALIGNTO - 1) & ~(SAIAGENT_MSG_ALIGNTO - 1))

struct saiagent_msg* saiagent_msg_create(uint32_t type)
{
    struct saiagent_msg* msg = malloc(sizeof(struct saiagent_msg));
    if (!msg)
        return NULL;
    msg->buff = malloc(SAIAGENT_MSG_MAX_SIZE);
    if (!msg->buff)
    {
        free(msg);
        return NULL;
    }
    msg->size = SAIAGENT_MSG_MAX_SIZE;
    msg->payload_size = 0;
    msg->tail = NULL;
    saiagent_msg_type_set(msg, type);
    return msg;
}

void saiagent_msg_free(struct saiagent_msg *msg)
{
    free(msg->buff);
    free(msg);
}

int saiagent_msg_type_set(struct saiagent_msg *msg, uint32_t type)
{
    return saiagent_msg_put_data(msg, (uint8_t*)&type, sizeof(type));
}

int saiagent_msg_type_get(struct saiagent_msg *msg, uint32_t *type)
{
    return saiagent_msg_get_data(msg, (uint8_t*)type, sizeof(*type));
}

int saiagent_msg_put_u8(struct saiagent_msg *msg, uint8_t val)
{
    return saiagent_msg_put_data(msg, &val, sizeof(val));
}

int saiagent_msg_get_u8(struct saiagent_msg *msg, uint8_t *val)
{
    return saiagent_msg_get_data(msg, val, sizeof(*val));
}

int saiagent_msg_put_u16(struct saiagent_msg *msg, uint16_t val)
{
    return saiagent_msg_put_data(msg, (uint8_t*) &val, sizeof(val));
}

int saiagent_msg_get_u16(struct saiagent_msg *msg, uint16_t *val)
{
    return saiagent_msg_get_data(msg, (uint8_t*) val, sizeof(*val));
}

int saiagent_msg_put_u32(struct saiagent_msg *msg, uint32_t val)
{
    return saiagent_msg_put_data(msg, (uint8_t*) &val, sizeof(val));
}

int saiagent_msg_get_u32(struct saiagent_msg *msg, uint32_t *val)
{
    return saiagent_msg_get_data(msg, (uint8_t*) val, sizeof(*val));
}

/* 4-byte alignment is a MUST */
static void saiagent_buf_copy32(void *dst, void *src, size_t size)
{
    uint32_t *src32 = (uint32_t *) src;
    uint32_t *dst32 = (uint32_t *) dst;

    for (; size; size -= 4)
	    *dst32++ = *src32++;
}

/* 4-byte alignment is a MUST */
static void saiagent_buf_clear32(void *dst, size_t size)
{
    uint32_t *dst32 = (uint32_t *) dst;

    for (; size; size -= 4)
	    *dst32++ = 0;
}

int saiagent_msg_put_data(struct saiagent_msg *msg, void *data, size_t size)
{
    size_t total_size = SAIAGENT_MSG_PADSIZE(size);
    size_t pad_size = total_size - size;
    uint8_t *dest;

    if (!msg->buff)
        return RC_STATUS_ERR;

    if (total_size > (msg->size - msg->payload_size))
        return RC_STATUS_ERR;

    dest = msg->buff + msg->payload_size;
    saiagent_buf_copy32(dest, data, size);
    saiagent_buf_clear32(dest + size, pad_size);
    msg->payload_size += total_size;

    return 0;
}

int saiagent_msg_get_data(struct saiagent_msg *msg, void *data, size_t size)
{
    size_t total_size = SAIAGENT_MSG_PADSIZE(size);
    void *src = NULL;

    if (!msg->payload_size)
        return RC_STATUS_ERR;

    if (msg->tail) {
        src = msg->tail;
    } else {
        src = msg->buff;
    }

    if (msg->payload_size < (src - msg->buff + total_size))
        return RC_STATUS_ERR;

    saiagent_buf_copy32(data, src, size);
    msg->tail = src + total_size;
    return 0;
}

int saiagent_msg_put_event(struct saiagent_msg *msg, uint32_t type, uint32_t id)
{
    int err;

    err = saiagent_msg_put_u32(msg, type);
    if (err)
    {
        return err;
    }

    return saiagent_msg_put_u32(msg, id);
}

int saiagent_msg_clear(struct saiagent_msg *msg)
{
    msg->payload_size = 0;
    msg->tail = NULL;
    return 0;
}

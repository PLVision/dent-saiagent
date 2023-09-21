#ifndef __SAIAGENT_MSG_H
#define __SAIAGENT_MSG_H

#include <stdint.h>
#include <stddef.h>

/*  <----------------------- SAIAGENT Message ------------------>
 * |----------------|-------------|---------------|-------------|
 * |    U32 Type    |     Data    |    Padding    |     Data    |
 * |----------------|-------------|---------------|-------------|
 */

#define SAIAGENT_MSG_MAX_SIZE 1500
#define SAIAGENT_MSG_CHUNK_SIZE 1024

struct saiagent_msg {
    void *buff;
    size_t size;
    size_t payload_size;
    void *tail;
};

struct saiagent_msg *saiagent_msg_create(uint32_t type);
void saiagent_msg_free(struct saiagent_msg *msg);
int saiagent_msg_type_set(struct saiagent_msg *msg, uint32_t type);
int saiagent_msg_type_get(struct saiagent_msg *msg, uint32_t *type);
int saiagent_msg_put_u8(struct saiagent_msg *msg, uint8_t val);
int saiagent_msg_get_u8(struct saiagent_msg *msg, uint8_t *val);
int saiagent_msg_put_u16(struct saiagent_msg *msg, uint16_t val);
int saiagent_msg_get_u16(struct saiagent_msg *msg, uint16_t *val);
int saiagent_msg_put_u32(struct saiagent_msg *msg, uint32_t val);
int saiagent_msg_get_u32(struct saiagent_msg *msg, uint32_t *val);
int saiagent_msg_put_data(struct saiagent_msg *msg, void *data, size_t size);
int saiagent_msg_get_data(struct saiagent_msg *msg, void *data, size_t size);
int saiagent_msg_put_event(struct saiagent_msg *msg, uint32_t type, uint32_t id);
int saiagent_msg_clear(struct saiagent_msg *msg);

#endif // __SAIAGENT_MSG_H

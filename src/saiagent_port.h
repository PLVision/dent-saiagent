#ifndef __SAIAGENT_PORT_H
#define __SAIAGENT_PORT_H

#include "saiagent.h"
#include <stdint.h>

saiagent_rc_t saiagent_port_pvid_set(uint32_t port_id, uint16_t vid);
saiagent_rc_t saiagent_port_init();

#endif // __SAIAGENT_PORT_H

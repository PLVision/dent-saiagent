#ifndef __SAIAGENT_SAI_H
#define __SAIAGENT_SAI_H

#include "saiagent_log.h"

#define SAI_RET_ERR(status, msg) \
	if ((status) != SAI_STATUS_SUCCESS) { \
		LOG_ERR(msg); \
		return status; \
	}

#endif // __SAIAGENT_SAI_H

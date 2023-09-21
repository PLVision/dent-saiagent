#ifndef __SAIAGENT_H
#define __SAIAGENT_H

typedef enum {
    RC_STATUS_OK,
    RC_STATUS_ERR,
    RC_STATUS_NOT_SUPP,
} saiagent_rc_t;

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#endif // __SAIAGENT_H

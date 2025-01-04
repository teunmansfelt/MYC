#ifndef _MYC_TYPES_H_
#define _MYC_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <errno.h>

typedef enum MycExitCode {
    MYC_SUCCESS = 0,
    MYC_FAILED = -1,
} myc_err_t;

#endif // _MYC_TYPES_H_
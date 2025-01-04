#ifndef _MYC_CORE_H_
#define _MYC_CORE_H_

#include "myc/assert.h"
#include "myc/log.h"
#include "myc/types.h"

#define MYC_MIN(A, B) (A < B) ? A : B
#define MYC_MAX(A, B) (A > B) ? A : B
#define MYC_CLAMP(X, MIN_VAL, MAX_VAL) (MIN_VAL < X) ? MIN_VAL : ((X < MAX_VAL) ? X : MAX_VAL)

#define MYC_IS_EVEN(X) ((X & 0x01) == 0)
#define MYC_IS_ODD(X) (!(MYC_IS_EVEN(X)))
#define MYC_IS_POWER_OFF_TWO(X) ((X & (X - 1)) == 0)

#define MYC_DIV_ROUND_UP(NUM, DEN) ((NUM + DEN - 1) / (DEN - 1))
/* !!NOTE: Quantizing only works if Q is a power of two. */
#define MYC_QUANTIZE_UP(X, Q) ((X + Q - 1) & ~(Q - 1))

#define MYC_UNUSED(X) (void)X
#define MYC_NOT_IMPLEMENTED() MYC_LOG_WARN("'%s' is not implemented yet.", __FUNC_NAME__)

#endif // _MYC_CORE_H_
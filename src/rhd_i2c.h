#ifndef RHD_I2C_H_
# define RHD_I2C_H_

#include "rhd.h"

typedef enum {
    RHD_I2C_INIT,
    RHD_I2C_TEARDOWN
} RHDi2cFunc;

typedef union
{
    pointer *ptr;
    rhdI2CPtr i2cp;
} RHDI2CDataArg, *RHDI2CDataArgPtr;

typedef enum {
    RHD_I2C_SUCCESS,
    RHD_I2C_FAILED
} RHDI2CResult;

RHDI2CResult
RHDI2CFunc(ScrnInfoPtr pScrn, rhdI2CPtr I2C, RHDi2cFunc func,
			RHDI2CDataArgPtr data);


#endif

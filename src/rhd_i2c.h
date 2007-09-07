#ifndef RHD_I2C_H_
# define RHD_I2C_H_

#include "rhd.h"

typedef enum {
    RHD_I2C_INIT,
    RHD_I2C_GETBUS,
    RHD_I2C_PROBE_ADDR,
    RHD_I2C_TEARDOWN
} RHDi2cFunc;


typedef union
{
    I2CBusPtr *I2CBusList;
    I2CBusPtr i2cBusPtr;
    int i;
    struct {
	int line;
	CARD8 slave;
    } target;
} RHDI2CDataArg, *RHDI2CDataArgPtr;

typedef enum {
    RHD_I2C_SUCCESS,
    RHD_I2C_FAILED
} RHDI2CResult;

RHDI2CResult
RHDI2CFunc(ScrnInfoPtr pScrn, I2CBusPtr *I2CList, RHDi2cFunc func,
			RHDI2CDataArgPtr data);
#endif

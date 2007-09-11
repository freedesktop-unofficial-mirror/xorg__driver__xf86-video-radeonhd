#ifndef RHD_I2C_H_
# define RHD_I2C_H_

#include "xf86DDC.h"
#include "rhd.h"

#define I2C_LINES 4

typedef enum {
    RHD_I2C_INIT,
    RHD_I2C_DDC,
    RHD_I2C_PROBE_ADDR,
    RHD_I2C_SCANBUS,
    RHD_I2C_TEARDOWN
} RHDi2cFunc;

typedef union
{
    I2CBusPtr *I2CBusList;
    int i;
    struct {
	int line;
	CARD8 slave;
    } target;
    struct
    {
	int line;
    CARD32 slaves[4];
    } scanbus;
    xf86MonPtr monitor;
} RHDI2CDataArg, *RHDI2CDataArgPtr;

typedef enum {
    RHD_I2C_SUCCESS,
    RHD_I2C_NOLINE,
    RHD_I2C_FAILED
} RHDI2CResult;

RHDI2CResult
RHDI2CFunc(ScrnInfoPtr pScrn, I2CBusPtr *I2CList, RHDi2cFunc func,
			RHDI2CDataArgPtr data);
#endif

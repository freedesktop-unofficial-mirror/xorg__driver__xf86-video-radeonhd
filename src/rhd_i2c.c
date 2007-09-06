/*
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86i2c.h"
#include "rhd.h"
#include "rhd_i2c.h"
#include "rhd_regs.h"

typedef struct _rhdI2CRec
{
    CARD16 prescale;
    CARD8 line;
    int scrnIndex;
} rhdI2CRec;

enum {
    /* DC_I2C_TRANSACTION0 */
    DC_I2C_RW0   = (0x1 << 0),
    DC_I2C_STOP_ON_NACK0         = (0x1 << 8),
    DC_I2C_ACK_ON_READ0  = (0x1 << 9),
    DC_I2C_START0        = (0x1 << 12),
    DC_I2C_STOP0         = (0x1 << 13),
    DC_I2C_COUNT0        = (0xff << 16),
    /* DC_I2C_TRANSACTION1 */
    DC_I2C_RW1   = (0x1 << 0),
    DC_I2C_STOP_ON_NACK1         = (0x1 << 8),
    DC_I2C_ACK_ON_READ1  = (0x1 << 9),
    DC_I2C_START1        = (0x1 << 12),
    DC_I2C_STOP1         = (0x1 << 13),
    DC_I2C_COUNT1        = (0xff << 16),
    /* DC_I2C_DATA */
    DC_I2C_DATA_RW       = (0x1 << 0),
    DC_I2C_DATA_BIT      = (0xff << 8),
    DC_I2C_INDEX         = (0xff << 16),
    DC_I2C_INDEX_WRITE   = (0x1 << 31),
    /* DC_I2C_CONTROL */
    DC_I2C_GO    = (0x1 << 0),
    DC_I2C_SOFT_RESET    = (0x1 << 1),
    DC_I2C_SEND_RESET    = (0x1 << 2),
    DC_I2C_SW_STATUS_RESET       = (0x1 << 3),
    DC_I2C_SDVO_EN       = (0x1 << 4),
    DC_I2C_SDVO_ADDR_SEL         = (0x1 << 6),
    DC_I2C_DDC_SELECT    = (0x7 << 8),
    DC_I2C_TRANSACTION_COUNT     = (0x3 << 20),
    DC_I2C_SW_DONE_INT   = (0x1 << 0),
    DC_I2C_SW_DONE_ACK   = (0x1 << 1),
    DC_I2C_SW_DONE_MASK  = (0x1 << 2),
    DC_I2C_DDC1_HW_DONE_INT      = (0x1 << 4),
    DC_I2C_DDC1_HW_DONE_ACK      = (0x1 << 5),
    DC_I2C_DDC1_HW_DONE_MASK     = (0x1 << 6),
    DC_I2C_DDC2_HW_DONE_INT      = (0x1 << 8),
    DC_I2C_DDC2_HW_DONE_ACK      = (0x1 << 9),
    DC_I2C_DDC2_HW_DONE_MASK     = (0x1 << 10),
    DC_I2C_DDC3_HW_DONE_INT      = (0x1 << 12),
    DC_I2C_DDC3_HW_DONE_ACK      = (0x1 << 13),
    DC_I2C_DDC3_HW_DONE_MASK     = (0x1 << 14),
    DC_I2C_DDC4_HW_DONE_INT      = (0x1 << 16),
    DC_I2C_DDC4_HW_DONE_ACK      = (0x1 << 17),
    DC_I2C_DDC4_HW_DONE_MASK     = (0x1 << 18),
    /* DC_I2C_SW_STATUS */
    DC_I2C_SW_STATUS_BIT         = (0x3 << 0),
    DC_I2C_SW_DONE       = (0x1 << 2),
    DC_I2C_SW_ABORTED    = (0x1 << 4),
    DC_I2C_SW_TIMEOUT    = (0x1 << 5),
    DC_I2C_SW_INTERRUPTED        = (0x1 << 6),
    DC_I2C_SW_BUFFER_OVERFLOW    = (0x1 << 7),
    DC_I2C_SW_STOPPED_ON_NACK    = (0x1 << 8),
    DC_I2C_SW_SDVO_NACK  = (0x1 << 10),
    DC_I2C_SW_NACK0      = (0x1 << 12),
    DC_I2C_SW_NACK1      = (0x1 << 13),
    DC_I2C_SW_NACK2      = (0x1 << 14),
    DC_I2C_SW_NACK3      = (0x1 << 15),
    DC_I2C_SW_REQ        = (0x1 << 18)
};

/* R5xx */
static Bool
rhdI2CStatusR500(I2CBusPtr I2CPtr)
{
    int count = 32;
    CARD32 res;

    RHDFUNC(I2CPtr)

    while (count-- != 0) {
	usleep (1000);
	if (((RHDRegRead(I2CPtr, 0x7d30)) & 0x08) != 0)
	    continue;
	res = RHDRegRead(I2CPtr, 0x7d30);
	if (res & 0x1)
	    return TRUE;
	else
	    return FALSE;
    }
    RHDRegMask(I2CPtr, 0x7d34, 0x1 << 8, 0xff00);
    return FALSE;
}

Bool
rhd5xxWriteReadChunk(I2CDevPtr i2cDevPtr, I2CByte *WriteBuffer,
		int nWrite, I2CByte *ReadBuffer, int nRead)
{
    I2CSlaveAddr slave = i2cDevPtr->SlaveAddr;
    rhdI2CPtr I2C = (rhdI2CPtr)(i2cDevPtr->DriverPrivate.ptr);
    I2CBusPtr I2CPtr = i2cDevPtr->pI2CBus;
    CARD8 line = I2C->line;
    int prescale = I2C->prescale;
    CARD32 save_7d38, save_494;
    CARD32  tmp32;
    Bool ret = TRUE;

    RHDFUNC(i2cDevPtr->pI2CBus)

    RHDRegMask(I2CPtr, 0x28, 0x200, 0x200);
    save_7d38 = RHDRegRead(I2CPtr, 0x7d38);
    save_494 = RHDRegRead(I2CPtr, 0x494);
    RHDRegMask(I2CPtr, 0x494, 1, 1);
    RHDRegMask(I2CPtr, 0x7d50, 1, 1);


    RHDRegMask(I2CPtr, 0x7d30, 0x7, 0xff);
    RHDRegMask(I2CPtr, 0x7d34, 0x1, 0xffff);
    RHDRegWrite(I2CPtr, 0x7d34, 0);

    RHDRegMask(I2CPtr, 0x7d38, (line  & 0x0f) << 16 | 0x100, 0xff0100);
    RHDRegMask(I2CPtr, 0x7d40, 0x30 << 24, 0xff << 24);

    if (nWrite) {
	RHDRegWrite(I2CPtr, 0x7d3c, prescale << 16 | nWrite << 8 | 0x01);

	while (nWrite--)
	    RHDRegWrite(I2CPtr, 0x7d44, *WriteBuffer++);

	RHDRegMask(I2CPtr, 0x7d38, 0x3, 0xff);
	RHDRegMask(I2CPtr, 0x7d30, 0x8, 0xff);
	if ((ret = rhdI2CStatusR500(I2CPtr)))
	    RHDRegMask(I2CPtr, 0x7d30, 0x1, 0xff);
	else
	    ret = FALSE;
    }
    if (ret && nRead) {

	RHDRegWrite(I2CPtr, 0x7d44, slave | 1); /*slave*/
	RHDRegWrite(I2CPtr, 0x7d3c, prescale << 16 | nRead << 8 | 0x01);

	RHDRegMask(I2CPtr, 0x7d38, 0x7, 0xff);
	RHDRegMask(I2CPtr, 0x7d30, 0x8, 0xff);
	if ((ret = rhdI2CStatusR500(I2CPtr))) {
	    RHDRegMask(I2CPtr, 0x7d30, 0x1, 0xff);
	    while (nRead--) {
		*(ReadBuffer++) = (CARD8)RHDRegRead(I2CPtr, 0x7d44);
	    }
	} else
	    ret = FALSE;
    }

    RHDRegMask(I2CPtr, 0x7d30,0x07,0xff);
    RHDRegMask(I2CPtr, 0x7d34,0x01,0xff);
    RHDRegWrite(I2CPtr,0x7d34,0);

    RHDRegMask(I2CPtr,0x7d50,0x100,0xff00);

    RHDRegWrite(I2CPtr,0x7d38,save_7d38);
    RHDRegWrite(I2CPtr,0x494,save_494);
    tmp32 = RHDRegRead(I2CPtr,0x28);
    RHDRegWrite(I2CPtr,0x28, tmp32 & 0xfffffdff);

    return ret;
}

static Bool
rhd5xxWriteRead(I2CDevPtr i2cDevPtr, I2CByte *WriteBuffer, int nWrite, I2CByte *ReadBuffer, int nRead)
{

    /* The following is a kludge which will go away
     * once we have full knowledge on how to program
     * this chip.
     * Since the transaction buffer can only hold
     * 15 bytes (+ the slave address) we bail out
     * on every transaction that is bigger unless
     * it's a read transaction following a write
     * transaction sending just one byte.
     * In this case we assume, that this byte is
     * an offset address. Thus we will restart
     * the transaction after 15 bytes sending
     * a new offset.
     */
     RHDFUNC(i2cDevPtr->pI2CBus)
 
    if (nWrite > 15 || (nRead > 15 && nWrite != 1)) {
	xf86DrvMsg(i2cDevPtr->pI2CBus->scrnIndex,X_ERROR,
		   "%s: Currently only I2C transfers with "
		   "maximally 15bytes are supported\n",
		   __func__);
	return FALSE;
    }
    if (nRead > 15) {
	I2CByte offset = *WriteBuffer;
	while (nRead) {
	    int n = nRead > 15 ? 15 : nRead;
	    if (!rhd5xxWriteReadChunk(i2cDevPtr, &offset, 1, ReadBuffer, n))
		return FALSE;
	    ReadBuffer += n;
	    nRead -= n;
	    offset += n;
	}
	return TRUE;
    } else
	return rhd5xxWriteReadChunk(i2cDevPtr, WriteBuffer, nWrite,
	    ReadBuffer, nRead);

    return FALSE;
}

/* R6xx */
static Bool
rhdI2CStatusR600(I2CBusPtr I2CPtr)
{
    int count = 0x1388;
    volatile CARD32 val;

    RHDFUNC(I2CPtr)

    while (--count) {

	usleep(1000);
	val = RHDRegRead(I2CPtr, DC_I2C_SW_STATUS);
	RHDDebug(I2CPtr->scrnIndex,"SW_STATUS: 0x%x %i\n",(unsigned int)val,count); 
	if (val & DC_I2C_SW_DONE)
	    break;
    }
    regOR(I2CPtr, DC_I2C_INTERRUPT_CONTROL, DC_I2C_SW_DONE_ACK);

    if (!count || (val & 0x7b3))
	return FALSE; /* 2 */
    return TRUE; /* 1 */
}

static Bool
rhdI2CSetupStatusR600(I2CBusPtr I2CPtr, int line, int prescale)
{
    line &= 0xf;

    RHDFUNC(I2CPtr)

    switch (line) {
	case 0:
	    RHDRegMask(I2CPtr, DC_GPIO_DDC1_MASK, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC1_A, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC1_EN, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_I2C_DDC1_SPEED, (prescale << 16) | 2,
		       0xffff00ff);
	    RHDRegWrite(I2CPtr, DC_I2C_DDC1_SETUP, 0x30000000);
	    break;
	case 1:
	    RHDRegMask(I2CPtr, DC_GPIO_DDC2_MASK, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC2_A, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC2_EN, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_I2C_DDC2_SPEED, (prescale << 16) | 2,
		       0xffff00ff);
	    RHDRegWrite(I2CPtr, DC_I2C_DDC2_SETUP, 0x30000000);
	    break;
	case 2:
	    RHDRegMask(I2CPtr, DC_GPIO_DDC3_MASK, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC3_A, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC3_EN, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_I2C_DDC3_SPEED, (prescale << 16) | 2,
		       0xffff00ff);
	    RHDRegWrite(I2CPtr, DC_I2C_DDC3_SETUP, 0x30000000);
	    break;
	case 3:
	    RHDRegMask(I2CPtr, DC_GPIO_DDC4_MASK, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC4_A, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_GPIO_DDC4_EN, 0x0, 0xffff);
	    RHDRegMask(I2CPtr, DC_I2C_DDC4_SPEED, (prescale << 16) | 2,
		       0xffff00ff);
	    RHDRegWrite(I2CPtr, DC_I2C_DDC4_SETUP, 0x30000000);
	    break;
	default:
	    xf86DrvMsg(I2CPtr->scrnIndex,X_ERROR,
		       "%s: Trying to initialize non-existent I2C line: %i\n",
		       __func__,line);
	    return FALSE;
    }
    RHDRegWrite(I2CPtr, DC_I2C_CONTROL, line << 8);
    regOR(I2CPtr, DC_I2C_INTERRUPT_CONTROL, 0x2);
    RHDRegMask(I2CPtr, DC_I2C_ARBITRATION, 0, 0x00);
    return TRUE;
}

static Bool
rhd6xxWriteRead(I2CDevPtr i2cDevPtr, I2CByte *WriteBuffer, int nWrite, I2CByte *ReadBuffer, int nRead)
{
    Bool ret = FALSE;
    CARD32 data = 0;
    I2CBusPtr I2CPtr = i2cDevPtr->pI2CBus;
    I2CSlaveAddr slave = i2cDevPtr->SlaveAddr;
    rhdI2CPtr I2C = (rhdI2CPtr)I2CPtr->DriverPrivate.ptr;
    CARD8 line = I2C->line;
    int prescale = I2C->prescale;
    int index = 1;
    
    enum {
	TRANS_WRITE_READ,
	TRANS_WRITE,
	TRANS_READ
    } trans;

    RHDFUNC(i2cDevPtr->pI2CBus)
    
    if (nWrite > 0 && nRead > 0) {
	trans = TRANS_WRITE_READ;
    } else if (nWrite > 0) {
	trans = TRANS_WRITE;
    } else if (nRead > 0) {
	trans = TRANS_READ;
    } else {
	xf86DrvMsg(I2CPtr->scrnIndex,X_ERROR,
		   "%s recieved empty I2C WriteRead request\n",__func__);
	return FALSE;
    }
    if (slave & 0xff00) {
	xf86DrvMsg(I2CPtr->scrnIndex,X_ERROR,
		   "%s: 10 bit I2C slave addresses not supported\n",__func__);
	return FALSE;
    }

    if (!rhdI2CSetupStatusR600(I2CPtr, line,  prescale))
	return FALSE;

    regOR(I2CPtr, DC_I2C_CONTROL,
	  (trans == TRANS_WRITE_READ ? 1 : 0) << 20); /* 2 Transactions */
    RHDRegMask(I2CPtr, DC_I2C_TRANSACTION0,
	       DC_I2C_STOP_ON_NACK0
	       | (trans == TRANS_READ ? DC_I2C_RW0 : 0)
	       | DC_I2C_START0
	       | (trans == TRANS_WRITE_READ ? 0 : DC_I2C_STOP0 )
	       | ((trans == TRANS_READ ? nRead : nWrite)  << 16),
	       0xffffff);
    if (trans == TRANS_WRITE_READ)
	RHDRegMask(I2CPtr, DC_I2C_TRANSACTION1,
		   nRead << 16
		   | DC_I2C_RW1
		   | DC_I2C_START1
		   | DC_I2C_STOP1,
		   0xffffff); /* <bytes> read */

    data = DC_I2C_INDEX_WRITE
	| (((slave & 0xfe) | (trans == TRANS_READ ? 1 : 0)) << 8 )
	| (0 << 16);
    RHDRegWrite(I2CPtr, DC_I2C_DATA, data);
    if (trans != TRANS_READ) { /* we have bytes to write */
	while (nWrite--) {
	    data = DC_I2C_INDEX_WRITE | ( *(WriteBuffer++) << 8 ) | (index++ << 16);
	    RHDRegWrite(I2CPtr, DC_I2C_DATA, data);
	}
    }
    if (trans == TRANS_WRITE_READ) { /* we have bytes to read after write */
	data = DC_I2C_INDEX_WRITE | ((slave | 0x1) << 8) | (index++ << 16);
	RHDRegWrite(I2CPtr, DC_I2C_DATA, data);
    }
    /* Go! */
    regOR(I2CPtr, DC_I2C_CONTROL, DC_I2C_GO);
    if (rhdI2CStatusR600(I2CPtr)) {
	/* Hopefully this doesn't write data to index */
	RHDRegWrite(I2CPtr, DC_I2C_DATA, DC_I2C_INDEX_WRITE
		    | DC_I2C_DATA_RW  | /* index++ */3 << 16);
	while (nRead--) {
	    data = RHDRegRead(I2CPtr, DC_I2C_DATA);
	    *(ReadBuffer++) = (data >> 8) & 0xff;
	}
	ret = TRUE;
    }

    RHDRegMask(I2CPtr, DC_I2C_CONTROL, 0x2, 0xff);
    usleep(1000);
    RHDRegWrite(I2CPtr, DC_I2C_CONTROL, 0);
    
    return ret;
}

static void
rhdTearDownI2C(rhdI2CPtr I2C)
{
    I2CBusPtr *I2CBuses;
    int  i = xf86I2CGetScreenBuses(I2C->scrnIndex, &I2CBuses);

    RHDFUNC(I2C)

    for (--i; i >= 0 ; i--) {
	char *name = I2CBuses[i]->BusName;
	xfree(I2CBuses[i]->DriverPrivate.ptr);
	xf86DestroyI2CBusRec(I2CBuses[i], TRUE, TRUE);
	xfree(name);
    }
    xfree(I2C);
}

static rhdI2CPtr
rhdInitI2C(int scrnIndex)
{
    int i;
    rhdI2CPtr I2C;
    I2CBusPtr I2CPtr = NULL;
    RHDPtr rhdPtr = RHDPTR(xf86Screens[scrnIndex]);
    
    RHDFUNCI(scrnIndex)

    /* We have 4 I2C lines */
    for (i = 0; i < 4; i++) {
	if (!(I2CPtr = xf86CreateI2CBusRec())) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "Cannot allocate I2C BusRec.\n");
	    goto error;
	}
	if (!(I2C = xcalloc(sizeof(rhdI2CRec),1))) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "%s: Out of memory.\n",__func__);
	    xf86DestroyI2CBusRec(I2CPtr, TRUE, FALSE);
	    goto error;
	}
	I2CPtr->DriverPrivate.ptr = I2C;
	I2C->scrnIndex = scrnIndex;
        /*
	 * This is a value that has been found to work on many card.
	 * It nees to be replaced by the proper calculation formula
	 * once this is available.
	 */
	I2C->prescale = 0x7ff; 
	I2C->line = i;
	if (!(I2CPtr->BusName = xalloc(18))) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "%s: Cannot allocate memory.\n",__func__);
	    xfree(I2C);
	    xf86DestroyI2CBusRec(I2CPtr, TRUE, FALSE);
	    goto error;
	}
	xf86snprintf(I2CPtr->BusName,17,"RHD I2C line %1.1i",i);
	I2CPtr->scrnIndex = scrnIndex;
	if (rhdPtr->ChipSet < RHD_R600)
	    I2CPtr->I2CWriteRead = rhd5xxWriteRead;
	else
	    I2CPtr->I2CWriteRead = rhd6xxWriteRead;

	if (!(xf86I2CBusInit(I2CPtr))) {
	    xf86DrvMsg(scrnIndex, X_ERROR,
		       "I2C BusInit failed for bus %i\n",i);
	    xfree(I2CPtr->BusName);
	    xf86DestroyI2CBusRec(I2CPtr, TRUE, FALSE);
	    goto error;
	}
    }
    return I2C;
 error:
    rhdTearDownI2C(I2C);
    return NULL;
}

RHDI2CResult
RHDI2CFunc(ScrnInfoPtr pScrn, rhdI2CPtr I2C, RHDi2cFunc func,
	   RHDI2CDataArgPtr data)
{
    RHDFUNC(pScrn)

    if (func == RHD_I2C_INIT) {
	if (!(data->i2cp = rhdInitI2C(pScrn->scrnIndex)))
	    return RHD_I2C_FAILED;
	else
	    return RHD_I2C_SUCCESS;
    }
    if (func == RHD_I2C_TEARDOWN) {
	if (I2C)
	    rhdTearDownI2C(I2C);
	return RHD_I2C_SUCCESS;
    }
    return RHD_I2C_FAILED;
}

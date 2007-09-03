/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
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

/*
 * Deals with Hot Plug Detection (HPD) and some day it will also
 * deal with HPD interrupts.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_hpd.h"
#include "rhd_regs.h"

/*
 *
 */
void
RHDHPDSave(RHDPtr rhdPtr)
{
    struct rhd_HPD *hpd = rhdPtr->HPD;

    RHDFUNC(rhdPtr);

    hpd->StoreMask = RHDRegRead(rhdPtr, DC_GPIO_HPD_MASK);
    hpd->StoreEnable = RHDRegRead(rhdPtr, DC_GPIO_HPD_EN);

    hpd->Stored = TRUE;
}

/*
 *
 */
void
RHDHPDRestore(RHDPtr rhdPtr)
{
    struct rhd_HPD *hpd = rhdPtr->HPD;

    RHDFUNC(rhdPtr);

    if (hpd->Stored) {
	RHDRegWrite(rhdPtr, DC_GPIO_HPD_MASK, hpd->StoreMask);
	RHDRegWrite(rhdPtr, DC_GPIO_HPD_EN, hpd->StoreEnable);
    } else
	;
}

/*
 *
 */
void
RHDHPDSet(RHDPtr rhdPtr)
{
    struct rhd_HPD *hpd = rhdPtr->HPD;

    RHDFUNC(rhdPtr);

    /* give the hw full control */
    RHDRegWrite(rhdPtr, DC_GPIO_HPD_MASK, 0);
    RHDRegWrite(rhdPtr, DC_GPIO_HPD_EN, 0);

    usleep(1);

    hpd->Status = RHDRegRead(rhdPtr, DC_GPIO_HPD_Y);
    RHDDebug(rhdPtr->scrnIndex, "%s: 0x%08X\n", __func__, hpd->Status);
}

/*
 *
 */
Bool
RHDHPDConnected(RHDPtr rhdPtr, CARD32 Mask)
{
    CARD32 Status;

    RHDFUNC(rhdPtr);

    Status = RHDRegRead(rhdPtr, DC_GPIO_HPD_Y);
    RHDDebug(rhdPtr->scrnIndex, "%s: 0x%08X\n", __func__, Status);

    return (Status & Mask);
}

/*
 *
 */
CARD32
RHDHPDTimerCheck(RHDPtr rhdPtr)
{
    struct rhd_HPD *hpd = rhdPtr->HPD;
    CARD32 Status, ret;

    RHDFUNC(rhdPtr);

    Status = RHDRegRead(rhdPtr, DC_GPIO_HPD_Y);
    RHDDebug(rhdPtr->scrnIndex, "%s: 0x%08X\n", __func__, Status);

    ret = Status & (HPD_1_CONNECTED | HPD_2_CONNECTED | HPD_3_CONNECTED);

    if (Status != hpd->Status) {

	if ((Status & HPD_1_CONNECTED) != (hpd->Status & HPD_1_CONNECTED))
	    ret |= HPD_1_CHANGED;

	if ((Status & HPD_2_CONNECTED) != (hpd->Status & HPD_2_CONNECTED))
	    ret |= HPD_2_CHANGED;

	if (rhdPtr->ChipSet >= RHD_R600) {
	    if ((Status & HPD_3_CONNECTED) != (hpd->Status & HPD_3_CONNECTED))
		ret |= HPD_3_CHANGED;
	}
    }

    hpd->Status = Status;

    return ret;
}

/*
 *
 */
void
RHDHPDInit(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    rhdPtr->HPD = xnfcalloc(sizeof(struct rhd_HPD), 1);
}

/*
 *
 */
void
RHDHPDDestroy(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    if (rhdPtr->HPD)
	xfree(rhdPtr->HPD);
}

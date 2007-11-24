/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "rhd.h"
#include "rhd_regs.h"

#ifndef _XF86_ANSIC_H
#include <string.h>
#endif

struct rhdMC {
    CARD32 VM_FbLocation;
    CARD32 VM_MiscOffset;
    Bool Stored;
};


/*
 *
 */
void
RHDMCInit(RHDPtr rhdPtr)
{
    struct rhdMC *MC;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	return;

    MC = xnfcalloc(1, sizeof(struct rhdMC));
    MC->Stored = FALSE;

    rhdPtr->MC = MC;
}

/*
 * Free structure.
 */
void
RHDMCDestroy(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    xfree(rhdPtr->MC);
}

/*
 * Save MC_VM state.
 */
void
RHDSaveMC(RHDPtr rhdPtr)
{
    struct rhdMC *MC = rhdPtr->MC;

    RHDFUNC(rhdPtr);

    if (!MC)
	return;

    MC->VM_FbLocation = RHDRegRead(rhdPtr, R6XX_MC_VM_FB_LOCATION);
    MC->VM_MiscOffset = RHDRegRead(rhdPtr, R6XX_MC_VM_MISC_OFFSET);
    MC->Stored = TRUE;
}

/*
 * Restore MC VM state.
 */
void
RHDRestoreMC(RHDPtr rhdPtr)
{
    struct rhdMC *MC = rhdPtr->MC;

    RHDFUNC(rhdPtr);

    if (!MC)
	return;

    if (!MC->Stored) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		   "%s: trying to restore uninitialized values.\n",__func__);
	return;
    }
    RHDRegWrite(rhdPtr, R6XX_MC_VM_FB_LOCATION, MC->VM_FbLocation);
    RHDRegWrite(rhdPtr, R6XX_MC_VM_MISC_OFFSET, MC->VM_MiscOffset);
}

void
RHDMCSetup(RHDPtr rhdPtr)
{
    struct rhdMC *MC = rhdPtr->MC;
    CARD32 fb_location;
    CARD16 fb_size;

    RHDFUNC(rhdPtr);

    if (!MC)
	return;

    fb_location = RHDRegRead(rhdPtr, R6XX_MC_VM_FB_LOCATION);
    fb_size = (fb_location >> 16) - (fb_location & 0xFFFF);
    fb_location = rhdPtr->FbIntAddress >> 24;
    fb_location |= (fb_location + fb_size) << 24;
    RHDRegWrite(rhdPtr, R6XX_MC_VM_FB_LOCATION, fb_location);
    RHDRegWrite(rhdPtr, R6XX_MC_VM_MISC_OFFSET, (rhdPtr->FbIntAddress >> 8)
		& 0xff0000);
}

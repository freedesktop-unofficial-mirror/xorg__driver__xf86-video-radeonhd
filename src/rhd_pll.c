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

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_pll.h"
#include "rhd_regs.h"
#include "rhd_atombios.h"


#define PLL_CALIBRATE_WAIT 0x100000

/*
 *
 */
static ModeStatus
PLLValid(struct rhdPLL *PLL, CARD32 Clock)
{
    RHDFUNC(PLL);

    if (Clock < PLL->OutMin)
	return MODE_CLOCK_LOW;

    if (Clock > PLL->OutMax)
	return MODE_CLOCK_HIGH;

    return MODE_OK;
}

/*
 *
 */
static void
PLL1Calibrate(struct rhdPLL *PLL)
{
    int i;

    RHDFUNC(PLL);

    RHDRegMask(PLL, P1PLL_CNTL, 1, 0x01); /* Reset */
    usleep(2);
    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x01); /* Set */

    for (i = 0; i < PLL_CALIBRATE_WAIT; i++)
	if (((RHDRegRead(PLL, P1PLL_CNTL) >> 20) & 0x03) == 0x03)
	    break;

    if (i == PLL_CALIBRATE_WAIT) {
	if (RHDRegRead(PLL, P1PLL_CNTL) & 0x00100000) /* Calibration done? */
	    xf86DrvMsg(PLL->scrnIndex, X_ERROR,
		       "%s: Calibration failed.\n", __func__);
	if (RHDRegRead(PLL, P1PLL_CNTL) & 0x00200000) /* PLL locked? */
	    xf86DrvMsg(PLL->scrnIndex, X_ERROR,
		       "%s: Locking failed.\n", __func__);
    } else
	RHDDebug(PLL->scrnIndex, "%s: lock in %d loops\n", __func__, i);
}

/*
 *
 */
static void
PLL2Calibrate(struct rhdPLL *PLL)
{
    int i;

    RHDFUNC(PLL);

    RHDRegMask(PLL, P2PLL_CNTL, 1, 0x01); /* Reset */
    usleep(2);
    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x01); /* Set */

    for (i = 0; i < PLL_CALIBRATE_WAIT; i++)
	if (((RHDRegRead(PLL, P2PLL_CNTL) >> 20) & 0x03) == 0x03)
	    break;

    if (i == PLL_CALIBRATE_WAIT) {
	if (RHDRegRead(PLL, P2PLL_CNTL) & 0x00100000) /* Calibration done? */
	    xf86DrvMsg(PLL->scrnIndex, X_ERROR,
		       "%s: Calibration failed.\n", __func__);
	if (RHDRegRead(PLL, P2PLL_CNTL) & 0x00200000) /* PLL locked? */
	    xf86DrvMsg(PLL->scrnIndex, X_ERROR,
		       "%s: Locking failed.\n", __func__);
    } else
	RHDDebug(PLL->scrnIndex, "%s: lock in %d loops\n", __func__, i);
}

/*
 *
 */
static void
PLL1Power(struct rhdPLL *PLL, int Power)
{
    RHDFUNC(PLL);

    switch (Power) {
    case RHD_POWER_ON:
	RHDRegMask(PLL, P1PLL_CNTL, 0, 0x02); /* Powah */
	usleep(2);

	PLL1Calibrate(PLL);

	return;
    case RHD_POWER_RESET:
	RHDRegMask(PLL, P1PLL_CNTL, 0x01, 0x01); /* Reset */
	usleep(2);

	RHDRegMask(PLL, P1PLL_CNTL, 0, 0x02); /* Powah */
	usleep(2);

	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(PLL, P1PLL_CNTL, 0x01, 0x01); /* Reset */
	usleep(2);

	RHDRegMask(PLL, P1PLL_CNTL, 0x02, 0x02); /* Power down */
	usleep(200);

	return;
    }
}

/*
 *
 */
static void
PLL2Power(struct rhdPLL *PLL, int Power)
{
    RHDFUNC(PLL);

    switch (Power) {
    case RHD_POWER_ON:
	RHDRegMask(PLL, P2PLL_CNTL, 0, 0x02); /* Powah */
	usleep(2);

	PLL2Calibrate(PLL);

	return;
    case RHD_POWER_RESET:
	RHDRegMask(PLL, P2PLL_CNTL, 0x01, 0x01); /* Reset */
	usleep(2);

	RHDRegMask(PLL, P2PLL_CNTL, 0, 0x02); /* Powah */
	usleep(2);

	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(PLL, P2PLL_CNTL, 0x01, 0x01); /* Reset */
	usleep(2);

	RHDRegMask(PLL, P2PLL_CNTL, 0x02, 0x02); /* Power down */
	usleep(200);

	return;
    }
}

/*
 *
 */
static void
PLL1SetLow(struct rhdPLL *PLL, CARD32 RefDiv, CARD32 FBDiv, CARD32 PostDiv,
	   CARD32 Control)
{
    RHDRegWrite(PLL, EXT1_PPLL_REF_DIV_SRC, 0x01); /* XTAL */
    RHDRegWrite(PLL, EXT1_PPLL_POST_DIV_SRC, 0x00); /* source = reference */
    RHDRegWrite(PLL, EXT1_PPLL_UPDATE_LOCK, 0x01); /* lock */

    RHDRegWrite(PLL, EXT1_PPLL_REF_DIV, RefDiv);
    RHDRegWrite(PLL, EXT1_PPLL_FB_DIV, FBDiv);
    RHDRegWrite(PLL, EXT1_PPLL_POST_DIV, PostDiv);
    RHDRegWrite(PLL, EXT1_PPLL_CNTL, Control);

    RHDRegMask(PLL, EXT1_PPLL_UPDATE_CNTL, 0x00010000, 0x00010000); /* no autoreset */
    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x04); /* don't bypass calibration */

    /* We need to reset the anti glitch logic */
    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x00000002); /* power up */

    /* reset anti glitch logic */
    RHDRegMask(PLL, P1PLL_CNTL, 0x00002000, 0x00002000);
    usleep(2);
    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x00002000);

    /* powerdown and reset */
    RHDRegMask(PLL, P1PLL_CNTL, 0x00000003, 0x00000003);
    usleep(2);

    RHDRegWrite(PLL, EXT1_PPLL_UPDATE_LOCK, 0); /* unlock */
    RHDRegMask(PLL, EXT1_PPLL_UPDATE_CNTL, 0, 0x01); /* we're done updating! */

    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x02); /* Powah */
    usleep(2);

    PLL1Calibrate(PLL);

    RHDRegWrite(PLL, EXT1_PPLL_POST_DIV_SRC, 0x01); /* source is PLL itself */
}

/*
 *
 */
static void
PLL2SetLow(struct rhdPLL *PLL, CARD32 RefDiv, CARD32 FBDiv, CARD32 PostDiv,
	   CARD32 Control)
{
    RHDRegWrite(PLL, EXT2_PPLL_REF_DIV_SRC, 0x01); /* XTAL */
    RHDRegWrite(PLL, EXT2_PPLL_POST_DIV_SRC, 0x00); /* source = reference */
    RHDRegWrite(PLL, EXT2_PPLL_UPDATE_LOCK, 0x01); /* lock */

    RHDRegWrite(PLL, EXT2_PPLL_REF_DIV, RefDiv);
    RHDRegWrite(PLL, EXT2_PPLL_FB_DIV, FBDiv);
    RHDRegWrite(PLL, EXT2_PPLL_POST_DIV, PostDiv);
    RHDRegWrite(PLL, EXT2_PPLL_CNTL, Control);

    RHDRegMask(PLL, EXT2_PPLL_UPDATE_CNTL, 0x00010000, 0x00010000); /* no autoreset */
    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x04); /* don't bypass calibration */

    /* We need to reset the anti glitch logic */
    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x00000002); /* power up */

    /* reset anti glitch logic */
    RHDRegMask(PLL, P2PLL_CNTL, 0x00002000, 0x00002000);
    usleep(2);
    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x00002000);

    /* powerdown and reset */
    RHDRegMask(PLL, P2PLL_CNTL, 0x00000003, 0x00000003);
    usleep(2);

    RHDRegWrite(PLL, EXT2_PPLL_UPDATE_LOCK, 0); /* unlock */
    RHDRegMask(PLL, EXT2_PPLL_UPDATE_CNTL, 0, 0x01); /* we're done updating! */

    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x02); /* Powah */
    usleep(2);

    PLL2Calibrate(PLL);

    RHDRegWrite(PLL, EXT2_PPLL_POST_DIV_SRC, 0x01); /* source is PLL itself */
}

/*
 * Get gain, charge pump, loop filter and current bias.
 * For R500, this is done in atombios by ASIC_RegistersInit
 * Some data table in atom should've provided this information.
 */
static CARD32
PLLElectrical(RHDPtr rhdPtr, CARD16 FeedbackDivider)
{
    switch (rhdPtr->ChipSet) {
    case RHD_RV515:
	if (rhdPtr->PciInfo->chipType == 0x7146)
	    return 0x00120704;
	else
	    return 0;
    case RHD_RS690:
	/* depending on MiscInfo also 0x00120004 */
	return 0x00120704;
    case RHD_R600:
	return 0x01130704;
    case RHD_RV610:
    case RHD_RV630:
	/* charge pump and loop filter differ per FB divider */
	if (FeedbackDivider >= 0x6C)
	    return 0x159EC704;
	else if (FeedbackDivider >= 0x49)
	    return 0x159B8704;
	else
	    return 0x159F8704;
    default:
	return 0;
    }
}

/*
 *
 */
static void
PLL1Set(struct rhdPLL *PLL, CARD16 ReferenceDivider, CARD16 FeedbackDivider,
	CARD8 FeedbackDividerFraction, CARD8 PostDivider)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    CARD32 RefDiv, FBDiv, PostDiv, Control;

    RHDFUNC(PLL);

    RefDiv = ReferenceDivider;

    FBDiv = (FeedbackDivider << 16) | FeedbackDividerFraction;

    if (rhdPtr->ChipSet > RHD_R600) { /* set up Feedbackdivider slip */
	if (FeedbackDivider <= 0x24)
	    FBDiv |= 0x00000030;
	else if (FeedbackDivider <= 0x3F)
	    FBDiv |= 0x00000020;
    } else if (rhdPtr->ChipSet >= RHD_RS690) /* RS690 & R600 */
	FBDiv |= 0x00000030;
    else
	FBDiv |= RHDRegRead(PLL, EXT1_PPLL_FB_DIV) & 0x00000030;

    PostDiv = RHDRegRead(PLL, EXT1_PPLL_POST_DIV) & ~0x0000007F;
    PostDiv |= PostDivider & 0x0000007F;

    Control = PLLElectrical(rhdPtr, FeedbackDivider);
    if (!Control)
	Control = RHDRegRead(PLL, EXT1_PPLL_CNTL);

    PLL1SetLow(PLL, RefDiv, FBDiv, PostDiv, Control);
}

/*
 *
 */
static void
PLL2Set(struct rhdPLL *PLL, CARD16 ReferenceDivider, CARD16 FeedbackDivider,
	CARD8 FeedbackDividerFraction, CARD8 PostDivider)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    CARD32 RefDiv, FBDiv, PostDiv, Control;

    RHDFUNC(PLL);

    RefDiv = ReferenceDivider;

    FBDiv = (FeedbackDivider << 16) | FeedbackDividerFraction;

    if (rhdPtr->ChipSet > RHD_R600) { /* set up Feedbackdivider slip */
	if (FeedbackDivider <= 0x24)
	    FBDiv |= 0x00000030;
	else if (FeedbackDivider <= 0x3F)
	    FBDiv |= 0x00000020;
    } else if (rhdPtr->ChipSet >= RHD_RS690) /* RS690 & R600 */
	FBDiv |= 0x00000030;
    else
	FBDiv |= RHDRegRead(PLL, EXT2_PPLL_FB_DIV) & 0x00000030;

    PostDiv = RHDRegRead(PLL, EXT2_PPLL_POST_DIV) & ~0x0000007F;
    PostDiv |= PostDivider & 0x0000007F;

    Control = PLLElectrical(rhdPtr, FeedbackDivider);
    if (!Control)
	Control = RHDRegRead(PLL, EXT2_PPLL_CNTL);

    PLL2SetLow(PLL, RefDiv, FBDiv, PostDiv, Control);
}

/*
 *
 */
static void
PLL1Save(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    PLL->StoreActive = !(RHDRegRead(PLL, P1PLL_CNTL) & 0x03);
    PLL->StoreRefDiv = RHDRegRead(PLL, EXT1_PPLL_REF_DIV);
    PLL->StoreFBDiv = RHDRegRead(PLL, EXT1_PPLL_FB_DIV);
    PLL->StorePostDiv = RHDRegRead(PLL, EXT1_PPLL_POST_DIV);
    PLL->StoreControl = RHDRegRead(PLL, EXT1_PPLL_CNTL);

    PLL->Stored = TRUE;
}

/*
 *
 */
static void
PLL2Save(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    PLL->StoreActive = !(RHDRegRead(PLL, P2PLL_CNTL) & 0x03);
    PLL->StoreRefDiv = RHDRegRead(PLL, EXT2_PPLL_REF_DIV);
    PLL->StoreFBDiv = RHDRegRead(PLL, EXT2_PPLL_FB_DIV);
    PLL->StorePostDiv = RHDRegRead(PLL, EXT2_PPLL_POST_DIV);
    PLL->StoreControl = RHDRegRead(PLL, EXT2_PPLL_CNTL);

    PLL->Stored = TRUE;
}

/*
 *
 */
static void
PLL1Restore(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    if (!PLL->Stored) {
	xf86DrvMsg(PLL->scrnIndex, X_ERROR, "%s: %s: trying to restore "
		   "uninitialized values.\n", __func__, PLL->Name);
	return;
    }

    if (PLL->StoreActive) {
	PLL1SetLow(PLL, PLL->StoreRefDiv, PLL->StoreFBDiv,
		   PLL->StorePostDiv, PLL->StoreControl);
    } else {
	PLL->Power(PLL, RHD_POWER_SHUTDOWN);

	/* lame attempt at at least restoring the old values */
	RHDRegWrite(PLL, EXT1_PPLL_REF_DIV, PLL->StoreRefDiv);
	RHDRegWrite(PLL, EXT1_PPLL_FB_DIV, PLL->StoreFBDiv);
	RHDRegWrite(PLL, EXT1_PPLL_POST_DIV, PLL->StorePostDiv);
	RHDRegWrite(PLL, EXT1_PPLL_CNTL, PLL->StoreControl);
    }
}

/*
 *
 */
static void
PLL2Restore(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    if (!PLL->Stored) {
	xf86DrvMsg(PLL->scrnIndex, X_ERROR, "%s: %s: trying to restore "
		   "uninitialized values.\n", __func__, PLL->Name);
	return;
    }

    if (PLL->StoreActive) {
	PLL2SetLow(PLL, PLL->StoreRefDiv, PLL->StoreFBDiv,
		   PLL->StorePostDiv, PLL->StoreControl);
    } else {
	PLL->Power(PLL, RHD_POWER_SHUTDOWN);

	/* lame attempt at at least restoring the old values */
	RHDRegWrite(PLL, EXT2_PPLL_REF_DIV, PLL->StoreRefDiv);
	RHDRegWrite(PLL, EXT2_PPLL_FB_DIV, PLL->StoreFBDiv);
	RHDRegWrite(PLL, EXT2_PPLL_POST_DIV, PLL->StorePostDiv);
	RHDRegWrite(PLL, EXT2_PPLL_CNTL, PLL->StoreControl);
    }
}

/* Some defaults for when we don't have this info */
#define RHD_PLL_DEFAULT_REFERENCE    27000 /* it's right there on the card */
#define RHD_PLL_DEFAULT_PLLOUT_MIN  600000 /* x6 otherwise clock is unstable */
#define RHD_PLL_DEFAULT_PLLOUT_MAX 1100000 /* Lowest value seen so far */
#define RHD_PLL_DEFAULT_MIN          16000 /* guess */
#define RHD_PLL_DEFAULT_MAX         400000 /* 400Mhz modes... hrm */

/*
 *
 */
void
RHDPLLsInit(RHDPtr rhdPtr)
{
    struct rhdPLL *PLL;
    CARD32 RefClock, InMin, InMax, OutMin, OutMax;
    AtomBIOSArg arg;
    int ret;

    RHDFUNC(rhdPtr);

    /* Retrieve the maximum internal PLL frequency */
    InMax = RHD_PLL_DEFAULT_PLLOUT_MAX;
#ifdef ATOM_BIOS
    if (rhdPtr->atomBIOS) {
	ret = RHDAtomBIOSFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			      GET_MAX_PIXEL_CLOCK_PLL_OUTPUT, &arg);
	if (ret == ATOM_SUCCESS) {
	    if (arg.val) {
		if ((arg.val * 10) < RHD_PLL_DEFAULT_PLLOUT_MAX)
		    xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING,
			       "Lower PLL maximum detected than the default.\n"
			       "Please contact the authors ASAP.\n");
		InMax = arg.val * 10;
	    }
	}
    } else
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Failed to retrieve the maximum"
		   " internal PLL clock from ATOM.\n");
#endif
    /* keep the defaults */
    RefClock = RHD_PLL_DEFAULT_REFERENCE;
    InMin = RHD_PLL_DEFAULT_PLLOUT_MIN;
    OutMin = RHD_PLL_DEFAULT_MIN;
    OutMax = RHD_PLL_DEFAULT_MAX;

    /* PLL1 */
    PLL = (struct rhdPLL *) xnfcalloc(sizeof(struct rhdPLL), 1);

    PLL->scrnIndex = rhdPtr->scrnIndex;
    PLL->Name = PLL_NAME_PLL1;
    PLL->Id = PLL_ID_PLL1;

    PLL->RefClock = RefClock;
    PLL->InMin = InMin;
    PLL->InMax = InMax;
    PLL->OutMin = OutMin;
    PLL->OutMax = OutMax;

    PLL->Valid = PLLValid;
    PLL->Set = PLL1Set;
    PLL->Power = PLL1Power;
    PLL->Save = PLL1Save;
    PLL->Restore = PLL1Restore;

    rhdPtr->PLLs[0] = PLL;

    /* PLL2 */
    PLL = (struct rhdPLL *) xnfcalloc(sizeof(struct rhdPLL), 1);

    PLL->scrnIndex = rhdPtr->scrnIndex;
    PLL->Name = PLL_NAME_PLL2;
    PLL->Id = PLL_ID_PLL2;

    PLL->RefClock = RefClock;
    PLL->InMin = InMin;
    PLL->InMax = InMax;
    PLL->OutMin = OutMin;
    PLL->OutMax = OutMax;

    PLL->Valid = PLLValid;
    PLL->Set = PLL2Set;
    PLL->Power = PLL2Power;
    PLL->Save = PLL2Save;
    PLL->Restore = PLL2Restore;

    rhdPtr->PLLs[1] = PLL;
}

/*
 *
 */
ModeStatus
RHDPLLValid(struct rhdPLL *PLL, CARD32 Clock)
{
    RHDFUNC(PLL);

    if (Clock < PLL->OutMin)
	return MODE_CLOCK_LOW;
    if (Clock > PLL->OutMax)
	return MODE_CLOCK_HIGH;

    if (PLL->Valid)
	return PLL->Valid(PLL, Clock);
    else
	return MODE_OK;
}


/*
 * Calculate the PLL parameters for a given dotclock.
 */
static Bool
PLLCalculate(struct rhdPLL *PLL, CARD32 PixelClock,
	     CARD16 *RefDivider, CARD16 *FBDivider, CARD8 *PostDivider)
{
/* limited by the number of bits available */
#define FB_DIV_LIMIT 1024 /* rv6x0 doesn't like 2048 */
#define REF_DIV_LIMIT 1024
#define POST_DIV_LIMIT 128

    CARD32 FBDiv, RefDiv, PostDiv, BestDiff = 0xFFFFFFFF;
    float Ratio;

    Ratio = ((float) PixelClock) / ((float) PLL->RefClock);

    for (PostDiv = 2; PostDiv < POST_DIV_LIMIT; PostDiv++) {
	CARD32 PLLIn = PixelClock * PostDiv;

	if (PLLIn < PLL->InMin)
	    continue;
	if (PLLIn > PLL->InMax)
	    break;

	for (RefDiv = 128; RefDiv >= 2; RefDiv--) {
	    int Diff;

	    FBDiv = (CARD32) ((Ratio * PostDiv * RefDiv) + 0.5);
	    if (!FBDiv)
		break;
			      if (FBDiv >= FB_DIV_LIMIT) continue;

	    Diff = abs(PixelClock - (FBDiv * PLL->RefClock) / (PostDiv * RefDiv));
	    xf86DrvMsg(PLL->scrnIndex, X_INFO, "RefDiv=%d Diff=%d\n",RefDiv,Diff);
	    if (Diff < BestDiff) {
		*FBDivider = FBDiv;
		*RefDivider = RefDiv;
		*PostDivider = PostDiv;
		BestDiff = Diff;
	    }

	    if (BestDiff == 0)
		break;
	}
	if (BestDiff == 0)
	    break;
    }

    if (BestDiff != 0xFFFFFFFF) {
	RHDDebug(PLL->scrnIndex, "PLL Calculation: %dkHz = "
		   "(((0x%X / 0x%X) * 0x%X) / 0x%X) (%dkHz off)\n",
		   (int) PixelClock, (unsigned int) PLL->RefClock, *RefDivider,
		   *FBDivider, *PostDivider, (int) BestDiff);
	xf86DrvMsg(PLL->scrnIndex, X_INFO, "PLL for %dkHz uses %dkHz internally.\n",
		   (int) PixelClock,
		   (int) (PLL->RefClock * *FBDivider) / *RefDivider);
	return TRUE;
    } else { /* Should never happen */
	xf86DrvMsg(PLL->scrnIndex, X_ERROR,
		   "%s: Failed to get a valid PLL setting for %dkHz\n",
		   __func__, (int) PixelClock);
	return FALSE;
    }
}

/*
 *
 */
void
RHDPLLSet(struct rhdPLL *PLL, CARD32 Clock)
{
    CARD16 RefDivider = 0, FBDivider = 0;
    CARD8 PostDivider = 0;

    RHDDebug(PLL->scrnIndex, "%s: Setting %s to %dkHz\n", __func__,
	     PLL->Name, Clock);

    if (PLLCalculate(PLL, Clock, &RefDivider, &FBDivider, &PostDivider)) {
	PLL->Set(PLL, RefDivider, FBDivider, 0, PostDivider);

	PLL->CurrentClock = Clock;
	PLL->Active = TRUE;
    } else
	xf86DrvMsg(PLL->scrnIndex, X_WARNING,
		   "%s: Not altering any settings.\n", __func__);
}

/*
 *
 */
void
RHDPLLPower(struct rhdPLL *PLL, int Power)
{
    RHDFUNC(PLL);

    if (PLL->Power)
	PLL->Power(PLL, Power);
}

/*
 *
 */
void
RHDPLLsPowerAll(RHDPtr rhdPtr, int Power)
{
    struct rhdPLL *PLL;

    RHDFUNC(rhdPtr);

    PLL = rhdPtr->PLLs[0];
    if (PLL->Power)
	PLL->Power(PLL, Power);

    PLL = rhdPtr->PLLs[1];
    if (PLL->Power)
	PLL->Power(PLL, Power);
}

/*
 *
 */
void
RHDPLLsShutdownInactive(RHDPtr rhdPtr)
{
    struct rhdPLL *PLL;

    RHDFUNC(rhdPtr);

    PLL = rhdPtr->PLLs[0];
    if (PLL->Power && !PLL->Active)
	PLL->Power(PLL, RHD_POWER_SHUTDOWN);

    PLL = rhdPtr->PLLs[1];
    if (PLL->Power && !PLL->Active)
	PLL->Power(PLL, RHD_POWER_SHUTDOWN);
}

/*
 *
 */
void
RHDPLLsSave(RHDPtr rhdPtr)
{
    struct rhdPLL *PLL;

    RHDFUNC(rhdPtr);

    PLL = rhdPtr->PLLs[0];
    if (PLL->Save)
	PLL->Save(PLL);

    PLL = rhdPtr->PLLs[1];
    if (PLL->Save)
	PLL->Save(PLL);
}

/*
 *
 */
void
RHDPLLsRestore(RHDPtr rhdPtr)
{
    struct rhdPLL *PLL;

    RHDFUNC(rhdPtr);

    PLL = rhdPtr->PLLs[0];
    if (PLL->Restore)
	PLL->Restore(PLL);

    PLL = rhdPtr->PLLs[1];
    if (PLL->Restore)
	PLL->Restore(PLL);
}

/*
 *
 */
void
RHDPLLsDestroy(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    xfree(rhdPtr->PLLs[0]);
    xfree(rhdPtr->PLLs[1]);
}

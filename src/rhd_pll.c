/*
 * Copyright 2007  Luc Verhaegen <lverhagen@novell.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_pll.h"
#include "rhd_regs.h"


#define PLL_CALIBRATE_WAIT 0x100000

/*
 *
 */
static ModeStatus
PLLValid(struct rhd_PLL *PLL, CARD32 Clock)
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
PLL1Calibrate(struct rhd_PLL *PLL)
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
PLL2Calibrate(struct rhd_PLL *PLL)
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
PLL1Power(struct rhd_PLL *PLL, int Power)
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
PLL2Power(struct rhd_PLL *PLL, int Power)
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
PLL1Set(struct rhd_PLL *PLL, CARD16 ReferenceDivider, CARD16 FeedbackDivider,
	CARD8 FeedbackDividerFraction, CARD8 PostDivider)
{
    RHDFUNC(PLL);

    RHDRegWrite(PLL, EXT1_PPLL_REF_DIV_SRC, 0x01); /* XTAL */
    RHDRegWrite(PLL, EXT1_PPLL_POST_DIV_SRC, 0x00); /* source = reference */
    RHDRegWrite(PLL, EXT1_PPLL_UPDATE_LOCK, 0x01); /* lock */

    RHDRegWrite(PLL, EXT1_PPLL_REF_DIV, ReferenceDivider);
    RHDRegMask(PLL, EXT1_PPLL_FB_DIV,
	       (FeedbackDivider << 16) | (FeedbackDividerFraction >> 24), 0xFFFF000F);
    RHDRegMask(PLL, EXT1_PPLL_POST_DIV, PostDivider, 0x007F);

    RHDRegMask(PLL, EXT1_PPLL_UPDATE_CNTL, 0x00010000, 0x00010000); /* no autoreset */
    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x04); /* don't bypass calibration */
    RHDRegWrite(PLL, EXT1_PPLL_UPDATE_LOCK, 0); /* unlock */
    RHDRegMask(PLL, EXT1_PPLL_UPDATE_CNTL, 0, 0x01); /* we're done updating! */

    /* Set up gain, charge pump, loop filter and current bias.
     * For R500, this is done in atombios by ASIC_RegistersInit */
    switch (RHDPTR(xf86Screens[PLL->scrnIndex])->ChipSet) {
    case RHD_R600:
	RHDRegWrite(PLL, EXT1_PPLL_CNTL, 0x01130704);
	break;
    case RHD_RV610:
    case RHD_RV630:
	RHDRegMask(PLL, EXT1_PPLL_CNTL, 0x159F8704, 0xFFFFE000);
	if (FeedbackDivider >= 0x6C)
	    RHDRegMask(PLL, EXT1_PPLL_CNTL, 0x00001EC7, 0x00001FFF);
	else if (FeedbackDivider >= 0x49)
	    RHDRegMask(PLL, EXT1_PPLL_CNTL, 0x00001B87, 0x00001FFF);
	else
	    RHDRegMask(PLL, EXT1_PPLL_CNTL, 0x00000704, 0x00001FFF);
	break;
    default:
	break;
    }

    RHDRegMask(PLL, P1PLL_CNTL, 0, 0x02); /* Powah */
    usleep(2);

    PLL1Calibrate(PLL);

    RHDRegWrite(PLL, EXT1_PPLL_POST_DIV_SRC, 0x01); /* source is PLL itself */
}

/*
 *
 */
static void
PLL2Set(struct rhd_PLL *PLL, CARD16 ReferenceDivider, CARD16 FeedbackDivider,
	CARD8 FeedbackDividerFraction, CARD8 PostDivider)
{
    RHDFUNC(PLL);

    RHDRegWrite(PLL, EXT2_PPLL_REF_DIV_SRC, 0x01); /* XTAL */
    RHDRegWrite(PLL, EXT2_PPLL_POST_DIV_SRC, 0x00); /* source = reference */
    RHDRegWrite(PLL, EXT2_PPLL_UPDATE_LOCK, 0x01); /* lock */

    RHDRegWrite(PLL, EXT2_PPLL_REF_DIV, ReferenceDivider);
    RHDRegMask(PLL, EXT2_PPLL_FB_DIV,
	       (FeedbackDivider << 16) | (FeedbackDividerFraction >> 24), 0xFFFF000F);
    RHDRegMask(PLL, EXT2_PPLL_POST_DIV, PostDivider, 0x007F);

    RHDRegMask(PLL, EXT2_PPLL_UPDATE_CNTL, 0x00010000, 0x00010000); /* no autoreset */
    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x04); /* don't bypass calibration */
    RHDRegWrite(PLL, EXT2_PPLL_UPDATE_LOCK, 0); /* unlock */
    RHDRegMask(PLL, EXT2_PPLL_UPDATE_CNTL, 0, 0x01); /* we're done updating! */

    /* Set up gain, charge pump, loop filter and current bias.
     * For R500, this is done in atombios by ASIC_RegistersInit */
    switch (RHDPTR(xf86Screens[PLL->scrnIndex])->ChipSet) {
    case RHD_R600:
	RHDRegWrite(PLL, EXT2_PPLL_CNTL, 0x01130704);
	break;
    case RHD_RV610:
    case RHD_RV630:
	RHDRegMask(PLL, EXT2_PPLL_CNTL, 0x159F8704, 0xFFFFE000);
	if (FeedbackDivider >= 0x6C)
	    RHDRegMask(PLL, EXT2_PPLL_CNTL, 0x00001EC7, 0x00001FFF);
	else if (FeedbackDivider >= 0x49)
	    RHDRegMask(PLL, EXT2_PPLL_CNTL, 0x00001B87, 0x00001FFF);
	else
	    RHDRegMask(PLL, EXT2_PPLL_CNTL, 0x00000704, 0x00001FFF);
	break;
    default:
	break;
    }

    RHDRegMask(PLL, P2PLL_CNTL, 0, 0x02); /* Powah */
    usleep(2);

    PLL2Calibrate(PLL);

    RHDRegWrite(PLL, EXT2_PPLL_POST_DIV_SRC, 0x01); /* source is PLL itself */
}

/*
 *
 */
static void
PLL1Save(struct rhd_PLL *PLL)
{
    RHDFUNC(PLL);

    PLL->Store_Active = !(RHDRegRead(PLL, P1PLL_CNTL) & 0x03);
    PLL->Store_RefDivider = RHDRegRead(PLL, EXT1_PPLL_REF_DIV) & 0x3FF;
    PLL->Store_FBDivider = (RHDRegRead(PLL, EXT1_PPLL_FB_DIV) >> 16) & 0x7FF;
    PLL->Store_FBDividerFraction = RHDRegRead(PLL, EXT1_PPLL_FB_DIV) & 0x0F;
    PLL->Store_PostDivider = RHDRegRead(PLL, EXT1_PPLL_POST_DIV) & 0x7F;

    PLL->Stored = TRUE;
}

/*
 *
 */
static void
PLL2Save(struct rhd_PLL *PLL)
{
    RHDFUNC(PLL);

    PLL->Store_Active = !(RHDRegRead(PLL, P2PLL_CNTL) & 0x03);
    PLL->Store_RefDivider = RHDRegRead(PLL, EXT2_PPLL_REF_DIV) & 0x3FF;
    PLL->Store_FBDivider = (RHDRegRead(PLL, EXT2_PPLL_FB_DIV) >> 16) & 0x7FF;
    PLL->Store_FBDividerFraction = RHDRegRead(PLL, EXT2_PPLL_FB_DIV) & 0x0F;
    PLL->Store_PostDivider = RHDRegRead(PLL, EXT2_PPLL_POST_DIV) & 0x7F;

    PLL->Stored = TRUE;
}

static void
PLLRestore(struct rhd_PLL *PLL)
{
    RHDFUNC(PLL);

    if (!PLL->Stored) {
	xf86DrvMsg(PLL->scrnIndex, X_ERROR, "%s: %s: trying to restore "
		   "uninitialized values.\n", __func__, PLL->Name);
	return;
    }

    if (PLL->Store_Active) {
	PLL->Set(PLL, PLL->Store_RefDivider, PLL->Store_FBDivider,
		 PLL->Store_FBDividerFraction, PLL->Store_PostDivider);
	/* RHDRegWrite(rhdPtr, PCLK_CRTC1_CNTL, restore->PCLK_CRTC1_Control); */
	/* RHDRegWrite(rhdPtr, PCLK_CRTC2_CNTL, restore->PCLK_CRTC2_Control); */
    } else
	PLL->Power(PLL, RHD_POWER_SHUTDOWN);
}

/*
 *
 */
void
RHDPLLsInit(RHDPtr rhdPtr)
{
    struct rhd_PLL *PLL;

    RHDFUNC(rhdPtr);

    /* PLL1 */
    PLL = (struct rhd_PLL *) xnfcalloc(sizeof(struct rhd_PLL), 1);

    PLL->scrnIndex = rhdPtr->scrnIndex;
    PLL->Name = PLL_NAME_PLL1;
    PLL->Id = PLL_ID_PLL1;

    /* Fill in ATOM values here */
    PLL->RefClock = 27000;
    PLL->InMin = 100000 * 6; /* x6 otherwise clock is unstable */
    PLL->InMax = 1350000;
    PLL->OutMin = 16000; /* guess */
    PLL->OutMax = 400000;

    PLL->Valid = PLLValid;
    PLL->Set = PLL1Set;
    PLL->Power = PLL1Power;
    PLL->Save = PLL1Save;
    PLL->Restore = PLLRestore;

    rhdPtr->PLLs[0] = PLL;

    /* PLL2 */
    PLL = (struct rhd_PLL *) xnfcalloc(sizeof(struct rhd_PLL), 1);

    PLL->scrnIndex = rhdPtr->scrnIndex;
    PLL->Name = PLL_NAME_PLL2;
    PLL->Id = PLL_ID_PLL2;

    PLL->RefClock = 27000;
    PLL->InMin = 100000 * 6; /* x6 otherwise clock is unstable */
    PLL->InMax = 1350000;
    PLL->OutMin = 16000; /* guess */
    PLL->OutMax = 400000;

    PLL->Valid = PLLValid;
    PLL->Set = PLL2Set;
    PLL->Power = PLL2Power;
    PLL->Save = PLL2Save;
    PLL->Restore = PLLRestore;

    rhdPtr->PLLs[1] = PLL;
}

/*
 *
 */
ModeStatus
RHDPLLValid(struct rhd_PLL *PLL, CARD32 Clock)
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
PLLCalculate(struct rhd_PLL *PLL, CARD32 PixelClock,
	     CARD16 *RefDivider, CARD16 *FBDivider, CARD8 *PostDivider)
{
/* limited by the number of bits available */
#define FB_DIV_LIMIT 2048
#define REF_DIV_LIMIT 1024
#define POST_DIV_LIMIT 128

    CARD32 FBDiv, RefDiv, PostDiv, BestDiff = 0xFFFFFFFF;
    float Ratio;

    Ratio = ((float) PixelClock) / ((float) PLL->RefClock);

    for (PostDiv = POST_DIV_LIMIT - 1; PostDiv > 1; PostDiv--) {
	CARD32 PLLIn = PixelClock * PostDiv;

	if (PLLIn < PLL->InMin)
	    break;
	if (PLLIn > PLL->InMax)
	    continue;

	for (RefDiv = 1; RefDiv < REF_DIV_LIMIT; RefDiv++) {
	    int Diff;

	    FBDiv = (float) (Ratio * PostDiv * RefDiv) + 0.5;
	    if (!FBDiv)
		continue;
	    if (FBDiv >= FB_DIV_LIMIT)
		break;

	    Diff = PixelClock - (FBDiv * PLL->RefClock) / (PostDiv * RefDiv);
	    if (Diff < 0)
		Diff *= -1;

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
RHDPLLSet(struct rhd_PLL *PLL, CARD32 Clock)
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
RHDPLLPower(struct rhd_PLL *PLL, int Power)
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
    struct rhd_PLL *PLL;

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
    struct rhd_PLL *PLL;

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
    struct rhd_PLL *PLL;

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
    struct rhd_PLL *PLL;

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

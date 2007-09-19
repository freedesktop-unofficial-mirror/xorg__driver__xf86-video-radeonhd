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

/*
 * Deals with the Shared LVDS/TMDS encoder.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_crtc.h"
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_regs.h"

struct rhdLVTMAPrivate {
    Bool DualLink;
    Bool LVDS24Bit;
    Bool FPDI; /* LDI otherwise */
    CARD16 TXClockPattern;

    CARD32 MacroControl;

    /* Power timing for LVDS */
    CARD16 PowerRefDiv;
    CARD16 BlonRefDiv;
    CARD16 PowerDigToDE;
    CARD16 PowerDEToBL;
    CARD16 OffDelay;

    Bool Stored;

    CARD32 StoreControl;
    CARD32 StoreSourceSelect;
    CARD32 StoreBitDepthControl;
    CARD32 StoreDataSynchronisation;
    CARD32 StorePWRSEQRefDiv;
    CARD32 StorePWRSEQDelay1;
    CARD32 StorePWRSEQDelay2;
    CARD32 StorePWRSEQControl;
    CARD32 StorePWRSEQState;
    CARD32 StoreLVDSDataControl;
    CARD32 StoreMode;
    CARD32 StoreTxEnable;
    CARD32 StoreMacroControl;
    CARD32 StoreTXControl;
};

/*
 *
 */
static ModeStatus
LVDSModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDFUNC(Output);

    return MODE_OK;
}

/*
 *
 */
static void
LVDSSet(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;

    RHDFUNC(Output);

    RHDRegMask(Output, LVTMA_CNTL, 0x00000001, 0x00000001); /* enable */
    usleep(20);

    RHDRegWrite(Output, LVTMA_MODE, 0); /* set to LVDS */

    /* Select CRTC, select syncA, no stereosync */
    RHDRegMask(Output, LVTMA_SOURCE_SELECT, Output->Crtc->Id, 0x00010101);

    if (Private->LVDS24Bit) { /* 24bits */
	RHDRegMask(Output, LVTMA_LVDS_DATA_CNTL, 0x00000001, 0x00000001); /* enable 24bits */
	RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x00100000, 0x00100000); /* dithering bit depth = 24 */

	if (Private->FPDI) /* FPDI? */
	    RHDRegMask(Output, LVTMA_LVDS_DATA_CNTL, 0x00000010, 0x00000010); /* 24 bit format: FPDI or LDI? */
	else
	    RHDRegMask(Output, LVTMA_LVDS_DATA_CNTL, 0, 0x00000010);
    } else {
	RHDRegMask(Output, LVTMA_LVDS_DATA_CNTL, 0, 0x00000001); /* disable 24bits */
	RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0, 0x00100101); /* dithering bit depth != 24 */
    }

#if 0
    if (LVDS_Info->LVDS_Misc & 0x40) { /* enable dithering? */
	if (LVDS_Info->LVDS_Misc & 0x0C) /* no idea. */
	    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x01000000, 0x01000000); /* grey level 4 */
	else
	    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0, 0x01000000); /* grey level 2 */

	/* enable temporal bit depth reduction */
	RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x00010000, 0x00010000);
    } else
	RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0, 0x00010101);
#endif
    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x01010100, 0x01010101);

    /* reset the temporal dithering */
    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x04000000, 0x04000000);
    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0, 0x04000000);

    /* go for RGB 4:4:4 RGB/YCbCr  */
    RHDRegMask(Output, LVTMA_CNTL, 0, 0x00010000);

    if (Private->DualLink)
	RHDRegMask(Output, LVTMA_CNTL, 0x01000000, 0x01000000);
    else
	RHDRegMask(Output, LVTMA_CNTL, 0, 0x01000000);

    /* PLL and TX voltages */
    RHDRegWrite(Output, LVTMA_MACRO_CONTROL, Private->MacroControl);

    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0x00000010, 0x00000010); /* use pclk_lvtma_direct */
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0, 0xCC000000);
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, Private->TXClockPattern << 16, 0x03FF0000);
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0x00000001, 0x00000001); /* enable PLL */
    usleep(20);

    /* reset transmitter */
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
    usleep(2);
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0, 0x00000002);
    usleep(20);

    /* start data synchronisation */
    RHDRegMask(Output, LVTMA_DATA_SYNCHRONIZATION, 0x00000001, 0x00000001);
    RHDRegMask(Output, LVTMA_DATA_SYNCHRONIZATION, 0x00000100, 0x00000100); /* reset */
    usleep(2);
    RHDRegMask(Output, LVTMA_DATA_SYNCHRONIZATION, 0, 0x00000100);
}

/*
 *
 */
static void
LVDSPWRSEQInit(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;
    CARD32 tmp = 0;

    tmp = (Private->PowerDigToDE * 10) >> 2;
    RHDRegMask(Output, LVTMA_PWRSEQ_DELAY1, tmp, 0x000000FF);
    RHDRegMask(Output, LVTMA_PWRSEQ_DELAY1, tmp << 24, 0xFF000000);

    tmp =  (Private->PowerDEToBL * 10) >> 2;
    RHDRegMask(Output, LVTMA_PWRSEQ_DELAY1, tmp << 8, 0x0000FF00);
    RHDRegMask(Output, LVTMA_PWRSEQ_DELAY1, tmp << 16, 0x00FF0000);

    RHDRegWrite(Output, LVTMA_PWRSEQ_DELAY2, Private->OffDelay >> 2);
    RHDRegWrite(Output, LVTMA_PWRSEQ_REF_DIV,
		Private->PowerRefDiv | (Private->BlonRefDiv << 16));

    /* Enable power sequencer and allow it to override everything */
    RHDRegMask(Output, LVTMA_PWRSEQ_CNTL, 0x0000000D, 0x0000000D);

    /* give full control to the sequencer */
    RHDRegMask(Output, LVTMA_PWRSEQ_CNTL, 0, 0x02020200);
}

/*
 *
 */
static void
LVDSEnable(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;
    CARD32 tmp = 0;
    int i;

    RHDFUNC(Output);

    LVDSPWRSEQInit(Output);

    /* set up the transmitter */
    RHDRegMask(Output, LVTMA_TRANSMITTER_ENABLE, 0x0000001E, 0x0000001E);
    if (Private->LVDS24Bit) /* 24bit ? */
	RHDRegMask(Output, LVTMA_TRANSMITTER_ENABLE, 0x00000020, 0x00000020);

    if (Private->DualLink) {
	RHDRegMask(Output, LVTMA_TRANSMITTER_ENABLE, 0x00001E00, 0x00001E00);

	if (Private->LVDS24Bit)
	    RHDRegMask(Output, LVTMA_TRANSMITTER_ENABLE, 0x00002000, 0x00002000);
    }

    RHDRegMask(Output, LVTMA_PWRSEQ_CNTL, 0x00000010, 0x00000010);

    for (i = 0; i <= Private->OffDelay; i++) {
	usleep(1000);

	tmp = (RHDRegRead(Output, LVTMA_PWRSEQ_STATE) >> 8) & 0x0F;
	if (tmp == 4)
	    break;
    }

    if (i == Private->OffDelay) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR, "%s: failed to reach "
		   "POWERUP_DONE state after %d loops (%d)\n",
		   __func__, i, (int) tmp);
    }
}

/*
 *
 */
static void
LVDSDisable(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;
    CARD32 tmp = 0;
    int i;

    RHDFUNC(Output);

    if (!(RHDRegRead(Output, LVTMA_PWRSEQ_CNTL) & 0x00000010))
	return;

    LVDSPWRSEQInit(Output);

    RHDRegMask(Output, LVTMA_PWRSEQ_CNTL, 0, 0x00000010);

    for (i = 0; i <= Private->OffDelay; i++) {
	usleep(1000);

	tmp = (RHDRegRead(Output, LVTMA_PWRSEQ_STATE) >> 8) & 0x0F;
	if (tmp == 9)
	    break;
    }

    if (i == Private->OffDelay) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR, "%s: failed to reach "
		   "POWERDOWN_DONE state after %d loops (%d)\n",
		   __func__, i, (int) tmp);
    }

    RHDRegMask(Output, LVTMA_TRANSMITTER_ENABLE, 0, 0x0000FFFF);
}

#if 0
/*
 *
 */
static void
LVDSShutdown(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002); /* PLL in reset */
    RHDRegMask(Output, LVTMA_TRANSMITTER_CONTROL, 0, 0x00000001); /* disable LVDS */
    RHDRegMask(Output, LVTMA_DATA_SYNCHRONIZATION, 0, 0x00000001);
    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0x04000000, 0x04000000); /* reset temp dithering */
    RHDRegMask(Output, LVTMA_BIT_DEPTH_CONTROL, 0, 0x00111111); /* disable all dithering */
    RHDRegWrite(Output, LVTMA_CNTL, 0); /* disable */
}
#endif

/*
 *
 */
static void
LVDSPower(struct rhdOutput *Output, int Power)
{
    RHDFUNC(Output);

    switch (Power) {
    case RHD_POWER_ON:
	LVDSEnable(Output);
	break;
    case RHD_POWER_RESET:
	/*	LVDSDisable(Output);
		break;*/
    case RHD_POWER_SHUTDOWN:
    default:
	LVDSDisable(Output);
	/* LVDSShutdown(Output); */
	break;
    }
    return;
}

/*
 *
 */
static void
LVTMASave(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;

    RHDFUNC(Output);

    Private->StoreControl = RHDRegRead(Output, LVTMA_CNTL);
    Private->StoreSourceSelect = RHDRegRead(Output,  LVTMA_SOURCE_SELECT);
    Private->StoreBitDepthControl = RHDRegRead(Output, LVTMA_BIT_DEPTH_CONTROL);
    Private->StoreDataSynchronisation = RHDRegRead(Output, LVTMA_DATA_SYNCHRONIZATION);
    Private->StorePWRSEQRefDiv = RHDRegRead(Output, LVTMA_PWRSEQ_REF_DIV);
    Private->StorePWRSEQDelay1 = RHDRegRead(Output, LVTMA_PWRSEQ_DELAY1);
    Private->StorePWRSEQDelay2 = RHDRegRead(Output, LVTMA_PWRSEQ_DELAY2);
    Private->StorePWRSEQControl = RHDRegRead(Output, LVTMA_PWRSEQ_CNTL);
    Private->StorePWRSEQState = RHDRegRead(Output, LVTMA_PWRSEQ_STATE);
    Private->StoreLVDSDataControl = RHDRegRead(Output, LVTMA_LVDS_DATA_CNTL);
    Private->StoreMode = RHDRegRead(Output, LVTMA_MODE);
    Private->StoreTxEnable = RHDRegRead(Output, LVTMA_TRANSMITTER_ENABLE);
    Private->StoreMacroControl = RHDRegRead(Output, LVTMA_MACRO_CONTROL);
    Private->StoreTXControl = RHDRegRead(Output, LVTMA_TRANSMITTER_CONTROL);

    Private->Stored = TRUE;
}

/*
 * This needs to reset things like the temporal dithering and the TX appropriately.
 * Currently it's a dumb register dump.
 */
static void
LVTMARestore(struct rhdOutput *Output)
{
    struct rhdLVTMAPrivate *Private = (struct rhdLVTMAPrivate *) Output->Private;

    RHDFUNC(Output);

    if (!Private->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    RHDRegWrite(Output, LVTMA_CNTL, Private->StoreControl);
    RHDRegWrite(Output, LVTMA_SOURCE_SELECT, Private->StoreSourceSelect);
    RHDRegWrite(Output, LVTMA_BIT_DEPTH_CONTROL,  Private->StoreBitDepthControl);
    RHDRegWrite(Output, LVTMA_DATA_SYNCHRONIZATION, Private->StoreDataSynchronisation);
    RHDRegWrite(Output, LVTMA_PWRSEQ_REF_DIV, Private->StorePWRSEQRefDiv);
    RHDRegWrite(Output, LVTMA_PWRSEQ_DELAY1, Private->StorePWRSEQDelay1);
    RHDRegWrite(Output, LVTMA_PWRSEQ_DELAY2,  Private->StorePWRSEQDelay2);
    RHDRegWrite(Output, LVTMA_PWRSEQ_CNTL, Private->StorePWRSEQControl);
    RHDRegWrite(Output, LVTMA_PWRSEQ_STATE, Private->StorePWRSEQState);
    RHDRegWrite(Output, LVTMA_LVDS_DATA_CNTL, Private->StoreLVDSDataControl);
    RHDRegWrite(Output, LVTMA_MODE, Private->StoreMode);
    RHDRegWrite(Output, LVTMA_TRANSMITTER_ENABLE, Private->StoreTxEnable);
    RHDRegWrite(Output, LVTMA_MACRO_CONTROL, Private->StoreMacroControl);
    RHDRegWrite(Output, LVTMA_TRANSMITTER_CONTROL,  Private->StoreTXControl);
}

/*
 *
 */
static void
LVTMADestroy(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!Output->Private)
	return;

    xfree(Output->Private);
    Output->Private = NULL;
}

/*
 *
 */
struct rhdOutput *
RHDLVTMAInit(RHDPtr rhdPtr, CARD8 Type)
{
    struct rhdOutput *Output;
    struct rhdLVTMAPrivate *Private;

    RHDFUNC(rhdPtr);

    /* Stop everything except LVDS at this time */
    if (Type != RHD_CONNECTOR_PANEL) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: unhandled connector type:"
		   " %d\n", __func__, Type);
	return NULL;
    }

    if (rhdPtr->ChipSet != RHD_M56) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: any other device than the"
		   " M56 is still unsupported.\n", __func__);
	return NULL;
    }

    Output = xnfcalloc(sizeof(struct rhdOutput), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->Name = "LVDS/TMDS";
    Output->Id = RHD_OUTPUT_LVTMA;

    Output->Sense = NULL;

    Output->ModeValid = LVDSModeValid;
    Output->Mode = LVDSSet;
    Output->Power = LVDSPower;
    Output->Save = LVTMASave;
    Output->Restore = LVTMARestore;
    Output->Destroy = LVTMADestroy;

    Private = xnfcalloc(sizeof(struct rhdLVTMAPrivate), 1);

    /* TODO: Retrieve from atombios -- impossible for all */
    Private->PowerDigToDE = rhdPtr->Card->Lvds.PowerDigToDE;
    Private->PowerDEToBL = rhdPtr->Card->Lvds.PowerDEToBL;
    Private->OffDelay = rhdPtr->Card->Lvds.OffDelay;
    Private->PowerRefDiv = rhdPtr->Card->Lvds.PowerRefDiv;
    Private->BlonRefDiv = rhdPtr->Card->Lvds.BlonRefDiv;

    /* Not in atombios tables afaik */
    Private->TXClockPattern = 0x0063;
    Private->MacroControl =  0x0C720407;

    /* TODO: Retrieve from atombios */
    Private->DualLink = RHDRegRead(rhdPtr, LVTMA_CNTL) & 0x01000000;
    Private->LVDS24Bit = RHDRegRead(rhdPtr, LVTMA_LVDS_DATA_CNTL) & 0x00000001;
    Private->FPDI = RHDRegRead(rhdPtr, LVTMA_LVDS_DATA_CNTL) & 0x00000001;

    if (Private->LVDS24Bit)
	xf86DrvMsg(rhdPtr->scrnIndex, X_PROBED,
		   "Detected a 24bit %s, %s link panel.\n",
		   Private->DualLink ? "dual" : "single",
		   Private->FPDI ? "FPDI": "LDI");
    else
	xf86DrvMsg(rhdPtr->scrnIndex, X_PROBED,
		   "Detected a 18bit %s link panel.\n",
		   Private->DualLink ? "dual" : "single");

    Output->Private = Private;

    return Output;
}
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

/*
 * Deals with the Primary TMDS device (TMDSA).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_output.h"
#include "rhd_regs.h"

struct rhd_TMDS_Private {
    Bool Stored;

    CARD32 StoreControl;
    CARD32 StoreSource;
    CARD32 StoreFormat;
    CARD32 StoreForce;
    CARD32 StoreReduction;
    CARD32 StoreDCBalancer;
    CARD32 StoreDataSynchro;
    CARD32 StoreTXEnable;
    CARD32 StoreMacro;
    CARD32 StoreTXControl;
    CARD32 StoreTXAdjust;
};

/*
 *
 */
static Bool
TMDSASense(struct rhd_Output *Output)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[Output->scrnIndex]);
    CARD32 Enable, Control, Detect;
    Bool ret;

    RHDFUNC(Output);

    Enable = RHDRegRead(Output, TMDSA_TRANSMITTER_ENABLE);
    Control = RHDRegRead(Output, TMDSA_TRANSMITTER_CONTROL);
    Detect = RHDRegRead(Output, TMDSA_LOAD_DETECT);

    /* r500 needs a tiny bit more work :) */
    if (rhdPtr->ChipSet < RHD_R600) {
	RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0x00000003, 0x00000003);
	RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0x00000001, 0x00000003);
    }

    RHDRegMask(Output, TMDSA_LOAD_DETECT, 0x00000001, 0x00000001);
    usleep(1);
    ret = RHDRegRead(Output, TMDSA_LOAD_DETECT) & 0x00000010;
    RHDRegMask(Output, TMDSA_LOAD_DETECT, Detect, 0x00000001);

    if (rhdPtr->ChipSet < RHD_R600) {
	RHDRegWrite(Output, TMDSA_TRANSMITTER_ENABLE, Enable);
	RHDRegWrite(Output, TMDSA_TRANSMITTER_CONTROL, Control);
    }

    RHDDebug(Output->scrnIndex, "%s: %s\n", __func__,
	     ret ? "Attached" : "Disconnected");

    return ret;
}

/*
 * TODO: use only when atombios tables fail us.
 */
static void
TMDSAVoltageControl(RHDPtr rhdPtr)
{
    CARD16 id = rhdPtr->PciInfo->chipType;

    switch (id) {
    case 0x7104: /* R520 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00C00414);
	break;
    case 0x7147: /* RV505 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00C00418);
	break;
    case 0x7146: /* RV515 */
    case 0x71C1: /* RV535 */
    case 0x7280: /* RV570 */
    case 0x7288: /* RV570 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00C0041F);
	break;
    case 0x7152: /* RV515 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00A00415);
	break;
    case 0x71C2: /* RV530 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00A00416);
	break;
    case 0x71D2: /* RV530 */
    case 0x7249: /* R580 */
	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00A00513);
	break;
    case 0x9400: /* R600 */
 	RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00910519);
	/* RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00910419); */ /* HDMI */
	/* RHDRegWrite(rhdPtr, TMDSA_MACRO_CONTROL, 0x00610519); */ /* DVI, PLL <= 7500 */
	break;
    case 0x94C1: /* RV610 */
    case 0x94C3: /* RV610 */
	RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x00010416);
	RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x00010308);

	/* HDMI: */
	/* RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x0001040A);
	   RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x00000008); */

	/* DVI, PLL <= 75Mhz */
	/* RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x00030410);
	   RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x0000003); */
	break;
    case 0x9588: /* RV630 */
    case 0x9589: /* RV630 */
	RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x00010416);
	RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x00010388);

	/* HDMI: */
	/* RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x0001040A);
	   RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x00000088); */

	/* DVI, PLL <= 75Mhz */
	/* RHDRegWrite(rhdPtr, TMDSA_PLL_ADJUST, 0x00030410);
	   RHDRegWrite(rhdPtr, TMDSA_TRANSMITTER_ADJUST, 0x00000033); */
	break;
    default:
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		   "%s: unhandled chipset: 0x%04X.\n", __func__, id);
	break;
    }
}

/*
 *
 */
static void
TMDSASet(struct rhd_Output *Output)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[Output->scrnIndex]);

    RHDFUNC(Output);

    /* Clear out some HPD events first: this should be under driver control. */
    RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0, 0x0000000C);
    RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0, 0x00070000);
    RHDRegMask(Output, TMDSA_CNTL, 0, 0x00000010);

    /* Disable the transmitter */
    RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0, 0x00001D1F);

    /* Disable bit reduction and reset temporal dither */
    RHDRegMask(Output, TMDSA_BIT_DEPTH_CONTROL, 0, 0x00010101);
    if (rhdPtr->ChipSet < RHD_R600) {
	RHDRegMask(Output, TMDSA_BIT_DEPTH_CONTROL, 0x04000000, 0x04000000);
	usleep(2);
	RHDRegMask(Output, TMDSA_BIT_DEPTH_CONTROL, 0, 0x04000000);
    } else {
	RHDRegMask(Output, TMDSA_BIT_DEPTH_CONTROL, 0x02000000, 0x02000000);
	usleep(2);
	RHDRegMask(Output, TMDSA_BIT_DEPTH_CONTROL, 0, 0x02000000);
    }

    /* reset phase on vsync and use RGB */
    RHDRegMask(Output, TMDSA_CNTL, 0x00001000, 0x00011000);

    /* Select CRTC, select syncA, no stereosync */
    RHDRegMask(Output, TMDSA_SOURCE_SELECT, Output->Crtc, 0x00010101);

    /* Single link, for now */
    RHDRegWrite(Output, TMDSA_COLOR_FORMAT, 0);
    RHDRegMask(Output, TMDSA_CNTL, 0, 0x01000000);

    /* Disable force data */
    RHDRegMask(Output, TMDSA_FORCE_OUTPUT_CNTL, 0, 0x00000001);

    /* DC balancer enable */
    RHDRegMask(Output, TMDSA_DCBALANCER_CONTROL, 0x00000001, 0x00000001);

    TMDSAVoltageControl(rhdPtr);

    /* use IDCLK */
    RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000010);

    /* reset transmitter */
    RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
    usleep(2);
    RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000002);
    usleep(20);

    /* restart data synchronisation */
    if (rhdPtr->ChipSet < RHD_R600) {
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R500, 0x00000001, 0x00000001);
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R500, 0x00000100, 0x00000100);
	usleep(2);
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R500, 0, 0x00000001);
    } else {
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R600, 0x00000001, 0x00000001);
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R600, 0x00000100, 0x00000100);
	usleep(2);
	RHDRegMask(Output, TMDSA_DATA_SYNCHRONIZATION_R600, 0, 0x00000001);
    }
}

/*
 *
 */
static void
TMDSAPower(struct rhd_Output *Output, int Power)
{
    RHDFUNC(Output);

    switch (Power) {
    case RHD_POWER_ON:
	RHDRegMask(Output, TMDSA_CNTL, 0x00000001, 0x00000001);
	RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0x0000001F, 0x0000001F);
	RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0x00000001, 0x00000001);
	usleep(2);
	RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000002);
	return;
    case RHD_POWER_RESET:
	RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0, 0x0000001F);
	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0x00000002, 0x00000002);
	usleep(2);
	RHDRegMask(Output, TMDSA_TRANSMITTER_CONTROL, 0, 0x00000001);
	RHDRegMask(Output, TMDSA_TRANSMITTER_ENABLE, 0, 0x0000001F);
	RHDRegMask(Output, TMDSA_CNTL, 0, 0x00000001);
	return;
    }
}

/*
 *
 */
static void
TMDSASave(struct rhd_Output *Output)
{
    int ChipSet = RHDPTR(xf86Screens[Output->scrnIndex])->ChipSet;
    struct rhd_TMDS_Private *Private = (struct rhd_TMDS_Private *) Output->Private;

    RHDFUNC(Output);

    Private->StoreControl = RHDRegRead(Output, TMDSA_CNTL);
    Private->StoreSource = RHDRegRead(Output, TMDSA_SOURCE_SELECT);
    Private->StoreFormat = RHDRegRead(Output, TMDSA_COLOR_FORMAT);
    Private->StoreForce = RHDRegRead(Output, TMDSA_FORCE_OUTPUT_CNTL);
    Private->StoreReduction = RHDRegRead(Output, TMDSA_BIT_DEPTH_CONTROL);
    Private->StoreDCBalancer = RHDRegRead(Output, TMDSA_DCBALANCER_CONTROL);

    if (ChipSet < RHD_R600)
	Private->StoreDataSynchro = RHDRegRead(Output, TMDSA_DATA_SYNCHRONIZATION_R500);
    else
	Private->StoreDataSynchro = RHDRegRead(Output, TMDSA_DATA_SYNCHRONIZATION_R600);

    Private->StoreTXEnable = RHDRegRead(Output, TMDSA_TRANSMITTER_ENABLE);
    Private->StoreMacro = RHDRegRead(Output, TMDSA_MACRO_CONTROL);
    Private->StoreTXControl = RHDRegRead(Output, TMDSA_TRANSMITTER_CONTROL);

    if (ChipSet >= RHD_RV610)
	Private->StoreTXAdjust = RHDRegRead(Output, TMDSA_TRANSMITTER_ADJUST);

    Private->Stored = TRUE;
}

/*
 *
 */
static void
TMDSARestore(struct rhd_Output *Output)
{
    int ChipSet = RHDPTR(xf86Screens[Output->scrnIndex])->ChipSet;
    struct rhd_TMDS_Private *Private = (struct rhd_TMDS_Private *) Output->Private;

    RHDFUNC(Output);

    if (!Private->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    RHDRegWrite(Output, TMDSA_CNTL, Private->StoreControl);
    RHDRegWrite(Output, TMDSA_SOURCE_SELECT, Private->StoreSource);
    RHDRegWrite(Output, TMDSA_COLOR_FORMAT, Private->StoreFormat);
    RHDRegWrite(Output, TMDSA_FORCE_OUTPUT_CNTL, Private->StoreForce);
    RHDRegWrite(Output, TMDSA_BIT_DEPTH_CONTROL, Private->StoreReduction);
    RHDRegWrite(Output, TMDSA_DCBALANCER_CONTROL, Private->StoreDCBalancer);

    if (ChipSet < RHD_R600)
	RHDRegWrite(Output, TMDSA_DATA_SYNCHRONIZATION_R500, Private->StoreDataSynchro);
    else
	RHDRegWrite(Output, TMDSA_DATA_SYNCHRONIZATION_R600, Private->StoreDataSynchro);

    RHDRegWrite(Output, TMDSA_TRANSMITTER_ENABLE, Private->StoreTXEnable);
    RHDRegWrite(Output, TMDSA_MACRO_CONTROL, Private->StoreMacro);
    RHDRegWrite(Output, TMDSA_TRANSMITTER_CONTROL, Private->StoreTXControl);

    if (ChipSet >= RHD_RV610)
	RHDRegWrite(Output, TMDSA_TRANSMITTER_ADJUST, Private->StoreTXAdjust);
}

/*
 *
 */
static void
TMDSADestroy(struct rhd_Output *Output)
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
struct rhd_Output *
RHDTMDSAInit(RHDPtr rhdPtr)
{
    struct rhd_Output *Output;
    struct rhd_DAC_Private *Private;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhd_Output), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->rhdPtr = rhdPtr;
    Output->Name = "TMDS A";
    /*
      int Type;
      int Connector;
      int Active;
    */

    Output->Sense = TMDSASense;
    Output->Mode = TMDSASet;
    Output->Power = TMDSAPower;
    Output->Save = TMDSASave;
    Output->Restore = TMDSARestore;
    Output->Destroy = TMDSADestroy;

    Private = xnfcalloc(sizeof(struct rhd_TMDS_Private), 1);
    Output->Private = Private;

    return Output;
}

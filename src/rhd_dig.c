/*
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
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
#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
#else
# include <unistd.h>
#endif

#include "rhd.h"
#include "rhd_crtc.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_regs.h"
#include "rhd_card.h"
#ifdef ATOM_BIOS
#include "rhd_atombios.h"
#endif

#define FMT2_OFFSET 0x800
#define DIG2_OFFSET 0x400

/*
 * Transmitter
 */
struct transmitter {
    enum rhdSensedOutput (*Sense) (struct rhdOutput *Output,
				   enum rhdConnectorType Type);
    ModeStatus (*ModeValid) (struct rhdOutput *Output, DisplayModePtr Mode);
    void (*Mode) (struct rhdOutput *Output, struct rhdCrtc *Crtc, DisplayModePtr Mode);
    void (*Power) (struct rhdOutput *Output, int Power);
    void (*Save) (struct rhdOutput *Output);
    void (*Restore) (struct rhdOutput *Output);
    void (*Destroy) (struct rhdOutput *Output);
    void *Private;
};

/*
 * Encoder
 */
struct encoder {
    ModeStatus (*ModeValid) (struct rhdOutput *Output, DisplayModePtr Mode);
    void (*Mode) (struct rhdOutput *Output, struct rhdCrtc *Crtc, DisplayModePtr Mode);
    void (*Power) (struct rhdOutput *Output, int Power);
    void (*Save) (struct rhdOutput *Output);
    void (*Restore) (struct rhdOutput *Output);
    void (*Destroy) (struct rhdOutput *Output);
    void *Private;
};

/*
 *
 */
enum encoderMode {
    DISPLAYPORT = 0,
    LVDS = 1,
    TMDS_DVI = 2,
    TMDS_HDMI = 3,
    SDVO = 4
};

struct DIGPrivate
{
    struct encoder Encoder;
    struct transmitter Transmitter;
    CARD32 Offset;
    enum encoderMode EncoderMode;
    Bool Coherent;
    Bool DualLink;
    /* LVDS */
    Bool LVDS24Bit;
    Bool FPDI;
    Bool LVDSSpatialDither;
    Bool LVDSTemporalDither;
    int LVDSGreyLevel;
};

/*
 * LVTMA Transmitter
 */

struct LVTMATransmitterPrivate
{
    Bool Stored;

    CARD32 StoredTransmitterControl;
    CARD32 StoredTransmitterAdjust;
    CARD32 StoredPreemphasisControl;
    CARD32 StoredMacroControl;
    CARD32 StoredLVTMADataSynchronization;
    CARD32 StoredTransmiterEnable;
};

/*
 *
 */
static ModeStatus
LVTMATransmitterModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDFUNC(Output);

    if (Output->Connector->Type == RHD_CONNECTOR_DVI_SINGLE
	&& Mode->SynthClock > 165000)
	return MODE_CLOCK_HIGH;

    return MODE_OK;
}

/*
 *
 */
static void
LVTMATransmitterSet(struct rhdOutput *Output, struct rhdCrtc *Crtc, DisplayModePtr Mode)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    CARD32 value = 0;
    AtomBiosArgRec data;
    RHDPtr rhdPtr = RHDPTRI(Output);

    RHDFUNC(Output);

     /* ?? */
    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
	       RV62_LVTMA_USE_CLK_DATA, RV62_LVTMA_USE_CLK_DATA);
    /* set coherent / not coherent mode; whatever that is */
    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
	       Private->Coherent ? 0 : RV62_LVTMA_BYPASS_PLL, RV62_LVTMA_BYPASS_PLL);

#ifdef ATOM_BIOS
    RHDDebug(Output->scrnIndex, "%s: SynthClock: %i Hex: %x EncoderMode: %x\n",__func__,
	     (Mode->SynthClock),(Mode->SynthClock / 10), Private->EncoderMode);

    /* Set up magic value that's used for list lookup */
    value = ((Mode->SynthClock / 10 / ((Private->DualLink) ? 2 : 1)) & 0xffff)
	| (Private->EncoderMode << 16)
	| ((Private->Coherent ? 0x2 : 0) << 24);

    RHDDebug(Output->scrnIndex, "%s: GetConditionalGoldenSettings for: %x\n", __func__, value);

    /* Get data from DIG2TransmitterControl table */
    data.val = 0x4d;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS, ATOMBIOS_GET_CODE_DATA_TABLE,
			&data) == ATOM_SUCCESS) {
	AtomBiosArgRec data1;
	CARD32 *d_p;

	data1.GoldenSettings.BIOSPtr = data.CommandDataTable.loc;
	data1.GoldenSettings.End = data1.GoldenSettings.BIOSPtr + data.CommandDataTable.size;
	data1.GoldenSettings.value = value;

	/* now find pointer */
	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_GET_CONDITIONAL_GOLDEN_SETTINGS, &data1) == ATOM_SUCCESS) {
	    d_p = (CARD32*)data1.GoldenSettings.BIOSPtr;

	    RHDDebug(Output->scrnIndex, "TransmitterAdjust: 0x%8.8x\n",d_p[0]);
	    RHDRegWrite(Output, RV620_LVTMA_TRANSMITTER_ADJUST, d_p[0]);

	    RHDDebug(Output->scrnIndex, "PreemphasisControl: 0x%8.8x\n",d_p[1]);
	    RHDRegWrite(Output, RV620_LVTMA_PREEMPHASIS_CONTROL, d_p[1]);

	    RHDDebug(Output->scrnIndex, "MacroControl: 0x%8.8x\n",d_p[2]);
	    RHDRegWrite(Output, RV620_LVTMA_MACRO_CONTROL, d_p[2]);
	} else
	    xf86DrvMsg(Output->scrnIndex, X_WARNING, "%s: cannot get golden settings\n",__func__);
    } else
#endif
    {
	xf86DrvMsg(Output->scrnIndex, X_WARNING, "%s: No AtomBIOS supplied "
		   "electrical parameters available\n", __func__);
    }
    /* use differential post divider input */
    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
	       RV62_LVTMA_IDSCKSEL, RV62_LVTMA_IDSCKSEL);
}

/*
 *
 */
static void
LVTMATransmitterPower(struct rhdOutput *Output, int Power)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;

    RHDFUNC(Output);

    switch (Power) {
	case RHD_POWER_ON:
	    /* enable PLL */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       RV62_LVTMA_PLL_ENABLE, RV62_LVTMA_PLL_ENABLE);
	    usleep(14);
	    /* PLL reset on */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       RV62_LVTMA_PLL_RESET, RV62_LVTMA_PLL_RESET);
	    usleep(10);
	    /* PLL reset off */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       0, RV62_LVTMA_PLL_RESET);
	    usleep(1000);
	    /* start data synchronization */
	    RHDRegMask(Output, RV620_LVTMA_DATA_SYNCHRONIZATION,
		       RV62_LVTMA_PFREQCHG, RV62_LVTMA_PFREQCHG);
	    usleep(1);
	    /* restart write address logic */
	    RHDRegMask(Output, RV620_LVTMA_DATA_SYNCHRONIZATION,
		       RV62_LVTMA_DSYNSEL, RV62_LVTMA_DSYNSEL);
	    /* ?? */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       RV62_LVTMA_MODE, RV62_LVTMA_MODE);
	    /* enable lower link */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_ENABLE,
		       RV62_LVTMA_LNKL,
		       RV62_LVTMA_LNK_ALL);
	    if (Private->DualLink) {
		usleep (28);
		/* enable upper link */
		RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_ENABLE,
			   RV62_LVTMA_LNKU,
			   RV62_LVTMA_LNKU);
	    }
	    return;
	case RHD_POWER_RESET:
	    /* disable all links */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_ENABLE,
		       0, RV62_LVTMA_LNK_ALL);
	    return;
	case RHD_POWER_SHUTDOWN:
	default:
	    /* disable transmitter */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_ENABLE,
		       0, RV62_LVTMA_LNK_ALL);
	    /* PLL reset */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       RV62_LVTMA_PLL_RESET, RV62_LVTMA_PLL_RESET);
	    usleep(10);
	    /* end PLL reset */
	    RHDRegMask(Output, RV620_LVTMA_TRANSMITTER_CONTROL,
		       0, RV62_LVTMA_PLL_RESET);
	    /* disable data synchronization */
	    RHDRegMask(Output, RV620_LVTMA_DATA_SYNCHRONIZATION,
		       0, RV62_LVTMA_DSYNSEL);
	    /* reset macro control */
	    RHDRegWrite(Output, RV620_LVTMA_TRANSMITTER_ADJUST, 0);
	    return;
    }
}

/*
 *
 */
static void
LVTMATransmitterSave(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;
    struct LVTMATransmitterPrivate *Private = (struct LVTMATransmitterPrivate*)digPrivate->Transmitter.Private;

    RHDFUNC(Output);

    Private->StoredTransmitterControl       = RHDRegRead(Output, RV620_LVTMA_TRANSMITTER_CONTROL);
    Private->StoredTransmitterAdjust        = RHDRegRead(Output, RV620_LVTMA_TRANSMITTER_ADJUST);
    Private->StoredPreemphasisControl       = RHDRegRead(Output, RV620_LVTMA_PREEMPHASIS_CONTROL);
    Private->StoredMacroControl             = RHDRegRead(Output, RV620_LVTMA_MACRO_CONTROL);
    Private->StoredLVTMADataSynchronization = RHDRegRead(Output, RV620_LVTMA_DATA_SYNCHRONIZATION);
    Private->StoredTransmiterEnable         = RHDRegRead(Output, RV620_LVTMA_TRANSMITTER_ENABLE);

    Private->Stored = TRUE;
}

/*
 *
 */
static void
LVTMATransmitterRestore(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;
    struct LVTMATransmitterPrivate *Private = (struct LVTMATransmitterPrivate*)digPrivate->Transmitter.Private;

    RHDFUNC(Output);

    if (!Private->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    RHDRegWrite(Output, RV620_LVTMA_TRANSMITTER_CONTROL, Private->StoredTransmitterControl);
    RHDRegWrite(Output, RV620_LVTMA_TRANSMITTER_ADJUST, Private->StoredTransmitterAdjust);
    RHDRegWrite(Output, RV620_LVTMA_PREEMPHASIS_CONTROL, Private->StoredPreemphasisControl);
    RHDRegWrite(Output, RV620_LVTMA_MACRO_CONTROL, Private->StoredMacroControl);
    RHDRegWrite(Output, RV620_LVTMA_DATA_SYNCHRONIZATION, Private->StoredLVTMADataSynchronization);
    RHDRegWrite(Output, RV620_LVTMA_TRANSMITTER_ENABLE, Private->StoredTransmiterEnable);
}

/*
 *
 */
static void
LVTMATransmitterDestroy(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;

    RHDFUNC(Output);

    if (!digPrivate)
	return;

    xfree(digPrivate->Transmitter.Private);
}

/*
 *  Encoder
 */

struct DIGEncoder
{
    Bool Stored;

    CARD32 StoredDIGClockPattern;
    CARD32 StoredLVDSDataCntl;
    CARD32 StoredTMDSPixelEncoding;
    CARD32 StoredTMDSCntl;
    CARD32 StoredDIGCntl;
    CARD32 StoredDIG_7FA4;
    CARD32 StoredDIGMisc1;
    CARD32 StoredDIGMisc2;
    CARD32 StoredDIGMisc3;
    CARD32 StoredDIGScratch;
    CARD32 StoredDIGScratch3;
};

/*
 *
 */
static ModeStatus
EncoderModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDFUNC(Output);

    return MODE_OK;
}

/*
 *
 */
static void
LVDSEncoder(struct rhdOutput *Output, CARD32 fmt_offset)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    CARD32 off = Private->Offset;
    CARD32 fmt_cntl = 0;

    RHDFUNC(Output);

    /* Clock pattern ? */
    RHDRegMask(Output, off + RV620_DIG1_CLOCK_PATTERN, 0x0063, 0xFFFF);
    /* set panel type: 18/24 bit mode */
    RHDRegMask(Output, off + RV620_LVDS1_DATA_CNTL,
	       (Private->LVDS24Bit ? RV62_LVDS_24BIT_ENABLE : 0)
	       | (Private->FPDI ? RV62_LVDS_24BIT_FORMAT : 0),
	       RV62_LVDS_24BIT_ENABLE | RV62_LVDS_24BIT_FORMAT);
    /* set dither depth to 18/24 */
    fmt_cntl = Private->LVDS24Bit
	? (RV62_FMT_SPATIAL_DITHER_DEPTH | RV62_FMT_TEMPORAL_DITHER_DEPTH)
	: 0;
    RHDRegMask(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL, fmt_offset,
	       RV62_FMT_SPATIAL_DITHER_DEPTH | RV62_FMT_TEMPORAL_DITHER_DEPTH);
    /* set temporal dither */
    if (Private->LVDSTemporalDither) {
	fmt_cntl = Private->LVDSGreyLevel ? RV62_FMT_TEMPORAL_LEVEL : 0x0;
	/* grey level */
	RHDRegMask(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL,
		   fmt_offset, RV62_FMT_TEMPORAL_LEVEL);
	/* turn on temporal dither and reset */
	RHDRegMask(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL,
		   RV62_FMT_TEMPORAL_DITHER_EN | RV62_FMT_TEMPORAL_DITHER_RESET,
		   RV62_FMT_TEMPORAL_DITHER_EN | RV62_FMT_TEMPORAL_DITHER_RESET);
	usleep(20);
	/* turn off reset */
	RHDRegMask(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL, 0x0,
		   RV62_FMT_TEMPORAL_DITHER_RESET);
    }
    /* spatial dither */
    RHDRegMask(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL,
	       Private->LVDSSpatialDither ? RV62_FMT_SPATIAL_DITHER_EN : 0,
	       RV62_FMT_SPATIAL_DITHER_EN);
}

/*
 *
 */
static void
TMDSEncoder(struct rhdOutput *Output, CARD32 fmt_offset)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    CARD32 off = Private->Offset;

    RHDFUNC(Output);

    /* clock pattern ? */
    RHDRegMask(Output, off + RV620_DIG1_CLOCK_PATTERN, 0x001F, 0xFFFF);
    /* color format RGB - normal color format 24bpp, Twin-Single 30bpp or Dual 48bpp*/
    RHDRegMask(Output, off + RV620_TMDS1_CNTL, 0x0,
	       RV62_TMDS_PIXEL_ENCODING | RV62_TMDS_COLOR_FORMAT);
    /* no dithering */
    RHDRegWrite(Output, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL, 0);
}

/*
 *
 */
static void
EncoderSet(struct rhdOutput *Output, struct rhdCrtc *Crtc, DisplayModePtr Mode)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    CARD32 off = Private->Offset;
    CARD32 fmt_offset = (Output->Crtc->Id) ? FMT2_OFFSET : 0;

    RHDFUNC(Output);
    if (Output->Id == RHD_OUTPUT_UNIPHYA) {
	if (!Private->DualLink) {
	    /* enable links */
	    RHDRegMask(Output, off + RV620_DIG1_CNTL, 0, /* RV62_DIG_SWAP | */ RV62_DIG_DUAL_LINK_ENABLE);
	    /* enable link B? */
	    RHDRegMask(Output, RV620_DIG_REG_7FA4, 0x1, 0x1);
	} else {
	    RHDRegMask(Output, off + RV620_DIG1_CNTL, /* RV62_DIG_SWAP | */ RV62_DIG_DUAL_LINK_ENABLE,
		       /* RV62_DIG_SWAP |  */ RV62_DIG_DUAL_LINK_ENABLE );
	    /* disable link B? */
	    RHDRegMask(Output, RV620_DIG_REG_7FA4, 0, 0x1);
	}
    } else if (Output->Id == RHD_OUTPUT_UNIPHYB) {
	RHDRegMask(Output, off + RV620_DIG1_CNTL, 0, /* RV62_DIG_SWAP | */ RV62_DIG_DUAL_LINK_ENABLE);
	/* disable link B ?? */
	RHDRegMask(Output, RV620_DIG_REG_7FA4, 0, 0x1);
    }

    if (Private->EncoderMode == LVDS)
	LVDSEncoder(Output,fmt_offset);
    else if (Private->EncoderMode == DISPLAYPORT)
	RhdAssertFailed("No displayport support yet!",__FILE__, __LINE__, __func__);  /* bugger ! */
    else
	TMDSEncoder(Output,fmt_offset);

    /* 4:4:4 encoding */
    RHDRegMask(Output,  fmt_offset + RV620_FMT1_CONTROL, 0, RV62_FMT_PIXEL_ENCODING);
    /* disable color clamping */
    RHDRegWrite(Output, fmt_offset + RV620_FMT1_CLAMP_CNTL, 0);

    /* Start DIG, set links, disable stereo sync, select FMT source */
    RHDRegMask(Output, off + RV620_DIG1_CNTL,
	       (Private->EncoderMode & 0x7) << 8
	       | RV62_DIG_START
	       | (Private->DualLink ? RV62_DIG_DUAL_LINK_ENABLE : 0)
	       | Output->Crtc->Id,
	       RV62_DIG_MODE
	       | RV62_DIG_START
	       | RV62_DIG_DUAL_LINK_ENABLE
	       | RV62_DIG_STEREOSYNC_SELECT
	       | RV62_DIG_SOURCE_SELECT);

    /* scratch ? */
    RHDRegMask(Output, RV620_DIG_SCRATCH3, 0x0, 0x3 << ((off > 0) ? 12 : 8));
}

/*
 *
 */
static void
EncoderPower(struct rhdOutput *Output, int Power)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    CARD32 off = Private->Offset;

    RHDFUNC(Output);
    /* disable DP ?*/
    RHDRegMask(Output, RV620_DIG_SCRATCH1, 0x0, 0x300 << off ? 0x4 :0);

    switch (Power) {
	case RHD_POWER_ON:
	    /* enable DIG */
	    RHDRegMask(Output, off + RV620_DIG1_CNTL, 0x10, 0x10);
	    RHDRegMask(Output, off ? RV620_DIG_SCRATCH1 : RV620_DIG_SCRATCH2, 0x1, 0x1); /* @@@ */
	    return;
	case RHD_POWER_RESET:
	case RHD_POWER_SHUTDOWN:
	default:
	    /* disable DIG */
	    RHDRegMask(Output, off + RV620_DIG1_CNTL, 0x0, 0x1010);
	    RHDRegMask(Output, off ? RV620_DIG_SCRATCH1 : RV620_DIG_SCRATCH2, 0x0, 0x1); /* @@@ */
	    return;
    }
}

/*
 *
 */
static void
EncoderSave(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;
    struct DIGEncoder *Private = (struct DIGEncoder *)(digPrivate->Encoder.Private);
    CARD32 off = digPrivate->Offset;

    RHDFUNC(Output);

    Private->StoredDIGClockPattern = RHDRegRead(Output, off + RV620_DIG1_CLOCK_PATTERN);
    Private->StoredLVDSDataCntl    = RHDRegRead(Output, off + RV620_LVDS1_DATA_CNTL);
    Private->StoredDIGCntl         = RHDRegRead(Output, off + RV620_DIG1_CNTL);
    Private->StoredTMDSCntl        = RHDRegRead(Output, off + RV620_TMDS1_CNTL);
    Private->StoredDIG_7FA4        = RHDRegRead(Output, RV620_DIG_REG_7FA4);
    Private->StoredDIGScratch      = RHDRegRead(Output, off ? RV620_DIG_SCRATCH2 : RV620_DIG_SCRATCH1);
    Private->StoredDIGScratch3     = RHDRegRead(Output, RV620_DIG_SCRATCH3);

    Private->Stored = TRUE;
}

/*
 *
 */
static void
EncoderRestore(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;
    struct DIGEncoder *Private = (struct DIGEncoder *)(digPrivate->Encoder.Private);
    CARD32 off = digPrivate->Offset;

    RHDFUNC(Output);

    if (!Private->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    RHDRegWrite(Output, off + RV620_DIG1_CLOCK_PATTERN, Private->StoredDIGClockPattern);
    RHDRegWrite(Output, off + RV620_LVDS1_DATA_CNTL, Private->StoredLVDSDataCntl);
    RHDRegWrite(Output, off + RV620_DIG1_CNTL, Private->StoredDIGCntl);
    RHDRegWrite(Output, off + RV620_TMDS1_CNTL, Private->StoredTMDSCntl);
    RHDRegWrite(Output, RV620_DIG_REG_7FA4, Private->StoredDIG_7FA4);
    RHDRegWrite(Output, off ? RV620_DIG_SCRATCH2 : RV620_DIG_SCRATCH1, Private->StoredDIGScratch);
    RHDRegWrite(Output, RV620_DIG_SCRATCH3, Private->StoredDIGScratch3);
}

/*
 *
 */
static void
EncoderDestroy(struct rhdOutput *Output)
{
    struct DIGPrivate *digPrivate = (struct DIGPrivate *)Output->Private;

    RHDFUNC(Output);

    if (!digPrivate || !digPrivate->Encoder.Private)
	return;

    xfree(digPrivate->Encoder.Private);
}

/*
 * Housekeeping
 */
void
GetLVDSInfo(RHDPtr rhdPtr, struct DIGPrivate *Private)
{
    CARD32 off = Private->Offset;
    CARD32 fmt_offset;
    CARD32 tmp;

    RHDFUNC(rhdPtr);

    Private->LVDS24Bit = ((RHDRegRead(rhdPtr, off  + RV620_LVDS1_DATA_CNTL)
			   & RV62_LVDS_24BIT_ENABLE) != 0);
    Private->FPDI = ((RHDRegRead(rhdPtr, off + RV620_LVDS1_DATA_CNTL)
				 & RV62_LVDS_24BIT_FORMAT) != 0);
    Private->DualLink = ((RHDRegRead(rhdPtr, off + RV620_DIG1_CNTL)
				 & RV62_DIG_DUAL_LINK_ENABLE) != 0);

    tmp = RHDRegRead(rhdPtr, off + RV620_DIG1_CNTL);
    fmt_offset = (tmp & RV62_DIG_SOURCE_SELECT) ? FMT2_OFFSET :0;
    tmp = RHDRegRead(rhdPtr, fmt_offset + RV620_FMT1_BIT_DEPTH_CONTROL);
    Private->LVDSSpatialDither = ((tmp & 0x100) != 0);
    Private->LVDSGreyLevel = ((tmp & 0x10000) != 0);
    Private->LVDSTemporalDither = Private->LVDSGreyLevel || ((tmp & 0x1000000) != 0);

#ifdef ATOM_BIOS
    {
	AtomBiosArgRec data;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_LVDS_24BIT, &data) == ATOM_SUCCESS)
	    Private->LVDS24Bit = data.val;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
				 ATOM_LVDS_FPDI, &data) == ATOM_SUCCESS)
	    Private->FPDI = data.val;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_LVDS_DUALLINK, &data) == ATOM_SUCCESS)
	    Private->DualLink = data.val;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_LVDS_SPATIAL_DITHER, &data) == ATOM_SUCCESS)
	    Private->LVDSSpatialDither = data.val;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_LVDS_TEMPORAL_DITHER, &data) == ATOM_SUCCESS)
	    Private->LVDSTemporalDither = data.val;

	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_LVDS_GREYLVL, &data) == ATOM_SUCCESS)
	    Private->LVDSGreyLevel = data.val;
    }
#endif

}

/*
 * Infrastructure
 */

static ModeStatus
DigModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;
    ModeStatus Status;

    RHDFUNC(Output);

    if ((Status = Transmitter->ModeValid(Output, Mode)) == MODE_OK)
	return ((Encoder->ModeValid(Output, Mode)));
    else
	return Status;
}

/*
 *
 */
static void
DigPower(struct rhdOutput *Output, int Power)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;

    RHDFUNC(Output);

    switch (Power) {
	case RHD_POWER_ON:
	    Encoder->Power(Output, Power);
	    Transmitter->Power(Output, Power);
	    return;
	case RHD_POWER_RESET:
	    Transmitter->Power(Output, Power);
	    Encoder->Power(Output, Power);
	    return;
	case RHD_POWER_SHUTDOWN:
	default:
	    Transmitter->Power(Output, Power);
	    Encoder->Power(Output, Power);
	    return;
    }
}

/*
 *
 */
static void
DigMode(struct rhdOutput *Output, DisplayModePtr Mode)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;
    struct rhdCrtc *Crtc = Output->Crtc;

    RHDFUNC(Output);

    /*
     * For dual link capable DVI we need to decide from the pix clock if we are dual link.
     * Do it here as it is convenient.
     */
    if (Output->Connector->Type == RHD_CONNECTOR_DVI)
	Private->DualLink = (Mode->SynthClock > 165000) ? TRUE : FALSE;

    Encoder->Mode(Output, Crtc, Mode);
    Transmitter->Mode(Output, Crtc, Mode);
}

/*
 *
 */
static void
DigSave(struct rhdOutput *Output)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;

    RHDFUNC(Output);

    Encoder->Save(Output);
    Transmitter->Save(Output);
}

/*
 *
 */
static void
DigRestore(struct rhdOutput *Output)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;

    RHDFUNC(Output);

    Encoder->Restore(Output);
    Transmitter->Restore(Output);
}

/*
 *
 */
static void
DigDestroy(struct rhdOutput *Output)
{
    struct DIGPrivate *Private = (struct DIGPrivate *)Output->Private;
    struct transmitter *Transmitter = &Private->Transmitter;
    struct encoder *Encoder = &Private->Encoder;

    RHDFUNC(Output);

    Encoder->Destroy(Output);
    Transmitter->Destroy(Output);

    Output->Private = NULL;
}

/*
 *
 */
struct rhdOutput *
RHDDIGInit(RHDPtr rhdPtr,  enum rhdOutputType outputType, CARD8 ConnectorType)
{
    struct rhdOutput *Output;
    struct DIGPrivate *Private;
    struct DIGEncoder *Encoder;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhdOutput), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->Id = outputType;

    Output->Sense = NULL;
    Output->ModeValid = DigModeValid;
    Output->Mode = DigMode;
    Output->Power = DigPower;
    Output->Save = DigSave;
    Output->Restore = DigRestore;
    Output->Destroy = DigDestroy;

    Private = xnfcalloc(sizeof(struct DIGPrivate), 1);
    Output->Private = Private;

    switch (outputType) {
	case RHD_OUTPUT_UNIPHYA:
	    Output->Name = "UNIPHY_A";
	    Private->Offset = 0;
	    /* for now */
	    xfree(Private);
	    xfree(Output);
	    return NULL;
	    break;
	case RHD_OUTPUT_UNIPHYB:
	    Output->Name = "UNIPHY_B";
	    Private->Offset = DIG2_OFFSET;
	    /* for now */
	    xfree(Private);
	    xfree(Output);
	    return NULL;
	    break;
	case RHD_OUTPUT_KLDSKP_LVTMA:
	    Output->Name = "UNIPHY_KLDSK_LVTMA";
	    Private->Offset = DIG2_OFFSET;
	    Private->Transmitter.Private =
		(struct LVTMATransmitterPrivate *)xnfcalloc(sizeof (struct LVTMATransmitterPrivate), 1);

	    Private->Transmitter.Sense = NULL;
	    Private->Transmitter.ModeValid = LVTMATransmitterModeValid;
	    Private->Transmitter.Mode = LVTMATransmitterSet;
	    Private->Transmitter.Power = LVTMATransmitterPower;
	    Private->Transmitter.Save = LVTMATransmitterSave;
	    Private->Transmitter.Restore = LVTMATransmitterRestore;
	    Private->Transmitter.Destroy = LVTMATransmitterDestroy;
	    break;
	default:
	    xfree(Private);
	    xfree(Output);
	    return NULL;
    }

    Encoder = (struct DIGEncoder *)(xnfcalloc(sizeof (struct DIGEncoder),1));
    Private->Encoder.Private = Encoder;
    Private->Encoder.ModeValid = EncoderModeValid;
    Private->Encoder.Mode = EncoderSet;
    Private->Encoder.Power = EncoderPower;
    Private->Encoder.Save = EncoderSave;
    Private->Encoder.Restore = EncoderRestore;
    Private->Encoder.Destroy = EncoderDestroy;

    switch (ConnectorType) {
	case RHD_CONNECTOR_PANEL:
	    Private->EncoderMode = LVDS;
	    GetLVDSInfo(rhdPtr, Private);
	    break;
	case RHD_CONNECTOR_DVI:
	    Private->DualLink = FALSE;
	    Private->EncoderMode = TMDS_DVI; /* will be set later acc to pxclk */
	    break;
	case RHD_CONNECTOR_DVI_SINGLE:
	    Private->DualLink = FALSE;
	    Private->EncoderMode = TMDS_DVI;  /* currently also HDMI */
	    break;
    }

    return Output;
}

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
#include "rhd_crtc.h"
#include "rhd_pll.h"
#include "rhd_regs.h"

struct rhd_Crtc_Store {
    CARD32 GrphEnable;
    CARD32 GrphControl;
    CARD32 GrphXStart;
    CARD32 GrphYStart;
    CARD32 GrphXEnd;
    CARD32 GrphYEnd;
    CARD32 GrphPrimarySurfaceAddress;
    CARD32 GrphSurfaceOffsetX;
    CARD32 GrphSurfaceOffsetY;
    CARD32 GrphPitch;
    CARD32 GrphLutSel;

    CARD32 ModeViewPortSize;
    CARD32 ModeViewPortStart;
    CARD32 ModeDesktopHeight;
    CARD32 ModeOverScanH;
    CARD32 ModeOverScanV;

    CARD32 CrtcControl;

    CARD32 CrtcHTotal;
    CARD32 CrtcHBlankStartEnd;
    CARD32 CrtcHSyncA;
    CARD32 CrtcHSyncACntl;
    CARD32 CrtcHSyncB;
    CARD32 CrtcHSyncBCntl;

    CARD32 CrtcVTotal;
    CARD32 CrtcVBlankStartEnd;
    CARD32 CrtcVSyncA;
    CARD32 CrtcVSyncACntl;
    CARD32 CrtcVSyncB;
    CARD32 CrtcVSyncBCntl;

    CARD32 CrtcBlankControl;
    CARD32 CrtcPCLKControl;

};

/*
 *
 */
static Bool
D1FBValid(struct rhd_Crtc *Crtc, CARD16 Pitch, CARD16 Width, CARD16 Height,
	  int bpp, int Offset)
{
    ScrnInfoPtr pScrn = xf86Screens[Crtc->scrnIndex];
    CARD32 tmp;

    RHDFUNC(Crtc);

    if (Offset > (pScrn->videoRam * 1024)) {
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: offset (0x%08X) exceeds "
		   "available Framebuffer memory (0x%08X)\n", __func__,
		   Offset, pScrn->videoRam * 1024);
	return FALSE;
    }

    if (Offset & 0xFFF) {
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: Offset (0x%08X) is not "
		   "4K aligned\n", __func__, Offset);
	return FALSE;
    }

    tmp = 256; /* 256byte alignment */
    switch (bpp) {
    case 8:
	break;
    case 16:
	tmp /= 2;
	break;
    case 24:
    case 32:
	tmp /= 4;
	break;
    default:
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: %dbpp is not supported\n",
		   __func__, bpp);
	return FALSE;
    }

    if (Pitch & (tmp - 1)) {
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: Pitch (0x%08X) is not "
		   "256bytes aligned\n", __func__, Pitch);
	return FALSE;
    }

    if (Width > 8192) {
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: Width (%d) is larger than "
		   "8192 pixels.\n", __func__, Width);
	return FALSE;
    }

    if (Height > 8192) {
	xf86DrvMsg(Crtc->scrnIndex, X_INFO, "%s: Height (%d) is larger than "
		   "8192 lines.\n", __func__, Height);
	return FALSE;
    }

    return TRUE;
}

/*
 *
 */
static void
D1FBSet(struct rhd_Crtc *Crtc, CARD16 Pitch, CARD16 Width, CARD16 Height, int bpp,
	    int Offset)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[Crtc->scrnIndex]);

    RHDFUNC(Crtc);

    RHDRegMask(Crtc, D1GRPH_ENABLE, 1, 0x00000001);

    switch (bpp) {
    case 8:
	RHDRegMask(Crtc, D1GRPH_CONTROL, 0, 0x10703);
	break;
    case 16:
	RHDRegMask(Crtc, D1GRPH_CONTROL, 0x00101, 0x10703);
	break;
    case 24:
    case 32:
    default:
	RHDRegMask(Crtc, D1GRPH_CONTROL, 0x00002, 0x10703);
	break;
    /* TODO: 64bpp ;p */
    }

    RHDRegWrite(Crtc, D1GRPH_PRIMARY_SURFACE_ADDRESS, rhdPtr->FbIntAddress + Offset);
    RHDRegWrite(Crtc, D1GRPH_PITCH, Pitch);
    RHDRegWrite(Crtc, D1GRPH_SURFACE_OFFSET_X, 0);
    RHDRegWrite(Crtc, D1GRPH_SURFACE_OFFSET_Y, 0);
    RHDRegWrite(Crtc, D1GRPH_X_START, 0);
    RHDRegWrite(Crtc, D1GRPH_Y_START, 0);
    RHDRegWrite(Crtc, D1GRPH_X_END, Width);
    RHDRegWrite(Crtc, D1GRPH_Y_END, Height);

    /* D1Mode registers */
    RHDRegWrite(rhdPtr, D1MODE_DESKTOP_HEIGHT, Height);

    Crtc->Pitch = Pitch;
    Crtc->Width = Width;
    Crtc->Height = Height;
    Crtc->bpp = bpp;
    Crtc->Offset = Offset;
}

/*
 *
 */
static void
D2FBSet(struct rhd_Crtc *Crtc, CARD16 Pitch, CARD16 Width, CARD16 Height, int bpp,
	int Offset)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[Crtc->scrnIndex]);

    RHDFUNC(Crtc);

    RHDRegMask(Crtc, D2GRPH_ENABLE, 1, 0x00000001);

    switch (bpp) {
    case 8:
	RHDRegMask(Crtc, D2GRPH_CONTROL, 0, 0x10703);
	break;
    case 16:
	RHDRegMask(Crtc, D2GRPH_CONTROL, 0x00101, 0x10703);
	break;
    case 24:
    case 32:
    default:
	RHDRegMask(Crtc, D2GRPH_CONTROL, 0x00002, 0x10703);
	break;
    /* TODO: 64bpp ;p */
    }

    RHDRegWrite(Crtc, D2GRPH_PRIMARY_SURFACE_ADDRESS, rhdPtr->FbIntAddress + Offset);
    RHDRegWrite(Crtc, D2GRPH_PITCH, Pitch);
    RHDRegWrite(Crtc, D2GRPH_SURFACE_OFFSET_X, 0);
    RHDRegWrite(Crtc, D2GRPH_SURFACE_OFFSET_Y, 0);
    RHDRegWrite(Crtc, D2GRPH_X_START, 0);
    RHDRegWrite(Crtc, D2GRPH_Y_START, 0);
    RHDRegWrite(Crtc, D2GRPH_X_END, Width);
    RHDRegWrite(Crtc, D2GRPH_Y_END, Height);

    /* D2Mode registers */
    RHDRegWrite(rhdPtr, D2MODE_DESKTOP_HEIGHT, Height);

    Crtc->Pitch = Pitch;
    Crtc->Width = Width;
    Crtc->Height = Height;
    Crtc->bpp = bpp;
    Crtc->Offset = Offset;
}

/*
 *
 */
static ModeStatus
D1ModeValid(struct rhd_Crtc *Crtc, DisplayModePtr Mode)
{
    RHDFUNC(Crtc);

    /* FILL ME!!!! */

    return MODE_OK;
}

/*
 *
 */
static ModeStatus
D2ModeValid(struct rhd_Crtc *Crtc, DisplayModePtr Mode)
{
    RHDFUNC(Crtc);

    /* FILL ME!!!! */

    return MODE_OK;
}

/*
 *
 */
static void
D1ModeSet(struct rhd_Crtc *Crtc, DisplayModePtr Mode)
{
    CARD16 BlankStart, BlankEnd;

    RHDFUNC(Crtc);

    /* D1Mode registers */
    RHDRegWrite(Crtc, D1MODE_VIEWPORT_SIZE,
		Mode->CrtcVDisplay | (Mode->CrtcHDisplay << 16));
    RHDRegWrite(Crtc, D1MODE_VIEWPORT_START, 0);
    /* Docs for D1MODE_VIEWPORT_END ???? */
    RHDRegWrite(Crtc, D1MODE_EXT_OVERSCAN_LEFT_RIGHT,
		((Mode->CrtcHTotal - Mode->CrtcHBlankEnd) << 16) |
		(Mode->CrtcHBlankStart - Mode->CrtcHDisplay));
    RHDRegWrite(Crtc, D1MODE_EXT_OVERSCAN_TOP_BOTTOM,
		((Mode->CrtcVTotal - Mode->CrtcVBlankEnd) << 16) |
		(Mode->CrtcVBlankStart - Mode->CrtcVDisplay));

    /* D1Crtc registers */
    RHDRegMask(Crtc, D1CRTC_CONTROL, 0, 0x01000000); /* enable read requests */

    /* Set the actual CRTC timing */
    RHDRegWrite(Crtc, D1CRTC_H_TOTAL, Mode->CrtcHTotal - 1);

    BlankStart = Mode->CrtcHTotal + Mode->CrtcHBlankStart - Mode->CrtcHSyncStart;
    BlankEnd = Mode->CrtcHBlankEnd - Mode->CrtcHSyncStart;
    RHDRegWrite(Crtc, D1CRTC_H_BLANK_START_END, BlankStart | (BlankEnd << 16));

    RHDRegWrite(Crtc, D1CRTC_H_SYNC_A, (Mode->CrtcHSyncEnd - Mode->CrtcHSyncStart) << 16);
    RHDRegWrite(Crtc, D1CRTC_H_SYNC_A_CNTL, Mode->Flags & V_NHSYNC);

    RHDRegWrite(Crtc, D1CRTC_V_TOTAL, Mode->CrtcVTotal - 1);

    BlankStart = Mode->CrtcVTotal + Mode->CrtcVBlankStart - Mode->CrtcVSyncStart;
    BlankEnd = Mode->CrtcVBlankEnd - Mode->CrtcVSyncStart;
    RHDRegWrite(Crtc, D1CRTC_V_BLANK_START_END, BlankStart | (BlankEnd << 16));

    RHDRegWrite(Crtc, D1CRTC_V_SYNC_A, (Mode->CrtcVSyncEnd - Mode->CrtcVSyncStart) << 16);
    RHDRegWrite(Crtc, D1CRTC_V_SYNC_A_CNTL, Mode->Flags & V_NVSYNC);

    Crtc->CurrentMode = Mode;
}

/*
 *
 */
static void
D2ModeSet(struct rhd_Crtc *Crtc, DisplayModePtr Mode)
{
    CARD16 BlankStart, BlankEnd;

    RHDFUNC(Crtc);

    /* D2Mode registers */
    RHDRegWrite(Crtc, D2MODE_VIEWPORT_SIZE,
		Mode->CrtcVDisplay | (Mode->CrtcHDisplay << 16));
    RHDRegWrite(Crtc, D2MODE_VIEWPORT_START, 0);
    /* Docs for D2MODE_VIEWPORT_END ???? */
    RHDRegWrite(Crtc, D2MODE_EXT_OVERSCAN_LEFT_RIGHT,
		((Mode->CrtcHTotal - Mode->CrtcHBlankEnd) << 16) |
		(Mode->CrtcHBlankStart - Mode->CrtcHDisplay));
    RHDRegWrite(Crtc, D2MODE_EXT_OVERSCAN_TOP_BOTTOM,
		((Mode->CrtcVTotal - Mode->CrtcVBlankEnd) << 16) |
		(Mode->CrtcVBlankStart - Mode->CrtcVDisplay));

    /* D2Crtc registers */
    RHDRegMask(Crtc, D2CRTC_CONTROL, 0, 0x01000000); /* enable read requests */

    /* Set the actual CRTC timing */
    RHDRegWrite(Crtc, D2CRTC_H_TOTAL, Mode->CrtcHTotal - 1);

    BlankStart = Mode->CrtcHTotal + Mode->CrtcHBlankStart - Mode->CrtcHSyncStart;
    BlankEnd = Mode->CrtcHBlankEnd - Mode->CrtcHSyncStart;
    RHDRegWrite(Crtc, D2CRTC_H_BLANK_START_END, BlankStart | (BlankEnd << 16));

    RHDRegWrite(Crtc, D2CRTC_H_SYNC_A, (Mode->CrtcHSyncEnd - Mode->CrtcHSyncStart) << 16);
    RHDRegWrite(Crtc, D2CRTC_H_SYNC_A_CNTL, Mode->Flags & V_NHSYNC);

    RHDRegWrite(Crtc, D2CRTC_V_TOTAL, Mode->CrtcVTotal - 1);

    BlankStart = Mode->CrtcVTotal + Mode->CrtcVBlankStart - Mode->CrtcVSyncStart;
    BlankEnd = Mode->CrtcVBlankEnd - Mode->CrtcVSyncStart;
    RHDRegWrite(Crtc, D2CRTC_V_BLANK_START_END, BlankStart | (BlankEnd << 16));

    RHDRegWrite(Crtc, D2CRTC_V_SYNC_A, (Mode->CrtcVSyncEnd - Mode->CrtcVSyncStart) << 16);
    RHDRegWrite(Crtc, D2CRTC_V_SYNC_A_CNTL, Mode->Flags & V_NVSYNC);

    Crtc->CurrentMode = Mode;
}

/*
 *
 */
static void
D1PLLSelect(struct rhd_Crtc *Crtc, struct rhd_PLL *PLL)
{
    RHDFUNC(Crtc);

    RHDRegMask(Crtc, PCLK_CRTC1_CNTL, PLL->Id << 16, 0x00010000);
    Crtc->PLL = PLL;
}

/*
 *
 */
static void
D2PLLSelect(struct rhd_Crtc *Crtc, struct rhd_PLL *PLL)
{
    RHDFUNC(Crtc);

    RHDRegMask(Crtc, PCLK_CRTC2_CNTL, PLL->Id << 16, 0x00010000);
    Crtc->PLL = PLL;
}

/*
 *
 */
static void
D1LUTSelect(struct rhd_Crtc *Crtc, int LUT)
{
    RHDFUNC(Crtc);

    RHDRegWrite(Crtc, D1GRPH_LUT_SEL, LUT & 1);
    Crtc->LUT = LUT;
}

/*
 *
 */
static void
D2LUTSelect(struct rhd_Crtc *Crtc, int LUT)
{
    RHDFUNC(Crtc);

    RHDRegWrite(Crtc, D2GRPH_LUT_SEL, LUT & 1);
    Crtc->LUT = LUT;
}

/*
 *
 */
static void
D1ViewPortStart(struct rhd_Crtc *Crtc, CARD16 X, CARD16 Y)
{
    RHDFUNC(Crtc);

    xf86DrvMsg(Crtc->scrnIndex, X_WARNING, "%s: not implemented.\n", __func__);
    return;

    /* kills the linebuffer!!! requires full powerdown to fix. */
    RHDRegWrite(Crtc, D1MODE_VIEWPORT_START, (X << 16) | Y);
}

/*
 *
 */
static void
D2ViewPortStart(struct rhd_Crtc *Crtc, CARD16 X, CARD16 Y)
{
    RHDFUNC(Crtc);

    xf86DrvMsg(Crtc->scrnIndex, X_WARNING, "%s: not implemented.\n", __func__);
    return;

    /* kills the linebuffer!!! requires full powerdown to fix. */
    RHDRegWrite(Crtc, D2MODE_VIEWPORT_START, (X << 16) | Y);
}

#define CRTC_SYNC_WAIT 0x100000
/*
 *
 */
static void
D1CRTCDisable(struct rhd_Crtc *Crtc)
{
    if (RHDRegRead(Crtc, D1CRTC_CONTROL) & 1) {
	CARD8 delay = (RHDRegRead(Crtc, D1CRTC_CONTROL) >> 8) & 0xFF;
	int i;

	RHDRegMask(Crtc, D1CRTC_CONTROL, 0, 0xFF01);

	for (i = 0; i < CRTC_SYNC_WAIT; i++)
	    if (!(RHDRegRead(Crtc, D1CRTC_STATUS) & 1)) {
		RHDRegMask(Crtc, D1CRTC_CONTROL, delay << 8, 0xFF00);
		RHDDebug(Crtc->scrnIndex, "%s: %d loops\n", __func__, i);
		return;
	    }
	xf86DrvMsg(Crtc->scrnIndex, X_ERROR,
		   "%s: Failed to Unsync %s\n", __func__, Crtc->Name);
    }
}

/*
 *
 */
static void
D2CRTCDisable(struct rhd_Crtc *Crtc)
{
    if (RHDRegRead(Crtc, D2CRTC_CONTROL) & 1) {
	CARD8 delay = (RHDRegRead(Crtc, D2CRTC_CONTROL) >> 8) & 0xFF;
	int i;

	RHDRegMask(Crtc, D2CRTC_CONTROL, 0, 0xFF01);

	for (i = 0; i < CRTC_SYNC_WAIT; i++)
	    if (!(RHDRegRead(Crtc, D2CRTC_STATUS) & 1)) {
		RHDRegMask(Crtc, D2CRTC_CONTROL, delay << 8, 0xFF00);
		RHDDebug(Crtc->scrnIndex, "%s: %d loops\n", __func__, i);
		return;
	    }
	xf86DrvMsg(Crtc->scrnIndex, X_ERROR,
		   "%s: Failed to Unsync %s\n", __func__, Crtc->Name);
    }
}

/*
 *
 */
static void
D1Power(struct rhd_Crtc *Crtc, int Power)
{
    RHDFUNC(Crtc);

    switch (Power) {
    case RHD_POWER_ON:
	RHDRegMask(Crtc, D1GRPH_ENABLE, 0x00000001, 0x00000001);
	usleep(2);
	RHDRegMask(Crtc, D1CRTC_BLANK_CONTROL, 0, 0x00000100);
	RHDRegMask(Crtc, D1CRTC_CONTROL, 0, 0x01000000); /* enable read requests */
	RHDRegMask(Crtc, D1CRTC_CONTROL, 1, 1);
	return;
    case RHD_POWER_RESET:
	RHDRegMask(Crtc, D1CRTC_CONTROL, 0x01000000, 0x01000000); /* disable read requests */
	D1CRTCDisable(Crtc);
	RHDRegMask(Crtc, D1CRTC_BLANK_CONTROL, 0x00000100, 0x00000100);
	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(Crtc, D1CRTC_CONTROL, 0x01000000, 0x01000000); /* disable read requests */
	D1CRTCDisable(Crtc);
	RHDRegMask(Crtc, D1CRTC_BLANK_CONTROL, 0x00000100, 0x00000100);
	RHDRegMask(Crtc, D1GRPH_ENABLE, 0, 0x00000001);
	return;
    }
}

/*
 *
 */
static void
D2Power(struct rhd_Crtc *Crtc, int Power)
{
    RHDFUNC(Crtc);

    switch (Power) {
    case RHD_POWER_ON:
	RHDRegMask(Crtc, D2GRPH_ENABLE, 0x00000001, 0x00000001);
	usleep(2);
	RHDRegMask(Crtc, D2CRTC_BLANK_CONTROL, 0, 0x00000100);
	RHDRegMask(Crtc, D2CRTC_CONTROL, 0, 0x01000000); /* enable read requests */
	RHDRegMask(Crtc, D2CRTC_CONTROL, 1, 1);
	return;
    case RHD_POWER_RESET:
	RHDRegMask(Crtc, D2CRTC_CONTROL, 0x01000000, 0x01000000); /* disable read requests */
	D2CRTCDisable(Crtc);
	RHDRegMask(Crtc, D2CRTC_BLANK_CONTROL, 0x00000100, 0x00000100);
	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(Crtc, D2CRTC_CONTROL, 0x01000000, 0x01000000); /* disable read requests */
	D2CRTCDisable(Crtc);
	RHDRegMask(Crtc, D2CRTC_BLANK_CONTROL, 0x00000100, 0x00000100);
	RHDRegMask(Crtc, D2GRPH_ENABLE, 0, 0x00000001);
	return;
    }
}

/*
 *
 */
static void
D1Save(struct rhd_Crtc *Crtc)
{
    struct rhd_Crtc_Store *Store;

    RHDFUNC(Crtc);

    if (!Crtc->Store)
	Store = xnfcalloc(sizeof(struct rhd_Crtc_Store), 1);
    else
	Store = Crtc->Store;

    Store->GrphEnable = RHDRegRead(Crtc, D1GRPH_ENABLE);
    Store->GrphControl = RHDRegRead(Crtc, D1GRPH_CONTROL);
    Store->GrphXStart = RHDRegRead(Crtc, D1GRPH_X_START);
    Store->GrphYStart = RHDRegRead(Crtc, D1GRPH_Y_START);
    Store->GrphXEnd = RHDRegRead(Crtc, D1GRPH_X_END);
    Store->GrphYEnd = RHDRegRead(Crtc, D1GRPH_Y_END);
    Store->GrphPrimarySurfaceAddress = RHDRegRead(Crtc, D1GRPH_PRIMARY_SURFACE_ADDRESS);
    Store->GrphSurfaceOffsetX = RHDRegRead(Crtc, D1GRPH_SURFACE_OFFSET_X);
    Store->GrphSurfaceOffsetY = RHDRegRead(Crtc, D1GRPH_SURFACE_OFFSET_Y);
    Store->GrphPitch = RHDRegRead(Crtc, D1GRPH_PITCH);
    Store->GrphLutSel = RHDRegRead(Crtc, D1GRPH_LUT_SEL);

    Store->ModeViewPortSize = RHDRegRead(Crtc, D1MODE_VIEWPORT_SIZE);
    Store->ModeViewPortStart = RHDRegRead(Crtc, D1MODE_VIEWPORT_START);
    Store->ModeDesktopHeight = RHDRegRead(Crtc, D1MODE_DESKTOP_HEIGHT);
    Store->ModeOverScanH = RHDRegRead(Crtc, D1MODE_EXT_OVERSCAN_LEFT_RIGHT);
    Store->ModeOverScanV = RHDRegRead(Crtc, D1MODE_EXT_OVERSCAN_TOP_BOTTOM);

    Store->CrtcControl = RHDRegRead(Crtc, D1CRTC_CONTROL);

    Store->CrtcHTotal = RHDRegRead(Crtc, D1CRTC_H_TOTAL);
    Store->CrtcHBlankStartEnd = RHDRegRead(Crtc, D1CRTC_H_BLANK_START_END);
    Store->CrtcHSyncA = RHDRegRead(Crtc, D1CRTC_H_SYNC_A);
    Store->CrtcHSyncACntl = RHDRegRead(Crtc, D1CRTC_H_SYNC_A_CNTL);
    Store->CrtcHSyncB = RHDRegRead(Crtc, D1CRTC_H_SYNC_B);
    Store->CrtcHSyncBCntl = RHDRegRead(Crtc, D1CRTC_H_SYNC_B_CNTL);

    Store->CrtcVTotal = RHDRegRead(Crtc, D1CRTC_V_TOTAL);
    Store->CrtcVBlankStartEnd = RHDRegRead(Crtc, D1CRTC_V_BLANK_START_END);
    Store->CrtcVSyncA = RHDRegRead(Crtc, D1CRTC_V_SYNC_A);
    Store->CrtcVSyncACntl = RHDRegRead(Crtc, D1CRTC_V_SYNC_A_CNTL);
    Store->CrtcVSyncB = RHDRegRead(Crtc, D1CRTC_V_SYNC_B);
    Store->CrtcVSyncBCntl = RHDRegRead(Crtc, D1CRTC_V_SYNC_B_CNTL);

    Store->CrtcBlankControl = RHDRegRead(Crtc, D1CRTC_BLANK_CONTROL);

    Store->CrtcPCLKControl = RHDRegRead(Crtc, PCLK_CRTC1_CNTL);

    Crtc->Store = Store;
}

/*
 *
 */
static void
D2Save(struct rhd_Crtc *Crtc)
{
    struct rhd_Crtc_Store *Store;

    RHDFUNC(Crtc);

    if (!Crtc->Store)
	Store = xnfcalloc(sizeof(struct rhd_Crtc_Store), 1);
    else
	Store = Crtc->Store;

    Store->GrphEnable = RHDRegRead(Crtc, D2GRPH_ENABLE);
    Store->GrphControl = RHDRegRead(Crtc, D2GRPH_CONTROL);
    Store->GrphXStart = RHDRegRead(Crtc, D2GRPH_X_START);
    Store->GrphYStart = RHDRegRead(Crtc, D2GRPH_Y_START);
    Store->GrphXEnd = RHDRegRead(Crtc, D2GRPH_X_END);
    Store->GrphYEnd = RHDRegRead(Crtc, D2GRPH_Y_END);
    Store->GrphPrimarySurfaceAddress = RHDRegRead(Crtc, D2GRPH_PRIMARY_SURFACE_ADDRESS);
    Store->GrphSurfaceOffsetX = RHDRegRead(Crtc, D2GRPH_SURFACE_OFFSET_X);
    Store->GrphSurfaceOffsetY = RHDRegRead(Crtc, D2GRPH_SURFACE_OFFSET_Y);
    Store->GrphPitch = RHDRegRead(Crtc, D2GRPH_PITCH);
    Store->GrphLutSel = RHDRegRead(Crtc, D2GRPH_LUT_SEL);

    Store->ModeViewPortSize = RHDRegRead(Crtc, D2MODE_VIEWPORT_SIZE);
    Store->ModeViewPortStart = RHDRegRead(Crtc, D2MODE_VIEWPORT_START);
    Store->ModeDesktopHeight = RHDRegRead(Crtc, D2MODE_DESKTOP_HEIGHT);
    Store->ModeOverScanH = RHDRegRead(Crtc, D2MODE_EXT_OVERSCAN_LEFT_RIGHT);
    Store->ModeOverScanV = RHDRegRead(Crtc, D2MODE_EXT_OVERSCAN_TOP_BOTTOM);

    Store->CrtcControl = RHDRegRead(Crtc, D2CRTC_CONTROL);

    Store->CrtcHTotal = RHDRegRead(Crtc, D2CRTC_H_TOTAL);
    Store->CrtcHBlankStartEnd = RHDRegRead(Crtc, D2CRTC_H_BLANK_START_END);
    Store->CrtcHSyncA = RHDRegRead(Crtc, D2CRTC_H_SYNC_A);
    Store->CrtcHSyncACntl = RHDRegRead(Crtc, D2CRTC_H_SYNC_A_CNTL);
    Store->CrtcHSyncB = RHDRegRead(Crtc, D2CRTC_H_SYNC_B);
    Store->CrtcHSyncBCntl = RHDRegRead(Crtc, D2CRTC_H_SYNC_B_CNTL);

    Store->CrtcVTotal = RHDRegRead(Crtc, D2CRTC_V_TOTAL);
    Store->CrtcVBlankStartEnd = RHDRegRead(Crtc, D2CRTC_V_BLANK_START_END);
    Store->CrtcVSyncA = RHDRegRead(Crtc, D2CRTC_V_SYNC_A);
    Store->CrtcVSyncACntl = RHDRegRead(Crtc, D2CRTC_V_SYNC_A_CNTL);
    Store->CrtcVSyncB = RHDRegRead(Crtc, D2CRTC_V_SYNC_B);
    Store->CrtcVSyncBCntl = RHDRegRead(Crtc, D2CRTC_V_SYNC_B_CNTL);

    Store->CrtcBlankControl = RHDRegRead(Crtc, D2CRTC_BLANK_CONTROL);

    Store->CrtcPCLKControl = RHDRegRead(Crtc, PCLK_CRTC2_CNTL);

    Crtc->Store = Store;
}

/*
 *
 */
static void
D1Restore(struct rhd_Crtc *Crtc)
{
    struct rhd_Crtc_Store *Store = Crtc->Store;

    RHDFUNC(Crtc);

    if (!Store) {
	xf86DrvMsg(Crtc->scrnIndex, X_ERROR, "%s: no registers stored!\n",
		   __func__);
	return;
    }

    RHDRegWrite(Crtc, D1GRPH_ENABLE, Store->GrphEnable);
    RHDRegWrite(Crtc, D1GRPH_CONTROL, Store->GrphControl);
    RHDRegWrite(Crtc, D1GRPH_X_START, Store->GrphXStart);
    RHDRegWrite(Crtc, D1GRPH_Y_START, Store->GrphYStart);
    RHDRegWrite(Crtc, D1GRPH_X_END, Store->GrphXEnd);
    RHDRegWrite(Crtc, D1GRPH_Y_END, Store->GrphYEnd);
    RHDRegWrite(Crtc, D1GRPH_PRIMARY_SURFACE_ADDRESS,
		Store->GrphPrimarySurfaceAddress);
    RHDRegWrite(Crtc, D1GRPH_SURFACE_OFFSET_X, Store->GrphSurfaceOffsetX);
    RHDRegWrite(Crtc, D1GRPH_SURFACE_OFFSET_Y, Store->GrphSurfaceOffsetY);

    RHDRegWrite(Crtc, D1GRPH_PITCH, Store->GrphPitch);
    RHDRegWrite(Crtc, D1GRPH_LUT_SEL, Store->GrphLutSel);

    RHDRegWrite(Crtc, D1MODE_VIEWPORT_SIZE, Store->ModeViewPortSize);
    RHDRegWrite(Crtc, D1MODE_VIEWPORT_START, Store->ModeViewPortStart);
    RHDRegWrite(Crtc, D1MODE_DESKTOP_HEIGHT, Store->ModeDesktopHeight);
    RHDRegWrite(Crtc, D1MODE_EXT_OVERSCAN_LEFT_RIGHT, Store->ModeOverScanH);
    RHDRegWrite(Crtc, D1MODE_EXT_OVERSCAN_TOP_BOTTOM, Store->ModeOverScanV);

    RHDRegWrite(Crtc, D1CRTC_CONTROL, Store->CrtcControl);

    RHDRegWrite(Crtc, D1CRTC_H_TOTAL, Store->CrtcHTotal);
    RHDRegWrite(Crtc, D1CRTC_H_BLANK_START_END, Store->CrtcHBlankStartEnd);
    RHDRegWrite(Crtc, D1CRTC_H_SYNC_A, Store->CrtcHSyncA);
    RHDRegWrite(Crtc, D1CRTC_H_SYNC_A_CNTL, Store->CrtcHSyncACntl);
    RHDRegWrite(Crtc, D1CRTC_H_SYNC_B, Store->CrtcHSyncB);
    RHDRegWrite(Crtc, D1CRTC_H_SYNC_B_CNTL, Store->CrtcHSyncBCntl);

    RHDRegWrite(Crtc, D1CRTC_V_TOTAL, Store->CrtcVTotal);
    RHDRegWrite(Crtc, D1CRTC_V_BLANK_START_END, Store->CrtcVBlankStartEnd);
    RHDRegWrite(Crtc, D1CRTC_V_SYNC_A, Store->CrtcVSyncA);
    RHDRegWrite(Crtc, D1CRTC_V_SYNC_A_CNTL, Store->CrtcVSyncACntl);
    RHDRegWrite(Crtc, D1CRTC_V_SYNC_B, Store->CrtcVSyncB);
    RHDRegWrite(Crtc, D1CRTC_V_SYNC_B_CNTL, Store->CrtcVSyncBCntl);

    RHDRegWrite(Crtc, D1CRTC_BLANK_CONTROL, Store->CrtcBlankControl);

    RHDRegWrite(Crtc, PCLK_CRTC1_CNTL, Store->CrtcPCLKControl);
}

/*
 *
 */
static void
D2Restore(struct rhd_Crtc *Crtc)
{
    struct rhd_Crtc_Store *Store = Crtc->Store;

    RHDFUNC(Crtc);

    if (!Store) {
	xf86DrvMsg(Crtc->scrnIndex, X_ERROR, "%s: no registers stored!\n",
		   __func__);
	return;
    }

    RHDRegWrite(Crtc, D2GRPH_ENABLE, Store->GrphEnable);
    RHDRegWrite(Crtc, D2GRPH_CONTROL, Store->GrphControl);
    RHDRegWrite(Crtc, D2GRPH_X_START, Store->GrphXStart);
    RHDRegWrite(Crtc, D2GRPH_Y_START, Store->GrphYStart);
    RHDRegWrite(Crtc, D2GRPH_X_END, Store->GrphXEnd);
    RHDRegWrite(Crtc, D2GRPH_Y_END, Store->GrphYEnd);
    RHDRegWrite(Crtc, D2GRPH_PRIMARY_SURFACE_ADDRESS,
		Store->GrphPrimarySurfaceAddress);
    RHDRegWrite(Crtc, D2GRPH_SURFACE_OFFSET_X, Store->GrphSurfaceOffsetX);
    RHDRegWrite(Crtc, D2GRPH_SURFACE_OFFSET_Y, Store->GrphSurfaceOffsetY);
    RHDRegWrite(Crtc, D2GRPH_PITCH, Store->GrphPitch);
    RHDRegWrite(Crtc, D2GRPH_LUT_SEL, Store->GrphLutSel);

    RHDRegWrite(Crtc, D2MODE_VIEWPORT_SIZE, Store->ModeViewPortSize);
    RHDRegWrite(Crtc, D2MODE_VIEWPORT_START, Store->ModeViewPortStart);
    RHDRegWrite(Crtc, D2MODE_DESKTOP_HEIGHT, Store->ModeDesktopHeight);
    RHDRegWrite(Crtc, D2MODE_EXT_OVERSCAN_LEFT_RIGHT, Store->ModeOverScanH);
    RHDRegWrite(Crtc, D2MODE_EXT_OVERSCAN_TOP_BOTTOM, Store->ModeOverScanV);

    RHDRegWrite(Crtc, D2CRTC_CONTROL, Store->CrtcControl);

    RHDRegWrite(Crtc, D2CRTC_H_TOTAL, Store->CrtcHTotal);
    RHDRegWrite(Crtc, D2CRTC_H_BLANK_START_END, Store->CrtcHBlankStartEnd);
    RHDRegWrite(Crtc, D2CRTC_H_SYNC_A, Store->CrtcHSyncA);
    RHDRegWrite(Crtc, D2CRTC_H_SYNC_A_CNTL, Store->CrtcHSyncACntl);
    RHDRegWrite(Crtc, D2CRTC_H_SYNC_B, Store->CrtcHSyncB);
    RHDRegWrite(Crtc, D2CRTC_H_SYNC_B_CNTL, Store->CrtcHSyncBCntl);

    RHDRegWrite(Crtc, D2CRTC_V_TOTAL, Store->CrtcVTotal);
    RHDRegWrite(Crtc, D2CRTC_V_BLANK_START_END, Store->CrtcVBlankStartEnd);
    RHDRegWrite(Crtc, D2CRTC_V_SYNC_A, Store->CrtcVSyncA);
    RHDRegWrite(Crtc, D2CRTC_V_SYNC_A_CNTL, Store->CrtcVSyncACntl);
    RHDRegWrite(Crtc, D2CRTC_V_SYNC_B, Store->CrtcVSyncB);
    RHDRegWrite(Crtc, D2CRTC_V_SYNC_B_CNTL, Store->CrtcVSyncBCntl);

    RHDRegWrite(Crtc, D2CRTC_BLANK_CONTROL, Store->CrtcBlankControl);

    RHDRegWrite(Crtc, PCLK_CRTC2_CNTL, Store->CrtcPCLKControl);
}

/*
 *
 */
void
RHDCRTCDestroy(RHDPtr rhdPtr)
{
    struct rhd_Crtc *Crtc;

    RHDFUNC(rhdPtr);

    Crtc = rhdPtr->Crtc[0];
    if (Crtc) {
	if (Crtc->Store)
	    xfree(Crtc->Store);
	xfree(Crtc);
    }

    Crtc = rhdPtr->Crtc[1];
    if (Crtc) {
	if (Crtc->Store)
	    xfree(Crtc->Store);
	xfree(Crtc);
    }
}

/*
 *
 */
void
RHDCRTCInit(RHDPtr rhdPtr)
{
    struct rhd_Crtc *Crtc;

    RHDFUNC(rhdPtr);

    Crtc = xnfcalloc(sizeof(struct rhd_Crtc), 1);
    Crtc->scrnIndex = rhdPtr->scrnIndex;
    Crtc->Name = "CRTC 1";
    Crtc->Id = RHD_CRTC_1;

    Crtc->FBValid = D1FBValid;
    Crtc->FBSet = D1FBSet;
    Crtc->ModeValid = D1ModeValid;
    Crtc->ModeSet = D1ModeSet;
    Crtc->PLLSelect = D1PLLSelect;
    Crtc->LUTSelect = D1LUTSelect;
    Crtc->FrameSet = D1ViewPortStart;

    Crtc->Power = D1Power;

    Crtc->Save = D1Save;
    Crtc->Restore = D1Restore;

    rhdPtr->Crtc[0] = Crtc;

    Crtc = xnfcalloc(sizeof(struct rhd_Crtc), 1);
    Crtc->scrnIndex = rhdPtr->scrnIndex;
    Crtc->Name = "CRTC 2";
    Crtc->Id = RHD_CRTC_2;

    Crtc->FBValid = D1FBValid;
    Crtc->FBSet = D2FBSet;
    Crtc->ModeValid = D2ModeValid;
    Crtc->ModeSet = D2ModeSet;
    Crtc->PLLSelect = D2PLLSelect;
    Crtc->LUTSelect = D2LUTSelect;
    Crtc->FrameSet = D2ViewPortStart;

    Crtc->Power = D2Power;

    Crtc->Save = D2Save;
    Crtc->Restore = D2Restore;

    rhdPtr->Crtc[1] = Crtc;
}

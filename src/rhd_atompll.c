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
#include "rhd_pll.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_crtc.h"
#include "rhd_regs.h"
#include "rhd_atombios.h"
#include "rhd_biosscratch.h"

struct atomPLLPrivate {
    enum atomPxclk Pxclk;
    struct atomPixelClockConfig Config;
    struct atomCodeTableVersion Version;

    CARD32 StoreFBDivFrac;
    enum atomDevice StoreDevice;
    enum rhdConnectorType StoreConnectorType;
    enum rhdOutputType StoreOutputType;
    int StoreCrtc;
};

/*
 *
 */
static void
getSetPixelClockParameters(struct rhdPLL *PLL, struct atomPixelClockConfig *Config,
			   enum rhdConnectorType ct, enum rhdOutputType ot, enum atomDevice device)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;

    switch (Private->Version.cref) {
	case 2:
	    Config->u.v2.Device = device;
	    Config->u.v2.Force = TRUE;
	    break;
	case 3:
	    switch (ct) {
		case RHD_CONNECTOR_DVI:
		case RHD_CONNECTOR_DVI_SINGLE:
		    Config->u.v3.EncoderMode = atomDVI;
		    break;
		case RHD_CONNECTOR_PANEL:
		    Config->u.v3.EncoderMode = atomLVDS;
		    break;
#if 0
		case RHD_CONNECTOR_DP:
		case RHD_CONNECTOR_DP_DUAL:
		    Config->u.v3.EncoderMode = atomDP;
		    break;
		case RHD_CONNECTOR_HDMI_A:
		case RHD_CONNECTOR_HDMI_B:
		    Config->u.v3.EncoderMode = atomHDMI;
		    break;
#endif
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unknown connector type\n");
	    }
	    switch (ot) {
		case RHD_OUTPUT_DACA:
		    Config->u.v3.OutputType = atomOutputDacA;
		    break;
		case RHD_OUTPUT_DACB:
		    Config->u.v3.OutputType = atomOutputDacB;
		    break;
		case RHD_OUTPUT_KLDSKP_LVTMA:
		    Config->u.v3.OutputType = atomOutputKldskpLvtma;
		    break;
		case RHD_OUTPUT_UNIPHYA:
		    Config->u.v3.OutputType = atomOutputUniphyA;
		    break;
		case RHD_OUTPUT_UNIPHYB:
		    Config->u.v3.OutputType = atomOutputUniphyB;
		    break;
		case RHD_OUTPUT_DVO:
		    Config->u.v3.OutputType = atomOutputDvo;
		    break;
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Unhandled ouptut type\n",__func__);
		    break;
	    }
	    Config->u.v3.Force = TRUE;
	    break;
	default:
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unsupported SelectPixelClock version; %i\n",Private->Version.cref);
	    break;
    }
}

/*
 *
 */
static void
rhdAtomPLLSave(struct rhdPLL *PLL, CARD32 PllCntl, CARD32 OwnerVal)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;
    CARD32 Crtc1Cntl, Crtc2Cntl;
    enum atomCrtc owner;
    struct rhdConnector *Connector;
    struct rhdOutput *Output;


    Crtc1Cntl = RHDRegRead(PLL, PCLK_CRTC1_CNTL);
    Crtc2Cntl = RHDRegRead(PLL, PCLK_CRTC2_CNTL);

    if (PllCntl & 0x2)
	PLL->StoreActive = FALSE;
    else
	PLL->StoreActive = TRUE;

    if ((Crtc1Cntl & 0x00010000) == OwnerVal)
	owner = atomCrtc1;
    else if ((Crtc2Cntl & 0x00010000) == OwnerVal)
	owner = atomCrtc2;
    else {
	owner = atomCrtc1; /* whatever... */
	PLL->StoreActive = FALSE;
    }

    Private->StoreCrtc = owner;
    Private->StoreDevice = RHDGetDeviceOnCrtc(rhdPtr, owner);

    if (Private->StoreDevice != atomNone) {
	rhdAtomFindOutputConnectorForDevice(rhdPtr, Private->StoreDevice, &Connector, &Output);
	Private->StoreConnectorType = Connector->Type;
	Private->StoreOutputType = Output->Id;
    } else
	PLL->StoreActive = FALSE;

    RHDDebug(PLL->scrnIndex, "Saving PLL %i on CRTC: %i %s active\n",
	     (PLL->Id == PLL_ID_PLL1) ? 1 : 2,
	     (owner == atomCrtc1) ? 1 : 2,
	     (PLL->StoreActive) ? "" : "not");

    PLL->Stored = TRUE;

    /* Set parameters found at startup for sutdownInactive(). This is somewhat ugly... */
    Private->Config.Crtc = Private->StoreCrtc;
    Private->Config.Enable = PLL->StoreActive;
    if (Private->StoreDevice != atomNone)
	getSetPixelClockParameters(PLL, &Private->Config, Private->StoreConnectorType,
				   Private->StoreOutputType, Private->StoreDevice);
}

/*
 *
 */
static void
rhdAtomPLL1Save(struct rhdPLL *PLL)
{
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;
    CARD32 PllCntl;

    RHDFUNC(PLL);

    PLL->StoreRefDiv   = RHDRegRead(PLL, EXT1_PPLL_REF_DIV) & 0x1FF;
    PLL->StoreFBDiv    = (RHDRegRead(PLL, EXT1_PPLL_FB_DIV) >> 16) & 0x3FF;
    Private->StoreFBDivFrac = RHDRegRead(PLL, EXT1_PPLL_FB_DIV) & 0x7;
    PLL->StorePostDiv  = RHDRegRead(PLL, EXT1_PPLL_POST_DIV) & 0x3F;
    PllCntl = RHDRegRead(PLL, P1PLL_CNTL);

    rhdAtomPLLSave(PLL, PllCntl, 0x00000000);
}


/*
 *
 */
static void
rhdAtomPLL2Save(struct rhdPLL *PLL)
{
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;
    CARD32 PllCntl;

    RHDFUNC(PLL);

    PLL->StoreRefDiv   = RHDRegRead(PLL, EXT2_PPLL_REF_DIV) & 0x1FF;
    PLL->StoreFBDiv    = (RHDRegRead(PLL, EXT2_PPLL_FB_DIV) >> 16) & 0x3FF;
    Private->StoreFBDivFrac = RHDRegRead(PLL, EXT2_PPLL_FB_DIV) & 0x7;
    PLL->StorePostDiv  = RHDRegRead(PLL, EXT2_PPLL_POST_DIV) & 0x3F;
    PllCntl = RHDRegRead(PLL, P2PLL_CNTL);

    rhdAtomPLLSave(PLL, PllCntl, 0x00010000);
}

/*
 *
 */
static void
rhdAtomPLLRestore(struct rhdPLL *PLL)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    struct atomPixelClockConfig Config;
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;

    RHDFUNC(PLL);

    if (!PLL->Stored) {
	xf86DrvMsg(PLL->scrnIndex, X_ERROR, "%s: %s: trying to restore "
		   "uninitialized values.\n", __func__, PLL->Name);
	return;
    }
    Config.PixelClock = PLL->StoreActive
	? ((PLL->StoreFBDiv * PLL->RefClock) / (PLL->StorePostDiv * PLL->StoreRefDiv * 10))
	: 0;

    Config.Enable = PLL->StoreActive;
    Config.RefDiv = PLL->StoreRefDiv;
    Config.FbDiv = PLL->StoreFBDiv;
    Config.PostDiv = PLL->StorePostDiv;
    Config.FracFbDiv = Private->StoreFBDivFrac;
    Config.Crtc = Private->StoreCrtc;

    if (Private->StoreDevice != atomNone)
	getSetPixelClockParameters(PLL, &Config, Private->StoreConnectorType,
				   Private->StoreOutputType, Private->StoreDevice);
    RHDDebug(PLL->scrnIndex, "Restoring PixelClock %i with %i kHz, (%i * %i) / ( %i * %i )"
	     " on CRTC %i device: %x\n",
	     Private->Pxclk, Config.PixelClock, PLL->RefClock, PLL->StoreFBDiv, PLL->StorePostDiv,
	     PLL->StoreRefDiv, Config.Crtc == atomCrtc1 ? 1 : 2, Config.u.v2.Device);
    rhdAtomSetPixelClock(rhdPtr->atomBIOS, Private->Pxclk, &Config);
}

/*
 *
 */
static void
rhdAtomPLLPower(struct rhdPLL *PLL, int Power)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;
    struct atomPixelClockConfig *config = &Private->Config;

    RHDFUNC(PLL);

    switch (Power) {
	case RHD_POWER_ON:
	    if (config->PixelClock > 0)
		config->Enable = TRUE;
	    else {
		xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
			   "%s: cannot enable pixel clock without frequency set\n",__func__);
		config->Enable = FALSE;
	    }
	    break;
	case RHD_POWER_RESET:
	case RHD_POWER_SHUTDOWN:
	    return;
	    config->Enable = FALSE;
	default:
	    break;
    }
    rhdAtomSetPixelClock(rhdPtr->atomBIOS, Private->Pxclk, config);
}

/*
 *
 */
static void
rhdAtomPLLSet(struct rhdPLL *PLL, int PixelClock, CARD16 ReferenceDivider,
	    CARD16 FeedbackDivider, CARD8 PostDivider)
{
    RHDPtr rhdPtr = RHDPTRI(PLL);
    struct atomPLLPrivate *Private = (struct atomPLLPrivate *)PLL->Private;
    struct rhdCrtc *Crtc = NULL;

    RHDFUNC(PLL);
    RHDDebug(rhdPtr->scrnIndex, "%s: %i kHz RefDiv: %x FeedbackDiv: %x PostDiv: %x\n",
	     __func__, PixelClock, ReferenceDivider, FeedbackDivider, PostDivider);
    
    Private->Config.PixelClock = PixelClock;
    Private->Config.RefDiv = ReferenceDivider;
    Private->Config.FbDiv = FeedbackDivider;
    Private->Config.PostDiv = PostDivider;
    Private->Config.FracFbDiv = 0;
    if (rhdPtr->Crtc[0]->PLL == PLL) {
	Private->Config.Crtc = atomCrtc1;
	Crtc = rhdPtr->Crtc[0];
    } else if (rhdPtr->Crtc[1]->PLL == PLL) {
	Private->Config.Crtc = atomCrtc2;
	Crtc = rhdPtr->Crtc[1];
    } else
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Trying to set an unassigned PLL\n");

    if (Crtc && Private->Version.cref > 1) {
	struct rhdOutput *Output;
	for (Output = rhdPtr->Outputs; Output; Output = Output->Next) {
	    if (Output->Crtc == Crtc)
		break;
	}
	if (Output)
	    getSetPixelClockParameters(PLL, &Private->Config,
				      Output->Connector->Type, Output->Id,
				      Output->OutputDriverPrivate->Device);
    }
    Private->Config.Enable = TRUE;
    rhdAtomSetPixelClock(rhdPtr->atomBIOS, Private->Pxclk, &Private->Config);
}

/*
 *
 */
Bool
RHDAtomPLLsInit(RHDPtr rhdPtr)
{
    struct rhdPLL *PLL;
    struct atomPLLPrivate *Private;
    CARD32 RefClock, IntMin, IntMax, PixMin, PixMax;
    AtomBiosArgRec arg;
    int i;

    RHDFUNC(rhdPtr);

    RHDSetupLimits(rhdPtr, &RefClock, &IntMin, &IntMax, &PixMin, &PixMax);

    for (i = 0; i < 2; i++) {

	PLL = (struct rhdPLL *) xnfcalloc(sizeof(struct rhdPLL), 1);
	Private = (struct atomPLLPrivate *) xnfcalloc(sizeof(struct atomPLLPrivate),1);
	PLL->Private = Private;

	Private->Version = rhdAtomSetPixelClockVersion(rhdPtr->atomBIOS);
	if (Private->Version.cref > 3) {
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unsupported SelectPixelClock version; %i\n",
		       Private->Version.cref);
	    xfree(PLL->Private);
	    xfree(PLL);
	    return FALSE;
	}

	PLL->scrnIndex = rhdPtr->scrnIndex;
	if (i == 0) {
	    PLL->Name = PLL_NAME_PLL1;
	    PLL->Id = PLL_ID_PLL1;
	    PLL->Save = rhdAtomPLL1Save;
	    Private->Pxclk = atomPclk1;
	} else {
	    PLL->Name = PLL_NAME_PLL2;
	    PLL->Id = PLL_ID_PLL2;
	    PLL->Save = rhdAtomPLL2Save;
	    Private->Pxclk = atomPclk2;
	}

	PLL->RefClock = RefClock;
	PLL->IntMin = IntMin;
	PLL->IntMax = IntMax;
	PLL->PixMin = PixMin;
	PLL->PixMax = PixMax;

	PLL->Valid = NULL;

	PLL->Set = rhdAtomPLLSet;
	PLL->Power = rhdAtomPLLPower;
	PLL->Restore = rhdAtomPLLRestore;

	rhdPtr->PLLs[i] = PLL;
    }

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS, GET_REF_CLOCK, &arg))
	PLL->RefClock = arg.val;
    else
	PLL->RefClock = 0x2700;

    return TRUE;
}

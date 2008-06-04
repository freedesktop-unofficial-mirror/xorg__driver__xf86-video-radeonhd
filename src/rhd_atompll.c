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
#include "rhd_regs.h"
#include "rhd_atombios.h"

struct atomPLLPrivate {
    enum atomPxclk Pxclk;
    struct atomPixelClockConfig Config;
    struct atomCodeTableVersion Version;
};

/*
 *
 */
static void
rhdAtomPLLSave(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    PLL->Stored = TRUE;
}

/*
 *
 */
static void
rhdAtomPLLRestore(struct rhdPLL *PLL)
{
    RHDFUNC(PLL);

    if (!PLL->Stored) {
	xf86DrvMsg(PLL->scrnIndex, X_ERROR, "%s: %s: trying to restore "
		   "uninitialized values.\n", __func__, PLL->Name);
	return;
    }
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
		config->enable = TRUE;
	    else {
		xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
			   "%s: cannot enable pixel clock without frequency set\n",__func__);
		config->enable = FALSE;
	    }
	    break;
	case RHD_POWER_RESET:
	case RHD_POWER_SHUTDOWN:
	    config->enable = FALSE;
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

    Private->Config.PixelClock = PixelClock / 10;
    Private->Config.refDiv = ReferenceDivider;
    Private->Config.fbDiv = FeedbackDivider;
    Private->Config.postDiv = PostDivider;
    Private->Config.fracFbDiv = 0;
    if (rhdPtr->Crtc[0]->PLL == PLL) {
	Private->Config.Crtc = atomCrtc1;
	Crtc = rhdPtr->Crtc[0];
    } else if (rhdPtr->Crtc[1]->PLL == PLL) {
	Private->Config.Crtc = atomCrtc2;
	Crtc = rhdPtr->Crtc[0];
    } else
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Trying to set an unassigned PLL\n");

    if (Crtc && Private->Version.cref > 1) {
	struct rhdOutput *Output;
	for (Output = rhdPtr->Outputs; Output; Output = Output->Next) {
	    if (Output->Crtc == Crtc)
		break;
	}
	if (Output) {
	    switch (Private->Version.cref) {
		case 2:
		    Private->Config.u.v2.deviceIndex = Output->OutputDriverPrivate->Device;
		    Private->Config.u.v2.force = TRUE;
		    break;
		case 3:
		    switch (Output->Connector->Type) {
			case RHD_CONNECTOR_DVI:
			case RHD_CONNECTOR_DVI_SINGLE:
			    Private->Config.u.v3.EncoderMode = atomDVI_1Link;
			    break;
			case RHD_CONNECTOR_PANEL:
			    Private->Config.u.v3.EncoderMode = atomLVDS;
			    break;
#if 0
			case RHD_CONNECTOR_DP:
			case RHD_CONNECTOR_DP_DUAL:
			    Private->Config.u.v3.EncoderMode = atomDP;
			    break;
			case RHD_CONNECTOR_HDMI_A:
			case RHD_CONNECTOR_HDMI_B:
			    Private->Config.u.v3.EncoderMode = atomHDMI;
			    break;
#endif
			default:
			    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unknown connector type\n");
		    }
		    switch (Output->Id) {
			case RHD_OUTPUT_DACA:
			    Private->Config.u.v3.OutputType = atomOutputDacA;
			    break;
			case RHD_OUTPUT_DACB:
			    Private->Config.u.v3.OutputType = atomOutputDacB;
			    break;
			case RHD_OUTPUT_KLDSKP_LVTMA:
			    Private->Config.u.v3.OutputType = atomOutputKldskpLvtma;
			    break;
			case RHD_OUTPUT_UNIPHYA:
			    Private->Config.u.v3.OutputType = atomOutputUniphyA;
			    break;
			case RHD_OUTPUT_UNIPHYB:
			    Private->Config.u.v3.OutputType = atomOutputUniphyB;
			    break;
			case RHD_OUTPUT_DVO:
			    Private->Config.u.v3.OutputType = atomOutputDvo;
			    break;
			default:
			    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Unhandled ouptut type\n",__func__);
			    break;
		    }
		    Private->Config.u.v3.force = TRUE;
		    break;
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unsupported SelectPixelClock version; %i\n",Private->Version.cref);
		    break;
	    }
	}
    }
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
    int i;

    RHDFUNC(rhdPtr);

    RHDSetupLimits(rhdPtr, &RefClock, &IntMin, &IntMax, &PixMin, &PixMax);

    for (i = 0; i < 2; i++) {
	PLL = (struct rhdPLL *) xnfcalloc(sizeof(struct rhdPLL), 1);
	Private = (struct atomPLLPrivate *) xnfcalloc(sizeof(struct atomPLLPrivate),1);
	PLL->Private = Private;
	Private->Version = rhdAtomSetPixelClockVersion(rhdPtr->atomBIOS);
	if (Private->Version.cref > 3) {
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unsupported SelectPixelClock version; %i\n",Private->Version.cref);
	    xfree(PLL->Private);
	    xfree(PLL);
	    return FALSE;
	}
	PLL->scrnIndex = rhdPtr->scrnIndex;
	if (i == 0) {
	    PLL->Name = PLL_NAME_PLL1;
	    PLL->Id = PLL_ID_PLL1;
	    Private->Pxclk = atomPclk1;
	} else {
	    PLL->Name = PLL_NAME_PLL2;
	    PLL->Id = PLL_ID_PLL2;
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
	PLL->Save = rhdAtomPLLSave;
	PLL->Restore = rhdAtomPLLRestore;

	rhdPtr->PLLs[i] = PLL;
    }

    return TRUE;
}

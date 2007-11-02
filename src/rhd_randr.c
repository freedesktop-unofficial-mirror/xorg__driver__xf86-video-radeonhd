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
 * RandR interface.
 *
 * Only supports RandR 1.2 ATM.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Xserver interface */
#include "xf86.h"
#ifdef RANDR
# include "randrstr.h"
#endif

/* Driver specific headers */
#include "rhd.h"
#include "rhd_randr.h"


#ifdef RANDR_12_INTERFACE


/* Xserver interface */
#include "xf86i2c.h"		/* Missing in old versions of xf86Crtc.h */
#include "xf86Crtc.h"
#include "xf86Parser.h"
#define DPMS_SERVER
#include "X11/extensions/dpms.h"

/* Driver specific headers */
#include "rhd_crtc.h"
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_modes.h"
#include "rhd_monitor.h"
#include "rhd_vga.h"
#include "rhd_pll.h"

/* System headers */
#ifndef _XF86_ANSIC_H
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#endif


/*
 * Driver internal data
 */

/* Outputs and Connectors are combined for RandR due to missing abstraction */
struct rhdRandr {
    xf86CrtcPtr    RandrCrtc[2];
    xf86OutputPtr *RandrOutput;  /* NULL-terminated */
} ;

/* Driver private for Randr outputs due to missing abstraction */
typedef struct _rhdRandrOutput {
    char                 Name[64];
    struct rhdConnector *Connector;
    struct rhdOutput    *Output;
} rhdRandrOutputRec, *rhdRandrOutputPtr;


/*
 * xf86CrtcConfig callback functions
 */

static Bool
rhdRRXF86CrtcResize(ScrnInfoPtr pScrn, int width, int height)
{
    RHDFUNC(pScrn);
    pScrn->virtualX = width;
    pScrn->virtualY = height; 
    return TRUE;
}

/*
 * xf86Crtc callback functions
 */

/* Turns the crtc on/off, or sets intermediate power levels if available. */
static void
rhdRRCrtcDpms(xf86CrtcPtr crtc, int mode)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    struct rhdCrtc *Crtc = (struct rhdCrtc *) crtc->driver_private;

    RHDDebug(Crtc->scrnIndex, "%s: Crtc %d : %s\n", __func__,
	     Crtc==rhdPtr->Crtc[0]?0:1,
	     mode==DPMSModeOn ? "On" : mode==DPMSModeOff ? "Off" : "Other");

    switch (mode) {
    case DPMSModeOn:
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_ON);
	Crtc->Power(Crtc, RHD_POWER_ON);
	break;
    case DPMSModeSuspend:
    case DPMSModeStandby:
	Crtc->Power(Crtc, RHD_POWER_RESET);
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_RESET);
	break;
    case DPMSModeOff:
	Crtc->Power(Crtc, RHD_POWER_SHUTDOWN);
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_SHUTDOWN);
	break;
    default:
	ASSERT(!"Unknown DPMS mode");
    }
}

/* Lock CRTC prior to mode setting, mostly for DRI.
 * Returns whether unlock is needed */
static Bool
rhdRRCrtcLock (xf86CrtcPtr crtc)
{
    return FALSE;
}

#if 0
/**
 * Unlock CRTC after mode setting, mostly for DRI
 */
void
(*unlock) (xf86CrtcPtr crtc);

/**
 * Callback to adjust the mode to be set in the CRTC.
 *
 * This allows a CRTC to adjust the clock or even the entire set of
 * timings, which is used for panels with fixed timings or for
 * buses with clock limitations.
 *
 * This call gives the CRTC a chance to see what mode will be set and to
 * comment on the mode by changing 'adjusted_mode' as needed. This function
 * shall not modify the state of the crtc hardware at all. If the CRTC cannot
 * accept this mode, this function may return FALSE.
 */
static Bool
rhdRRCrtcModeFixup(xf86CrtcPtr    crtc, 
		   DisplayModePtr mode,
		   DisplayModePtr adjusted_mode)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    RHDFUNC(rhdPtr);
    return FALSE;
}
#endif

/* Dummys, because they are not tested for NULL */
static void
rhdRRCrtcModeSetDUMMY(xf86CrtcPtr    crtc, 
		      DisplayModePtr mode,
		      DisplayModePtr adjusted_mode,
		      int x, int y)
{ }
static Bool
rhdRRCrtcModeFixupDUMMY(xf86CrtcPtr    crtc, 
			DisplayModePtr mode,
			DisplayModePtr adjusted_mode)
{ return TRUE; }
static void
rhdRRCrtcPrepareDUMMY(xf86CrtcPtr crtc)
{ }
static void
rhdRRCrtcCommitDUMMY(xf86CrtcPtr crtc)
{ }

#if 0
/* Set the color ramps for the CRTC to the given values. */
static void
rhdRRCrtcGammaSet(xf86CrtcPtr crtc,
		 CARD16 *red, CARD16 *green, CARD16 *blue, int size)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    RHDFUNC(rhdPtr);
}

    void
    crtc->funcs->prepare (xf86CrtcPtr crtc)

This call is made just before the mode is set to make the hardware ready for
the operation. A usual function to perform here is to disable the crtc so
that mode setting can occur with clocks turned off and outputs deactivated.

    void
    crtc->funcs->commit (xf86CrtcPtr crtc)

Once the mode has been applied to the CRTC and Outputs, this function is
invoked to let the hardware turn things back on.

    void *
    crtc->funcs->shadow_allocate (xf86CrtcPtr crtc, int width, int height)

This function allocates frame buffer space for a shadow frame buffer. When
allocated, the crtc must scan from the shadow instead of the main frame
buffer. This is used for rotation. The address returned is passed to the
shadow_create function. This function should return NULL on failure.

    PixmapPtr
    crtc->funcs->shadow_create (xf86CrtcPtr crtc, void *data,
                                int width, int height)

This function creates a pixmap object that will be used as a shadow of the
main frame buffer for CRTCs which are rotated or reflected. 'data' is the
value returned by shadow_allocate.

    void
    crtc->funcs->shadow_destroy (xf86CrtcPtr crtc, PixmapPtr pPixmap,
                                 void *data)

Destroys any associated shadow objects. If pPixmap is NULL, then a pixmap
was not created, but 'data' may still be non-NULL indicating that the shadow
had been allocated.

#endif


/*
 * xf86Output callback functions
 */

/* Turns the output on/off, or sets intermediate power levels if available. */
static void
rhdRROutputDpms(xf86OutputPtr       out,
		int                 mode)
{
    rhdRandrOutputPtr rout   = (rhdRandrOutputPtr) out->driver_private;

    RHDDebug(rout->Output->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name,
	     mode==DPMSModeOn ? "On" : mode==DPMSModeOff ? "Off" : "Other");

    switch (mode) {
    case DPMSModeOn:
	rout->Output->Power(rout->Output, RHD_POWER_ON);
	break;
    case DPMSModeSuspend:
    case DPMSModeStandby:
	rout->Output->Power(rout->Output, RHD_POWER_RESET);
	break;
    case DPMSModeOff:
	rout->Output->Power(rout->Output, RHD_POWER_SHUTDOWN);
	break;
    default:
	ASSERT(!"Unknown DPMS mode");
    }
}

/* Helper: setup Crtc for validation & fixup */
static Bool
setupCrtc(RHDPtr rhdPtr, struct rhdCrtc *Crtc, struct rhdOutput *Output,
	  DisplayModePtr Mode)
{
    int i;

    /* ATM: if already assigned, use the same */
    if (Output->Crtc == Crtc)
	return TRUE;
    ASSERT(!Output->Crtc || !Output->Crtc->Active);
    ASSERT(!Output->Active);
    ASSERT(!Crtc->Active);

    /* PLL & LUT setup - static at the moment */
    Output->Crtc = Crtc;
    for (i = 0; i < 2; i++)
	if (Crtc == rhdPtr->Crtc[i])
	    break;
    ASSERT(i<2);
    Crtc->PLL = rhdPtr->PLLs[i];
    Crtc->LUT = rhdPtr->LUT[i];

    return TRUE;
}

/* RandR has a weird sense of how validation and fixup should be handled:
 * - Fixup and validation can interfere, they would have to be looped
 * - Validation should be done after fixup
 * - There is no crtc validation AT ALL
 * - The necessity of adjusted_mode is questionable
 * - Outputs and crtcs are checked independently, and one at a time.
 *   This can interfere, the driver should know the complete setup for
 *   validation and fixup.
 *   We also cannot emulate, because we never know when validation is finished.
 * Therefore we only use a single function for all and have to work around
 * not knowing what the other crtcs might be needed for. */
static int
rhdRROutputModeValid(xf86OutputPtr  out,
		     DisplayModePtr Mode)
{
    RHDPtr             rhdPtr = RHDPTR(out->scrn);
    ScrnInfoPtr        pScrn = xf86Screens[rhdPtr->scrnIndex];
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    int                Status;

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name, Mode->name);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    /* If out->crtc is not NULL, it is not necessarily the Crtc that will
     * be used, so let's better skip crtc based checks... */
    Status = RHDRRModeFixup(out->scrn, Mode, NULL, rout->Connector,
			    rout->Output, NULL);	// TODO: Monitor
    RHDDebug(rhdPtr->scrnIndex, "%s: %s -> Status %d\n", __func__,
	     Mode->name, Status);
    return Status;
}

static Bool
rhdRROutputModeFixup(xf86OutputPtr  out,
		     DisplayModePtr OrigMode,
		     DisplayModePtr Mode)
{
    RHDPtr             rhdPtr = RHDPTR(out->scrn);
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc    *Crtc   = NULL;

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name, Mode->name);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    if (out->crtc)
	Crtc = (struct rhdCrtc *) out->crtc->driver_private;

    if (! setupCrtc(rhdPtr, Crtc, rout->Output, Mode) )
	return FALSE;
    
    if (RHDRRModeFixup(out->scrn, Mode, Crtc, rout->Connector, rout->Output,
		       NULL)	// TODO: Monitor
	!= MODE_OK)
	return FALSE;
    return TRUE;
}

/* Set video mode.
 * Again, randr calls for Crtc and Output separately, while order should be
 * up to the driver. So let's do everything here. */
static void
rhdRROutputModeSet(xf86OutputPtr  out,
		   DisplayModePtr OrigMode,
		   DisplayModePtr Mode)
{
    RHDPtr rhdPtr = RHDPTR(out->scrn);
    ScrnInfoPtr pScrn = xf86Screens[rhdPtr->scrnIndex];
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc    *Crtc   = NULL;
    struct rhdMonitor *Monitor = NULL;

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name, Mode->name);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);
    ASSERT(out->crtc);
    Crtc = (struct rhdCrtc *) out->crtc->driver_private;

    Crtc->Active = TRUE;
    rout->Output->Active = TRUE;
    rout->Output->Crtc   = Crtc;
    rout->Output->Connector = rout->Connector;

    /* TODO: what of the following to mode setup, what to validate / fixup */
    Monitor = RHDMonitorInit(rout->Connector);

    if (!Monitor && (rout->Connector->Type == RHD_CONNECTOR_PANEL)) {
	// TODO: do this in fixup?!?
	xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "Unable to attach a"
		   " monitor to connector \"%s\"\n", rout->Connector->Name);
	rout->Output->Active = FALSE;
    } else {
	rout->Connector->Monitor = Monitor;
	if (Monitor) {
	    /* If this is a digitally attached monitor, enable
	     * * reduced blanking.
	     * * TODO: iiyama vm pro 453: CRT with DVI-D == No reduced.
	     * */
	    if ((rout->Output->Id == RHD_OUTPUT_TMDSA) ||
		(rout->Output->Id == RHD_OUTPUT_LVTMA))
		Monitor->ReducedAllowed = TRUE;

#if 0
	    /* allow user to override settings globally */
	    if (ForceReduced.set)
		Monitor->ReducedAllowed = ForceReduced.val.bool;
#endif

	    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
		       "Connector \"%s\" uses Monitor \"%s\":\n",
		       rout->Connector->Name, Monitor->Name);
	    RHDMonitorPrint(Monitor);
	} else
	    xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING,
		       "Connector \"%s\": Failed to retrieve Monitor"
		       " information.\n", rout->Connector->Name);
 }

    /* Bitbanging */
    pScrn->vtSema = TRUE;

    /* no active output == no mess */
    rout->Output->Power(rout->Output, RHD_POWER_RESET);

    /* Disable CRTCs to stop noise from appearing. */
    // TODO: necessary? output is off...
    Crtc->Power(Crtc, RHD_POWER_RESET);

    /* now disable our VGA Mode */
    // TODO: only once
    RHDVGADisable(rhdPtr);

    /* Set up D1 and appendages */
    Crtc->FBSet(Crtc, pScrn->displayWidth, pScrn->virtualX, pScrn->virtualY,
		pScrn->depth, rhdPtr->FbFreeStart);
    Crtc->ModeSet(Crtc, Mode);
    RHDPLLSet(Crtc->PLL, Mode->Clock);		/* This also powers up PLL */
    Crtc->PLLSelect(Crtc, Crtc->PLL);
    Crtc->LUTSelect(Crtc, Crtc->LUT);
    RHDOutputsMode(rhdPtr, Crtc);
    Crtc->Power(Crtc, RHD_POWER_ON);
    rout->Output->Power(rout->Output, RHD_POWER_ON);

    /* TODO: necessary? dpms should do that. shut down that what we don't use */
    //RHDPLLsShutdownInactive(rhdPtr);
}

/* Probe for a connected output, and return detect_status. */
static xf86OutputStatus
rhdRROutputDetect(xf86OutputPtr output)
{
    RHDPtr rhdPtr = RHDPTR(output->scrn);
    rhdRandrOutputPtr rout = (rhdRandrOutputPtr) output->driver_private;

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s\n", __func__, rout->Name);
    if (rout->Output->Sense) {
	if (rout->Output->Sense(rout->Output, rout->Connector->Type))
	    return XF86OutputStatusConnected;
	else
	    return XF86OutputStatusDisconnected;
    }
    return XF86OutputStatusUnknown;
}

/* Query the device for the modes it provides. Set MonInfo, mm_width/height. */
static DisplayModePtr
rhdRROutputGetModes(xf86OutputPtr output)
{
    RHDPtr            rhdPtr = RHDPTR(output->scrn);
    rhdRandrOutputPtr rout = (rhdRandrOutputPtr) output->driver_private;
    xf86MonPtr	      edid_mon;

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s\n", __func__, rout->Name);
    edid_mon = xf86OutputGetEDID (output, rout->Connector->DDC);
    xf86OutputSetEDID (output, edid_mon);
    
    return xf86OutputGetEDIDModes (output);
}

#if 0
    /**
     * Callback when an output's property has changed.
     */
    Bool
    (*set_property)(xf86OutputPtr output,
		    Atom property,
		    RRPropertyValuePtr value);
#endif

/* Dummys, because they are not tested for NULL */
static void
rhdRROutputPrepareDUMMY(xf86OutputPtr out)
{ }
static void
rhdRROutputCommitDUMMY(xf86OutputPtr out)
{ }


/*
 * Xorg Interface
 */

static const xf86CrtcConfigFuncsRec rhdRRCrtcConfigFuncs = {
    rhdRRXF86CrtcResize
};

static const xf86CrtcFuncsRec rhdRRCrtcFuncs = {
    rhdRRCrtcDpms,
    /*rhdRRCrtcSave*/ NULL, /*rhdRRCrtcRestore*/ NULL,
    rhdRRCrtcLock, /*rhdRRCrtcUnlock*/ NULL,
    rhdRRCrtcModeFixupDUMMY,
    rhdRRCrtcPrepareDUMMY, rhdRRCrtcModeSetDUMMY, rhdRRCrtcCommitDUMMY,
    /*rhdRRCrtcGammaSet*/ NULL,
    /*rhdRRCrtcShadowAllocate, rhdRRCrtcShadowCreate, rhdRRCrtcShadowDestroy,*/ NULL, NULL, NULL,
    /*set_cursor_colors*/ NULL, /*set_cursor_position*/ NULL,
    /*show_cursor*/ NULL, /*hide_cursor*/ NULL,
    /*load_cursor_image*/ NULL, /*load_cursor_argb*/ NULL,
    /*rhdRRCrtcDestroy*/ NULL
};

static const xf86OutputFuncsRec rhdRROutputFuncs = {
    /*create_resources*/ NULL, rhdRROutputDpms,
    /*rhdRROutputSave*/ NULL, /*rhdRROutputRestore*/ NULL,
    rhdRROutputModeValid, rhdRROutputModeFixup,
    rhdRROutputPrepareDUMMY, rhdRROutputCommitDUMMY,
    rhdRROutputModeSet, rhdRROutputDetect, rhdRROutputGetModes,
#ifdef RANDR_12_INTERFACE
    /*rhdRROutputSetProperty*/ NULL,
#endif
    /*destroy*/ NULL
};

/* Helper: Create xf86OutputRec with unique name per Connector/Output combo */
static xf86OutputPtr
createRandrOutput(ScrnInfoPtr pScrn,
		  struct rhdConnector *conn, struct rhdOutput *out)
{
    char *c;
    xf86OutputPtr xo;
    rhdRandrOutputPtr rro;

    ASSERT (conn);
    ASSERT (out);
    rro = xnfcalloc(1, sizeof(rhdRandrOutputRec));
    rro->Connector = conn;
    rro->Output    = out;
    sprintf(rro->Name, "%.30s/%.30s", conn->Name, out->Name);
    for (c = rro->Name; *c; c++)
	if (isspace(*c))
	    *c='_';
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "RandR: Added Connector %s\n", rro->Name);
    xo = xf86OutputCreate(pScrn, &rhdRROutputFuncs, rro->Name);
    ASSERT(xo);
    xo->driver_private = rro;
    xo->possible_crtcs  = ~0;		/* No limitations */
    xo->possible_clones = ~0;		/* No limitations */
    xo->interlaceAllowed = FALSE;
    xo->doubleScanAllowed = FALSE;
    xo->subpixel_order = SubPixelUnknown;
    xo->use_screen_monitor = FALSE;
    return xo;
}

/* Call in PreInit after connectors and outputs have been set up */
Bool
RHDRandrPreInit(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhdRandr *randr;
    int i, j, numCombined = 0;
    xf86OutputPtr *RandrOutput;

    RHDFUNC(rhdPtr);
    if (rhdPtr->noRandr.val.bool) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "RandR 1.2 support disabled due to configuration\n");
	return FALSE;
    }

    randr = xnfcalloc(1, sizeof(struct rhdRandr));
    xf86CrtcConfigInit(pScrn, &rhdRRCrtcConfigFuncs);
    /* Maximum for D1GRPH_X/Y_END: 8k */
    xf86CrtcSetSizeRange (pScrn, 320, 200, 8000, 8000);

    for (i = 0; i < 2; i++) {
	randr->RandrCrtc[i] = xf86CrtcCreate(pScrn, &rhdRRCrtcFuncs);
	ASSERT(randr->RandrCrtc[i]);
	randr->RandrCrtc[i]->driver_private = rhdPtr->Crtc[i];
    }

    /* First count, then allocate */
    for (i = 0; i < RHD_CONNECTORS_MAX; i++) {
	struct rhdConnector *conn = rhdPtr->Connector[i];
	if (conn)
	    for (j = 0; j < MAX_OUTPUTS_PER_CONNECTOR; j++) {
		struct rhdOutput *out = conn->Output[j];
		if (out)
		    numCombined++;
	    }
    }
    randr->RandrOutput = RandrOutput = 
	xnfcalloc(numCombined+1, sizeof(struct xf86OutputPtr *));

    /* Create combined unique output names */
    for (i = 0; i < RHD_CONNECTORS_MAX; i++) {
	struct rhdConnector *conn = rhdPtr->Connector[i];
	if (conn) {
	    for (j = 0; j < MAX_OUTPUTS_PER_CONNECTOR; j++) {
		struct rhdOutput *out = conn->Output[j];
		if (out)
		    *RandrOutput++ = createRandrOutput(pScrn, conn, out);
	    }
	}
    }
    *RandrOutput = NULL;

    if (!xf86InitialConfiguration(pScrn, FALSE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "RandR: No valid modes.\n");
	return FALSE;
    }
    rhdPtr->randr = randr;
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "RandR 1.2 support enabled, but not finished yet\n");

    return TRUE;
}

/* Call in ScreenInit after frame buffer + acceleration layers */
Bool
RHDRandrScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    RHDFUNC(rhdPtr);
    if (!xf86DiDGAInit(pScreen, (unsigned long) rhdPtr->FbBase)) // TODO: support or not?
	return FALSE;
    if (!xf86CrtcScreenInit(pScreen))
	return FALSE;
    return TRUE;
}

Bool
RHDRandrModeInit(ScrnInfoPtr pScrn)
{
    return xf86SetDesiredModes(pScrn);			// TODO: needed or not?
}

Bool
RHDRandrSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}


#else /* RANDR_12_INTERFACE */


Bool
RHDRandrPreInit(ScrnInfoPtr pScrn)
{
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "RandR 1.2 support disabled (not available at compile time)\n");
    return FALSE;
}

Bool
RHDRandrScreenInit(ScreenPtr pScreen)
{ ASSERT(!"No RandR 1.2 support on compile time"); return FALSE; }

Bool
RHDRandrModeInit(ScrnInfoPtr pScrn)
{ ASSERT(!"No RandR 1.2 support on compile time"); return FALSE; }

Bool
RHDRandrSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{ ASSERT(!"No RandR 1.2 support on compile time"); return FALSE; }

#endif /* RANDR_12_INTERFACE */


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
 * Only supports RandR 1.2 ATM or no RandR at all.
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
#include "rhd.h"
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

/* List of allocated RandR Crtc and Output description structs */
struct rhdRandr {
    xf86CrtcPtr    RandrCrtc[2];
    xf86OutputPtr *RandrOutput;  /* NULL-terminated */
} ;

/* Outputs and Connectors are combined for RandR due to missing abstraction */
typedef struct _rhdRandrOutput {
    char                 Name[64];
    struct rhdConnector *Connector;
    struct rhdOutput    *Output;
} rhdRandrOutputRec, *rhdRandrOutputPtr;


/* Debug: print out state */
void
RHDDebugRandrState (RHDPtr rhdPtr, const char *msg)
{
    int i;
    xf86OutputPtr *ro;
    ErrorF("*** State at %s:\n", msg);
    for (i = 0; i < 2; i++) {
	xf86CrtcPtr    rc = rhdPtr->randr->RandrCrtc[i];
	struct rhdCrtc *c = (struct rhdCrtc *) rc->driver_private;
	ErrorF("   RRCrtc #%d [rhd %s]: active %d [%d]  mode %s (%dx%d)\n",
	       i, c->Name, rc->enabled, c->Active,
	       rc->mode.name ? rc->mode.name : "unnamed",
	       rc->mode.HDisplay, rc->mode.VDisplay);
    }
    for (ro = rhdPtr->randr->RandrOutput; *ro; ro++) {
	rhdRandrOutputPtr o = (rhdRandrOutputPtr) (*ro)->driver_private;
	ErrorF("   RROut  '%s':  Crtc %s [%s]  active [%d]  %s\n",
	       (*ro)->name,
	       (*ro)->crtc ? ((struct rhdCrtc *)(*ro)->crtc->driver_private)->Name : "null",
	       o->Output->Crtc ? o->Output->Crtc->Name : "null",
	       o->Output->Active,
	       (*ro)->status == XF86OutputStatusConnected ? "connected" :
	       (*ro)->status == XF86OutputStatusDisconnected ? "disconnected" :
	       (*ro)->status == XF86OutputStatusUnknown ? "unknownState" :
	       "badState" );
    }
}

/*
 * xf86CrtcConfig callback functions
 */

static Bool
rhdRRXF86CrtcResize(ScrnInfoPtr pScrn, int width, int height)
{
    RHDFUNC(pScrn);
    /* This is strange... if we set virtualX/virtualY like the intel driver
     * does, we limit ourself in the future to this maximum size.
     * The check for this is internally in RandR, no idea why the intel driver
     * actually works this way...
     * Even more curious: if we DON'T update virtual, everything seems to
     * work as expected... */
#if 0
    pScrn->virtualX = width;
    pScrn->virtualY = height; 
#endif
    return TRUE;
}


/*
 * xf86Crtc callback functions
 */

/* Turns the crtc on/off, or sets intermediate power levels if available. */
static void
rhdRRCrtcDpms(xf86CrtcPtr crtc, int mode)
{
    RHDPtr rhdPtr        = RHDPTR(crtc->scrn);
    struct rhdCrtc *Crtc = (struct rhdCrtc *) crtc->driver_private;

    RHDDebug(Crtc->scrnIndex, "%s: %s: %s\n", __func__, Crtc->Name,
	     mode==DPMSModeOn ? "On" : mode==DPMSModeOff ? "Off" : "Other");

    switch (mode) {
    case DPMSModeOn:
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_ON);
	Crtc->Power(Crtc, RHD_POWER_ON);
	Crtc->Active = TRUE;
	break;
    case DPMSModeSuspend:
    case DPMSModeStandby:
	Crtc->Power(Crtc, RHD_POWER_RESET);
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_RESET);
	Crtc->Active = FALSE;
	break;
    case DPMSModeOff:
	Crtc->Power(Crtc, RHD_POWER_SHUTDOWN);
	if (Crtc->PLL)
	    Crtc->PLL->Power(Crtc->PLL, RHD_POWER_SHUTDOWN);
	Crtc->Active = FALSE;
	break;
    default:
	ASSERT(!"Unknown DPMS mode");
    }
    RHDDebugRandrState(rhdPtr, "POST-CrtcDpms");
}

/* Lock CRTC prior to mode setting, mostly for DRI.
 * Returns whether unlock is needed */
static Bool
rhdRRCrtcLock(xf86CrtcPtr crtc)
{
    return FALSE;
}

#if 0
/* Unlock CRTC after mode setting, mostly for DRI */
static void
rhdRRCrtcUnlock (xf86CrtcPtr crtc)
{ }
#endif

/* Helper: setup PLL and LUT for Crtc */
static void
setupCrtc(RHDPtr rhdPtr, struct rhdCrtc *Crtc)
{
    int i;

    /* PLL & LUT setup - static at the moment */
    if (Crtc->PLL)
	return;
    for (i = 0; i < 2; i++)
	if (Crtc == rhdPtr->Crtc[i])
	    break;
    ASSERT(i<2);
    Crtc->PLL = rhdPtr->PLLs[i];
    Crtc->LUT = rhdPtr->LUT[i];
}

/* Set video mode.
 * randr calls for Crtc and Output separately, while order should be
 * up to the driver. Well. */
static void
rhdRRCrtcPrepare(xf86CrtcPtr crtc)
{
    RHDPtr          rhdPtr = RHDPTR(crtc->scrn);
    ScrnInfoPtr     pScrn  = xf86Screens[rhdPtr->scrnIndex];
    struct rhdCrtc *Crtc   = (struct rhdCrtc *) crtc->driver_private;

    RHDFUNC(rhdPtr);
    setupCrtc(rhdPtr, Crtc);
    pScrn->vtSema = TRUE;

    /* Disable CRTCs to stop noise from appearing. */
    Crtc->Power(Crtc, RHD_POWER_RESET);

    /* now disable our VGA Mode */
    RHDVGADisable(rhdPtr);
}
static void
rhdRRCrtcModeSet(xf86CrtcPtr  crtc,
		 DisplayModePtr OrigMode, DisplayModePtr Mode,
		 int x, int y)
{
    RHDPtr          rhdPtr = RHDPTR(crtc->scrn);
    ScrnInfoPtr     pScrn  = xf86Screens[rhdPtr->scrnIndex];
    struct rhdCrtc *Crtc   = (struct rhdCrtc *) crtc->driver_private;

    /* RandR may give us a mode without a name... (xf86RandRModeConvert) */
    if (!Mode->name && crtc->mode.name)
	Mode->name = xstrdup(crtc->mode.name);

    RHDDebug(rhdPtr->scrnIndex, "%s: %s : %s\n", __func__,
	     Crtc->Name, Mode->name);

    /* Set up mode */
    Crtc->FBSet(Crtc, pScrn->displayWidth, pScrn->virtualX, pScrn->virtualY,
		pScrn->depth, rhdPtr->FbFreeStart);
    Crtc->FrameSet(Crtc, x, y);
    Crtc->ModeSet(Crtc, Mode);
    RHDPLLSet(Crtc->PLL, Mode->Clock);		/* This also powers up PLL */
    Crtc->PLLSelect(Crtc, Crtc->PLL);
    Crtc->LUTSelect(Crtc, Crtc->LUT);
}
static void
rhdRRCrtcCommit(xf86CrtcPtr crtc)
{
    RHDPtr          rhdPtr = RHDPTR(crtc->scrn);
    struct rhdCrtc *Crtc   = (struct rhdCrtc *) crtc->driver_private;

    RHDFUNC(rhdPtr);

    Crtc->Active = TRUE;
    Crtc->Power(Crtc, RHD_POWER_ON);

    RHDDebugRandrState(rhdPtr, Crtc->Name);
}


/* Dummy, because not tested for NULL */
static Bool
rhdRRCrtcModeFixupDUMMY(xf86CrtcPtr    crtc, 
			DisplayModePtr mode,
			DisplayModePtr adjusted_mode)
{ return TRUE; }
#if 0
/* Set the color ramps for the CRTC to the given values. */
static void
rhdRRCrtcGammaSet(xf86CrtcPtr crtc,
		 CARD16 *red, CARD16 *green, CARD16 *blue, int size)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    RHDFUNC(rhdPtr);
}

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
    RHDPtr rhdPtr          = RHDPTR(out->scrn);
    rhdRandrOutputPtr rout = (rhdRandrOutputPtr) out->driver_private;

    RHDDebug(rout->Output->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name,
	     mode==DPMSModeOn ? "On" : mode==DPMSModeOff ? "Off" : "Other");

    switch (mode) {
    case DPMSModeOn:
	rout->Output->Power(rout->Output, RHD_POWER_ON);
	rout->Output->Active = TRUE;
	break;
    case DPMSModeSuspend:
    case DPMSModeStandby:
	rout->Output->Power(rout->Output, RHD_POWER_RESET);
	rout->Output->Active = FALSE;
	break;
    case DPMSModeOff:
	rout->Output->Power(rout->Output, RHD_POWER_SHUTDOWN);
	rout->Output->Active = FALSE;
	break;
    default:
	ASSERT(!"Unknown DPMS mode");
    }
    RHDDebugRandrState(rhdPtr, "POST-OutputDpms");
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
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    int                Status;

    /* RandR may give us a mode without a name... (xf86RandRModeConvert) */
    if (Mode && !Mode->name && out->crtc->mode.name)
	Mode->name = xstrdup(out->crtc->mode.name);

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name, Mode->name);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    /* If out->crtc is not NULL, it is not necessarily the Crtc that will
     * be used, so let's better skip crtc based checks... */
    /* Monitor is handled by RandR */
    Status = RHDRRModeFixup(out->scrn, Mode, NULL, rout->Connector,
			    rout->Output, NULL);
    RHDDebug(rhdPtr->scrnIndex, "%s: %s -> Status %d\n", __func__,
	     Mode->name, Status);
    return Status;
}

/* The crtc is only known on fixup time. Now it's actually to late to reject a
 * mode and give a reasonable answer why (return is bool), but we'll better not
 * set a mode than scrap our hardware */
static Bool
rhdRROutputModeFixup(xf86OutputPtr  out,
		     DisplayModePtr OrigMode,
		     DisplayModePtr Mode)
{
    RHDPtr             rhdPtr = RHDPTR(out->scrn);
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc    *Crtc   = NULL;

    /* RandR may give us a mode without a name... (xf86RandRModeConvert) */
    if (Mode && !Mode->name && out->crtc->mode.name)
	Mode->name = xstrdup(out->crtc->mode.name);

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s\n", __func__,
	     rout->Name, Mode->name);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    if (out->crtc)
	Crtc = (struct rhdCrtc *) out->crtc->driver_private;
    setupCrtc(rhdPtr, Crtc);
    
    /* Monitor is handled by RandR */
    if (RHDRRModeFixup(out->scrn, Mode, Crtc, rout->Connector, rout->Output,
		       NULL) != MODE_OK) {
	RHDDebug(rhdPtr->scrnIndex, "%s: %s FAILED\n", __func__, Mode->name);
	return FALSE;
    }
    return TRUE;
}

/* Set output mode.
 * Again, randr calls for Crtc and Output separately, while order should be
 * up to the driver. There wouldn't be a need for prepare/set/commit then.
 * We cannot do everything in OutputModeSet, because the viewport isn't known
 * here. */
static void
rhdRROutputPrepare(xf86OutputPtr out)
{
    RHDPtr            rhdPtr = RHDPTR(out->scrn);
    ScrnInfoPtr       pScrn  = xf86Screens[rhdPtr->scrnIndex];
    rhdRandrOutputPtr rout   = (rhdRandrOutputPtr) out->driver_private;

    RHDFUNC(rhdPtr);
    pScrn->vtSema = TRUE;

    /* no active output == no mess */
    rout->Output->Power(rout->Output, RHD_POWER_RESET);

}
static void
rhdRROutputModeSet(xf86OutputPtr  out,
		   DisplayModePtr OrigMode, DisplayModePtr Mode)
{
    RHDPtr            rhdPtr = RHDPTR(out->scrn);
    rhdRandrOutputPtr rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc   *Crtc   = (struct rhdCrtc *) out->crtc->driver_private;

    /* RandR may give us a mode without a name... (xf86RandRModeConvert) */
    if (!Mode->name && out->crtc->mode.name)
	Mode->name = xstrdup(out->crtc->mode.name);

    RHDDebug(rhdPtr->scrnIndex, "%s: Output %s : %s to %s\n", __func__,
	     rout->Name, Mode->name, Crtc->Name);

    /* Set up mode */
    rout->Output->Crtc = Crtc;
    rout->Output->Mode(rout->Output);
}
static void
rhdRROutputCommit(xf86OutputPtr out)
{
    RHDPtr            rhdPtr = RHDPTR(out->scrn);
    rhdRandrOutputPtr rout   = (rhdRandrOutputPtr) out->driver_private;

    RHDFUNC(rhdPtr);

    rout->Output->Active    = TRUE;
    rout->Output->Connector = rout->Connector;
    rout->Output->Power(rout->Output, RHD_POWER_ON);

    RHDDebugRandrState(rhdPtr, rout->Name);
}


/* Probe for a connected output. */
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


/*
 * Xorg Interface
 */

static const xf86CrtcConfigFuncsRec rhdRRCrtcConfigFuncs = {
    rhdRRXF86CrtcResize
};

static const xf86CrtcFuncsRec rhdRRCrtcFuncs = {
    rhdRRCrtcDpms,
    /*save*/ NULL, /*restore*/ NULL,
    rhdRRCrtcLock, /*rhdRRCrtcUnlock*/ NULL,
    rhdRRCrtcModeFixupDUMMY,
    rhdRRCrtcPrepare, rhdRRCrtcModeSet, rhdRRCrtcCommit,
    /*rhdRRCrtcGammaSet*/ NULL,
    /*rhdRRCrtcShadowAllocate, rhdRRCrtcShadowCreate, rhdRRCrtcShadowDestroy,*/ NULL, NULL, NULL,
    /*set_cursor_colors*/ NULL, /*set_cursor_position*/ NULL,
    /*show_cursor*/ NULL, /*hide_cursor*/ NULL,
    /*load_cursor_image*/ NULL, /*load_cursor_argb*/ NULL,
    /*rhdRRCrtcDestroy*/ NULL
};

static const xf86OutputFuncsRec rhdRROutputFuncs = {
    /*create_resources*/ NULL, rhdRROutputDpms,
    /*save*/ NULL, /*restore*/ NULL,
    rhdRROutputModeValid, rhdRROutputModeFixup,
    rhdRROutputPrepare, rhdRROutputCommit,
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
    RHDDebugRandrState(rhdPtr, "POST-ScreenInit");
    return TRUE;
}

/* ModeInit: Set modes according to current layout */
Bool
RHDRandrModeInit(ScrnInfoPtr pScrn)
{
    Bool ret;
    RHDPtr rhdPtr = RHDPTR(pScrn);
    ret = xf86SetDesiredModes(pScrn);
    RHDDebugRandrState(rhdPtr, "POST-ModeInit");
    return ret;
}

/* SwitchMode: Legacy: Set one mode */
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
{ ASSERT(0); return FALSE; }

Bool
RHDRandrModeInit(ScrnInfoPtr pScrn)
{ ASSERT(0); return FALSE; }

Bool
RHDRandrSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{ ASSERT(0); return FALSE; }


#endif /* RANDR_12_INTERFACE */


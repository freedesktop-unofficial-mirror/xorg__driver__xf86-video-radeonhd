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

//#ifdef RANDR_12_INTERFACE   // TODO

#include "xf86.h"
#include "randrstr.h"
#include "xf86i2c.h"		/* Missing in xf86Crtc.h */
#include "xf86Crtc.h"
#include "xf86Parser.h"

/* Driver specific headers */
#include "rhd.h"
#include "rhd_randr.h"
#include "rhd_crtc.h"
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_modes.h"

/* System headers */
#ifndef _XF86_ANSIC_H
#include <ctype.h>
#include <string.h>
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
rhdRRXF86CrtcResize(ScrnInfoPtr scrn, int width, int height)
{
    scrn->virtualX = width;
    scrn->virtualY = height; 
    return TRUE;
}

/*
 * xf86Crtc callback functions
 */

/**
 * Turns the crtc on/off, or sets intermediate power levels if available.
 *
 * Unsupported intermediate modes drop to the lower power setting.  If the
 * mode is DPMSModeOff, the crtc must be disabled sufficiently for it to
 * be safe to call mode_set.
 *
 * Where 'mode' is one of DPMSModeOff, DPMSModeSuspend, DPMSModeStandby or
 * DPMSModeOn. This requests that the crtc go to the specified power state.
 * When changing power states, the output dpms functions are invoked before the
 * crtc dpms functions. */
static void
rhdRRCrtcDpms(xf86CrtcPtr crtc, int mode)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    RHDFUNC(rhdPtr);
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

/**
 * This function applies the specified mode (possibly adjusted by the CRTC
 * and/or Outputs).
 */
static void
rhdRRCrtcModeSet(xf86CrtcPtr    crtc, 
		 DisplayModePtr mode,
		 DisplayModePtr adjusted_mode,
		 int x, int y)
{
    RHDPtr rhdPtr = RHDPTR(crtc->scrn);
    RHDFUNC(rhdPtr);
}

/* Dummys, because they are not tested for NULL */
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

/**
 * Turns the output on/off, or sets intermediate power levels if available.
 *
 * Unsupported intermediate modes drop to the lower power setting.  If the
 * mode is DPMSModeOff, the output must be disabled, as the DPLL may be
 * disabled afterwards.
 */
static void
rhdRROutputDpms(xf86OutputPtr       output,
		int                 mode)
{
    RHDPtr rhdPtr = RHDPTR(output->scrn);
    RHDFUNC(rhdPtr);
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
    ASSERT(!Output->Crtc);
    ASSERT(!Output->Active);
    ASSERT(!Crtc->Active);

    /* Setup - static ATM */
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
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc    *Crtc   = NULL;

    RHDFUNC(rhdPtr);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    if (out->crtc)
	Crtc = (struct rhdCrtc *) out->crtc->driver_private;

    if (! setupCrtc(rhdPtr, Crtc, rout->Output, Mode) )
	return MODE_ERROR;
    
    return RHDRRModeFixup(out->scrn, Mode, Crtc, rout->Connector, rout->Output,
			  NULL);	// TODO: Monitor
}

static Bool
rhdRROutputModeFixup(xf86OutputPtr  out,
		     DisplayModePtr Mode,
		     DisplayModePtr AdjustedMode)
{
    RHDPtr             rhdPtr = RHDPTR(out->scrn);
    rhdRandrOutputPtr  rout   = (rhdRandrOutputPtr) out->driver_private;
    struct rhdCrtc    *Crtc   = NULL;

    RHDFUNC(rhdPtr);
    ASSERT(rout->Connector);
    ASSERT(rout->Output);

    if (out->crtc)
	Crtc = (struct rhdCrtc *) out->crtc->driver_private;

    if (! setupCrtc(rhdPtr, Crtc, rout->Output, AdjustedMode) )
	return FALSE;
    
    if (RHDRRModeFixup(out->scrn, AdjustedMode,
		       Crtc, rout->Connector, rout->Output,
		       NULL)	// TODO: Monitor
	!= MODE_OK)
	return FALSE;
    return TRUE;
}

/**
 * Callback for setting up a video mode after fixups have been made.
 *
 * This is only called while the output is disabled.  The dpms callback
 * must be all that's necessary for the output, to turn the output on
 * after this function is called.
 */
static void
rhdRROutputModeSet(xf86OutputPtr  output,
		   DisplayModePtr mode,
		   DisplayModePtr adjusted_mode)
{
    RHDPtr rhdPtr = RHDPTR(output->scrn);
    RHDFUNC(rhdPtr);
}

/* Probe for a connected output, and return detect_status. */
static xf86OutputStatus
rhdRROutputDetect(xf86OutputPtr output)
{
    RHDPtr rhdPtr = RHDPTR(output->scrn);
    rhdRandrOutputPtr rout = (rhdRandrOutputPtr) output->driver_private;

    RHDFUNC(rhdPtr);
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

    RHDFUNC(rhdPtr);
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
    rhdRRCrtcPrepareDUMMY, rhdRRCrtcModeSet, rhdRRCrtcCommitDUMMY,
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


/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario,
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
#else
# include <string.h>
# include <stdio.h>
# include <unistd.h>
#endif

/* X and server generic header files */
#include "xf86.h"
#include "xaa.h"
#ifdef USE_EXA
# include "exa.h"
#endif
#include "xf86PciInfo.h"
#include "windowstr.h"

/* GLX/DRI/DRM definitions */
#define _XF86DRI_SERVER_
#include "dri.h"
#include "radeon_drm.h"
#include "GL/glxint.h"
#include "GL/glxtokens.h"
#include "sarea.h"

/* Driver data structures */
#include "rhd.h"
#include "rhd_regs.h"
#include "rhd_dri.h"
#include "r5xx_accel.h"	// TODO Nuke (back-to-front w/ offsets)

#ifdef RANDR_12_SUPPORT		// FIXME check / move to rhd_randr.c
# include "xf86Crtc.h"
#endif

/* data only needed by dri */
struct rhdDri {
    int               scrnIndex;

#if 0
    CARD32            BusCntl;
#endif
// TODO: is it secure to have this in the middle of the FB?!?
//   unsigned long     FbSecureSize;     /* Size of secured fb area at end of framebuffer */

// FIXME
//    RADEONSavePtr     ModeReg;          /* Current mode                      */

#if 0
#ifdef USE_EXA
    int               engineMode;
#define EXA_ENGINEMODE_UNKNOWN 0
#define EXA_ENGINEMODE_2D      1
#define EXA_ENGINEMODE_3D      2
#endif
#endif
// FIXME
//    xf86CursorInfoPtr cursor;
//   Bool              allowColorTiling;
//   Bool              tilingEnabled; /* mirror of sarea->tiling_enabled */

    int               pixel_code;

//   Bool              noBackBuffer;
    Bool              directRenderingInited;
    drmVersionPtr     pLibDRMVersion;
    drmVersionPtr     pKernelDRMVersion;
    DRIInfoPtr        pDRIInfo;
    int               drmFD;
    int               numVisualConfigs;
    __GLXvisualConfig *pVisualConfigs;
    RADEONConfigPrivPtr pVisualConfigsPriv;
    Bool             (*DRICloseScreen)(int, ScreenPtr);	// FIXME use driver global closescreen

    drm_handle_t         fbHandle;
    drm_handle_t         registerHandle;

    drm_handle_t         pciMemHandle;

    Bool              depthMoves;       /* Enable depth moves -- slow! */ //TODO
    Bool              allowPageFlip;    /* Enable 3d page flipping */	//TODO
#ifdef DAMAGE
    DamagePtr         pDamage;
    RegionRec         driRegion;
#endif
// Probably not usefull any more
//  Bool              have3DWindows;    /* Are there any 3d clients? */

    int               pciAperSize;
    drmSize           gartSize;
    drm_handle_t         agpMemHandle;     /* Handle from drmAgpAlloc */
    unsigned long     gartOffset;
    int               agpMode;

#if 0
    Bool              CPRuns;           /* CP is running */
    Bool              CPInUse;          /* CP has been used by X server */
    Bool              CPStarted;        /* CP has started */
#endif
    int               CPMode;           /* CP mode that server/clients use */
    int               CPusecTimeout;    /* CP timeout in usecs */
    Bool              needCacheFlush;	//TODO

				/* CP ring buffer data */
    unsigned long     ringStart;        /* Offset into GART space */
    drm_handle_t      ringHandle;       /* Handle from drmAddMap */
    drmSize           ringMapSize;      /* Size of map */
    int               ringSize;         /* Size of ring (in MB) */
    drmAddress        ring;             /* Map */
    int               ringSizeLog2QW;

    // TODO: what is r/o ring space for (1 page)
    unsigned long     ringReadOffset;   /* Offset into GART space */
    drm_handle_t      ringReadPtrHandle; /* Handle from drmAddMap */
    drmSize           ringReadMapSize;  /* Size of map */
    drmAddress        ringReadPtr;      /* Map */

				/* CP vertex/indirect buffer data */
    unsigned long     bufStart;         /* Offset into GART space */
    drm_handle_t      bufHandle;        /* Handle from drmAddMap */
    drmSize           bufMapSize;       /* Size of map */
    int               bufSize;          /* Size of buffers (in MB) */
    drmAddress        buf;              /* Map */
    int               bufNumBufs;       /* Number of buffers */ //TODO ?
    drmBufMapPtr      buffers;          /* Buffer map */

				/* CP GART Texture data */
    unsigned long     gartTexStart;      /* Offset into GART space */
    drm_handle_t         gartTexHandle;     /* Handle from drmAddMap */
    drmSize           gartTexMapSize;    /* Size of map */
    int               gartTexSize;       /* Size of GART tex space (in MB) */
    drmAddress        gartTex;           /* Map */
    int               log2GARTTexGran;

				/* DRI screen private data */
    int               backX;	// TODO Nuke (back-to-front w/ offsets)
    int               backY;	// TODO Nuke (back-to-front w/ offsets)

    int               frontOffset;
    int               frontPitch;
    int               backOffset;
    int               backPitch;
    int               depthOffset;
    int               depthPitch;
    int               depthBits;
    int               textureOffset;
    int               textureSize;
    int               log2TexGran;

    int               pciGartSize;
    CARD32            pciGartOffset;
#ifdef USE_XAA
    CARD32            frontPitchOffset;	// TODO Nuke (back-to-front w/ offsets)
    CARD32            backPitchOffset;	// TODO Nuke (back-to-front w/ offsets)
    CARD32            depthPitchOffset;	// TODO Nuke (back-to-front w/ offsets)

				/* offscreen memory management */
//   int               backLines;	// TODO Nuke (back-to-front w/ offsets)
//    FBAreaPtr         backArea;	//
//   int               depthTexLines;	//
//   FBAreaPtr         depthTexArea;	//
#endif

#if 0
// RADEON_RE_TOP_LEFT, RADEON_RE_WIDTH_HEIGHT, RADEON_AUX_SC_CNTL no longer exist (RADEONCP_REFRESH)
				/* Saved scissor values */
    CARD32            re_top_left;
    CARD32            re_width_height;

    CARD32            aux_sc_cntl;
#endif

    int               irq;

#if 0
#ifdef PER_CONTEXT_SAREA
    int               perctx_sarea_size;
#endif
#endif

#if 0
    /* Debugging info for BEGIN_RING/ADVANCE_RING pairs. */
    int               dma_begin_count;
    char              *dma_debug_func;
    int               dma_debug_lineno;
#endif

//   Bool              XInited3D;    /* X itself has the 3D context */

} ;


static size_t radeon_drm_page_size;
static char *dri_driver_name = "radeon";
static char *r300_driver_name = "r300";
#define RADEON_VERSION_MAJOR 4
#define RADEON_VERSION_MAJOR_TILED 5
#define RADEON_VERSION_MINOR 3
#define RADEON_VERSION_PATCH 0

void RADEONDRICloseScreen(ScreenPtr pScreen);
void RADEONDRIAllocatePCIGARTTable(ScrnInfoPtr pScrn);

static void RADEONDRITransitionTo2d(ScreenPtr pScreen);
static void RADEONDRITransitionTo3d(ScreenPtr pScreen);
static void RADEONDRITransitionMultiToSingle3d(ScreenPtr pScreen);
static void RADEONDRITransitionSingleToMulti3d(ScreenPtr pScreen);

#ifdef DAMAGE
static void RADEONDRIRefreshArea(ScrnInfoPtr pScrn, RegionPtr pReg);

#if (DRIINFO_MAJOR_VERSION > 5 ||		\
     (DRIINFO_MAJOR_VERSION == 5 && DRIINFO_MINOR_VERSION >= 1))
static void RADEONDRIClipNotify(ScreenPtr pScreen, WindowPtr *ppWin, int num);
#endif
#endif

/* Initialize the visual configs that are supported by the hardware.
 * These are combined with the visual configs that the indirect
 * rendering core supports, and the intersection is exported to the
 * client.
 */
static Bool RADEONInitVisualConfigs(ScreenPtr pScreen)
{
    ScrnInfoPtr          pScrn             = xf86Screens[pScreen->myNum];
    struct rhdDri *        info              = RHDPTR(pScrn)->dri;
    int                  numConfigs        = 0;
    __GLXvisualConfig   *pConfigs          = 0;
    RADEONConfigPrivPtr  pRADEONConfigs    = 0;
    RADEONConfigPrivPtr *pRADEONConfigPtrs = 0;
    int                  i, accum, stencil, db, use_db;

//    use_db = !info->noBackBuffer ? 1 : 0;
use_db = 1;

    switch (info->pixel_code) {
    case 8:  /* 8bpp mode is not support */
    case 15: /* FIXME */
    case 24: /* FIXME */
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[dri] RADEONInitVisualConfigs failed "
		   "(depth %d not supported).  "
		   "Disabling DRI.\n", info->pixel_code);
	return FALSE;

#define RADEON_USE_ACCUM   1
#define RADEON_USE_STENCIL 1

    case 16:
	numConfigs = 1;
	if (RADEON_USE_ACCUM)   numConfigs *= 2;
	if (RADEON_USE_STENCIL) numConfigs *= 2;
	if (use_db)             numConfigs *= 2;

	if (!(pConfigs
	      = (__GLXvisualConfig *)xcalloc(sizeof(__GLXvisualConfig),
					     numConfigs))) {
	    return FALSE;
	}
	if (!(pRADEONConfigs
	      = (RADEONConfigPrivPtr)xcalloc(sizeof(RADEONConfigPrivRec),
					     numConfigs))) {
	    xfree(pConfigs);
	    return FALSE;
	}
	if (!(pRADEONConfigPtrs
	      = (RADEONConfigPrivPtr *)xcalloc(sizeof(RADEONConfigPrivPtr),
					       numConfigs))) {
	    xfree(pConfigs);
	    xfree(pRADEONConfigs);
	    return FALSE;
	}

	i = 0;
	for (db = use_db; db >= 0; db--) {
	  for (accum = 0; accum <= RADEON_USE_ACCUM; accum++) {
	    for (stencil = 0; stencil <= RADEON_USE_STENCIL; stencil++) {
		pRADEONConfigPtrs[i] = &pRADEONConfigs[i];

		pConfigs[i].vid                = (VisualID)(-1);
		pConfigs[i].class              = -1;
		pConfigs[i].rgba               = TRUE;
		pConfigs[i].redSize            = 5;
		pConfigs[i].greenSize          = 6;
		pConfigs[i].blueSize           = 5;
		pConfigs[i].alphaSize          = 0;
		pConfigs[i].redMask            = 0x0000F800;
		pConfigs[i].greenMask          = 0x000007E0;
		pConfigs[i].blueMask           = 0x0000001F;
		pConfigs[i].alphaMask          = 0x00000000;
		if (accum) { /* Simulated in software */
		    pConfigs[i].accumRedSize   = 16;
		    pConfigs[i].accumGreenSize = 16;
		    pConfigs[i].accumBlueSize  = 16;
		    pConfigs[i].accumAlphaSize = 0;
		} else {
		    pConfigs[i].accumRedSize   = 0;
		    pConfigs[i].accumGreenSize = 0;
		    pConfigs[i].accumBlueSize  = 0;
		    pConfigs[i].accumAlphaSize = 0;
		}
		if (db)
		    pConfigs[i].doubleBuffer   = TRUE;
		else
		    pConfigs[i].doubleBuffer   = FALSE;
		pConfigs[i].stereo             = FALSE;
		pConfigs[i].bufferSize         = 16;
		pConfigs[i].depthSize          = info->depthBits;
		if (pConfigs[i].depthSize == 24 ? (RADEON_USE_STENCIL - stencil)
						: stencil) {
		    pConfigs[i].stencilSize    = 8;
		} else {
		    pConfigs[i].stencilSize    = 0;
		}
		pConfigs[i].auxBuffers         = 0;
		pConfigs[i].level              = 0;
		if (accum ||
		    (pConfigs[i].stencilSize && pConfigs[i].depthSize == 16)) {
		   pConfigs[i].visualRating    = GLX_SLOW_CONFIG;
		} else {
		   pConfigs[i].visualRating    = GLX_NONE;
		}
		pConfigs[i].transparentPixel   = GLX_NONE;
		pConfigs[i].transparentRed     = 0;
		pConfigs[i].transparentGreen   = 0;
		pConfigs[i].transparentBlue    = 0;
		pConfigs[i].transparentAlpha   = 0;
		pConfigs[i].transparentIndex   = 0;
		i++;
	    }
	  }
	}
	break;

    case 32:
	numConfigs = 1;
	if (RADEON_USE_ACCUM)   numConfigs *= 2;
	if (RADEON_USE_STENCIL) numConfigs *= 2;
	if (use_db)             numConfigs *= 2;

	if (!(pConfigs
	      = (__GLXvisualConfig *)xcalloc(sizeof(__GLXvisualConfig),
					     numConfigs))) {
	    return FALSE;
	}
	if (!(pRADEONConfigs
	      = (RADEONConfigPrivPtr)xcalloc(sizeof(RADEONConfigPrivRec),
					     numConfigs))) {
	    xfree(pConfigs);
	    return FALSE;
	}
	if (!(pRADEONConfigPtrs
	      = (RADEONConfigPrivPtr *)xcalloc(sizeof(RADEONConfigPrivPtr),
					       numConfigs))) {
	    xfree(pConfigs);
	    xfree(pRADEONConfigs);
	    return FALSE;
	}

	i = 0;
	for (db = use_db; db >= 0; db--) {
	  for (accum = 0; accum <= RADEON_USE_ACCUM; accum++) {
	    for (stencil = 0; stencil <= RADEON_USE_STENCIL; stencil++) {
		pRADEONConfigPtrs[i] = &pRADEONConfigs[i];

		pConfigs[i].vid                = (VisualID)(-1);
		pConfigs[i].class              = -1;
		pConfigs[i].rgba               = TRUE;
		pConfigs[i].redSize            = 8;
		pConfigs[i].greenSize          = 8;
		pConfigs[i].blueSize           = 8;
		pConfigs[i].alphaSize          = 8;
		pConfigs[i].redMask            = 0x00FF0000;
		pConfigs[i].greenMask          = 0x0000FF00;
		pConfigs[i].blueMask           = 0x000000FF;
		pConfigs[i].alphaMask          = 0xFF000000;
		if (accum) { /* Simulated in software */
		    pConfigs[i].accumRedSize   = 16;
		    pConfigs[i].accumGreenSize = 16;
		    pConfigs[i].accumBlueSize  = 16;
		    pConfigs[i].accumAlphaSize = 16;
		} else {
		    pConfigs[i].accumRedSize   = 0;
		    pConfigs[i].accumGreenSize = 0;
		    pConfigs[i].accumBlueSize  = 0;
		    pConfigs[i].accumAlphaSize = 0;
		}
		if (db)
		    pConfigs[i].doubleBuffer   = TRUE;
		else
		    pConfigs[i].doubleBuffer   = FALSE;
		pConfigs[i].stereo             = FALSE;
		pConfigs[i].bufferSize         = 32;
		pConfigs[i].depthSize          = info->depthBits;
		if (pConfigs[i].depthSize == 24 ? (RADEON_USE_STENCIL - stencil)
						: stencil) {
		    pConfigs[i].stencilSize    = 8;
		} else {
		    pConfigs[i].stencilSize    = 0;
		}
		pConfigs[i].auxBuffers         = 0;
		pConfigs[i].level              = 0;
		if (accum ||
		    (pConfigs[i].stencilSize && pConfigs[i].depthSize == 16)) {
		   pConfigs[i].visualRating    = GLX_SLOW_CONFIG;
		} else {
		   pConfigs[i].visualRating    = GLX_NONE;
		}
		pConfigs[i].transparentPixel   = GLX_NONE;
		pConfigs[i].transparentRed     = 0;
		pConfigs[i].transparentGreen   = 0;
		pConfigs[i].transparentBlue    = 0;
		pConfigs[i].transparentAlpha   = 0;
		pConfigs[i].transparentIndex   = 0;
		i++;
	    }
	  }
	}
	break;
    }

    info->numVisualConfigs   = numConfigs;
    info->pVisualConfigs     = pConfigs;
    info->pVisualConfigsPriv = pRADEONConfigs;
    GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pRADEONConfigPtrs);
    return TRUE;
}

/* Create the Radeon-specific context information */
static Bool RADEONCreateContext(ScreenPtr pScreen, VisualPtr visual,
				drm_context_t hwContext, void *pVisualConfigPriv,
				DRIContextType contextStore)
{
#ifdef PER_CONTEXT_SAREA
    ScrnInfoPtr          pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *        info  = RHDPTR(pScrn)->dri;
    RADEONDRIContextPtr  ctx_info;

    ctx_info = (RADEONDRIContextPtr)contextStore;
    if (!ctx_info) return FALSE;

    if (drmAddMap(info->drmFD, 0,
		  info->perctx_sarea_size,
		  DRM_SHM,
		  DRM_REMOVABLE,
		  &ctx_info->sarea_handle) < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "[dri] could not create private sarea for ctx id (%d)\n",
		   (int)hwContext);
	return FALSE;
    }

    if (drmAddContextPrivateMapping(info->drmFD, hwContext,
				    ctx_info->sarea_handle) < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "[dri] could not associate private sarea to ctx id (%d)\n",
		   (int)hwContext);
	drmRmMap(info->drmFD, ctx_info->sarea_handle);
	return FALSE;
    }

    ctx_info->ctx_id = hwContext;
#endif
    return TRUE;
}

/* Destroy the Radeon-specific context information */
static void RADEONDestroyContext(ScreenPtr pScreen, drm_context_t hwContext,
				 DRIContextType contextStore)
{
#ifdef PER_CONTEXT_SAREA
    ScrnInfoPtr          pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *        info = RHDPTR(pScrn)->dri;
    RADEONDRIContextPtr  ctx_info;

    ctx_info = (RADEONDRIContextPtr)contextStore;
    if (!ctx_info) return;

    if (drmRmMap(info->drmFD, ctx_info->sarea_handle) < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "[dri] could not remove private sarea for ctx id (%d)\n",
		   (int)hwContext);
    }
#endif
}

/* Called when the X server is woken up to allow the last client's
 * context to be saved and the X server's context to be loaded.  This is
 * not necessary for the Radeon since the client detects when it's
 * context is not currently loaded and then load's it itself.  Since the
 * registers to start and stop the CP are privileged, only the X server
 * can start/stop the engine.
 */
static void RADEONEnterServer(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *  info  = RHDPTR(pScrn)->dri;
    drm_radeon_sarea_t * pSAREAPriv;


    RADEON_MARK_SYNC(info, pScrn);

    pSAREAPriv = DRIGetSAREAPrivate(pScrn->pScreen);
    if (pSAREAPriv->ctx_owner != (signed) DRIGetContext(pScrn->pScreen)) {
//	info->XInited3D = FALSE;
	info->needCacheFlush = TRUE; /*(info->ChipFamily >= CHIP_FAMILY_R300)*/
    }

#ifdef DAMAGE
    if (!info->pDamage && info->allowPageFlip) {
	PixmapPtr pPix  = pScreen->GetScreenPixmap(pScreen);
	info->pDamage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
				     pScreen, pPix);

	if (info->pDamage == NULL) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "No screen damage record, page flipping disabled\n");
	    info->allowPageFlip = 0;
	} else {
	    DamageRegister(&pPix->drawable, info->pDamage);

	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "Damage tracking initialized for page flipping\n");
	}
    }
#endif
}

/* Called when the X server goes to sleep to allow the X server's
 * context to be saved and the last client's context to be loaded.  This
 * is not necessary for the Radeon since the client detects when it's
 * context is not currently loaded and then load's it itself.  Since the
 * registers to start and stop the CP are privileged, only the X server
 * can start/stop the engine.
 */
static void RADEONLeaveServer(ScreenPtr pScreen)
{
//    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
//    struct rhdDri *  info  = RHDPTR(pScrn)->dri;
///    RING_LOCALS;

#ifdef DAMAGE
    if (info->pDamage) {
	RegionPtr pDamageReg = DamageRegion(info->pDamage);
	int nrects = pDamageReg ? REGION_NUM_RECTS(pDamageReg) : 0;

	if (nrects) {
	    RADEONDRIRefreshArea(pScrn, pDamageReg);
	}
    }
#endif

    /* The CP is always running, but if we've generated any CP commands
     * we must flush them to the kernel module now.
     */
///    RADEONCP_RELEASE(pScrn, info);

/*
#ifdef USE_EXA
    info->engineMode = EXA_ENGINEMODE_UNKNOWN;
#endif
*/
}

/* Contexts can be swapped by the X server if necessary.  This callback
 * is currently only used to perform any functions necessary when
 * entering or leaving the X server, and in the future might not be
 * necessary.
 */
static void RADEONDRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
				 DRIContextType oldContextType,
				 void *oldContext,
				 DRIContextType newContextType,
				 void *newContext)
{
    if ((syncType==DRI_3D_SYNC) && (oldContextType==DRI_2D_CONTEXT) &&
	(newContextType==DRI_2D_CONTEXT)) { /* Entering from Wakeup */
	RADEONEnterServer(pScreen);
    }

    if ((syncType==DRI_2D_SYNC) && (oldContextType==DRI_NO_CONTEXT) &&
	(newContextType==DRI_2D_CONTEXT)) { /* Exiting from Block Handler */
	RADEONLeaveServer(pScreen);
    }
}

#ifdef USE_XAA

/* The Radeon has depth tiling on all the time. Rely on surface regs to
 * translate the addresses (only works if allowColorTiling is true).
 */

/* 16-bit depth buffer functions */
#define WRITE_DEPTH16(_x, _y, d)					\
    *(CARD16 *)(pointer)(buf + 2*(_x + _y*info->frontPitch)) = (d)

#define READ_DEPTH16(d, _x, _y)						\
    (d) = *(CARD16 *)(pointer)(buf + 2*(_x + _y*info->frontPitch))

/* 32-bit depth buffer (stencil and depth simultaneously) functions */
#define WRITE_DEPTHSTENCIL32(_x, _y, d)					\
    *(CARD32 *)(pointer)(buf + 4*(_x + _y*info->frontPitch)) = (d)

#define READ_DEPTHSTENCIL32(d, _x, _y)					\
    (d) = *(CARD32 *)(pointer)(buf + 4*(_x + _y*info->frontPitch))

/* Screen to screen copy of data in the depth buffer */
static void RADEONScreenToScreenCopyDepth(ScrnInfoPtr pScrn,
					  int xa, int ya,
					  int xb, int yb,
					  int w, int h)
{
    RHDPtr         rhdPtr = RHDPTR(pScrn);
    struct rhdDri *info   = rhdPtr->dri;
    unsigned char *buf    = (unsigned char *) rhdPtr->FbBase + info->depthOffset;
    int            xstart, xend, xdir;
    int            ystart, yend, ydir;
    int            x, y, d;

    if (xa < xb) xdir = -1, xstart = w-1, xend = 0;
    else         xdir =  1, xstart = 0,   xend = w-1;

    if (ya < yb) ydir = -1, ystart = h-1, yend = 0;
    else         ydir =  1, ystart = 0,   yend = h-1;

    switch (pScrn->bitsPerPixel) {
    case 16:
	for (x = xstart; x != xend; x += xdir) {
	    for (y = ystart; y != yend; y += ydir) {
		READ_DEPTH16(d, xa+x, ya+y);
		WRITE_DEPTH16(xb+x, yb+y, d);
	    }
	}
	break;

    case 32:
	for (x = xstart; x != xend; x += xdir) {
	    for (y = ystart; y != yend; y += ydir) {
		READ_DEPTHSTENCIL32(d, xa+x, ya+y);
		WRITE_DEPTHSTENCIL32(xb+x, yb+y, d);
	    }
	}
	break;

    default:
	break;
    }
}

#endif /* USE_XAA */

/* Initialize the state of the back and depth buffers */
static void RADEONDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 indx)
{
   /* NOOP.  There's no need for the 2d driver to be clearing buffers
    * for the 3d client.  It knows how to do that on its own.
    */
}

// TODO Nuke (back-to-front w/ offsets)
/* Copy the back and depth buffers when the X server moves a window.
 *
 * This routine is a modified form of XAADoBitBlt with the calls to
 * ScreenToScreenBitBlt built in. My routine has the prgnSrc as source
 * instead of destination. My origin is upside down so the ydir cases
 * are reversed.
 */
static void RADEONDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg,
				 RegionPtr prgnSrc, CARD32 indx)
{
#ifdef USE_XAA
    ScreenPtr      pScreen  = pParent->drawable.pScreen;
    ScrnInfoPtr    pScrn    = xf86Screens[pScreen->myNum];
    RHDPtr         rhdPtr   = RHDPTR(pScrn);
    struct rhdDri *info     = rhdPtr->dri;

    BoxPtr         pboxTmp, pboxNext, pboxBase;
    DDXPointPtr    pptTmp;
    int            xdir, ydir;

    int            screenwidth = pScrn->virtualX;
    int            screenheight = pScrn->virtualY;

    BoxPtr         pbox     = REGION_RECTS(prgnSrc);
    int            nbox     = REGION_NUM_RECTS(prgnSrc);

    BoxPtr         pboxNew1 = NULL;
    BoxPtr         pboxNew2 = NULL;
    DDXPointPtr    pptNew1  = NULL;
    DDXPointPtr    pptNew2  = NULL;
    DDXPointPtr    pptSrc   = &ptOldOrg;

    int            dx       = pParent->drawable.x - ptOldOrg.x;
    int            dy       = pParent->drawable.y - ptOldOrg.y;

    /* XXX: Fix in EXA case. */
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA)
	return;

    /* If the copy will overlap in Y, reverse the order */
    if (dy > 0) {
	ydir = -1;

	if (nbox > 1) {
	    /* Keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)xalloc(sizeof(BoxRec)*nbox);
	    if (!pboxNew1) return;

	    pptNew1 = (DDXPointPtr)xalloc(sizeof(DDXPointRec)*nbox);
	    if (!pptNew1) {
		xfree(pboxNew1);
		return;
	    }

	    pboxBase = pboxNext = pbox+nbox-1;

	    while (pboxBase >= pbox) {
		while ((pboxNext >= pbox) && (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;

		pboxTmp = pboxNext+1;
		pptTmp  = pptSrc + (pboxTmp - pbox);

		while (pboxTmp <= pboxBase) {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++  = *pptTmp++;
		}

		pboxBase = pboxNext;
	    }

	    pboxNew1 -= nbox;
	    pbox      = pboxNew1;
	    pptNew1  -= nbox;
	    pptSrc    = pptNew1;
	}
    } else {
	/* No changes required */
	ydir = 1;
    }

    /* If the regions will overlap in X, reverse the order */
    if (dx > 0) {
	xdir = -1;

	if (nbox > 1) {
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)xalloc(sizeof(BoxRec)*nbox);
	    pptNew2  = (DDXPointPtr)xalloc(sizeof(DDXPointRec)*nbox);

	    if (!pboxNew2 || !pptNew2) {
		xfree(pptNew2);
		xfree(pboxNew2);
		xfree(pptNew1);
		xfree(pboxNew1);
		return;
	    }

	    pboxBase = pboxNext = pbox;

	    while (pboxBase < pbox+nbox) {
		while ((pboxNext < pbox+nbox)
		       && (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;

		pboxTmp = pboxNext;
		pptTmp  = pptSrc + (pboxTmp - pbox);

		while (pboxTmp != pboxBase) {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++  = *--pptTmp;
		}

		pboxBase = pboxNext;
	    }

	    pboxNew2 -= nbox;
	    pbox      = pboxNew2;
	    pptNew2  -= nbox;
	    pptSrc    = pptNew2;
	}
    } else {
	/* No changes are needed */
	xdir = 1;
    }

    /* pretty much a hack. */	// TODO Nuke (back-to-front w/ offsets)
    ((struct R5xx2DInfo *)rhdPtr->TwoDInfo)->dst_pitch_offset = info->backPitchOffset;
///    if (info->tilingEnabled)
///       info->dst_pitch_offset |= RADEON_DST_TILE_MACRO;

    (*rhdPtr->XAAInfo->SetupForScreenToScreenCopy)(pScrn, xdir, ydir, GXcopy,
						   (CARD32)(-1), -1);

    for (; nbox-- ; pbox++) {
	int  xa    = pbox->x1;
	int  ya    = pbox->y1;
	int  destx = xa + dx;
	int  desty = ya + dy;
	int  w     = pbox->x2 - xa + 1;
	int  h     = pbox->y2 - ya + 1;

	if (destx < 0)                xa -= destx, w += destx, destx = 0;
	if (desty < 0)                ya -= desty, h += desty, desty = 0;
	if (destx + w > screenwidth)  w = screenwidth  - destx;
	if (desty + h > screenheight) h = screenheight - desty;

	if (w <= 0) continue;
	if (h <= 0) continue;

	(*rhdPtr->XAAInfo->SubsequentScreenToScreenCopy)(pScrn,
							 xa, ya,
							 destx, desty,
							 w, h);

	if (info->depthMoves) {
	    RADEONScreenToScreenCopyDepth(pScrn,
					  xa, ya,
					  destx, desty,
					  w, h);
	}
    }

    ((struct R5xx2DInfo *)rhdPtr->TwoDInfo)->dst_pitch_offset = info->frontPitchOffset;    // TODO Nuke (back-to-front w/ offsets)

    xfree(pptNew2);
    xfree(pboxNew2);
    xfree(pptNew1);
    xfree(pboxNew1);

    rhdPtr->XAAInfo->NeedToSync = TRUE;
#endif /* USE_XAA */
}

static void RADEONDRIInitGARTValues(struct rhdDri * info)
{
    int            s, l;

    info->gartOffset = 0;

				/* Initialize the CP ring buffer data */
    info->ringStart       = info->gartOffset;
    info->ringMapSize     = info->ringSize*1024*1024 + radeon_drm_page_size;
    info->ringSizeLog2QW  = RADEONMinBits(info->ringSize*1024*1024/8)-1;

    info->ringReadOffset  = info->ringStart + info->ringMapSize;
    info->ringReadMapSize = radeon_drm_page_size;

				/* Reserve space for vertex/indirect buffers */
    info->bufStart        = info->ringReadOffset + info->ringReadMapSize;
    info->bufMapSize      = info->bufSize*1024*1024;

				/* Reserve the rest for GART textures */
    info->gartTexStart     = info->bufStart + info->bufMapSize;
    s = (info->gartSize*1024*1024 - info->gartTexStart);
    l = RADEONMinBits((s-1) / RADEON_NR_TEX_REGIONS);
    if (l < RADEON_LOG_TEX_GRANULARITY) l = RADEON_LOG_TEX_GRANULARITY;
    info->gartTexMapSize   = (s >> l) << l;
    info->log2GARTTexGran  = l;
}

/* Set AGP transfer mode according to requests and constraints */
static Bool RADEONSetAgpMode(struct rhdDri * info, ScreenPtr pScreen)
{
    unsigned long mode   = drmAgpGetMode(info->drmFD);	/* Default mode */
    unsigned int  vendor = drmAgpVendorId(info->drmFD);
    unsigned int  device = drmAgpDeviceId(info->drmFD);
    /* ignore agp 3.0 mode bit from the chip as it's buggy on some cards with
       pcie-agp rialto bridge chip - use the one from bridge which must match */
    CARD32 agp_status = (RHDRegRead (info, AGP_STATUS) | AGPv3_MODE) & mode;
    Bool is_v3 = (agp_status & AGPv3_MODE);
    unsigned int defaultMode;
    MessageType from;

    if (is_v3) {
	defaultMode = (agp_status & AGPv3_8X_MODE) ? 8 : 4;
    } else {
	if (agp_status & AGP_4X_MODE) defaultMode = 4;
	else if (agp_status & AGP_2X_MODE) defaultMode = 2;
	else defaultMode = 1;
    }

    from = X_DEFAULT;

// FIXME
#if 0
    if (xf86GetOptValInteger(rhdPtr->Options, OPTION_AGP_MODE, &info->agpMode)) {
	if ((info->agpMode < (is_v3 ? 4 : 1)) ||
            (info->agpMode > (is_v3 ? 8 : 4)) ||
	    (info->agpMode & (info->agpMode - 1))) {
	    xf86DrvMsg(pScreen->myNum, X_ERROR,
		       "Illegal AGP Mode: %d (valid values: %s), leaving at "
		       "%dx\n", info->agpMode, is_v3 ? "4, 8" : "1, 2, 4",
		       defaultMode);
	    info->agpMode = defaultMode;
	} else
	    from = X_CONFIG;
    } else
#endif
	info->agpMode = defaultMode;

    xf86DrvMsg(pScreen->myNum, from, "Using AGP %dx\n", info->agpMode);

    mode &= ~AGP_MODE_MASK;
    if (is_v3) {
	/* only set one mode bit for AGPv3 */
	switch (info->agpMode) {
	case 8:          mode |= AGPv3_8X_MODE; break;
	case 4: default: mode |= AGPv3_4X_MODE;
	}
	/*TODO: need to take care of other bits valid for v3 mode
	 *      currently these bits are not used in all tested cards.
	 */
    } else {
	switch (info->agpMode) {
	case 4:          mode |= AGP_4X_MODE;
	case 2:          mode |= AGP_2X_MODE;
	case 1: default: mode |= AGP_1X_MODE;
	}
    }

#if 0
    /* AGP Fast Writes.
     * TODO: take into account that certain agp modes don't support fast
     * writes at all */
    mode &= ~RADEON_AGP_FW_MODE; /* Disable per default */
    if (xf86ReturnOptValBool(rhdPtr->Options, OPTION_AGP_FW, FALSE)) {
	xf86DrvMsg(pScreen->myNum, X_WARNING,
		   "WARNING: Using the AGPFastWrite option is not recommended.\n");
	xf86Msg(X_NONE, "\tThis option does not provide much of a noticable speed"
		" boost, while it\n\twill probably hard lock your machine."
		" All bets are off!\n");

	/* Black list some host/AGP bridges. */
	if ((vendor == PCI_VENDOR_AMD) && (device == PCI_CHIP_AMD761))
	    xf86DrvMsg(pScreen->myNum, X_PROBED, "Ignoring AGPFastWrite option "
		       "for the AMD 761 northbridge.\n");
	else {
	    xf86DrvMsg(pScreen->myNum, X_CONFIG, "Enabling AGP Fast Writes.\n");
	    mode |= RADEON_AGP_FW_MODE;
	}
    } /* Don't mention this otherwise, so that people don't get funny ideas */

    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Mode 0x%08lx [AGP 0x%04x/0x%04x; Card 0x%04x/0x%04x]\n",
	       mode, vendor, device,
	       PCI_DEV_VENDOR_ID(info->PciInfo),
	       PCI_DEV_DEVICE_ID(info->PciInfo));
#endif
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Mode 0x%08lx [AGP 0x%04x/0x%04x]\n",
	       mode, vendor, device);

    if (drmAgpEnable(info->drmFD, mode) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] AGP not enabled\n");
	drmAgpRelease(info->drmFD);
	return FALSE;
    }

    /* Workaround for some hardware bugs */
/*
    if (info->ChipFamily < CHIP_FAMILY_R200)
	RHDRegWrite (info, RADEON_AGP_CNTL, RHDRegRead (info, RADEON_AGP_CNTL) | 0x000e0000);
*/

				/* Modify the mode if the default mode
				 * is not appropriate for this
				 * particular combination of graphics
				 * card and AGP chipset.
				 */

    return TRUE;
}

/* Initialize Radeon's AGP registers */
static void RADEONSetAgpBase(struct rhdDri * info)
{
    RHDRegWrite (info, AGP_BASE, drmAgpBase(info->drmFD));
}

/* Initialize the AGP state.  Request memory for use in AGP space, and
 * initialize the Radeon registers to point to that memory.
 */
static Bool RADEONDRIAgpInit(struct rhdDri * info, ScreenPtr pScreen)
{
    int            ret;

    if (drmAgpAcquire(info->drmFD) < 0) {
	xf86DrvMsg(pScreen->myNum, X_WARNING, "[agp] AGP not available\n");
	return FALSE;
    }

    if (!RADEONSetAgpMode(info, pScreen))
	return FALSE;

    RADEONDRIInitGARTValues(info);

    if ((ret = drmAgpAlloc(info->drmFD, info->gartSize*1024*1024, 0, NULL,
			   &info->agpMemHandle)) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Out of memory (%d)\n", ret);
	drmAgpRelease(info->drmFD);
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] %d kB allocated with handle 0x%08x\n",
	       info->gartSize*1024, info->agpMemHandle);

    if (drmAgpBind(info->drmFD,
		   info->agpMemHandle, info->gartOffset) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Could not bind\n");
	drmAgpFree(info->drmFD, info->agpMemHandle);
	drmAgpRelease(info->drmFD);
	return FALSE;
    }

    if (drmAddMap(info->drmFD, info->ringStart, info->ringMapSize,
		  DRM_AGP, DRM_READ_ONLY, &info->ringHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add ring mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] ring handle = 0x%08x\n", info->ringHandle);

    if (drmMap(info->drmFD, info->ringHandle, info->ringMapSize,
	       &info->ring) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[agp] Could not map ring\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Ring mapped at 0x%08lx\n",
	       (unsigned long)info->ring);

    if (drmAddMap(info->drmFD, info->ringReadOffset, info->ringReadMapSize,
		  DRM_AGP, DRM_READ_ONLY, &info->ringReadPtrHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add ring read ptr mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[agp] ring read ptr handle = 0x%08x\n",
	       info->ringReadPtrHandle);

    if (drmMap(info->drmFD, info->ringReadPtrHandle, info->ringReadMapSize,
	       &info->ringReadPtr) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map ring read ptr\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Ring read ptr mapped at 0x%08lx\n",
	       (unsigned long)info->ringReadPtr);

    if (drmAddMap(info->drmFD, info->bufStart, info->bufMapSize,
		  DRM_AGP, 0, &info->bufHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add vertex/indirect buffers mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[agp] vertex/indirect buffers handle = 0x%08x\n",
	       info->bufHandle);

    if (drmMap(info->drmFD, info->bufHandle, info->bufMapSize,
	       &info->buf) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map vertex/indirect buffers\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] Vertex/indirect buffers mapped at 0x%08lx\n",
	       (unsigned long)info->buf);

    if (drmAddMap(info->drmFD, info->gartTexStart, info->gartTexMapSize,
		  DRM_AGP, 0, &info->gartTexHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not add GART texture map mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[agp] GART texture map handle = 0x%08x\n",
	       info->gartTexHandle);

    if (drmMap(info->drmFD, info->gartTexHandle, info->gartTexMapSize,
	       &info->gartTex) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] Could not map GART texture map\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[agp] GART Texture map mapped at 0x%08lx\n",
	       (unsigned long)info->gartTex);

    RADEONSetAgpBase(info);

    return TRUE;
}

/* Initialize the PCI GART state.  Request memory for use in PCI space,
 * and initialize the Radeon registers to point to that memory.
 */
static Bool RADEONDRIPciInit(struct rhdDri * info, ScreenPtr pScreen)
{
    int  ret;
    int  flags = DRM_READ_ONLY | DRM_LOCKED | DRM_KERNEL;

    ret = drmScatterGatherAlloc(info->drmFD, info->gartSize*1024*1024,
				&info->pciMemHandle);
    if (ret < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[pci] Out of memory (%d)\n", ret);
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] %d kB allocated with handle 0x%08x\n",
	       info->gartSize*1024, info->pciMemHandle);

    RADEONDRIInitGARTValues(info);

    if (drmAddMap(info->drmFD, info->ringStart, info->ringMapSize,
		  DRM_SCATTER_GATHER, flags, &info->ringHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not add ring mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] ring handle = 0x%08x\n", info->ringHandle);

    if (drmMap(info->drmFD, info->ringHandle, info->ringMapSize,
	       &info->ring) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, "[pci] Could not map ring\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Ring mapped at 0x%08lx\n",
	       (unsigned long)info->ring);
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Ring contents 0x%08lx\n",
	       *(unsigned long *)(pointer)info->ring);

    if (drmAddMap(info->drmFD, info->ringReadOffset, info->ringReadMapSize,
		  DRM_SCATTER_GATHER, flags, &info->ringReadPtrHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not add ring read ptr mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[pci] ring read ptr handle = 0x%08x\n",
	       info->ringReadPtrHandle);

    if (drmMap(info->drmFD, info->ringReadPtrHandle, info->ringReadMapSize,
	       &info->ringReadPtr) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not map ring read ptr\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Ring read ptr mapped at 0x%08lx\n",
	       (unsigned long)info->ringReadPtr);
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Ring read ptr contents 0x%08lx\n",
	       *(unsigned long *)(pointer)info->ringReadPtr);

    if (drmAddMap(info->drmFD, info->bufStart, info->bufMapSize,
		  DRM_SCATTER_GATHER, 0, &info->bufHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not add vertex/indirect buffers mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[pci] vertex/indirect buffers handle = 0x%08x\n",
	       info->bufHandle);

    if (drmMap(info->drmFD, info->bufHandle, info->bufMapSize,
	       &info->buf) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not map vertex/indirect buffers\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Vertex/indirect buffers mapped at 0x%08lx\n",
	       (unsigned long)info->buf);
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] Vertex/indirect buffers contents 0x%08lx\n",
	       *(unsigned long *)(pointer)info->buf);

    if (drmAddMap(info->drmFD, info->gartTexStart, info->gartTexMapSize,
		  DRM_SCATTER_GATHER, 0, &info->gartTexHandle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not add GART texture map mapping\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
 	       "[pci] GART texture map handle = 0x%08x\n",
	       info->gartTexHandle);

    if (drmMap(info->drmFD, info->gartTexHandle, info->gartTexMapSize,
	       &info->gartTex) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] Could not map GART texture map\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[pci] GART Texture map mapped at 0x%08lx\n",
	       (unsigned long)info->gartTex);

    return TRUE;
}

/* Add a map for the MMIO registers that will be accessed by any
 * DRI-based clients.
 */
static Bool RADEONDRIMapInit(RHDPtr rhdPtr, ScreenPtr pScreen)
{
    struct rhdDri *info = rhdPtr->dri;
				/* Map registers */
    if (drmAddMap(info->drmFD,
		  rhdPtr->MMIOPCIAddress, rhdPtr->MMIOMapSize,
		  DRM_REGISTERS, DRM_READ_ONLY, &info->registerHandle) < 0) {
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] register handle = 0x%08x\n", info->registerHandle);

    return TRUE;
}

/* Initialize the kernel data structures */
static int RADEONDRIKernelInit(RHDPtr rhdPtr, ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn  = xf86Screens[pScreen->myNum];
    struct rhdDri *info   = rhdPtr->dri;
    int            bytesPerPixel = pScrn->bitsPerPixel / 8;
    drm_radeon_init_t  drmInfo;

    memset(&drmInfo, 0, sizeof(drm_radeon_init_t));
/*    if ( info->ChipFamily >= CHIP_FAMILY_R300 ) */
       drmInfo.func             = RADEON_INIT_R300_CP;
/*    else
    if ( info->ChipFamily >= CHIP_FAMILY_R200 )
       drmInfo.func		= DRM_RADEON_INIT_R200_CP;
    else
       drmInfo.func		= DRM_RADEON_INIT_CP;*/

    drmInfo.sarea_priv_offset   = sizeof(XF86DRISAREARec);
    drmInfo.is_pci              = (rhdPtr->cardType != RHD_CARD_AGP);
    drmInfo.cp_mode             = info->CPMode;
    drmInfo.gart_size           = info->gartSize*1024*1024;
    drmInfo.ring_size           = info->ringSize*1024*1024;
    drmInfo.usec_timeout        = info->CPusecTimeout;

    drmInfo.fb_bpp              = info->pixel_code;
    drmInfo.depth_bpp           = (info->depthBits - 8) * 2;

    drmInfo.front_offset        = info->frontOffset;
    drmInfo.front_pitch         = info->frontPitch * bytesPerPixel;
    drmInfo.back_offset         = info->backOffset;
    drmInfo.back_pitch          = info->backPitch * bytesPerPixel;
    drmInfo.depth_offset        = info->depthOffset;
    drmInfo.depth_pitch         = info->depthPitch * drmInfo.depth_bpp / 8;

#if 1
    drmInfo.fb_offset           = info->fbHandle;
    drmInfo.mmio_offset         = info->registerHandle;
#endif
    drmInfo.ring_offset         = info->ringHandle;
    drmInfo.ring_rptr_offset    = info->ringReadPtrHandle;
    drmInfo.buffers_offset      = info->bufHandle;
    drmInfo.gart_textures_offset= info->gartTexHandle;

    if (drmCommandWrite(info->drmFD, DRM_RADEON_CP_INIT,
			&drmInfo, sizeof(drm_radeon_init_t)) < 0)
	return FALSE;

    /* DRM_RADEON_CP_INIT does an engine reset, which resets some engine
     * registers back to their default values, so we need to restore
     * those engine register here.
     */
///    RADEONEngineRestore(pScrn);

    return TRUE;
}

static void RADEONDRIGartHeapInit(struct rhdDri * info, ScreenPtr pScreen)
{
    drm_radeon_mem_init_heap_t drmHeap;

    /* Start up the simple memory manager for GART space */
    drmHeap.region = RADEON_MEM_REGION_GART;
    drmHeap.start  = 0;
    drmHeap.size   = info->gartTexMapSize;

    if (drmCommandWrite(info->drmFD, DRM_RADEON_INIT_HEAP,
			&drmHeap, sizeof(drmHeap))) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Failed to initialize GART heap manager\n");
    } else {
	xf86DrvMsg(pScreen->myNum, X_INFO,
		   "[drm] Initialized kernel GART heap manager, %d\n",
		   info->gartTexMapSize);
    }
}

/* Add a map for the vertex buffers that will be accessed by any
 * DRI-based clients.
 */
static Bool RADEONDRIBufInit(RHDPtr rhdPtr, ScreenPtr pScreen)
{
    struct rhdDri *info = rhdPtr->dri;
    
				/* Initialize vertex buffers */
    info->bufNumBufs = drmAddBufs(info->drmFD,
				  info->bufMapSize / RADEON_BUFFER_SIZE,
				  RADEON_BUFFER_SIZE,
				  (rhdPtr->cardType != RHD_CARD_AGP) ? DRM_SG_BUFFER : DRM_AGP_BUFFER,
				  info->bufStart);

    if (info->bufNumBufs <= 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Could not create vertex/indirect buffers list\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] Added %d %d byte vertex/indirect buffers\n",
	       info->bufNumBufs, RADEON_BUFFER_SIZE);

    if (!(info->buffers = drmMapBufs(info->drmFD))) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[drm] Failed to map vertex/indirect buffers list\n");
	return FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO,
	       "[drm] Mapped %d vertex/indirect buffers\n",
	       info->buffers->count);

    return TRUE;
}

static void RADEONDRIIrqInit(RHDPtr rhdPtr, ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *info  = rhdPtr->dri;

    if (!info->irq) {
	info->irq = drmGetInterruptFromBusID(
	    info->drmFD,
	    PCI_BUS(rhdPtr->PciInfo),
	    PCI_DEV(rhdPtr->PciInfo),
	    PCI_FUNC(rhdPtr->PciInfo));

	if ((drmCtlInstHandler(info->drmFD, info->irq)) != 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "[drm] failure adding irq handler, "
		       "there is a device already using that irq\n"
		       "[drm] falling back to irq-free operation\n");
	    info->irq = 0;
	} else {
// FIXME
//	    info->ModeReg->gen_int_cntl = RHDRegRead (info,  RADEON_GEN_INT_CNTL );
	}
    }

    if (info->irq)
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "[drm] dma control initialized, using IRQ %d\n",
		   info->irq);
}


#if 0 // FIXME
/* Initialize the CP state, and start the CP (if used by the X server) */
static void RADEONDRICPInit(ScrnInfoPtr pScrn)
{
    struct rhdDri *  info = RHDPTR(pScrn)->dri;

				/* Turn on bus mastering */
    info->BusCntl &= ~RADEON_BUS_MASTER_DIS;

				/* Make sure the CP is on for the X server */
    RADEONCP_START(pScrn, info);
#ifdef USE_XAA
    if (rhdPtr->AccelMethod == RHD_ACCEL_XAA)
	((struct R5xx2DInfo *)rhdPtr->TwoDInfo)->dst_pitch_offset = info->frontPitchOffset;    // TODO Nuke (back-to-front w/ offsets)
#endif
}
#endif


/* Get the DRM version and do some basic useability checks of DRI */
Bool RADEONDRIGetVersion(ScrnInfoPtr pScrn)
{
    RHDPtr         rhdPtr = RHDPTR(pScrn);
    struct rhdDri *info   = rhdPtr->dri;
    int            major, minor, patch, fd;
    int		   req_minor, req_patch;
    char           *busId;

    /* Check that the GLX, DRI, and DRM modules have been loaded by testing
     * for known symbols in each module.
     */
    if (!xf86LoaderCheckSymbol("GlxSetVisualConfigs")) return FALSE;
    if (!xf86LoaderCheckSymbol("drmAvailable"))        return FALSE;
    if (!xf86LoaderCheckSymbol("DRIQueryVersion")) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "[dri] RADEONDRIGetVersion failed (libdri.a too old)\n"
		 "[dri] Disabling DRI.\n");
      return FALSE;
    }

    /* Check the DRI version */
    DRIQueryVersion(&major, &minor, &patch);
    if (major != DRIINFO_MAJOR_VERSION || minor < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed because of a version "
		   "mismatch.\n"
		   "[dri] libdri version is %d.%d.%d but version %d.%d.x is "
		   "needed.\n"
		   "[dri] Disabling DRI.\n",
		   major, minor, patch,
                   DRIINFO_MAJOR_VERSION, 0);
	return FALSE;
    }

    /* Check the lib version */
    if (xf86LoaderCheckSymbol("drmGetLibVersion"))
	info->pLibDRMVersion = drmGetLibVersion(info->drmFD);
    if (info->pLibDRMVersion == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed because libDRM is really "
		   "way to old to even get a version number out of it.\n"
		   "[dri] Disabling DRI.\n");
	return FALSE;
    }
    if (info->pLibDRMVersion->version_major != 1 ||
	info->pLibDRMVersion->version_minor < 2) {
	    /* incompatible drm library version */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed because of a "
		   "version mismatch.\n"
		   "[dri] libdrm.a module version is %d.%d.%d but "
		   "version 1.2.x is needed.\n"
		   "[dri] Disabling DRI.\n",
		   info->pLibDRMVersion->version_major,
		   info->pLibDRMVersion->version_minor,
		   info->pLibDRMVersion->version_patchlevel);
	drmFreeVersion(info->pLibDRMVersion);
	info->pLibDRMVersion = NULL;
	return FALSE;
    }

    /* Create a bus Id */
    if (xf86LoaderCheckSymbol("DRICreatePCIBusID")) {
	busId = DRICreatePCIBusID(rhdPtr->PciInfo);
    } else {
	busId = xalloc(64);
	sprintf(busId,
		"PCI:%d:%d:%d",
		PCI_BUS(rhdPtr->PciInfo),
		PCI_DEV(rhdPtr->PciInfo),
		PCI_FUNC(rhdPtr->PciInfo));
    }

    /* Low level DRM open */
    fd = drmOpen(dri_driver_name, busId);
    xfree(busId);
    if (fd < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed to open the DRM\n"
		   "[dri] Disabling DRI.\n");
	return FALSE;
    }

    /* Get DRM version & close DRM */
    info->pKernelDRMVersion = drmGetVersion(fd);
    drmClose(fd);
    if (info->pKernelDRMVersion == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed to get the DRM version\n"
		   "[dri] Disabling DRI.\n");
	return FALSE;
    }

    /* Now check if we qualify */
#if 0
    if (info->ChipFamily >= CHIP_FAMILY_R300) {
        req_minor = 17;
#endif
        req_minor = 28;
        req_patch = 0;
#if 0
    } else if (info->IsIGP) {
        req_minor = 10;
	req_patch = 0;
    } else { / * Many problems have been reported with 1.7 in the 2.4 kernel * /
	req_minor = 8;
	req_patch = 0;
    }
#endif

    /* We don't, bummer ! */
    if (info->pKernelDRMVersion->version_major != 1 ||
	info->pKernelDRMVersion->version_minor < req_minor ||
	(info->pKernelDRMVersion->version_minor == req_minor &&
	 info->pKernelDRMVersion->version_patchlevel < req_patch)) {
        /* Incompatible drm version */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONDRIGetVersion failed because of a version "
		   "mismatch.\n"
		   "[dri] radeon.o kernel module version is %d.%d.%d "
		   "but version 1.%d.%d or newer is needed.\n"
		   "[dri] Disabling DRI.\n",
		   info->pKernelDRMVersion->version_major,
		   info->pKernelDRMVersion->version_minor,
		   info->pKernelDRMVersion->version_patchlevel,
		   req_minor,
		   req_patch);
	drmFreeVersion(info->pKernelDRMVersion);
	info->pKernelDRMVersion = NULL;
	return FALSE;
    }

    return TRUE;
}

Bool RADEONDRISetVBlankInterrupt(ScrnInfoPtr pScrn, Bool on)
{
#ifdef RADEON_SETPARAM_VBLANK_CRTC	// FIXME no needed with right drm vers
    struct rhdDri *  info    = RHDPTR(pScrn)->dri;
    int value = 0;

    if (info && info->pKernelDRMVersion->version_minor >= 28) {
        if (on) {
#ifdef RANDR_12_SUPPORT		// FIXME check / move to rhd_randr.c
	    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
  	    if (xf86_config->num_crtc > 1 && xf86_config->crtc[1]->enabled)
	        value = DRM_RADEON_VBLANK_CRTC1 | DRM_RADEON_VBLANK_CRTC2;
	    else
#endif
	        value = DRM_RADEON_VBLANK_CRTC1;
	}

	if (RADEONDRISetParam(pScrn, RADEON_SETPARAM_VBLANK_CRTC, value)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "RADEON Vblank Crtc Setup Failed %d\n", value);
	    return FALSE;
	}
    }
#endif
    return TRUE;
}

/* PreInit */
Bool RADEONDRIPreInit(ScrnInfoPtr pScrn)
{
    RHDPtr         rhdPtr = RHDPTR(pScrn);
    struct rhdDri *info;
    MessageType    from;
    char          *reason;

    if (! rhdPtr->useDRI.val.bool) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Direct rendering turned off by default. Use Option DRI to enable.\n");
	return NULL;
    }

    info = xnfcalloc(1, sizeof(struct rhdDri));
    info->scrnIndex = rhdPtr->scrnIndex;
    rhdPtr->dri = info;

    info->directRenderingInited = FALSE;
#if 0
    info->CPInUse = FALSE;
    info->CPStarted = FALSE;
#endif
    info->pLibDRMVersion = NULL;
    info->pKernelDRMVersion = NULL;

    if (xf86IsEntityShared(rhdPtr->pEnt->index)) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "Direct Rendering Disabled -- "
		   "Dual-head configuration is not working with "
		   "DRI at present.\n"
		   "Please use a RandR merged framebuffer setup if you "
		   "want Dual-head with DRI.\n");
	xfree(info);
	rhdPtr->dri = NULL;
	return FALSE;
    }
#if 0
    if (info->IsSecondary) {
	xfree(info);
	return NULL;
    }
#endif

#if 0
    if (rhdPtr->ChipSet >= RHD_R600) {
	if (xf86ReturnOptValBool(rhdPtr->Options, OPTION_DRI, FALSE)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Direct rendering for R600 an up forced on - "
		       "This is NOT officially supported at the hardware level "
		       "and may cause instability or lockups\n");
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "Direct rendering not officially supported on R600 and up\n");
	    xfree(info);
	    return NULL;
	}
    }
#endif


#if 0
    if (!xf86ReturnOptValBool(rhdPtr->Options, OPTION_DRI, TRUE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Direct rendering forced off\n");
	xfree(info);
	return NULL;
    }
#endif

#if 0
    if (xf86ReturnOptValBool(rhdPtr->Options, OPTION_NOACCEL, FALSE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "[dri] Acceleration disabled, not initializing the DRI\n");
	xfree(info);
	return NULL;
    }
#endif

    if (!RADEONDRIGetVersion(pScrn)) {
	xfree(info);
	rhdPtr->dri = NULL;
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "[dri] Found DRI library version %d.%d.%d and kernel"
	       " module version %d.%d.%d\n",
	       info->pLibDRMVersion->version_major,
	       info->pLibDRMVersion->version_minor,
	       info->pLibDRMVersion->version_patchlevel,
	       info->pKernelDRMVersion->version_major,
	       info->pKernelDRMVersion->version_minor,
	       info->pKernelDRMVersion->version_patchlevel);

#if 0
    if (info->Chipset == PCI_CHIP_RS400_5A41 ||
	info->Chipset == PCI_CHIP_RS400_5A42 ||
	info->Chipset == PCI_CHIP_RC410_5A61 ||
	info->Chipset == PCI_CHIP_RC410_5A62 ||
	info->Chipset == PCI_CHIP_RS480_5954 ||
	info->Chipset == PCI_CHIP_RS480_5955 ||
	info->Chipset == PCI_CHIP_RS482_5974 ||
	info->Chipset == PCI_CHIP_RS485_5975) {

	if (info->pKernelDRMVersion->version_minor < 27) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "Direct rendering broken on XPRESS 200 and 200M with DRI less than 1.27\n");
	    xfree(info);
	    return NULL;
	}
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Direct rendering experimental on RS400/Xpress 200 enabled\n");
    }
#endif

#if 0
    if (xf86ReturnOptValBool(rhdPtr->Options, OPTION_CP_PIO, FALSE)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing CP into PIO mode\n");
	info->CPMode = RADEON_DEFAULT_CP_PIO_MODE;
    } else {
#endif
	info->CPMode = RADEON_DEFAULT_CP_BM_MODE;
#if 0
    }
#endif

    info->gartSize      = RADEON_DEFAULT_GART_SIZE;
    info->ringSize      = RADEON_DEFAULT_RING_SIZE;
    info->bufSize       = RADEON_DEFAULT_BUFFER_SIZE;
    info->gartTexSize   = RADEON_DEFAULT_GART_TEX_SIZE;
    info->pciAperSize   = RADEON_DEFAULT_PCI_APER_SIZE;
    info->CPusecTimeout = RADEON_DEFAULT_CP_TIMEOUT;

#if 0
    if ((xf86GetOptValInteger(rhdPtr->Options,
			      OPTION_GART_SIZE, (int *)&(info->gartSize))) ||
	(xf86GetOptValInteger(rhdPtr->Options,
			      OPTION_GART_SIZE_OLD, (int *)&(info->gartSize)))) {
	switch (info->gartSize) {
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
	case 256:
	    break;

	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Illegal GART size: %d MB\n", info->gartSize);
	    xfree(info);
	    return NULL;
	}
    }
#endif

#if 0
    if (xf86GetOptValInteger(rhdPtr->Options,
			     OPTION_RING_SIZE, &(info->ringSize))) {
	if (info->ringSize < 1 || info->ringSize >= (int)info->gartSize) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Illegal ring buffer size: %d MB\n",
		       info->ringSize);
	    xfree(info);
	    return NULL;
	}
    }
#endif

#if 0
    if (xf86GetOptValInteger(rhdPtr->Options,
			     OPTION_PCIAPER_SIZE, &(info->pciAperSize))) {
	switch(info->pciAperSize) {
	case 32:
	case 64:
	case 128:
	case 256:
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Illegal pci aper size: %d MB\n",
		       info->pciAperSize);
	    xfree(info);
	    return NULL;
	}
    }
#endif


#if 0
    if (xf86GetOptValInteger(rhdPtr->Options,
			     OPTION_BUFFER_SIZE, &(info->bufSize))) {
	if (info->bufSize < 1 || info->bufSize >= (int)info->gartSize) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Illegal vertex/indirect buffers size: %d MB\n",
		       info->bufSize);
	    xfree(info);
	    return NULL;
	}
	if (info->bufSize > 2) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Illegal vertex/indirect buffers size: %d MB\n",
		       info->bufSize);
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Clamping vertex/indirect buffers size to 2 MB\n");
	    info->bufSize = 2;
	}
    }
#endif

    if (info->ringSize + info->bufSize + info->gartTexSize >
	(int)info->gartSize) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Buffers are too big for requested GART space\n");
	xfree(info);
	rhdPtr->dri = NULL;
	return FALSE;
    }

    info->gartTexSize = info->gartSize - (info->ringSize + info->bufSize);

#if 0
    if (xf86GetOptValInteger(rhdPtr->Options, OPTION_USEC_TIMEOUT,
			     &(info->CPusecTimeout))) {
	/* This option checked by the RADEON DRM kernel module */
    }
#endif

#if 0
    /* Two options to try and squeeze as much texture memory as possible
     * for dedicated 3d rendering boxes
     */
    info->noBackBuffer = xf86ReturnOptValBool(rhdPtr->Options,
					      OPTION_NO_BACKBUFFER,
					      FALSE);
#endif

    info->allowPageFlip = 0;

#ifdef DAMAGE
#if 0
    if (info->noBackBuffer) {
	from = X_DEFAULT;
	reason = " because back buffer disabled";
    } else {
#endif
    from = X_DEFAULT;
#if 0
	from = xf86GetOptValBool(rhdPtr->Options, OPTION_PAGE_FLIP,
				 &info->allowPageFlip) ? X_CONFIG : X_DEFAULT;

	if (IS_AVIVO_VARIANT) {
#endif
	    info->allowPageFlip = 0;
#if 0
	    reason = " on r5xx and newer chips.\n";
	} else {
	    reason = "";
	}

    }
#endif
#else
    from = X_DEFAULT;
    reason = " because Damage layer not available at build time";
#endif

    xf86DrvMsg(pScrn->scrnIndex, from, "Page Flipping %sabled%s\n",
	       info->allowPageFlip ? "en" : "dis", reason);

#if 0
    info->DMAForXv = TRUE;
    from = xf86GetOptValBool(rhdPtr->Options, OPTION_XV_DMA, &info->DMAForXv)
	? X_CONFIG : X_INFO;
    xf86DrvMsg(pScrn->scrnIndex, from,
	       "Will %stry to use DMA for Xv image transfers\n",
	       info->DMAForXv ? "" : "not ");
#endif

    radeon_drm_page_size = getpagesize();

    info->pixel_code     = (pScrn->bitsPerPixel != 16
			    ? pScrn->bitsPerPixel
			    : pScrn->depth);

    switch (info->pixel_code) {
    case 8:
    case 15:
    case 24:
	/* These modes are not supported (yet). */
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[dri] RADEONInitVisualConfigs failed "
		   "(depth %d not supported).  "
		   "Disabling DRI.\n", info->pixel_code);
	rhdPtr->dri = NULL;
	return FALSE;

	/* Only 16 and 32 color depths are supports currently. */
    case 16:
    case 32:
	break;
    }

    /* Currently 32bpp pixel buffer implies 32bpp depth. Same for 16bpp */
    info->depthBits = pScrn->depth;
#if 0

    from = xf86GetOptValInteger(info->Options, OPTION_DEPTH_BITS,
				&info->depthBits)
	? X_CONFIG : X_DEFAULT;

    if (info->depthBits != 16 && info->depthBits != 24) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Value for Option \"DepthBits\" must be 16 or 24\n");
	info->depthBits = pScrn->depth;
	from = X_DEFAULT;
    }

    xf86DrvMsg(pScrn->scrnIndex, from,
	       "Using %d bit depth buffer\n", info->depthBits);
#endif

    /* Depth moves are disabled by default since they are extremely slow */
    info->depthMoves = FALSE;
#if 0
    info->depthMoves = xf86ReturnOptValBool(info->Options,
						 OPTION_DEPTH_MOVE, FALSE);
    if (info->depthMoves && info->allowColorTiling) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Enabling depth moves\n");
    } else if (info->depthMoves) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Depth moves don't work without color tiling, disabled\n");
	info->depthMoves = FALSE;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Depth moves disabled by default\n");
    }
#endif

#if 0
    	if (!RADEONSetupMemXAA_DRI(scrnIndex, pScreen))
	    return FALSE;
    	pScrn->fbOffset    = info->frontOffset;
#endif

    return TRUE;
}


/* Allocate all framebuffer chunks.
 * ATM this is still static. When this gets dynamic, it doesn't have to occure
 * in PreInit at all any more */
Bool RADEONDRIAllocateBuffers(ScrnInfoPtr pScrn)
{
    RHDPtr         rhdPtr  = RHDPTR(pScrn);
    struct rhdDri *info    = rhdPtr->dri;
    int            bytesPerPixel = pScrn->bitsPerPixel / 8;
    int            depthBytesPerPixel = (info->depthBits == 24 ? 32 : info->depthBits) / 8;
    int            bytesPerLine = pScrn->displayWidth * bytesPerPixel;
    int            size, depth_size;
    unsigned int   old_freeoffset, old_freesize;

    size = pScrn->displayWidth * bytesPerPixel;
#if 0
    /* Need to adjust screen size for 16 line tiles, and then make it align to
     * the buffer alignment requirement.
     */
    if (info->allowColorTiling)
	size *= RADEON_ALIGN(pScrn->virtualY, 16);
    else
#endif
	size *= pScrn->virtualY;

    old_freeoffset = rhdPtr->FbFreeStart;
    old_freesize   = rhdPtr->FbFreeSize;

    info->frontPitch  = pScrn->displayWidth;
    info->frontOffset = rhdPtr->FbScanoutStart;

#if 0
    /* If backbuffer is disabled, don't allocate memory for it */
    if (info->noBackBuffer) {
	info->textureSize += bufferSize;
    }
#endif

    info->backPitch   = pScrn->displayWidth;
    info->backOffset  = RHDAllocFb(rhdPtr, size, "DRI Back Buffer");

    /* FIXME: Uh-oh. This assumes that the back buffer is immedeately
     * allocated after the front buffer. Otherwise backY/X will be too high */
    info->backY = (info->backOffset - info->frontOffset)
		  / bytesPerLine;
    info->backX = (info->backOffset - info->frontOffset
		   - (info->backY * bytesPerLine)) / bytesPerPixel;

    /* Due to tiling, the Z buffer pitch must be a multiple of 32 pixels,
     * which is always the case if color tiling is used due to color pitch
     * but not necessarily otherwise, and its height a multiple of 16 lines.
     */
    info->depthPitch  = ALIGN(pScrn->displayWidth,32);
    depth_size = ALIGN(pScrn->virtualY, 16) * info->depthPitch
	* depthBytesPerPixel;
    info->depthOffset = RHDAllocFb(rhdPtr, depth_size, "DRI Depth Buffer");
    if (info->backOffset == -1 || info->depthOffset == -1) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		   "DRI: Failed allocating buffers, disabling\n");
	rhdPtr->FbFreeStart = old_freeoffset;
	rhdPtr->FbFreeSize  = old_freesize;
	return FALSE;
    }

    RADEONDRIAllocatePCIGARTTable(pScrn);
    
    info->textureSize = rhdPtr->FbFreeSize;
    if (info->textureSize > 0) {
	int l = RADEONMinBits((info->textureSize-1) / RADEON_NR_TEX_REGIONS);
	if (l < RADEON_LOG_TEX_GRANULARITY)
	    l = RADEON_LOG_TEX_GRANULARITY;
	/* Round the texture size up to the nearest whole number of
	 * texture regions.  Again, be greedy about this, don't
	 * round down.
	 */
	info->log2TexGran = l;
	info->textureSize = (info->textureSize >> l) << l;
    } else {
	info->textureSize = 0;
    }
    if (info->textureSize < 512 * 1024)
	/* Minimum texture size is for 2 256x256x32bpp textures */
	info->textureSize = 0;

#if 0	// TODO Nuke (back-to-front w/ offsets)
    /* RADEON_BUFFER_ALIGN is not sufficient for backbuffer!
       At least for pageflip + color tiling, need to make sure it's 16 scanlines aligned,
       otherwise the copy-from-front-to-back will fail (width_bytes * 16 will also guarantee
       it's still 4kb aligned for tiled case). Need to round up offset (might get into cursor
       area otherwise).
       This might cause some space at the end of the video memory to be unused, since it
       can't be used (?) due to that log_tex_granularity thing???
       Could use different copyscreentoscreen function for the pageflip copies
       (which would use different src and dst offsets) to avoid this. */   
    if (info->allowColorTiling && !info->noBackBuffer) {
	info->textureSize = info->FbMapSize - ((info->FbMapSize - info->textureSize +
			  width_bytes * 16 - 1) / (width_bytes * 16)) * (width_bytes * 16);
    }
#endif

    if (info->textureSize > 0) {
	info->textureOffset = RHDAllocFb(rhdPtr, info->textureSize,
					 "DRI Textures");
	ASSERT(info->textureOffset != -1);
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Using %d MB GART aperture\n", info->gartSize);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Using %d MB for the ring buffer\n", info->ringSize);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Using %d MB for vertex/indirect buffers\n", info->bufSize);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Using %d MB for GART textures\n", info->gartTexSize);

#if 0	// TODO Nuke (back-to-front w/ offsets)
    scanlines = (info->FbMapSize-info->FbSecureSize) / width_bytes;
    if (scanlines > 8191)
	scanlines = 8191;

    if (scanlines > 8191)
	scanlines = 8191;

    MemBox.x1 = 0;
    MemBox.y1 = 0;
    MemBox.x2 = pScrn->displayWidth;
    MemBox.y2 = scanlines;

    if (!xf86InitFBManager(pScreen, &MemBox)) {
        xf86DrvMsg(scrnIndex, X_ERROR,
		   "Memory manager initialization to "
		   "(%d,%d) (%d,%d) failed\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
	return FALSE;
    } else {
	int  width, height;

	xf86DrvMsg(scrnIndex, X_INFO,
		   "Memory manager initialized to (%d,%d) (%d,%d)\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
    }

    /* why oh why can't we just request modes which are guaranteed to be 16 lines
       aligned... sigh */
    if ((fbarea = xf86AllocateOffscreenArea(pScreen,
					    pScrn->displayWidth,
					    info->allowColorTiling ? 
					    ((pScrn->virtualY + 15) & ~15)
					    - pScrn->virtualY + 2 : 2,
					    0, NULL, NULL,
					    NULL))) {
	xf86DrvMsg(scrnIndex, X_INFO,
		   "Reserved area from (%d,%d) to (%d,%d)\n",
		   fbarea->box.x1, fbarea->box.y1,
		   fbarea->box.x2, fbarea->box.y2);
    } else {
	xf86DrvMsg(scrnIndex, X_ERROR, "Unable to reserve area\n");
    }


    if (xf86QueryLargestOffscreenArea(pScreen, &width,
				      &height, 0, 0, 0)) {
	xf86DrvMsg(scrnIndex, X_INFO,
		   "Largest offscreen area available: %d x %d\n",
		   width, height);

#if 0	// TODO Nuke (back-to-front w/ offsets)
	/* Lines in offscreen area needed for depth buffer and
	 * textures
	 */
	info->depthTexLines = (scanlines
			       - info->depthOffset / width_bytes);
	info->backLines	    = (scanlines
			       - info->backOffset / width_bytes
			       - info->depthTexLines);
	info->backArea	    = NULL;
#endif
    } else {
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Unable to determine largest offscreen area "
		   "available\n");
	return FALSE;
    }
#endif

#ifdef USE_XAA	// TODO Nuke (back-to-front w/ offsets)
    info->frontPitchOffset = (((info->frontPitch * bytesPerPixel / 64) << 22) |
			      ((info->frontOffset+rhdPtr->FbIntAddress) >> 10));

    info->backPitchOffset = (((info->backPitch * bytesPerPixel / 64) << 22) |
			     ((info->backOffset+rhdPtr->FbIntAddress) >> 10));

    info->depthPitchOffset = (((info->depthPitch * depthBytesPerPixel / 64) << 22) |
			      ((info->depthOffset+rhdPtr->FbIntAddress) >> 10));
#endif

    return TRUE;
}


/* Initialize the screen-specific data structures for the DRI and the
 * Radeon.  This is the main entry point to the device-specific
 * initialization code.  It calls device-independent DRI functions to
 * create the DRI data structures and initialize the DRI state.
 */
Bool RADEONDRIScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn   = xf86Screens[pScreen->myNum];
    RHDPtr         rhdPtr  = RHDPTR(pScrn);
    struct rhdDri *info    = rhdPtr->dri;
    DRIInfoPtr     pDRIInfo;
    RADEONDRIPtr   pRADEONDRI;

    info->DRICloseScreen = NULL;
    /* Create the DRI data structure, and fill it in before calling the
     * DRIScreenInit().
     */
    if (!(pDRIInfo = DRICreateInfoRec())) return FALSE;

    info->pDRIInfo                       = pDRIInfo;
    pDRIInfo->drmDriverName              = dri_driver_name;

/*    if ( (info->ChipFamily >= CHIP_FAMILY_R300) ) {*/
       pDRIInfo->clientDriverName        = r300_driver_name;
/*    } else    
    if ( info->ChipFamily >= CHIP_FAMILY_R200 )
       pDRIInfo->clientDriverName	 = R200_DRIVER_NAME;
    else 
       pDRIInfo->clientDriverName	 = dri_driver_name;*/

    if (xf86LoaderCheckSymbol("DRICreatePCIBusID")) {
	pDRIInfo->busIdString = DRICreatePCIBusID(rhdPtr->PciInfo);
    } else {
	pDRIInfo->busIdString            = xalloc(64);
	sprintf(pDRIInfo->busIdString,
		"PCI:%d:%d:%d",
		PCI_BUS(rhdPtr->PciInfo),
		PCI_DEV(rhdPtr->PciInfo),
		PCI_FUNC(rhdPtr->PciInfo));
    }
#if 1
    pDRIInfo->ddxDriverMajorVersion      = /* TODO info->allowColorTiling ?
    				RADEON_VERSION_MAJOR_TILED : */ RADEON_VERSION_MAJOR;
    pDRIInfo->ddxDriverMinorVersion      = RADEON_VERSION_MINOR;
    pDRIInfo->ddxDriverPatchVersion      = RADEON_VERSION_PATCH;
#endif
//    pDRIInfo->ddxDriverMajorVersion      = RHD_MAJOR_VERSION;
//    pDRIInfo->ddxDriverMinorVersion      = RHD_MINOR_VERSION;
//    pDRIInfo->ddxDriverPatchVersion      = RHD_PATCHLEVEL;
    pDRIInfo->frameBufferPhysicalAddress = (void *) (rhdPtr->FbPCIAddress + info->frontOffset);

    // TODO: is it secure to have this in the middle of the FB?!?
    // pDRIInfo->frameBufferSize            = rhdPtr->FbMapSize - info->FbSecureSize;
    pDRIInfo->frameBufferSize            = rhdPtr->FbMapSize;
    pDRIInfo->frameBufferStride          = (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);
    pDRIInfo->ddxDrawableTableEntry      = RADEON_MAX_DRAWABLES;
    pDRIInfo->maxDrawableTableEntry      = (SAREA_MAX_DRAWABLES
					    < RADEON_MAX_DRAWABLES
					    ? SAREA_MAX_DRAWABLES
					    : RADEON_MAX_DRAWABLES);
    /* kill DRIAdjustFrame. We adjust sarea frame info ourselves to work
       correctly with pageflip + mergedfb/color tiling */
    pDRIInfo->wrap.AdjustFrame = NULL;

#ifdef PER_CONTEXT_SAREA
    /* This is only here for testing per-context SAREAs.  When used, the
       magic number below would be properly defined in a header file. */
    info->perctx_sarea_size = 64 * 1024;
#endif

#ifdef NOT_DONE
    /* FIXME: Need to extend DRI protocol to pass this size back to
     * client for SAREA mapping that includes a device private record
     */
    pDRIInfo->SAREASize = ((sizeof(XF86DRISAREARec) + 0xfff)
			   & 0xfff); /* round to page */
    /* + shared memory device private rec */
#else
    /* For now the mapping works by using a fixed size defined
     * in the SAREA header
     */
    if (sizeof(XF86DRISAREARec)+sizeof(drm_radeon_sarea_t) > SAREA_MAX) {
	ErrorF("Data does not fit in SAREA\n");
	return FALSE;
    }
    pDRIInfo->SAREASize = SAREA_MAX;
#endif

    if (!(pRADEONDRI = (RADEONDRIPtr)xcalloc(sizeof(RADEONDRIRec),1))) {
	DRIDestroyInfoRec(info->pDRIInfo);
	info->pDRIInfo = NULL;
	return FALSE;
    }
    pDRIInfo->devPrivate     = pRADEONDRI;
    pDRIInfo->devPrivateSize = sizeof(RADEONDRIRec);
    pDRIInfo->contextSize    = sizeof(RADEONDRIContextRec);

    pDRIInfo->CreateContext  = RADEONCreateContext;
    pDRIInfo->DestroyContext = RADEONDestroyContext;
    pDRIInfo->SwapContext    = RADEONDRISwapContext;
    pDRIInfo->InitBuffers    = RADEONDRIInitBuffers;
    pDRIInfo->MoveBuffers    = RADEONDRIMoveBuffers;
    pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;
    pDRIInfo->TransitionTo2d = RADEONDRITransitionTo2d;
    pDRIInfo->TransitionTo3d = RADEONDRITransitionTo3d;
    pDRIInfo->TransitionSingleToMulti3D = RADEONDRITransitionSingleToMulti3d;
    pDRIInfo->TransitionMultiToSingle3D = RADEONDRITransitionMultiToSingle3d;
#if defined(DAMAGE) && (DRIINFO_MAJOR_VERSION > 5 ||	\
			(DRIINFO_MAJOR_VERSION == 5 &&	\
			 DRIINFO_MINOR_VERSION >= 1))
    pDRIInfo->ClipNotify     = RADEONDRIClipNotify;
#endif

    pDRIInfo->createDummyCtx     = TRUE;
    pDRIInfo->createDummyCtxPriv = FALSE;

#ifdef USE_EXA
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
#if DRIINFO_MAJOR_VERSION == 5 && DRIINFO_MINOR_VERSION >= 3
       int major, minor, patch;

       DRIQueryVersion(&major, &minor, &patch);

///       if (minor >= 3)
#endif
#if DRIINFO_MAJOR_VERSION > 5 || \
    (DRIINFO_MAJOR_VERSION == 5 && DRIINFO_MINOR_VERSION >= 3)
///	  pDRIInfo->texOffsetStart = RADEONTexOffsetStart;
#endif
    }
#endif

    if (!DRIScreenInit(pScreen, pDRIInfo, &info->drmFD)) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[dri] DRIScreenInit failed.  Disabling DRI.\n");
	xfree(pDRIInfo->devPrivate);
	pDRIInfo->devPrivate = NULL;
	DRIDestroyInfoRec(pDRIInfo);
	pDRIInfo = NULL;
	return FALSE;
    }
				/* Initialize AGP */
    if (rhdPtr->cardType == RHD_CARD_AGP &&
	!RADEONDRIAgpInit(info, pScreen)) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[agp] AGP failed to initialize. Disabling the DRI.\n" );
	xf86DrvMsg(pScreen->myNum, X_INFO,
		   "[agp] You may want to make sure the agpgart kernel "
		   "module\nis loaded before the radeon kernel module.\n");
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

				/* Initialize PCI */
    if (rhdPtr->cardType != RHD_CARD_AGP &&
	!RADEONDRIPciInit(info, pScreen)) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
		   "[pci] PCI failed to initialize. Disabling the DRI.\n" );
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

				/* DRIScreenInit doesn't add all the
				 * common mappings.  Add additional
				 * mappings here.
				 */
    if (!RADEONDRIMapInit(rhdPtr, pScreen)) {
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

#if 0
				/* DRIScreenInit adds the frame buffer
				   map, but we need it as well */
    {
	void *scratch_ptr;
        int scratch_int;

	DRIGetDeviceInfo(pScreen, &info->fbHandle,
                         &scratch_int, &scratch_int,
                         &scratch_int, &scratch_int,
                         &scratch_ptr);
    }
#endif

				/* FIXME: When are these mappings unmapped? */

    if (!RADEONInitVisualConfigs(pScreen)) {
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] Visual configs initialized\n");

    /* Tell DRI about new memory map */
/// Always true for pKernelDRMVersion->version_minor >= 23
//    if (info->newMemoryMap) {
        if (RADEONDRISetParam(pScrn, RADEON_SETPARAM_NEW_MEMMAP, 1) < 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "[drm] failed to enable new memory map\n");
		RADEONDRICloseScreen(pScreen);
		return FALSE;
	}
//    }

    return TRUE;
}

static Bool RADEONDRIDoCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *  info  = RHDPTR(pScrn)->dri;

    RADEONDRICloseScreen(pScreen);

    pScreen->CloseScreen = info->DRICloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Finish initializing the device-dependent DRI state, and call
 * DRIFinishScreenInit() to complete the device-independent DRI
 * initialization.
 */
Bool RADEONDRIFinishScreenInit(ScreenPtr pScreen)
{
    ScrnInfoPtr         pScrn  = xf86Screens[pScreen->myNum];
    RHDPtr              rhdPtr = RHDPTR(pScrn);
    struct rhdDri      *info   = rhdPtr->dri;
    drm_radeon_sarea_t *pSAREAPriv;
    RADEONDRIPtr        pRADEONDRI;

    if (! info)
	return FALSE;
    if (rhdPtr->cardType != RHD_CARD_PCIE /*&&
        info->pKernelDRMVersion->version_minor >= 19*/)
    {
      if (RADEONDRISetParam(pScrn, RADEON_SETPARAM_PCIGART_LOCATION, info->pciGartOffset) < 0)
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "[drm] failed set pci gart location\n");

#ifdef RADEON_SETPARAM_PCIGART_TABLE_SIZE	// FIXME no needed with right drm vers
//      if (info->pKernelDRMVersion->version_minor >= 26) {
	if (RADEONDRISetParam(pScrn, RADEON_SETPARAM_PCIGART_TABLE_SIZE, info->pciGartSize) < 0)
	  xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		     "[drm] failed set pci gart table size\n");
//      }
#endif
    }
    RHDDebug(pScrn->scrnIndex, "DRI Finishing init !\n");

    info->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;
    /* info->pDRIInfo->driverSwapMethod = DRI_SERVER_SWAP; */

    /* NOTE: DRIFinishScreenInit must be called before *DRIKernelInit
     * because *DRIKernelInit requires that the hardware lock is held by
     * the X server, and the first time the hardware lock is grabbed is
     * in DRIFinishScreenInit.
     */
    if (!DRIFinishScreenInit(pScreen)) {
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

    /* Initialize the kernel data structures */
    if (!RADEONDRIKernelInit(rhdPtr, pScreen)) {
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

    /* Initialize the vertex buffers list */
    if (!RADEONDRIBufInit(rhdPtr, pScreen)) {
	RADEONDRICloseScreen(pScreen);
	return FALSE;
    }

    /* Initialize IRQ */
    RADEONDRIIrqInit(rhdPtr, pScreen);

    /* Initialize kernel GART memory manager */
    RADEONDRIGartHeapInit(info, pScreen);

    /* Initialize and start the CP if required */
///    RADEONDRICPInit(pScrn);

    /* Initialize the SAREA private data structure */
    pSAREAPriv = (drm_radeon_sarea_t *)DRIGetSAREAPrivate(pScreen);
    memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

    pRADEONDRI                    = (RADEONDRIPtr)info->pDRIInfo->devPrivate;

    pRADEONDRI->deviceID          = rhdPtr->PciDeviceID;
    pRADEONDRI->width             = pScrn->virtualX;
    pRADEONDRI->height            = pScrn->virtualY;
    pRADEONDRI->depth             = pScrn->depth;
    pRADEONDRI->bpp               = pScrn->bitsPerPixel;

    pRADEONDRI->IsPCI             = (rhdPtr->cardType != RHD_CARD_AGP);
    pRADEONDRI->AGPMode           = info->agpMode;

    pRADEONDRI->frontOffset       = info->frontOffset;
    pRADEONDRI->frontPitch        = info->frontPitch;
    pRADEONDRI->backOffset        = info->backOffset;
    pRADEONDRI->backPitch         = info->backPitch;
    pRADEONDRI->depthOffset       = info->depthOffset;
    pRADEONDRI->depthPitch        = info->depthPitch;
    pRADEONDRI->textureOffset     = info->textureOffset;
    pRADEONDRI->textureSize       = info->textureSize;
    pRADEONDRI->log2TexGran       = info->log2TexGran;

    pRADEONDRI->registerHandle    = info->registerHandle;
    pRADEONDRI->registerSize      = rhdPtr->MMIOMapSize;

    pRADEONDRI->statusHandle      = info->ringReadPtrHandle;
    pRADEONDRI->statusSize        = info->ringReadMapSize;

    pRADEONDRI->gartTexHandle     = info->gartTexHandle;
    pRADEONDRI->gartTexMapSize    = info->gartTexMapSize;
    pRADEONDRI->log2GARTTexGran   = info->log2GARTTexGran;
    pRADEONDRI->gartTexOffset     = info->gartTexStart;

    pRADEONDRI->sarea_priv_offset = sizeof(XF86DRISAREARec);

#ifdef PER_CONTEXT_SAREA
    /* Set per-context SAREA size */
    pRADEONDRI->perctx_sarea_size = info->perctx_sarea_size;
#endif

    info->directRenderingInited = TRUE;

    /* Wrap CloseScreen */
    info->DRICloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = RADEONDRIDoCloseScreen;

    /* disable vblank at startup */
    RADEONDRISetVBlankInterrupt (pScrn, FALSE);

#if 0 // FIXME
    /* DRI final init might have changed the memory map, we need to adjust
     * our local image to make sure we restore them properly on mode
     * changes or VT switches
     */
    RADEONAdjustMemMapRegisters(pScrn, info->ModeReg);
#endif

#if 0   /// display buffer watermark calculation (legacy_crtc.c)
    if ((info->DispPriority == 1) && (info->cardType==CARD_AGP)) {
	/* we need to re-calculate bandwidth because of AGPMode difference. */
	RADEONInitDispBandwidth(pScrn);
    }
#endif
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Direct rendering enabled\n");

#if 0
    /* we might already be in tiled mode, tell drm about it */
    if (info->directRenderingEnabled && info->tilingEnabled) {
	if (RADEONDRISetParam(pScrn, RADEON_SETPARAM_SWITCH_TILING, (info->tilingEnabled ? 1 : 0)) < 0)
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "[drm] failed changing tiling status\n");
    }
#endif

    return TRUE;
}

/**
 * This function will attempt to get the Radeon hardware back into shape
 * after a resume from disc.
 *
 * Charl P. Botha <http://cpbotha.net>
 */
void RADEONDRIResume(ScreenPtr pScreen)
{
    int _ret;
    ScrnInfoPtr    pScrn   = xf86Screens[pScreen->myNum];
    RHDPtr         rhdPtr  = RHDPTR(pScrn);
    struct rhdDri *info    = rhdPtr->dri;

#if 0
    if (info->pKernelDRMVersion->version_minor >= 9) {
#endif
	xf86DrvMsg(pScreen->myNum, X_INFO,
		   "[RESUME] Attempting to re-init Radeon hardware.\n");
#if 0
    } else {
	xf86DrvMsg(pScreen->myNum, X_WARNING,
		   "[RESUME] Cannot re-init Radeon hardware, DRM too old\n"
		   "(need 1.9.0  or newer)\n");
	return;
    }
#endif

    if (rhdPtr->cardType == RHD_CARD_AGP) {
#if 0
	if (!RADEONSetAgpMode(info, pScreen))
	    return;
#endif

	RADEONSetAgpBase(info);
    }

    _ret = drmCommandNone(info->drmFD, DRM_RADEON_CP_RESUME);
    if (_ret) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "%s: CP resume %d\n", __FUNCTION__, _ret);
	/* FIXME: return? */
    }

///    RADEONEngineRestore(pScrn);

///    RADEONDRICPInit(pScrn);
}

void RADEONDRIStop(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *  info  = RHDPTR(pScrn)->dri;
///    RING_LOCALS;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, LOG_DEBUG, "RADEONDRIStop\n");

    /* Stop the CP */
    if (info->directRenderingInited) {
	/* If we've generated any CP commands, we must flush them to the
	 * kernel module now.
	 */
///	RADEONCP_RELEASE(pScrn, info);
///	RADEONCP_STOP(pScrn, info);
    }
    info->directRenderingInited = FALSE;
}

/* The screen is being closed, so clean up any state and free any
 * resources used by the DRI.
 */
void RADEONDRICloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri *  info  = RHDPTR(pScrn)->dri;
    drm_radeon_init_t  drmInfo;

     xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, LOG_DEBUG,
		    "RADEONDRICloseScreen\n");

#ifdef DAMAGE
     REGION_UNINIT(pScreen, &info->driRegion);
#endif

     if (info->irq) {
	RADEONDRISetVBlankInterrupt (pScrn, FALSE);
	drmCtlUninstHandler(info->drmFD);
	info->irq = 0;
// FIXME
//	info->ModeReg->gen_int_cntl = 0;
    }

    /* De-allocate vertex buffers */
    if (info->buffers) {
	drmUnmapBufs(info->buffers);
	info->buffers = NULL;
    }

    /* De-allocate all kernel resources */
    memset(&drmInfo, 0, sizeof(drm_radeon_init_t));
    drmInfo.func = RADEON_CLEANUP_CP;
    drmCommandWrite(info->drmFD, DRM_RADEON_CP_INIT,
		    &drmInfo, sizeof(drm_radeon_init_t));

    /* De-allocate all GART resources */
    if (info->gartTex) {
	drmUnmap(info->gartTex, info->gartTexMapSize);
	info->gartTex = NULL;
    }
    if (info->buf) {
	drmUnmap(info->buf, info->bufMapSize);
	info->buf = NULL;
    }
    if (info->ringReadPtr) {
	drmUnmap(info->ringReadPtr, info->ringReadMapSize);
	info->ringReadPtr = NULL;
    }
    if (info->ring) {
	drmUnmap(info->ring, info->ringMapSize);
	info->ring = NULL;
    }
    if (info->agpMemHandle != DRM_AGP_NO_HANDLE) {
	drmAgpUnbind(info->drmFD, info->agpMemHandle);
	drmAgpFree(info->drmFD, info->agpMemHandle);
	info->agpMemHandle = DRM_AGP_NO_HANDLE;
	drmAgpRelease(info->drmFD);
    }
    if (info->pciMemHandle) {
	drmScatterGatherFree(info->drmFD, info->pciMemHandle);
	info->pciMemHandle = 0;
    }

    /* De-allocate all DRI resources */
    DRICloseScreen(pScreen);

    /* De-allocate all DRI data structures */
    if (info->pDRIInfo) {
	if (info->pDRIInfo->devPrivate) {
	    xfree(info->pDRIInfo->devPrivate);
	    info->pDRIInfo->devPrivate = NULL;
	}
	DRIDestroyInfoRec(info->pDRIInfo);
	info->pDRIInfo = NULL;
    }
    if (info->pVisualConfigs) {
	xfree(info->pVisualConfigs);
	info->pVisualConfigs = NULL;
    }
    if (info->pVisualConfigsPriv) {
	xfree(info->pVisualConfigsPriv);
	info->pVisualConfigsPriv = NULL;
    }
}

/* Use callbacks from dri.c to support pageflipping mode for a single
 * 3d context without need for any specific full-screen extension.
 *
 * Also use these callbacks to allocate and free 3d-specific memory on
 * demand.
 */


#ifdef DAMAGE

/* Use the damage layer to maintain a list of dirty rectangles.
 * These are blitted to the back buffer to keep both buffers clean
 * during page-flipping when the 3d application isn't fullscreen.
 *
 * An alternative to this would be to organize for all on-screen drawing
 * operations to be duplicated for the two buffers.  That might be
 * faster, but seems like a lot more work...
 */


static void RADEONDRIRefreshArea(ScrnInfoPtr pScrn, RegionPtr pReg)
{
    struct rhdDri *       info       = RHDPTR(pScrn)->dri;
    int                 i, num;
    ScreenPtr           pScreen    = pScrn->pScreen;
    drm_radeon_sarea_t *  pSAREAPriv = DRIGetSAREAPrivate(pScreen);
#ifdef USE_EXA
    PixmapPtr           pPix = pScreen->GetScreenPixmap(pScreen);
#endif
    RegionRec region;
    BoxPtr pbox;

    if (!info->directRenderingInited || !info->CPStarted)
	return;

    /* Don't want to do this when no 3d is active and pages are
     * right-way-round
     */
/*    if (!pSAREAPriv->pfAllowPageFlip && pSAREAPriv->pfCurrentPage == 0) Called pfState in radeon_drm?!? */
    if (!pSAREAPriv->pfState && pSAREAPriv->pfCurrentPage == 0)
	return;

    REGION_NULL(pScreen, &region);
    REGION_SUBTRACT(pScreen, &region, pReg, &info->driRegion);

    num = REGION_NUM_RECTS(&region);

    if (!num) {
	goto out;
    }

    pbox = REGION_RECTS(&region);

    /* pretty much a hack. */

#ifdef USE_EXA
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
	CARD32 src_pitch_offset, dst_pitch_offset, datatype;

	RADEONGetPixmapOffsetPitch(pPix, &src_pitch_offset);
	dst_pitch_offset = src_pitch_offset + (info->backOffset >> 10);
	RADEONGetDatatypeBpp(pScrn->bitsPerPixel, &datatype);
	info->xdir = info->ydir = 1;

	RADEONDoPrepareCopyCP(pScrn, src_pitch_offset, dst_pitch_offset, datatype,
			      GXcopy, ~0);
    }
#endif

#ifdef USE_XAA
    if (rhdPtr->AccelMethod == RHD_ACCEL_XAA) {
	/* Make sure accel has been properly inited */
	if (info->accel == NULL || info->accel->SetupForScreenToScreenCopy == NULL)
	    goto out;
///	if (info->tilingEnabled)
///	    rhdPtr->TwoDInfo->dst_pitch_offset |= RADEON_DST_TILE_MACRO;
	(*info->accel->SetupForScreenToScreenCopy)(pScrn,
						   1, 1, GXcopy,
						   (CARD32)(-1), -1);
    }
#endif

    for (i = 0 ; i < num ; i++, pbox++) {
	int xa = max(pbox->x1, 0), xb = min(pbox->x2, pScrn->virtualX-1);
	int ya = max(pbox->y1, 0), yb = min(pbox->y2, pScrn->virtualY-1);

	if (xa <= xb && ya <= yb) {
#ifdef USE_EXA
	    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
		RADEONCopyCP(pPix, xa, ya, xa, ya, xb - xa + 1, yb - ya + 1);
	    }
#endif

#ifdef USE_XAA
	    if (rhdPtr->AccelMethod == RHD_ACCEL_XAA) {
		(*info->accel->SubsequentScreenToScreenCopy)(pScrn, xa, ya,
							     xa + info->backX,
							     ya + info->backY,
							     xb - xa + 1,
							     yb - ya + 1);
	    }
#endif
	}
    }

#ifdef USE_XAA
///    rhdPtr->TwoDInfo->dst_pitch_offset &= ~RADEON_DST_TILE_MACRO;
#endif

out:
    REGION_NULL(pScreen, &region);
    DamageEmpty(info->pDamage);
}

#endif /* DAMAGE */

static void RADEONEnablePageFlip(ScreenPtr pScreen)
{
#ifdef DAMAGE
    ScrnInfoPtr         pScrn      = xf86Screens[pScreen->myNum];
    struct rhdDri *       info       = RHDPTR(pScrn)->dri;

    if (info->allowPageFlip) {
	drm_radeon_sarea_t * pSAREAPriv = DRIGetSAREAPrivate(pScreen);
	BoxRec box = { .x1 = 0, .y1 = 0, .x2 = pScrn->virtualX - 1,
		       .y2 = pScrn->virtualY - 1 };
	RegionPtr pReg = REGION_CREATE(pScreen, &box, 1);

	pSAREAPriv->pfState = 1; /*pfAllowPageFlip = 1;*/
	RADEONDRIRefreshArea(pScrn, pReg);
	REGION_DESTROY(pScreen, pReg);
    }
#endif
}

static void RADEONDisablePageFlip(ScreenPtr pScreen)
{
    /* Tell the clients not to pageflip.  How?
     *   -- Field in sarea, plus bumping the window counters.
     *   -- DRM needs to cope with Front-to-Back swapbuffers.
     */
    drm_radeon_sarea_t *  pSAREAPriv = DRIGetSAREAPrivate(pScreen);

    pSAREAPriv->pfState = 0; /*pfAllowPageFlip = 0;*/
}

static void RADEONDRITransitionSingleToMulti3d(ScreenPtr pScreen)
{
    RADEONDisablePageFlip(pScreen);
}

static void RADEONDRITransitionMultiToSingle3d(ScreenPtr pScreen)
{
    /* Let the remaining 3d app start page flipping again */
//    RADEONEnablePageFlip(pScreen);
}

static void RADEONDRITransitionTo3d(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn  = xf86Screens[pScreen->myNum];
    RHDPtr         rhdPtr = RHDPTR(pScrn);
//    struct rhdDri *info   = rhdPtr->dri;
#ifdef USE_XAA
#if 0
    FBAreaPtr      fbarea;
    int            width, height;
#endif

    /* EXA allocates these areas up front, so it doesn't do the following
     * stuff.
     */
    // TODO: Verify
    if (rhdPtr->AccelMethod != RHD_ACCEL_EXA) {
	/* reserve offscreen area for back and depth buffers and textures */

#if 0	// TODO Nuke (back-to-front w/ offsets)
	/* If we still have an area for the back buffer reserved, free it
	 * first so we always start with all free offscreen memory, except
	 * maybe for Xv
	 */
	if (info->backArea) {
	    xf86FreeOffscreenArea(info->backArea);
	    info->backArea = NULL;
        }
#endif

#if 0
	xf86PurgeUnlockedOffscreenAreas(pScreen);

	xf86QueryLargestOffscreenArea(pScreen, &width, &height, 0, 0, 0);
#endif

// FIXME not necessary until we have xvideo
#if 0
	/* Free Xv linear offscreen memory if necessary
	 * FIXME: This is hideous.  What about telling xv "oh btw you have no memory
	 * any more?" -- anholt
	 */
	if (height < (info->depthTexLines + info->backLines)) {
	    RADEONPortPrivPtr portPriv = info->adaptor->pPortPrivates[0].ptr;
	    xf86FreeOffscreenLinear((FBLinearPtr)portPriv->video_memory);
	    portPriv->video_memory = NULL;
	    xf86QueryLargestOffscreenArea(pScreen, &width, &height, 0, 0, 0);
	}
#endif

#if 0 	// TODO Nuke (back-to-front w/ offsets)
	/* Reserve placeholder area so the other areas will match the
	 * pre-calculated offsets
	 * FIXME: We may have other locked allocations and thus this would allocate
	 * in the wrong place.  The XV surface allocations seem likely. -- anholt
	 */
	fbarea = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth,
					   height
					   - info->depthTexLines
					   - info->backLines,
					   pScrn->displayWidth,
					   NULL, NULL, NULL);
	if (!fbarea)
	    xf86DrvMsg(pScreen->myNum, X_ERROR, "Unable to reserve placeholder "
		       "offscreen area, you might experience screen corruption\n");

	info->backArea = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth,
						   info->backLines,
						   pScrn->displayWidth,
						   NULL, NULL, NULL);
	if (!info->backArea)
	    xf86DrvMsg(pScreen->myNum, X_ERROR, "Unable to reserve offscreen "
		       "area for back buffer, you might experience screen "
		       "corruption\n");

	info->depthTexArea = xf86AllocateOffscreenArea(pScreen,
						       pScrn->displayWidth,
						       info->depthTexLines,
						       pScrn->displayWidth,
						       NULL, NULL, NULL);
	if (!info->depthTexArea)
	    xf86DrvMsg(pScreen->myNum, X_ERROR, "Unable to reserve offscreen "
		       "area for depth buffer and textures, you might "
		       "experience screen corruption\n");
#endif

#if 0
	xf86FreeOffscreenArea(fbarea);
#endif
    }
#endif /* USE_XAA */

//  info->have3DWindows = 1;

///    RADEONChangeSurfaces(pScrn);
//    RADEONEnablePageFlip(pScreen);
    
    RADEONDRISetVBlankInterrupt(pScrn, TRUE);

// FIXME
#if 0
    if (info->cursor)
	xf86ForceHWCursor (pScreen, TRUE);
#endif
}

static void RADEONDRITransitionTo2d(ScreenPtr pScreen)
{
    ScrnInfoPtr         pScrn      = xf86Screens[pScreen->myNum];
    struct rhdDri *       info       = RHDPTR(pScrn)->dri;
    drm_radeon_sarea_t *  pSAREAPriv = DRIGetSAREAPrivate(pScreen);

    /* Try flipping back to the front page if necessary */
    if (pSAREAPriv->pfCurrentPage == 1)
	drmCommandNone(info->drmFD, DRM_RADEON_FLIP);

    /* Shut down shadowing if we've made it back to the front page */
    if (pSAREAPriv->pfCurrentPage == 0) {
	RADEONDisablePageFlip(pScreen);
#if 0	// TODO Nuke (back-to-front w/ offsets)
#ifdef USE_XAA
	if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
	    xf86FreeOffscreenArea(info->backArea);
	    info->backArea = NULL;
	}
#endif
#endif
    } else {
	xf86DrvMsg(pScreen->myNum, X_WARNING,
		   "[dri] RADEONDRITransitionTo2d: "
		   "kernel failed to unflip buffers.\n");
    }

#if 0	// TODO Nuke (back-to-front w/ offsets)
#ifdef USE_XAA
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA)
	xf86FreeOffscreenArea(info->depthTexArea);
#endif
#endif

//  info->have3DWindows = 0;

///    RADEONChangeSurfaces(pScrn);

    RADEONDRISetVBlankInterrupt(pScrn, FALSE);

// FIXME
#if 0
    if (info->cursor)
	xf86ForceHWCursor (pScreen, FALSE);
#endif
}

#if defined(DAMAGE) && (DRIINFO_MAJOR_VERSION > 5 ||	\
			(DRIINFO_MAJOR_VERSION == 5 &&	\
			 DRIINFO_MINOR_VERSION >= 1))
static void
RADEONDRIClipNotify(ScreenPtr pScreen, WindowPtr *ppWin, int num)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct rhdDri * info = RHDPTR(pScrn)->dri;

    REGION_UNINIT(pScreen, &info->driRegion);
    REGION_NULL(pScreen, &info->driRegion);

    if (num > 0) {
	int i;

	for (i = 0; i < num; i++) {
	    WindowPtr pWin = ppWin[i];

	    if (pWin) {
		REGION_UNION(pScreen, &info->driRegion, &pWin->clipList,
			     &info->driRegion);
	    }
	}
    }
}
#endif

int RADEONDRIGetPciAperTableSize(ScrnInfoPtr pScrn)
{
    struct rhdDri *  info   = RHDPTR(pScrn)->dri;
    int page_size  = getpagesize();
    int ret_size;
    int num_pages;

    num_pages = (info->pciAperSize * 1024 * 1024) / page_size;

    ret_size = num_pages * sizeof(unsigned int);

    return ret_size;
}

void RADEONDRIAllocatePCIGARTTable(ScrnInfoPtr pScrn)
{
    RHDPtr         rhdPtr = RHDPTR(pScrn);
    struct rhdDri *info   = RHDPTR(pScrn)->dri;

    if (rhdPtr->cardType != RHD_CARD_PCIE /*||
	info->pKernelDRMVersion->version_minor < 19*/)
      return;

#if 0
    if (info->FbSecureSize==0)
      return;

    /* set the old default size of pci gart table */
    if (info->pKernelDRMVersion->version_minor < 26)
      info->pciGartSize = 32768;
#endif

    info->pciGartSize = RADEONDRIGetPciAperTableSize(pScrn);

#if 0
    info->pciGartOffset = (rhdPtr->FbMapSize - info->FbSecureSize);
#endif
    info->pciGartOffset = RHDAllocFb(rhdPtr, info->pciGartSize,
				     "PCIe GART table");
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "Will use %d kb for PCI GART table at offset 0x%x\n",
	       info->pciGartSize/1024, (unsigned)info->pciGartOffset);
}

int RADEONDRISetParam(ScrnInfoPtr pScrn, unsigned int param, int64_t value)
{
    drm_radeon_setparam_t  radeonsetparam;
    struct rhdDri *  info   = RHDPTR(pScrn)->dri;
    int ret;

    memset(&radeonsetparam, 0, sizeof(drm_radeon_setparam_t));
    radeonsetparam.param = param;
    radeonsetparam.value = value;
    ret = drmCommandWrite(info->drmFD, DRM_RADEON_SETPARAM,
			  &radeonsetparam, sizeof(drm_radeon_setparam_t));
    return ret;
}

// FIXME , call in EnterVT, RADEONDRIFinishScreenInit
#if 0
static void RADEONAdjustMemMapRegisters(ScrnInfoPtr pScrn, RADEONSavePtr save)
{
    RADEONInfoPtr  info   = RADEONPTR(pScrn);
    CARD32 fb, agp, agp_hi;
    int changed = 0;

    if (info->IsSecondary)
      return;

    radeon_read_mc_fb_agp_location(pScrn, LOC_FB | LOC_AGP, &fb, &agp, &agp_hi);

    if (fb != save->mc_fb_location || agp != save->mc_agp_location ||
	agp_hi != save->mc_agp_location_hi)
	changed = 1;

    if (changed) {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "DRI init changed memory map, adjusting ...\n");
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "  MC_FB_LOCATION  was: 0x%08lx is: 0x%08lx\n",
		   (long unsigned int)save->mc_fb_location, (long unsigned int)fb);
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		   "  MC_AGP_LOCATION was: 0x%08lx is: 0x%08lx\n",
		   (long unsigned int)save->mc_agp_location, (long unsigned int)agp);
	save->mc_fb_location = fb;
	save->mc_agp_location = agp;
	save->mc_agp_location_hi = agp_hi;
	if (info->ChipFamily >= CHIP_FAMILY_R600)
	    info->fbLocation = (save->mc_fb_location & 0xffff) << 24;
	else
	    info->fbLocation = (save->mc_fb_location & 0xffff) << 16;

	if (!IS_AVIVO_VARIANT) {
	    save->display_base_addr = info->fbLocation;
	    save->display2_base_addr = info->fbLocation;
	    save->ov0_base_addr = info->fbLocation;
	}

	// TODO Nuke (back-to-front w/ offsets)
	((struct R5xx2DInfo *)rhdPtr->TwoDInfo)->dst_pitch_offset =
	    (((pScrn->displayWidth * info->CurrentLayout.pixel_bytes / 64)
	      << 22) | ((info->fbLocation + pScrn->fbOffset) >> 10));

	RADEONRestoreMemMapRegisters(pScrn, save);
    }

#if 0
#ifdef USE_EXA
    if (info->accelDFS)
    {
	drmRadeonGetParam gp;
	int gart_base;

	memset(&gp, 0, sizeof(gp));
	gp.param = RADEON_PARAM_GART_BASE;
	gp.value = &gart_base;

	if (drmCommandWriteRead(info->drmFD, DRM_RADEON_GETPARAM, &gp,
				sizeof(gp)) < 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Failed to determine GART area MC location, not using "
		       "accelerated DownloadFromScreen hook!\n");
	    info->accelDFS = FALSE;
	} else {
	    info->gartLocation = gart_base;
	}
    }
#endif /* USE_EXA */
#endif
}
#endif

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

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *
 */

#ifndef _RADEON_DRI_
#define _RADEON_DRI_

#include "GL/glxint.h"
#include "xf86drm.h"

/* FIXME: USE_XAA not used in rhd, but considered mandatory */
#define USE_XAA 1

/* FIXME: probably to be put somewhere else */
typedef enum {
	CARD_PCI,
	CARD_AGP,
	CARD_PCIE
} RADEONCardType;

/* dripriv.h entries */
#define RADEON_MAX_DRAWABLES 256

extern void GlxSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
				void **configprivs);

typedef struct {
    /* Nothing here yet */
    int dummy;
} RADEONConfigPrivRec, *RADEONConfigPrivPtr;

typedef struct {
#ifdef PER_CONTEXT_SAREA
    drm_context_t ctx_id;
    drm_handle_t sarea_handle;
#else
    /* Nothing here yet */
    int dummy;
#endif
} RADEONDRIContextRec, *RADEONDRIContextPtr;

/* DRI Driver defaults */
#define RADEON_DEFAULT_CP_PIO_MODE    RADEON_CSQ_PRIPIO_INDPIO
#define RADEON_DEFAULT_CP_BM_MODE     RADEON_CSQ_PRIBM_INDBM
#define RADEON_DEFAULT_GART_SIZE      8 /* MB (must be 2^n and > 4MB) */
#define RADEON_DEFAULT_RING_SIZE      1 /* MB (must be page aligned) */
#define RADEON_DEFAULT_BUFFER_SIZE    2 /* MB (must be page aligned) */
#define RADEON_DEFAULT_GART_TEX_SIZE  1 /* MB (must be page aligned) */

#define RADEON_DEFAULT_CP_TIMEOUT     10000  /* usecs */

#define RADEON_DEFAULT_PCI_APER_SIZE 32 /* in MB */

#define RADEON_CARD_TYPE_RADEON       1

#define RADEONCP_USE_RING_BUFFER(m)					\
    (((m) == RADEON_CSQ_PRIBM_INDDIS) ||				\
     ((m) == RADEON_CSQ_PRIBM_INDBM))

typedef struct {
    /* DRI screen private data */
    int           deviceID;	/* PCI device ID */
    int           width;	/* Width in pixels of display */
    int           height;	/* Height in scanlines of display */
    int           depth;	/* Depth of display (8, 15, 16, 24) */
    int           bpp;		/* Bit depth of display (8, 16, 24, 32) */

    int           IsPCI;	/* Current card is a PCI card */
    int           AGPMode;

    int           frontOffset;  /* Start of front buffer */
    int           frontPitch;
    int           backOffset;   /* Start of shared back buffer */
    int           backPitch;
    int           depthOffset;  /* Start of shared depth buffer */
    int           depthPitch;
    int           textureOffset;/* Start of texture data in frame buffer */
    int           textureSize;
    int           log2TexGran;

    /* MMIO register data */
    drm_handle_t     registerHandle;
    drmSize       registerSize;

    /* CP in-memory status information */
    drm_handle_t     statusHandle;
    drmSize       statusSize;

    /* CP GART Texture data */
    drm_handle_t     gartTexHandle;
    drmSize       gartTexMapSize;
    int           log2GARTTexGran;
    int           gartTexOffset;
    unsigned int  sarea_priv_offset;

#ifdef PER_CONTEXT_SAREA
    drmSize      perctx_sarea_size;
#endif
} RADEONDRIRec, *RADEONDRIPtr;

/* data only needed by dri */
/* TODO: this contains some data also used/needed by other subsystems,
 * like memory layout. Have to clean that up */
struct rhdDri {
    int               scrnIndex;

    pciVideoPtr       PciInfo;
    int               Chipset;

    unsigned long     LinearAddr;       /* Frame buffer physical address     */
    unsigned long     MMIOAddr;         /* MMIO region physical address      */

#if 0
    void              *MMIO;            /* Map of MMIO region                */
#endif
    void              *FB;              /* Map of frame buffer               */

    CARD32            BusCntl;
    unsigned long     MMIOSize;         /* MMIO region physical address      */
    unsigned long     FbMapSize;        /* Size of frame buffer, in bytes    */
    unsigned long     FbSecureSize;     /* Size of secured fb area at end of
                                           framebuffer */

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
#ifdef USE_XAA
    XAAInfoRecPtr     accel;
#endif
// FIXME
//    xf86CursorInfoPtr cursor;
    Bool              allowColorTiling;
    Bool              tilingEnabled; /* mirror of sarea->tiling_enabled */

    int               pixel_code;
    CARD32            dst_pitch_offset;

    Bool              noBackBuffer;
    Bool              directRenderingEnabled;
    Bool              directRenderingInited;
    drmVersionPtr     pLibDRMVersion;
    drmVersionPtr     pKernelDRMVersion;
    DRIInfoPtr        pDRIInfo;
    int               drmFD;
    int               numVisualConfigs;
    __GLXvisualConfig *pVisualConfigs;
    RADEONConfigPrivPtr pVisualConfigsPriv;
    Bool             (*DRICloseScreen)(int, ScreenPtr);

#if 0
    drm_handle_t         fbHandle;

    drmSize           registerSize;
#endif
    drm_handle_t         registerHandle;

    RADEONCardType    cardType;            /* Current card is a PCI card */
    drm_handle_t         pciMemHandle;

    Bool              depthMoves;       /* Enable depth moves -- slow! */
    Bool              allowPageFlip;    /* Enable 3d page flipping */
#ifdef DAMAGE
    DamagePtr         pDamage;
    RegionRec         driRegion;
#endif
    Bool              have3DWindows;    /* Are there any 3d clients? */

    int               pciAperSize;
    drmSize           gartSize;
    drm_handle_t         agpMemHandle;     /* Handle from drmAgpAlloc */
    unsigned long     gartOffset;
    int               agpMode;

//    Bool              CPRuns;           /* CP is running */
    Bool              CPInUse;          /* CP has been used by X server */
    Bool              CPStarted;        /* CP has started */
    int               CPMode;           /* CP mode that server/clients use */
    int               CPFifoSize;       /* Size of the CP command FIFO */
    int               CPusecTimeout;    /* CP timeout in usecs */
    Bool              needCacheFlush;

				/* CP ring buffer data */
    unsigned long     ringStart;        /* Offset into GART space */
    drm_handle_t         ringHandle;       /* Handle from drmAddMap */
    drmSize           ringMapSize;      /* Size of map */
    int               ringSize;         /* Size of ring (in MB) */
    drmAddress        ring;             /* Map */
    int               ringSizeLog2QW;

    unsigned long     ringReadOffset;   /* Offset into GART space */
    drm_handle_t         ringReadPtrHandle; /* Handle from drmAddMap */
    drmSize           ringReadMapSize;  /* Size of map */
    drmAddress        ringReadPtr;      /* Map */

				/* CP vertex/indirect buffer data */
    unsigned long     bufStart;         /* Offset into GART space */
    drm_handle_t         bufHandle;        /* Handle from drmAddMap */
    drmSize           bufMapSize;       /* Size of map */
    int               bufSize;          /* Size of buffers (in MB) */
    drmAddress        buf;              /* Map */
    int               bufNumBufs;       /* Number of buffers */
    drmBufMapPtr      buffers;          /* Buffer map */

				/* CP GART Texture data */
    unsigned long     gartTexStart;      /* Offset into GART space */
    drm_handle_t         gartTexHandle;     /* Handle from drmAddMap */
    drmSize           gartTexMapSize;    /* Size of map */
    int               gartTexSize;       /* Size of GART tex space (in MB) */
    drmAddress        gartTex;           /* Map */
    int               log2GARTTexGran;

				/* CP accleration */
    drmBufPtr         indirectBuffer;
    int               indirectStart;

				/* DRI screen private data */
//    int               fbX;
//    int               fbY;
    int               backX;
    int               backY;
//    int               depthX;
//    int               depthY;

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
    void              *pciGartBackup;
#ifdef USE_XAA
    CARD32            frontPitchOffset;
    CARD32            backPitchOffset;

				/* offscreen memory management */
    int               backLines;
    FBAreaPtr         backArea;
    int               depthTexLines;
    FBAreaPtr         depthTexArea;
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

    /* Debugging info for BEGIN_RING/ADVANCE_RING pairs. */
    int               dma_begin_count;
    char              *dma_debug_func;
    int               dma_debug_lineno;

    /* general */
    Bool              useEXA;

    /* X itself has the 3D context */
    Bool              XInited3D;

    Bool want_vblank_interrupts;

} ;

/*
 * TODO: From other sources, potentially to be put somewhere else
 */
static __inline__ void RADEON_MARK_SYNC(struct rhdDri *info, ScrnInfoPtr pScrn)
{
#ifdef USE_EXA
    if (info->useEXA)
	exaMarkSync(pScrn->pScreen);
#endif
#ifdef USE_XAA
    if (!info->useEXA)
	SET_SYNC_FLAG(info->accel);
#endif
}

static __inline__ void RADEON_SYNC(struct rhdDri *info, ScrnInfoPtr pScrn)
{
#ifdef USE_EXA
    if (info->useEXA)
	exaWaitSync(pScrn->pScreen);
#endif
#ifdef USE_XAA
    if (!info->useEXA && info->accel)
	info->accel->Sync(pScrn);
#endif
}

/* Compute log base 2 of val */
static __inline__ int RADEONMinBits(int val)
{
    int  bits;

    if (!val) return 1;
    for (bits = 0; val; val >>= 1, ++bits);
    return bits;
}



#endif

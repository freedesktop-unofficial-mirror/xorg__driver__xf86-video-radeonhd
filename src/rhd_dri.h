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
    /* Nothing here yet */
    int dummy;
} RADEONDRIContextRec, *RADEONDRIContextPtr;

/* DRI Driver defaults */
#define RADEON_DEFAULT_CP_PIO_MODE    RADEON_CSQ_PRIPIO_INDPIO
#define RADEON_DEFAULT_CP_BM_MODE     RADEON_CSQ_PRIBM_INDBM
#define RADEON_DEFAULT_GART_SIZE      16 /* MB (must be 2^n and > 4MB) */
#define RADEON_DEFAULT_RING_SIZE      2  /* MB (must be page aligned) */
#define RADEON_DEFAULT_BUFFER_SIZE    2  /* MB (must be page aligned) */

#define RADEON_DEFAULT_CP_TIMEOUT     10000  /* usecs */

#define RADEON_DEFAULT_PCI_APER_SIZE  32 /* in MB */

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
    drm_handle_t  registerHandle;
    drmSize       registerSize;

    /* CP in-memory status information */
    drm_handle_t  statusHandle;
    drmSize       statusSize;

    /* CP GART Texture data */
    drm_handle_t  gartTexHandle;
    drmSize       gartTexMapSize;
    int           log2GARTTexGran;
    int           gartTexOffset;
    unsigned int  sarea_priv_offset;

#ifdef PER_CONTEXT_SAREA
    drmSize      perctx_sarea_size;
#endif
} RADEONDRIRec, *RADEONDRIPtr;

/* partially from radeon.h */
extern Bool RADEONDRIPreInit(ScrnInfoPtr pScrn);
extern Bool RADEONDRIAllocateBuffers(ScrnInfoPtr pScrn);
extern Bool RADEONDRIScreenInit(ScreenPtr pScreen);

extern Bool RADEONDRICloseScreen(ScreenPtr pScreen);
extern Bool RADEONDRIFinishScreenInit(ScreenPtr pScreen);
extern int RADEONDRIGetPciAperTableSize(ScrnInfoPtr pScrn);
extern void RADEONDRIEnterVT(ScreenPtr pScreen);
extern void RADEONDRILeaveVT(ScreenPtr pScreen);
extern Bool RADEONDRIScreenInit(ScreenPtr pScreen);
extern int RADEONDRISetParam(ScrnInfoPtr pScrn,
			     unsigned int param, int64_t value);

/*
 * TODO: From other sources, potentially to be put somewhere else
 */
static __inline__ void RADEON_MARK_SYNC(struct rhdDri *info, ScrnInfoPtr pScrn)
{
#if 0
#ifdef USE_EXA
    if (info->useEXA)
	exaMarkSync(pScrn->pScreen);
#endif
#ifdef USE_XAA
    if (!info->useEXA)
	SET_SYNC_FLAG(info->accel);
#endif
#endif
}

static __inline__ void RADEON_SYNC(struct rhdDri *info, ScrnInfoPtr pScrn)
{
#if 0
#ifdef USE_EXA
    if (info->useEXA)
	exaWaitSync(pScrn->pScreen);
#endif
#ifdef USE_XAA
    if (!info->useEXA && info->accel)
	info->accel->Sync(pScrn);
#endif
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

/* to be put into rhd_regs.h? Check for official names */
#define RADEON_CP_CSQ_CNTL                  0x0740
#       define RADEON_CSQ_CNT_PRIMARY_MASK     (0xff << 0)
#       define RADEON_CSQ_PRIDIS_INDDIS        (0    << 28)
#       define RADEON_CSQ_PRIPIO_INDDIS        (1    << 28)
#       define RADEON_CSQ_PRIBM_INDDIS         (2    << 28)
#       define RADEON_CSQ_PRIPIO_INDBM         (3    << 28)
#       define RADEON_CSQ_PRIBM_INDBM          (4    << 28)
#       define RADEON_CSQ_PRIPIO_INDPIO        (15   << 28)

#endif

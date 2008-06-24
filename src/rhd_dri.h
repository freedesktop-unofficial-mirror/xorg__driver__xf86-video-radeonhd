/*
 * Copyright 2000  ATI Technologies Inc., Markham, Ontario,
 * Copyright 2000  VA Linux Systems Inc., Fremont, California.
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
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

#ifndef _RHD_DRI_
#define _RHD_DRI_

#include "GL/glxint.h"
#include "xf86drm.h"

extern Bool RHDDRIPreInit(ScrnInfoPtr pScrn);
extern Bool RHDDRIAllocateBuffers(ScrnInfoPtr pScrn);
extern Bool RHDDRIScreenInit(ScreenPtr pScreen);

extern Bool RHDDRICloseScreen(ScreenPtr pScreen);
extern Bool RHDDRIFinishScreenInit(ScreenPtr pScreen);
extern void RHDDRIEnterVT(ScreenPtr pScreen);
extern void RHDDRILeaveVT(ScreenPtr pScreen);
extern Bool RHDDRIScreenInit(ScreenPtr pScreen);

/* FIXME: to be put into rhd_regs.h? Check for official names */
#define RADEON_CP_CSQ_CNTL                  0x0740
#       define RADEON_CSQ_CNT_PRIMARY_MASK     (0xff << 0)
#       define RADEON_CSQ_PRIDIS_INDDIS        (0    << 28)
#       define RADEON_CSQ_PRIPIO_INDDIS        (1    << 28)
#       define RADEON_CSQ_PRIBM_INDDIS         (2    << 28)
#       define RADEON_CSQ_PRIPIO_INDBM         (3    << 28)
#       define RADEON_CSQ_PRIBM_INDBM          (4    << 28)
#       define RADEON_CSQ_PRIPIO_INDPIO        (15   << 28)

typedef struct {
    /* Nothing here yet */
    int dummy;
} RADEONConfigPrivRec, *RADEONConfigPrivPtr;

typedef struct {
    /* Nothing here yet */
    int dummy;
}
RADEONDRIContextRec, *RADEONDRIContextPtr;

struct rhdDri {
    int               scrnIndex;

    /* FIXME: Some Save&Restore is still a TODO
     * Need to save/restore/update GEN_INT_CNTL (interrupts) on drm init.
     * AGP_BASE, MC_FB_LOCATION, MC_AGP_LOCATION are (partially) handled
     * in _mc.c */

#if 0
    /* TODO: color tiling
     * discuss: should front buffer ever be tiled?
     * should xv surfaces ever be tiled?
     * should anything else ever *not* be tiled?) */
    Bool              allowColorTiling;
    Bool              tilingEnabled; /* mirror of sarea->tiling_enabled */
#endif

    DRIInfoPtr        pDRIInfo;
    int               drmFD;
    int               numVisualConfigs;
    __GLXvisualConfig *pVisualConfigs;
    RADEONConfigPrivPtr pVisualConfigsPriv;

    drm_handle_t      registerHandle;
    drm_handle_t      pciMemHandle;
    int               irq;

    int               have3Dwindows;

    drmSize           gartSize;
    drm_handle_t      agpMemHandle;     /* Handle from drmAgpAlloc */
    unsigned long     gartOffset;
    int               agpMode;
    Bool              CPStarted;

    /* CP ring buffer data */
    unsigned long     ringStart;        /* Offset into GART space */
    drm_handle_t      ringHandle;       /* Handle from drmAddMap */
    drmSize           ringMapSize;      /* Size of map */
    int               ringSize;         /* Size of ring (in MB) */
    drmAddress        ring;             /* Map */
    int               ringSizeLog2QW;

    /* TODO: what is r/o ring space for (1 page) */
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
    int               bufNumBufs;       /* Number of buffers */
    drmBufMapPtr      buffers;          /* Buffer map */

    /* CP GART Texture data */
    unsigned long     gartTexStart;      /* Offset into GART space */
    drm_handle_t      gartTexHandle;     /* Handle from drmAddMap */
    drmSize           gartTexMapSize;    /* Size of map */
    int               gartTexSize;       /* Size of GART tex space (in MB) */
    drmAddress        gartTex;           /* Map */
    int               log2GARTTexGran;

    /* DRI screen private data */
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
    void             *pciGartBackup;
#define GART_LOCATION_INVALID (~(CARD32)0)
    uint32_t         gartLocation;
};

#endif

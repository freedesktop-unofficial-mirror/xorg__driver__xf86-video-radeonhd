/*
 * Copyright 2005 Eric Anholt
 * Copyright 2005 Benjamin Herrenschmidt
 * Copyright 2006 Tungsten Graphics, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *    Zack Rusin <zrusin@trolltech.com>
 *    Benjamin Herrenschmidt <benh@kernel.crashing.org>
 *    Michel Dänzer <michel@tungstengraphics.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
#else
# include <string.h>
#endif

#include "rhd.h"
#include "r5xx_accel.h"
#include "r5xx_regs.h"

#include "exa.h"

extern struct R5xxRop R5xxRops[];

struct R5xxExaPrivate {
    int scrnIndex;

    int xdir;
    int ydir;

    int exaSyncMarker;
    int exaMarkerSynced;
#if X_BYTE_ORDER == X_BIG_ENDIAN
    unsigned long swapper_surfaces[3];
#endif /* X_BYTE_ORDER */
};

/*
 * Helpers.
 */
inline CARD8
R5xxEXADatatypeGet(int bitsPerPixel)
{
    switch (bitsPerPixel) {
    case 8:
	return R5XX_DATATYPE_CI8;
    case 16:
	return R5XX_DATATYPE_RGB565;
    case 32:
	return R5XX_DATATYPE_ARGB8888;
    default:
	return 0;
    }
}

#define R5XX_EXA_PITCH_CHECK(pitch) (((pitch) >= 0x4000) || ((pitch) & 0x003F))
#define R5XX_EXA_OFFSET_CHECK(offset) ((offset) & 0xFFF)
#define RHDPTRE(p) (RHDPTR(xf86Screens[(p)->myNum]))

/*
 *
 */
static int
R5xxEXAMarkSync(ScreenPtr pScreen)
{
    struct R5xxExaPrivate *ExaPrivate = RHDPTR(xf86Screens[pScreen->myNum])->TwoDPrivate;

    ExaPrivate->exaSyncMarker++;

    return ExaPrivate->exaSyncMarker;
}

/*
 *
 */
static void
R5xxEXASync(ScreenPtr pScreen, int marker)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct R5xxExaPrivate *ExaPrivate = RHDPTR(pScrn)->TwoDPrivate;

    if (ExaPrivate->exaMarkerSynced != marker) {
	R5xx2DIdle(pScrn);
	ExaPrivate->exaMarkerSynced = marker;
    }
}

/*
 *
 */
static Bool
R5xxEXAPrepareSolid(PixmapPtr pPix, int alu, Pixel pm, Pixel fg)
{
    RHDPtr rhdPtr = RHDPTRE(pPix->drawable.pScreen);
    CARD32 datatype, pitch, offset;

    datatype = R5xxEXADatatypeGet(pPix->drawable.bitsPerPixel);
    if (!datatype) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Unsupported bitdepth %d\n",
		   __func__, pPix->drawable.bitsPerPixel);
	return FALSE;
    }

    pitch = exaGetPixmapPitch(pPix);
    if (R5XX_EXA_PITCH_CHECK(pitch)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid pitch: %d\n",
		   __func__, (unsigned int) pitch);
	return FALSE;
    }

    offset = exaGetPixmapOffset(pPix);
    if (R5XX_EXA_OFFSET_CHECK(offset)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid offset: %d\n",
		   __func__, (unsigned int) offset);
	return FALSE;
    }
    offset += rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    R5xxFIFOWait(rhdPtr->scrnIndex, 5);
    RHDRegWrite(rhdPtr, R5XX_DP_GUI_MASTER_CNTL, R5XX_GMC_DST_PITCH_OFFSET_CNTL |
		R5XX_GMC_BRUSH_SOLID_COLOR | (datatype << 8) |
		R5XX_GMC_SRC_DATATYPE_COLOR | R5xxRops[alu].pattern |
		R5XX_GMC_CLR_CMP_CNTL_DIS);
    RHDRegWrite(rhdPtr, R5XX_DP_BRUSH_FRGD_CLR, fg);
    RHDRegWrite(rhdPtr, R5XX_DP_WRITE_MASK, pm);
    RHDRegWrite(rhdPtr, R5XX_DP_CNTL,
		R5XX_DST_X_LEFT_TO_RIGHT | R5XX_DST_Y_TOP_TO_BOTTOM);
    RHDRegWrite(rhdPtr, R5XX_DST_PITCH_OFFSET, (pitch << 16) | (offset >> 10));

    return TRUE;
}

/*
 *
 */
static void
R5xxEXASolid(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    RHDPtr rhdPtr = RHDPTRE(pPix->drawable.pScreen);

    R5xxFIFOWait(rhdPtr->scrnIndex, 2);
    RHDRegWrite(rhdPtr, R5XX_DST_Y_X, (y1 << 16) | x1);
    RHDRegWrite(rhdPtr, R5XX_DST_HEIGHT_WIDTH, ((y2 - y1) << 16) | (x2 - x1));
}

/*
 *
 */
static void
R5xxEXADoneSolid(PixmapPtr pPix)
{
    ;
}

/*
 *
 */
static Bool
R5xxEXAPrepareCopy(PixmapPtr pSrc, PixmapPtr pDst, int xdir, int ydir, int rop,
		   Pixel planemask)
{
    RHDPtr rhdPtr = RHDPTRE(pDst->drawable.pScreen);
    struct R5xxExaPrivate *ExaPrivate = rhdPtr->TwoDPrivate;
    CARD32 datatype, srcpitch, srcoffset, dstpitch, dstoffset;

    ExaPrivate->xdir = xdir;
    ExaPrivate->ydir = ydir;

    datatype = R5xxEXADatatypeGet(pDst->drawable.bitsPerPixel);
    if (!datatype) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Unsupported bitdepth %d\n",
		   __func__, pDst->drawable.bitsPerPixel);
	return FALSE;
    }

    srcpitch = exaGetPixmapPitch(pSrc);
    if (R5XX_EXA_PITCH_CHECK(srcpitch)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid source pitch: %d\n",
		   __func__, (unsigned int) srcpitch);
	return FALSE;
    }

    srcoffset = exaGetPixmapOffset(pSrc);
    if (R5XX_EXA_OFFSET_CHECK(srcoffset)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid source offset: %d\n",
		   __func__, (unsigned int) srcoffset);
	return FALSE;
    }
    srcoffset += rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    dstpitch = exaGetPixmapPitch(pDst);
    if (R5XX_EXA_PITCH_CHECK(dstpitch)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid destination pitch: %d\n",
		   __func__, (unsigned int) dstpitch);
	return FALSE;
    }

    dstoffset = exaGetPixmapOffset(pDst);
    if (R5XX_EXA_OFFSET_CHECK(dstoffset)) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "%s: Invalid destination offset: %d\n",
		   __func__, (unsigned int) dstoffset);
	return FALSE;
    }
    dstoffset += rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    R5xxFIFOWait(rhdPtr->scrnIndex, 5);
    RHDRegWrite(rhdPtr, R5XX_DP_GUI_MASTER_CNTL,
		R5XX_GMC_DST_PITCH_OFFSET_CNTL | R5XX_GMC_SRC_PITCH_OFFSET_CNTL |
		R5XX_GMC_BRUSH_NONE | (datatype << 8) |
		R5XX_GMC_SRC_DATATYPE_COLOR | R5xxRops[rop].rop |
		R5XX_DP_SRC_SOURCE_MEMORY | R5XX_GMC_CLR_CMP_CNTL_DIS);
    RHDRegWrite(rhdPtr, R5XX_DP_WRITE_MASK, planemask);
    RHDRegWrite(rhdPtr, R5XX_DP_CNTL,
		((xdir >= 0 ? R5XX_DST_X_LEFT_TO_RIGHT : 0) |
		 (ydir >= 0 ? R5XX_DST_Y_TOP_TO_BOTTOM : 0)));
    RHDRegWrite(rhdPtr, R5XX_DST_PITCH_OFFSET, (dstpitch << 16) | (dstoffset >> 10));
    RHDRegWrite(rhdPtr, R5XX_SRC_PITCH_OFFSET, (srcpitch << 16) | (srcoffset >> 10));

    return TRUE;
}

/*
 *
 */
void
R5xxEXACopy(PixmapPtr pDst, int srcX, int srcY, int dstX, int dstY, int w, int h)
{
    RHDPtr rhdPtr = RHDPTRE(pDst->drawable.pScreen);
    struct R5xxExaPrivate *ExaPrivate = rhdPtr->TwoDPrivate;

    if (ExaPrivate->xdir < 0) {
	srcX += w - 1;
	dstX += w - 1;
    }
    if (ExaPrivate->ydir < 0) {
	srcY += h - 1;
	dstY += h - 1;
    }

    R5xxFIFOWait(rhdPtr->scrnIndex, 3);

    RHDRegWrite(rhdPtr, R5XX_SRC_Y_X, (srcY << 16) | srcX);
    RHDRegWrite(rhdPtr, R5XX_DST_Y_X, (dstY << 16) | dstX);
    RHDRegWrite(rhdPtr, R5XX_DST_HEIGHT_WIDTH, (h  << 16) | w);
}

/*
 *
 */
static void
R5xxEXADoneCopy(PixmapPtr pDst)
{
    ;
}

/*
 *
 */
static Bool
R5xxEXAUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
		      char *src, int src_pitch)
{
    RHDPtr rhdPtr = RHDPTRE(pDst->drawable.pScreen);
    CARD8 *dst = ((CARD8 *) rhdPtr->FbBase) +
	rhdPtr->FbScanoutStart + exaGetPixmapOffset(pDst);
    int dst_pitch = exaGetPixmapPitch(pDst);

    if (pDst->drawable.bitsPerPixel < 8)
	return FALSE;

    /* Do we need that sync here ? probably not .... */
    exaWaitSync(pDst->drawable.pScreen);

    w *= pDst->drawable.bitsPerPixel / 8;
    dst += (x * pDst->drawable.bitsPerPixel / 8) + (y * dst_pitch);

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
}

/*
 *
 */
static Bool
R5xxEXADownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
			  char *dst, int dst_pitch)
{
    RHDPtr rhdPtr = RHDPTRE(pSrc->drawable.pScreen);
    CARD8 *src = ((CARD8 *) rhdPtr->FbBase) +
	rhdPtr->FbScanoutStart + exaGetPixmapOffset(pSrc);
    int	src_pitch = exaGetPixmapPitch(pSrc);

    /* Can't accelerate download */
    exaWaitSync(pSrc->drawable.pScreen);

    src += (x * pSrc->drawable.bitsPerPixel / 8) + (y * src_pitch);
    w *= pSrc->drawable.bitsPerPixel / 8;

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
}


#if X_BYTE_ORDER == X_BIG_ENDIAN

/*
 * offset will undoubtedly have to be adapted to the moving scanout + offscreen.
 */
static Bool
R5xxEXAPrepareAccess(PixmapPtr pPix, int index)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    struct R5xxExaPrivate *ExaPrivate = RHDPTR(pScrn)->TwoDPrivate;
    CARD32 offset = exaGetPixmapOffset(pPix);
    int soff;
    CARD32 size, flags;

    /* Front buffer is always set with proper swappers */
    if (offset == 0)
        return TRUE;

    /* If same bpp as front buffer, just do nothing as the main
     * swappers will apply
     */
    if (pPix->drawable.bitsPerPixel == pScrn->bitsPerPixel)
        return TRUE;

    /* We need to setup a separate swapper, let's request a
     * surface. We need to align the size first
     */
    size = RHD_FB_CHUNK(exaGetPixmapSize(pPix));

    /* Set surface to tiling disabled with appropriate swapper */
    switch (pPix->drawable.bitsPerPixel) {
    case 16:
        flags = R5XX_SURF_AP0_SWP_16BPP | R5XX_SURF_AP1_SWP_16BPP;
	break;
    case 32:
        flags = R5XX_SURF_AP0_SWP_32BPP | R5XX_SURF_AP1_SWP_32BPP;
	break;
    default:
        flags = 0;
    }

    soff = (index + 1) * 0x10;
    RHDRegWrite(pScrn, R5XX_SURFACE0_INFO + soff, flags);
    RHDRegWrite(pScrn, R5XX_SURFACE0_LOWER_BOUND + soff, offset);
    RHDRegWrite(pScrn, R5XX_SURFACE0_UPPER_BOUND + soff, offset + size - 1);
    ExaPrivate->swapper_surfaces[index] = offset;
    return TRUE;
}

/*
 *
 */
static void
R5xxEXAFinishAccess(PixmapPtr pPix, int index)
{
    RHDPtr rhdPtr = RHDPTRE(pPix->drawable.pScreen);
    struct R5xxExaPrivate *ExaPrivate = rhdPtr->TwoDPrivate;
    CARD32 offset = exaGetPixmapOffset(pPix);
    int soff;

    /* Front buffer is always set with proper swappers */
    if (offset == 0)
        return;

    if (ExaPrivate->swapper_surfaces[index] == 0)
        return;

    soff = (index + 1) * 0x10;
    RHDRegWrite(rhdPtr, R5XX_SURFACE0_INFO + soff, 0);
    RHDRegWrite(rhdPtr, R5XX_SURFACE0_LOWER_BOUND + soff, 0);
    RHDRegWrite(rhdPtr, R5XX_SURFACE0_UPPER_BOUND + soff, 0);
    ExaPrivate->swapper_surfaces[index] = 0;
}

#endif /* X_BYTE_ORDER == X_BIG_ENDIAN */

/*
 *
 */
Bool
R5xxEXAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    ExaDriverRec *EXAInfo;
    struct R5xxExaPrivate *ExaPrivate;

    RHDFUNC(pScrn);

    EXAInfo = exaDriverAlloc();
    if (EXAInfo == NULL)
	return FALSE;

    EXAInfo->exa_major = EXA_VERSION_MAJOR;
    EXAInfo->exa_minor = EXA_VERSION_MINOR;

    EXAInfo->flags = EXA_OFFSCREEN_PIXMAPS;
    EXAInfo->pixmapOffsetAlign = 0x1000;
    EXAInfo->pixmapPitchAlign = 64;

#if EXA_VERSION_MAJOR > 2 || (EXA_VERSION_MAJOR == 2 && EXA_VERSION_MINOR >= 3)
    EXAInfo->maxPitchBytes = 16320;
    EXAInfo->maxX = 8192;
#else
    EXAInfo->maxX = 16320 / 4;
#endif
    EXAInfo->maxY = 8192;

    EXAInfo->memoryBase = (CARD8 *) rhdPtr->FbBase + rhdPtr->FbScanoutStart;
    EXAInfo->offScreenBase = rhdPtr->FbOffscreenStart - rhdPtr->FbScanoutStart;
    EXAInfo->memorySize = rhdPtr->FbScanoutSize + rhdPtr->FbOffscreenSize;

    EXAInfo->PrepareSolid = R5xxEXAPrepareSolid;
    EXAInfo->Solid = R5xxEXASolid;
    EXAInfo->DoneSolid = R5xxEXADoneSolid;

    EXAInfo->PrepareCopy = R5xxEXAPrepareCopy;
    EXAInfo->Copy = R5xxEXACopy;
    EXAInfo->DoneCopy = R5xxEXADoneCopy;

    EXAInfo->MarkSync = R5xxEXAMarkSync;
    EXAInfo->WaitMarker = R5xxEXASync;
    EXAInfo->UploadToScreen = R5xxEXAUploadToScreen;
    EXAInfo->DownloadFromScreen = R5xxEXADownloadFromScreen;

#if X_BYTE_ORDER == X_BIG_ENDIAN
    EXAInfo->PrepareAccess = R5xxEXAPrepareAccess;
    EXAInfo->FinishAccess = R5xxEXAFinishAccess;
#endif /* X_BYTE_ORDER == X_BIG_ENDIAN */

    if (!exaDriverInit(pScreen, EXAInfo)) {
	xfree(EXAInfo);
	return FALSE;
    }

    RHDPTR(pScrn)->EXAInfo = EXAInfo;

    ExaPrivate = xnfcalloc(1, sizeof(struct R5xxExaPrivate));
    ExaPrivate->scrnIndex = pScrn->scrnIndex;

    rhdPtr->TwoDPrivate = ExaPrivate;

    exaMarkSync(pScreen);

    /* start the engine already */
    R5xx2DStart(pScrn);

    return TRUE;
}

/*
 *
 */
void
R5xxEXACloseScreen(ScreenPtr pScreen)
{
    exaDriverFini(pScreen);
}

/*
 *
 */
void
R5xxEXADestroy(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if (rhdPtr->EXAInfo) {
	xfree(rhdPtr->EXAInfo);
	rhdPtr->EXAInfo = NULL;
    }

    if (rhdPtr->TwoDPrivate) {
	xfree(rhdPtr->TwoDPrivate);
	rhdPtr->TwoDPrivate = NULL;
    }
}

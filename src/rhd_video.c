/*
 * Copyright 2008 Alex Deucher
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
 *
 * Based on radeon_exa_render.c and kdrive ati_video.c by Eric Anholt, et al.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef USE_EXA
#include "exa.h"
#endif
#ifdef USE_XAA
#include "xaa.h"
#endif

#include "xf86xv.h"

#include "rhd.h"
#include "rhd_cs.h"

#include "r5xx_regs.h"
#include "rhd_video.h"

#include "xf86.h"
#include "dixstruct.h"
#include "xf86fbman.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

/* @@@ please go away! */
#define IS_R500_3D \
    ((rhdPtr->ChipSet != RHD_RS690) && \
     (rhdPtr->ChipSet != RHD_RS600) && \
     (rhdPtr->ChipSet != RHD_RS740))

#ifdef USE_EXA
static void
ATIVideoSave(ScreenPtr pScreen, ExaOffscreenArea *area)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct RHDPortPriv *pPriv =
	((XF86VideoAdaptorPtr)(rhdPtr->adaptor))->pPortPrivates[0].ptr;

    if (pPriv->video_memory == area)
        pPriv->video_memory = NULL;
}
#endif /* USE_EXA */

/* Allocates memory, either by resizing the allocation pointed to by mem_struct,
 * or by freeing mem_struct (if non-NULL) and allocating a new space.  The size
 * is measured in bytes, and the offset from the beginning of card space is
 * returned.
 */

static CARD32
rhdAllocateMemory(
    ScrnInfoPtr pScrn,
    void **mem_struct,
    int size
    ){
    ScreenPtr pScreen;
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int offset = 0;

    pScreen = screenInfo.screens[pScrn->scrnIndex];
#ifdef USE_EXA
	if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
	    ExaOffscreenArea *area = *mem_struct;

	    if (area != NULL) {
		if (area->size >= size)
		    return area->offset;

		exaOffscreenFree(pScrn->pScreen, area);
	    }

	    area = exaOffscreenAlloc(pScrn->pScreen, size, 64, TRUE, ATIVideoSave,
				     NULL);
	    *mem_struct = area;
	    if (area == NULL)
		return 0;
	    offset = area->offset;
	}
#endif /* USE_EXA */
#ifdef USE_XAA
	if (rhdPtr->AccelMethod == RHD_ACCEL_XAA) {
	    FBLinearPtr linear = *mem_struct;
	    int cpp = pScrn->bitsPerPixel >> 3;

	    /* XAA allocates in units of pixels at the screen bpp, so adjust size
	     * appropriately.
	     */
	    size = (size + cpp - 1) / cpp;

	    if (linear) {
		if (linear->size >= size)
		    return linear->offset * cpp;

		if (xf86ResizeOffscreenLinear(linear, size))
		    return linear->offset * cpp;

		xf86FreeOffscreenLinear(linear);
	    }

	    linear = xf86AllocateOffscreenLinear(pScreen, size, 16,
						 NULL, NULL, NULL);
	    *mem_struct = linear;

	    if (!linear) {
		int max_size;

		xf86QueryLargestOffscreenLinear(pScreen, &max_size, 16,
						PRIORITY_EXTREME);

		if(max_size < size)
		    return 0;

		xf86PurgeUnlockedOffscreenAreas(pScreen);
		linear = xf86AllocateOffscreenLinear(pScreen, size, 16,
						     NULL, NULL, NULL);
		*mem_struct = linear;
		if (!linear)
		    return 0;
	    }
	    offset = linear->offset * cpp;
	}
#endif /* USE_XAA */

    return offset;
}

/*
 *
 */
void
rhdFreeMemory(
   ScrnInfoPtr pScrn,
   void *mem_struct
){
    RHDPtr rhdPtr = RHDPTR(pScrn);

#ifdef USE_EXA
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
	ExaOffscreenArea *area = mem_struct;

	if (area != NULL)
	    exaOffscreenFree(pScrn->pScreen, area);
    }
#endif /* USE_EXA */
#ifdef USE_XAA
    if (rhdPtr->AccelMethod == RHD_ACCEL_XAA) {
	FBLinearPtr linear = mem_struct;

	if (linear != NULL)
	    xf86FreeOffscreenLinear(linear);
    }
#endif /* USE_XAA */
}

/*
 *
 */
static void
rhdStopVideo(ScrnInfoPtr pScrn, pointer data, Bool cleanup)
{
}

/*
 *
 */
static int
rhdSetPortAttribute(ScrnInfoPtr  pScrn,
		       Atom	    attribute,
		       INT32	    value,
		       pointer	    data)
{
    return BadMatch;
}

/*
 *
 */
static int
rhdGetPortAttribute(ScrnInfoPtr  pScrn,
		       Atom	    attribute,
		       INT32	    *value,
		       pointer	    data)
{
    return BadMatch;
}

/*
 *
 */
static void
rhdQueryBestSize(
  ScrnInfoPtr pScrn,
  Bool motion,
  short vid_w, short vid_h,
  short drw_w, short drw_h,
  unsigned int *p_w, unsigned int *p_h,
  pointer data
){

    *p_w = drw_w;
    *p_h = drw_h;
}

/*
 *
 */
static int
rhdQueryImageAttributes(
    ScrnInfoPtr pScrn,
    int id,
    unsigned short *w, unsigned short *h,
    int *pitches, int *offsets
){
    int size, tmp;

    if (*w > 2048) *w = 2048;
    if (*h > 2048) *h = 2048;

    *w = (*w + 1) & ~1;
    if(offsets) offsets[0] = 0;

    switch(id) {
    case FOURCC_YV12:
    case FOURCC_I420:
	*h = (*h + 1) & ~1;
	size = (*w + 3) & ~3;
	if(pitches) pitches[0] = size;
	size *= *h;
	if(offsets) offsets[1] = size;
	tmp = ((*w >> 1) + 3) & ~3;
	if(pitches) pitches[1] = pitches[2] = tmp;
	tmp *= (*h >> 1);
	size += tmp;
	if(offsets) offsets[2] = size;
	size += tmp;
	break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
	size = *w << 1;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    }

    return size;
}

/*
 *
 */
static void
rhdCopyData(
  ScrnInfoPtr pScrn,
  unsigned char *src,
  unsigned char *dst,
  unsigned int srcPitch,
  unsigned int dstPitch,
  unsigned int h,
  unsigned int w,
  unsigned int bpp
){

#ifdef NOT_YET /* TODO, but CP specific. */
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if (rhdPtr->CS->Type == RHD_CS_CPDMA)
    {
	CARD8 *buf;
	CARD32 bufPitch, dstPitchOff;
	int x, y;
	unsigned int hpass;

	/* Get the byte-swapping right for big endian systems */
	if ( bpp == 2 ) {
	    w *= 2;
	    bpp = 1;
	}
	RADEONHostDataParams( pScrn, dst, dstPitch, bpp, &dstPitchOff, &x, &y );

	while ( (buf = RADEONHostDataBlit( pScrn, bpp, w, dstPitchOff, &bufPitch,
					   x, &y, &h, &hpass )) )
	{
	    RADEONHostDataBlitCopyPass( pScrn, bpp, buf, src, hpass, bufPitch,
					srcPitch );
	    src += hpass * srcPitch;
	}

	FLUSH_RING();

	return;
    }
    else
#endif /* NOT_YET */
    {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	unsigned int val, new;
	val = RHDRegRead(rhdPtr, R5XX_SURFACE_CNTL);
	new = val &
	    ~(R5XX_NONSURF_AP0_SWP_32BPP | R5XX_NONSURF_AP1_SWP_32BPP |
	      R5XX_NONSURF_AP0_SWP_16BPP | R5XX_NONSURF_AP1_SWP_16BPP);

	if (bpp == 4)
	    new |= R5XX_NONSURF_AP0_SWP_32BPP
		|  R5XX_NONSURF_AP1_SWP_32BPP;

	RHDRegWrite(rhdPtr, R5XX_SURFACE_CNTL, new);
#endif
	w *= bpp;

	while (h--) {
	    memcpy(dst, src, w);
	    src += srcPitch;
	    dst += dstPitch;
	}

#if X_BYTE_ORDER == X_BIG_ENDIAN
	/* restore byte swapping */
	RHDRegWrite(rhdPtr, R5XX_SURFACE_CNTL, val);
#endif
    }
}

/*
 *
 */
static void
rhdCopyMungedData(
   ScrnInfoPtr pScrn,
   unsigned char *src1,
   unsigned char *src2,
   unsigned char *src3,
   unsigned char *dst1,
   unsigned int srcPitch,
   unsigned int srcPitch2,
   unsigned int dstPitch,
   unsigned int h,
   unsigned int w
){
#ifdef NOT_YET /* TODO, but CP specific. */
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if (rhdPtr->CS->Type == RHD_CS_CPDMA) {
	CARD8 *buf;
	CARD32 y = 0, bufPitch, dstPitchOff;
	int blitX, blitY;
	unsigned int hpass;

	/* XXX Fix endian flip on R300 */

	RADEONHostDataParams( pScrn, dst1, dstPitch, 4, &dstPitchOff, &blitX, &blitY );

	while ( (buf = RADEONHostDataBlit( pScrn, 4, w/2, dstPitchOff, &bufPitch,
					   blitX, &blitY, &h, &hpass )) )
	{
	    while ( hpass-- )
	    {
		unsigned int *d = (unsigned int *) buf;
		unsigned char *s1 = src1, *s2 = src2, *s3 = src3;
		unsigned int n = bufPitch / 4;

		while ( n ) {
		    *(d++) = s1[0] | (s1[1] << 16) | (s3[0] << 8) | (s2[0] << 24);
		    s1 += 2; s2++; s3++;
		    n--;
		}

		src1 += srcPitch;
		if ( y & 1 )
		{
		    src2 += srcPitch2;
		    src3 += srcPitch2;
		}
		buf += bufPitch;
		y++;
	    }
	}

	FLUSH_RING();
    }
    else
#endif /* NOT_YET */
    {
	CARD32 *dst;
	CARD8 *s1, *s2, *s3;
	unsigned int i, j;

#if X_BYTE_ORDER == X_BIG_ENDIAN
	CARD32 val, new;
	val = RHDRegRead(rhdPtr, R5XX_SURFACE_CNTL);
	new = (val | R5XX_NONSURF_AP0_SWP_32BPP) & ~R5XX_NONSURF_AP0_SWP_16BPP);
	RHDRegWrite(rhdPtr, R5XX_SURFACE_CNTL, new);
#endif

	w /= 2;

	for( j = 0; j < h; j++ )
	{
	    dst = (pointer)dst1;
	    s1 = src1;  s2 = src2;  s3 = src3;
	    i = w;
	    while( i > 4 )
	    {
		dst[0] = s1[0] | (s1[1] << 16) | (s3[0] << 8) | (s2[0] << 24);
		dst[1] = s1[2] | (s1[3] << 16) | (s3[1] << 8) | (s2[1] << 24);
		dst[2] = s1[4] | (s1[5] << 16) | (s3[2] << 8) | (s2[2] << 24);
		dst[3] = s1[6] | (s1[7] << 16) | (s3[3] << 8) | (s2[3] << 24);
		dst += 4; s2 += 4; s3 += 4; s1 += 8;
		i -= 4;
	    }
	    while( i-- )
	    {
		dst[0] = s1[0] | (s1[1] << 16) | (s3[0] << 8) | (s2[0] << 24);
		dst++; s2++; s3++;
		s1 += 2;
	    }

	    dst1 += dstPitch;
	    src1 += srcPitch;
	    if( j & 1 )
	    {
		src2 += srcPitch2;
		src3 += srcPitch2;
	    }
	}
#if X_BYTE_ORDER == X_BIG_ENDIAN
	/* restore byte swapping */
	RHDRegWrite(rhdPtr, R5XX_SURFACE_CNTL, val);
#endif
    }
}

/*
 *
 */
static int
rhdPutImageTextured(ScrnInfoPtr pScrn,
		       short src_x, short src_y,
		       short drw_x, short drw_y,
		       short src_w, short src_h,
		       short drw_w, short drw_h,
		       int id,
		       unsigned char *buf,
		       short width,
		       short height,
		       Bool sync,
		       RegionPtr clipBoxes,
		       pointer data,
		       DrawablePtr pDraw)
{
    ScreenPtr pScreen = pScrn->pScreen;
    RHDPtr rhdPtr = RHDPTR(pScrn);
    RHDPortPrivPtr pPriv = (RHDPortPrivPtr)data;
    INT32 x1, x2, y1, y2;
    int srcPitch, srcPitch2, dstPitch;
    int s2offset, s3offset, tmp;
    int top, left, npixels, nlines, size;
    BoxRec dstBox;
    int dst_width = width, dst_height = height;

    /* make the compiler happy */
    s2offset = s3offset = srcPitch2 = 0;

    /* Clip */
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    if (!xf86XVClipVideoHelper(&dstBox, &x1, &x2, &y1, &y2, clipBoxes, width, height))
	return Success;

    src_w = (x2 - x1) >> 16;
    src_h = (y2 - y1) >> 16;
    drw_w = dstBox.x2 - dstBox.x1;
    drw_h = dstBox.y2 - dstBox.y1;

    if ((x1 >= x2) || (y1 >= y2))
	return Success;

    switch(id) {
    case FOURCC_YV12:
    case FOURCC_I420:
	dstPitch = ((dst_width << 1) + 15) & ~15;
	srcPitch = (width + 3) & ~3;
	srcPitch2 = ((width >> 1) + 3) & ~3;
	size = dstPitch * dst_height;
	break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
	dstPitch = ((dst_width << 1) + 15) & ~15;
	srcPitch = (width << 1);
	srcPitch2 = 0;
	size = dstPitch * dst_height;
	break;
    }

   if (rhdPtr->CS->Type == RHD_CS_CPDMA)
       /* The upload blit only supports multiples of 64 bytes */
       dstPitch = (dstPitch + 63) & ~63;
   else
       dstPitch = (dstPitch + 15) & ~15;

    if (pPriv->video_memory != NULL && size != pPriv->size) {
	rhdFreeMemory(pScrn, pPriv->video_memory);
	pPriv->video_memory = NULL;
    }

    if (pPriv->video_memory == NULL) {
	pPriv->video_offset = rhdAllocateMemory(pScrn,
						       &pPriv->video_memory,
						       size * 2);
	if (pPriv->video_offset == 0)
	    return BadAlloc;
    }

    if (pDraw->type == DRAWABLE_WINDOW)
	pPriv->pPixmap = (*pScreen->GetWindowPixmap)((WindowPtr)pDraw);
    else
	pPriv->pPixmap = (PixmapPtr)pDraw;


#if defined(USE_EXA) && ((EXA_VERSION_MAJOR > 2) || (EXA_VERSION_MAJOR == 2 && EXA_VERSION_MINOR >= 1))
    if (rhdPtr->AccelMethod == RHD_ACCEL_EXA) {
	/* Force the pixmap into framebuffer so we can draw to it. */
	exaMoveInPixmap(pPriv->pPixmap);
    } else
#endif
    /*
     * TODO: Copy the pixmap into the FB ourselves!!!
     */
    if (((rhdPtr->AccelMethod != RHD_ACCEL_NONE) || (rhdPtr->AccelMethod != RHD_ACCEL_SHADOWFB)) &&
	(((char *)pPriv->pPixmap->devPrivate.ptr < ((char *)rhdPtr->FbBase + rhdPtr->FbScanoutStart)) ||
	 ((char *)pPriv->pPixmap->devPrivate.ptr >= ((char *)rhdPtr->FbBase + rhdPtr->FbMapSize)))) {
	/* If the pixmap wasn't in framebuffer, then we have no way to
	 * force it there. So, we simply refuse to draw and fail.
	 */
	return BadAlloc;
    }

    /* copy data */
    top = y1 >> 16;
    left = (x1 >> 16) & ~1;
    npixels = ((((x2 + 0xffff) >> 16) + 1) & ~1) - left;

    pPriv->src_offset = pPriv->video_offset + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;
    pPriv->src_addr = ((CARD8 *)rhdPtr->FbBase + rhdPtr->FbScanoutStart + pPriv->video_offset + (top * dstPitch));
    pPriv->src_pitch = dstPitch;
    pPriv->size = size;
    pPriv->pDraw = pDraw;

    switch(id) {
    case FOURCC_YV12:
    case FOURCC_I420:
	top &= ~1;
	nlines = ((((y2 + 0xffff) >> 16) + 1) & ~1) - top;
	s2offset = srcPitch * height;
	s3offset = (srcPitch2 * (height >> 1)) + s2offset;
	top &= ~1;
	pPriv->src_addr += left << 1;
	tmp = ((top >> 1) * srcPitch2) + (left >> 1);
	s2offset += tmp;
	s3offset += tmp;
	if (id == FOURCC_I420) {
	    tmp = s2offset;
	    s2offset = s3offset;
	    s3offset = tmp;
	}
	rhdCopyMungedData(pScrn, buf + (top * srcPitch) + left,
			     buf + s2offset, buf + s3offset, pPriv->src_addr,
			     srcPitch, srcPitch2, dstPitch, nlines, npixels);
	break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
	nlines = ((y2 + 0xffff) >> 16) - top;
	rhdCopyData(pScrn, buf, pPriv->src_addr, srcPitch, dstPitch, nlines, npixels, 2);
	break;
    }

    /* update cliplist */
    if (!REGION_EQUAL(pScrn->pScreen, &pPriv->clip, clipBoxes)) {
	REGION_COPY(pScrn->pScreen, &pPriv->clip, clipBoxes);
    }

    pPriv->id = id;
    pPriv->src_w = src_w;
    pPriv->src_h = src_h;
    pPriv->drw_x = drw_x;
    pPriv->drw_y = drw_y;
    pPriv->dst_w = drw_w;
    pPriv->dst_h = drw_h;
    pPriv->w = width;
    pPriv->h = height;

    RHDRADEONDisplayTexturedVideo(pScrn, pPriv);

    return Success;
}

/*
 *
 */
#define IMAGE_MAX_WIDTH		2048
#define IMAGE_MAX_HEIGHT	2048

#define IMAGE_MAX_WIDTH_R500	4096
#define IMAGE_MAX_HEIGHT_R500	4096

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] =
{
    {
	0,
	"XV_IMAGE",
	IMAGE_MAX_WIDTH, IMAGE_MAX_HEIGHT,
	{1, 1}
    }
};

static XF86VideoEncodingRec DummyEncodingR500[1] =
{
    {
	0,
	"XV_IMAGE",
	IMAGE_MAX_WIDTH_R500, IMAGE_MAX_HEIGHT_R500,
	{1, 1}
    }
};

#define NUM_FORMATS 3

static XF86VideoFormatRec Formats[NUM_FORMATS] =
{
    {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#define NUM_IMAGES 4

static XF86ImageRec Images[NUM_IMAGES] =
{
    XVIMAGE_YUY2,
    XVIMAGE_YV12,
    XVIMAGE_I420,
    XVIMAGE_UYVY
};

/*
 *
 */
static XF86VideoAdaptorPtr
rhdSetupImageTexturedVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr    rhdPtr = RHDPTR(pScrn);
    struct RHDPortPriv *pPortPriv;
    XF86VideoAdaptorPtr adapt;
    int i;
    int num_texture_ports = 16;

    adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) + num_texture_ports *
		    (sizeof(struct RHDPortPriv) + sizeof(DevUnion)));
    if (adapt == NULL)
	return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = 0;
    adapt->name = "RadeonHD Textured Video";
    adapt->nEncodings = 1;
    if (IS_R500_3D)
	adapt->pEncodings = DummyEncodingR500;
    else
	adapt->pEncodings = DummyEncoding;
    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = num_texture_ports;
    adapt->pPortPrivates = (DevUnion*)(&adapt[1]);

    pPortPriv =
	(struct RHDPortPriv *)(&adapt->pPortPrivates[num_texture_ports]);

    adapt->nAttributes = 0;
    adapt->pAttributes = NULL;
    adapt->pImages = Images;
    adapt->nImages = NUM_IMAGES;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = rhdStopVideo;
    adapt->SetPortAttribute = rhdSetPortAttribute;
    adapt->GetPortAttribute = rhdGetPortAttribute;
    adapt->QueryBestSize = rhdQueryBestSize;
    adapt->PutImage = rhdPutImageTextured;
    adapt->ReputImage = NULL;
    adapt->QueryImageAttributes = rhdQueryImageAttributes;

    for (i = 0; i < num_texture_ports; i++) {
	struct RHDPortPriv *pPriv = &pPortPriv[i];

	/* gotta uninit this someplace, XXX: shouldn't be necessary for textured */
	REGION_NULL(pScreen, &pPriv->clip);
	adapt->pPortPrivates[i].ptr = (pointer) (pPriv);
    }
#ifdef USE_EXA
    rhdPtr->adaptor = adapt;  /* this is only needed for exaOffscreenAlloc */
#endif
    return adapt;
}

/*
 *
 */
void
RHDInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr    rhdPtr = RHDPTR(pScrn);
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr texturedAdaptor = NULL;
    int num_adaptors;

    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
    newAdaptors = xalloc((num_adaptors + 2) * sizeof(XF86VideoAdaptorPtr *));
    if (newAdaptors == NULL)
	return;

    memcpy(newAdaptors, adaptors, num_adaptors * sizeof(XF86VideoAdaptorPtr));
    adaptors = newAdaptors;

    if (rhdPtr->ChipSet < RHD_R600 && rhdPtr->TwoDPrivate
	&& (rhdPtr->CS->Type == RHD_CS_CP || rhdPtr->CS->Type == RHD_CS_CPDMA)) {
	texturedAdaptor = rhdSetupImageTexturedVideo(pScreen);
	if (texturedAdaptor != NULL) {
	    adaptors[num_adaptors++] = texturedAdaptor;
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Set up textured video\n");
	} else
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to set up textured video\n");
    } else
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Textured Video only for R5xx/IGP\n");

    if(num_adaptors)
	xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(newAdaptors)
	xfree(newAdaptors);

}



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "rhd.h"
#include "rhd_cp.h"

#include "radeon_reg.h"
#include "radeon_video.h"

#include "xf86.h"
#include "dixstruct.h"
#include "xf86fbman.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define GET_PORT_PRIVATE(pScrn) \
   (RADEONPortPrivPtr)((RHDPTR(pScrn))->adaptor->pPortPrivates[0].ptr)

#define FOURCC_RGBA32   0x41424752

#define XVIMAGE_RGBA32(byte_order)   \
        { \
                FOURCC_RGBA32, \
                XvRGB, \
                byte_order, \
                { 'R', 'G', 'B', 'A', \
                  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
                32, \
                XvPacked, \
                1, \
                32, 0x00FF0000, 0x0000FF00, 0x000000FF, \
                0, 0, 0, 0, 0, 0, 0, 0, 0, \
                {'A', 'R', 'G', 'B', \
                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
                XvTopToBottom \
        }               

#define FOURCC_RGB24    0x00000000

#define XVIMAGE_RGB24   \
        { \
                FOURCC_RGB24, \
                XvRGB, \
                LSBFirst, \
                { 'R', 'G', 'B', 0, \
                  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
                24, \
                XvPacked, \
                1, \
                24, 0x00FF0000, 0x0000FF00, 0x000000FF, \
                0, 0, 0, 0, 0, 0, 0, 0, 0, \
                { 'R', 'G', 'B', \
                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
                XvTopToBottom \
        }

#define FOURCC_RGBT16   0x54424752

#define XVIMAGE_RGBT16(byte_order)   \
        { \
                FOURCC_RGBT16, \
                XvRGB, \
                byte_order, \
                { 'R', 'G', 'B', 'T', \
                  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
                16, \
                XvPacked, \
                1, \
                16, 0x00007C00, 0x000003E0, 0x0000001F, \
                0, 0, 0, 0, 0, 0, 0, 0, 0, \
                {'A', 'R', 'G', 'B', \
                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
                XvTopToBottom \
        }               

#define FOURCC_RGB16    0x32424752

#define XVIMAGE_RGB16(byte_order)   \
        { \
                FOURCC_RGB16, \
                XvRGB, \
                byte_order, \
                { 'R', 'G', 'B', 0x00, \
                  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
                16, \
                XvPacked, \
                1, \
                16, 0x0000F800, 0x000007E0, 0x0000001F, \
                0, 0, 0, 0, 0, 0, 0, 0, 0, \
                {'R', 'G', 'B', \
                  0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
                XvTopToBottom \
        }               

#ifdef USE_EXA
static void
ATIVideoSave(ScreenPtr pScreen, ExaOffscreenArea *area)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr info = RHDPTR(pScrn);
    RADEONPortPrivPtr pPriv = info->adaptor->pPortPrivates[0].ptr;

    if (pPriv->video_memory == area)
        pPriv->video_memory = NULL;
}
#endif /* USE_EXA */

void RADEONInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr    info = RHDPTR(pScrn);
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr texturedAdaptor = NULL;
    int num_adaptors;


    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
    newAdaptors = xalloc((num_adaptors + 2) * sizeof(XF86VideoAdaptorPtr *));
    if (newAdaptors == NULL)
	return;

    memcpy(newAdaptors, adaptors, num_adaptors * sizeof(XF86VideoAdaptorPtr));
    adaptors = newAdaptors;

#ifdef USE_DRI
    if ((info->ChipSet < RHD_R600)
	&& (info->directRenderingEnabled)) {
	texturedAdaptor = RADEONSetupImageTexturedVideo(pScreen);
	if (texturedAdaptor != NULL) {
	    adaptors[num_adaptors++] = texturedAdaptor;
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Set up textured video\n");
	} else
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to set up textured video\n");
    } else
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Textured video requires CP on R5xx/IGP\n");

    if(num_adaptors)
	xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(newAdaptors)
	xfree(newAdaptors);

}

void
RADEONStopVideo(ScrnInfoPtr pScrn, pointer data, Bool cleanup)
{
  RADEONPortPrivPtr pPriv = (RADEONPortPrivPtr)data;

    if (pPriv->textured)
	return;
}

int
RADEONSetPortAttribute(ScrnInfoPtr  pScrn,
		       Atom	    attribute,
		       INT32	    value,
		       pointer	    data)
{
    RADEONPortPrivPtr	pPriv = (RADEONPortPrivPtr)data;

    if (pPriv->textured)
	return BadMatch;

    return Success;
}

int
RADEONGetPortAttribute(ScrnInfoPtr  pScrn,
		       Atom	    attribute,
		       INT32	    *value,
		       pointer	    data)
{
    RADEONPortPrivPtr	pPriv = (RADEONPortPrivPtr)data;

    if (pPriv->textured)
	return BadMatch;

    return Success;
}

void
RADEONQueryBestSize(
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

void
RADEONCopyData(
  ScrnInfoPtr pScrn,
  unsigned char *src,
  unsigned char *dst,
  unsigned int srcPitch,
  unsigned int dstPitch,
  unsigned int h,
  unsigned int w,
  unsigned int bpp
){
    RHDPtr info = RHDPTR(pScrn);

    /* Get the byte-swapping right for big endian systems */
    if ( bpp == 2 ) {
	w *= 2;
	bpp = 1;
    }

#ifdef USE_DRI

    if ( info->directRenderingEnabled && info->DMAForXv )
    {
	uint8_t *buf;
	uint32_t bufPitch, dstPitchOff;
	int x, y;
	unsigned int hpass;

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
#endif /* USE_DRI */
    {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	unsigned int swapper = info->surface_cntl &
		~(RADEON_NONSURF_AP0_SWP_32BPP | RADEON_NONSURF_AP1_SWP_32BPP |
		  RADEON_NONSURF_AP0_SWP_16BPP | RADEON_NONSURF_AP1_SWP_16BPP);

	switch(bpp) {
	case 2:
	    swapper |= RADEON_NONSURF_AP0_SWP_16BPP
		    |  RADEON_NONSURF_AP1_SWP_16BPP;
	    break;
	case 4:
	    swapper |= RADEON_NONSURF_AP0_SWP_32BPP
		    |  RADEON_NONSURF_AP1_SWP_32BPP;
	    break;
	}
	RHDRegWrite(info, RADEON_SURFACE_CNTL, swapper);
#endif
	w *= bpp;

	while (h--) {
	    memcpy(dst, src, w);
	    src += srcPitch;
	    dst += dstPitch;
	}

#if X_BYTE_ORDER == X_BIG_ENDIAN
	/* restore byte swapping */
	RHDRegWrite(info, RADEON_SURFACE_CNTL, info->surface_cntl);
#endif
    }
}

#if 0
static void
RADEONCopyRGB24Data(
  ScrnInfoPtr pScrn,
  unsigned char *src,
  unsigned char *dst,
  unsigned int srcPitch,
  unsigned int dstPitch,
  unsigned int h,
  unsigned int w
){
    uint32_t *dptr;
    uint8_t *sptr;
    unsigned int i, j;
    RHDPtr info = RHDPTR(pScrn);
#ifdef USE_DRI

    if ( info->directRenderingEnabled && info->DMAForXv )
    {
	uint32_t bufPitch, dstPitchOff;
	int x, y;
	unsigned int hpass;

	RADEONHostDataParams( pScrn, dst, dstPitch, 4, &dstPitchOff, &x, &y );

	while ( (dptr = ( uint32_t* )RADEONHostDataBlit( pScrn, 4, w, dstPitchOff,
						       &bufPitch, x, &y, &h,
						       &hpass )) )
	{
	    for( j = 0; j < hpass; j++ )
	    {
		sptr = src;

		for ( i = 0 ; i < w; i++, sptr += 3 )
		{
		    dptr[i] = (sptr[2] << 16) | (sptr[1] << 8) | sptr[0];
		}

		src += srcPitch;
		dptr += bufPitch / 4;
	    }
	}

	FLUSH_RING();

	return;
    }
    else
#endif /* USE_DRI */
    {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	RHDRegWrite(info, RADEON_SURFACE_CNTL, (info->surface_cntl
				   | RADEON_NONSURF_AP0_SWP_32BPP)
				  & ~RADEON_NONSURF_AP0_SWP_16BPP);
#endif

	for (j = 0; j < h; j++) {
	    dptr = (uint32_t *)(dst + j * dstPitch);
	    sptr = src + j * srcPitch;

	    for (i = 0; i < w; i++, sptr += 3) {
		dptr[i] = (sptr[2] << 16) | (sptr[1] << 8) | sptr[0];
	    }
	}

#if X_BYTE_ORDER == X_BIG_ENDIAN
	/* restore byte swapping */
	RHDRegWrite(info, RADEON_SURFACE_CNTL, info->ModeReg->surface_cntl);
#endif
    }
}
#endif

#ifdef USE_DRI
static void RADEON_420_422(
    unsigned int *d,
    unsigned char *s1,
    unsigned char *s2,
    unsigned char *s3,
    unsigned int n
)
{
    while ( n ) {
	*(d++) = s1[0] | (s1[1] << 16) | (s3[0] << 8) | (s2[0] << 24);
	s1+=2; s2++; s3++;
	n--;
    }
}
#endif

void
RADEONCopyMungedData(
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
    RHDPtr info = RHDPTR(pScrn);
#ifdef USE_DRI

    if ( info->directRenderingEnabled && info->DMAForXv )
    {
	uint8_t *buf;
	uint32_t y = 0, bufPitch, dstPitchOff;
	int blitX, blitY;
	unsigned int hpass;

	/* XXX Fix endian flip on R300 */

	RADEONHostDataParams( pScrn, dst1, dstPitch, 4, &dstPitchOff, &blitX, &blitY );

	while ( (buf = RADEONHostDataBlit( pScrn, 4, w/2, dstPitchOff, &bufPitch,
					   blitX, &blitY, &h, &hpass )) )
	{
	    while ( hpass-- )
	    {
		RADEON_420_422( (unsigned int *) buf, src1, src2, src3,
				bufPitch / 4 );
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
#endif /* USE_DRI */
    {
	uint32_t *dst;
	uint8_t *s1, *s2, *s3;
	unsigned int i, j;

#if X_BYTE_ORDER == X_BIG_ENDIAN
	RHDRegWrite(info, RADEON_SURFACE_CNTL, (info->surface_cntl
				   | RADEON_NONSURF_AP0_SWP_32BPP)
				  & ~RADEON_NONSURF_AP0_SWP_16BPP);
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
	RHDRegWrite(info, RADEON_SURFACE_CNTL, info->ModeReg->surface_cntl);
#endif
    }
}


/* Allocates memory, either by resizing the allocation pointed to by mem_struct,
 * or by freeing mem_struct (if non-NULL) and allocating a new space.  The size
 * is measured in bytes, and the offset from the beginning of card space is
 * returned.
 */
uint32_t
RADEONAllocateMemory(
   ScrnInfoPtr pScrn,
   void **mem_struct,
   int size
){
    ScreenPtr pScreen;
    RHDPtr info = RHDPTR(pScrn);
    int offset = 0;

    pScreen = screenInfo.screens[pScrn->scrnIndex];
#ifdef USE_EXA
    if (info->useEXA) {
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
    if (!info->useEXA) {
	FBLinearPtr linear = *mem_struct;
	int cpp = pScrn->bitsPerPixel >> 3;

	/* XAA allocates in units of pixels at the screen bpp, so adjust size
	 * appropriately.
	 */
	size = (size + cpp - 1) / cpp;

	if (linear) {
	    if(linear->size >= size)
		return linear->offset * cpp;

	    if(xf86ResizeOffscreenLinear(linear, size))
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

void
RADEONFreeMemory(
   ScrnInfoPtr pScrn,
   void *mem_struct
){
    RHDPtr info = RHDPTR(pScrn);

#ifdef USE_EXA
    if (info->useEXA) {
	ExaOffscreenArea *area = mem_struct;

	if (area != NULL)
	    exaOffscreenFree(pScrn->pScreen, area);
    }
#endif /* USE_EXA */
#ifdef USE_XAA
    if (!info->useEXA) {
	FBLinearPtr linear = mem_struct;

	if (linear != NULL)
	    xf86FreeOffscreenLinear(linear);
    }
#endif /* USE_XAA */
}

int
RADEONQueryImageAttributes(
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
    case FOURCC_RGBA32:
	size = *w << 2;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    case FOURCC_RGB24:
	size = *w * 3;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    case FOURCC_RGBT16:
    case FOURCC_RGB16:
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


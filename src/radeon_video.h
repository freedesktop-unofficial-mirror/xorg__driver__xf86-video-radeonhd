#ifndef __RADEON_VIDEO_H__
#define __RADEON_VIDEO_H__

#include "xf86xv.h"

/* Xvideo port struct */
typedef struct {
    RegionRec     clip;

    int           size;

    void         *video_memory;
    int           video_offset;

    /* textured video */
    Bool textured;
    DrawablePtr pDraw;
    PixmapPtr pPixmap;

    uint32_t src_offset;
    uint32_t src_pitch;
    uint8_t *src_addr;

    int id;
    int src_w, src_h, dst_w, dst_h;
    int w, h;
    int drw_x, drw_y;
} RADEONPortPrivRec, *RADEONPortPrivPtr;


uint32_t
RADEONAllocateMemory(ScrnInfoPtr pScrn, void **mem_struct, int size);
void
RADEONFreeMemory(ScrnInfoPtr pScrn, void *mem_struct);

int  RADEONSetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
int  RADEONGetPortAttribute(ScrnInfoPtr, Atom ,INT32 *, pointer);
void RADEONStopVideo(ScrnInfoPtr, pointer, Bool);
void RADEONQueryBestSize(ScrnInfoPtr, Bool, short, short, short, short,
			 unsigned int *, unsigned int *, pointer);
int  RADEONQueryImageAttributes(ScrnInfoPtr, int, unsigned short *,
			unsigned short *,  int *, int *);

XF86VideoAdaptorPtr
RADEONSetupImageTexturedVideo(ScreenPtr pScreen);

void
RADEONCopyData(ScrnInfoPtr pScrn,
	       unsigned char *src, unsigned char *dst,
	       unsigned int srcPitch, unsigned int dstPitch,
	       unsigned int h, unsigned int w, unsigned int bpp);

void
RADEONCopyMungedData(ScrnInfoPtr pScrn,
		     unsigned char *src1, unsigned char *src2,
		     unsigned char *src3, unsigned char *dst1,
		     unsigned int srcPitch, unsigned int srcPitch2,
		     unsigned int dstPitch, unsigned int h, unsigned int w);

#endif

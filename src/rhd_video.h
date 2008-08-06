#ifndef __RADEON_VIDEO_H__
#define __RADEON_VIDEO_H__

#include "xf86xv.h"

/* Xvideo port struct */
typedef struct RHDPortPriv {
    RegionRec     clip;

    int           size;

    void         *video_memory;
    int           video_offset;

    /* textured video */
    DrawablePtr pDraw;
    PixmapPtr pPixmap;

    CARD32 src_offset;
    CARD32 src_pitch;
    CARD8 *src_addr;

    int id;
    int src_w, src_h, dst_w, dst_h;
    int w, h;
    int drw_x, drw_y;
} RHDPortPrivRec, *RHDPortPrivPtr;

extern void RHDRADEONDisplayTexturedVideo(ScrnInfoPtr pScrn, struct RHDPortPriv *pPriv);
extern void RHDInitVideo(ScreenPtr pScreen);

#endif

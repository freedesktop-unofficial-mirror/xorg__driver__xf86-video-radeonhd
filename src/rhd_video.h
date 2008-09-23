#ifndef __RADEON_VIDEO_H__
#define __RADEON_VIDEO_H__

/* Xvideo port struct */
struct RHDPortPriv {
    RegionRec clip;

    int size;
    void *video_memory;
    int video_offset;

    /* textured video */
    DrawablePtr pDraw;
    PixmapPtr pPixmap;

    CARD32 src_offset;
    CARD32 src_pitch;

    int id;
    int src_w;
    int src_h;
    int dst_w;
    int dst_h;
    int w;
    int h;
    int drw_x;
    int drw_y;
};

extern void RHDRADEONDisplayTexturedVideo(ScrnInfoPtr pScrn, struct RHDPortPriv *pPriv);
extern void RHDInitVideo(ScreenPtr pScreen);

#endif

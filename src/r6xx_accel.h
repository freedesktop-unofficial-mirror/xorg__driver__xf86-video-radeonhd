
#include "exa.h"

#include "xf86drm.h"

/* r600_exa.c */
Bool R6xxEXAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen);
void R6xxEXACloseScreen(ScreenPtr pScreen);
void R6xxEXADestroy(ScrnInfoPtr pScrn);

void R6xxCacheFlush(struct RhdCS *CS);
void R6xxEngineWaitIdleFull(struct RhdCS *CS);

extern unsigned int
RHDDRIGetIntGARTLocation(ScrnInfoPtr pScrn);

extern PixmapPtr
RADEONGetDrawablePixmap(DrawablePtr pDrawable);

extern Bool RADEONCheckTexturePOT(PicturePtr pPict, Bool canTile);
extern Bool RADEONPitchMatches(PixmapPtr pPix);

struct r6xx_solid_vertex {
    float x;
    float y;
};

struct r6xx_copy_vertex {
    float x;
    float y;
    float s;
    float t;
};

struct r6xx_comp_vertex {
    float x;
    float y;
    float src_s;
    float src_t;
};

struct r6xx_comp_mask_vertex {
    float x;
    float y;
    float src_s;
    float src_t;
    float mask_s;
    float mask_t;
};

struct r6xx_accel_state {
    Bool XHas3DEngineState;

    int               exaSyncMarker;
    int               exaMarkerSynced;

    drmBufPtr         ib;
    int               vb_index;

    // shader storage
    ExaOffscreenArea  *shaders;
    uint32_t          solid_vs_offset;
    uint32_t          solid_ps_offset;
    uint32_t          copy_vs_offset;
    uint32_t          copy_ps_offset;
    uint32_t          comp_vs_offset;
    uint32_t          comp_ps_offset;
    uint32_t          comp_mask_vs_offset;
    uint32_t          comp_mask_ps_offset;
    uint32_t          xv_vs_offset;
    uint32_t          xv_ps_offset;

    // UTS/DFS
    drmBufPtr         scratch;

    // copy
    Bool              same_surface;
    int               rop;
    uint32_t          planemask;

    //comp
    unsigned short texW[2];
    unsigned short texH[2];
    Bool is_transform[2];
    struct _PictTransform *transform[2];
    Bool has_mask;
    /* Whether we are tiling horizontally and vertically */
    Bool need_src_tile_x;
    Bool need_src_tile_y;
    /* Size of tiles ... set to 65536x65536 if not tiling in that direction */
    Bool src_tile_width;
    Bool src_tile_height;
};


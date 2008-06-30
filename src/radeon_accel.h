/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
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

#ifndef _RADEON_ACCEL_H
# define _RADEON_ACCEL_H

struct rhdAccel {
    int               fifo_slots;       /* Free slots in the FIFO (64 max)   */

    /* Computed values for Radeon */
    uint32_t          dp_gui_master_cntl;
    uint32_t          dp_gui_master_cntl_clip;
    uint32_t          trans_color;

    /* Saved values for ScreenToScreenCopy */
    int               xdir;
    int               ydir;

    uint32_t          dst_pitch_offset;

    int               num_gb_pipes;

    unsigned short    texW[2];
    unsigned short    texH[2];

    uint32_t         surface_cntl;

    /* X itself has the 3D context */
    Bool             XHas3DEngineState;

# ifdef USE_XAA
				/* ScanlineScreenToScreenColorExpand support */
    unsigned char     *scratch_buffer[1];
    unsigned char     *scratch_save;
    int               scanline_x;
    int               scanline_y;
    int               scanline_w;
    int               scanline_h;
    int               scanline_h_w;
    int               scanline_words;
    int               scanline_direct;
    int               scanline_bpp;     /* Only used for ImageWrite */
    int               scanline_fg;
    int               scanline_bg;
    int               scanline_hpass;
    int               scanline_x1clip;
    int               scanline_x2clip;
				/* Saved values for DashedTwoPointLine */
    int               dashLen;
    uint32_t          dashPattern;
    int               dash_fg;
    int               dash_bg;
# endif
# if USE_EXA
    int               exaSyncMarker;
    int               exaMarkerSynced;
    int               engineMode;
#  define EXA_ENGINEMODE_UNKNOWN 0
#  define EXA_ENGINEMODE_2D      1
#  define EXA_ENGINEMODE_3D      2
# endif

};

/* radeon_accel.c */
extern void RADEONEngineFlush(ScrnInfoPtr pScrn);
extern void RADEONEngineInit(ScrnInfoPtr pScrn);
extern void RADEONEngineReset(ScrnInfoPtr pScrn);
extern void RADEONEngineRestore(ScrnInfoPtr pScrn);
extern uint8_t *RADEONHostDataBlit(ScrnInfoPtr pScrn, unsigned int cpp,
				   unsigned int w, uint32_t dstPitchOff,
				   uint32_t *bufPitch, int x, int *y,
				   unsigned int *h, unsigned int *hpass);
extern void RADEONHostDataBlitCopyPass(ScrnInfoPtr pScrn,
                                       unsigned int bpp,
                                       uint8_t *dst, uint8_t *src,
                                       unsigned int hpass,
                                       unsigned int dstPitch,
                                       unsigned int srcPitch);
extern void RADEONCopySwap(uint8_t *dst, uint8_t *src, unsigned int size, int swap);
extern void RADEONHostDataParams(ScrnInfoPtr pScrn, uint8_t *dst,
                                 uint32_t pitch, int cpp,
                                 uint32_t *dstPitchOffset, int *x, int *y);
extern void RADEONInit3DEngine(ScrnInfoPtr pScrn);
extern void RADEONWaitForFifoFunction(ScrnInfoPtr pScrn, int entries);
# ifdef USE_DRI
extern drmBufPtr RADEONCPGetBuffer(ScrnInfoPtr pScrn);
extern void RADEONCPFlushIndirect(ScrnInfoPtr pScrn, int discard);
extern void RADEONCPReleaseIndirect(ScrnInfoPtr pScrn);
extern int RADEONCPStop(ScrnInfoPtr pScrn,  RHDPtr info);
# endif

/* radeon_commonfuncs.c */
extern void RADEONWaitForIdleMMIO(ScrnInfoPtr pScrn);
#ifdef USE_DRI
extern void RADEONWaitForIdleCP(ScrnInfoPtr pScrn);
#endif

#endif /*  _RADEON_ACCEL_H */

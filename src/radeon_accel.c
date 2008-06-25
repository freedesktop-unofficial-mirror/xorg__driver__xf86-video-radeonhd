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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *   Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * Credits:
 *
 *   Thanks to Ani Joshi <ajoshi@shell.unixbox.com> for providing source
 *   code to his Radeon driver.  Portions of this file are based on the
 *   initialization code for that driver.
 *
 * References:
 *
 * !!!! FIXME !!!!
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 * Notes on unimplemented XAA optimizations:
 *
 *   SetClipping:   This has been removed as XAA expects 16bit registers
 *                  for full clipping.
 *   TwoPointLine:  The Radeon supports this. Not Bresenham.
 *   DashedLine with non-power-of-two pattern length: Apparently, there is
 *                  no way to set the length of the pattern -- it is always
 *                  assumed to be 8 or 32 (or 1024?).
 *   ScreenToScreenColorExpandFill: See p. 4-17 of the Technical Reference
 *                  Manual where it states that monochrome expansion of frame
 *                  buffer data is not supported.
 *   CPUToScreenColorExpandFill, direct: The implementation here uses a hybrid
 *                  direct/indirect method.  If we had more data registers,
 *                  then we could do better.  If XAA supported a trigger write
 *                  address, the code would be simpler.
 *   Color8x8PatternFill: Apparently, an 8x8 color brush cannot take an 8x8
 *                  pattern from frame buffer memory.
 *   ImageWrites:   Same as CPUToScreenColorExpandFill
 *
 */

#include <errno.h>
#include <string.h>
				/* Driver data structures */
#include "rhd.h"
#include "rhd_dri.h"
#include "rhd_cp.h"

#include "radeon_reg.h"
#ifdef USE_DRI
#define _XF86DRI_SERVER_
#include "radeon_dri.h"
#include "radeon_drm.h"
#include "sarea.h"
#endif

				/* Line support */
#include "miline.h"

				/* X and server generic header files */
#include "xf86.h"

#ifdef USE_XAA
static struct {
    int rop;
    int pattern;
} RADEON_ROP[] = {
    { RADEON_ROP3_ZERO, RADEON_ROP3_ZERO }, /* GXclear        */
    { RADEON_ROP3_DSa,  RADEON_ROP3_DPa  }, /* Gxand          */
    { RADEON_ROP3_SDna, RADEON_ROP3_PDna }, /* GXandReverse   */
    { RADEON_ROP3_S,    RADEON_ROP3_P    }, /* GXcopy         */
    { RADEON_ROP3_DSna, RADEON_ROP3_DPna }, /* GXandInverted  */
    { RADEON_ROP3_D,    RADEON_ROP3_D    }, /* GXnoop         */
    { RADEON_ROP3_DSx,  RADEON_ROP3_DPx  }, /* GXxor          */
    { RADEON_ROP3_DSo,  RADEON_ROP3_DPo  }, /* GXor           */
    { RADEON_ROP3_DSon, RADEON_ROP3_DPon }, /* GXnor          */
    { RADEON_ROP3_DSxn, RADEON_ROP3_PDxn }, /* GXequiv        */
    { RADEON_ROP3_Dn,   RADEON_ROP3_Dn   }, /* GXinvert       */
    { RADEON_ROP3_SDno, RADEON_ROP3_PDno }, /* GXorReverse    */
    { RADEON_ROP3_Sn,   RADEON_ROP3_Pn   }, /* GXcopyInverted */
    { RADEON_ROP3_DSno, RADEON_ROP3_DPno }, /* GXorInverted   */
    { RADEON_ROP3_DSan, RADEON_ROP3_DPan }, /* GXnand         */
    { RADEON_ROP3_ONE,  RADEON_ROP3_ONE  }  /* GXset          */
};
#endif

/* Compute log base 2 of val */
int RADEONMinBits(int val)
{
    int  bits;

    if (!val) return 1;
    for (bits = 0; val; val >>= 1, ++bits);
    return bits;
}

/* The FIFO has 64 slots.  This routines waits until at least `entries'
 * of these slots are empty.
 */
void RADEONWaitForFifoFunction(ScrnInfoPtr pScrn, int entries)
{
    RHDPtr info = RHDPTR(pScrn);
    int            i;

    for (;;) {
	for (i = 0; i < RADEON_TIMEOUT; i++) {
	    info->accel_state->fifo_slots =
		RHDRegRead(info, RADEON_RBBM_STATUS) & RADEON_RBBM_FIFOCNT_MASK;
	    if (info->accel_state->fifo_slots >= entries) return;
	}
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		       "FIFO timed out: %u entries, stat=0x%08x\n",
		       (unsigned int)RHDRegRead(info, RADEON_RBBM_STATUS) & RADEON_RBBM_FIFOCNT_MASK,
		       (unsigned int)RHDRegRead(info, RADEON_RBBM_STATUS));
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "FIFO timed out, resetting engine...\n");
	RADEONEngineReset(pScrn);
	RADEONEngineRestore(pScrn);
#ifdef USE_DRI
	if (info->directRenderingEnabled) {
	    RADEONCP_RESET(pScrn, info);
	    RADEONCP_START(pScrn, info);
	}
#endif
    }
}

/* Flush all dirty data in the Pixel Cache to memory */
void RADEONEngineFlush(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);
    int            i;

    RHDRegMask(pScrn, R300_DSTCACHE_CTLSTAT,
	    R300_RB2D_DC_FLUSH_ALL,
	    ~R300_RB2D_DC_FLUSH_ALL);
    for (i = 0; i < RADEON_TIMEOUT; i++) {
	if (!(RHDRegRead(info, R300_DSTCACHE_CTLSTAT) & R300_RB2D_DC_BUSY))
	    break;
    }
    if (i == RADEON_TIMEOUT) {
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		       "DC flush timeout: %x\n",
		       (unsigned int)RHDRegRead(info, R300_DSTCACHE_CTLSTAT));
    }
}

/* Reset graphics card to known state */
void RADEONEngineReset(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);
    uint32_t       rbbm_soft_reset;
    uint32_t       host_path_cntl;
    uint32_t tmp;

    /* The following RBBM_SOFT_RESET sequence can help un-wedge
     * an R300 after the command processor got stuck.
     */
    rbbm_soft_reset = RHDRegRead(info, RADEON_RBBM_SOFT_RESET);
    RHDRegWrite(info, RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
                                   RADEON_SOFT_RESET_CP |
                                   RADEON_SOFT_RESET_HI |
                                   RADEON_SOFT_RESET_SE |
                                   RADEON_SOFT_RESET_RE |
                                   RADEON_SOFT_RESET_PP |
                                   RADEON_SOFT_RESET_E2 |
                                   RADEON_SOFT_RESET_RB));
    RHDRegRead(info, RADEON_RBBM_SOFT_RESET);
    RHDRegWrite(info, RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset & (uint32_t)
                                   ~(RADEON_SOFT_RESET_CP |
                                     RADEON_SOFT_RESET_HI |
                                     RADEON_SOFT_RESET_SE |
                                     RADEON_SOFT_RESET_RE |
                                     RADEON_SOFT_RESET_PP |
                                     RADEON_SOFT_RESET_E2 |
                                     RADEON_SOFT_RESET_RB)));
    RHDRegRead(info, RADEON_RBBM_SOFT_RESET);
    RHDRegWrite(info, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset);
    RHDRegRead(info, RADEON_RBBM_SOFT_RESET);

    RADEONEngineFlush(pScrn);


    /* Soft resetting HDP thru RBBM_SOFT_RESET register can cause some
     * unexpected behaviour on some machines.  Here we use
     * RADEON_HOST_PATH_CNTL to reset it.
     */
    host_path_cntl = RHDRegRead(info, RADEON_HOST_PATH_CNTL);
    rbbm_soft_reset = RHDRegRead(info, RADEON_RBBM_SOFT_RESET);



    RHDRegWrite(info, RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
						RADEON_SOFT_RESET_CP |
						RADEON_SOFT_RESET_HI |
						RADEON_SOFT_RESET_E2));
    RHDRegRead(info, RADEON_RBBM_SOFT_RESET);
    RHDRegWrite(info, RADEON_RBBM_SOFT_RESET, 0);
    tmp = RHDRegRead(info, RADEON_RB3D_DSTCACHE_MODE);
    RHDRegWrite(info, RADEON_RB3D_DSTCACHE_MODE, tmp | (1 << 17)); /* FIXME */

    RHDRegWrite(info, RADEON_HOST_PATH_CNTL, host_path_cntl | RADEON_HDP_SOFT_RESET);
    RHDRegRead(info, RADEON_HOST_PATH_CNTL);
    RHDRegWrite(info, RADEON_HOST_PATH_CNTL, host_path_cntl);

}

/* Restore the acceleration hardware to its previous state */
void RADEONEngineRestore(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		   "EngineRestore (%d/%d)\n",
		   PIXEL_CODE(pScrn),
		    pScrn->bitsPerPixel);

    /* Setup engine location. This shouldn't be necessary since we
     * set them appropriately before any accel ops, but let's avoid
     * random bogus DMA in case we inadvertently trigger the engine
     * in the wrong place (happened).
     */
    RADEONWaitForFifo(pScrn, 2);
    RHDRegWrite(info, RADEON_DST_PITCH_OFFSET, info->accel_state->dst_pitch_offset);
    RHDRegWrite(info, RADEON_SRC_PITCH_OFFSET, info->accel_state->dst_pitch_offset);

    RADEONWaitForFifo(pScrn, 1);
#if X_BYTE_ORDER == X_BIG_ENDIAN
    RHDRegMask(pScrn, RADEON_DP_DATATYPE,
	    RADEON_HOST_BIG_ENDIAN_EN,
	    ~RADEON_HOST_BIG_ENDIAN_EN);
#else
    RHDRegMask(pScrn, RADEON_DP_DATATYPE, 0, ~RADEON_HOST_BIG_ENDIAN_EN);
#endif

    /* Restore SURFACE_CNTL */
    RHDRegWrite(info, RADEON_SURFACE_CNTL, info->accel_state->surface_cntl);

    RADEONWaitForFifo(pScrn, 1);
    RHDRegWrite(info, RADEON_DEFAULT_SC_BOTTOM_RIGHT, (RADEON_DEFAULT_SC_RIGHT_MAX
					    | RADEON_DEFAULT_SC_BOTTOM_MAX));
    RADEONWaitForFifo(pScrn, 1);
    RHDRegWrite(info, RADEON_DP_GUI_MASTER_CNTL, (info->accel_state->dp_gui_master_cntl
				       | RADEON_GMC_BRUSH_SOLID_COLOR
				       | RADEON_GMC_SRC_DATATYPE_COLOR));

    RADEONWaitForFifo(pScrn, 5);
    RHDRegWrite(info, RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
    RHDRegWrite(info, RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
    RHDRegWrite(info, RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
    RHDRegWrite(info, RADEON_DP_SRC_BKGD_CLR,   0x00000000);
    RHDRegWrite(info, RADEON_DP_WRITE_MASK,     0xffffffff);

    RADEONWaitForIdleMMIO(pScrn);

    info->accel_state->XHas3DEngineState = FALSE;
}

/* Initialize the acceleration hardware */
void RADEONEngineInit(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);
    int pixel_code = PIXEL_CODE(pScrn);
    uint32_t gb_tile_config;
    
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		   "EngineInit (%d/%d)\n",
		   PIXEL_CODE(pScrn),
		   pScrn->bitsPerPixel);

#ifdef USE_DRI
    if (info->directRenderingEnabled) {
	struct drm_radeon_getparam np;
	int num_pipes;

	memset(&np, 0, sizeof(np));
	np.param = RADEON_PARAM_NUM_GB_PIPES;
	np.value = &num_pipes;

	if (drmCommandWriteRead(info->dri->drmFD, DRM_RADEON_GETPARAM, &np,
				sizeof(np)) < 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Failed to determine num pipes from DRM, falling back to "
		       "manual look-up!\n");
	    info->accel_state->num_gb_pipes = 0;
	} else {
	    info->accel_state->num_gb_pipes = num_pipes;
	}
    }
#endif

    if (info->accel_state->num_gb_pipes == 0) {
	uint32_t gb_pipe_sel = RHDRegRead(info, R400_GB_PIPE_SELECT);

	info->accel_state->num_gb_pipes = ((gb_pipe_sel >> 12) & 0x3) + 1;
	if (IS_R500_3D)
	    RHDWritePLL(pScrn, R500_DYN_SCLK_PWMEM_PIPE, (1 | ((gb_pipe_sel >> 8) & 0xf) << 4));
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	       "num pipes is %d\n", info->accel_state->num_gb_pipes);

    gb_tile_config = (R300_ENABLE_TILING | R300_TILE_SIZE_16 | R300_SUBPIXEL_1_16);

    switch(info->accel_state->num_gb_pipes) {
    case 2: gb_tile_config |= R300_PIPE_COUNT_R300; break;
    case 3: gb_tile_config |= R300_PIPE_COUNT_R420_3P; break;
    case 4: gb_tile_config |= R300_PIPE_COUNT_R420; break;
    default:
    case 1: gb_tile_config |= R300_PIPE_COUNT_RV350; break;
    }
    
    RHDRegWrite(info, R300_GB_TILE_CONFIG, gb_tile_config);
    RHDRegWrite(info, RADEON_WAIT_UNTIL, RADEON_WAIT_2D_IDLECLEAN | RADEON_WAIT_3D_IDLECLEAN);
    RHDRegWrite(info, R300_DST_PIPE_CONFIG, RHDRegRead(info, R300_DST_PIPE_CONFIG) | R300_PIPE_AUTO_CONFIG);
    RHDRegWrite(info, R300_RB2D_DSTCACHE_MODE, (RHDRegRead(info, R300_RB2D_DSTCACHE_MODE) |
				     R300_DC_AUTOFLUSH_ENABLE |
				     R300_DC_DC_DISABLE_IGNORE_PE));

    RADEONEngineReset(pScrn);

    switch (pixel_code) {
    case 8:  info->accel_state->datatype = 2; break;
    case 15: info->accel_state->datatype = 3; break;
    case 16: info->accel_state->datatype = 4; break;
    case 24: info->accel_state->datatype = 5; break;
    case 32: info->accel_state->datatype = 6; break;
    default:
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		       "Unknown depth/bpp = %d/%d (code = %d)\n",
		       pScrn->depth, pScrn->bitsPerPixel,
		       pixel_code);
    }
    info->accel_state->pitch = ((pScrn->displayWidth >> 3) *
				((pScrn->bitsPerPixel >> 3) == 3 ? 3 : 1));

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, RADEON_LOGLEVEL_DEBUG,
		   "Pitch for acceleration = %d\n", info->accel_state->pitch);

    info->accel_state->dp_gui_master_cntl =
	((info->accel_state->datatype << RADEON_GMC_DST_DATATYPE_SHIFT)
	 | RADEON_GMC_CLR_CMP_CNTL_DIS
	 | RADEON_GMC_DST_PITCH_OFFSET_CNTL);

#ifdef USE_DRI
    info->accel_state->sc_left         = 0x00000000;
    info->accel_state->sc_right        = RADEON_DEFAULT_SC_RIGHT_MAX;
    info->accel_state->sc_top          = 0x00000000;
    info->accel_state->sc_bottom       = RADEON_DEFAULT_SC_BOTTOM_MAX;

    info->accel_state->re_top_left     = 0x00000000;
    info->accel_state->re_width_height = ((8191 << R300_SCISSOR_X_SHIFT) |
			     (8191 << R300_SCISSOR_Y_SHIFT));
#endif

    RADEONEngineRestore(pScrn);
}


#define ACCEL_MMIO
#define ACCEL_PREAMBLE()
#define BEGIN_ACCEL(n)          RADEONWaitForFifo(pScrn, (n))
#define OUT_ACCEL_REG(reg, val) RHDRegWrite(info, reg, val)
#define FINISH_ACCEL()

#include "radeon_commonfuncs.c"
#include "radeon_accelfuncs.c"

#undef ACCEL_MMIO
#undef ACCEL_PREAMBLE
#undef BEGIN_ACCEL
#undef OUT_ACCEL_REG
#undef FINISH_ACCEL

#ifdef USE_DRI

#define ACCEL_CP
#define ACCEL_PREAMBLE()						\
    RING_LOCALS;							\
    RADEONCP_REFRESH(pScrn, info)
#define BEGIN_ACCEL(n)          BEGIN_RING(2*(n))
#define OUT_ACCEL_REG(reg, val) OUT_RING_REG(reg, val)
#define FINISH_ACCEL()          ADVANCE_RING()


#include "radeon_commonfuncs.c"
#include "radeon_accelfuncs.c"

#undef ACCEL_CP
#undef ACCEL_PREAMBLE
#undef BEGIN_ACCEL
#undef OUT_ACCEL_REG
#undef FINISH_ACCEL

/* Stop the CP */
int RADEONCPStop(ScrnInfoPtr pScrn, RHDPtr info)
{
    struct drm_radeon_cp_stop  stop;
    int              ret, i;

    stop.flush = 1;
    stop.idle  = 1;

    ret = drmCommandWrite(info->dri->drmFD, DRM_RADEON_CP_STOP, &stop,
			  sizeof(struct drm_radeon_cp_stop));

    if (ret == 0) {
	return 0;
    } else if (errno != EBUSY) {
	return -errno;
    }

    stop.flush = 0;

    i = 0;
    do {
	ret = drmCommandWrite(info->dri->drmFD, DRM_RADEON_CP_STOP, &stop,
			      sizeof(struct drm_radeon_cp_stop));
    } while (ret && errno == EBUSY && i++ < RADEON_IDLE_RETRY);

    if (ret == 0) {
	return 0;
    } else if (errno != EBUSY) {
	return -errno;
    }

    stop.idle = 0;

    if (drmCommandWrite(info->dri->drmFD, DRM_RADEON_CP_STOP,
			&stop, sizeof(struct drm_radeon_cp_stop))) {
	return -errno;
    } else {
	return 0;
    }
}

/* Get an indirect buffer for the CP 2D acceleration commands  */
drmBufPtr RADEONCPGetBuffer(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);
    drmDMAReq      dma;
    drmBufPtr      buf = NULL;
    int            indx = 0;
    int            size = 0;
    int            i = 0;
    int            ret;

#if 0
    /* FIXME: pScrn->pScreen has not been initialized when this is first
     * called from RADEONSelectBuffer via RADEONDRICPInit.  We could use
     * the screen index from pScrn, which is initialized, and then get
     * the screen from screenInfo.screens[index], but that is a hack.
     */
    dma.context = DRIGetContext(pScrn->pScreen);
#else
    /* This is the X server's context */
    dma.context = 0x00000001;
#endif

    dma.send_count    = 0;
    dma.send_list     = NULL;
    dma.send_sizes    = NULL;
    dma.flags         = 0;
    dma.request_count = 1;
    dma.request_size  = RADEON_BUFFER_SIZE;
    dma.request_list  = &indx;
    dma.request_sizes = &size;
    dma.granted_count = 0;

    while (1) {
	do {
	    ret = drmDMA(info->dri->drmFD, &dma);
	    if (ret && ret != -EBUSY) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "%s: CP GetBuffer %d\n", __FUNCTION__, ret);
	    }
	} while ((ret == -EBUSY) && (i++ < RADEON_TIMEOUT));

	if (ret == 0) {
	    buf = &info->dri->buffers->list[indx];
	    buf->used = 0;
	    if (RADEON_VERBOSE) {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "   GetBuffer returning %d %p\n",
			   buf->idx, buf->address);
	    }
	    return buf;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "GetBuffer timed out, resetting engine...\n");
	RADEONEngineReset(pScrn);
	RADEONEngineRestore(pScrn);

	/* Always restart the engine when doing CP 2D acceleration */
	RADEONCP_RESET(pScrn, info);
	RADEONCP_START(pScrn, info);
    }
}

/* Flush the indirect buffer to the kernel for submission to the card */
void RADEONCPFlushIndirect(ScrnInfoPtr pScrn, int discard)
{
    RHDPtr info = RHDPTR(pScrn);
    drmBufPtr          buffer = info->cp->indirectBuffer;
    int                start  = info->cp->indirectStart;
    struct drm_radeon_indirect  indirect;

    if (!buffer) return;
    if (start == buffer->used && !discard) return;

    if (RADEON_VERBOSE) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Flushing buffer %d\n",
		   buffer->idx);
    }

    indirect.idx     = buffer->idx;
    indirect.start   = start;
    indirect.end     = buffer->used;
    indirect.discard = discard;

    drmCommandWriteRead(info->dri->drmFD, DRM_RADEON_INDIRECT,
			&indirect, sizeof(struct drm_radeon_indirect));

    if (discard) {
	info->cp->indirectBuffer = RADEONCPGetBuffer(pScrn);
	info->cp->indirectStart  = 0;
    } else {
	/* Start on a double word boundary */
	info->cp->indirectStart  = buffer->used = (buffer->used + 7) & ~7;
	if (RADEON_VERBOSE) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "   Starting at %d\n",
		       info->cp->indirectStart);
	}
    }
}

/* Flush and release the indirect buffer */
void RADEONCPReleaseIndirect(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);
    drmBufPtr          buffer = info->cp->indirectBuffer;
    int                start  = info->cp->indirectStart;
    struct drm_radeon_indirect  indirect;

    info->cp->indirectBuffer = NULL;
    info->cp->indirectStart  = 0;

    if (!buffer) return;

    if (RADEON_VERBOSE) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Releasing buffer %d\n",
		   buffer->idx);
    }

    indirect.idx     = buffer->idx;
    indirect.start   = start;
    indirect.end     = buffer->used;
    indirect.discard = 1;

    drmCommandWriteRead(info->dri->drmFD, DRM_RADEON_INDIRECT,
			&indirect, sizeof(struct drm_radeon_indirect));
}

/** \brief Calculate HostDataBlit parameters from pointer and pitch
 *
 * This is a helper for the trivial HostDataBlit users that don't need to worry
 * about tiling etc.
 */
void
RADEONHostDataParams(ScrnInfoPtr pScrn, uint8_t *dst, uint32_t pitch, int cpp,
		     uint32_t *dstPitchOff, int *x, int *y)
{
    RHDPtr info = RHDPTR(pScrn);
    uint32_t dstOffs = dst - (uint8_t*)info->FbBase + info->FbIntAddress;

    *dstPitchOff = pitch << 16 | (dstOffs & ~RADEON_BUFFER_ALIGN) >> 10;
    *y = ( dstOffs & RADEON_BUFFER_ALIGN ) / pitch;
    *x = ( ( dstOffs & RADEON_BUFFER_ALIGN ) - ( *y * pitch ) ) / cpp;
}

/* Set up a hostdata blit to transfer data from system memory to the
 * framebuffer. Returns the address where the data can be written to and sets
 * the dstPitch and hpass variables as required.
 */
uint8_t*
RADEONHostDataBlit(
    ScrnInfoPtr pScrn,
    unsigned int cpp,
    unsigned int w,
    uint32_t dstPitchOff,
    uint32_t *bufPitch,
    int x,
    int *y,
    unsigned int *h,
    unsigned int *hpass
){
    RHDPtr info = RHDPTR(pScrn);
    uint32_t format, dwords;
    uint8_t *ret;
    RING_LOCALS;

    if ( *h == 0 )
    {
	return NULL;
    }

    switch ( cpp )
    {
    case 4:
	format = RADEON_GMC_DST_32BPP;
	*bufPitch = 4 * w;
	break;
    case 2:
	format = RADEON_GMC_DST_16BPP;
	*bufPitch = 2 * ((w + 1) & ~1);
	break;
    case 1:
	format = RADEON_GMC_DST_8BPP_CI;
	*bufPitch = (w + 3) & ~3;
	break;
    default:
	xf86DrvMsg( pScrn->scrnIndex, X_ERROR,
		    "%s: Unsupported cpp %d!\n", __func__, cpp );
	return NULL;
    }

#if X_BYTE_ORDER == X_BIG_ENDIAN
    /* Swap doesn't work on R300 and later, it's handled during the
     * copy to ind. buffer pass
     */
    BEGIN_RING(2);
    if (cpp == 2)
	OUT_RING_REG(RADEON_RBBM_GUICNTL,
		     RADEON_HOST_DATA_SWAP_HDW);
    else if (cpp == 1)
	OUT_RING_REG(RADEON_RBBM_GUICNTL,
		     RADEON_HOST_DATA_SWAP_32BIT);
    else
	OUT_RING_REG(RADEON_RBBM_GUICNTL,
		     RADEON_HOST_DATA_SWAP_NONE);
    ADVANCE_RING();
#endif

    /*RADEON_PURGE_CACHE();
      RADEON_WAIT_UNTIL_IDLE();*/

    *hpass = min( *h, ( ( RADEON_BUFFER_SIZE - 10 * 4 ) / *bufPitch ) );
    dwords = *hpass * *bufPitch / 4;

    BEGIN_RING( (int)dwords + 10 );
    OUT_RING( CP_PACKET3( RADEON_CP_PACKET3_CNTL_HOSTDATA_BLT, dwords + 10 - 2 ) );
    OUT_RING( RADEON_GMC_DST_PITCH_OFFSET_CNTL
	    | RADEON_GMC_DST_CLIPPING
	    | RADEON_GMC_BRUSH_NONE
	    | format
	    | RADEON_GMC_SRC_DATATYPE_COLOR
	    | RADEON_ROP3_S
	    | RADEON_DP_SRC_SOURCE_HOST_DATA
	    | RADEON_GMC_CLR_CMP_CNTL_DIS
	    | RADEON_GMC_WR_MSK_DIS );
    OUT_RING( dstPitchOff );
    OUT_RING( (*y << 16) | x );
    OUT_RING( ((*y + *hpass) << 16) | (x + w) );
    OUT_RING( 0xffffffff );
    OUT_RING( 0xffffffff );
    OUT_RING( *y << 16 | x );
    OUT_RING( *hpass << 16 | (*bufPitch / cpp) );
    OUT_RING( dwords );

    ret = ( uint8_t* )&__head[__count];

    __count += dwords;
    ADVANCE_RING();

    *y += *hpass;
    *h -= *hpass;

    return ret;
}

void RADEONCopySwap(uint8_t *dst, uint8_t *src, unsigned int size, int swap)
{
    switch(swap) {
    case RADEON_HOST_DATA_SWAP_HDW:
        {
	    unsigned int *d = (unsigned int *)dst;
	    unsigned int *s = (unsigned int *)src;
	    unsigned int nwords = size >> 2;

	    for (; nwords > 0; --nwords, ++d, ++s)
		*d = ((*s & 0xffff) << 16) | ((*s >> 16) & 0xffff);
	    return;
        }
    case RADEON_HOST_DATA_SWAP_32BIT:
        {
	    unsigned int *d = (unsigned int *)dst;
	    unsigned int *s = (unsigned int *)src;
	    unsigned int nwords = size >> 2;

	    for (; nwords > 0; --nwords, ++d, ++s)
#ifdef __powerpc__
		asm volatile("stwbrx %0,0,%1" : : "r" (*s), "r" (d));
#else
		*d = ((*s >> 24) & 0xff) | ((*s >> 8) & 0xff00)
			| ((*s & 0xff00) << 8) | ((*s & 0xff) << 24);
#endif
	    return;
        }
    case RADEON_HOST_DATA_SWAP_16BIT:
        {
	    unsigned short *d = (unsigned short *)dst;
	    unsigned short *s = (unsigned short *)src;
	    unsigned int nwords = size >> 1;

	    for (; nwords > 0; --nwords, ++d, ++s)
#ifdef __powerpc__
		asm volatile("stwbrx %0,0,%1" : : "r" (*s), "r" (d));
#else
	        *d = ((*s >> 24) & 0xff) | ((*s >> 8) & 0xff00)
			| ((*s & 0xff00) << 8) | ((*s & 0xff) << 24);
#endif
	    return;
	}
    }
    if (src != dst)
	    memmove(dst, src, size);
}

/* Copies a single pass worth of data for a hostdata blit set up by
 * RADEONHostDataBlit().
 */
void
RADEONHostDataBlitCopyPass(
    ScrnInfoPtr pScrn,
    unsigned int cpp,
    uint8_t *dst,
    uint8_t *src,
    unsigned int hpass,
    unsigned int dstPitch,
    unsigned int srcPitch
){

#if X_BYTE_ORDER == X_BIG_ENDIAN
    RHDPtr info = RHDPTR(pScrn);
#endif

    /* RADEONHostDataBlitCopy can return NULL ! */
    if( (dst==NULL) || (src==NULL)) return;

    if ( dstPitch == srcPitch )
    {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	switch(cpp) {
	case 1:
	    RADEONCopySwap(dst, src, hpass * dstPitch,
			   RADEON_HOST_DATA_SWAP_32BIT);
	    return;
	case 2:
	    RADEONCopySwap(dst, src, hpass * dstPitch,
			   RADEON_HOST_DATA_SWAP_HDW);
	    return;
	}
#endif
	memcpy( dst, src, hpass * dstPitch );
    }
    else
    {
	unsigned int minPitch = min( dstPitch, srcPitch );
	while ( hpass-- )
	{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    switch(cpp) {
	    case 1:
		RADEONCopySwap(dst, src, minPitch,
			       RADEON_HOST_DATA_SWAP_32BIT);
		goto next;
	    case 2:
		RADEONCopySwap(dst, src, minPitch,
			       RADEON_HOST_DATA_SWAP_HDW);
		goto next;
	    }
#endif
	    memcpy( dst, src, minPitch );
#if X_BYTE_ORDER == X_BIG_ENDIAN
	next:
#endif
	    src += srcPitch;
	    dst += dstPitch;
	}
    }
}

#endif

Bool RADEONAccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    RHDPtr info = RHDPTR(pScrn);

    if (info->ChipSet >= RHD_R600)
	return FALSE;

#ifdef USE_EXA
    if (info->useEXA) {
# ifdef USE_DRI
	if (info->directRenderingEnabled) {
	    if (!RADEONDrawInitCP(pScreen))
		return FALSE;
	} else
# endif /* USE_DRI */
	{
	    if (!RADEONDrawInitMMIO(pScreen))
		return FALSE;
	}
    }
#endif /* USE_EXA */
#ifdef USE_XAA
    if (!info->useEXA) {
	XAAInfoRecPtr  a;

	if (!(a = info->accel = XAACreateInfoRec())) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "XAACreateInfoRec Error\n");
	    return FALSE;
	}

#ifdef USE_DRI
	if (info->directRenderingEnabled)
	    RADEONAccelInitCP(pScreen, a);
	else
#endif /* USE_DRI */
	    RADEONAccelInitMMIO(pScreen, a);

	RADEONEngineInit(pScrn);

	if (!XAAInit(pScreen, a)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "XAAInit Error\n");
	    return FALSE;
	}
    }
#endif /* USE_XAA */
    return TRUE;
}

void RADEONInit3DEngine(ScrnInfoPtr pScrn)
{
    RHDPtr info = RHDPTR(pScrn);

#ifdef USE_DRI
    if (info->directRenderingEnabled) {
	drm_radeon_sarea_t *pSAREAPriv;

	pSAREAPriv = DRIGetSAREAPrivate(pScrn->pScreen);
	pSAREAPriv->ctx_owner = DRIGetContext(pScrn->pScreen);
	RADEONInit3DEngineCP(pScrn);
    } else
#endif
	RADEONInit3DEngineMMIO(pScrn);

    info->accel_state->XHas3DEngineState = TRUE;
}

#ifdef USE_XAA
Bool
RADEONSetupMemXAA(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    RHDPtr info = RHDPTR(pScrn);
    BoxRec         MemBox;
    int            y2;

    int width_bytes = pScrn->displayWidth * (pScrn->bitsPerPixel >> 3);

    MemBox.x1 = 0;
    MemBox.y1 = 0;
    MemBox.x2 = pScrn->displayWidth;
#if USE_DRI
    if (info->directRenderingEnabled)
	y2 = pScrn->displayWidth * pScrn->virtualY * 3;
    else
#endif
	y2 = (info->FbScanoutSize + info->FbOffscreenSize) / width_bytes;

    if (y2 >= 32768)
	y2 = 32767; /* because MemBox.y2 is signed short */
    MemBox.y2 = y2;
    
    /* The acceleration engine uses 14 bit
     * signed coordinates, so we can't have any
     * drawable caches beyond this region.
     */
    if (MemBox.y2 > 8191)
	MemBox.y2 = 8191;

    if (!xf86InitFBManager(pScreen, &MemBox)) {
	xf86DrvMsg(scrnIndex, X_ERROR,
		   "Memory manager initialization to "
		   "(%d,%d) (%d,%d) failed\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
	return FALSE;
    } else {
	int       width, height;
	FBAreaPtr fbarea;

	xf86DrvMsg(scrnIndex, X_INFO,
		   "Memory manager initialized to (%d,%d) (%d,%d)\n",
		   MemBox.x1, MemBox.y1, MemBox.x2, MemBox.y2);
	if ((fbarea = xf86AllocateOffscreenArea(pScreen,
						pScrn->displayWidth,
						info->allowColorTiling ? 
						((pScrn->virtualY + 15) & ~15)
						- pScrn->virtualY + 2 : 2,
						0, NULL, NULL,
						NULL))) {
	    xf86DrvMsg(scrnIndex, X_INFO,
		       "Reserved area from (%d,%d) to (%d,%d)\n",
		       fbarea->box.x1, fbarea->box.y1,
		       fbarea->box.x2, fbarea->box.y2);
	} else {
	    xf86DrvMsg(scrnIndex, X_ERROR, "Unable to reserve area\n");
	}
	if (xf86QueryLargestOffscreenArea(pScreen, &width, &height,
					      0, 0, 0)) {
	    xf86DrvMsg(scrnIndex, X_INFO,
		       "Largest offscreen area available: %d x %d\n",
		       width, height);
	}
	return TRUE;
    }    
}
#endif /* USE_XAA */

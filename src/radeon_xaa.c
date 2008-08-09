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
 */

#ifdef USE_XAA

#include <errno.h>
#include <string.h>
				/* Driver data structures */
#ifdef USE_XAA
#include "xaa.h"
#endif
#ifdef USE_EXA
#include "exa.h"
#endif
#ifdef USE_DRI
#define _XF86DRI_SERVER_
#include "dri.h"
#include "GL/glxint.h"
#endif

#include "rhd.h"
#ifdef USE_DRI
# include "rhd_dri.h"
# include "rhd_cp.h"
#else
# define uint8_t CARD8
# define uint16_t CARD16
# define uint32_t CARD32
#endif

#include "radeon_accel.h"

#include "radeon_reg.h"
#ifdef USE_DRI
# define _XF86DRI_SERVER_
# include "radeon_dri.h"
# include "radeon_drm.h"
# include "sarea.h"
#endif

#include "miline.h"

                               /* X and server generic header files */
#include "xf86.h"

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

#define ACCEL_MMIO
#define ACCEL_PREAMBLE()
#define BEGIN_ACCEL(n)          RADEONWaitForFifo(pScrn, (n))
#define OUT_ACCEL_REG(reg, val) RHDRegWrite(info, reg, val)
#define FINISH_ACCEL()

#include "radeon_accelfuncs.c"

#undef ACCEL_MMIO
#undef ACCEL_PREAMBLE
#undef BEGIN_ACCEL
#undef OUT_ACCEL_REG
#undef FINISH_ACCEL

#ifdef USE_DRI

# define ACCEL_CP
# define ACCEL_PREAMBLE()						\
    RING_LOCALS;							\
    RADEONCP_REFRESH(pScrn, info)
# define BEGIN_ACCEL(n)          BEGIN_RING(2*(n))
# define OUT_ACCEL_REG(reg, val) OUT_RING_REG(reg, val)
# define FINISH_ACCEL()          ADVANCE_RING()

# include "radeon_accelfuncs.c"

# undef ACCEL_CP
# undef ACCEL_PREAMBLE
# undef BEGIN_ACCEL
# undef OUT_ACCEL_REG
# undef FINISH_ACCEL
#endif /* USE_DRI */

Bool
RADEON_XAAInit(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    RHDPtr info = RHDPTR(pScrn);

    if (!(info->xaa = XAACreateInfoRec())) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "XAACreateInfoRec Error\n");

	return FALSE;
    }

#ifdef USE_DRI
    if (info->directRenderingEnabled)
	RADEONAccelInitCP(pScreen, info->xaa);
    else
#endif
	RADEONAccelInitMMIO(pScreen, info->xaa);

    RADEONEngineInit(pScrn);

    if (!XAAInit(pScreen, info->xaa)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "XAAInit Error\n");
	RADEONCloseXAA(pScreen);

	return FALSE;
    }

    return TRUE;
}

void
RADEONCloseXAA(ScreenPtr pScreen)
{
    ScrnInfoPtr    pScrn = xf86Screens[pScreen->myNum];
    RHDPtr info = RHDPTR(pScrn);

    XAADestroyInfoRec(info->xaa);
    info->xaa = NULL;
    
    if (info->accel_state && info->accel_state->scratch_save)
	xfree(info->accel_state->scratch_save);
    info->accel_state->scratch_save = NULL;
}

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

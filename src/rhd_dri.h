/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario,
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

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *
 */

#ifndef _RHD_DRI_
#define _RHD_DRI_

#include "GL/glxint.h"
#include "xf86drm.h"

/* FIXME: probably to be put somewhere else */
typedef enum {
	CARD_PCI,
	CARD_AGP,
	CARD_PCIE
} RADEONCardType;

extern Bool RADEONDRIPreInit(ScrnInfoPtr pScrn);
extern Bool RADEONDRIAllocateBuffers(ScrnInfoPtr pScrn);
extern Bool RADEONDRIScreenInit(ScreenPtr pScreen);

extern Bool RADEONDRICloseScreen(ScreenPtr pScreen);
extern Bool RADEONDRIFinishScreenInit(ScreenPtr pScreen);
extern void RADEONDRIEnterVT(ScreenPtr pScreen);
extern void RADEONDRILeaveVT(ScreenPtr pScreen);
extern Bool RADEONDRIScreenInit(ScreenPtr pScreen);

/* to be put into rhd_regs.h? Check for official names */
#define RADEON_CP_CSQ_CNTL                  0x0740
#       define RADEON_CSQ_CNT_PRIMARY_MASK     (0xff << 0)
#       define RADEON_CSQ_PRIDIS_INDDIS        (0    << 28)
#       define RADEON_CSQ_PRIPIO_INDDIS        (1    << 28)
#       define RADEON_CSQ_PRIBM_INDDIS         (2    << 28)
#       define RADEON_CSQ_PRIPIO_INDBM         (3    << 28)
#       define RADEON_CSQ_PRIBM_INDBM          (4    << 28)
#       define RADEON_CSQ_PRIPIO_INDPIO        (15   << 28)

#endif

/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Cursor handling.
 *
 * Only supports ARGB cursors.
 * Bitmap cursors are converted to ARGB internally.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* All drivers should typically include these */
#include "xf86.h"

#include "xf86Cursor.h"
#include "cursorstr.h"
#include "servermd.h"

/* Driver specific headers */
#include "rhd.h"
#include "rhd_cursor.h"
#include "rhd_crtc.h"
#include "rhd_regs.h"

/* System headers */
#include <assert.h>


#define REG_CUR_OFFSET(x) ((x)*0x200)

#define OFFSET_CURSOR(i) ((i) ? rhdPtr->D2CursorOffset : rhdPtr->D1CursorOffset)

/* Internal interface to RealizeCursor - we need width/height */
struct rhd_Cursor_Bits {
    int width, height;
    /* Cursor source bitmap follows */
    /* Cursor mask bitmap follows */
} ;



/*
 * Bit-banging ONLY
 */

/* RadeonHD registers are double buffered, exchange only during vertical blank.
 * By locking registers, a set of registers is updated atomically.
 * Probably not necessary for cursors, but trivial and fast. */
static void
lockCursor(RHDPtr rhdPtr, int regoffset)
{
    /* Double Buffering: Set _UPDATE_LOCK bit */
    RHDRegMask(rhdPtr, D1CUR_UPDATE + regoffset, 0x00010000, 0x00010000);
}

static void
unlockCursor(RHDPtr rhdPtr, int regoffset)
{
    /* Double Buffering: Clear _UPDATE_LOCK bit */
    RHDRegMask(rhdPtr, D1CUR_UPDATE + regoffset, 0x00000000, 0x00010000);
}

/* RadeonHD has hardware support for hotspots, but doesn't allow negative
 * cursor coordinates. Emulated in rhdSetCursorPosition. */
static void
setCursorPos(RHDPtr rhdPtr, int regoffset, unsigned int x, unsigned int y,
	     unsigned int hotx, unsigned int hoty)
{
    /* R600 only has 13 bits, but well... */
    assert (x >= 0 && x < 0x10000);
    assert (y >= 0 && y < 0x10000);
    RHDRegWrite(rhdPtr, D1CUR_POSITION + regoffset, x << 16 | y);
    /* Note: unknown whether hotspot may be ouside width/height */
    assert (hotx >= 0 && hotx < MAX_CURSOR_WIDTH);
    assert (hoty >= 0 && hoty < MAX_CURSOR_HEIGHT);
    RHDRegWrite(rhdPtr, D1CUR_HOT_SPOT + regoffset, hotx << 16 | hoty);
}

static void
enableCursor(RHDPtr rhdPtr, int regoffset)
{
    /* pre-multiplied ARGB, Enable */
    RHDRegWrite(rhdPtr, D1CUR_CONTROL + regoffset, 0x00000201);
}

static void
disableCursor(RHDPtr rhdPtr, int regoffset)
{
    RHDRegWrite(rhdPtr, D1CUR_CONTROL + regoffset, 0);
}

/* Activate already uploaded cursor image. */
static void
setCursorImage(RHDPtr rhdPtr, int regoffset, int offset, int width, int height)
{
#ifndef NDEBUG
    /* Every cursor address needs to be 4k aligned - non-fatal though */
    if (offset & 0xFFF)
	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "%s: Offset (0x%08X) is not "
		   "4K aligned\n", __func__, offset);
#endif
    RHDRegWrite(rhdPtr, D1CUR_SURFACE_ADDRESS + regoffset,
		rhdPtr->FbIntAddress + offset);
    assert (width  > 0 && width  <= MAX_CURSOR_WIDTH);
    assert (height > 0 && height <= MAX_CURSOR_HEIGHT);
    RHDRegWrite(rhdPtr, D1CUR_SIZE + regoffset,
		(width-1) << 16 | (height-1));
}

/* Upload image.
 * Hardware only supports 64-wide cursor images.
 * img: (MAX_CURSOR_WIDTH * height) ARGB tuples */
static void
uploadCursorImage(RHDPtr rhdPtr, int offset, int width, int height, CARD32 *img)
{
#ifndef NDEBUG
    /* Every cursor address needs to be 4k aligned - non-fatal though */
    if (offset & 0xFFF)
	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "%s: Offset (0x%08X) is not "
		   "4K aligned\n", __func__, offset);
#endif
    memcpy(((char *) rhdPtr->FbBase + offset), img, MAX_CURSOR_WIDTH*height*4);
}


/*
 * Helper functions
 */

/* TRUE if any portion of cursor is visible on Crtc.
 * x/y: top left cursor image pos (-hotspot) */
static int
cursorVisible(RHDPtr rhdPtr, struct rhd_Crtc *Crtc, int x, int y)
{
    return (Crtc->Active                        &&
	    x >= Crtc->X - rhdPtr->CursorWidth  &&
	    x <  Crtc->X + Crtc->Width          &&
	    y >= Crtc->Y - rhdPtr->CursorHeight &&
	    y <  Crtc->Y + Crtc->Height);
}

/* Convert bitmaps as defined in rhd_CursorBits to ARGB tupels */
static void
convertBitsToARGB(struct rhd_Cursor_Bits *bits, CARD32 *dest,
		  CARD32 color0, CARD32 color1)
{
    CARD8 *src      = (CARD8 *) &bits[1];
    int    srcPitch = BitmapBytePad(bits->width);
    CARD8 *mask     = src + srcPitch * bits->height;
    int x, y;

    for (y = 0; y < bits->height; y++) {
	CARD8  *s = src, *m = mask;
	CARD32 *d = dest;
	for (x = 0; x < bits->width; x++) {
	    if (m[x/8] & (1<<(x&7))) {
		if (s[x/8] & (1<<(x&7)))
		    *d++ = color1;
		else
		    *d++ = color0;
	    } else
		*d++ = 0;
	}
	src  += srcPitch;
	mask += srcPitch;
	dest += MAX_CURSOR_WIDTH;
    }
}


/*
 * Interface
 */

void
rhdShowCursor(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int i;

    rhdPtr->HWCursorShown = TRUE;

    for (i = 0; i < 2; i++) {
	struct rhd_Crtc *Crtc = rhdPtr->Crtc[i];
	if (cursorVisible (rhdPtr, Crtc, rhdPtr->CursorX, rhdPtr->CursorY)) {
	    lockCursor  (rhdPtr, REG_CUR_OFFSET(i));
	    enableCursor(rhdPtr, REG_CUR_OFFSET(i));
	    unlockCursor(rhdPtr, REG_CUR_OFFSET(i));
	}
    }
}

void
rhdHideCursor(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int i;

    rhdPtr->HWCursorShown = FALSE;

    for (i = 0; i < 2; i++) {
	lockCursor   (rhdPtr, REG_CUR_OFFSET(i));
	disableCursor(rhdPtr, REG_CUR_OFFSET(i));
	unlockCursor (rhdPtr, REG_CUR_OFFSET(i));
    }
}

static void
rhdSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int i;

    /* Given cursor pos is always relative to frame - make absolute here */
    rhdPtr->CursorX = x + rhdPtr->FrameX;
    rhdPtr->CursorY = y + rhdPtr->FrameY;

    for (i = 0; i < 2; i++) {
	struct rhd_Crtc *Crtc = rhdPtr->Crtc[i];
	lockCursor  (rhdPtr, REG_CUR_OFFSET(i));
	if (cursorVisible(rhdPtr, Crtc, rhdPtr->CursorX, rhdPtr->CursorY)) {
	    int cx   = rhdPtr->CursorX - Crtc->X;
	    int cy   = rhdPtr->CursorY - Crtc->Y;
	    enableCursor(rhdPtr, REG_CUR_OFFSET(i));
	    /* Hardware doesn't allow negative cursor pos. Use hardware
	     * hotspot support for that. Cannot exceed width, but cursor is
	     * not visible in this case. */
	    setCursorPos(rhdPtr, REG_CUR_OFFSET(i),
			 cx >= 0 ? cx : 0,   cy >= 0 ? cy : 0,
			 cx >= 0 ? 0  : -cx, cy >= 0 ? 0  : -cy);
	} else
	    disableCursor(rhdPtr, REG_CUR_OFFSET(i));
	unlockCursor(rhdPtr, REG_CUR_OFFSET(i));
    }
}

static void
rhdSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int i;

    rhdPtr->CursorColor0 = bg | 0xff000000;
    rhdPtr->CursorColor1 = fg | 0xff000000;

    if (rhdPtr->CursorBits) {
	/* Re-convert cursor bits if color changed */
	convertBitsToARGB(rhdPtr->CursorBits,   rhdPtr->CursorImage,
			  rhdPtr->CursorColor0, rhdPtr->CursorColor1);

	for (i = 0; i < 2; i++) {
	    lockCursor       (rhdPtr, REG_CUR_OFFSET(i));
	    uploadCursorImage(rhdPtr, OFFSET_CURSOR(i),
			      rhdPtr->CursorWidth, rhdPtr->CursorHeight,
			      rhdPtr->CursorImage);
	    setCursorImage   (rhdPtr, REG_CUR_OFFSET(i), OFFSET_CURSOR(i),
			      rhdPtr->CursorWidth, rhdPtr->CursorHeight);
	    unlockCursor     (rhdPtr, REG_CUR_OFFSET(i));
	}
    }
}


static void
rhdLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhd_Cursor_Bits *bits = (struct rhd_Cursor_Bits *) src;
    int i;

    rhdPtr->CursorBits   = bits;
    rhdPtr->CursorWidth  = bits->width;
    rhdPtr->CursorHeight = bits->height;
    convertBitsToARGB(bits, rhdPtr->CursorImage,
		      rhdPtr->CursorColor0, rhdPtr->CursorColor1);

    for (i = 0; i < 2; i++) {
	lockCursor       (rhdPtr, REG_CUR_OFFSET(i));
	uploadCursorImage(rhdPtr, OFFSET_CURSOR(i),
			  rhdPtr->CursorWidth, rhdPtr->CursorHeight,
			  rhdPtr->CursorImage);
	setCursorImage   (rhdPtr, REG_CUR_OFFSET(i), OFFSET_CURSOR(i),
			  rhdPtr->CursorWidth, rhdPtr->CursorHeight);
	unlockCursor     (rhdPtr, REG_CUR_OFFSET(i));
    }
}

static Bool
rhdUseHWCursorARGB(ScreenPtr pScreen, CursorPtr cur)
{
    /* Inconsistency in interface: UseHWCursor == NULL is trivial accept,
     * UseHWCursorARGB == NULL is trivial reject. */
    return TRUE;
}

static void
rhdLoadCursorARGB(ScrnInfoPtr pScrn, CursorPtr cur)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int i;

    rhdPtr->CursorBits   = NULL;
    rhdPtr->CursorWidth  = cur->bits->width;
    rhdPtr->CursorHeight = cur->bits->height;

    /* Hardware only supports 64-wide cursor images. */
    for (i = 0; i < cur->bits->height; i++)
	memcpy(rhdPtr->CursorImage + MAX_CURSOR_WIDTH*i,
	       cur->bits->argb + cur->bits->width*i,
	       cur->bits->width*4);

    for (i = 0; i < 2; i++) {
	lockCursor       (rhdPtr, REG_CUR_OFFSET(i));
	uploadCursorImage(rhdPtr, OFFSET_CURSOR(i),
			  rhdPtr->CursorWidth, rhdPtr->CursorHeight,
			  rhdPtr->CursorImage);
	setCursorImage   (rhdPtr, REG_CUR_OFFSET(i), OFFSET_CURSOR(i),
			  rhdPtr->CursorWidth, rhdPtr->CursorHeight);
	unlockCursor     (rhdPtr, REG_CUR_OFFSET(i));
    }
}

/* Save cursor parameters for later re-use */
static unsigned char*
rhdRealizeCursor(xf86CursorInfoPtr infoPtr, CursorPtr cur)
{
    int    len = BitmapBytePad(cur->bits->width) * cur->bits->height;
    struct rhd_Cursor_Bits *bits = xalloc(sizeof(struct rhd_Cursor_Bits)
					  + 2*len);
    char  *bitmap = (char *) &bits[1];

    bits->width  = cur->bits->width;
    bits->height = cur->bits->height;
    memcpy (bitmap,     cur->bits->source, len);
    memcpy (bitmap+len, cur->bits->mask,   len);

    return (unsigned char *) bits;
}

/*
 * Init
 */

Bool
RHDCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if (!infoPtr)
	return FALSE;

    infoPtr->MaxWidth  = MAX_CURSOR_WIDTH;
    infoPtr->MaxHeight = MAX_CURSOR_HEIGHT;
    infoPtr->Flags     = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
			 HARDWARE_CURSOR_UPDATE_UNHIDDEN |
			 HARDWARE_CURSOR_AND_SOURCE_WITH_MASK
#if defined (ARGB_CURSOR) && defined (HARDWARE_CURSOR_ARGB)
			 | HARDWARE_CURSOR_ARGB
#endif
			 ;

    infoPtr->SetCursorColors   = rhdSetCursorColors;
    infoPtr->SetCursorPosition = rhdSetCursorPosition;
    infoPtr->LoadCursorImage   = rhdLoadCursorImage;
    infoPtr->HideCursor        = rhdHideCursor;
    infoPtr->ShowCursor        = rhdShowCursor;
    infoPtr->UseHWCursor       = NULL;
#ifdef ARGB_CURSOR
    infoPtr->UseHWCursorARGB   = rhdUseHWCursorARGB; /* may not be NULL */
    infoPtr->LoadCursorARGB    = rhdLoadCursorARGB;
#endif
    infoPtr->RealizeCursor     = rhdRealizeCursor;

    if (!xf86InitCursor(pScreen, infoPtr)) {
        xf86DestroyCursorInfoRec(infoPtr);
        return FALSE;
    }
    rhdPtr->CursorInfo   = infoPtr;
    rhdPtr->CursorImage  = xalloc(MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT * 4);

    return TRUE;
}


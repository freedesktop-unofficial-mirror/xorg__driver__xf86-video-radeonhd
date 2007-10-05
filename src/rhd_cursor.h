/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
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

#ifndef _RHD_CURSOR_H
# define _RHD_CURSOR_H

#define MAX_CURSOR_WIDTH  64
#define MAX_CURSOR_HEIGHT 64

/*
 *
 */
struct rhdCursor
{
    int scrnIndex;

    int RegOffset;

    int Width;
    int Height;
    int Base;

    int X;
    int Y;

    void (*Lock) (struct rhdCursor *Cursor, Bool Lock);
    void (*Enable) (struct rhdCursor *Cursor, Bool Enable);
    void (*Position) (struct rhdCursor *Cursor,
		      CARD32 x, CARD32 y, CARD32 hotx, CARD32 hoty);
    void (*Set) (struct rhdCursor *Cursor);
    void (*Load) (struct rhdCursor *Cursor, CARD32 *img);
    void (*Save) (struct rhdCursor *Cursor);
    void (*Restore) (struct rhdCursor *Cursor);

    Bool Stored;

    CARD32  StoreControl;
    CARD32  StoreOffset;
    CARD32  StoreSize;
    CARD32  StorePosition;
    CARD32  StoreHotSpot;
    CARD8  *StoreImage;
    int     StoreImageSize;
};

void RHDCursorsInit(RHDPtr rhdPtr);
void RHDCursorsDestroy(RHDPtr rhdPtr);
Bool RHDxf86InitCursor(ScreenPtr pScreen);
void rhdShowCursor(ScrnInfoPtr);
void rhdHideCursor(ScrnInfoPtr);
void rhdReloadCursor(ScrnInfoPtr pScrn);
void rhdSaveCursor(ScrnInfoPtr pScrn);

#endif

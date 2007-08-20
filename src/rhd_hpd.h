/*
 * Copyright 2007  Luc Verhaegen <lverhagen@novell.com>
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

#ifndef _RHD_HPD_H
#define _RHD_HPD_H

/*
 *
 */
struct rhd_HPD {
    Bool Stored;
    CARD32 StoreMask;
    CARD32 StoreEnable;

#define HPD_1_CONNECTED 0x00000001
#define HPD_1_CHANGED   0x00000002
#define HPD_2_CONNECTED 0x00000100
#define HPD_2_CHANGED   0x00000200
#define HPD_3_CONNECTED 0x00010000
#define HPD_3_CHANGED   0x00020000
    CARD32 Status;
};

void RHDHPDSave(RHDPtr rhdPtr);
void RHDHPDRestore(RHDPtr rhdPtr);
void RHDHPDSet(RHDPtr rhdPtr);
Bool RHDHPDConnected(RHDPtr rhdPtr, CARD32 Mask);
CARD32 RHDHPDTimerCheck(RHDPtr rhdPtr);
void RHDHPDInit(RHDPtr rhdPtr);
void RHDHPDDestroy(RHDPtr rhdPtr);

#endif /* _RHD_HPD_H */

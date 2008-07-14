/*
 * Copyright 2008  Christian König <deathsimple@vodafone.de>
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

#ifndef _RHD_AUDIO_H
#define _RHD_AUDIO_H

struct rhdAudio {

	int scrnIndex;

	Bool Stored;
	Bool StoreEnabled;

	CARD32 StoreUnknown0;
	CARD32 StoreUnknown1;

	CARD32 StorePll1Mul;
	CARD32 StorePll1Div;
	CARD32 StorePll2Mul;
	CARD32 StorePll2Div;
	CARD32 StoreClockSrcSel;
};

void RHDAudioInit(RHDPtr rhdPtr);
void RHDAudioSetClock(RHDPtr rhdPtr, struct rhdPLL *pll);
void RHDAudioSetEnable(RHDPtr rhdPtr, Bool Enable);
void RHDAudioSave(RHDPtr rhdPtr);
void RHDAudioRestore(RHDPtr rhdPtr);
void RHDAudioDestroy(RHDPtr rhdPtr);

#endif /* _RHD_AUDIO_H */

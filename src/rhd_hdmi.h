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

#ifndef _RHD_HDMI_H
#define _RHD_HDMI_H

struct rhdHdmi {

	int scrnIndex;

	CARD16 Offset;

	Bool Stored;
	CARD32 StoreEnable;
	CARD32 StoreControl;
	CARD32 StoreUnknown[0xe];
	CARD32 StoreVideoInfoFrame[0x4];
	CARD32 StoreAudioInfoFrame[0x2];
};

struct rhdHdmi* RHDHdmiInit(RHDPtr rhdPtr, CARD16 Offset);
void RHDHdmiEnable(struct rhdHdmi *rhdHdmi, Bool Enable);
void RHDHdmiSave(struct rhdHdmi* rhdHdmi);
void RHDHdmiRestore(struct rhdHdmi* rhdHdmi);
void RHDHdmiDestroy(struct rhdHdmi* rhdHdmi);

#endif /* _RHD_HDMI_H */

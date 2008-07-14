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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "rhd.h"
#include "rhd_hdmi.h"
#include "rhd_regs.h"

enum HdmiColorFormat {
    RGB = 0,
    /* YCC 4:2:2 or 4:4:4 ? */
    YCC = 2
};

/*
 *
 */
static void
HdmiInfoFrameChecksum(CARD8 packetType, CARD8 versionNumber, CARD8 length, CARD8* frame)
{
    int i;
    frame[0] = packetType + versionNumber + length;
    for(i=1;i<=length;i++)
	frame[0] += frame[i];
    frame[0] = 0x100 - frame[0];
}

/*
 *
 */
static void
HdmiVideoInfoFrame(
    struct rhdHdmi *hdmi,
    enum HdmiColorFormat ColorFormat,
    Bool ActiveInformationPresent,
    CARD8 ActiveFormatAspectRatio,
    CARD8 ScanInformation,
    CARD8 Colorimetry,
    CARD8 ExColorimetry,
    CARD8 Quantization,
    Bool ITC,
    CARD8 PictureAspectRatio,
    CARD8 VideoFormatIdentification,
    CARD8 PixelRepetition,
    CARD8 NonUniformPictureScaling,
    CARD8 BarInfoDataValid,
    CARD16 TopBar,
    CARD16 BottomBar,
    CARD16 LeftBar,
    CARD16 RightBar
)
{
    CARD8 frame[14];

    frame[0x0] = 0;
    frame[0x1] =
	(ScanInformation & 0x3) |
	((BarInfoDataValid & 0x3) << 2) |
	((ActiveInformationPresent & 0x1) << 4) |
	((ColorFormat & 0x3) << 5);
    frame[0x2] =
	(ActiveFormatAspectRatio & 0xF) |
	((PictureAspectRatio & 0x3) << 4) |
	((Colorimetry & 0x3) << 6);
    frame[0x3] =
	(NonUniformPictureScaling & 0x3) |
	((Quantization & 0x3) << 2) |
	((ExColorimetry & 0x7) << 4) |
	((ITC & 0x1) << 7);
    frame[0x4] = (VideoFormatIdentification & 0x7F);
    frame[0x5] = (PixelRepetition & 0xF);
    frame[0x6] = (TopBar & 0xFF);
    frame[0x7] = (TopBar >> 8);
    frame[0x8] = (BottomBar & 0xFF);
    frame[0x9] = (BottomBar >> 8);
    frame[0xA] = (LeftBar & 0xFF);
    frame[0xB] = (LeftBar >> 8);
    frame[0xC] = (RightBar & 0xFF);
    frame[0xD] = (RightBar >> 8);

    HdmiInfoFrameChecksum(0x82, 0x02, 0x0D, frame);

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_0,
	frame[0x0] | (frame[0x1] << 8) | (frame[0x2] << 16) | (frame[0x3] << 24));
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_1,
	frame[0x4] | (frame[0x5] << 8) | (frame[0x6] << 16) | (frame[0x7] << 24));
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_2,
	frame[0x8] | (frame[0x9] << 8) | (frame[0xA] << 16) | (frame[0xB] << 24));
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_3,
	frame[0xC] | (frame[0xD] << 8));
}

/*
 *
 */
static void
HdmiAudioInfoFrame(
    struct rhdHdmi *hdmi,
    CARD8 ChannelCount,
    CARD8 CodingType,
    CARD8 SampleSize,
    CARD8 SampleFrequency,
    CARD8 Format,
    CARD8 ChannelAllocation,
    CARD8 LevelShift,
    Bool DownmixInhibit
)
{
    CARD8 frame[11];

    frame[0x0] = 0;
    frame[0x1] = (ChannelCount & 0x7) | ((CodingType & 0xF) << 4);
    frame[0x2] = (SampleSize & 0x3) | ((SampleFrequency & 0x7) << 2);
    frame[0x3] = Format;
    frame[0x4] = ChannelAllocation;
    frame[0x5] = ((LevelShift & 0xF) << 3) | ((DownmixInhibit & 0x1) << 7);
    frame[0x6] = 0;
    frame[0x7] = 0;
    frame[0x8] = 0;
    frame[0x9] = 0;
    frame[0xA] = 0;

    HdmiInfoFrameChecksum(0x84, 0x01, 0x0A, frame);

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_0, 
	frame[0x0] | (frame[0x1] << 8) | (frame[0x2] << 16) | (frame[0x3] << 24));
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_1,
	frame[0x4] | (frame[0x5] << 8) | (frame[0x6] << 16) | (frame[0x8] << 24));
}

/*
 *
 */
struct rhdHdmi*
RHDHdmiInit(RHDPtr rhdPtr, CARD16 Offset)
{
    struct rhdHdmi *hdmi;
    RHDFUNC(rhdPtr);

    if(rhdPtr->ChipSet >= RHD_R600) {
	hdmi = (struct rhdHdmi *) xnfcalloc(sizeof(struct rhdHdmi), 1);
	hdmi->scrnIndex = rhdPtr->scrnIndex;
	hdmi->Offset = Offset;
	hdmi->Stored = FALSE;
	return hdmi;
    } else
	return NULL;
}

/*
 *
 */
void
RHDHdmiEnable(struct rhdHdmi *hdmi, Bool Enable)
{
    if(!hdmi) return;
    RHDFUNC(hdmi);

    if(!Enable)
	RHDRegWrite(hdmi, hdmi->Offset+HDMI_ENABLE, 0x0);
    else switch(hdmi->Offset) {
	case HDMI_TMDS: RHDRegWrite(hdmi, hdmi->Offset+HDMI_ENABLE, 0x101); break;
	case HDMI_LVTMA: RHDRegWrite(hdmi, hdmi->Offset+HDMI_ENABLE, 0x105); break;
	case HDMI_DIG: RHDRegWrite(hdmi, hdmi->Offset+HDMI_ENABLE, 0x110); break;
    }

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_CNTL, 0x4020011);

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_0, 0x1000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_1, 0x31);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_2, 0x93);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_3, 0x202);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_4, 0x0);

    HdmiVideoInfoFrame(hdmi, RGB, FALSE, 0, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_5, 0x1220a000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_6, 0x1000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_7, 0x14244000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_8, 0x1880);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_9, 0x1220a000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_A, 0x1800);

    HdmiAudioInfoFrame(hdmi, 1, 0, 0, 0, 0, 0, 0, FALSE);

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_B, 0x100000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_C, 0x200000);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_D, 0x1000);
    
}

/*
 *
 */
void
RHDHdmiSave(struct rhdHdmi *hdmi)
{
    if(!hdmi) return;
    RHDFUNC(hdmi);

    hdmi->StoreEnable = RHDRegRead(hdmi, hdmi->Offset+HDMI_ENABLE);
    hdmi->StoreControl = RHDRegRead(hdmi, hdmi->Offset+HDMI_CNTL);
    hdmi->StoreUnknown[0x0] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_0);
    hdmi->StoreUnknown[0x1] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_1);
    hdmi->StoreUnknown[0x2] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_2);
    hdmi->StoreUnknown[0x3] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_3);
    hdmi->StoreUnknown[0x4] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_4);
    hdmi->StoreVideoInfoFrame[0x0] = RHDRegRead(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_0);
    hdmi->StoreVideoInfoFrame[0x1] = RHDRegRead(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_1);
    hdmi->StoreVideoInfoFrame[0x2] = RHDRegRead(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_2);
    hdmi->StoreVideoInfoFrame[0x3] = RHDRegRead(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_3);
    hdmi->StoreUnknown[0x5] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_5);
    hdmi->StoreUnknown[0x6] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_6);
    hdmi->StoreUnknown[0x7] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_7);
    hdmi->StoreUnknown[0x8] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_8);
    hdmi->StoreUnknown[0x9] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_9);
    hdmi->StoreUnknown[0xa] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_A);
    hdmi->StoreAudioInfoFrame[0x0] = RHDRegRead(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_0);
    hdmi->StoreAudioInfoFrame[0x1] = RHDRegRead(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_1);
    hdmi->StoreUnknown[0xb] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_B);
    hdmi->StoreUnknown[0xc] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_C);
    hdmi->StoreUnknown[0xd] = RHDRegRead(hdmi, hdmi->Offset+HDMI_UNKNOWN_D);

    hdmi->Stored = TRUE;
}

/*
 *
 */
void
RHDHdmiRestore(struct rhdHdmi *hdmi)
{
    if(!hdmi) return;
    RHDFUNC(hdmi);

    if (!hdmi->Stored) {
        xf86DrvMsg(hdmi->scrnIndex, X_ERROR, "%s: trying to restore "
                   "uninitialized values.\n", __func__);
        return;
    }

    RHDRegWrite(hdmi, hdmi->Offset+HDMI_ENABLE, hdmi->StoreEnable);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_CNTL, hdmi->StoreControl);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_0, hdmi->StoreUnknown[0x0]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_1, hdmi->StoreUnknown[0x1]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_2, hdmi->StoreUnknown[0x2]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_3, hdmi->StoreUnknown[0x3]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_4, hdmi->StoreUnknown[0x4]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_0, hdmi->StoreVideoInfoFrame[0x0]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_1, hdmi->StoreVideoInfoFrame[0x1]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_2, hdmi->StoreVideoInfoFrame[0x2]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_VIDEOINFOFRAME_3, hdmi->StoreVideoInfoFrame[0x3]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_5, hdmi->StoreUnknown[0x5]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_6, hdmi->StoreUnknown[0x6]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_7, hdmi->StoreUnknown[0x7]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_8, hdmi->StoreUnknown[0x8]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_9, hdmi->StoreUnknown[0x9]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_A, hdmi->StoreUnknown[0xa]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_0, hdmi->StoreAudioInfoFrame[0x0]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_AUDIOINFOFRAME_1, hdmi->StoreAudioInfoFrame[0x1]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_B, hdmi->StoreUnknown[0xb]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_C, hdmi->StoreUnknown[0xc]);
    RHDRegWrite(hdmi, hdmi->Offset+HDMI_UNKNOWN_D, hdmi->StoreUnknown[0xd]);
}

/*
 *
 */
void
RHDHdmiDestroy(struct rhdHdmi *hdmi)
{
    if(!hdmi) return;
    RHDFUNC(hdmi);

    xfree(hdmi);
}

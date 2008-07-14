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
#include "rhd_pll.h"
#include "rhd_audio.h"
#include "rhd_regs.h"

/*
 *
 */
void
RHDAudioInit(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet >= RHD_R600) {
	struct rhdAudio *Audio = (struct rhdAudio *) xnfcalloc(sizeof(struct rhdAudio), 1);

	Audio->scrnIndex = rhdPtr->scrnIndex;
	Audio->Stored = FALSE;

	rhdPtr->Audio = Audio;
    } else
	rhdPtr->Audio = NULL;
}

/*
 *
 */
void
RHDAudioSetClock(RHDPtr rhdPtr, struct rhdPLL *pll)
{
    struct rhdAudio *Audio = rhdPtr->Audio;
    CARD32 AudioRate = 48000;
    CARD32 Clock = pll->CurrentClock;

    if (!Audio)	return;

    if (pll->Id == PLL_ID_PLL1) {
	RHDRegWrite(Audio, AUDIO_PLL1_MUL, AudioRate*50);
	RHDRegWrite(Audio, AUDIO_PLL1_DIV, Clock*100);
	RHDRegWrite(Audio, AUDIO_CLK_SRCSEL, 0);

    } else if (pll->Id == PLL_ID_PLL2) {
	RHDRegWrite(Audio, AUDIO_PLL2_MUL, AudioRate*50);
	RHDRegWrite(Audio, AUDIO_PLL2_DIV, Clock*100);
	RHDRegWrite(Audio, AUDIO_CLK_SRCSEL, 1);
    }
}

/*
 *
 */
void
RHDAudioSetEnable(RHDPtr rhdPtr, Bool Enable)
{
    struct rhdAudio *Audio = rhdPtr->Audio;
    if (!Audio)	return;

    RHDFUNC(Audio);

    RHDRegMask(Audio, AUDIO_ENABLE, Enable ? 0x80000000 : 0x0, 0x80000000);
    RHDRegWrite(Audio, AUDIO_UNKNOWN_0, 0x170);
    RHDRegWrite(Audio, AUDIO_UNKNOWN_1, 0x1);
}

/*
 *
 */
void
RHDAudioSave(RHDPtr rhdPtr)
{
    struct rhdAudio *Audio = rhdPtr->Audio;
    if (!Audio)	return;

    RHDFUNC(Audio);

    Audio->StoreEnabled = RHDRegRead(Audio, AUDIO_ENABLE) & 0x80000000 ? TRUE : FALSE;

    Audio->StoreUnknown0 = RHDRegRead(Audio, AUDIO_UNKNOWN_0);
    Audio->StoreUnknown1 = RHDRegRead(Audio, AUDIO_UNKNOWN_1);

    Audio->StorePll1Mul     = RHDRegRead(Audio, AUDIO_PLL1_MUL);
    Audio->StorePll1Div     = RHDRegRead(Audio, AUDIO_PLL1_DIV);
    Audio->StorePll2Mul     = RHDRegRead(Audio, AUDIO_PLL2_MUL);
    Audio->StorePll2Div     = RHDRegRead(Audio, AUDIO_PLL2_DIV);
    Audio->StoreClockSrcSel = RHDRegRead(Audio, AUDIO_CLK_SRCSEL);

    Audio->Stored = TRUE;
}

/*
 *
 */
void
RHDAudioRestore(RHDPtr rhdPtr)
{
    struct rhdAudio *Audio = rhdPtr->Audio;
    if (!Audio)	return;

    RHDFUNC(Audio);

    if (!Audio->Stored) {
        xf86DrvMsg(Audio->scrnIndex, X_ERROR, "%s: trying to restore "
                   "uninitialized values.\n", __func__);
        return;
    }

    RHDAudioSetEnable(rhdPtr, Audio->StoreEnabled);

    RHDRegWrite(Audio, AUDIO_UNKNOWN_0, Audio->StoreUnknown0);
    RHDRegWrite(Audio, AUDIO_UNKNOWN_1, Audio->StoreUnknown1);

    RHDRegWrite(Audio, AUDIO_PLL1_MUL, Audio->StorePll1Mul);
    RHDRegWrite(Audio, AUDIO_PLL1_DIV, Audio->StorePll1Div);
    RHDRegWrite(Audio, AUDIO_PLL2_MUL, Audio->StorePll2Mul);
    RHDRegWrite(Audio, AUDIO_PLL2_DIV, Audio->StorePll2Div);
    RHDRegWrite(Audio, AUDIO_CLK_SRCSEL, Audio->StoreClockSrcSel);
}

/*
 *
 */
void
RHDAudioDestroy(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    if (!rhdPtr->Audio)	return;

    xfree(rhdPtr->Audio);
}

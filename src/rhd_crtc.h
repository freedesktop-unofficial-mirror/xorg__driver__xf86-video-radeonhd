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

#ifndef _RHD_CRTC_H
#define _RHD_CRTC_H

struct rhd_Crtc {
    int scrnIndex;

    char *Name;
#define RHD_CRTC_1 0
#define RHD_CRTC_2 1
    int Id; /* for others to hook onto */

    Bool Active;

    int Offset; /* Current offset */
    int bpp;
    int Pitch;
    int Width;
    int Height;
    int X, Y; /* Current frame */

    struct rhd_PLL *PLL; /* Currently attached PLL */
    int LUT; /* Currently attached LUT */

    DisplayModePtr CurrentMode;
    DisplayModePtr Modes; /* Cycle through these */

    ModeStatus (*FBValid) (struct rhd_Crtc *Crtc, CARD16 Width, CARD16 Height,
			   int bpp, CARD32 Size, CARD32 *pPitch);
    void (*FBSet) (struct rhd_Crtc *Crtc, CARD16 Pitch, CARD16 Width,
		   CARD16 Height, int bpp, int Offset);

    ModeStatus (*ModeValid) (struct rhd_Crtc *Crtc, DisplayModePtr Mode);
    void (*ModeSet) (struct rhd_Crtc *Crtc, DisplayModePtr Mode);

    void (*PLLSelect) (struct rhd_Crtc *Crtc, struct rhd_PLL *PLL);
    void (*LUTSelect) (struct rhd_Crtc *Crtc, int LUT);

    void (*FrameSet) (struct rhd_Crtc *Crtc, CARD16 X, CARD16 Y);

    void (*Power) (struct rhd_Crtc *Crtc, int Power);

    struct rhd_Crtc_Store *Store;
    void (*Save) (struct rhd_Crtc *Crtc);
    void (*Restore) (struct rhd_Crtc *Crtc);

    /* Gamma, scaling */
};

void RHDCRTCInit(RHDPtr rhdPtr);
void RHDCRTCDestroy(RHDPtr rhdPtr);

#endif /* _RHD_CRTC_H */

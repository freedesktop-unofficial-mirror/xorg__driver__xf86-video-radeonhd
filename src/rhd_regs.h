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
#ifndef _RHD_REGS_H
# define _RHD_REGS_H

enum {
/* VGA registers */
    VGA_RENDER_CONTROL             = 0x0300,
    VGA_MEMORY_BASE_ADDRESS        = 0x0310,
    VGA_HDP_CONTROL                = 0x0328,
    D1VGA_CONTROL                  = 0x0330,
    D2VGA_CONTROL                  = 0x0338,

    D1CRTC_H_TOTAL                 = 0x6000,
    D1CRTC_H_BLANK_START_END       = 0x6004,
    D1CRTC_H_SYNC_A                = 0x6008,
    D1CRTC_H_SYNC_A_CNTL           = 0x600C,
    D1CRTC_H_SYNC_B                = 0x6010,
    D1CRTC_H_SYNC_B_CNTL           = 0x6014,

    D1CRTC_V_TOTAL                 = 0x6020,
    D1CRTC_V_BLANK_START_END       = 0x6024,
    D1CRTC_V_SYNC_A                = 0x6028,
    D1CRTC_V_SYNC_A_CNTL           = 0x602C,
    D1CRTC_V_SYNC_B                = 0x6030,
    D1CRTC_V_SYNC_B_CNTL           = 0x6034,

    D1GRPH_X_END                   = 0x6134,
    D1GRPH_Y_END                   = 0x6138,
    D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
    D1MODE_VIEWPORT_SIZE           = 0x6584,

/* CRTC2 registers */
    D2CRTC_CONTROL                 = 0x6880,

/* D2GRPH registers */
    D2GRPH_ENABLE                  = 0x6900,

/* DAC B */
    DACB_SOURCE_SELECT             = 0x7A04
};


/* Some handy functions that makes life so much more readable */
CARD32 RHDRegRead(RHDPtr rhdPtr, CARD16 offset);
void RHDRegWrite(RHDPtr rhdPtr, CARD16 offset, CARD32 value);
void RHDRegMask(RHDPtr rhdPtr, CARD16 offset, CARD32 value, CARD32 mask);

#endif /* _RHD_REGS_H */

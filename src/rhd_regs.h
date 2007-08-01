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
    FB_INTERNAL_ADDRESS            = 0x0134,

    /* VGA registers */
    VGA_RENDER_CONTROL             = 0x0300,
    VGA_MEMORY_BASE_ADDRESS        = 0x0310,
    VGA_HDP_CONTROL                = 0x0328,
    D1VGA_CONTROL                  = 0x0330,
    D2VGA_CONTROL                  = 0x0338,

    EXT1_PPLL_REF_DIV_SRC          = 0x0400,
    EXT1_PPLL_REF_DIV              = 0x0404,
    EXT1_PPLL_UPDATE_LOCK          = 0x0408,
    EXT1_PPLL_UPDATE_CNTL          = 0x040C,
    EXT2_PPLL_REF_DIV_SRC          = 0x0410,
    EXT2_PPLL_REF_DIV              = 0x0414,
    EXT2_PPLL_UPDATE_LOCK          = 0x0418,
    EXT2_PPLL_UPDATE_CNTL          = 0x041C,

    EXT1_PPLL_FB_DIV               = 0x0430,
    EXT2_PPLL_FB_DIV               = 0x0434,
    EXT1_PPLL_POST_DIV_SRC         = 0x0438,
    EXT1_PPLL_POST_DIV             = 0x043C,
    EXT2_PPLL_POST_DIV_SRC         = 0x0440,
    EXT2_PPLL_POST_DIV             = 0x0444,
    P1PLL_CNTL                     = 0x0450,
    P2PLL_CNTL                     = 0x0454,

    PCLK_CRTC1_CNTL                = 0x0480,
    PCLK_CRTC2_CNTL                = 0x0484,

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

    D1CRTC_CONTROL                 = 0x6080,
    D1CRTC_STATUS                  = 0x609C,

    D1GRPH_PITCH                   = 0x6120,
    D1GRPH_X_END                   = 0x6134,
    D1GRPH_Y_END                   = 0x6138,
    D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
    D1MODE_VIEWPORT_SIZE           = 0x6584,

/* CRTC2 registers */
    D2CRTC_CONTROL                 = 0x6880,
    D2CRTC_STATUS                  = 0x609C,

/* D2GRPH registers */
    D2GRPH_ENABLE                  = 0x6900,

    /* DAC B */
    DACA_ENABLE                    = 0x7800,
    DACA_SOURCE_SELECT             = 0x7804,
    DACA_FORCE_OUTPUT_CNTL         = 0x783C,
    DACA_POWERDOWN                 = 0x7850,

    /* DAC B */
    DACB_ENABLE                    = 0x7A00,
    DACB_SOURCE_SELECT             = 0x7A04,
    DACB_FORCE_OUTPUT_CNTL         = 0x7A3C,
    DACB_POWERDOWN                 = 0x7A50,

    /* I2C */
    DC_I2C_CONTROL                 = 0x7D30,
    DC_I2C_DDC1_SETUP              = 0x7D50
};

#endif /* _RHD_REGS_H */

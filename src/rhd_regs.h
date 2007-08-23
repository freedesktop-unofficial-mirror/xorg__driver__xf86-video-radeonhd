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
    R5XX_CONFIG_MEMSIZE            = 0x00F8,

    R5XX_FB_INTERNAL_ADDRESS       = 0x0134,

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

    R6XX_CONFIG_MEMSIZE            = 0x5428,
    R6XX_CONFIG_FB_BASE            = 0x542C, /* AKA CONFIG_F0_BASE */

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

    /* D1GRPH registers */
    D1GRPH_ENABLE                  = 0x6100,
    D1GRPH_CONTROL                 = 0x6104,
    D1GRPH_PITCH                   = 0x6120,
    D1GRPH_X_END                   = 0x6134,
    D1GRPH_Y_END                   = 0x6138,
    D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
    D1MODE_VIEWPORT_SIZE           = 0x6584,

    /* CRTC2 registers */
    D2CRTC_H_TOTAL                 = 0x6800,
    D2CRTC_H_BLANK_START_END       = 0x6804,
    D2CRTC_H_SYNC_A                = 0x6808,
    D2CRTC_H_SYNC_A_CNTL           = 0x680C,
    D2CRTC_H_SYNC_B                = 0x6810,
    D2CRTC_H_SYNC_B_CNTL           = 0x6814,

    D2CRTC_V_TOTAL                 = 0x6820,
    D2CRTC_V_BLANK_START_END       = 0x6824,
    D2CRTC_V_SYNC_A                = 0x6828,
    D2CRTC_V_SYNC_A_CNTL           = 0x682C,
    D2CRTC_V_SYNC_B                = 0x6830,
    D2CRTC_V_SYNC_B_CNTL           = 0x6834,

    D2CRTC_CONTROL                 = 0x6880,
    D2CRTC_STATUS                  = 0x689C,

    /* D2GRPH registers */
    D2GRPH_ENABLE                  = 0x6900,
    D2GRPH_CONTROL                 = 0x6904,
    D2GRPH_PITCH                   = 0x6920,
    D2GRPH_X_END                   = 0x6934,
    D2GRPH_Y_END                   = 0x6938,
    D2GRPH_PRIMARY_SURFACE_ADDRESS = 0x6910,
    D2MODE_VIEWPORT_SIZE           = 0x6D84,


    /* DAC A */
    DACA_ENABLE                    = 0x7800,
    DACA_SOURCE_SELECT             = 0x7804,
    DACA_AUTODETECT_CONTROL        = 0x7828,
    DACA_FORCE_OUTPUT_CNTL         = 0x783C,
    DACA_FORCE_DATA                = 0x7840,
    DACA_POWERDOWN                 = 0x7850,
    DACA_CONTROL1                  = 0x7854,
    DACA_CONTROL2                  = 0x7858,
    DACA_COMPARATOR_ENABLE         = 0x785C,
    DACA_COMPARATOR_OUTPUT         = 0x7860,

    /* TMDSA */
    TMDSA_CNTL                     = 0x7880,
    TMDSA_SOURCE_SELECT            = 0x7884,
    TMDSA_COLOR_FORMAT             = 0x7888,
    TMDSA_FORCE_OUTPUT_CNTL        = 0x788C,
    TMDSA_BIT_DEPTH_CONTROL        = 0x7894,
    TMDSA_DCBALANCER_CONTROL       = 0x78D0,
    TMDSA_DATA_SYNCHRONIZATION_R500 = 0x78D8,
    TMDSA_DATA_SYNCHRONIZATION_R600 = 0x78DC,
    TMDSA_TRANSMITTER_ENABLE       = 0x7904,
    TMDSA_LOAD_DETECT              = 0x7908,
    TMDSA_MACRO_CONTROL            = 0x790C, /* r5x0 and r600: 3 for pll and 1 for TX */
    TMDSA_PLL_ADJUST               = 0x790C, /* rv6x0: pll only */
    TMDSA_TRANSMITTER_CONTROL      = 0x7910,
    TMDSA_TRANSMITTER_ADJUST       = 0x7920, /* rv6x0: TX part of macro control */

    /* DAC B */
    DACB_ENABLE                    = 0x7A00,
    DACB_SOURCE_SELECT             = 0x7A04,
    DACB_AUTODETECT_CONTROL        = 0x7A28,
    DACB_FORCE_OUTPUT_CNTL         = 0x7A3C,
    DACB_FORCE_DATA                = 0x7A40,
    DACB_POWERDOWN                 = 0x7A50,
    DACB_CONTROL1                  = 0x7A54,
    DACB_CONTROL2                  = 0x7A58,
    DACB_COMPARATOR_ENABLE         = 0x7A5C,
    DACB_COMPARATOR_OUTPUT         = 0x7A60,

    /* I2C */
    DC_I2C_CONTROL                 = 0x7D30,
    DC_I2C_DDC1_SETUP              = 0x7D50,

    /* HPD */
    DC_GPIO_HPD_MASK               = 0x7E90,
    DC_GPIO_HPD_A                  = 0x7E94,
    DC_GPIO_HPD_EN                 = 0x7E98,
    DC_GPIO_HPD_Y                  = 0x7E9C
};

#endif /* _RHD_REGS_H */

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

#ifndef _RHD_H
#define _RHD_H

#define RHD_VERSION 0001
#define RHD_NAME "RADEONHD"
#define RHD_DRIVER_NAME "radeonhd"

#define RHD_MAJOR_VERSION 0
#define RHD_MINOR_VERSION 0
#define RHD_PATCHLEVEL    1

enum RHD_CHIPSETS {
    RHD_UNKOWN = 0,
    RHD_RV505,
    RHD_RV515,
    RHD_R520,
    RHD_RV530,
    RHD_RV535,
    RHD_RV570,
    RHD_R580,
    RHD_M52,
    RHD_M54,
    RHD_M56,
    RHD_M58,
    RHD_M71,
    RHD_M72,
    RHD_M76,
    RHD_RS690,
    RHD_R600,
    RHD_RV610,
    RHD_RV630,
    RHD_CHIP_END
};

struct rhd_card {
    CARD16 device;
    CARD16 card_vendor;
    CARD16 card_device;
    char *name;

    /* add whatever quirk handling we need here */
};

/* Just define where which PCI BAR lives for now. Will deal with different
 * locations as soon as cards with a different BAR layout arrives.
 */
#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2

typedef struct RHDRegs {

    /* All to do with VGA handling. */
    Bool IsVGA;
    CARD32 VGAFBOffset;
    CARD8 *VGAFB;
    int VGAFBSize; /* most cases, 256kB */

    CARD32 VGA_Render_Control;
    CARD32 VGA_HDP_Control;
    CARD32 D1VGA_Control;
    CARD32 D2VGA_Control;

    /* CRTC1 */
    CARD32 D1CRTC_Control;

    CARD32 D1CRTC_H_Total;
    CARD32 D1CRTC_H_Blank_Start_End;
    CARD32 D1CRTC_H_Sync_A;
    CARD32 D1CRTC_H_Sync_A_Cntl;
    CARD32 D1CRTC_H_Sync_B;
    CARD32 D1CRTC_H_Sync_B_Cntl;

    CARD32 D1CRTC_V_Total;
    CARD32 D1CRTC_V_Blank_Start_End;
    CARD32 D1CRTC_V_Sync_A;
    CARD32 D1CRTC_V_Sync_A_Cntl;
    CARD32 D1CRTC_V_Sync_B;
    CARD32 D1CRTC_V_Sync_B_Cntl;

    CARD32 D1GRPH_Enable;
    CARD32 D1GRPH_Control;
    CARD32 D1GRPH_X_End;
    CARD32 D1GRPH_Y_End;
    CARD32 D1GRPH_Primary_Surface_Address;
    CARD32 D1GRPH_Pitch;
    CARD32 D1Mode_ViewPort_Size;

    /* PLL1 */
    Bool PLL1Active;
    CARD16 PLL1RefDivider;
    CARD16 PLL1FBDivider;
    CARD8 PLL1FBDividerFraction;
    CARD8 PLL1PostDivider;

    CARD32 PCLK_CRTC1_Control;

    /* CRTC2 */
    CARD32 D2CRTC_Control;

    CARD32 D2CRTC_H_Total;
    CARD32 D2CRTC_H_Blank_Start_End;
    CARD32 D2CRTC_H_Sync_A;
    CARD32 D2CRTC_H_Sync_A_Cntl;
    CARD32 D2CRTC_H_Sync_B;
    CARD32 D2CRTC_H_Sync_B_Cntl;

    CARD32 D2CRTC_V_Total;
    CARD32 D2CRTC_V_Blank_Start_End;
    CARD32 D2CRTC_V_Sync_A;
    CARD32 D2CRTC_V_Sync_A_Cntl;
    CARD32 D2CRTC_V_Sync_B;
    CARD32 D2CRTC_V_Sync_B_Cntl;

    CARD32 D2GRPH_Enable;
    CARD32 D2GRPH_Control;
    CARD32 D2GRPH_X_End;
    CARD32 D2GRPH_Y_End;
    CARD32 D2GRPH_Primary_Surface_Address;
    CARD32 D2GRPH_Pitch;
    CARD32 D2Mode_ViewPort_Size;

    /* PLL2 */
    Bool PLL2Active;
    CARD16 PLL2RefDivider;
    CARD16 PLL2FBDivider;
    CARD8 PLL2FBDividerFraction;
    CARD8 PLL2PostDivider;

    CARD32 PCLK_CRTC2_Control;
} RHDRegs, *RHDRegPtr;


typedef struct _RHDopt {
    Bool set;
    union  {
        Bool bool;
        int integer;
        unsigned long uslong;
        double real;
        double freq;
        char *string;
    } val;
} RHDOpt, *RHDOptPtr;

typedef struct RHDRec {
    int                 scrnIndex;

    int                 ChipSet;
    pciVideoPtr         PciInfo;
    PCITAG              PciTag;
    int			entityIndex;
    struct rhd_card     *Card;

    OptionInfoPtr       Options;
    RHDOpt              noAccel;
    RHDOpt              swCursor;
    RHDOpt              onPciBurst;

    RHDRegs             savedRegs;

    unsigned int        FbMapSize;
    pointer             FbBase;   /* map base of fb   */
    unsigned int        FbIntAddress; /* card internal address of FB */

    unsigned int        MMIOMapSize;
    pointer             MMIOBase; /* map base if mmio */

    struct _xf86CursorInfoRec  *CursorInfo;
    Bool                HWCursorShown;
    CloseScreenProcPtr  CloseScreen;

    struct rhd_Output   *Outputs;
} RHDRec, *RHDPtr;

#define RHDPTR(p) 	((RHDPtr)((p)->driverPrivate))

/* rhd_helper.c */
void RhdGetOptValBool(const OptionInfoRec *table, int token,
                      RHDOptPtr optp, Bool def);
void RhdGetOptValInteger(const OptionInfoRec *table, int token,
                         RHDOptPtr optp, int def);
void RhdGetOptValULong(const OptionInfoRec *table, int token,
                       RHDOptPtr optp, unsigned long def);
void RhdGetOptValReal(const OptionInfoRec *table, int token,
                      RHDOptPtr optp, double def);
void RhdGetOptValFreq(const OptionInfoRec *table, int token,
                      OptFreqUnits expectedUnits, RHDOptPtr optp, double def);
void RhdGetOptValString(const OptionInfoRec *table, int token,
                        RHDOptPtr optp, char *def);

/* rhd_driver.c */
/* Some handy functions that makes life so much more readable */
CARD32 _RHDRegRead(int scrnIndex, CARD16 offset);
#define RHDRegRead(ptr, offset) _RHDRegRead((ptr)->scrnIndex, (offset))
void _RHDRegWrite(int scrnIndex, CARD16 offset, CARD32 value);
#define RHDRegWrite(ptr, offset, value) _RHDRegWrite((ptr)->scrnIndex, (offset), (value))
void _RHDRegMask(int scrnIndex, CARD16 offset, CARD32 value, CARD32 mask);
#define RHDRegMask(ptr, offset, value, mask) _RHDRegMask((ptr)->scrnIndex, (offset), (value), (mask))

/* Extra debugging verbosity: decimates gdb usage */

/* __func__ is really nice, but not universal */
#if !defined(__GNUC__) && !defined(C99)
#define __func__ "unknown"
#endif

#define LOG_DEBUG 7
void RHDDebug(int scrnIndex, const char *format, ...);
#define RHDFUNC(ptr) RHDDebug((ptr)->scrnIndex, "FUNCTION: %s\n", __func__);

#endif /* _RHD_H */

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
# define _RHD_H

# include "xf86Cursor.h"

enum RHD_CHIPSETS {
    RHD_UNKOWN = 0,
    RHD_RV530,
    RHD_RV515,
    RHD_CHIP_END
};

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

    CARD32 D1GRPH_X_End;
    CARD32 D1GRPH_Y_End;
    CARD32 D1GRPH_Primary_Surface_Address;
    CARD32 D1Mode_ViewPort_Size;

    /* D2GRPH */
    CARD32 D2GRPH_Enable;

    /* CRTC2 */
    CARD32 D2CRTC_Control;

    /* DACB  */
    CARD32 DACB_Source_Select;

} RHDRegs, *RHDRegPtr;


typedef struct _RHDopt {
    Bool set;
    union  {
        Bool bool;
        int integer;
        unsigned long ulong;
        double real;
        double freq;
        char *string;
    } val;
} RHDOpt, *RHDOptPtr;

typedef struct RHDRec {
    int                 RhdChipset;
    pciVideoPtr         PciInfo;
    PCITAG              PciTag;
    int			entityIndex;
    OptionInfoPtr       Options;
    RHDOpt              noAccel;
    RHDOpt              swCursor;
    RHDOpt              onPciBurst;

    RHDRegs             savedRegs;

    unsigned int        FbMapSize;
    unsigned int        FbAddress; /* real address of FB */
    pointer             FbBase;   /* map base of fb   */

    unsigned int        MMIOMapSize;
    pointer             MMIOBase; /* map base if mmio */

    xf86CursorInfoPtr   CursorInfo;
    Bool                HWCursorShown;
    CloseScreenProcPtr  CloseScreen;
} RHDRec, *RHDPtr;

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

/* rhd_atombios.c */
pointer RHDInitAtomBIOS(ScrnInfoPtr pScrn);
void RHDUninitAtomBIOS(ScrnInfoPtr pScrn, pointer handle);

/* rhd.h */
/* Some handy functions that makes life so much more readable */
CARD32 RHDRegRead(RHDPtr rhdPtr, CARD16 offset);
void RHDRegWrite(RHDPtr rhdPtr, CARD16 offset, CARD32 value);
void RHDRegMask(RHDPtr rhdPtr, CARD16 offset, CARD32 value, CARD32 mask);

#endif /* _RHD_H */

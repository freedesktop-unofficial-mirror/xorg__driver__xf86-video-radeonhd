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

#ifndef _RHD_PLL_H
#define _RHD_PLL_H

struct rhd_PLL {
    int scrnIndex;

#define PLL_NAME_PLL1 "PLL 1"
#define PLL_NAME_PLL2 "PLL 2"
    char *Name;

/* also used as an index to rhdPtr->PLLs */
#define PLL_ID_PLL1  0
#define PLL_ID_PLL2  1
#define PLL_ID_NONE -1
    int Id;

    CARD32 CurrentClock;
    Bool Active;

    /* from defaults or from atom */
    CARD32 RefClock;
    CARD32 InMin;
    CARD32 InMax;
    CARD32 OutMin;
    CARD32 OutMax;

    ModeStatus (*Valid) (struct rhd_PLL *PLL, CARD32 Clock);
    void (*Set) (struct rhd_PLL *PLL, CARD16 ReferenceDivider,
		 CARD16 FeedbackDivider, CARD8 FeedbackDividerFraction,
		 CARD8 PostDivider);
    void (*Power) (struct rhd_PLL *PLL, int Power);
    void (*Save) (struct rhd_PLL *PLL);
    void (*Restore) (struct rhd_PLL *PLL);

    /* For save/restore */
    Bool Stored;
    Bool Store_Active;
    CARD16 Store_RefDivider;
    CARD16 Store_FBDivider;
    CARD8 Store_FBDividerFraction;
    CARD8 Store_PostDivider;
};

void RHDPLLsInit(RHDPtr rhdPtr);
ModeStatus RHDPLLValid(struct rhd_PLL *PLL, CARD32 Clock);
void RHDPLLSet(struct rhd_PLL *PLL, CARD32 Clock);
void RHDPLLPower(struct rhd_PLL *PLL, int Power);
void RHDPLLsPowerAll(RHDPtr rhdPtr, int Power);
void RHDPLLsShutdownInactive(RHDPtr rhdPtr);
void RHDPLLsSave(RHDPtr rhdPtr);
void RHDPLLsRestore(RHDPtr rhdPtr);
void RHDPLLsDestroy(RHDPtr rhdPtr);

#endif /* _RHD_PLL_H */

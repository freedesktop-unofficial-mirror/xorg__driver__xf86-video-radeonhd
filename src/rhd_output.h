/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
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

#ifndef _RHD_OUTPUT_H
#define _RHD_OUTPUT_H

/* Also needed for connector -> output mapping */
#define RHD_OUTPUT_NONE  0
#define RHD_OUTPUT_DACA  1
#define RHD_OUTPUT_DACB  2
#define RHD_OUTPUT_TMDSA 3
#define RHD_OUTPUT_LVTMA 4

/*
 *
 * This structure should deal with everything output related.
 *
 */
struct rhd_Output {
    struct rhd_Output *Next;

    int scrnIndex;

    char *Name;
    int Id;

    Bool Active;

    struct rhd_Crtc *Crtc;
    struct rhdConnector *Connector;

    Bool (*Sense) (struct rhd_Output *Output, int Type);
    ModeStatus (*ModeValid) (struct rhd_Output *Output, DisplayModePtr Mode);
    void (*Mode) (struct rhd_Output *Output);
    void (*Power) (struct rhd_Output *Output, int Power);
    void (*Save) (struct rhd_Output *Output);
    void (*Restore) (struct rhd_Output *Output);
    void (*Destroy) (struct rhd_Output *Output);

    /* Output Private data */
    void *Private;
};

void RHDOutputAdd(RHDPtr rhdPtr, struct rhd_Output *Output);
void RHDOutputsMode(RHDPtr rhdPtr, struct rhd_Crtc *Crtc);
void RHDOutputsPower(RHDPtr rhdPtr, int Power);
void RHDOutputsShutdownInactive(RHDPtr rhdPtr);
void RHDOutputsSave(RHDPtr rhdPtr);
void RHDOutputsRestore(RHDPtr rhdPtr);
void RHDOutputsDestroy(RHDPtr rhdPtr);

/* output local functions. */
struct rhd_Output *RHDDACAInit(RHDPtr rhdPtr);
struct rhd_Output *RHDDACBInit(RHDPtr rhdPtr);
struct rhd_Output *RHDTMDSAInit(RHDPtr rhdPtr);
struct rhd_Output *RHDLVTMAInit(RHDPtr rhdPtr, CARD8 Type);

#endif /* _RHD_OUTPUT_H */

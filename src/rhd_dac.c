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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "rhd.h"
#include "rhd_output.h"
#include "rhd_regs.h"

#define REG_DACA_OFFSET 0
#define REG_DACB_OFFSET 0x200

struct rhd_DAC_Private {
    Bool Stored;

    CARD32 Store_Powerdown;
    CARD32 Store_Force_Output_Control;
    CARD32 Store_Source_Select;
    CARD32 Store_Enable;
};

/*
 *
 */
static inline void
rhdDACSet(struct rhd_Output *Output, CARD16 offset)
{
    RHDRegWrite(Output, offset + DACA_FORCE_OUTPUT_CNTL, 0);
    RHDRegMask(Output, offset + DACA_SOURCE_SELECT, Output->Crtc, 0x00000001);
}

/*
 *
 */
static void
rhdDACASet(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    rhdDACSet(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
rhdDACBSet(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    rhdDACSet(Output, REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
rhdDACPower(struct rhd_Output *Output, CARD16 offset, int Power)
{
    switch (Power) {
    case RHD_POWER_ON:
	RHDRegWrite(Output, offset + DACA_POWERDOWN, 0);
	RHDRegWrite(Output, offset + DACA_ENABLE, 1);
	return;
    case RHD_POWER_SHUTDOWN:
	RHDRegWrite(Output, offset + DACA_POWERDOWN, 0x01010101);
	RHDRegWrite(Output, offset + DACA_ENABLE, 0);
	return;
    case RHD_POWER_RESET: /* don't bother */
    default:
	return;
    }
}

/*
 *
 */
static void
rhdDACAPower(struct rhd_Output *Output, int Power)
{
    RHDFUNC(Output);

    rhdDACPower(Output, REG_DACA_OFFSET, Power);
}

/*
 *
 */
static void
rhdDACBPower(struct rhd_Output *Output, int Power)
{
    RHDFUNC(Output);

    rhdDACPower(Output, REG_DACB_OFFSET, Power);
}

/*
 *
 */
static inline void
rhdDACSave(struct rhd_Output *Output, CARD16 offset)
{
    struct rhd_DAC_Private *Private = (struct rhd_DAC_Private *) Output->Private;

    Private->Store_Powerdown = RHDRegRead(Output, offset + DACA_POWERDOWN);
    Private->Store_Force_Output_Control = RHDRegRead(Output, offset + DACA_FORCE_OUTPUT_CNTL);
    Private->Store_Source_Select = RHDRegRead(Output, offset + DACA_SOURCE_SELECT);
    Private->Store_Enable = RHDRegRead(Output, offset + DACA_ENABLE);

    Private->Stored = TRUE;
}

/*
 *
 */
static void
rhdDACASave(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    rhdDACSave(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
rhdDACBSave(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    rhdDACSave(Output, REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
rhdDACRestore(struct rhd_Output *Output, CARD16 offset)
{
    struct rhd_DAC_Private *Private = (struct rhd_DAC_Private *) Output->Private;

    RHDRegWrite(Output, offset + DACA_POWERDOWN, Private->Store_Powerdown);
    RHDRegWrite(Output, offset + DACA_FORCE_OUTPUT_CNTL, Private->Store_Force_Output_Control);
    RHDRegWrite(Output, offset + DACA_SOURCE_SELECT, Private->Store_Source_Select);
    RHDRegWrite(Output, offset + DACA_ENABLE, Private->Store_Enable);
}

/*
 *
 */
static void
rhdDACARestore(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    if (!((struct rhd_DAC_Private *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    rhdDACRestore(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
rhdDACBRestore(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    if (!((struct rhd_DAC_Private *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    rhdDACRestore(Output, REG_DACB_OFFSET);
}

/*
 *
 */
static void
rhdDACDestroy(struct rhd_Output *Output)
{
    RHDFUNC(Output);

    if (!Output->Private)
	return;

    xfree(Output->Private);
    Output->Private = NULL;
}

/*
 *
 */
struct rhd_Output *
RHDDACAInit(RHDPtr rhdPtr)
{
    struct rhd_Output *Output;
    struct rhd_DAC_Private *Private;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhd_Output), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->rhdPtr = rhdPtr;
    Output->Name = "DAC A";
    /*
      int Type;
      int Connector;
      int Active;
    */

    Output->Sense = NULL;
    Output->Mode = rhdDACASet;
    Output->Power = rhdDACAPower;
    Output->Save = rhdDACASave;
    Output->Restore = rhdDACARestore;
    Output->Destroy = rhdDACDestroy;

    Private = xnfcalloc(sizeof(struct rhd_DAC_Private), 1);
    Output->Private = Private;

    return Output;
}

/*
 *
 */
struct rhd_Output *
RHDDACBInit(RHDPtr rhdPtr)
{
    struct rhd_Output *Output;
    struct rhd_DAC_Private *Private;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhd_Output), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->rhdPtr = rhdPtr;
    Output->Name = "DAC B";
    /*
      int Type;
      int Connector;
      int Active;
    */

    Output->Sense = NULL;
    Output->Mode = rhdDACBSet;
    Output->Power = rhdDACBPower;
    Output->Save = rhdDACBSave;
    Output->Restore = rhdDACBRestore;
    Output->Destroy = rhdDACDestroy;

    Private = xnfcalloc(sizeof(struct rhd_DAC_Private), 1);
    Output->Private = Private;

    return Output;
}

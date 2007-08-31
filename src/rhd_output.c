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
#include "rhd_crtc.h"

static void
rhdOutputAdd(RHDPtr rhdPtr, struct rhd_Output *New)
{
    struct rhd_Output *Last = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    if (!New)
	return;

    if (Last) {
	while (Last->Next)
	    Last = Last->Next;

	Last->Next = New;
    } else
	rhdPtr->Outputs = New;
}

/*
 *
 */
void
RHDOutputsInit(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    rhdOutputAdd(rhdPtr, RHDDACAInit(rhdPtr));
    rhdOutputAdd(rhdPtr, RHDDACBInit(rhdPtr));
    rhdOutputAdd(rhdPtr, RHDTMDSAInit(rhdPtr));
}

/*
 *
 */
Bool
RHDOutputsSense(RHDPtr rhdPtr)
{
    struct rhd_Output *Output = rhdPtr->Outputs;
    Bool Attached = FALSE;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (Output->Sense) {
	    if (Output->Sense(Output))
		Attached = TRUE;
	} else
	    Attached = TRUE;

	Output = Output->Next;
    }

    return Attached;
}

/*
 *
 */
void
RHDOutputsMode(RHDPtr rhdPtr, int Crtc)
{
    struct rhd_Output *Output = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (Output->Active && Output->Mode && (Output->Crtc == Crtc))
	    Output->Mode(Output);

	Output = Output->Next;
    }
}

/*
 *
 */
void
RHDOutputsPower(RHDPtr rhdPtr, int Power)
{
    struct rhd_Output *Output = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (Output->Active && Output->Power)
	    Output->Power(Output, Power);

	Output = Output->Next;
    }
}

/*
 *
 */
void
RHDOutputsShutdownInactive(RHDPtr rhdPtr)
{
    struct rhd_Output *Output = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (!Output->Active && Output->Power) {
	    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Shutting down %s\n", Output->Name);
	    Output->Power(Output, RHD_POWER_SHUTDOWN);
	}

	Output = Output->Next;
    }
}

/*
 *
 */
void
RHDOutputsSave(RHDPtr rhdPtr)
{
    struct rhd_Output *Output = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (Output->Save)
	    Output->Save(Output);

	Output = Output->Next;
    }
}

/*
 *
 */
void
RHDOutputsRestore(RHDPtr rhdPtr)
{
    struct rhd_Output *Output = rhdPtr->Outputs;

    RHDFUNC(rhdPtr);

    while (Output) {
	if (Output->Restore)
	    Output->Restore(Output);

	Output = Output->Next;
    }
}

/*
 *
 */
void
RHDOutputsDestroy(RHDPtr rhdPtr)
{
    struct rhd_Output *Output = rhdPtr->Outputs, *Next;

    RHDFUNC(rhdPtr);

    while (Output) {
	Next = Output->Next;

	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Destroying %s\n", Output->Name);

	if (Output->Destroy)
	    Output->Destroy(Output);

	xfree(Output);

	Output = Next;
    }
}

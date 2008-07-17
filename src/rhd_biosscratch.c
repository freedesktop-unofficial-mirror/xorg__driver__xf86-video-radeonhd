/*
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
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
# include "config.h"
#endif
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
/* only for testing now */
#include "xf86DDC.h"
#include "edid.h"

#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
#else
# include <unistd.h>
# include <string.h>
# include <stdio.h>
#endif

#include "rhd.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_crtc.h"

#ifdef ATOM_BIOS
# include "rhd_atombios.h"
# include "rhd_atomout.h"
# include "rhd_biosscratch.h"
# include "rhd_atomwrapper.h"
# include "xf86int10.h"
# ifdef ATOM_BIOS_PARSER
#  define INT8 INT8
#  define INT16 INT16
#  define INT32 INT32
#  include "CD_Common_Types.h"
# else
#  ifndef ULONG
typedef unsigned int ULONG;
#   define ULONG ULONG
#  endif
#  ifndef UCHAR
typedef unsigned char UCHAR;
#   define UCHAR UCHAR
#  endif
#  ifndef USHORT
typedef unsigned short USHORT;
#   define USHORT USHORT
#  endif
# endif

# include "atombios.h"

/*
 *
 */
enum rhdSensedOutput
rhdAtomBIOSScratchDACSenseResults(struct rhdOutput *Output, enum atomDAC DAC, enum atomDevice Device)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    CARD32 BIOS_0;
    Bool TV = FALSE;

    RHDFUNC(Output);

    if (rhdPtr->ChipSet < RHD_R600)
	BIOS_0 = RHDRegRead(Output, 0x10);
    else
	BIOS_0 = RHDRegRead(Output, 0x1724);

    switch (Device) {
    	case atomNone:
	case atomCRT2:
    	case atomCRT1:
	case atomLCD1:
	case atomLCD2:
	case atomDFP1:
	case atomDFP2:
	case atomDFP3:
	    TV = FALSE;
	    break;
	case atomTV1:
	case atomTV2:
	case atomCV:
	    TV = TRUE;
	    break;
    }

    RHDDebug(Output->scrnIndex, "BIOSScratch_0: 0x%4.4x\n",BIOS_0);

    switch (DAC) {
	case atomDACA:
	    break;
	case atomDACB:
	    BIOS_0 >>= 8;
	    break;
	case atomDACExt:
	    return RHD_SENSED_NONE;
    }

    if (!TV) {
	if (BIOS_0 & ATOM_S0_CRT1_MASK) {
	    RHDDebug(Output->scrnIndex, "%s sensed RHD_SENSED_VGA\n",__func__);
	    return RHD_SENSED_VGA;
	}
    } else {
	if (BIOS_0 & ATOM_S0_TV1_COMPOSITE_A) {
	    RHDDebug(Output->scrnIndex, "%s: RHD_SENSED_TV_COMPOSITE\n",__func__);
	    return RHD_SENSED_TV_COMPOSITE;
	} else if (BIOS_0 & ATOM_S0_TV1_SVIDEO_A) {
	    RHDDebug(Output->scrnIndex, "%s: RHD_SENSED_TV_SVIDE\n",__func__);
	    return RHD_SENSED_TV_SVIDEO;
	} else if (BIOS_0 & ATOM_S0_CV_MASK_A) {
	    RHDDebug(Output->scrnIndex, "%s: RHD_SENSED_TV_COMPONENT\n",__func__);
	    return RHD_SENSED_TV_COMPONENT;
	}
    }

    RHDDebug(Output->scrnIndex, "%s: RHD_SENSED_NONE\n",__func__);
    return RHD_SENSED_NONE;
}

/*
 *
 */
static void
rhdAtomBIOSScratchUpdateAttachedState(RHDPtr rhdPtr, enum atomDevice dev, Bool attached)
{
    CARD32 BIOS_0;
    CARD32 Addr;
    CARD32 Mask;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	Addr = 0x10;
    else
	Addr = 0x1724;

    BIOS_0 = RHDRegRead(rhdPtr, Addr);

    switch (dev) {
	case atomDFP1:
	    Mask = ATOM_S0_DFP1;
	    break;
	case atomDFP2:
	    Mask = ATOM_S0_DFP2;
	    break;
	case atomLCD1:
	    Mask = ATOM_S0_LCD1;
	    break;
	case atomLCD2:
	    Mask = ATOM_S0_LCD2;
	    break;
	case atomTV2:
	    Mask = ATOM_S0_TV2;
	    break;
	case atomDFP3:
	    Mask = ATOM_S0_DFP3;
	    break;
	default:
	    return;
    }
    if (attached)
	BIOS_0 |= Mask;
    else
	BIOS_0 &= ~Mask;

    RHDRegWrite(rhdPtr, Addr, BIOS_0);
}

/*
 *
 */
static void
rhdAtomBIOSScratchUpdateOnState(RHDPtr rhdPtr, enum atomDevice dev, Bool on)
{
    CARD32 BIOS_3;
    CARD32 Addr;
    CARD32 Mask = 0;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	Addr = 0x1C;
    else
	Addr = 0x1730;

    BIOS_3 = RHDRegRead(rhdPtr, Addr);

    switch (dev) {
	case atomCRT1:
	    Mask = ATOM_S3_CRT1_ACTIVE;
	    break;
	case atomLCD1:
	    Mask = ATOM_S3_LCD1_ACTIVE;
	    break;
	case atomTV1:
	    Mask = ATOM_S3_TV1_ACTIVE;
	    break;
	case atomDFP1:
	    Mask =  ATOM_S3_DFP1_ACTIVE;
	    break;
	case atomCRT2:
	    Mask = ATOM_S3_CRT2_ACTIVE;
	    break;
	case atomLCD2:
	    Mask = ATOM_S3_LCD2_ACTIVE;
	    break;
	case atomTV2:
	    Mask = ATOM_S3_TV2_ACTIVE;
	    break;
	case atomDFP2:
	    Mask = ATOM_S3_DFP2_ACTIVE;
	    break;
	case  atomCV:
	    Mask = ATOM_S3_CV_ACTIVE;
	    break;
	case atomDFP3:
	    Mask = ATOM_S3_DFP3_ACTIVE;
	    break;
	case atomNone:
	    return;
    }
    if (on)
	BIOS_3 |= Mask;
    else
	BIOS_3 &= ~Mask;

    RHDRegWrite(rhdPtr, Addr, BIOS_3);
}

/*
 *
 */
static void
rhdAtomBIOSScratchSetCrtcState(RHDPtr rhdPtr, enum atomDevice dev, enum atomCrtc Crtc)
{
    CARD32 BIOS_3;
    CARD32 Addr;
    CARD32 Mask = 0;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	Addr = 0x1C;
    else
	Addr = 0x1730;

    BIOS_3 = RHDRegRead(rhdPtr, Addr);

    switch (dev) {
	case atomCRT1:
	    Mask = ATOM_S3_CRT1_CRTC_ACTIVE;
	    break;
	case atomLCD1:
	    Mask = ATOM_S3_LCD1_CRTC_ACTIVE;
	    break;
	case atomTV1:
	    Mask = ATOM_S3_TV1_CRTC_ACTIVE;
	    break;
	case atomDFP1:
	    Mask =  ATOM_S3_DFP1_CRTC_ACTIVE;
	    break;
	case atomCRT2:
	    Mask = ATOM_S3_CRT2_CRTC_ACTIVE;
	    break;
	case atomLCD2:
	    Mask = ATOM_S3_LCD2_CRTC_ACTIVE;
	    break;
	case atomTV2:
	    Mask = ATOM_S3_TV2_CRTC_ACTIVE;
	    break;
	case atomDFP2:
	    Mask = ATOM_S3_DFP2_CRTC_ACTIVE;
	    break;
	case  atomCV:
	    Mask = ATOM_S3_CV_CRTC_ACTIVE;
	    break;
	case atomDFP3:
	    Mask = ATOM_S3_DFP3_CRTC_ACTIVE;
	    break;
	case atomNone:
	    return;
    }
    if (Crtc == atomCrtc2)
	BIOS_3 |= Mask;
    else
	BIOS_3 &= ~Mask;

    RHDRegWrite(rhdPtr, Addr, BIOS_3);
}

/*
 *
 */
void
RHDAtomBIOSScratchBlLevel(RHDPtr rhdPtr, enum rhdBIOSScratchBlAction action, int *val)
{
    CARD32 Addr;

    if (rhdPtr->ChipSet < RHD_R600)
	Addr = 0x18;
    else
	Addr = 0x172C;

    switch (action) {
	case rhdBIOSScratchBlGet:
	    *val = (RHDRegRead(rhdPtr, Addr) >> 8) & 0xFF;
	    break;
	case rhdBIOSScratchBlSet:
	    RHDRegMask(rhdPtr, Addr, (*val) << 8, 0xFF00);
	    break;
    }
}

/*
 * This function finds the AtomBIOS device ID of the device that we currently
 * want to drive with a specific output. It contains a logic to deal with CRTC vs. TV
 * on DACs.
 * This function preferrably gets called from within the function that also updates
 * the BIOS scratch registers.
 */
static enum atomDevice
rhdBIOSScratchSetDeviceForOutput(struct rhdOutput *Output)
{
    int i = 0;

    RHDFUNC(Output);

    if (!Output->Connector) {
	RHDDebug(Output->scrnIndex,"%s: No connector assigned to output %s\n",__func__,Output->Name);
	return atomNone;
    }

    if (!Output->OutputDriverPrivate) {
	RHDDebug(Output->scrnIndex,"%s: Output %s has no DriverPrivate\n",__func__,Output->Name);
	return atomNone;
    }

    while (Output->OutputDriverPrivate->OutputDevices[i].DeviceId != atomNone) {
	if (Output->OutputDriverPrivate->OutputDevices[i].ConnectorType == Output->Connector->Type){
	    switch (Output->OutputDriverPrivate->OutputDevices[i].DeviceId) {
		case atomCrtc1:
		case atomCrtc2:
		    if (Output->SensedType == RHD_SENSED_VGA)
			break;
		    i++;
		    continue;
		case atomTV1:
		case atomTV2:
		    if (Output->SensedType == RHD_SENSED_TV_SVIDEO
			|| Output->SensedType == RHD_SENSED_TV_COMPOSITE)
			break;
		    i++;
		    continue;
		case atomCV:
		    if (Output->SensedType == RHD_SENSED_TV_COMPONENT)
			break;
		    i++;
		    continue;
		default:
		    break;
	    }
	    Output->OutputDriverPrivate->Device = Output->OutputDriverPrivate->OutputDevices[i].DeviceId;

	    return Output->OutputDriverPrivate->Device;
	}
	i++;
    }
    return atomNone;
}

/*
 * This function is public as it is used from within other outputs, too.
 */
static enum atomDevice
rhdBIOSScratchUpdateBIOSScratchForOutput(struct rhdOutput *Output)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    struct rhdOutputDevices *devList;
    enum atomDevice Device;
    int i = 0;

    RHDFUNC(Output);

    if (!Output->OutputDriverPrivate) {
	RHDDebug(Output->scrnIndex,"%s: no output driver private present\n",__func__);
	return atomNone;
    }
    devList = Output->OutputDriverPrivate->OutputDevices;

    if (Output->Connector) {
	/* connected - enable */
	Device = rhdBIOSScratchSetDeviceForOutput(Output);

	ASSERT(Device != atomNone);

	if (Output->Crtc)
	    rhdAtomBIOSScratchSetCrtcState(rhdPtr, Device,
					   Output->Crtc->Id == 1 ? atomCrtc2 : atomCrtc1);
	rhdAtomBIOSScratchUpdateOnState(rhdPtr, Device, Output->Active);
	rhdAtomBIOSScratchUpdateAttachedState(rhdPtr, Device, TRUE);

	while (devList[i].DeviceId != atomNone) {
	    if (devList[i].DeviceId != Device)
		rhdAtomBIOSScratchUpdateOnState(rhdPtr, devList[i].DeviceId, FALSE);
	        i++;
	}

    } else {
	/* not connected - just disable everything */
	Device = atomNone;
	Output->OutputDriverPrivate->Device = Device;

	while (devList[i].DeviceId != atomNone) {
	    rhdAtomBIOSScratchUpdateOnState(rhdPtr, devList[i].DeviceId, FALSE);
	    rhdAtomBIOSScratchUpdateAttachedState(rhdPtr, devList[i].DeviceId, FALSE);
	    i++;
	}
    }

    return Device;
}

/*
 *
 */
static void
rhdBIOSScratchPower(struct rhdOutput *Output, int Power)
{
    rhdBIOSScratchUpdateBIOSScratchForOutput(Output);
    Output->OutputDriverPrivate->Power(Output, Power);
}

/*
 *
 */
static void
rhdBIOSScratchMode(struct rhdOutput *Output, DisplayModePtr Mode)
{
    rhdBIOSScratchUpdateBIOSScratchForOutput(Output);
    Output->OutputDriverPrivate->Mode(Output, Mode);
}

/*
 * This destroys the privates again. It is implemented as an output destroy wrapper.
 */
static void
rhdBIOSScratchDestroyOutputDriverPrivate(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (Output->OutputDriverPrivate) {
	void (*Destroy) (struct rhdOutput *Output) = Output->OutputDriverPrivate->Destroy;

	xfree(Output->OutputDriverPrivate->OutputDevices);
	xfree(Output->OutputDriverPrivate);
	Output->OutputDriverPrivate = NULL;
	if (Destroy)
	    Destroy(Output);
    }
}

/*
 * This sets up the AtomBIOS driver output private.
 * It allocates the data structure and sets up the list of devices
 * including the connector they are associated with.
 */
Bool
rhdAtomSetupOutputDriverPrivate(struct rhdAtomOutputDeviceList *Devices, struct rhdOutput *Output)
{
    struct rhdOutputDevices *od = NULL;
    struct BIOSScratchOutputPrivate *OutputDriverPrivate;
    int i = 0, cnt = 0;

    RHDFUNC(Output);

    if (!Devices) {
	RHDDebug(Output->scrnIndex, "%s: Device list doesn't exist.\n");
	return FALSE;
    }

    while (Devices[i].DeviceId != atomNone) {
	if (Devices[i].OutputType == Output->Id) {
	    if (!(od = (struct rhdOutputDevices *)xrealloc(od, sizeof(struct rhdOutputDevices) * (cnt + 1))))
		return FALSE;
	    od[cnt].DeviceId = Devices[i].DeviceId;
	    od[cnt].ConnectorType = Devices[i].ConnectorType;
	    cnt++;
	}
	i++;
    }
    if (!(od = (struct rhdOutputDevices *)xrealloc(od, sizeof(struct rhdOutputDevices) * (cnt + 1))))
	return FALSE;
    od[cnt].DeviceId = atomNone;

    if (!(OutputDriverPrivate = (struct BIOSScratchOutputPrivate *)xalloc(sizeof(struct BIOSScratchOutputPrivate)))) {
	xfree(od);
	return FALSE;
    }
    OutputDriverPrivate->OutputDevices = od;
    OutputDriverPrivate->Destroy = Output->Destroy;
    Output->Destroy = rhdBIOSScratchDestroyOutputDriverPrivate;
    OutputDriverPrivate->Power = Output->Power;
    Output->Power = rhdBIOSScratchPower;
    OutputDriverPrivate->Mode = Output->Mode;
    Output->Mode = rhdBIOSScratchMode;
    Output->OutputDriverPrivate = OutputDriverPrivate;

    return TRUE;
}

/*
 *
 */
enum atomDevice
RHDGetDeviceOnCrtc(RHDPtr rhdPtr, enum atomCrtc Crtc)
{
    CARD32 BIOS_3;
    CARD32 Addr;
    CARD32 Mask = 0;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	Addr = 0x1C;
    else
	Addr = 0x1730;

    if (Crtc == atomCrtc1)
	Mask = ~Mask;

    BIOS_3 = RHDRegRead(rhdPtr, Addr);
    RHDDebug(rhdPtr->scrnIndex, "%s: BIOS_3 = 0x%x\n",__func__,BIOS_3);

    if (BIOS_3 & ATOM_S3_CRT1_ACTIVE
	&& ((BIOS_3 ^ Mask) & ATOM_S3_CRT1_CRTC_ACTIVE))
	return atomCRT1;
    else if (BIOS_3 & ATOM_S3_LCD1_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_LCD1_CRTC_ACTIVE))
	return atomLCD1;
    else if (BIOS_3 & ATOM_S3_DFP1_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_DFP1_CRTC_ACTIVE))
	return atomDFP1;
    else if (BIOS_3 & ATOM_S3_CRT2_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_CRT2_CRTC_ACTIVE))
	return atomCRT2;
    else if (BIOS_3 & ATOM_S3_LCD2_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_LCD2_CRTC_ACTIVE))
	return atomLCD2;
    else if (BIOS_3 & ATOM_S3_TV2_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_TV2_CRTC_ACTIVE))
	return atomTV2;
    else if (BIOS_3 & ATOM_S3_DFP2_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_DFP2_CRTC_ACTIVE))
	return atomDFP2;
    else if (BIOS_3 & ATOM_S3_CV_ACTIVE
	     && ((BIOS_3 ^ Mask) & ATOM_S3_CV_CRTC_ACTIVE))
	return atomCV;
    else
	return atomNone;
}

struct rhdBiosScratchRegisters {
    CARD32 Scratch0;
    CARD32 Scratch2;
    CARD32 Scratch3;
};

struct rhdBiosScratchRegisters *
RHDSaveBiosScratchRegisters(RHDPtr rhdPtr)
{
    struct rhdBiosScratchRegisters *regs;
    CARD32 S0Addr, S2Addr, S3Addr;

    RHDFUNC(rhdPtr);

    if (!(regs = (struct rhdBiosScratchRegisters *)xalloc(sizeof(struct rhdBiosScratchRegisters))))
	return NULL;

    if (rhdPtr->ChipSet < RHD_R600) {
	S0Addr = 0x10;
	S2Addr = 0x18;
	S3Addr = 0x1C;
    } else {
	S0Addr = 0x1724;
	S2Addr = 0x172C;
	S3Addr = 0x1730;
    }
    regs->Scratch0 = RHDRegRead(rhdPtr, S0Addr);
    regs->Scratch2 = RHDRegRead(rhdPtr, S2Addr);
    regs->Scratch3 = RHDRegRead(rhdPtr, S3Addr);

    return regs;
}

void
RHDRestoreBiosScratchRegisters(RHDPtr rhdPtr, struct rhdBiosScratchRegisters *regs)
{
    CARD32 S0Addr, S2Addr, S3Addr;

    RHDFUNC(rhdPtr);

    if (!regs)
	return;

    if (rhdPtr->ChipSet < RHD_R600) {
	S0Addr = 0x10;
	S2Addr = 0x18;
	S3Addr = 0x1C;
    } else {
	S0Addr = 0x1724;
	S2Addr = 0x172C;
	S3Addr = 0x1730;
    }
    RHDRegWrite(rhdPtr, S0Addr, regs->Scratch0);
    RHDRegWrite(rhdPtr, S2Addr, regs->Scratch2);
    RHDRegWrite(rhdPtr, S3Addr, regs->Scratch3);

    xfree(regs);
}

#endif /* ATOM_BIOS */


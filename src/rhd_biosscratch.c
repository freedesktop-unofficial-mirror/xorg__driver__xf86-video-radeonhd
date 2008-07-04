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
#include "rhd_atombios.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_crtc.h"

#ifdef ATOM_BIOS
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
    Bool TV;

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
 * This function is public as it is used from within other outputs, too.
 */
void
RHDAtomUpdateBIOSScratchForOutput(struct rhdOutput *Output)
{
    RHDPtr rhdPtr = RHDPTRI(Output);

    RHDFUNC(Output);

    if (!Output->OutputDriverPrivate)
	return;

    if (Output->Crtc)
	rhdAtomBIOSScratchSetCrtcState(rhdPtr, Output->OutputDriverPrivate->Device,
				       Output->Crtc->Id == 1 ? atomCrtc2 : atomCrtc1);
    rhdAtomBIOSScratchUpdateOnState(rhdPtr, Output->OutputDriverPrivate->Device,
				    Output->Active);
    rhdAtomBIOSScratchUpdateAttachedState(rhdPtr,  Output->OutputDriverPrivate->Device,
					  Output->Connector != NULL);
}

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
    CARD32 Scratch3;
};

struct rhdBiosScratchRegisters *
RHDSaveBiosScratchRegisters(RHDPtr rhdPtr)
{
    struct rhdBiosScratchRegisters *regs;
    CARD32 S0Addr, S3Addr;

    RHDFUNC(rhdPtr);

    if (!(regs = (struct rhdBiosScratchRegisters *)xalloc(sizeof(struct rhdBiosScratchRegisters))))
	return NULL;

    if (rhdPtr->ChipSet < RHD_R600) {
	S0Addr = 0x10;
	S3Addr = 0x1C;
    } else {
	S0Addr = 0x1724;
	S3Addr = 0x1730;
    }
    regs->Scratch0 = RHDRegRead(rhdPtr, S0Addr);
    regs->Scratch3 = RHDRegRead(rhdPtr, S3Addr);

    return regs;
}

void
RHDRestoreBiosScratchRegisters(RHDPtr rhdPtr, struct rhdBiosScratchRegisters *regs)
{
    CARD32 S0Addr, S3Addr;

    RHDFUNC(rhdPtr);

    if (!regs)
	return;

    if (rhdPtr->ChipSet < RHD_R600) {
	S0Addr = 0x10;
	S3Addr = 0x1C;
    } else {
	S0Addr = 0x1724;
	S3Addr = 0x1730;
    }
    RHDRegWrite(rhdPtr, S0Addr, regs->Scratch0);
    RHDRegWrite(rhdPtr, S3Addr, regs->Scratch3);

    xfree(regs);
}

#endif

#if 0
enum atomScratchInfo {
    atomScratchConnected,
    atomScratchRomLocation,
    atomScratchBusDevMask,
    atomScratchScratchBlLevel,
    atomScratchDPMS,
    atomScratchRotation,
    atomScratchTVStandard,
    atomScratchOutputActive,
    atomScratchAssignedCrtc,
    atomScratchAccMode,
    atomScratchBlockDisplaySwitch
};

atomGetBiosScratch(atomBiosHandlePtr handle, enum atomScratchInfo scratchInfo,
		   enum atomDevice dev, enum atomCrtc crtc, enum atomDAC DAC)
{
    switch (id) {
	case atomScratchConnected:
	    break;
	case atomScratchRomLocation:
	    break;
	case atomScratchBusDevMask:
	    break;
	case atomScratchScratchBlLevel:
	    break;
	case atomScratchDPMS:
	    break;
	case atomScratchRotation:
	    break;
	case atomScratchTVStandard:
	    break;
	case atomScratchOutputActive:
	    break;
	case atomScratchAssignedCrtc:
	    break;
	case atomScratchAccMode:
	    break;
	case atomScratchBlockDisplaySwitch:
	    break;
    }
}
#endif

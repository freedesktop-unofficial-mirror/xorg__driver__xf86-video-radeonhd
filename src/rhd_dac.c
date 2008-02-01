/*
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
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
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
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
#include "rhd_regs.h"


#define REG_DACA_OFFSET 0
#define RV620_REG_DACA_OFFSET 0
#define REG_DACB_OFFSET 0x200
#define RV620_REG_DACB_OFFSET 0x100

struct rhdDACPrivate {
    Bool Stored;

    CARD32 Store_Powerdown;
    CARD32 Store_Force_Output_Control;
    CARD32 Store_Force_Data;
    CARD32 Store_Source_Select;
    CARD32 Store_Sync_Select;
    CARD32 Store_Enable;
    CARD32 Store_Control1;
    CARD32 Store_Control2;
    CARD32 Store_Tristate_Control;
    CARD32 Store_Dac_Unknown1;
    CARD32 Store_Dac_Unknown2;
    CARD32 Store_Auto_Calib_Control;
    CARD32 Store_Dac_Bgadj_Src;
};

/* ----------------------------------------------------------- */

/*
 *
 */
static unsigned char
DACSense(struct rhdOutput *Output, CARD32 offset, Bool TV)
{
    CARD32 CompEnable, Control1, Control2, DetectControl, Enable;
    CARD8 ret;

    CompEnable = RHDRegRead(Output, offset + DACA_COMPARATOR_ENABLE);
    Control1 = RHDRegRead(Output, offset + DACA_CONTROL1);
    Control2 = RHDRegRead(Output, offset + DACA_CONTROL2);
    DetectControl = RHDRegRead(Output, offset + DACA_AUTODETECT_CONTROL);
    Enable = RHDRegRead(Output, offset + DACA_ENABLE);

    RHDRegWrite(Output, offset + DACA_ENABLE, 1);
    RHDRegMask(Output, offset + DACA_AUTODETECT_CONTROL, 0, 0x00000003);
    RHDRegMask(Output, offset + DACA_CONTROL2, 0, 0x00000001);
    RHDRegMask(Output, offset + DACA_CONTROL2, 0, 0x00ff0000);

    if (TV)
	RHDRegMask(Output, offset + DACA_CONTROL2, 0x00000100, 0x00000100);
    else
	RHDRegMask(Output, offset + DACA_CONTROL2, 0, 0x00000100);

    RHDRegWrite(Output, offset + DACA_FORCE_DATA, 0);
    RHDRegMask(Output, offset + DACA_CONTROL2, 0x00000001, 0x0000001);

    RHDRegMask(Output, offset + DACA_COMPARATOR_ENABLE, 0x00070000, 0x00070000);
    RHDRegWrite(Output, offset + DACA_CONTROL1, 0x00050802);
    RHDRegMask(Output, offset + DACA_POWERDOWN, 0, 0x00000001); /* Shut down Bandgap Voltage Reference Power */
    usleep(5);

    RHDRegMask(Output, offset + DACA_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RHDRegWrite(Output, offset + DACA_FORCE_DATA, 0x1e6); /* 486 out of 1024 */
    usleep(200);

    RHDRegMask(Output, offset + DACA_POWERDOWN, 0x01010100, 0x01010100); /* Enable RGB */
    usleep(88);

    RHDRegMask(Output, offset + DACA_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RHDRegMask(Output, offset + DACA_COMPARATOR_ENABLE, 0x00000100, 0x00000100);
    usleep(100);

    /* Get RGB detect values
     * If only G is detected, we could have a monochrome monitor,
     * but we don't bother with this at the moment.
     */
    ret = (RHDRegRead(Output, offset + DACA_COMPARATOR_OUTPUT) & 0x0E) >> 1;

    RHDRegMask(Output, offset + DACA_COMPARATOR_ENABLE, CompEnable, 0x00FFFFFF);
    RHDRegWrite(Output, offset + DACA_CONTROL1, Control1);
    RHDRegMask(Output, offset + DACA_CONTROL2, Control2, 0x000001FF);
    RHDRegMask(Output, offset + DACA_AUTODETECT_CONTROL, DetectControl, 0x000000FF);
    RHDRegMask(Output, offset + DACA_ENABLE, Enable, 0x000000FF);

    RHDDebug(Output->scrnIndex, "%s: DAC: 0x0%1X\n", __func__, ret);

    return ret;
}

/*
 *
 */
static enum rhdSensedOutput
DACASense(struct rhdOutput *Output, enum rhdConnectorType Type)
{
    RHDFUNC(Output);

    switch (Type) {
    case RHD_CONNECTOR_DVI:
    case RHD_CONNECTOR_VGA:
	return  (DACSense(Output, REG_DACA_OFFSET, FALSE) == 0x7)
	    ? RHD_SENSED_VGA
	    : RHD_SENSED_NONE;
    case RHD_CONNECTOR_TV:
	switch (DACSense(Output, REG_DACA_OFFSET, TRUE) & 0x7) {
	    case 0x7:
		return RHD_SENSED_TV_COMPONENT;
	    case 0x6:
		return RHD_SENSED_TV_SVIDEO;
	    case 0x1:
		return RHD_SENSED_TV_COMPOSITE;
	    default:
		return RHD_SENSED_NONE;
	}
    default:
	xf86DrvMsg(Output->scrnIndex, X_WARNING,
		   "%s: connector type %d is not supported.\n",
		   __func__, Type);
	return RHD_SENSED_NONE;
    }
}

/*
 *
 */
static enum rhdSensedOutput
DACBSense(struct rhdOutput *Output, enum rhdConnectorType Type)
{
    RHDFUNC(Output);

    switch (Type) {
    case RHD_CONNECTOR_DVI:
    case RHD_CONNECTOR_VGA:
	return  (DACSense(Output, REG_DACB_OFFSET, FALSE) == 0x7)
	    ? RHD_SENSED_VGA
	    : RHD_SENSED_NONE;
    case RHD_CONNECTOR_TV:
	switch (DACSense(Output, REG_DACB_OFFSET, TRUE) & 0x7) {
	    case 0x7:
		return RHD_SENSED_TV_COMPONENT;
	    case 0x6:
		return RHD_SENSED_TV_SVIDEO;
	    case 0x1:
		return RHD_SENSED_TV_COMPOSITE;
	    default:
		return RHD_SENSED_NONE;
	}
    default:
	xf86DrvMsg(Output->scrnIndex, X_WARNING,
		   "%s: connector type %d is not supported.\n",
		   __func__, Type);
	return RHD_SENSED_NONE;
    }
}

/*
 *
 */
static inline void
DACSet(struct rhdOutput *Output, CARD16 offset)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    CARD32 source;
    CARD32 mode;
    CARD32 tv;
    CARD32 powerdown;
    CARD32 white_fine;

    switch (rhdPtr->tvMode) {
	case RHD_TV_NTSC:
	case RHD_TV_NTSCJ:
	    mode = 0x1;
	    break;
	case RHD_TV_PAL:
	case RHD_TV_PALN:
	case RHD_TV_PALCN:
	case RHD_TV_PAL60:
	default:
	    mode = 0x0;
	    break;
    }

    white_fine = (offset > 0) ? 0x2000 : 0x2600;

    switch (Output->SensedType) {
	case RHD_SENSED_TV_SVIDEO:
	    tv = 0x100;
	    source = 0x2; /* tv encoder */
	    powerdown = 0 /* 0x100 */;
	    white_fine = 0x2500;
	    break;
	case RHD_SENSED_TV_COMPOSITE:
	    tv = 0x100;
	    source = 0x2; /* tv encoder */
	    powerdown = 0 /* 0x1010000 */;
	    white_fine = 0x2500;
	    break;
	case RHD_SENSED_TV_COMPONENT:
	    mode = 3; /* HDTV */
	    tv = 0x100; /* tv on?? */
	    source = 0x2; /* tv encoder  ?? */
	    powerdown = 0x0;
	    break;
	case RHD_SENSED_VGA:
	default:
	    mode = 2;
	    tv = 0;
	    source = Output->Crtc->Id;
	    powerdown = 0;
	    break;
    }
    RHDRegMask(Output,  offset + DACB_CONTROL1, white_fine , 0xff00);
    RHDRegMask(Output, offset + DACA_CONTROL1, mode, 0xff); /* no fine control yet */
    RHDRegMask(Output,  offset + DACA_CONTROL2, tv, 0xff00); /* tv enable/disable */
    RHDRegMask(Output,  offset + DACA_SOURCE_SELECT, source, 0x00000003);
    RHDRegMask(Output,  offset + DACA_FORCE_OUTPUT_CNTL, 0x0701, 0x0701);
    RHDRegMask(Output,  offset + DACA_FORCE_DATA, 0, 0x0000ffff);
}

/*
 *
 */
static void
DACASet(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSet(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBSet(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSet(Output, REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
DACPower(struct rhdOutput *Output, CARD16 offset, int Power)
{
    CARD32 powerdown;


    switch (Power) {
    case RHD_POWER_ON:
	switch (Output->SensedType) {
	    case RHD_SENSED_TV_SVIDEO:
		powerdown = 0 /* 0x100 */;
		break;
	    case RHD_SENSED_TV_COMPOSITE:
		powerdown = 0 /* 0x1010000 */;
		break;
	    case RHD_SENSED_TV_COMPONENT:
		powerdown = 0x0;
		break;
	    case RHD_SENSED_VGA:
	    default:
		powerdown = 0;
		break;
	}
 	RHDRegWrite(Output, offset + DACA_ENABLE, 1);
	RHDRegWrite(Output, offset + DACA_POWERDOWN, 0);
	usleep (14);
	RHDRegMask(Output,  offset + DACA_POWERDOWN, powerdown, 0xffffff00);
	usleep(2);
	RHDRegWrite(Output, offset + DACA_FORCE_OUTPUT_CNTL, 0);
	RHDRegMask(Output,  offset + DACA_SYNC_SELECT, 0, 0x00000101);
	RHDRegWrite(Output, offset + DACA_SYNC_TRISTATE_CONTROL, 0);
	return;
    case RHD_POWER_RESET: /* don't bother */
	return;
    case RHD_POWER_SHUTDOWN:
    default:
	RHDRegMask(Output, offset + DACA_FORCE_DATA, 0x0, 0xffff);
	RHDRegMask(Output, offset + DACA_FORCE_OUTPUT_CNTL, 0x701, 0x701);
	RHDRegWrite(Output, offset + DACA_POWERDOWN, 0x01010100);
	RHDRegWrite(Output, offset + DACA_POWERDOWN, 0x01010101);
	RHDRegWrite(Output, offset + DACA_ENABLE, 0);
 	RHDRegWrite(Output, offset + DACA_ENABLE, 0);
	return;
    }
}

/*
 *
 */
static void
DACAPower(struct rhdOutput *Output, int Power)
{
    RHDFUNC(Output);

    DACPower(Output, REG_DACA_OFFSET, Power);
}

/*
 *
 */
static void
DACBPower(struct rhdOutput *Output, int Power)
{
    RHDFUNC(Output);

    DACPower(Output, REG_DACB_OFFSET, Power);
}

/*
 *
 */
static inline void
DACSave(struct rhdOutput *Output, CARD16 offset)
{
    struct rhdDACPrivate *Private = (struct rhdDACPrivate *) Output->Private;

    Private->Store_Powerdown = RHDRegRead(Output, offset + DACA_POWERDOWN);
    Private->Store_Force_Output_Control = RHDRegRead(Output, offset + DACA_FORCE_OUTPUT_CNTL);
    Private->Store_Force_Data = RHDRegRead(Output, offset + DACA_FORCE_DATA);
    Private->Store_Source_Select = RHDRegRead(Output, offset + DACA_SOURCE_SELECT);
    Private->Store_Sync_Select = RHDRegRead(Output, offset + DACA_SYNC_SELECT);
    Private->Store_Enable = RHDRegRead(Output, offset + DACA_ENABLE);
    Private->Store_Control1 = RHDRegRead(Output, offset + DACA_CONTROL1);
    Private->Store_Control2 = RHDRegRead(Output, offset + DACA_CONTROL2);
    Private->Store_Tristate_Control = RHDRegRead(Output, offset + DACA_SYNC_TRISTATE_CONTROL);
    
    Private->Stored = TRUE;
}

/*
 *
 */
static void
DACASave(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSave(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBSave(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSave(Output, REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
DACRestore(struct rhdOutput *Output, CARD16 offset)
{
    struct rhdDACPrivate *Private = (struct rhdDACPrivate *) Output->Private;

    RHDRegWrite(Output, offset + DACA_POWERDOWN, Private->Store_Powerdown);
    RHDRegWrite(Output, offset + DACA_FORCE_OUTPUT_CNTL, Private->Store_Force_Output_Control);
    RHDRegWrite(Output, offset + DACA_FORCE_DATA, Private->Store_Force_Data);
    RHDRegWrite(Output, offset + DACA_SOURCE_SELECT, Private->Store_Source_Select);
    RHDRegWrite(Output, offset + DACA_SYNC_SELECT, Private->Store_Sync_Select);
    RHDRegWrite(Output, offset + DACA_ENABLE, Private->Store_Enable);
    RHDRegWrite(Output, offset + DACA_CONTROL1, Private->Store_Control1);
    RHDRegWrite(Output, offset + DACA_CONTROL2, Private->Store_Control2);
    RHDRegWrite(Output, offset + DACA_SYNC_TRISTATE_CONTROL, Private->Store_Tristate_Control);
}

/*
 *
 */
static void
DACARestore(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!((struct rhdDACPrivate *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    DACRestore(Output, REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBRestore(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!((struct rhdDACPrivate *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    DACRestore(Output, REG_DACB_OFFSET);
}

/* ----------------------------------------------------------- */

/*
 *
 */
static CARD32
DACSenseRV620(struct rhdOutput *Output, CARD32 offset, Bool TV)
{
    CARD32 ret;
    CARD32 DetectControl, AutodetectIntCtl, ForceData,
	Control1, Control2, CompEnable;

    RHDFUNC(Output);

    Control1 = RHDRegRead(Output, offset + RV620_DACA_MACRO_CNTL); /* 7ef4 */
    Control2 = RHDRegRead(Output, offset + RV620_DACA_CONTROL2); /* 7058 */
    ForceData = RHDRegRead(Output, offset + RV620_DACA_FORCE_DATA);
    AutodetectIntCtl = RHDRegRead(Output, offset + RV620_DACA_AUTODETECT_INT_CONTROL);
    DetectControl = RHDRegRead(Output, offset + RV620_DACA_AUTODETECT_CONTROL);
    CompEnable = RHDRegRead(Output, offset + RV620_DACA_COMPARATOR_ENABLE);

    if (TV)
	RHDRegMask(Output, offset + RV620_DACA_CONTROL2, 0x100, 0xff00);
    else
	RHDRegMask(Output, offset + RV620_DACA_CONTROL2, 0x00, 0xff00);
    RHDRegMask(Output, offset + RV620_DACA_FORCE_DATA, 0x18, 0xffff);
    RHDRegMask(Output, offset + RV620_DACA_AUTODETECT_INT_CONTROL, 0x01, 0x01);
    RHDRegMask(Output, offset + RV620_DACA_AUTODETECT_CONTROL, 0x00, 0xff);
    RHDRegMask(Output, offset + RV620_DACA_MACRO_CNTL,
	       (offset > 0) ? 0x2502 :  0x2002, 0xffff);
    RHDRegMask(Output, offset + RV620_DACA_COMPARATOR_ENABLE, 0x70000, 0x070000);
    RHDRegMask(Output, offset + RV620_DACA_AUTODETECT_CONTROL, 0x01, 0xff);
    usleep(32);
    ret = RHDRegRead(Output, offset + RV620_DACA_AUTODETECT_STATUS);
    RHDRegWrite(Output, offset + RV620_DACA_AUTODETECT_CONTROL, DetectControl);
    RHDRegWrite(Output, offset + RV620_DACA_MACRO_CNTL, Control1);
    RHDRegWrite(Output, offset + RV620_DACA_CONTROL2, Control2);
    RHDRegWrite(Output, offset + RV620_DACA_FORCE_DATA, ForceData);
    RHDRegWrite(Output, offset + RV620_DACA_AUTODETECT_INT_CONTROL, AutodetectIntCtl);
#ifdef DEBUG
    RHDDebug(Output->scrnIndex, "DAC%i: ret = 0x%x %s\n",offset ? "A" : "B",
	     ret,TV ? "TV" : "");
#endif
    return ret;
}

/*
 *
 */
static enum rhdSensedOutput
DACASenseRV620(struct rhdOutput *Output, enum rhdConnectorType Type)
{
    RHDFUNC(Output);

    switch (Type) {
    case RHD_CONNECTOR_DVI:
    case RHD_CONNECTOR_VGA:
	return  (DACSenseRV620(Output, RV620_REG_DACA_OFFSET, FALSE)
		  & 0x1010100) ? RHD_SENSED_VGA : RHD_SENSED_NONE;
    case RHD_CONNECTOR_TV:
	switch (DACSenseRV620(Output, RV620_REG_DACA_OFFSET, TRUE)
		& 0x1010100) {
	    case 0x1010100:
		return RHD_SENSED_TV_COMPONENT;
	    case 0x10100:
		return RHD_SENSED_TV_SVIDEO;
	    case 0x1000000:
		return RHD_SENSED_TV_COMPOSITE;
	    default:
		return RHD_SENSED_NONE;
	}
    default:
	xf86DrvMsg(Output->scrnIndex, X_WARNING,
		   "%s: connector type %d is not supported.\n",
		   __func__, Type);
	return RHD_SENSED_NONE;
    }
}

/*
 *
 */
static enum rhdSensedOutput
DACBSenseRV620(struct rhdOutput *Output, enum rhdConnectorType Type)
{
    RHDFUNC(Output);

    switch (Type) {
    case RHD_CONNECTOR_DVI:
    case RHD_CONNECTOR_VGA:
	return  (DACSenseRV620(Output, RV620_REG_DACB_OFFSET, FALSE)
		  & 0x1010100) ? RHD_SENSED_VGA : RHD_SENSED_NONE;
    case RHD_CONNECTOR_TV:
	switch (DACSenseRV620(Output, RV620_REG_DACB_OFFSET, TRUE)
		& 0x1010100) {
	    case 0x1000000:
		return RHD_SENSED_TV_COMPONENT;
	    case 0x1010100:
		return RHD_SENSED_TV_SVIDEO;
	    case 0x10100:
		return RHD_SENSED_TV_COMPOSITE;
	    default:
		return RHD_SENSED_NONE;
	}
    default:
	xf86DrvMsg(Output->scrnIndex, X_WARNING,
		   "%s: connector type %d is not supported.\n",
		   __func__, Type);
	return RHD_SENSED_NONE;
    }
}

/*
 *
 */
static inline void
DACSetRV620(struct rhdOutput *Output, CARD16 offset)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    CARD32 source;
    CARD32 mode;
    CARD32 tv;
    CARD32 white_fine;

    switch (rhdPtr->tvMode) {
	case RHD_TV_NTSC:
	case RHD_TV_NTSCJ:
	    white_fine = 0x2000;
	    mode = 0x1;
	    break;
	case RHD_TV_PAL:
	case RHD_TV_PALN:
	case RHD_TV_PALCN:
	case RHD_TV_PAL60:
	default:
	    white_fine = 0x2600;
	    mode = 0x0;
	    break;
    }

    switch (Output->SensedType) {
	case RHD_SENSED_TV_SVIDEO:
	    tv = 0x100;
	    source = 0x2; /* tv encoder */
	    break;
	case RHD_SENSED_TV_COMPOSITE:
	    tv = 0x100;
	    source = 0x2; /* tv encoder */
	    break;
	case RHD_SENSED_TV_COMPONENT:
	    white_fine = 0x2500;
	    mode = 3; /* HDTV */
	    tv = 0x100; /* tv on?? */
	    source = 0x2; /* tv encoder  ?? */
	    break;
	case RHD_SENSED_VGA:
	default:
	    white_fine = 0x2500;
	    mode = 2;
	    tv = 0;
	    source = Output->Crtc->Id;
	    break;
    }
    RHDRegWrite(Output, offset + RV620_DACA_MACRO_CNTL, mode); /* no fine control yet */
    RHDRegMask(Output,  offset + RV620_DACA_SOURCE_SELECT, source, 0x00000003);
    RHDRegMask(Output,  offset + RV620_DACA_CONTROL2, tv, 0x0100); /* tv enable/disable */
    /* use fine control from white_fine control register */
    RHDRegMask(Output, offset + RV620_DACA_AUTO_CALIB_CONTROL, 0x0, 0x4);
    RHDRegMask(Output, offset + RV620_DACA_BGADJ_SRC, 0x0, 0x30);
    RHDRegMask(Output, offset + RV620_DACA_MACRO_CNTL, 0x210000 | white_fine, 0xffff00);
}

/*
 *
 */
static void
DACASetRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSetRV620(Output, RV620_REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBSetRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSetRV620(Output, RV620_REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
DACPowerRV620(struct rhdOutput *Output, CARD16 offset, int Power)
{
    CARD32 powerdown;

    switch (Power) {
	case RHD_POWER_ON:
	    switch (Output->SensedType) {
		case RHD_SENSED_TV_SVIDEO:
		    powerdown = 0 /* 0x100 */;
		    break;
		case RHD_SENSED_TV_COMPOSITE:
		    powerdown = 0 /* 0x1010000 */;
		    break;
		case RHD_SENSED_TV_COMPONENT:
		    powerdown = 0x0;
		    break;
		case RHD_SENSED_VGA:
		default:
		    powerdown = 0;
		    break;
	    }

	    if (!(RHDRegRead(Output, offset + RV620_DACA_ENABLE) & 0x01))
		RHDRegMask(Output, offset + RV620_DACA_ENABLE, 0x1, 0xff);
	    RHDRegMask(Output,  offset + RV620_DACA_FORCE_OUTPUT_CNTL, 0x01, 0x01);
	    RHDRegMask(Output,  offset + RV620_DACA_POWERDOWN, 0x0, 0xff);
	    usleep (0x14);
	    RHDRegMask(Output,  offset + RV620_DACA_POWERDOWN, powerdown, 0xffffff00);
	    usleep(2);
	    RHDRegMask(Output,  offset + RV620_DACA_FORCE_DATA, 0, 0x0000ffff);
	    RHDRegWrite(Output, offset + RV620_DACA_FORCE_OUTPUT_CNTL, 0x0);
	    RHDRegWrite(Output, offset + RV620_DACA_SYNC_TRISTATE_CONTROL, 0);
	    return;
	case RHD_POWER_RESET: /* don't bother */
	    return;
	case RHD_POWER_SHUTDOWN:
	default:
	    RHDRegWrite(Output, offset + RV620_DACA_POWERDOWN, 0x01010100);
	    RHDRegWrite(Output, offset + RV620_DACA_POWERDOWN, 0x01010101);
	    RHDRegWrite(Output, offset + RV620_DACA_ENABLE, 0);
	    RHDRegMask(Output, offset + RV620_DACA_FORCE_DATA, 0, 0xffff);
	    RHDRegMask(Output, offset + RV620_DACA_FORCE_OUTPUT_CNTL, 0x701, 0x701);
	    return;
    }
}

/*
 *
 */
static void
DACAPowerRV620(struct rhdOutput *Output, int Power)
{
    RHDFUNC(Output);

    DACPowerRV620(Output, RV620_REG_DACA_OFFSET, Power);
}

/*
 *
 */
static void
DACBPowerRV620(struct rhdOutput *Output, int Power)
{
    RHDFUNC(Output);

    DACPowerRV620(Output, RV620_REG_DACB_OFFSET, Power);
}

/*
 *
 */
static inline void
DACSaveRV620(struct rhdOutput *Output, CARD16 offset)
{
    struct rhdDACPrivate *Private = (struct rhdDACPrivate *) Output->Private;

    Private->Store_Powerdown = RHDRegRead(Output, offset + RV620_DACA_POWERDOWN);
    Private->Store_Force_Output_Control = RHDRegRead(Output, offset + RV620_DACA_FORCE_OUTPUT_CNTL);
    Private->Store_Force_Data = RHDRegRead(Output, offset + RV620_DACA_FORCE_DATA);
    Private->Store_Source_Select = RHDRegRead(Output, offset + RV620_DACA_SOURCE_SELECT);
    Private->Store_Enable = RHDRegRead(Output, offset + RV620_DACA_ENABLE);
    Private->Store_Control1 = RHDRegRead(Output, offset + RV620_DACA_MACRO_CNTL);
    Private->Store_Control2 = RHDRegRead(Output, offset + RV620_DACA_CONTROL2);
    Private->Store_Tristate_Control = RHDRegRead(Output, offset + RV620_DACA_SYNC_TRISTATE_CONTROL);
    Private->Store_Auto_Calib_Control = RHDRegRead(Output, offset + RV620_DACA_AUTO_CALIB_CONTROL);
    Private->Store_Dac_Bgadj_Src = RHDRegRead(Output, offset + RV620_DACA_BGADJ_SRC);
    
    Private->Stored = TRUE;
}

/*
 *
 */
static void
DACASaveRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSaveRV620(Output, RV620_REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBSaveRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    DACSaveRV620(Output, RV620_REG_DACB_OFFSET);
}

/*
 *
 */
static inline void
DACRestoreRV620(struct rhdOutput *Output, CARD16 offset)
{
    struct rhdDACPrivate *Private = (struct rhdDACPrivate *) Output->Private;

    RHDRegWrite(Output, offset + RV620_DACA_BGADJ_SRC, Private->Store_Dac_Bgadj_Src);
    RHDRegWrite(Output, offset + RV620_DACA_AUTO_CALIB_CONTROL, Private->Store_Auto_Calib_Control);
    RHDRegWrite(Output, offset + RV620_DACA_POWERDOWN, Private->Store_Powerdown);
    RHDRegWrite(Output, offset + RV620_DACA_FORCE_OUTPUT_CNTL, Private->Store_Force_Output_Control);
    RHDRegWrite(Output, offset + RV620_DACA_FORCE_DATA, Private->Store_Force_Data);
    RHDRegWrite(Output, offset + RV620_DACA_SOURCE_SELECT, Private->Store_Source_Select);
    RHDRegWrite(Output, offset + RV620_DACA_ENABLE, Private->Store_Enable);
    RHDRegWrite(Output, offset + RV620_DACA_MACRO_CNTL, Private->Store_Control1);
    RHDRegWrite(Output, offset + RV620_DACA_CONTROL2, Private->Store_Control2);
    RHDRegWrite(Output, offset + RV620_DACA_SYNC_TRISTATE_CONTROL, Private->Store_Tristate_Control);

}

/*
 *
 */
static void
DACARestoreRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!((struct rhdDACPrivate *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }
    DACRestoreRV620(Output, RV620_REG_DACA_OFFSET);
}

/*
 *
 */
static void
DACBRestoreRV620(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!((struct rhdDACPrivate *) Output->Private)->Stored) {
	xf86DrvMsg(Output->scrnIndex, X_ERROR,
		   "%s: No registers stored.\n", __func__);
	return;
    }

    DACRestoreRV620(Output, RV620_REG_DACB_OFFSET);
}

/* ----------------------------------------------------------- */

/*
 *
 */
static ModeStatus
DACModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDFUNC(Output);

    if (Mode->Clock < 20000)
	return MODE_CLOCK_LOW;

    if (Mode->Clock > 400000)
	return MODE_CLOCK_HIGH;

    return MODE_OK;
}

/*
 *
 */
static void
DACDestroy(struct rhdOutput *Output)
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
struct rhdOutput *
RHDDACAInit(RHDPtr rhdPtr)
{
    struct rhdOutput *Output;
    struct rhdDACPrivate *Private;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhdOutput), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->Name = "DAC A";
    Output->Id = RHD_OUTPUT_DACA;

    if (rhdPtr->ChipSet < RHD_RV620) {
	Output->Sense = DACASense;
	Output->Mode = DACASet;
	Output->Power = DACAPower;
	Output->Save = DACASave;
	Output->Restore = DACARestore;
    } else {
	Output->Sense = DACASenseRV620;
	Output->Mode = DACASetRV620;
	Output->Power = DACAPowerRV620;
	Output->Save = DACASaveRV620;
	Output->Restore = DACARestoreRV620;
    }
    Output->ModeValid = DACModeValid;
    Output->Destroy = DACDestroy;
    Private = xnfcalloc(sizeof(struct rhdDACPrivate), 1);
    Output->Private = Private;

    return Output;
}

/*
 *
 */
struct rhdOutput *
RHDDACBInit(RHDPtr rhdPtr)
{
    struct rhdOutput *Output;
    struct rhdDACPrivate *Private;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhdOutput), 1);

    Output->scrnIndex = rhdPtr->scrnIndex;
    Output->Name = "DAC B";
    Output->Id = RHD_OUTPUT_DACB;

    if (rhdPtr->ChipSet < RHD_RV620) {
	Output->Sense = DACBSense;
	Output->Mode = DACBSet;
	Output->Power = DACBPower;
	Output->Save = DACBSave;
	Output->Restore = DACBRestore;
    } else {
	Output->Sense = DACBSenseRV620;
	Output->Mode = DACBSetRV620;
	Output->Power = DACBPowerRV620;
	Output->Save = DACBSaveRV620;
	Output->Restore = DACBRestoreRV620;
    }
    Output->ModeValid = DACModeValid;
    Output->Destroy = DACDestroy;

    Private = xnfcalloc(sizeof(struct rhdDACPrivate), 1);
    Output->Private = Private;

    return Output;
}

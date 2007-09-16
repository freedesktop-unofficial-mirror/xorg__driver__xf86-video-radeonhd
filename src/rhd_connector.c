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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

/* for usleep */
#include "xf86_ansic.h"

#include "rhd.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_regs.h"
#include "rhd_monitor.h"

#include "xf86i2c.h"
#include "rhd_i2c.h"

/*
 *
 */
struct rhdHPD {
    Bool Stored;
    CARD32 StoreMask;
    CARD32 StoreEnable;
};

/*
 *
 */
void
RHDHPDSave(RHDPtr rhdPtr)
{
    struct rhdHPD *hpd = rhdPtr->HPD;

    RHDFUNC(rhdPtr);

    hpd->StoreMask = RHDRegRead(rhdPtr, DC_GPIO_HPD_MASK);
    hpd->StoreEnable = RHDRegRead(rhdPtr, DC_GPIO_HPD_EN);

    hpd->Stored = TRUE;
}

/*
 *
 */
void
RHDHPDRestore(RHDPtr rhdPtr)
{
    struct rhdHPD *hpd = rhdPtr->HPD;

    RHDFUNC(rhdPtr);

    if (hpd->Stored) {
	RHDRegWrite(rhdPtr, DC_GPIO_HPD_MASK, hpd->StoreMask);
	RHDRegWrite(rhdPtr, DC_GPIO_HPD_EN, hpd->StoreEnable);
    } else
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		   "%s: no registers stored.\n", __func__);
}

/*
 *
 */
static void
RHDHPDSet(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    /* give the hw full control */
    RHDRegWrite(rhdPtr, DC_GPIO_HPD_MASK, 0);
    RHDRegWrite(rhdPtr, DC_GPIO_HPD_EN, 0);

    usleep(1);
}

/*
 *
 */
static Bool
RHDHPDCheck(struct rhdConnector *Connector)
{
    return (RHDRegRead(Connector, DC_GPIO_HPD_Y) & Connector->HPDMask);
}

/*
 *
 */
Bool
RHDConnectorsInit(RHDPtr rhdPtr, struct rhdcard *Card)
{
    struct rhdConnector *Connector;
    struct rhdOutput *Output;
    int i, j, k, l;

    RHDFUNC(rhdPtr);

    /* Init HPD */
    rhdPtr->HPD = xnfcalloc(sizeof(struct rhdHPD), 1);

    RHDHPDSave(rhdPtr);
    RHDHPDSet(rhdPtr);

    for (i = 0, j = 0; i < RHD_CONNECTORS_MAX; i++) {
	if (Card->Connectors[i].Type == RHD_CONNECTOR_NONE)
	    continue;

	RHDDebug(rhdPtr->scrnIndex, "%s: %d (%s) type %d, ddc %d, hpd %d\n",
		 __func__, i, Card->Connectors[i].Name, Card->Connectors[i].Type,
		 Card->Connectors[i].DDC, Card->Connectors[i].HPD);

	Connector = xnfcalloc(sizeof(struct rhdConnector), 1);

	Connector->scrnIndex = rhdPtr->scrnIndex;
	Connector->Type = Card->Connectors[i].Type;
	Connector->Name = Card->Connectors[i].Name;

	/* Get the DDC bus of this connector */
	if (Card->Connectors[i].DDC != RHD_DDC_NONE) {
	    RHDI2CDataArg data;
	    int ret;

	    data.i = Card->Connectors[i].DDC;
	    ret = RHDI2CFunc(rhdPtr->scrnIndex,
			     rhdPtr->I2C, RHD_I2C_GETBUS, &data);
	    if (ret == RHD_I2C_SUCCESS)
		Connector->DDC = data.i2cBusPtr;
	}

	/* attach HPD */
	switch(Card->Connectors[i].HPD) {
	case RHD_HPD_0:
	    Connector->HPDMask = 0x00000001;
	    Connector->HPDCheck = RHDHPDCheck;
	    break;
	case RHD_HPD_1:
	    Connector->HPDMask = 0x00000100;
	    Connector->HPDCheck = RHDHPDCheck;
	    break;
	case RHD_HPD_2:
	    Connector->HPDMask = 0x00010000;
	    Connector->HPDCheck = RHDHPDCheck;
	    break;
	default:
	    Connector->HPDCheck = NULL;
	    break;
	}

	/* create Outputs */
	for (k = 0; k < 2; k++) {
	    if (Card->Connectors[i].Output[k] == RHD_OUTPUT_NONE)
		continue;

	    /* Check whether the output exists already */
	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Id == Card->Connectors[i].Output[k])
		    break;

	    if (!Output) {
		switch (Card->Connectors[i].Output[k]) {
		case RHD_OUTPUT_DACA:
		    Output = RHDDACAInit(rhdPtr);
		    RHDOutputAdd(rhdPtr, Output);
		    break;
		case RHD_OUTPUT_DACB:
		    Output = RHDDACBInit(rhdPtr);
		    RHDOutputAdd(rhdPtr, Output);
		    break;
		case RHD_OUTPUT_TMDSA:
		    Output = RHDTMDSAInit(rhdPtr);
		    RHDOutputAdd(rhdPtr, Output);
		    break;
		case RHD_OUTPUT_LVTMA:
		    Output = RHDLVTMAInit(rhdPtr,  Card->Connectors[i].Type);
		    RHDOutputAdd(rhdPtr, Output);
		    break;
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
			       "%s: unhandled output id: %d\n", __func__,
			       Card->Connectors[i].Output[k]);
		    break;
		}
	    }

	    if (Output) {
		xf86DrvMsg(rhdPtr->scrnIndex, X_PROBED,
			   "Attaching Output %s to Connector %s\n",
			   Output->Name, Connector->Name);
		for (l = 0; l < 2; l++)
		    if (!Connector->Output[l]) {
			Connector->Output[l] = Output;
			break;
		    }
	    }
	}

	Connector->Monitor = RHDMonitorInit(Connector);

	rhdPtr->Connector[j] = Connector;
	j++;
    }

    RHDHPDRestore(rhdPtr);

    return (j && 1);
}

/*
 *
 */
void
RHDConnectorsDestroy(RHDPtr rhdPtr)
{
    struct rhdConnector *Connector;
    int i;

    RHDFUNC(rhdPtr);

    for (i = 0; i < RHD_CONNECTORS_MAX; i++) {
	Connector = rhdPtr->Connector[i];
	if (Connector) {
	    if (Connector->Monitor)
		RHDMonitorDestroy(Connector->Monitor);
	    xfree(Connector);
	}
    }
}

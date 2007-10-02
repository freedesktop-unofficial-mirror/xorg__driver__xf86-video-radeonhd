/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
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
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_regs.h"
#include "rhd_monitor.h"
#include "rhd_card.h"

#include "rhd_atombios.h"

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
RHDConnectorsInit(RHDPtr rhdPtr, struct rhdCard *Card)
{
    struct rhdConnectors *Connectors;
    struct rhdConnector *Connector;
    struct rhdOutput *Output;
    int i, j, k, l;
    AtomBIOSArg data;

    RHDFUNC(rhdPtr);

    /* Init HPD */
    rhdPtr->HPD = xnfcalloc(sizeof(struct rhdHPD), 1);

#ifdef ATOM_BIOS
    /* To test the connector table parser we make it the default */
    if (RHDAtomBIOSFunc(rhdPtr->scrnIndex,
			rhdPtr->atomBIOS, ATOMBIOS_GET_CONNECTORS, &data) == ATOM_SUCCESS) {
	Connectors = data.ptr;
    }
#else
    if (!Card)
	return FALSE;

    Connectors = Card->Connectors;
#endif

    RHDHPDSave(rhdPtr);
    RHDHPDSet(rhdPtr);

    for (i = 0, j = 0; i < RHD_CONNECTORS_MAX; i++) {
	if (Connectors[i].Type == RHD_CONNECTOR_NONE)
	    continue;

	RHDDebug(rhdPtr->scrnIndex, "%s: %d (%s) type %d, ddc %d, hpd %d\n",
		 __func__, i, Connectors[i].Name, Connectors[i].Type,
		 Connectors[i].DDC, Connectors[i].HPD);

	Connector = xnfcalloc(sizeof(struct rhdConnector), 1);

	Connector->scrnIndex = rhdPtr->scrnIndex;
	Connector->Type = Connectors[i].Type;
	Connector->Name = Connectors[i].Name;

	/* Get the DDC bus of this connector */
	if (Connectors[i].DDC != RHD_DDC_NONE) {
	    RHDI2CDataArg data;
	    int ret;

	    data.i = Connectors[i].DDC;
	    ret = RHDI2CFunc(rhdPtr->scrnIndex,
			     rhdPtr->I2C, RHD_I2C_GETBUS, &data);
	    if (ret == RHD_I2C_SUCCESS)
		Connector->DDC = data.i2cBusPtr;
	}

	/* attach HPD */
	switch(Connectors[i].HPD) {
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
	    if (Connectors[i].Output[k] == RHD_OUTPUT_NONE)
		continue;

	    /* Check whether the output exists already */
	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Id == Connectors[i].Output[k])
		    break;

	    if (!Output) {
		switch (Connectors[i].Output[k]) {
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
		    Output = RHDLVTMAInit(rhdPtr,  Connectors[i].Type);
		    RHDOutputAdd(rhdPtr, Output);
		    break;
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
			       "%s: unhandled output id: %d\n", __func__,
			       Connectors[i].Output[k]);
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

/*
 *
 */
void
RhdPrintConnectorTable(int scrnIndex, struct rhdConnectors *cp)
{
    int n;
    const char *c_name[] =
	{ "RHD_CONNECTOR_NONE", "RHD_CONNECTOR_VGA", "RHD_CONNECTOR_DVI",
	  "RHD_CONNECTOR_PANEL", "RHD_CONNECTOR_TV"
	};
    const char *ddc_name[] =
	{ "RHD_DDC_0", "RHD_DDC_1", "RHD_DDC_2", "RHD_DDC_3" };

    const char *hpd_name[] =
	{ "RHD_HPD_NONE", "RHD_HPD_0", "RHD_HPD_1", "RHD_HPD_2" };

    const char *output_name[] =
	{ "RHD_OUTPUT_NONE", "RHD_OUTPUT_DACA", "RHD_OUTPUT_DACB", "RHD_OUTPUT_TMDSA",
	  "RHD_OUTPUT_LVTMA"
	};

    for (n = 0; n < RHD_CONNECTORS_MAX; n++) {
	if (cp[n].Type == RHD_CONNECTOR_NONE)
	    break;
	xf86DrvMsg(scrnIndex, X_INFO, "Connector[%i] {%s, \"%s\", %s, %s, { %s, %s } }\n",
		   n, c_name[cp[n].Type], cp[n].Name,
		   cp[n].DDC == RHD_DDC_NONE ? "DDC_NONE" : ddc_name[cp[n].DDC],
		   hpd_name[cp[n].HPD], output_name[cp[n].Output[0]],
		   output_name[cp[n].Output[1]]);
    }
}

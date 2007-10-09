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

#ifndef _RHD_CARD_H
#define _RHD_CARD_H

/* Four bytes in TYPE/DDC layout: see rhd_connector.h */
struct rhdConnectorInfo {
    rhdConnectorType Type;
    char *Name;
    rhdDDC DDC;
    rhdHPD HPD;
    rhdOutputType Output[MAX_OUTPUTS_PER_CONNECTOR];
};

struct rhdCard {
    CARD16 device;
    CARD16 card_vendor;
    CARD16 card_device;
    char *name;

    struct rhdConnectorInfo ConnectorInfo[RHD_CONNECTORS_MAX];

    struct Lvds {
	CARD16 PowerRefDiv;
	CARD16 BlonRefDiv;
	CARD16 PowerDigToDE;
	CARD16 PowerDEToBL;
	CARD16 OffDelay;
    } Lvds;
};

void RhdPrintConnectorInfo(int scrnIndex, struct rhdConnectorInfo *cp);

#endif /* _RHD_CARD_H */

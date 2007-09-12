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

#ifndef _RHD_CONNECTOR_H
#define _RHD_CONNECTOR_H

/* so that we can map which is which */
#define RHD_CONNECTOR_NONE  0
#define RHD_CONNECTOR_VGA   1
#define RHD_CONNECTOR_DVI   2
#define RHD_CONNECTOR_PANEL 3
#define RHD_CONNECTOR_TV    4
/* add whatever */

/* map which DDC bus is where */
#define RHD_DDC_NONE 0xFF
#define RHD_DDC_0 0
#define RHD_DDC_1 1
#define RHD_DDC_2 2
#define RHD_DDC_3 3

/* map which HPD plug is used where */
#define RHD_HPD_NONE 0
#define RHD_HPD_0 1
#define RHD_HPD_1 2
#define RHD_HPD_2 3

struct rhdConnector {
    int scrnIndex;

    CARD8 Type;
    char *Name;

    struct _I2CBusRec *DDC;

    /* HPD handling here */
    int  HPDMask;
    Bool HPDAttached;
    Bool (*HPDCheck) (struct rhdConnector *Connector);

    /* Add rhdMonitor pointer here. */
    /* This is created either from default, config or from EDID */
    struct rhdMonitor *Monitor;

    /* Point back to our Outputs, so we can handle sensing better */
    struct rhdOutput *Output[2];
};

Bool RHDConnectorsInit(RHDPtr rhdPtr, struct rhdcard *Card);
void RHDHPDSave(RHDPtr rhdPtr);
void RHDHPDRestore(RHDPtr rhdPtr);
void RHDConnectorsDestroy(RHDPtr rhdPtr);

#endif /* _RHD_CONNECTOR_H */

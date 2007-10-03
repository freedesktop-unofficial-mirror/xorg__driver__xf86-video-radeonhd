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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86DDC.h"

#include "rhd.h"
#include "rhd_connector.h"
#include "rhd_modes.h"
#include "rhd_monitor.h"

/*
 *
 */
void
rhdMonitorPrint(struct rhdMonitor *Monitor)
{
    int i;

    xf86Msg(X_NONE, "    Bandwidth: %dMHz\n", Monitor->Bandwidth / 1000);
    xf86Msg(X_NONE, "    Horizontal timing:\n");
    for (i = 0; i < Monitor->numHSync; i++)
	xf86Msg(X_NONE, "        %3.1f - %3.1fkHz\n",  Monitor->HSync[i].lo,
		Monitor->HSync[i].hi);
    xf86Msg(X_NONE, "    Vertical timing:\n");
    for (i = 0; i < Monitor->numVRefresh; i++)
	xf86Msg(X_NONE, "        %3.1f - %3.1fHz\n",  Monitor->VRefresh[i].lo,
		Monitor->VRefresh[i].hi);

    if (Monitor->ReducedAllowed)
	xf86Msg(X_NONE, "    Allows reduced blanking.\n");
    if (Monitor->UseFixedModes)
	xf86Msg(X_NONE, "    Uses Fixed Modes.\n");

    if (!Monitor->Modes)
	xf86Msg(X_NONE, "    No modes are provided.\n");
    else {
	DisplayModePtr Mode;

	xf86Msg(X_NONE, "    Attached modes:\n");
	for (Mode = Monitor->Modes; Mode; Mode = Mode->next) {
	    xf86Msg(X_NONE, "        ");
	    RHDPrintModeline(Mode);
	}
    }
}

/*
 *
 */
struct rhdMonitor *
RHDConfigMonitor(MonPtr Config)
{
    struct rhdMonitor *Monitor;
    DisplayModePtr Mode;
    int i;

    if (!Config || !Config->id ||
	!strcasecmp(Config->id, "<default monitor>"))
	return NULL;

    Monitor = xnfcalloc(sizeof(struct rhdMonitor), 1);

    Monitor->Name = xnfstrdup(Config->id);

    if (Config->nHsync) {
        Monitor->numHSync = Config->nHsync;
        for (i = 0; i < Config->nHsync; i++) {
            Monitor->HSync[i].lo = Config->hsync[i].lo;
            Monitor->HSync[i].hi = Config->hsync[i].hi;
        }
    } else if (!Monitor->numHSync) {
        Monitor->numHSync = 3;
        Monitor->HSync[0].lo = 31.5;
        Monitor->HSync[0].hi = 31.5;
        Monitor->HSync[1].lo = 35.15;
        Monitor->HSync[1].hi = 35.15;
        Monitor->HSync[2].lo = 35.5;
        Monitor->HSync[2].hi = 35.5;
    }

    if (Config->nVrefresh) {
        Monitor->numVRefresh = Config->nVrefresh;
        for (i = 0; i < Config->nVrefresh; i++) {
            Monitor->VRefresh[i].lo = Config->vrefresh[i].lo;
            Monitor->VRefresh[i].hi = Config->vrefresh[i].hi;
        }
    } else if (!Monitor->numVRefresh) {
        Monitor->numVRefresh = 1;
        Monitor->VRefresh[0].lo = 50;
        Monitor->VRefresh[0].hi = 61;
    }

#ifdef MONREC_HAS_REDUCED
    if (Config->reducedblanking)
        Monitor->ReducedAllowed = TRUE;
#endif

#ifdef MONREC_HAS_BANDWIDTH
    if (Config->maxPixClock)
        Monitor->Bandwidth = Config->maxPixClock;
#endif

    for (Mode = Config->Modes; Mode; Mode = Mode->next)
	Monitor->Modes = RHDModesAdd(Monitor->Modes, RHDModeCopy(Mode));

     xf86DrvMsg(Monitor->scrnIndex, X_INFO, "Configured Monitor \"%s\":\n",
		Monitor->Name);
     rhdMonitorPrint(Monitor);

     return Monitor;
}

/*
 *
 */
struct rhdMonitor *
RHDDefaultMonitor(int scrnIndex)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    struct rhdMonitor *Monitor;
    DisplayModePtr Mode;

    Monitor = xnfcalloc(sizeof(struct rhdMonitor), 1);

    Monitor->scrnIndex = scrnIndex;

    Monitor->Name = xnfstrdup("Default (SVGA)");

    /* timing for pathetic 14" svga monitors */
    Monitor->numHSync = 3;
    Monitor->HSync[0].lo = 31.5;
    Monitor->HSync[0].hi = 31.5;
    Monitor->HSync[1].lo = 35.15;
    Monitor->HSync[1].hi = 35.15;
    Monitor->HSync[2].lo = 35.5;
    Monitor->HSync[2].hi = 35.5;

    Monitor->numVRefresh = 1;
    Monitor->VRefresh[0].lo = 50;
    Monitor->VRefresh[0].hi = 61;

    /* Try to add configged modes anyway */
    if (pScrn->confScreen->monitor)
	for (Mode = pScrn->confScreen->monitor->Modes; Mode; Mode = Mode->next)
	    Monitor->Modes = RHDModesAdd(Monitor->Modes, RHDModeCopy(Mode));

    xf86DrvMsg(Monitor->scrnIndex, X_INFO, "Default Monitor \"%s\":\n",
		Monitor->Name);
    rhdMonitorPrint(Monitor);

    return Monitor;
}

/*
 *
 */
struct rhdMonitor *
RHDMonitorInit(struct rhdConnector *Connector)
{
    struct rhdMonitor *Monitor;
    xf86MonPtr EDID = NULL;

    RHDFUNC(Connector);

    /* TODO: We might want to use panel resolution from atombios in future */
    if (!Connector->DDC)
	return NULL;

    EDID = xf86DoEDID_DDC2(Connector->scrnIndex, Connector->DDC);
    if (!EDID) {
	xf86DrvMsg(Connector->scrnIndex, X_INFO,
		   "No EDID data found on connector \"%s\"\n", Connector->Name);
	return NULL;
    }

    Monitor = xnfcalloc(sizeof(struct rhdMonitor), 1);

    Monitor->scrnIndex = Connector->scrnIndex;

    RHDMonitorEDIDSet(Monitor, EDID);
    xfree(EDID);

    if (Connector->Type == RHD_CONNECTOR_PANEL)
	/* Prevent other resolutions on directly connected panels */
	Monitor->UseFixedModes = TRUE;

    xf86DrvMsg(Monitor->scrnIndex, X_INFO, "Monitor \"%s\" connected to \"%s\":\n",
	       Monitor->Name, Connector->Name);
    rhdMonitorPrint(Monitor);

    return Monitor;
}

/*
 *
 */
void
RHDMonitorDestroy(struct rhdMonitor *Monitor)
{
    DisplayModePtr Mode, Next;

    for (Mode = Monitor->Modes; Mode;) {
	Next = Mode->next;

	xfree(Mode->name);
	xfree(Mode);

	Mode = Next;
    }

    xfree(Monitor->Name);
    xfree(Monitor);
}

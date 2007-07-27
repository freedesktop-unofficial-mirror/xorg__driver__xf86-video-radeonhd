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

#define MODULEVENDORSTRING "AMD GPG"  /* @@@ */
#include "xf86.h"
#include "xf86_OSproc.h"

/* For PIO/MMIO */
#include "compiler.h"

#include "xf86Resources.h"

#include "xf86PciInfo.h"
/* do we need to access PCI config space directly? */
#include "xf86Pci.h"

/* This is used for module versioning */
#include "xf86Version.h"

/* Memory manager */
#include "xf86fbman.h"

/* For SW cursor */
#include "mipointer.h"

/* optional backing store */
#include "mibstore.h"

/* mi colormap manipulation */
#include "micmap.h"

#include "xf86cmap.h"

#include "fb.h"

/* Needed by Resources Access Control (RAC) */
#include "xf86RAC.h"

#define DPMS_SERVER
#include "X11/extensions/dpms.h"

/* int10 - for now at least */
#include "xf86int10.h"
#include "vbe.h"

/* Needed for Device Data Channel (DDC) support */
#include "xf86DDC.h"

#include "picturestr.h"
/*
 * Driver data structures.
 */
#include "rhd.h"
#include "rhd_regs.h"
#include "rhd_macros.h"
#include "rhd_cursor.h"

/* ??? */
#include "servermd.h"

/* Mandatory functions */
static const OptionInfoRec *	RHDAvailableOptions(int chipid, int busid);
static void     RHDIdentify(int flags);
static Bool     RHDProbe(DriverPtr drv, int flags);
static Bool     RHDPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     RHDScreenInit(int Index, ScreenPtr pScreen, int argc,
                                  char **argv);
static Bool     RHDEnterVT(int scrnIndex, int flags);
static void     RHDLeaveVT(int scrnIndex, int flags);
static Bool     RHDCloseScreen(int scrnIndex, ScreenPtr pScreen);
static void     RHDFreeScreen(int scrnIndex, int flags);
static Bool     RHDSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
static void     RHDAdjustFrame(int scrnIndex, int x, int y, int flags);
static void     RHDDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                                             int PowerManagementMode,
                                             int flags);
static void     RHDLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                               LOCO *colors, VisualPtr pVisual);

static void     rhdSave(ScrnInfoPtr pScrn);
static void     rhdRestore(ScrnInfoPtr pScrn, RHDRegPtr restore);
static void     rhdUnlock(ScrnInfoPtr pScrn);
static void     rhdLock(ScrnInfoPtr pScrn);
static void     rhdSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     rhdModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     rhdMMIOMap(ScrnInfoPtr pScrn);
static void     rhdMMIOUnmap(ScrnInfoPtr pScrn);
static Bool     rhdFBMap(ScrnInfoPtr pScrn);
static void     rhdFBUnmap(ScrnInfoPtr pScrn);
static Bool     rhdSaveScreen(ScreenPtr pScrn, int on);

#define RHD_VERSION 0001
#define RHD_NAME "RADEONHD"
#define RHD_DRIVER_NAME "radeonhd"

#define RHD_MAJOR_VERSION 0
#define RHD_MINOR_VERSION 0
#define RHD_PATCHLEVEL    1

/* keep accross drivers */
static int pix24bpp = 0;

_X_EXPORT DriverRec RADEONHD = {
    RHD_VERSION,
    RHD_DRIVER_NAME,
    RHDIdentify,
    RHDProbe,
    RHDAvailableOptions,
    NULL,
    0
};

static SymTabRec RHDChipsets[] = {
    {RHD_RV530, "RV530" },
    {-1,      NULL }
};

static PciChipsets RHDPCIchipsets[] = {
    { RHD_RV530, 0x71C2, RES_SHARED_VGA },
    { -1,	     -1,	     RES_UNDEFINED}
};

typedef enum {
    OPTION_NOACCEL,
    OPTION_SW_CURSOR,
    OPTION_PCI_BURST,
    OPTION_VIDEORAM
} RHDOpts;

static const OptionInfoRec RHDOptions[] = {
    { OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_BURST,	 "pciBurst",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_VIDEORAM,  "videoram",     OPTV_INTEGER,   {0}, FALSE }, /* kill me */
    { -1,                  NULL,           OPTV_NONE,	{0}, FALSE }
};

static MODULESETUPPROTO(rhdSetup);

static XF86ModuleVersionInfo rhdVersRec =
{
	"radeonhd",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	RHD_MAJOR_VERSION, RHD_MINOR_VERSION, RHD_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

_X_EXPORT XF86ModuleData radeonhdModuleData = { &rhdVersRec, rhdSetup, NULL };

static pointer
rhdSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
        xf86AddDriver(&RADEONHD, module, 0);
#if 0  /* @@@ */
	LoaderRefSymLists(NULL);
#endif
        /* return non-NULL even with no teardown */
  	return (pointer)1;
    } else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

static Bool
RHDGetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(RHDRec), 1);

    if (pScrn->driverPrivate == NULL)
	return FALSE;
    return TRUE;
}

static void
RHDFreeRec(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr;

    if (pScrn->driverPrivate == NULL)
	return;

    rhdPtr = RHDPTR(pScrn);

    if (rhdPtr->Options)
	xfree(rhdPtr->Options);

    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static const OptionInfoRec *
RHDAvailableOptions(int chipid, int busid)
{
    return RHDOptions;
}

static void
RHDIdentify(int flags)
{
    xf86PrintChipsets(RHD_NAME, "Driver for RadeonHD chipsets",
			RHDChipsets);
}

static Bool
RHDProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections;
    int *usedChips;
    int i;

    if ((numDevSections = xf86MatchDevice(RHD_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }

    /* PCI BUS */
    if (xf86GetPciVideoInfo() ) {
	numUsed = xf86MatchPciInstances(RHD_NAME, PCI_VENDOR_ATI,
					RHDChipsets, RHDPCIchipsets,
					devSections,numDevSections,
					drv, &usedChips);

	if (numUsed > 0) {
	    if (flags & PROBE_DETECT)
		foundScreen = TRUE;
	    else for (i = 0; i < numUsed; i++) {
		ScrnInfoPtr pScrn = NULL;
		if ((pScrn = xf86ConfigPciEntity(pScrn, 0, usedChips[i],
						       RHDPCIchipsets,NULL, NULL,
						       NULL, NULL, NULL))) {
		    pScrn->driverVersion = RHD_VERSION;
		    pScrn->driverName    = RHD_DRIVER_NAME;
		    pScrn->name          = RHD_NAME;
		    pScrn->Probe         = RHDProbe;
		    pScrn->PreInit       = RHDPreInit;
		    pScrn->ScreenInit    = RHDScreenInit;
		    pScrn->SwitchMode    = RHDSwitchMode;
		    pScrn->AdjustFrame   = RHDAdjustFrame;
		    pScrn->EnterVT       = RHDEnterVT;
		    pScrn->LeaveVT       = RHDLeaveVT;
		    pScrn->FreeScreen    = RHDFreeScreen;
		    pScrn->ValidMode     = NULL; /* we do our own validation */
		    foundScreen = TRUE;
		}
	    }
	    xfree(usedChips);
	}
    }

    xfree(devSections);
    return foundScreen;
}

Bool
RHDPreInit(ScrnInfoPtr pScrn, int flags)
{
    RHDPtr rhdPtr;
    EntityInfoPtr pEnt = NULL;
    int i;

    if (flags & PROBE_DETECT)  {
        /* do dynamic mode probing */
	return TRUE;
    }

    /* Allocate the RhdRec driverPrivate */
    if (!RHDGetRec(pScrn)) {
	return FALSE;
    }

    rhdPtr = RHDPTR(pScrn);

    /* This driver doesn't expect more than one entity per screen */
    if (pScrn->numEntities > 1) {
	RHDFreeRec(pScrn);
	return FALSE;
    }

    for (i = 0; i < pScrn->numEntities; i++) {
	pEnt = xf86GetEntityInfo(pScrn->entityList[i]);
	if (pEnt->resources) {
	    RHDFreeRec(pScrn);
	    xfree(pEnt);
	    return FALSE;
	}

	rhdPtr->RhdChipset = pEnt->chipset;
	pScrn->chipset = (char *)xf86TokenToString(RHDChipsets, pEnt->chipset);
        rhdPtr->PciInfo = xf86GetPciInfoForEntity(pEnt->index);
        rhdPtr->PciTag = pciTag(rhdPtr->PciInfo->bus,
                                rhdPtr->PciInfo->device,
                                rhdPtr->PciInfo->func);
    }

    /* TODO: dig out the previous code so that this here actually makes sense. */
    if (!pEnt) {
	RHDFreeRec(pScrn);
	return FALSE;
    }

    /* if we need to store/restore VGA registers, then we will end up doing this
       through MMIO */
    if (xf86RegisterResources(pEnt->index, NULL, ResNone)) {
	RHDFreeRec(pScrn);
	xfree(pEnt);
	return FALSE;
    }
    xfree(pEnt);

    switch(rhdPtr->RhdChipset){
        default:
        xf86ErrorF("Unknown Chipset found");
	break;
    }
    xf86ErrorF("\n");

    /* xf86CollectOptions cluelessly depends on these and
       will SIGSEGV otherwise */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrn, 16, 0, 0, Support32bppFb)) {
	RHDFreeRec(pScrn);
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 8:
	case 15:
	case 16:
	case 24:
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    RHDFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86PrintDepthBpp(pScrn);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
    rhdPtr->Options = xnfcalloc(sizeof(RHDOptions), 1);
    memcpy(rhdPtr->Options, RHDOptions, sizeof(RHDOptions));

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, rhdPtr->Options);

    rhdPtr->noAccelSet =
	xf86ReturnOptValBool(rhdPtr->Options, OPTION_NOACCEL, FALSE);
    rhdPtr->swCursor =
	xf86ReturnOptValBool(rhdPtr->Options, OPTION_SW_CURSOR, FALSE);
    rhdPtr->onPciBurst =
	xf86ReturnOptValBool(rhdPtr->Options, OPTION_PCI_BURST, TRUE);

    if (!xf86GetOptValInteger(rhdPtr->Options, OPTION_VIDEORAM, &pScrn->videoRam))
	pScrn->videoRam = 0;

    /* We have none of these things yet. */
    rhdPtr->noAccelSet = TRUE;
    rhdPtr->swCursor = TRUE;

    /* We need access to IO space already */
    if (!rhdMMIOMap(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map MMIO.\n");
	RHDFreeRec(pScrn);
	return FALSE;
    }

    /* Use MC to detect how much RAM is there.
     * For now, just use an option. */
    if (!pScrn->videoRam) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No Video RAM detected.\n");
	RHDFreeRec(pScrn);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    /* detect outputs */
    /* @@@ */

#if 0  /* @@@ rgb bits boilerplate */
    if (pScrn->depth == 8)
	pScrn->rgbBits = 6;
#endif

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    RHDFreeRec(pScrn);
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
	    ;
	}
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
        RHDFreeRec(pScrn);
	return FALSE;
    } else {
        /* We don't currently support DirectColor at > 8bpp */
        if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
                       " (%s) is not supported at depth %d\n",
                       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    RHDFreeRec(pScrn);
	    return FALSE;
        }
    }

    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

        /* @@@ */
	if (!xf86SetGamma(pScrn, zeros)) {
            RHDFreeRec(pScrn);
	    return FALSE;
	}
    }

    /* @@@ need this? */
    pScrn->progClock = TRUE;

    /* Tell X that we support at least one mode */
    /* WARNING: xf86CVTMode doesn't exist before 7.1 */
    {
	int HDisplay = 1280, VDisplay = 1024;
	DisplayModePtr Mode = xf86CVTMode(HDisplay, VDisplay, 0, FALSE, FALSE);

	pScrn->virtualX = HDisplay;
	pScrn->virtualY = VDisplay;
	pScrn->displayWidth = HDisplay; /* need some alignment value here */
	pScrn->modes = Mode;
	pScrn->currentMode = Mode;
	Mode->next = Mode;
	Mode->prev = Mode;
    }

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	RHDFreeRec(pScrn);
	return FALSE;
    }

    if (!xf86LoadSubModule(pScrn, "xaa")) {
	RHDFreeRec(pScrn);
	return FALSE;
    }

    if (!rhdPtr->swCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    RHDFreeRec(pScrn);
	    return FALSE;
	}
    }

    rhdMMIOUnmap(pScrn);

    return TRUE;
}

/* Mandatory */
static Bool
RHDScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    RHDPtr rhdPtr;
    int ret;
    VisualPtr visual;
    unsigned int racflag = 0;

    pScrn = xf86Screens[pScreen->myNum];
    rhdPtr = RHDPTR(pScrn);

    /*
     * Whack the hardware
     */

    if (!rhdMMIOMap(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map MMIO.\n");
	return FALSE;
    }

    if (!rhdFBMap(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map FB.\n");
	return FALSE;
    }

    /* save previous mode */
    rhdSave(pScrn);

    /* now init the new mode */
    if (!rhdModeInit(pScrn,pScrn->currentMode))
	return FALSE;

    /* @@@ add code to unblank the screen */

    /* fix viewport */
    RHDAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    /*
     * now init DIX
     */

    miClearVisualTypes();

    /* Setup the visuals we support. */

    if (!miSetVisualTypes(pScrn->depth,
      		      miGetDefaultVisualMask(pScrn->depth),
		      pScrn->rgbBits, pScrn->defaultVisual))
         return FALSE;

    if (!miSetPixmapDepths ()) return FALSE;

    /* @@@ do shadow stuff here */

    /* init fb */
    ret = fbScreenInit(pScreen, rhdPtr->FbBase,
		       pScrn->virtualX, pScrn->virtualY,
		       pScrn->xDpi, pScrn->yDpi,
		       pScrn->displayWidth, pScrn->bitsPerPixel);
    if (!ret)
	return FALSE;
    if (pScrn->depth > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor
		&& visual->nplanes > 8) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    /* must be after RGB ordering fixed */
    fbPictureInit(pScreen, 0, 0);

    xf86SetBlackWhitePixels(pScreen);

    /* reserve space for scanout buffer hw cursor etc.
       initialize memory manager.*/

    {
        BoxRec AvailFBArea;
        int hwcur = 0/*@@@*/;
        int lines =  (pScrn->videoRam * 1024 - hwcur) /
            (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));
        AvailFBArea.x1 = 0;
        AvailFBArea.y1 = 0;
        AvailFBArea.x2 = pScrn->displayWidth;
        AvailFBArea.y2 = lines;
        xf86InitFBManager(pScreen, &AvailFBArea);

        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Using %i scanlines of offscreen memory \n",
                   lines - pScrn->virtualY);
    }

    /* Initialize 2D accel here */

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    /* Inititalize HW cursor */
    if (!rhdPtr->swCursor) {
        if (!RHDCursorInit(pScreen)) {
            xf86DrvMsg(scrnIndex, X_ERROR,
                       "Hardware cursor initialization failed\n");
            return FALSE;
        }
    }

    /* default colormap */
    if(!miCreateDefColormap(pScreen))
	return FALSE;
    /* fixme */
    if (!xf86HandleColormaps(pScreen, 256, pScrn->rgbBits,
                         RHDLoadPalette, NULL,
                         CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH))
	return FALSE;

    pScrn->racIoFlags = pScrn->racMemFlags = racflag;

    /* @@@@ initialize video overlays here */

    /* @@@ create saveScreen() funciton */
    pScreen->SaveScreen = rhdSaveScreen;

    /* Setup DPMS mode */

    xf86DPMSInit(pScreen, (DPMSSetProcPtr)RHDDisplayPowerManagementSet,0);
    /* @@@ Attention this linear address is needed vor v4l. It needs to be
     * checked what address we need here
     */
    /* pScrn->memPhysBase = (unsigned long)rhdPtr->NeoLinearAddr; ?
       pScrn->fbOffset = 0; */

    /* Wrap the current CloseScreen function */
    rhdPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = RHDCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    return TRUE;
}

/* Mandatory */
static Bool
RHDCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if(pScrn->vtSema){
        if (!rhdPtr->swCursor)
	    rhdHideCursor(pScrn);
	rhdRestore(pScrn, &rhdPtr->savedRegs);

	rhdLock(pScrn);
	rhdFBUnmap(pScrn);
	rhdMMIOUnmap(pScrn);
    }

    /* @@@ deacllocate any data structures that are rhdPtr private here */
    if (!rhdPtr->swCursor) {
        xf86DestroyCursorInfoRec(rhdPtr->CursorInfo);
        rhdPtr->CursorInfo = NULL;
    }

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = rhdPtr->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Optional */
static void
RHDFreeScreen(int scrnIndex, int flags)
{
    RHDFreeRec(xf86Screens[scrnIndex]);
}

/* Mandatory */
static Bool
RHDEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    /* Should we re-save the text mode on each VT enter? */
    if(!rhdModeInit(pScrn, pScrn->currentMode))
      return FALSE;

    /* @@@ video overlays can be initialized here */

    if (!rhdPtr->swCursor)
	rhdShowCursor(pScrn);
    RHDAdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    return TRUE;
}

/* Mandatory */
static void
RHDLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    /* Invalidate the cached acceleration registers */
    if (!rhdPtr->swCursor)
	rhdHideCursor(pScrn);
    rhdRestore(pScrn,  &rhdPtr->savedRegs);
    rhdLock(pScrn);
}

Bool
RHDSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return rhdModeInit(xf86Screens[scrnIndex], mode);
}

/*
 * High level bit banging functions
 */

void
RHDAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    /* @@@ put viewport code here */
}

static void
RHDDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                             int PowerManagementMode,
			     int flags)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if (!pScrn->vtSema)
	return;

    switch (PowerManagementMode) {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	break;
    }
}

static void
RHDLoadPalette(
   ScrnInfoPtr pScrn,
   int numColors,
   int *indices,
   LOCO *colors,
   VisualPtr pVisual)
{
    /* @@@ load palette code */
}

static Bool
rhdSaveScreen(ScreenPtr pScrn, int on)
{
    /* put code to blacken screen here */
    return TRUE;
}

/*
 *
 */
static Bool
rhdMMIOMap(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int BAR;

    /* There are bound to be chips which use a different BAR */
    switch(rhdPtr->RhdChipset) {
    default:
	BAR = 2;
	break;
    }

    rhdPtr->MMIOMapSize = 2 << rhdPtr->PciInfo->size[BAR];
    rhdPtr->MMIOBase =
        xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO, rhdPtr->PciTag,
		      rhdPtr->PciInfo->memBase[BAR], rhdPtr->MMIOMapSize);
    if (!rhdPtr->MMIOBase)
        return FALSE;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Mapped IO at %p (size 0x%08X)\n",
	       rhdPtr->MMIOBase, rhdPtr->MMIOMapSize);

    return TRUE;
}

/*
 *
 */
static void
rhdMMIOUnmap(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)rhdPtr->MMIOBase,
                    rhdPtr->MMIOMapSize);
    rhdPtr->MMIOBase = 0;
}

/*
 *
 */
static Bool
rhdFBMap(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    int BAR;

    /* There are bound to be chips which use a different BAR */
    switch(rhdPtr->RhdChipset) {
    default:
	BAR = 0;
	break;
    }

    rhdPtr->FbMapSize = 2 << rhdPtr->PciInfo->size[BAR];
    rhdPtr->FbAddress = rhdPtr->PciInfo->memBase[BAR];
    rhdPtr->FbBase =
        xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER, rhdPtr->PciTag,
		      rhdPtr->FbAddress, rhdPtr->FbMapSize);

    if (!rhdPtr->FbBase)
        return FALSE;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Mapped FB at %p (size 0x%08X)\n",
	       rhdPtr->FbBase, rhdPtr->FbMapSize);

    return TRUE;
}

/*
 *
 */
static void
rhdFBUnmap(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)rhdPtr->FbBase,
                    rhdPtr->FbMapSize);
    rhdPtr->FbBase = 0;
}

static Bool
rhdModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    rhdUnlock(pScrn);

    pScrn->vtSema = TRUE;

    rhdSetMode(pScrn, mode);

    return(TRUE);
}

/*
 * Low level bit banging functions
 */

static void
rhdLock(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    /* @@@ any lock code to prevent register access */
}

static void
rhdUnlock(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    /* @@@ any unlock code to allow access to all regs */
}

static void
rhdSave(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    RHDRegPtr save;

    save = &(rhdPtr->savedRegs);

    save->VGA_Render_Control = RHDRegRead(rhdPtr, VGA_RENDER_CONTROL);
    save->VGA_HDP_Control = RHDRegRead(rhdPtr, VGA_HDP_CONTROL);
    save->D1VGA_Control = RHDRegRead(rhdPtr, D1VGA_CONTROL);
    save->D2VGA_Control = RHDRegRead(rhdPtr, D2VGA_CONTROL);

    /* Check whether anything to do with VGA is enabled,
       if so, store things accordingly */
    if ((save->VGA_Render_Control & 0x00030000) || /* VGA_VSTATUS_CNTL */
	!(save->VGA_HDP_Control & 0x00000010) || /* VGA_MEMORY_DISABLE */
	(save->D1VGA_Control & 0x00000001) || /* D1VGA_MODE_ENABLE */
	(save->D2VGA_Control & 0x00000001)) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "VGA mode detected. Saving appropriately.\n");

	save->IsVGA = TRUE;
	save->VGAFBOffset =
	    RHDRegRead(rhdPtr, VGA_MEMORY_BASE_ADDRESS) - rhdPtr->FbAddress;

	save->VGAFBSize = 256 * 1024;
	save->VGAFB = xcalloc(save->VGAFBSize, 1);
	if (save->VGAFB)
	    memcpy(save->VGAFB, ((CARD8 *) rhdPtr->FbBase) + save->VGAFBOffset,
		   save->VGAFBSize);
	else {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Failed to allocate space for storing the VGA framebuffer.\n");
	    save->VGAFBSize = 0;
	}
    } else
	save->IsVGA = FALSE;

    save->DACB_Source_Select = RHDRegRead(rhdPtr, DACB_SOURCE_SELECT);
    save->D2CRTC_Control = RHDRegRead(rhdPtr, D2CRTC_CONTROL);
    save->D2GRPH_Enable = RHDRegRead(rhdPtr, D2GRPH_ENABLE);

    save->D1GRPH_X_End = RHDRegRead(rhdPtr, D1GRPH_X_END);
    save->D1GRPH_Y_End = RHDRegRead(rhdPtr, D1GRPH_Y_END);
    save->D1GRPH_Primary_Surface_Address = RHDRegRead(rhdPtr, D1GRPH_PRIMARY_SURFACE_ADDRESS);
    save->D1Mode_ViewPort_Size = RHDRegRead(rhdPtr, D1MODE_VIEWPORT_SIZE);

    save->D1CRTC_H_Total = RHDRegRead(rhdPtr, D1CRTC_H_TOTAL);
    save->D1CRTC_H_Blank_Start_End = RHDRegRead(rhdPtr, D1CRTC_H_BLANK_START_END);
    save->D1CRTC_H_Sync_A = RHDRegRead(rhdPtr, D1CRTC_H_SYNC_A);
    save->D1CRTC_H_Sync_A_Cntl = RHDRegRead(rhdPtr, D1CRTC_H_SYNC_A_CNTL);
    save->D1CRTC_H_Sync_B = RHDRegRead(rhdPtr, D1CRTC_H_SYNC_B);
    save->D1CRTC_H_Sync_B_Cntl = RHDRegRead(rhdPtr, D1CRTC_H_SYNC_B_CNTL);

    save->D1CRTC_V_Total = RHDRegRead(rhdPtr, D1CRTC_V_TOTAL);
    save->D1CRTC_V_Blank_Start_End = RHDRegRead(rhdPtr, D1CRTC_V_BLANK_START_END);
    save->D1CRTC_V_Sync_A = RHDRegRead(rhdPtr, D1CRTC_V_SYNC_A);
    save->D1CRTC_V_Sync_A_Cntl = RHDRegRead(rhdPtr, D1CRTC_V_SYNC_A_CNTL);
    save->D1CRTC_V_Sync_B = RHDRegRead(rhdPtr, D1CRTC_V_SYNC_B);
    save->D1CRTC_V_Sync_B_Cntl = RHDRegRead(rhdPtr, D1CRTC_V_SYNC_B_CNTL);
}

static void
rhdRestore(ScrnInfoPtr pScrn, RHDRegPtr restore)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    RHDRegWrite(rhdPtr, D1CRTC_H_TOTAL, restore->D1CRTC_H_Total);
    RHDRegWrite(rhdPtr, D1CRTC_H_BLANK_START_END, restore->D1CRTC_H_Blank_Start_End);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_A, restore->D1CRTC_H_Sync_A);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_A_CNTL, restore->D1CRTC_H_Sync_A_Cntl);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_B, restore->D1CRTC_H_Sync_B);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_B_CNTL, restore->D1CRTC_H_Sync_B_Cntl);

    RHDRegWrite(rhdPtr, D1CRTC_V_TOTAL, restore->D1CRTC_V_Total);
    RHDRegWrite(rhdPtr, D1CRTC_V_BLANK_START_END, restore->D1CRTC_V_Blank_Start_End);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_A, restore->D1CRTC_V_Sync_A);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_A_CNTL, restore->D1CRTC_V_Sync_A_Cntl);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_B, restore->D1CRTC_V_Sync_B);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_B_CNTL, restore->D1CRTC_V_Sync_B_Cntl);

    RHDRegWrite(rhdPtr, D1MODE_VIEWPORT_SIZE, restore->D1Mode_ViewPort_Size);
    RHDRegWrite(rhdPtr, D1GRPH_PRIMARY_SURFACE_ADDRESS, restore->D1GRPH_Primary_Surface_Address);

    RHDRegWrite(rhdPtr, D1GRPH_X_END, restore->D1GRPH_X_End);
    RHDRegWrite(rhdPtr, D1GRPH_Y_END, restore->D1GRPH_Y_End);

    RHDRegWrite(rhdPtr, D2GRPH_ENABLE, restore->D2GRPH_Enable);
    RHDRegWrite(rhdPtr, D2CRTC_CONTROL, restore->D2CRTC_Control);
    RHDRegWrite(rhdPtr, DACB_SOURCE_SELECT, restore->DACB_Source_Select);

    if (restore->IsVGA) {
	if (restore->VGAFBSize)
	    memcpy(((CARD8 *) rhdPtr->FbBase) + restore->VGAFBOffset,
		   restore->VGAFB, restore->VGAFBSize);

	RHDRegWrite(rhdPtr, VGA_RENDER_CONTROL, restore->VGA_Render_Control);
	RHDRegWrite(rhdPtr, VGA_HDP_CONTROL, restore->VGA_HDP_Control);
	RHDRegWrite(rhdPtr, D1VGA_CONTROL, restore->D1VGA_Control);
	RHDRegWrite(rhdPtr, D2VGA_CONTROL, restore->D2VGA_Control);
    }
}

static void
rhdSetMode(ScrnInfoPtr pScrn, DisplayModePtr Mode)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    /* Disable all of VGA */
    RHDRegMask(rhdPtr, VGA_RENDER_CONTROL, 0, 0x00030000);
    RHDRegMask(rhdPtr, VGA_HDP_CONTROL, 0x00000010, 0x00000010);
    RHDRegMask(rhdPtr, D1VGA_CONTROL, 0, 0x00000001);
    RHDRegMask(rhdPtr, D2VGA_CONTROL, 0, 0x00000001);

    /* this rv530 its CRT connection lives on DACB,
     * Tie it to CRTC1 */
    RHDRegMask(rhdPtr, DACB_SOURCE_SELECT, 0, 0x00000003);

    /* shut down D2 for now */
    RHDRegMask(rhdPtr, D2CRTC_CONTROL, 0, 0x00000001);
    RHDRegMask(rhdPtr, D2GRPH_ENABLE, 0, 0x00000001);

    /* different ordering than documented for VIEWPORT_SIZE */
    RHDRegWrite(rhdPtr, D1MODE_VIEWPORT_SIZE, Mode->VDisplay | (Mode->HDisplay << 16));
    RHDRegWrite(rhdPtr, D1GRPH_PRIMARY_SURFACE_ADDRESS, rhdPtr->FbAddress);
    RHDRegWrite(rhdPtr, D1GRPH_X_END, Mode->HDisplay);
    RHDRegWrite(rhdPtr, D1GRPH_Y_END, Mode->VDisplay);

    /* once we can program the dotclock, we can start using this code */
#if 0
    RHDRegWrite(rhdPtr, D1CRTC_H_TOTAL, Mode->CrtcHTotal);
    RHDRegWrite(rhdPtr, D1CRTC_H_BLANK_START_END, Mode->CrtcHBlankStart | (Mode->CrtcHBlankEnd << 16));
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_A, 0);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_A_CNTL, 0);
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_B, Mode->CrtcHSyncStart | ( Mode->CrtcHSyncEnd << 16));
    RHDRegWrite(rhdPtr, D1CRTC_H_SYNC_B_CNTL, Mode->Flags & V_NHSYNC);

    RHDRegWrite(rhdPtr, D1CRTC_V_TOTAL, Mode->CrtcVTotal);
    RHDRegWrite(rhdPtr, D1CRTC_V_BLANK_START_END, Mode->CrtcVBlankStart | (Mode->CrtcVBlankEnd << 16));
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_A, 0);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_A_CNTL, 0);
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_B, Mode->CrtcVSyncStart | ( Mode->CrtcVSyncEnd << 16));
    RHDRegWrite(rhdPtr, D1CRTC_V_SYNC_B_CNTL, Mode->Flags & V_NVSYNC);
#endif
}

/*
 *
 */
CARD32
RHDRegRead(RHDPtr rhdPtr, CARD16 offset)
{
    return *(volatile CARD32 *)((CARD8 *) rhdPtr->MMIOBase + offset);
}

/*
 *
 */
void
RHDRegWrite(RHDPtr rhdPtr, CARD16 offset, CARD32 value)
{
    *(volatile CARD32 *)((CARD8 *) rhdPtr->MMIOBase + offset) = value;
}

/*
 * This one might seem clueless, but it is an actual lifesaver.
 */
void
RHDRegMask(RHDPtr rhdPtr, CARD16 offset, CARD32 value, CARD32 mask)
{
    CARD32 tmp;

    tmp = RHDRegRead(rhdPtr, offset);
    tmp &= ~mask;
    tmp |= (value & mask);
    RHDRegWrite(rhdPtr, offset, tmp);
}

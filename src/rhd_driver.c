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

#define MODULEVENDORSTRING "AMD GPG"  /* @@@ */
#include "xf86.h"
#include "xf86_OSproc.h"

/* For PIO/MMIO */
#include "compiler.h"

/* for usleep */
#include "xf86_ansic.h"

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

/* For HW cursor */
#include "xf86Cursor.h"

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
#include "rhd_cursor.h"
#include "rhd_atombios.h"
#include "rhd_output.h"
#include "rhd_pll.h"
#include "rhd_vga.h"
#include "rhd_hpd.h"
#include "rhd_crtc.h"
#include "rhd_modes.h"
#include "rhd_lut.h"

/* ??? */
#include "servermd.h"

/* Mandatory functions */
static const OptionInfoRec *	RHDAvailableOptions(int chipid, int busid);
void     RHDIdentify(int flags);
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

static void     rhdProcessOptions(ScrnInfoPtr pScrn);
static void     rhdSave(RHDPtr rhdPtr);
static void     rhdRestore(RHDPtr rhdPtr);
static Bool     rhdModeLayoutSelect(RHDPtr rhdPtr);
static void     rhdModeLayoutPrint(RHDPtr rhdPtr);
static void     rhdModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     rhdMapMMIO(RHDPtr rhdPtr);
static void     rhdUnmapMMIO(RHDPtr rhdPtr);
static Bool     rhdMapFB(RHDPtr rhdPtr);
static void     rhdUnmapFB(RHDPtr rhdPtr);
static Bool     rhdSaveScreen(ScreenPtr pScrn, int on);
static CARD32   rhdGetVideoRamSize(RHDPtr rhdPtr);

/* rhd_id.c */
extern SymTabRec RHDChipsets[];
extern PciChipsets RHDPCIchipsets[];
void RHDIdentify(int flags);
Bool RHDChipExperimental(ScrnInfoPtr pScrn);
struct rhd_card *RHDCardIdentify(ScrnInfoPtr pScrn);

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

typedef enum {
    OPTION_NOACCEL,
    OPTION_SW_CURSOR,
    OPTION_PCI_BURST,
    OPTION_EXPERIMENTAL
} RHDOpts;

static const OptionInfoRec RHDOptions[] = {
    { OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_BURST, "pciBurst",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_EXPERIMENTAL, "experimental",	OPTV_BOOLEAN,   {0}, FALSE },
    { -1,               NULL,           OPTV_NONE,	{0}, FALSE }
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

    RHDPTR(pScrn)->scrnIndex = pScrn->scrnIndex;

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

    RHDVGADestroy(rhdPtr);
    RHDPLLsDestroy(rhdPtr);
    RHDLUTsDestroy(rhdPtr);
    RHDOutputsDestroy(rhdPtr);
    RHDHPDDestroy(rhdPtr);

    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static const OptionInfoRec *
RHDAvailableOptions(int chipid, int busid)
{
    return RHDOptions;
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

/*
 *
 */
static Bool
RHDPreInit(ScrnInfoPtr pScrn, int flags)
{
    RHDPtr rhdPtr;
    EntityInfoPtr pEnt = NULL;
    pointer biosHandle = NULL;
    Bool ret = FALSE;
    AtomBIOSArg arg;
    RHDOpt tmpOpt;
    DisplayModePtr Modes;

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
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Driver doesn't support more than one entity per screen\n");
	goto error0;
    }

    if (!(pEnt = xf86GetEntityInfo(pScrn->entityList[0]))) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Unable to get entity info\n");
	goto error0;
    }
    if (pEnt->resources) {
        xfree(pEnt);
	goto error0;
    }

    rhdPtr->ChipSet = pEnt->chipset;
    pScrn->chipset = (char *)xf86TokenToString(RHDChipsets, pEnt->chipset);

    pScrn->videoRam = pEnt->device->videoRam;

    rhdPtr->PciInfo = xf86GetPciInfoForEntity(pEnt->index);
    rhdPtr->PciTag = pciTag(rhdPtr->PciInfo->bus,
                            rhdPtr->PciInfo->device,
                            rhdPtr->PciInfo->func);
    rhdPtr->entityIndex = pEnt->index;

    /* We will disable access to VGA legacy resources emulation and
       save/restore VGA thru MMIO when necessary */
    if (xf86RegisterResources(pEnt->index, NULL, ResNone)) {
	xfree(pEnt);
	goto error0;
    }
    xfree(pEnt);

    /* xf86CollectOptions cluelessly depends on these and
       will SIGSEGV otherwise */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrn, 16, 0, 0, Support32bppFb)) {
	goto error0;
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
	    goto error0;
	}
    }
    xf86PrintDepthBpp(pScrn);

    rhdProcessOptions(pScrn);

    /* Check whether we should accept this hardware already */
    if (RHDChipExperimental(pScrn)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "This hardware is marked as EXPERIMENTAL.\n\t"
		   "It could be harmful to try to use this driver on this card.\n\t"
		   "To help rectify this, please send your X log to MAILINGLIST.\n");

	RhdGetOptValBool(rhdPtr->Options, OPTION_EXPERIMENTAL, &tmpOpt, FALSE);
	if (!tmpOpt.val.bool)
	    goto error0;
    }

    /* Now check whether we know this card */
    rhdPtr->Card = RHDCardIdentify(pScrn);
    if (rhdPtr->Card)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected an %s on a %s\n",
		   pScrn->chipset, rhdPtr->Card->name);
    else
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected an %s on an "
		   "unidentified card\n", pScrn->chipset);

    /* We have none of these things yet. */
    rhdPtr->noAccel.val.bool = TRUE;

    /* We need access to IO space already */
    if (!rhdMapMMIO(rhdPtr)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map MMIO.\n");
	goto error0;
    }

    /* We can use a register which is programmed by the BIOS to find otu the
       size of our framebuffer */
    if (!pScrn->videoRam) {
	pScrn->videoRam = rhdGetVideoRamSize(rhdPtr);
	if (!pScrn->videoRam) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No Video RAM detected.\n");
	    goto error1;
	}
    }

    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    if (RhdAtomBIOSFunc(pScrn, NULL, ATOMBIOS_INIT, &arg) == SUCCESS) {
	biosHandle = arg.ptr;
	/* for testing functions */
	RhdAtomBIOSFunc(pScrn, biosHandle, GET_MAX_PLL_CLOCK, &arg);
	RhdAtomBIOSFunc(pScrn, biosHandle, GET_MIN_PLL_CLOCK, &arg);
	RhdAtomBIOSFunc(pScrn, biosHandle, GET_MAX_PIXEL_CLK, &arg);
	RhdAtomBIOSFunc(pScrn, biosHandle, GET_REF_CLOCK, &arg);
    }

    /* Init modesetting structures */
    RHDVGAInit(rhdPtr);

    RHDCRTCInit(rhdPtr);
    RHDPLLsInit(rhdPtr);
    RHDLUTsInit(rhdPtr);

    /* Output handling needs to wrap itself in HPD handling */
    RHDHPDInit(rhdPtr);
    RHDHPDSave(rhdPtr);
    RHDHPDSet(rhdPtr);

    RHDOutputsInit(rhdPtr);

    RHDHPDRestore(rhdPtr);

    /* Pick anything for now */
    if (!rhdModeLayoutSelect(rhdPtr)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to detect a connected monitor\n");
	goto error2;
    }
    rhdModeLayoutPrint(rhdPtr);

    /* @@@ rgb bits boilerplate */
    if (pScrn->depth == 8)
	pScrn->rgbBits = 8;

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
	    goto error2;
	} else {
	    /* XXX check that weight returned is supported */
	    ;
	}
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	goto error2;
    } else {
        /* We don't currently support DirectColor at > 8bpp */
        if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
                       " (%s) is not supported at depth %d\n",
                       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    goto error2;
        }
    }

    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

        /* @@@ */
	if (!xf86SetGamma(pScrn, zeros)) {
	    goto error2;
	}
    }

    /* @@@ need this? */
    pScrn->progClock = TRUE;

    if (pScrn->display->virtualX && pScrn->display->virtualY)
        if (!RHDGetVirtualFromConfig(pScrn)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Unable to find valid framebuffer dimensions\n");
	    goto error2;
	}

    Modes = RHDModesPoolCreate(pScrn, FALSE);
    if (!Modes) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	goto error2;
    }

    if (!pScrn->virtualX || !pScrn->virtualY)
        RHDGetVirtualFromModesAndFilter(pScrn, Modes, FALSE);

    RHDModesAttach(pScrn, Modes);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
               "Using %dx%d Framebuffer with %d pitch\n", pScrn->virtualX,
               pScrn->virtualY, pScrn->displayWidth);
    xf86PrintModes(pScrn);

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	goto error2;
    }

    if (!xf86LoadSubModule(pScrn, "xaa")) {
	goto error2;
    }

    if (!rhdPtr->swCursor.val.bool) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    goto error2;
	}
    }
    ret = TRUE;

 error2:
    RhdAtomBIOSFunc(pScrn, biosHandle, ATOMBIOS_UNINIT, NULL);
 error1:
    rhdUnmapMMIO(rhdPtr);
 error0:
    if (!ret)
	RHDFreeRec(pScrn);

    return ret;
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

    if (!rhdMapMMIO(rhdPtr)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map MMIO.\n");
	return FALSE;
    }

    if (!rhdMapFB(rhdPtr)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to map FB.\n");
	return FALSE;
    }

    /* save previous mode */
    rhdSave(rhdPtr);

    /* now init the new mode */
    rhdModeInit(pScrn, pScrn->currentMode);

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
    if (!rhdPtr->swCursor.val.bool)
        if (!RHDCursorInit(pScreen))
            xf86DrvMsg(scrnIndex, X_ERROR,
                       "Hardware cursor initialization failed\n");

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
        if (rhdPtr->CursorInfo)
	    rhdHideCursor(pScrn);
	rhdRestore(rhdPtr);

	rhdUnmapFB(rhdPtr);
	rhdUnmapMMIO(rhdPtr);
    }

    /* @@@ deacllocate any data structures that are rhdPtr private here */
    if (rhdPtr->CursorInfo) {
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

    rhdSave(rhdPtr);

    rhdModeInit(pScrn, pScrn->currentMode);

    /* @@@ video overlays can be initialized here */

    if (rhdPtr->CursorInfo)
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
    if (rhdPtr->CursorInfo)
	rhdHideCursor(pScrn);
    rhdRestore(rhdPtr);
}

static Bool
RHDSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    rhdModeInit(xf86Screens[scrnIndex], mode);

    return TRUE;
}

/*
 * High level bit banging functions
 */

static void
RHDAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhd_Crtc *Crtc;

    rhdPtr->FrameX = x;
    rhdPtr->FrameY = x;

    Crtc = rhdPtr->Crtc[0];
    if ((Crtc->scrnIndex == scrnIndex) && Crtc->Active)
	Crtc->FrameSet(Crtc, x, y);

    Crtc = rhdPtr->Crtc[1];
    if ((Crtc->scrnIndex == scrnIndex) && Crtc->Active)
	Crtc->FrameSet(Crtc, x, y);
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
	RHDOutputsPower(rhdPtr, RHD_POWER_ON);
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	RHDOutputsPower(rhdPtr, RHD_POWER_RESET);
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	RHDOutputsPower(rhdPtr, RHD_POWER_RESET);
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	RHDOutputsPower(rhdPtr, RHD_POWER_RESET);
	break;
    }
}

/*
 *
 */
static void
RHDLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
	       VisualPtr pVisual)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhd_Crtc *Crtc;

    Crtc = rhdPtr->Crtc[0];
    if ((pScrn->scrnIndex == Crtc->scrnIndex) && Crtc->Active)
	Crtc->LUT->Set(Crtc->LUT, numColors, indices, colors);

    Crtc = rhdPtr->Crtc[1];
    if ((pScrn->scrnIndex == Crtc->scrnIndex) && Crtc->Active)
	Crtc->LUT->Set(Crtc->LUT, numColors, indices, colors);
}

/*
 *
 */
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
rhdMapMMIO(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    rhdPtr->MMIOMapSize = 1 << rhdPtr->PciInfo->size[RHD_MMIO_BAR];
    rhdPtr->MMIOBase =
        xf86MapPciMem(rhdPtr->scrnIndex, VIDMEM_MMIO, rhdPtr->PciTag,
		      rhdPtr->PciInfo->memBase[RHD_MMIO_BAR], rhdPtr->MMIOMapSize);
    if (!rhdPtr->MMIOBase)
        return FALSE;

    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Mapped IO at %p (size 0x%08X)\n",
	       rhdPtr->MMIOBase, rhdPtr->MMIOMapSize);

    return TRUE;
}

/*
 *
 */
static void
rhdUnmapMMIO(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    xf86UnMapVidMem(rhdPtr->scrnIndex, (pointer)rhdPtr->MMIOBase,
                    rhdPtr->MMIOMapSize);
    rhdPtr->MMIOBase = 0;
}

/*
 *
 */
static CARD32
rhdGetVideoRamSize(RHDPtr rhdPtr)
{
    CARD32 RamSize, BARSize;

    RHDFUNC(rhdPtr);

    if (rhdPtr->ChipSet < RHD_R600)
	RamSize = (RHDRegRead(rhdPtr, R5XX_CONFIG_MEMSIZE)) >> 10;
    else
	RamSize = (RHDRegRead(rhdPtr, R6XX_CONFIG_MEMSIZE)) >> 10;
    BARSize = 1 << (rhdPtr->PciInfo->size[RHD_FB_BAR] - 10);

    if (RamSize > BARSize) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "The detected amount of videoram"
		   " exceeds the PCI BAR aperture.\n");
	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Using only %dkB of the total "
		   "%dkB.\n", (int) BARSize, (int) RamSize);
	return BARSize;
    } else
	return RamSize;
}

/*
 *
 */
static Bool
rhdMapFB(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    rhdPtr->FbMapSize = 1 << rhdPtr->PciInfo->size[RHD_FB_BAR];
    rhdPtr->FbBase =
        xf86MapPciMem(rhdPtr->scrnIndex, VIDMEM_FRAMEBUFFER, rhdPtr->PciTag,
		      rhdPtr->PciInfo->memBase[RHD_FB_BAR], rhdPtr->FbMapSize);

    if (!rhdPtr->FbBase)
        return FALSE;

    /* These devices have an internal address reference, which some other
     * address registers in there also use. This can be different from the
     * address in the BAR */
    if (rhdPtr->ChipSet < RHD_R600)
	rhdPtr->FbIntAddress = RHDRegRead(rhdPtr, R5XX_FB_INTERNAL_ADDRESS) << 16;
    else
	rhdPtr->FbIntAddress = RHDRegRead(rhdPtr, R6XX_CONFIG_FB_BASE);

    if (rhdPtr->FbIntAddress != rhdPtr->PciInfo->memBase[RHD_FB_BAR])
	    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "PCI FB Address (BAR) is at "
		       "0x%08X while card Internal Address is 0x%08X\n",
		       (unsigned int) rhdPtr->PciInfo->memBase[RHD_FB_BAR],
		       rhdPtr->FbIntAddress);

    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Mapped FB at %p (size 0x%08X)\n",
	       rhdPtr->FbBase, rhdPtr->FbMapSize);

    return TRUE;
}

/*
 *
 */
static void
rhdUnmapFB(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    xf86UnMapVidMem(rhdPtr->scrnIndex, (pointer)rhdPtr->FbBase,
                    rhdPtr->FbMapSize);
    rhdPtr->FbBase = 0;
}

/*
 *
 */
static Bool
rhdModeLayoutSelect(RHDPtr rhdPtr)
{
    struct rhd_Output *Output;
    Bool ret = FALSE;
    int i = 0;

    RHDFUNC(rhdPtr);

    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	if (Output->Sense && !Output->Sense(Output))
	    Output->Active = FALSE;
	else {
	    Output->Active = TRUE;

	    ret = TRUE;

	    Output->Crtc = i & 1; /* ;) */
	    i++;

	    rhdPtr->Crtc[Output->Crtc]->Active = TRUE;
	}

    rhdPtr->Crtc[0]->PLL = rhdPtr->PLLs[0];
    rhdPtr->Crtc[0]->LUT = rhdPtr->LUT[0];

    rhdPtr->Crtc[1]->PLL = rhdPtr->PLLs[1];
    rhdPtr->Crtc[1]->LUT = rhdPtr->LUT[1];

    return ret;
}

/*
 *
 */
static void
rhdModeLayoutPrint(RHDPtr rhdPtr)
{
    struct rhd_Crtc *Crtc;
    struct rhd_Output *Output;
    Bool Found;

    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Listing modesetting layout:\n\n");

    /* CRTC 1 */
    Crtc = rhdPtr->Crtc[0];
    if (Crtc->Active) {
	xf86Msg(X_NONE, "\t%s: tied to %s and %s:\n",
		Crtc->Name, Crtc->PLL->Name, Crtc->LUT->Name);

	Found = FALSE;
	for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	    if (Output->Active && (Output->Crtc == Crtc->Id)) {
		if (!Found) {
		    xf86Msg(X_NONE, "\t\tOutputs: %s", Output->Name);
		    Found = TRUE;
		} else
		    xf86Msg(X_NONE, ", %s", Output->Name);
	    }

	if (!Found)
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		       "%s is active without outputs\n", Crtc->Name);
	else
	     xf86Msg(X_NONE, "\n");
    } else
	xf86Msg(X_NONE, "\t%s: unused\n", Crtc->Name);
    xf86Msg(X_NONE, "\n");

    /* CRTC 2 */
    Crtc = rhdPtr->Crtc[1];
    if (Crtc->Active) {
	xf86Msg(X_NONE, "\t%s: tied to %s and %s:\n",
		Crtc->Name, Crtc->PLL->Name, Crtc->LUT->Name);

	Found = FALSE;
	for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	    if (Output->Active && (Output->Crtc == Crtc->Id)) {
		if (!Found) {
		    xf86Msg(X_NONE, "\t\tOutputs: %s", Output->Name);
		    Found = TRUE;
		} else
		    xf86Msg(X_NONE, ", %s", Output->Name);
	    }

	if (!Found)
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR,
		       "%s is active without outputs\n", Crtc->Name);
	else
	    xf86Msg(X_NONE, "\n");
    } else
	xf86Msg(X_NONE, "\t%s: unused\n", Crtc->Name);
    xf86Msg(X_NONE, "\n");

    /* Print out unused Outputs */
    Found = FALSE;
    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	if (!Output->Active) {
	    if (!Found) {
		xf86Msg(X_NONE, "\tUnused Outputs: %s", Output->Name);
		Found = TRUE;
	    } else
		xf86Msg(X_NONE, ", %s", Output->Name);
	}

    if (Found)
	xf86Msg(X_NONE, "\n");
    xf86Msg(X_NONE, "\n");
}


/*
 *
 */
static void
rhdModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhd_Crtc *Crtc;

    RHDFUNC(rhdPtr);

    pScrn->vtSema = TRUE;

    /* no active outputs == no mess */
    RHDOutputsPower(rhdPtr, RHD_POWER_RESET);

    /* Disable CRTCs to stop noise from appearing. */
    rhdPtr->Crtc[0]->Power(rhdPtr->Crtc[0], RHD_POWER_RESET);
    rhdPtr->Crtc[1]->Power(rhdPtr->Crtc[1], RHD_POWER_RESET);

    /* now disable our VGA Mode */
    RHDVGADisable(rhdPtr);

    /* Set up D1 and appendages */
    Crtc = rhdPtr->Crtc[0];
    if (Crtc->Active) {
	Crtc->FBSet(Crtc, pScrn->displayWidth, pScrn->virtualX, pScrn->virtualY,
		    pScrn->depth, OFFSET_RESERVED);
	Crtc->ModeSet(Crtc, mode);
	RHDPLLSet(Crtc->PLL, mode->Clock);
	Crtc->PLLSelect(Crtc, Crtc->PLL);
	Crtc->LUTSelect(Crtc, Crtc->LUT);
	RHDOutputsMode(rhdPtr, Crtc->Id);
    }

    /* Set up D2 and appendages */
    Crtc = rhdPtr->Crtc[1];
    if (Crtc->Active) {
	Crtc->FBSet(Crtc, pScrn->displayWidth, pScrn->virtualX, pScrn->virtualY,
		    pScrn->depth, OFFSET_RESERVED);
	Crtc->ModeSet(Crtc, mode);
	RHDPLLSet(Crtc->PLL, mode->Clock);
	Crtc->PLLSelect(Crtc, Crtc->PLL);
	Crtc->LUTSelect(Crtc, Crtc->LUT);
	RHDOutputsMode(rhdPtr, Crtc->Id);
    }

    /* shut down that what we don't use */
    RHDPLLsShutdownInactive(rhdPtr);
    RHDOutputsShutdownInactive(rhdPtr);

    if (rhdPtr->Crtc[0]->Active)
	rhdPtr->Crtc[0]->Power(rhdPtr->Crtc[0], RHD_POWER_ON);
    else
	rhdPtr->Crtc[0]->Power(rhdPtr->Crtc[0], RHD_POWER_SHUTDOWN);

    if (rhdPtr->Crtc[1]->Active)
	rhdPtr->Crtc[1]->Power(rhdPtr->Crtc[1], RHD_POWER_ON);
    else
	rhdPtr->Crtc[1]->Power(rhdPtr->Crtc[1], RHD_POWER_SHUTDOWN);

    RHDOutputsPower(rhdPtr, RHD_POWER_ON);
}

/*
 *
 */
static void
rhdSave(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    RHDVGASave(rhdPtr);

    RHDOutputsSave(rhdPtr);

    RHDPLLsSave(rhdPtr);
    RHDLUTsSave(rhdPtr);

    rhdPtr->Crtc[0]->Save(rhdPtr->Crtc[0]);
    rhdPtr->Crtc[1]->Save(rhdPtr->Crtc[1]);
}

/*
 *
 */
static void
rhdRestore(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);

    RHDPLLsRestore(rhdPtr);
    RHDLUTsRestore(rhdPtr);

    rhdPtr->Crtc[0]->Restore(rhdPtr->Crtc[0]);
    rhdPtr->Crtc[1]->Restore(rhdPtr->Crtc[1]);

    RHDVGARestore(rhdPtr);

    RHDOutputsRestore(rhdPtr);
}

/*
 *
 */
CARD32
_RHDRegRead(int scrnIndex, CARD16 offset)
{
    return *(volatile CARD32 *)((CARD8 *) RHDPTR(xf86Screens[scrnIndex])->MMIOBase + offset);
}

/*
 *
 */
void
_RHDRegWrite(int scrnIndex, CARD16 offset, CARD32 value)
{
    *(volatile CARD32 *)((CARD8 *) RHDPTR(xf86Screens[scrnIndex])->MMIOBase + offset) = value;
}

/*
 * This one might seem clueless, but it is an actual lifesaver.
 */
void
_RHDRegMask(int scrnIndex, CARD16 offset, CARD32 value, CARD32 mask)
{
    CARD32 tmp;

    tmp = _RHDRegRead(scrnIndex, offset);
    tmp &= ~mask;
    tmp |= (value & mask);
    _RHDRegWrite(scrnIndex, offset, tmp);
}

/*
 * X should have something like this itself.
 * Also used by the RHDFUNC macro.
 */
void
RHDDebug(int scrnIndex, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, X_INFO, LOG_DEBUG, format, ap);
    va_end(ap);
}

/*
 * breakout functions
 */
static void
rhdProcessOptions(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
    rhdPtr->Options = xnfcalloc(sizeof(RHDOptions), 1);
    memcpy(rhdPtr->Options, RHDOptions, sizeof(RHDOptions));

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, rhdPtr->Options);

    RhdGetOptValBool(rhdPtr->Options, OPTION_NOACCEL, &rhdPtr->noAccel,
		     FALSE);
    RhdGetOptValBool(rhdPtr->Options, OPTION_SW_CURSOR, &rhdPtr->swCursor,
		     FALSE);
    RhdGetOptValBool(rhdPtr->Options, OPTION_PCI_BURST, &rhdPtr->onPciBurst,
		     FALSE);
}


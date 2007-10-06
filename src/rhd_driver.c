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

#ifdef ATOM_BIOS_PARSER
# define ATOM_ASIC_INIT
#endif
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
#include "rhd_connector.h"
#include "rhd_monitor.h"
#include "rhd_crtc.h"
#include "rhd_modes.h"
#include "rhd_lut.h"
#include "rhd_i2c.h"
#include "rhd_shadow.h"
#include "rhd_card.h"

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
static Bool     rhdModeLayoutSelect(RHDPtr rhdPtr, char *ignore);
static void     rhdModeLayoutPrint(RHDPtr rhdPtr);
static void     rhdModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     rhdMapMMIO(RHDPtr rhdPtr);
static void     rhdUnmapMMIO(RHDPtr rhdPtr);
static Bool     rhdMapFB(RHDPtr rhdPtr);
static void     rhdUnmapFB(RHDPtr rhdPtr);
static Bool     rhdSaveScreen(ScreenPtr pScrn, int on);
static CARD32   rhdGetVideoRamSize(RHDPtr rhdPtr);

static void rhdTestDDC(ScrnInfoPtr pScrn);

/* rhd_id.c */
extern SymTabRec RHDChipsets[];
extern PciChipsets RHDPCIchipsets[];
void RHDIdentify(int flags);
struct rhdCard *RHDCardIdentify(ScrnInfoPtr pScrn);

/* keep accross drivers */
static int pix24bpp = 0;

/* required for older X.Org releases */
#ifndef _X_EXPORT
#define _X_EXPORT
#endif

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
    OPTION_SHADOWFB,
    OPTION_PROBE_I2C,
    OPTION_IGNORECONNECTOR
} RHDOpts;

static const OptionInfoRec RHDOptions[] = {
    { OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PCI_BURST, "pciBurst",	OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_SHADOWFB, "shadowfb", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_PROBE_I2C, "probe_i2c",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_IGNORECONNECTOR, "ignoreconnector", OPTV_ANYSTR, {0}, FALSE },
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
    RHDConnectorsDestroy(rhdPtr);
    RHDCursorsDestroy(rhdPtr);
    RHDCrtcsDestroy(rhdPtr);
    RHDI2CFunc(pScrn->scrnIndex, rhdPtr->I2C, RHD_I2C_TEARDOWN, NULL);
#ifdef ATOM_BIOS
    RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
		    ATOMBIOS_TEARDOWN, NULL);
#endif
    RHDShadowDestroy(rhdPtr);

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
    Bool ret = FALSE;
    RHDI2CDataArg i2cArg;

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
#ifndef  ATOM_ASIC_INIT
    if (xf86LoadSubModule(pScrn, "int10")) {
	xf86Int10InfoPtr Int10;
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Initializing INT10\n");
	if ((Int10 = xf86InitInt10(rhdPtr->entityIndex))) {
	    /*
	     * here we kludge to get a copy of V_BIOS for
	     * the AtomBIOS code. After POSTing a PCI BIOS
	     * is not accessible any more. On a non-primary
	     * card it's lost after we do xf86FreeInt10(),
	     * so we save it here before we kill int10.
	     * This still begs the question what to do
	     * on a non-primary card that has been POSTed
	     * by an earlier Xserver start.
	     */
	    if ((rhdPtr->BIOSCopy = xalloc(RHD_VBIOS_SIZE))) {
		(void)memcpy(rhdPtr->BIOSCopy,
			     xf86int10Addr(Int10, Int10->BIOSseg << 4),
			     RHD_VBIOS_SIZE);
	    }
	    xf86FreeInt10(Int10);
	}
    }
#endif
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

#ifdef ATOM_BIOS
    {
	AtomBIOSArg atomBiosArg;

	if (RHDAtomBIOSFunc(pScrn->scrnIndex, NULL, ATOMBIOS_INIT, &atomBiosArg)
	    == ATOM_SUCCESS) {
	    rhdPtr->atomBIOS = atomBiosArg.ptr;
	}
    }
#else
    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "**************************************************\n");
    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "** Code has been built without AtomBIOS support **\n");
    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "** this may seriously affect the functionality ***\n");
    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "**              of this driver                 ***\n");
    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "**************************************************\n");
#endif

    /* We can use a register which is programmed by the BIOS to find out the
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

    rhdPtr->FbFreeStart = 0;
    rhdPtr->FbFreeSize = pScrn->videoRam * 1024;

#ifdef ATOM_BIOS
    if (rhdPtr->atomBIOS) { 	/* for testing functions */

        AtomBIOSArg atomBiosArg;

        atomBiosArg.fb.start = rhdPtr->FbFreeStart;
        atomBiosArg.fb.size = rhdPtr->FbFreeSize;
        if (RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS, ATOMBIOS_ALLOCATE_FB_SCRATCH,
			  &atomBiosArg) == ATOM_SUCCESS) {
	    rhdPtr->FbFreeStart = atomBiosArg.fb.start;
	    rhdPtr->FbFreeSize = atomBiosArg.fb.size;
	}
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS, GET_DEFAULT_ENGINE_CLOCK,
			&atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS, GET_DEFAULT_MEMORY_CLOCK,
			&atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			GET_MAX_PIXEL_CLOCK_PLL_OUTPUT, &atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			GET_MIN_PIXEL_CLOCK_PLL_OUTPUT, &atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			GET_MAX_PIXEL_CLOCK_PLL_INPUT, &atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			    GET_MIN_PIXEL_CLOCK_PLL_INPUT, &atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			    GET_MAX_PIXEL_CLK, &atomBiosArg);
	RHDAtomBIOSFunc(pScrn->scrnIndex, rhdPtr->atomBIOS,
			GET_REF_CLOCK, &atomBiosArg);
#ifdef ATOM_BIOS_PARSER
	rhdTestAtomBIOS(rhdPtr->atomBIOS);
#endif
    }
#endif

    if (xf86LoadSubModule(pScrn, "i2c")) {
	if (RHDI2CFunc(pScrn->scrnIndex, NULL, RHD_I2C_INIT, &i2cArg) == RHD_I2C_SUCCESS) {
	    rhdPtr->I2C = i2cArg.I2CBusList;
	    RhdGetOptValBool(rhdPtr->Options, OPTION_PROBE_I2C, &tmpOpt,
			     FALSE);
	    if (tmpOpt.val.bool) {
		RHDI2CDataArg data;
		int line = 0;
		data.scanbus.line = line++;
		while (RHDI2CFunc(pScrn->scrnIndex,
				  rhdPtr->I2C, RHD_I2C_SCANBUS, &data) != RHD_I2C_NOLINE) {
		    int i,j;
		    for (i = 0; i < ((rhdPtr->ChipSet < RHD_R600) ? 3 : 4); i++) {
			for (j = 0; j < 32; j++) {
			    if (data.scanbus.slaves[i] & (1 << j))
				xf86DrvMsg(pScrn->scrnIndex, X_INFO, "I2C Bus at slave address "
					   "%x found on line %i\n",(i * 32 + j) << 1, data.scanbus.line);
			}
		    }
		    data.scanbus.line = line++;
		}
	    }
	    if (xf86LoadSubModule(pScrn, "ddc")) {
		rhdTestDDC(pScrn);
	    } else
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s: Failed to load DDC module\n",__func__);
	} else
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "I2C init failed\n");
    } else
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s: Failed to load I2C module\n",__func__);

    /* Init modesetting structures */
    RHDVGAInit(rhdPtr);

    RHDCrtcsInit(rhdPtr);
    RHDPLLsInit(rhdPtr);
    RHDLUTsInit(rhdPtr);
    RHDCursorsInit(rhdPtr); /* do this irrespective of hw/sw cursor setting */

    if (!RHDConnectorsInit(rhdPtr, rhdPtr->Card)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Card information has invalid connector information\n");
	goto error1;
    }

    {
	char *ignore;

	ignore = xf86GetOptValString(rhdPtr->Options, OPTION_IGNORECONNECTOR);

	/* Pick anything for now */
	if (!rhdModeLayoutSelect(rhdPtr, ignore)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Failed to detect a connected monitor\n");
	    goto error1;
	}
    }

    rhdPtr->ConfigMonitor = RHDConfigMonitor(pScrn->confScreen->monitor);
    if (!rhdPtr->ConfigMonitor) {
	int i;

	for (i = 0; i < RHD_CONNECTORS_MAX; i++)
	    if (rhdPtr->Connector[i] && rhdPtr->Connector[i]->Monitor)
		break;

	if (i == RHD_CONNECTORS_MAX) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "No monitor information found. Attaching default.\n");
	    rhdPtr->ConfigMonitor = RHDDefaultMonitor(pScrn->scrnIndex);
	}
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
	    goto error1;
	} else {
	    /* XXX check that weight returned is supported */
	    ;
	}
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	goto error1;
    } else {
        /* We don't currently support DirectColor at > 8bpp */
        if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
                       " (%s) is not supported at depth %d\n",
                       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    goto error1;
        }
    }

    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

        /* @@@ */
	if (!xf86SetGamma(pScrn, zeros)) {
	    goto error1;
	}
    }

    /* @@@ need this? */
    pScrn->progClock = TRUE;

    if (pScrn->display->virtualX && pScrn->display->virtualY)
        if (!RHDGetVirtualFromConfig(pScrn)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Unable to find valid framebuffer dimensions\n");
	    goto error1;
	}

    Modes = RHDModesPoolCreate(pScrn, FALSE);
    if (!Modes) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	goto error1;
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
	goto error1;
    }

    if (!xf86LoadSubModule(pScrn, "xaa")) {
	goto error1;
    }

    if (!rhdPtr->swCursor.val.bool) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    goto error1;
	}
    }

    RHDShadowPreInit(pScrn);

    ret = TRUE;

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
    RHDFUNC(pScrn);

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
// FIXME: necessary? is EnterVT called?
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
    ret = RHDShadowScreenInit(pScreen);
    if (!ret)
	ret = fbScreenInit(pScreen,
			   (CARD8 *) rhdPtr->FbBase + rhdPtr->FbFreeStart,
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

    if (!RHDShadowSetup(pScreen))
	return FALSE;

    xf86SetBlackWhitePixels(pScreen);

    /* initialize memory manager.*/
    {
        BoxRec AvailFBArea;

        AvailFBArea.x1 = 0;
        AvailFBArea.y1 = 0;
        AvailFBArea.x2 = pScrn->displayWidth;
        AvailFBArea.y2 = rhdPtr->FbFreeSize /
	    (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));

        xf86InitFBManager(pScreen, &AvailFBArea);

        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Using %i scanlines of offscreen memory \n",
                   AvailFBArea.y2 - pScrn->virtualY);
    }

    /* Initialize 2D accel here */

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    /* Inititalize HW cursor */
    if (!rhdPtr->swCursor.val.bool)
        if (!RHDxf86InitCursor(pScreen))
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

    if(pScrn->vtSema)
	rhdRestore(rhdPtr);

    RHDShadowCloseScreen(pScreen);
    rhdUnmapFB(rhdPtr);
    rhdUnmapMMIO(rhdPtr);

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
	rhdReloadCursor(pScrn);
    /* rhdShowCursor() done by AdjustFrame */
    RHDAdjustFrame(pScrn->scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    return TRUE;
}

/* Mandatory */
static void
RHDLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    RHDPtr rhdPtr = RHDPTR(pScrn);

    /* TODO: Invalidate the cached acceleration registers */
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
    struct rhdCrtc *Crtc;

    Crtc = rhdPtr->Crtc[0];
    if ((Crtc->scrnIndex == scrnIndex) && Crtc->Active)
	Crtc->FrameSet(Crtc, x, y);

    Crtc = rhdPtr->Crtc[1];
    if ((Crtc->scrnIndex == scrnIndex) && Crtc->Active)
	Crtc->FrameSet(Crtc, x, y);

    if (rhdPtr->CursorInfo)
	rhdShowCursor(pScrn);
}

static void
RHDDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                             int PowerManagementMode,
			     int flags)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct rhdOutput *Output;
    struct rhdCrtc *Crtc1, *Crtc2;

    if (!pScrn->vtSema)
	return;

    Crtc1 = rhdPtr->Crtc[0];
    Crtc2 = rhdPtr->Crtc[1];

    switch (PowerManagementMode) {
    case DPMSModeOn:
	if (Crtc1->Active) {
	    Crtc1->Power(Crtc1, RHD_POWER_ON);

	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Power && Output->Active && (Output->Crtc == Crtc1))
		    Output->Power(Output, RHD_POWER_ON);
	}

	if (Crtc2->Active) {
	    Crtc2->Power(Crtc1, RHD_POWER_ON);

	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Power && Output->Active && (Output->Crtc == Crtc2))
		    Output->Power(Output, RHD_POWER_ON);
	}
	break;
    case DPMSModeStandby:
    case DPMSModeSuspend:
    case DPMSModeOff:
	if (Crtc1->Active) {
	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Power && Output->Active && (Output->Crtc == Crtc1))
		    Output->Power(Output, RHD_POWER_RESET);
	    Crtc1->Power(Crtc1, RHD_POWER_RESET);
	}

	if (Crtc2->Active) {
	    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
		if (Output->Power && Output->Active && (Output->Crtc == Crtc2))
		    Output->Power(Output, RHD_POWER_RESET);
	    Crtc2->Power(Crtc2, RHD_POWER_RESET);
	}
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
    struct rhdCrtc *Crtc;

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

    if (!rhdPtr->FbBase)
	return;

    xf86UnMapVidMem(rhdPtr->scrnIndex, (pointer)rhdPtr->FbBase,
                    rhdPtr->FbMapSize);
    rhdPtr->FbBase = 0;
}

/*
 *
 */
static Bool
rhdModeLayoutSelect(RHDPtr rhdPtr, char *ignore)
{
    struct rhdOutput *Output;
    struct rhdConnector *Connector;
    Bool Found = FALSE;
    int i = 0, j;

    RHDFUNC(rhdPtr);

    /* housekeeping */
    rhdPtr->Crtc[0]->PLL = rhdPtr->PLLs[0];
    rhdPtr->Crtc[0]->LUT = rhdPtr->LUT[0];

    rhdPtr->Crtc[1]->PLL = rhdPtr->PLLs[1];
    rhdPtr->Crtc[1]->LUT = rhdPtr->LUT[1];

    /* start layout afresh */
    for (Output = rhdPtr->Outputs; Output; Output = Output->Next) {
	Output->Active = FALSE;
	Output->Crtc = NULL;
	Output->Connector = NULL;
    }

    /* Check on the basis of Connector->HPD */
    for (i = 0; i < RHD_CONNECTORS_MAX; i++) {
	Connector = rhdPtr->Connector[i];

	if (!Connector)
	    continue;

	if (ignore && !strcasecmp(Connector->Name, ignore)) {
	    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
		       "Skipping connector \"%s\"\n", ignore);
	    continue;
	}

	if (Connector->HPDCheck) {
	    if (Connector->HPDCheck(Connector)) {
		Connector->HPDAttached = TRUE;

		/* First, try to sense */
		for (j = 0; j < 2; j++) {
		    Output = Connector->Output[j];
		    if (Output && Output->Sense &&
			Output->Sense(Output, Connector->Type)) {
			Output->Connector = Connector;
			break;
		    }
		}

		if (j == 2) {
		    /* now just enable the ones without sensing */
		    for (j = 0; j < 2; j++) {
			Output = Connector->Output[j];
			if (Output && !Output->Sense) {
			    Output->Connector = Connector;
			    break;
			}
		    }
		}
	    } else
		Connector->HPDAttached = FALSE;
	} else {
	    /* First, try to sense */
	    for (j = 0; j < 2; j++) {
		Output = Connector->Output[j];
		if (Output && Output->Sense &&
		    Output->Sense(Output, Connector->Type)) {
		    Output->Connector = Connector;
		    break;
		}
	    }

	    if (j == 2) {
		/* now just enable the ones without sensing */
		for (j = 0; j < 2; j++) {
		    Output = Connector->Output[j];
		    if (Output && !Output->Sense) {
			Output->Connector = Connector;
			break;
		    }
		}
	    }
	}
    }

    j = 0;
    for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	if (Output->Connector) {
	    struct rhdMonitor *Monitor = NULL;

	    Connector = Output->Connector;

	    Monitor = RHDMonitorInit(Connector);

	    if (!Monitor && (Connector->Type == RHD_CONNECTOR_PANEL)) {
		xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "Unable to attach a"
			   " monitor to connector \"%s\"\n", Connector->Name);
		Output->Active = FALSE;
	    } else {
		Connector->Monitor = Monitor;

		Output->Active = TRUE;

		Output->Crtc = rhdPtr->Crtc[j & 1]; /* ;) */
		j++;

		Output->Crtc->Active = TRUE;

		Found = TRUE;
	    }
	}

    return Found;
}

/*
 *
 */
static void
rhdModeLayoutPrint(RHDPtr rhdPtr)
{
    struct rhdCrtc *Crtc;
    struct rhdOutput *Output;
    Bool Found;

    xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Listing modesetting layout:\n\n");

    /* CRTC 1 */
    Crtc = rhdPtr->Crtc[0];
    if (Crtc->Active) {
	xf86Msg(X_NONE, "\t%s: tied to %s and %s:\n",
		Crtc->Name, Crtc->PLL->Name, Crtc->LUT->Name);

	Found = FALSE;
	for (Output = rhdPtr->Outputs; Output; Output = Output->Next)
	    if (Output->Active && (Output->Crtc == Crtc)) {
		if (!Found) {
		    xf86Msg(X_NONE, "\t\tOutputs: %s (%s)",
			    Output->Name, Output->Connector->Name);
		    Found = TRUE;
		} else
		    xf86Msg(X_NONE, ", %s (%s)", Output->Name,
			    Output->Connector->Name);
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
	    if (Output->Active && (Output->Crtc == Crtc)) {
		if (!Found) {
		    xf86Msg(X_NONE, "\t\tOutputs: %s (%s)",
			    Output->Name, Output->Connector->Name);
		    Found = TRUE;
		} else
		    xf86Msg(X_NONE, ", %s (%s)", Output->Name,
			    Output->Connector->Name);
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
		xf86Msg(X_NONE, "\t\tUnused Outputs: %s", Output->Name);
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
    struct rhdCrtc *Crtc;

    RHDFUNC(rhdPtr);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Setting up \"%s\" (%dx%d@%3.1fHz)\n",
	       mode->name, mode->CrtcHDisplay, mode->CrtcVDisplay,
	       mode->VRefresh);

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
		    pScrn->depth, rhdPtr->FbFreeStart);
	Crtc->ModeSet(Crtc, mode);
	RHDPLLSet(Crtc->PLL, mode->Clock);
	Crtc->PLLSelect(Crtc, Crtc->PLL);
	Crtc->LUTSelect(Crtc, Crtc->LUT);
	RHDOutputsMode(rhdPtr, Crtc);
    }

    /* Set up D2 and appendages */
    Crtc = rhdPtr->Crtc[1];
    if (Crtc->Active) {
	Crtc->FBSet(Crtc, pScrn->displayWidth, pScrn->virtualX, pScrn->virtualY,
		    pScrn->depth, rhdPtr->FbFreeStart);
	Crtc->ModeSet(Crtc, mode);
	RHDPLLSet(Crtc->PLL, mode->Clock);
	Crtc->PLLSelect(Crtc, Crtc->PLL);
	Crtc->LUTSelect(Crtc, Crtc->LUT);
	RHDOutputsMode(rhdPtr, Crtc);
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
    ScrnInfoPtr pScrn = xf86Screens[rhdPtr->scrnIndex];

    RHDFUNC(rhdPtr);

    RHDVGASave(rhdPtr);

    RHDOutputsSave(rhdPtr);

    RHDPLLsSave(rhdPtr);
    RHDLUTsSave(rhdPtr);

    rhdPtr->Crtc[0]->Save(rhdPtr->Crtc[0]);
    rhdPtr->Crtc[1]->Save(rhdPtr->Crtc[1]);
    rhdSaveCursor(pScrn);
}

/*
 *
 */
static void
rhdRestore(RHDPtr rhdPtr)
{
    ScrnInfoPtr pScrn = xf86Screens[rhdPtr->scrnIndex];

    RHDFUNC(rhdPtr);

    RHDPLLsRestore(rhdPtr);
    RHDLUTsRestore(rhdPtr);

    rhdPtr->Crtc[0]->Restore(rhdPtr->Crtc[0]);
    rhdPtr->Crtc[1]->Restore(rhdPtr->Crtc[1]);
    if (rhdPtr->CursorInfo) 
	rhdRestoreCursor(pScrn);

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

/* The following two are R5XX only. R6XX doesn't require these */
CARD32
_RHDReadMC(int scrnIndex, CARD16 offset)
{
    CARD32 addr = (offset & 0x1FF) | ((offset & 0xFE) << 7);
    _RHDRegWrite(scrnIndex, MC_IND_INDEX, addr);
    return _RHDRegRead(scrnIndex, MC_IND_DATA);
}

void
_RHDWriteMC(int scrnIndex, CARD16 offset, CARD32 data)
{
    CARD32 addr = (offset & 0x1FF) | ((offset & 0xFE) << 7);
    _RHDRegWrite(scrnIndex, MC_IND_INDEX, addr);
    _RHDRegWrite(scrnIndex, MC_IND_DATA, data);
}

CARD32
_RHDReadPLL(int scrnIndex, CARD16 offset)
{
    _RHDRegWrite(scrnIndex, CLOCK_CNTL_INDEX, (offset & PLL_ADDR));
    return _RHDRegRead(scrnIndex, CLOCK_CNTL_DATA);
}

void
_RHDWritePLL(int scrnIndex, CARD16 offset, CARD32 data)
{
    _RHDRegWrite(scrnIndex, CLOCK_CNTL_INDEX, (offset & PLL_ADDR) | PLL_WR_EN);
    _RHDRegWrite(scrnIndex, CLOCK_CNTL_DATA, data);
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
    RhdGetOptValBool(rhdPtr->Options, OPTION_SHADOWFB, &rhdPtr->shadowFB,
		     TRUE);
}

/*
 * Crude test if DDC is working.
 */

static void
rhdTestDDC(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    RHDI2CDataArg data;
    Bool done = FALSE;

    int i = 0;
    while (!done) {
	data.i = i++;
	switch (RHDI2CFunc(pScrn->scrnIndex, rhdPtr->I2C, RHD_I2C_DDC, &data)) {
	    case RHD_I2C_SUCCESS:
		if (data.monitor) {
		    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Found DDC on line %i:\n",
			       i - 1);
		    xf86PrintEDID(data.monitor);
		    xfree(data.monitor);
		} else {
		    xf86DrvMsgVerb(pScrn->scrnIndex, 7,X_INFO,
				   "No DDC data found on line %i\n",i - 1);
		}
		break;
	    case RHD_I2C_NOLINE:
		done = TRUE;
		break;
	    default:
		xf86DrvMsgVerb(pScrn->scrnIndex, 7,X_INFO, "No DDC line %i found.\n",data.i);
		break;
	}
    }
}


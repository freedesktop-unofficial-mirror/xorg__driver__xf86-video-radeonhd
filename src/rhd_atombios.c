/*
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
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

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "rhd.h"
#include "rhd_atomwrapper.h"
#include "xf86int10.h"
#include "rhd_atombios.h"
#define INT32 INT32
#include "CD_Common_Types.h"
/* these types are used in ATI's atombios.h */
#if 0
# ifndef ULONG
typedef CARD32 ULONG;
# endif
# ifndef USHORT
typedef CARD16 USHORT;
# endif
# ifndef UCHAR
typedef CARD8 UCHAR;
# endif
#endif

# include "atombios.h"

#define CAILFUNC(ptr) RHDFUNC((atomBIOSHandlePtr)ptr)

typedef struct _atomDataTables
{
    unsigned char                       *UtilityPipeLine;
    ATOM_MULTIMEDIA_CAPABILITY_INFO     *MultimediaCapabilityInfo;
    ATOM_MULTIMEDIA_CONFIG_INFO         *MultimediaConfigInfo;
    ATOM_STANDARD_VESA_TIMING           *StandardVESA_Timing;
    union {
        void                            *base;
        ATOM_FIRMWARE_INFO              *FirmwareInfo;
        ATOM_FIRMWARE_INFO_V1_2         *FirmwareInfo_V_1_2;
        ATOM_FIRMWARE_INFO_V1_3         *FirmwareInfo_V_1_3;
        ATOM_FIRMWARE_INFO_V1_4         *FirmwareInfo_V_1_4;
    } FirmwareInfo;
    ATOM_DAC_INFO                       *DAC_Info;
    union {
        void                            *base;
        ATOM_LVDS_INFO                  *LVDS_Info;
        ATOM_LVDS_INFO_V12              *LVDS_Info_v12;
    } LVDS_Info;
    ATOM_TMDS_INFO                      *TMDS_Info;
    ATOM_ANALOG_TV_INFO                 *AnalogTV_Info;
    union {
        void                            *base;
        ATOM_SUPPORTED_DEVICES_INFO     *SupportedDevicesInfo;
        ATOM_SUPPORTED_DEVICES_INFO_2   *SupportedDevicesInfo_2;
        ATOM_SUPPORTED_DEVICES_INFO_2d1 *SupportedDevicesInfo_2d1;
    } SupportedDevicesInfo;
    ATOM_GPIO_I2C_INFO                  *GPIO_I2C_Info;
    ATOM_VRAM_USAGE_BY_FIRMWARE         *VRAM_UsageByFirmware;
    ATOM_GPIO_PIN_LUT                   *GPIO_Pin_LUT;
    ATOM_VESA_TO_INTENAL_MODE_LUT       *VESA_ToInternalModeLUT;
    union {
        void                            *base;
        ATOM_COMPONENT_VIDEO_INFO       *ComponentVideoInfo;
        ATOM_COMPONENT_VIDEO_INFO_V21   *ComponentVideoInfo_v21;
    } ComponentVideoInfo;
/**/unsigned char                       *PowerPlayInfo;
    COMPASSIONATE_DATA                  *CompassionateData;
    ATOM_DISPLAY_DEVICE_PRIORITY_INFO   *SaveRestoreInfo;
/**/unsigned char                       *PPLL_SS_Info;
    ATOM_OEM_INFO                       *OemInfo;
    ATOM_XTMDS_INFO                     *XTMDS_Info;
    ATOM_ASIC_MVDD_INFO                 *MclkSS_Info;
    ATOM_OBJECT_HEADER                  *Object_Header;
    INDIRECT_IO_ACCESS                  *IndirectIOAccess;
    ATOM_MC_INIT_PARAM_TABLE            *MC_InitParameter;
/**/unsigned char                       *ASIC_VDDC_Info;
    ATOM_ASIC_INTERNAL_SS_INFO          *ASIC_InternalSS_Info;
/**/unsigned char                       *TV_VideoMode;
    union {
        void                            *base;
        ATOM_VRAM_INFO_V2               *VRAM_Info_v2;
        ATOM_VRAM_INFO_V3               *VRAM_Info_v3;
    } VRAM_Info;
    ATOM_MEMORY_TRAINING_INFO           *MemoryTrainingInfo;
    union {
        void                            *base;
        ATOM_INTEGRATED_SYSTEM_INFO     *IntegratedSystemInfo;
        ATOM_INTEGRATED_SYSTEM_INFO_V2  *IntegratedSystemInfo_v2;
    } IntegratedSystemInfo;
    ATOM_ASIC_PROFILING_INFO            *ASIC_ProfilingInfo;
    ATOM_VOLTAGE_OBJECT_INFO            *VoltageObjectInfo;
    ATOM_POWER_SOURCE_INFO              *PowerSourceInfo;
} atomDataTables, *atomDataTablesPtr;

typedef struct _atomBIOSHandle {
    int scrnIndex;
    unsigned char *BIOSBase;
    atomDataTablesPtr atomDataPtr;
    pointer *scratchBase;
    CARD32 fbBase;
    PCITAG PciTag;
} atomBIOSHandle;

enum {
    legacyBIOSLocation = 0xC0000,
    legacyBIOSMax = 0x10000
};

char *AtomBIOSQueryStr[] = {
    "Default Engine Clock",
    "Default Memory Clock",
    "Maximum Pixel ClockPLL Frequency Output",
    "Minimum Pixel ClockPLL Frequency Output",
    "Maximum Pixel ClockPLL Frequency Input",
    "Minimum Pixel ClockPLL Frequency Input",
    "Minimum Pixel Clock",
    "Reference Clock",
    "Start of VRAM area used by Firmware",
    "Framebuffer space used by Firmware (kb)",
    "TDMS Frequency",
    "PLL ChargePump",
    "PLL DutyCycle",
    "PLL VCO Gain",
    "PLL VoltageSwing"
};

static int
rhdAnalyzeCommonHdr(ATOM_COMMON_TABLE_HEADER *hdr)
{
    if (hdr->usStructureSize == 0xaa55)
        return FALSE;

    return TRUE;
}

static int
rhdAnalyzeRomHdr(unsigned char *rombase,
              ATOM_ROM_HEADER *hdr,
              int *data_offset)
{
    if (rhdAnalyzeCommonHdr(&hdr->sHeader) == -1) {
        return FALSE;
    }
    xf86ErrorF("\tSubsysemVendorID: 0x%4.4x SubsystemID: 0x%4.4x\n",
               hdr->usSubsystemVendorID,hdr->usSubsystemID);
    xf86ErrorF("\tIOBaseAddress: 0x%4.4x\n",hdr->usIoBaseAddress);
    xf86ErrorFVerb(3,"\tFilename: %s\n",rombase + hdr->usConfigFilenameOffset);
    xf86ErrorFVerb(3,"\tBIOS Bootup Message: %s\n",
		   rombase + hdr->usBIOS_BootupMessageOffset);

    *data_offset = hdr->usMasterDataTableOffset;

    return TRUE;
}

static int
rhdAnalyzeRomDataTable(unsigned char *base, int offset,
                    void *ptr,short *size)
{
    ATOM_COMMON_TABLE_HEADER *table = (ATOM_COMMON_TABLE_HEADER *)
        (base + offset);

   if (!*size || rhdAnalyzeCommonHdr(table) == -1) {
       if (*size) *size -= 2;
       *(void **)ptr = NULL;
       return FALSE;
   }
   *size -= 2;
   *(void **)ptr = (void *)(table);
   return TRUE;
}

static Bool
rhdGetAtomBiosTableRevisionAndSize(ATOM_COMMON_TABLE_HEADER *hdr,
                                   CARD8 *contentRev,
                                   CARD8 *formatRev,
                                   short *size)
{
    if (!hdr)
        return FALSE;

    if (contentRev) *contentRev = hdr->ucTableContentRevision;
    if (formatRev) *formatRev = hdr->ucTableFormatRevision;
    if (size) *size = (short)hdr->usStructureSize
                   - sizeof(ATOM_COMMON_TABLE_HEADER);
    return TRUE;
}

static Bool
rhdAnalyzeMasterDataTable(unsigned char *base,
                       ATOM_MASTER_DATA_TABLE *table,
                       atomDataTablesPtr data)
{
    ATOM_MASTER_LIST_OF_DATA_TABLES *data_table =
        &table->ListOfDataTables;
    short size;

    if (!rhdAnalyzeCommonHdr(&table->sHeader))
        return FALSE;
    if (!rhdGetAtomBiosTableRevisionAndSize(&table->sHeader,NULL,NULL,&size))
        return FALSE;
#define SET_DATA_TABLE(x) {\
   rhdAnalyzeRomDataTable(base,data_table->x,(void *)(&(data->x)),&size); \
    }

#define SET_DATA_TABLE_VERS(x) {\
   rhdAnalyzeRomDataTable(base,data_table->x,&(data->x.base),&size); \
    }

    SET_DATA_TABLE(UtilityPipeLine);
    SET_DATA_TABLE(MultimediaCapabilityInfo);
    SET_DATA_TABLE(MultimediaConfigInfo);
    SET_DATA_TABLE(StandardVESA_Timing);
    SET_DATA_TABLE_VERS(FirmwareInfo);
    SET_DATA_TABLE(DAC_Info);
    SET_DATA_TABLE_VERS(LVDS_Info);
    SET_DATA_TABLE(TMDS_Info);
    SET_DATA_TABLE(AnalogTV_Info);
    SET_DATA_TABLE_VERS(SupportedDevicesInfo);
    SET_DATA_TABLE(GPIO_I2C_Info);
    SET_DATA_TABLE(VRAM_UsageByFirmware);
    SET_DATA_TABLE(GPIO_Pin_LUT);
    SET_DATA_TABLE(VESA_ToInternalModeLUT);
    SET_DATA_TABLE_VERS(ComponentVideoInfo);
    SET_DATA_TABLE(PowerPlayInfo);
    SET_DATA_TABLE(CompassionateData);
    SET_DATA_TABLE(SaveRestoreInfo);
    SET_DATA_TABLE(PPLL_SS_Info);
    SET_DATA_TABLE(OemInfo);
    SET_DATA_TABLE(XTMDS_Info);
    SET_DATA_TABLE(MclkSS_Info);
    SET_DATA_TABLE(Object_Header);
    SET_DATA_TABLE(IndirectIOAccess);
    SET_DATA_TABLE(MC_InitParameter);
    SET_DATA_TABLE(ASIC_VDDC_Info);
    SET_DATA_TABLE(ASIC_InternalSS_Info);
    SET_DATA_TABLE(TV_VideoMode);
    SET_DATA_TABLE_VERS(VRAM_Info);
    SET_DATA_TABLE(MemoryTrainingInfo);
    SET_DATA_TABLE_VERS(IntegratedSystemInfo);
    SET_DATA_TABLE(ASIC_ProfilingInfo);
    SET_DATA_TABLE(VoltageObjectInfo);
    SET_DATA_TABLE(PowerSourceInfo);
#undef SET_DATA_TABLE

    return TRUE;
}

Bool
rhdGetAtombiosDataTable(ScrnInfoPtr pScrn, unsigned char *base,
                        atomDataTables *atomDataPtr)
{
    int  data_offset;
    unsigned short atom_romhdr_off =  *(unsigned short*)
        (base + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER);
    ATOM_ROM_HEADER *atom_rom_hdr =
        (ATOM_ROM_HEADER *)(base + atom_romhdr_off);

    RHDFUNC(pScrn)

    if (memcmp("ATOM",&atom_rom_hdr->uaFirmWareSignature,4)) {
        xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"No AtomBios signature found\n");
        return FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ATOM BIOS Rom: \n");
    if (!rhdAnalyzeRomHdr(base, atom_rom_hdr, &data_offset)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "RomHeader invalid\n");
        return FALSE;
    }
    if (!rhdAnalyzeMasterDataTable(base, (ATOM_MASTER_DATA_TABLE *)
                              (base + data_offset),
                                atomDataPtr)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ROM Master Table invalid\n");
        return FALSE;
    }
    return TRUE;
}

typedef enum {
    FB_BASE_VRAM,
    FB_BASE_RAM,
    FB_BASE_INVALID,
    FB_BASE_ATOM_INVALID = FB_BASE_INVALID,
    FB_BASE_NONE
} rhdFbBaseAllocStatus;

static rhdFbBaseAllocStatus
rhdAtomBIOSSetFbBase(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle, Bool TryHard)
{
    AtomBIOSArg data;
    unsigned int fb_base = 0;
    unsigned int fb_size = 0;
    handle->scratchBase = NULL;
    handle->fbBase = 0;
    
    if (RHDAtomBIOSFunc(pScrn, handle, GET_FW_FB_SIZE, &data)
	== ATOM_SUCCESS) {
	fb_size = data.val * 1024; /* convert to bytes */
	if (fb_size)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AtomBIOS requests %ikB"
		       " of VRAM scratch space\n",fb_size);
	else {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s: AtomBIOS specified VRAM "
		       "scratch space size invalid\n", __func__);
	    if (!TryHard)
		return FB_BASE_ATOM_INVALID;
	    /* default in case values are not filled in yet */
	    fb_size = 20 * 1024;
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, " default to: %i\n",fb_size);
	}
    }
    if (RHDAtomBIOSFunc(pScrn, handle, GET_FW_FB_START, &data)
	== ATOM_SUCCESS) {
	fb_base = data.val;
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AtomBIOS VRAM scratch base: 0x%x\n",
		   fb_base);
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "%s: AtomBIOS specified VRAM scratch "
		   "size invalid\n",__func__);
	fb_base = 0;
    }
    if (fb_base && fb_size && pScrn->videoRam) {
	int videoRam = pScrn->videoRam * 1024;
	/* 4k align */
	fb_size = (fb_size & ~(CARD32)0xfff) + ((fb_size & 0xfff) ? 1 : 0);
	if ((fb_base + fb_size) > videoRam) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "%s: FW FB scratch area %i (size: %i)"
		       " extends beyond framebuffer size %i\n",
		       __func__, fb_base, fb_size, videoRam);
	} else if ((fb_base + fb_size) < videoRam) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "%s: FW FB scratch area not located "
		       "at the end of VRAM. Scratch End: "
		       "0x%x VRAM End: 0x%x\n", __func__,
		       (unsigned int)(fb_base + fb_size),
		       videoRam);
	} else {
	    handle->fbBase = fb_base;
	    return FB_BASE_VRAM;
	}
    }
    if (!handle->fbBase) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Cannot get VRAM scratch space. "
		   "Allocating in main memory instead\n");
	handle->scratchBase = xcalloc(fb_size,1);
	return FB_BASE_RAM;
    }
    return FB_BASE_NONE;
}

static Bool
rhdASICInit(atomBIOSHandlePtr handle)
{
    ASIC_INIT_PS_ALLOCATION asicInit;
    AtomBIOSArg data;
    ScrnInfoPtr pScrn = xf86Screens[handle->scrnIndex];

    asicInit.sASICInitClocks.ulDefaultEngineClock = 70000;  /* in 10 Khz */
    asicInit.sASICInitClocks.ulDefaultMemoryClock = 70000;  /* in 10 Khz */
    data.execp.dataSpace = NULL;
    data.execp.index = 0x0;
    data.execp.pspace = &asicInit;
    xf86DrvMsg(handle->scrnIndex, X_INFO, "Calling ASIC Init\n");
    if (RHDAtomBIOSFunc(pScrn, handle,
			ATOMBIOS_EXEC, &data) == ATOM_SUCCESS) {
	xf86DrvMsg(handle->scrnIndex, X_INFO, "ASIC_INIT Successful\n");
	return TRUE;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ASIC_INIT Failed\n");
    return FALSE;
}

static atomBIOSHandlePtr
rhdInitAtomBIOS(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    unsigned char *ptr;
    atomDataTablesPtr atomDataPtr;
    atomBIOSHandlePtr handle = NULL;

    RHDFUNC(pScrn);

    if (rhdPtr->BIOSCopy) {
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Getting BIOS copy from INT10\n");
	ptr = rhdPtr->BIOSCopy;
	rhdPtr->BIOSCopy = NULL;
    } else {
	if (!xf86IsEntityPrimary(rhdPtr->entityIndex)) {
	    int length = 1 << rhdPtr->PciInfo->biosSize;
	    int read_len;
	    if (!(ptr = xcalloc(length, 1))) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,
			   "Cannot allocate %i bytes of memory "
			   "for BIOS image\n",length);
		return NULL;
	    }
	    xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Getting BIOS copy from PCI ROM\n");

	    if ((read_len =
		 xf86ReadPciBIOS(0, rhdPtr->PciTag, -1, ptr, length) < 0)) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,
			   "Cannot read BIOS image\n");
		goto error;
	    } else if (read_len != length)
		xf86DrvMsg(pScrn->scrnIndex,X_WARNING,
			   "Read only %i of %i bytes of BIOS image\n",
			   read_len, length);
	} else {
	    int length, read_len;
	    unsigned char tmp[32];
	    xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Getting BIOS copy from legacy VBIOS location\n");
	    if (xf86ReadBIOS(legacyBIOSLocation, 0, tmp, 32) < 0) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,
			   "Cannot obtain POSTed BIOS header\n");
		return NULL;
	    }
	    length = tmp[2] * 512;
	    if (length > legacyBIOSMax) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"Invalid BIOS length field\n");
		return NULL;
	    }
	    if (!(ptr = xcalloc(length,1))) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,
			   "Cannot allocate %i bytes of memory "
			   "for BIOS image\n",length);
		return NULL;
	    }
	    if ((read_len = xf86ReadBIOS(legacyBIOSLocation, 0, ptr, length)
		 < 0)) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"Cannot read POSTed BIOS\n");
		goto error;
	    }
	}
    }
    if (!(atomDataPtr = xcalloc(sizeof(atomDataTables),1))) {
	xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"Cannot allocate memory for "
		   "ATOM BIOS data tabes\n");
	goto error;
    }
    if (!rhdGetAtombiosDataTable(pScrn, ptr, atomDataPtr))
	goto error1;
    if (!(handle = xcalloc(sizeof(atomBIOSHandle),1))) {
	xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"Cannot allocate memory\n");
	goto error1;
    }
    handle->BIOSBase = ptr;
    handle->atomDataPtr = atomDataPtr;
    handle->scrnIndex = pScrn->scrnIndex;
    handle->PciTag = rhdPtr->PciTag;
    switch (rhdAtomBIOSSetFbBase(pScrn, handle, FALSE)) {
	case FB_BASE_ATOM_INVALID:
	    /*
	     * If the AtomBIOS table data is invalid we try to run ASIC init
	     * to init the tables.
	     */
	    
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AtomBIOS VRAM scratch space info"
		       " invalid. Trying AsicInit.\n");
	    if (rhdASICInit(handle)) {
		if (rhdAtomBIOSSetFbBase(pScrn, handle, FALSE) >= FB_BASE_INVALID) {
		    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "%s: Cannot allocate FB scratch area\n",__func__);
		}
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			   "%s: AsicInit failed. Won't be able to obtain in VRAM "
			   "FB scratch space\n",__func__);
		if (!rhdAtomBIOSSetFbBase(pScrn, handle, TRUE)) {
		    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "%s: No way to allocate FB scratch area\n",__func__);
		}
	    }
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "%s: Cannot set FB scratch area\n",__func__);
    }
    return handle;

 error1:
    xfree(atomDataPtr);
 error:
    xfree(ptr);
    return NULL;
}

void
rhdTearDownAtomBIOS(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle)
{
    RHDFUNC(pScrn);

    xfree(handle->BIOSBase);
    xfree(handle->atomDataPtr);
    xfree(handle);
}
AtomBiosResult
rhdAtomBIOSVramInfoQuery(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle, AtomBiosFunc func,
			     CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;

    RHDFUNC(pScrn);

    atomDataPtr = handle->atomDataPtr;

    switch (func) {
	case GET_FW_FB_START:
	    *val = atomDataPtr->VRAM_UsageByFirmware
		->asFirmwareVramReserveInfo[0].ulStartAddrUsedByFirmware;
	    break;
	case GET_FW_FB_SIZE:
	    *val = atomDataPtr->VRAM_UsageByFirmware
		->asFirmwareVramReserveInfo[0].usFirmwareUseInKb;
	    break;
	default:
	    return ATOM_NOT_IMPLEMENTED;
    }
    return ATOM_SUCCESS;
}

AtomBiosResult
rhdAtomBIOSTmdsInfoQuery(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle,
			 AtomBiosFunc func, int index, CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;

    atomDataPtr = handle->atomDataPtr;
    if (!rhdGetAtomBiosTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->FirmwareInfo.base),
	    NULL,NULL,NULL)) {
	return ATOM_FAILED;
    }

    RHDFUNC(pScrn);

    switch (func) {
	case ATOM_TMDS_FREQUENCY:
	    *val = atomDataPtr->TMDS_Info->asMiscInfo[index].usFrequency;
	    break;
	case ATOM_TMDS_PLL_CHARGE_PUMP:
	    *val = atomDataPtr->TMDS_Info->asMiscInfo[index].ucPLL_ChargePump;
	    break;
	case ATOM_TMDS_PLL_DUTY_CYCLE:
	    *val = atomDataPtr->TMDS_Info->asMiscInfo[index].ucPLL_DutyCycle;
	    break;
	case ATOM_TMDS_PLL_VCO_GAIN:
	    *val = atomDataPtr->TMDS_Info->asMiscInfo[index].ucPLL_VCO_Gain;
	    break;
	case ATOM_TMDS_PLL_VOLTAGE_SWING:
	    *val = atomDataPtr->TMDS_Info->asMiscInfo[index].ucPLL_VoltageSwing;
	    break;
	default:
	    return ATOM_NOT_IMPLEMENTED;
    }
    return ATOM_SUCCESS;
}

AtomBiosResult
rhdAtomBIOSFirmwareInfoQuery(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle,
			     AtomBiosFunc func, CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;

    RHDFUNC(pScrn);

    atomDataPtr = handle->atomDataPtr;
    if (!rhdGetAtomBiosTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->FirmwareInfo.base),
	    &crev,&frev,NULL)) {
	return ATOM_FAILED;
    }
    switch (crev) {
	case 1:
	    switch (func) {
		case GET_DEFAULT_ENGINE_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->ulDefaultEngineClock;
		    break;
		case GET_DEFAULT_MEMORY_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->ulDefaultMemoryClock;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->ulMaxPixelClockPLL_Output;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->usMinPixelClockPLL_Output;
		case GET_MAX_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->usMaxPixelClockPLL_Input;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->usMinPixelClockPLL_Input;
		    break;
		case GET_MAX_PIXEL_CLK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->usMaxPixelClock;
		    break;
		case GET_REF_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo->usReferenceClock;
		    break;
		default:
		    return ATOM_NOT_IMPLEMENTED;
	    }
	case 2:
	    switch (func) {
		case GET_DEFAULT_ENGINE_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->ulDefaultEngineClock;
		    break;
		case GET_DEFAULT_MEMORY_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->ulDefaultMemoryClock;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->ulMaxPixelClockPLL_Output;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->usMinPixelClockPLL_Output;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->usMaxPixelClockPLL_Input;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->usMinPixelClockPLL_Input;
		    break;
		case GET_MAX_PIXEL_CLK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->usMaxPixelClock;
		    break;
		case GET_REF_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_2->usReferenceClock;
		    break;
		default:
		    return ATOM_NOT_IMPLEMENTED;
	    }
	    break;
	case 3:
	    switch (func) {
		case GET_DEFAULT_ENGINE_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->ulDefaultEngineClock;
		    break;
		case GET_DEFAULT_MEMORY_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->ulDefaultMemoryClock;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->ulMaxPixelClockPLL_Output;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->usMinPixelClockPLL_Output;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->usMaxPixelClockPLL_Input;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->usMinPixelClockPLL_Input;
		    break;
		case GET_MAX_PIXEL_CLK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->usMaxPixelClock;
		    break;
		case GET_REF_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_3->usReferenceClock;
		    break;
		default:
		    return ATOM_NOT_IMPLEMENTED;
	    }
	    break;
	case 4:
	    switch (func) {
		case GET_DEFAULT_ENGINE_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->ulDefaultEngineClock;
		    break;
		case GET_DEFAULT_MEMORY_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->ulDefaultMemoryClock;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->usMaxPixelClockPLL_Input;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_INPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->usMinPixelClockPLL_Input;
		    break;
		case GET_MAX_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->ulMaxPixelClockPLL_Output;
		    break;
		case GET_MIN_PIXEL_CLOCK_PLL_OUTPUT:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->usMinPixelClockPLL_Output;
		    break;
		case GET_MAX_PIXEL_CLK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->usMaxPixelClock;
		    break;
		case GET_REF_CLOCK:
		    *val = atomDataPtr->FirmwareInfo
			.FirmwareInfo_V_1_4->usReferenceClock;
		    break;
		default:
		    return ATOM_NOT_IMPLEMENTED;
	    }
	    break;
	default:
	    return ATOM_NOT_IMPLEMENTED;
    }
    return ATOM_SUCCESS;
}

static Bool
rhdAtomExec (atomBIOSHandlePtr handle, int index, void *pspace,  pointer *dataSpace)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[handle->scrnIndex]);
    Bool ret = FALSE;
    char *msg;

    RHDFUNCI(handle->scrnIndex);
    if (dataSpace) {
	if (!handle->fbBase && !handle->scratchBase)
	    return FALSE;
	if (handle->fbBase) {
	    if (!rhdPtr->FbBase) {
		xf86DrvMsg(handle->scrnIndex, X_ERROR, "%s: "
			   "Cannot exec AtomBIOS: framebuffer not mapped\n",
			   __func__);
		return FALSE;
	    }
	    *dataSpace = (CARD8*)rhdPtr->FbBase + handle->fbBase;
	} else
	    *dataSpace = (CARD8*)handle->scratchBase;
    }
    ret = ParseTableWrapper(pspace, index, handle,
			    handle->BIOSBase,
			    &msg);
    if (!ret)
	xf86DrvMsg(handle->scrnIndex, X_ERROR, "%s\n",msg);
    else
	xf86DrvMsgVerb(handle->scrnIndex, X_INFO, 5, "%s\n",msg);

    return (ret) ? TRUE : FALSE;
}

AtomBiosResult
RHDAtomBIOSFunc(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle, AtomBiosFunc func,
		AtomBIOSArgPtr data)
{
    AtomBiosResult ret = ATOM_NOT_IMPLEMENTED;
    CARD32 val;

    RHDFUNC(pScrn);

    if (func == ATOMBIOS_INIT) {
	if (!(data->atomp = rhdInitAtomBIOS(pScrn)))
	    return ATOM_FAILED;
	return ATOM_SUCCESS;
    }
    if (!handle)
	return ATOM_FAILED;
    if (func <= ATOMBIOS_TEARDOWN) {
	rhdTearDownAtomBIOS(pScrn, handle);
	return ATOM_SUCCESS;
    }
    if (func == ATOMBIOS_EXEC) {
	if (!rhdAtomExec(handle, data->execp.index,
			 data->execp.pspace, data->execp.dataSpace))
	    return ATOM_FAILED;
	else
	    return ATOM_SUCCESS;
    } else if (func >= ATOM_QUERY_FUNCS && func < ATOM_VRAM_QUERIES) {
	ret = rhdAtomBIOSFirmwareInfoQuery(pScrn, handle, func, &val);
	data->val = val;
    } else if (func >= ATOM_VRAM_QUERIES && func < FUNC_END) {
	ret = rhdAtomBIOSVramInfoQuery(pScrn, handle, func, &val);
	data->val = val;
    } else {
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"%s: Received unknown query\n",__func__);
	return ATOM_NOT_IMPLEMENTED;
    }
    if (ret == ATOM_SUCCESS)
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"%s: %i\n",
		   AtomBIOSQueryStr[func - ATOM_QUERY_FUNCS], (unsigned int)val);
    else
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Query for %s: %s\n",
		   AtomBIOSQueryStr[func - ATOM_QUERY_FUNCS],
		   ret == ATOM_FAILED ? "failed" : "not implemented");
    return ret;
}

VOID*
CailAllocateMemory(VOID *CAIL,UINT16 size)
{
    CAILFUNC(CAIL);

    return malloc(size);
}

VOID
CailReleaseMemory(VOID *CAIL, VOID *addr)
{
    CAILFUNC(CAIL);

    free(addr);
}

VOID
CailDelayMicroSeconds(VOID *CAIL, UINT32 delay)
{
    CAILFUNC(CAIL);

    usleep(delay);
#if 0
    xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,X_INFO,"Delay %i usec\n",delay);
#endif
}

UINT32
CailReadATIRegister(VOID* CAIL, UINT32 index)
{
    UINT32 ret;
    CAILFUNC(CAIL);

    ret  =  RHDRegRead(((atomBIOSHandlePtr)CAIL), index << 2);
    DEBUGP(ErrorF("%s(%x) = %x\n",__func__,index << 2,ret));
    return ret;
}

VOID
CailWriteATIRegister(VOID *CAIL, UINT32 index, UINT32 data)
{
    CAILFUNC(CAIL);

    RHDRegWrite(((atomBIOSHandlePtr)CAIL),index << 2,data);
    DEBUGP(ErrorF("%s(%x,%x)\n",__func__,index << 2,data));
}

UINT32
CailReadFBData(VOID* CAIL, UINT32 index)
{
    UINT32 ret;

    CAILFUNC(CAIL);
    
    if (((atomBIOSHandlePtr)CAIL)->fbBase) {
	CARD8 *FBBase = (CARD8*)
	    RHDPTR(xf86Screens[((atomBIOSHandlePtr)CAIL)->scrnIndex])->FbBase;
	ret =  *((CARD32*)(FBBase + (((atomBIOSHandlePtr)CAIL)->fbBase) + index));
	DEBUGP(ErrorF("%s(%x) = %x\n",__func__,index,ret));
    } else if (((atomBIOSHandlePtr)CAIL)->scratchBase) {
	ret = *(CARD32*)((CARD8*)(((atomBIOSHandlePtr)CAIL)->scratchBase) + index);
	DEBUGP(ErrorF("%s(%x) = %x\n",__func__,index,ret));
    } else {
	xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,X_ERROR,
		   "%s: no fbbase set\n",__func__);
	return 0;
    }
    return ret;
}

VOID
CailWriteFBData(VOID *CAIL, UINT32 index, UINT32 data)
{
    CAILFUNC(CAIL);

    DEBUGP(ErrorF("%s(%x,%x)\n",__func__,index,data));
    if (((atomBIOSHandlePtr)CAIL)->fbBase) {
	CARD8 *FBBase = (CARD8*)
	    RHDPTR(xf86Screens[((atomBIOSHandlePtr)CAIL)->scrnIndex])->FbBase;
	*((CARD32*)(FBBase + (((atomBIOSHandlePtr)CAIL)->fbBase) + index)) = data;
    } else if (((atomBIOSHandlePtr)CAIL)->scratchBase) {
	*(CARD32*)((CARD8*)(((atomBIOSHandlePtr)CAIL)->scratchBase) + index) = data;
    } else
	xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,X_ERROR,
		   "%s: no fbbase set\n",__func__);
}

ULONG
CailReadMC(VOID *CAIL, ULONG Address)
{
    ULONG ret;
    
    CAILFUNC(CAIL);

    ret = RHDReadMC(((atomBIOSHandlePtr)CAIL), Address);
    DEBUGP(ErrorF("%s(%x) = %x\n",__func__,Address,ret));
    return ret;
}

VOID
CailWriteMC(VOID *CAIL, ULONG Address, ULONG data)
{
    CAILFUNC(CAIL);
    DEBUGP(ErrorF("%s(%x,%x)\n",__func__,Address,data));
    RHDWriteMC(((atomBIOSHandlePtr)CAIL), Address, data);
}

VOID
CailReadPCIConfigData(VOID*CAIL, VOID* ret, UINT32 index,UINT16 size)
{
    PCITAG tag = ((atomBIOSHandlePtr)CAIL)->PciTag;

    CAILFUNC(CAIL);

    switch (size) {
	case 8:
	    *(CARD8*)ret = pciReadByte(tag,index << 2);
	    break;
	case 16:
	    *(CARD16*)ret = pciReadWord(tag,index << 2);
	    break;
	case 32:
	    *(CARD32*)ret = pciReadLong(tag,index << 2);
	    break;
	default:
	xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,
		   X_ERROR,"%s: Unsupported size: %i\n",
		   __func__,(int)size);
	return;
	    break;
    }
    DEBUGP(ErrorF("%s(%x) = %x\n",__func__,index,*(unsigned int*)ret));

}

VOID
CailWritePCIConfigData(VOID*CAIL,VOID*src,UINT32 index,UINT16 size)
{
    PCITAG tag = ((atomBIOSHandlePtr)CAIL)->PciTag;

    CAILFUNC(CAIL);
    DEBUGP(ErrorF("%s(%x,%x)\n",__func__,index,(*(unsigned int*)src)));
    switch (size) {
	case 8:
	    pciWriteByte(tag,index << 2,*(CARD8*)src);
	    break;
	case 16:
	    pciWriteWord(tag,index << 2,*(CARD16*)src);
	    break;
	case 32:
	    pciWriteLong(tag,index << 2,*(CARD32*)src);
	    break;
	default:
	    xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,X_ERROR,
		       "%s: Unsupported size: %i\n",__func__,(int)size);
	    break;
    }
}

ULONG
CailReadPLL(VOID *CAIL, ULONG Address)
{
    CAILFUNC(CAIL);

    xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,
	       X_ERROR, "%s: ReadPLL not impemented\n",
	       __func__);
    return 0;
}

VOID
CailWritePLL(VOID *CAIL, ULONG Address,ULONG Data)
{
    CAILFUNC(CAIL);

    xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,
	       X_ERROR, "%s: WritePLL not impemented",
	       __func__);
}

void
rhdTestAtomBIOS(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    READ_EDID_FROM_HW_I2C_DATA_PARAMETERS i2cData;
    AtomBIOSArg data;
    int i;
    unsigned char *space;
    
    i2cData.usPrescale = 0x7fff;
    i2cData.usVRAMAddress = 0;
    i2cData.usStatus = 128;
    i2cData.ucSlaveAddr = 0xA0;
    
    data.execp.dataSpace = (void*)&space;
    data.execp.index = 0x36;
    data.execp.pspace = &i2cData;
    
    for (i = 0; i < 4; i++) {
	i2cData.ucLineNumber = i;
	if (RHDAtomBIOSFunc(pScrn, rhdPtr->atomBIOS, ATOMBIOS_EXEC, &data) == ATOM_SUCCESS) {
	    int j;
	    CARD8 chksum = 0;
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"%s: I2C channel %i STATUS: %x\n",
		       __func__,i,i2cData.usStatus);
	    /* read good ? */
	    if ((i2cData.usStatus >> 8) == HW_ASSISTED_I2C_STATUS_SUCCESS) {
		/* checksum good? */
		if (!(i2cData.usStatus & 0xff)) {
		    RhdDebugDump(pScrn->scrnIndex, space, 128);
		    for (j = 0; j < 128; j++)
			chksum += space[i];
		    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DDC Checksum: %i\n",chksum);
		}
	    }
	}
    }
}

Bool
rhdTestAsicInit(ScrnInfoPtr pScrn)
{
    return rhdASICInit(RHDPTR(pScrn)->atomBIOS);
}

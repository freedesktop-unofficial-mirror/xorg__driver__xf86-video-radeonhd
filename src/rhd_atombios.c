/*
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
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
# include "config.h"
#endif
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "rhd.h"
#include "rhd_atombios.h"
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_monitor.h"
#include "rhd_card.h"
#include "rhd_regs.h"
/* only for testing now */
#include "xf86DDC.h"

char *AtomBIOSQueryStr[] = {
    "Default Engine Clock",
    "Default Memory Clock",
    "Maximum Pixel ClockPLL Frequency Output",
    "Minimum Pixel ClockPLL Frequency Output",
    "Maximum Pixel ClockPLL Frequency Input",
    "Minimum Pixel ClockPLL Frequency Input",
    "Maximum Pixel Clock",
    "Reference Clock",
    "Start of VRAM area used by Firmware",
    "Framebuffer space used by Firmware (kb)",
    "TMDS Frequency",
    "TMDS PLL ChargePump",
    "TMDS PLL DutyCycle",
    "TMDS PLL VCO Gain",
    "TMDS PLL VoltageSwing"
    "LVDS Supported Refresh Rate",
    "LVDS Off Delay",
    "LVDS SEQ Dig onto DE",
    "LVDS SEQ DE to BL"
    "LVDS Misc"
};

char *AtomBIOSFuncStr[] = {
    "AtomBIOS Init",
    "AtomBIOS Teardown",
    "AtomBIOS Exec",
    "AtomBIOS Set FB Space",
    "AtomBIOS Get Connectors",
    "AtomBIOS Get Panel Timings"
};

#ifdef ATOM_BIOS
# include "rhd_atomwrapper.h"
# include "xf86int10.h"
# ifdef ATOM_BIOS_PARSER
#  define INT8 INT8
#  define INT16 INT16
#  define INT32 INT32
#  include "CD_Common_Types.h"
# else
#  ifndef ULONG
typedef unsigned int ULONG;
#   define ULONG ULONG
#  endif
#  ifndef UCHAR
typedef unsigned char UCHAR;
#   define UCHAR UCHAR
#  endif
#  ifndef USHORT
typedef unsigned short USHORT;
#   define USHORT USHORT
#  endif
# endif

# include "atombios.h"
# include "ObjectID.h"

# define LOG_CAIL LOG_DEBUG + 1

#ifdef ATOM_BIOS_PARSER
static void
CailDebug(int scrnIndex, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, X_INFO, LOG_CAIL, format, ap);
    va_end(ap);
}
#endif

# define CAILFUNC(ptr) \
  CailDebug(((atomBIOSHandlePtr)(ptr))->scrnIndex, "CAIL: %s\n", __func__)

/*
 * This works around a bug in atombios.h where
 * ATOM_MAX_SUPPORTED_DEVICE_INFO is specified incorrectly.
 */

#define ATOM_MAX_SUPPORTED_DEVICE_INFO_HD (ATOM_DEVICE_RESERVEDF_INDEX+1)
typedef struct _ATOM_SUPPORTED_DEVICES_INFO_HD
{
    ATOM_COMMON_TABLE_HEADER      sHeader;
    USHORT                        usDeviceSupport;
    ATOM_CONNECTOR_INFO_I2C       asConnInfo[ATOM_MAX_SUPPORTED_DEVICE_INFO_HD];
    ATOM_CONNECTOR_INC_SRC_BITMAP asIntSrcInfo[ATOM_MAX_SUPPORTED_DEVICE_INFO_HD];
} ATOM_SUPPORTED_DEVICES_INFO_HD;

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
        ATOM_SUPPORTED_DEVICES_INFO_HD  *SupportedDevicesInfo_HD;
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
    int BIOSImageSize;
} atomBIOSHandle;

enum {
    legacyBIOSLocation = 0xC0000,
    legacyBIOSMax = 0x10000
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
    if (!rhdAnalyzeCommonHdr(&hdr->sHeader)) {
        return FALSE;
    }
    xf86DrvMsg(-1,X_NONE,"\tSubsystemVendorID: 0x%4.4x SubsystemID: 0x%4.4x\n",
               hdr->usSubsystemVendorID,hdr->usSubsystemID);
    xf86DrvMsg(-1,X_NONE,"\tIOBaseAddress: 0x%4.4x\n",hdr->usIoBaseAddress);
    xf86DrvMsgVerb(-1,X_NONE,3,"\tFilename: %s\n",rombase + hdr->usConfigFilenameOffset);
    xf86DrvMsgVerb(-1,X_NONE,3,"\tBIOS Bootup Message: %s\n",
		   rombase + hdr->usBIOS_BootupMessageOffset);

    *data_offset = hdr->usMasterDataTableOffset;

    return TRUE;
}

static int
rhdAnalyzeRomDataTable(unsigned char *base, int offset,
                    void *ptr,unsigned short *size)
{
    ATOM_COMMON_TABLE_HEADER *table = (ATOM_COMMON_TABLE_HEADER *)
        (base + offset);

   if (!*size || !rhdAnalyzeCommonHdr(table)) {
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
                                   unsigned short *size)
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
    unsigned short size;

    if (!rhdAnalyzeCommonHdr(&table->sHeader))
        return FALSE;
    if (!rhdGetAtomBiosTableRevisionAndSize(&table->sHeader,NULL,NULL,&size))
        return FALSE;
# define SET_DATA_TABLE(x) {\
   rhdAnalyzeRomDataTable(base,data_table->x,(void *)(&(data->x)),&size); \
    }

# define SET_DATA_TABLE_VERS(x) {\
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
# undef SET_DATA_TABLE

    return TRUE;
}

static Bool
rhdGetAtombiosDataTable(int scrnIndex, unsigned char *base,
                        atomDataTables *atomDataPtr, int BIOSImageSize)
{
    int  data_offset;
    unsigned short atom_romhdr_off =  *(unsigned short*)
        (base + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER);
    ATOM_ROM_HEADER *atom_rom_hdr =
        (ATOM_ROM_HEADER *)(base + atom_romhdr_off);

    RHDFUNCI(scrnIndex);

    if (atom_romhdr_off + sizeof(ATOM_ROM_HEADER) > BIOSImageSize) {
	xf86DrvMsg(scrnIndex,X_ERROR,
		   "%s: AtomROM header extends beyond BIOS image\n",__func__);
	return FALSE;
    }

    if (memcmp("ATOM",&atom_rom_hdr->uaFirmWareSignature,4)) {
        xf86DrvMsg(scrnIndex,X_ERROR,"%s: No AtomBios signature found\n",__func__);
        return FALSE;
    }
    xf86DrvMsg(scrnIndex, X_INFO, "ATOM BIOS Rom: \n");
    if (!rhdAnalyzeRomHdr(base, atom_rom_hdr, &data_offset)) {
        xf86DrvMsg(scrnIndex, X_ERROR, "RomHeader invalid\n");
        return FALSE;
    }

    if (data_offset + sizeof (ATOM_MASTER_DATA_TABLE) > BIOSImageSize) {
	xf86DrvMsg(scrnIndex,X_ERROR,"%s: Atom data table outside of BIOS\n",
		   __func__);
    }

    if (!rhdAnalyzeMasterDataTable(base, (ATOM_MASTER_DATA_TABLE *)
                              (base + data_offset),
                                atomDataPtr)) {
        xf86DrvMsg(scrnIndex, X_ERROR, "%s: ROM Master Table invalid\n", __func__);
        return FALSE;
    }
    return TRUE;
}

static Bool
rhdBIOSGetFbBaseAndSize(atomBIOSHandlePtr handle, unsigned int *base, unsigned int *size)
{
    AtomBIOSArg data;
    if (RHDAtomBIOSFunc(handle->scrnIndex, handle, GET_FW_FB_SIZE, &data)
	== ATOM_SUCCESS) {
	if (data.val == 0) {
	    xf86DrvMsg(handle->scrnIndex, X_WARNING, "%s: AtomBIOS specified VRAM "
		       "scratch space size invalid\n", __func__);
	    return FALSE;
	}
	*size = (int)data.val;
    } else
	return FALSE;
    if (RHDAtomBIOSFunc(handle->scrnIndex, handle, GET_FW_FB_START, &data)
	== ATOM_SUCCESS) {
	if (data.val == 0)
	    return FALSE;
	*base = (int)data.val;
    }
    return TRUE;
}

/*
 * Uses videoRam form ScrnInfoRec.
 */
static Bool
rhdAtomBIOSAllocateFbScratch(atomBIOSHandlePtr handle,
		     unsigned *start, unsigned int *size)
{
    unsigned int fb_base = 0;
    unsigned int fb_size = 0;
    handle->scratchBase = NULL;
    handle->fbBase = 0;

    if (rhdBIOSGetFbBaseAndSize(handle, &fb_base, &fb_size)) {
	xf86DrvMsg(handle->scrnIndex, X_INFO, "AtomBIOS requests %ikB"
		   " of VRAM scratch space\n",fb_size);
	fb_size *= 1024; /* convert to bytes */
	xf86DrvMsg(handle->scrnIndex, X_INFO, "AtomBIOS VRAM scratch base: 0x%x\n",
		   fb_base);
    } else {
	    fb_size = 20 * 1024;
	    xf86DrvMsg(handle->scrnIndex, X_INFO, " default to: %i\n",fb_size);
    }
    if (fb_base && fb_size && *size) {
	/* 4k align */
	fb_size = (fb_size & ~(CARD32)0xfff) + ((fb_size & 0xfff) ? 1 : 0);
	if ((fb_base + fb_size) > (*start + *size)) {
	    xf86DrvMsg(handle->scrnIndex, X_WARNING,
		       "%s: FW FB scratch area %i (size: %i)"
		       " extends beyond available framebuffer size %i\n",
		       __func__, fb_base, fb_size, *size);
	} else if ((fb_base + fb_size) < (*start + *size)) {
	    xf86DrvMsg(handle->scrnIndex, X_WARNING,
		       "%s: FW FB scratch area not located "
		       "at the end of VRAM. Scratch End: "
		       "0x%x VRAM End: 0x%x\n", __func__,
		       (unsigned int)(fb_base + fb_size),
		       *size);
	} else if (fb_base < *start) {
	    xf86DrvMsg(handle->scrnIndex, X_WARNING,
		       "%s: FW FB scratch area extends below "
		       "the base of the free VRAM: 0x%x Base: 0x%x\n",
		       __func__, (unsigned int)(fb_base), *start);
	} else {
	    *size -= fb_size;
	    handle->fbBase = fb_base;
	    return TRUE;
	}
    }

    if (!handle->fbBase) {
	xf86DrvMsg(handle->scrnIndex, X_INFO,
		   "Cannot get VRAM scratch space. "
		   "Allocating in main memory instead\n");
	handle->scratchBase = xcalloc(fb_size,1);
	return TRUE;
    }
    return FALSE;
}

# ifdef ATOM_BIOS_PARSER
static Bool
rhdASICInit(atomBIOSHandlePtr handle)
{
    ASIC_INIT_PS_ALLOCATION asicInit;
    AtomBIOSArg data;

    RHDAtomBIOSFunc(handle->scrnIndex, handle,
		    GET_DEFAULT_ENGINE_CLOCK,
		    &data);
    asicInit.sASICInitClocks.ulDefaultEngineClock = data.val;  /* in 10 Khz */
    RHDAtomBIOSFunc(handle->scrnIndex, handle,
		    GET_DEFAULT_MEMORY_CLOCK,
		    &data);
    asicInit.sASICInitClocks.ulDefaultMemoryClock = data.val;  /* in 10 Khz */
    data.exec.dataSpace = NULL;
    data.exec.index = 0x0;
    data.exec.pspace = &asicInit;
    xf86DrvMsg(handle->scrnIndex, X_INFO, "Calling ASIC Init\n");
    if (RHDAtomBIOSFunc(handle->scrnIndex, handle,
			ATOMBIOS_EXEC, &data) == ATOM_SUCCESS) {
	xf86DrvMsg(handle->scrnIndex, X_INFO, "ASIC_INIT Successful\n");
	return TRUE;
    }
    xf86DrvMsg(handle->scrnIndex, X_INFO, "ASIC_INIT Failed\n");
    return FALSE;
}
# endif

static atomBIOSHandlePtr
rhdInitAtomBIOS(int scrnIndex)
{
    RHDPtr rhdPtr = RHDPTR(xf86Screens[scrnIndex]);
    unsigned char *ptr;
    atomDataTablesPtr atomDataPtr;
    atomBIOSHandlePtr handle = NULL;
    unsigned int dummy;
    int BIOSImageSize = 0;

    RHDFUNCI(scrnIndex);

    if (rhdPtr->BIOSCopy) {
	xf86DrvMsg(scrnIndex,X_INFO,"Getting BIOS copy from INT10\n");
	ptr = rhdPtr->BIOSCopy;
	rhdPtr->BIOSCopy = NULL;

	BIOSImageSize = ptr[2] * 512;
	if (BIOSImageSize > legacyBIOSMax) {
	    xf86DrvMsg(scrnIndex,X_ERROR,"Invalid BIOS length field\n");
	    return NULL;
	}
    } else {
	if (!xf86IsEntityPrimary(rhdPtr->entityIndex)) {
	    int read_len;

	    BIOSImageSize = 1 << rhdPtr->PciInfo->biosSize;
	    if (!(ptr = xcalloc(1, BIOSImageSize))) {
		xf86DrvMsg(scrnIndex,X_ERROR,
			   "Cannot allocate %i bytes of memory "
			   "for BIOS image\n",BIOSImageSize);
		return NULL;
	    }
	    xf86DrvMsg(scrnIndex,X_INFO,"Getting BIOS copy from PCI ROM\n");

	    if ((read_len =
		 xf86ReadPciBIOS(0, rhdPtr->PciTag, -1, ptr, BIOSImageSize) < 0)) {
		xf86DrvMsg(scrnIndex,X_ERROR,
			   "Cannot read BIOS image\n");
		goto error;
	    } else if (read_len != BIOSImageSize)
		xf86DrvMsg(scrnIndex,X_WARNING,
			   "Read only %i of %i bytes of BIOS image\n",
			   read_len, BIOSImageSize);
	} else {
	    int read_len;
	    unsigned char tmp[32];
	    xf86DrvMsg(scrnIndex,X_INFO,"Getting BIOS copy from legacy VBIOS location\n");
	    if (xf86ReadBIOS(legacyBIOSLocation, 0, tmp, 32) < 0) {
		xf86DrvMsg(scrnIndex,X_ERROR,
			   "Cannot obtain POSTed BIOS header\n");
		return NULL;
	    }
	    BIOSImageSize = tmp[2] * 512;
	    if (BIOSImageSize > legacyBIOSMax) {
		xf86DrvMsg(scrnIndex,X_ERROR,"Invalid BIOS length field\n");
		return NULL;
	    }
	    if (!(ptr = xcalloc(1,BIOSImageSize))) {
		xf86DrvMsg(scrnIndex,X_ERROR,
			   "Cannot allocate %i bytes of memory "
			   "for BIOS image\n",BIOSImageSize);
		return NULL;
	    }
	    if ((read_len = xf86ReadBIOS(legacyBIOSLocation, 0, ptr, BIOSImageSize)
		 < 0)) {
		xf86DrvMsg(scrnIndex,X_ERROR,"Cannot read POSTed BIOS\n");
		goto error;
	    }
	}
    }
    if (!(atomDataPtr = xcalloc(1, sizeof(atomDataTables)))) {
	xf86DrvMsg(scrnIndex,X_ERROR,"Cannot allocate memory for "
		   "ATOM BIOS data tabes\n");
	goto error;
    }
    if (!rhdGetAtombiosDataTable(scrnIndex, ptr, atomDataPtr,BIOSImageSize))
	goto error1;
    if (!(handle = xcalloc(1, sizeof(atomBIOSHandle)))) {
	xf86DrvMsg(scrnIndex,X_ERROR,"Cannot allocate memory\n");
	goto error1;
    }
    handle->BIOSBase = ptr;
    handle->atomDataPtr = atomDataPtr;
    handle->scrnIndex = scrnIndex;
    handle->PciTag = rhdPtr->PciTag;
    handle->BIOSImageSize = BIOSImageSize;

# if ATOM_BIOS_PARSER
    /* Try to find out if BIOS has been posted (either by system or int10 */
    if (!rhdBIOSGetFbBaseAndSize(handle, &dummy, &dummy)) {
	/* run AsicInit */
	if (!rhdASICInit(handle))
	    xf86DrvMsg(scrnIndex, X_WARNING,
		       "%s: AsicInit failed. Won't be able to obtain in VRAM "
		       "FB scratch space\n",__func__);
    }
# endif
    return handle;

 error1:
    xfree(atomDataPtr);
 error:
    xfree(ptr);
    return NULL;
}

static void
rhdTearDownAtomBIOS(atomBIOSHandlePtr handle)
{
    RHDFUNC(handle);

    xfree(handle->BIOSBase);
    xfree(handle->atomDataPtr);
    if (handle->scratchBase) xfree(handle->scratchBase);
    xfree(handle);
}

static AtomBiosResult
rhdAtomBIOSVramInfoQuery(atomBIOSHandlePtr handle, AtomBiosFunc func,
			     CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;

    RHDFUNC(handle);

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

static
AtomBiosResult
rhdAtomBIOSTmdsInfoQuery(atomBIOSHandlePtr handle,
			 AtomBiosFunc func,  CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;
    int index = *val;

    atomDataPtr = handle->atomDataPtr;
    if (!rhdGetAtomBiosTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->TMDS_Info),
	    NULL,NULL,NULL)) {
	return ATOM_FAILED;
    }

    RHDFUNC(handle);

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

static DisplayModePtr
rhdLvdsTimings(atomBIOSHandlePtr handle, ATOM_DTD_FORMAT *dtd)
{
    DisplayModePtr mode;
#define NAME_LEN 16
    char name[NAME_LEN];

    RHDFUNC(handle);

    if (!(mode = (DisplayModePtr)xcalloc(1,sizeof(DisplayModeRec))))
	return NULL;

    mode->CrtcHDisplay = mode->HDisplay = dtd->usHActive;
    mode->CrtcVDisplay = mode->VDisplay = dtd->usVActive;
    mode->CrtcHBlankStart = dtd->usHActive + dtd->ucHBorder;
    mode->CrtcHBlankEnd = mode->CrtcHBlankStart + dtd->usHBlanking_Time;
    mode->CrtcHTotal = mode->HTotal = mode->CrtcHBlankEnd + dtd->ucHBorder;
    mode->CrtcVBlankStart = dtd->usVActive + dtd->ucVBorder;
    mode->CrtcVBlankEnd = mode->CrtcVBlankStart + dtd->usVBlanking_Time;
    mode->CrtcVTotal = mode->VTotal = mode->CrtcVBlankEnd + dtd->ucVBorder;
    mode->CrtcHSyncStart = mode->HSyncStart = dtd->usHActive + dtd->usHSyncOffset;
    mode->CrtcHSyncEnd = mode->HSyncEnd = mode->HSyncStart + dtd->usHSyncWidth;
    mode->CrtcVSyncStart = mode->VSyncStart = dtd->usVActive + dtd->usVSyncOffset;
    mode->CrtcVSyncEnd = mode->VSyncEnd = mode->VSyncStart + dtd->usVSyncWidth;

    mode->SynthClock = mode->Clock  = dtd->usPixClk * 10;

    mode->HSync = ((float) mode->Clock) / ((float)mode->HTotal);
    mode->VRefresh = (1000.0 * ((float) mode->Clock))
	/ ((float)(((float)mode->HTotal) * ((float)mode->VTotal)));

    xf86snprintf(name, NAME_LEN, "%dx%d",
		 mode->HDisplay, mode->VDisplay);
    mode->name = xstrdup(name);

    RHDDebug(handle->scrnIndex,"%s: LVDS Modeline: %s  "
	     "%2.d  %i (%i) %i %i (%i) %i  %i (%i) %i %i (%i) %i\n",
	     __func__, mode->name, mode->Clock,
	     mode->HDisplay, mode->CrtcHBlankStart, mode->HSyncStart, mode->CrtcHSyncEnd,
	     mode->CrtcHBlankEnd, mode->HTotal,
	     mode->VDisplay, mode->CrtcVBlankStart, mode->VSyncStart, mode->VSyncEnd,
	     mode->CrtcVBlankEnd, mode->VTotal);

    return mode;
}

static unsigned char*
rhdLvdsDDC(atomBIOSHandlePtr handle, CARD32 offset, unsigned char *record)
{
    unsigned char *EDID;

    RHDFUNC(handle);

    while (*record != ATOM_RECORD_END_TYPE) {

	switch (*record) {
	    case LCD_MODE_PATCH_RECORD_MODE_TYPE:
		offset += sizeof(ATOM_PATCH_RECORD_MODE);
		if (offset > handle->BIOSImageSize) break;
		record += sizeof(ATOM_PATCH_RECORD_MODE);
		break;

	    case LCD_RTS_RECORD_TYPE:
		offset += sizeof(ATOM_LCD_RTS_RECORD);
		if (offset > handle->BIOSImageSize) break;
		record += sizeof(ATOM_LCD_RTS_RECORD);
		break;

	    case LCD_CAP_RECORD_TYPE:
		offset += sizeof(ATOM_LCD_MODE_CONTROL_CAP);
		if (offset > handle->BIOSImageSize) break;
		record += sizeof(ATOM_LCD_MODE_CONTROL_CAP);
		break;

	    case LCD_FAKE_EDID_PATCH_RECORD_TYPE:
		offset += sizeof(ATOM_FAKE_EDID_PATCH_RECORD);
		/* check if the structure still fully lives in the BIOS image */
		if (offset > handle->BIOSImageSize) break;
		offset += ((ATOM_FAKE_EDID_PATCH_RECORD*)record)->ucFakeEDIDLength
		    - sizeof(UCHAR);
		if (offset > handle->BIOSImageSize) break;
		/* dup string as we free it later */
		if (!(EDID = (unsigned char *)xalloc(
			  ((ATOM_FAKE_EDID_PATCH_RECORD*)record)->ucFakeEDIDLength)))
		    return NULL;
		memcpy(EDID,&((ATOM_FAKE_EDID_PATCH_RECORD*)record)->ucFakeEDIDString,
		       ((ATOM_FAKE_EDID_PATCH_RECORD*)record)->ucFakeEDIDLength);

		/* for testing */
		{
		    xf86MonPtr mon = xf86InterpretEDID(handle->scrnIndex,EDID);
		    xf86PrintEDID(mon);
		    xfree(mon);
		}
		return EDID;

	    case LCD_PANEL_RESOLUTION_RECORD_TYPE:
		offset += sizeof(ATOM_PANEL_RESOLUTION_PATCH_RECORD);
		if (offset > handle->BIOSImageSize) break;
		record += sizeof(ATOM_PANEL_RESOLUTION_PATCH_RECORD);
		break;

	    default:
		xf86DrvMsg(handle->scrnIndex, X_ERROR,
			   "%s: unknown record type: %x\n",__func__,*record);
		return NULL;
	}
    }

    return NULL;
}

static AtomBiosResult
rhdLvdsGetTimings(atomBIOSHandlePtr handle,  rhdPanelModePtr *ptr)

{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;
    rhdPanelModePtr m;
    unsigned long offset;

    RHDFUNC(handle);

    atomDataPtr = handle->atomDataPtr;

    if (!rhdGetAtomBiosTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->LVDS_Info.base),
	    &frev,&crev,NULL)) {
	return ATOM_FAILED;
    }

    switch (crev) {

	case 1:
	    if (!(m = (rhdPanelModePtr)xcalloc(1,sizeof(rhdPanelModeRec))))
		return ATOM_FAILED;
	    m->mode = rhdLvdsTimings(handle, &atomDataPtr->LVDS_Info
				       .LVDS_Info->sLCDTiming);
	    m->EDID = NULL;
	    break;

	case 2:
	    if (!(m = (rhdPanelModePtr)xalloc(sizeof(rhdPanelModeRec))))
		return ATOM_FAILED;
	    m->mode = rhdLvdsTimings(handle, &atomDataPtr->LVDS_Info
				     .LVDS_Info_v12->sLCDTiming);

	    offset = (unsigned long)&atomDataPtr->LVDS_Info.base
		- (unsigned long)handle->BIOSBase
		+ atomDataPtr->LVDS_Info.LVDS_Info_v12->usExtInfoTableOffset;

	    m->EDID = rhdLvdsDDC(handle, offset,
				 (unsigned char *)&atomDataPtr->LVDS_Info.base
				 + atomDataPtr->LVDS_Info
				 .LVDS_Info_v12->usExtInfoTableOffset);
	    break;

	default:
	    return ATOM_NOT_IMPLEMENTED;
    }

    *ptr = m;

    return ATOM_SUCCESS;
}

static AtomBiosResult
rhdAtomBIOSLvdsInfoQuery(atomBIOSHandlePtr handle,
			 AtomBiosFunc func,  CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;

    RHDFUNC(handle);

    atomDataPtr = handle->atomDataPtr;

    if (!rhdGetAtomBiosTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->LVDS_Info.base),
	    &frev,&crev,NULL)) {
	return ATOM_FAILED;
    }

    switch (crev) {
	case 1:
	    switch (func) {
		case ATOM_LVDS_SUPPORTED_REFRESH_RATE:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info->usSupportedRefreshRate;
		    break;
		case ATOM_LVDS_OFF_DELAY:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info->usOffDelayInMs;
		    break;
		case ATOM_LVDS_SEQ_DIG_ONTO_DE:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info->ucPowerSequenceDigOntoDEin10Ms;
		    break;
		case ATOM_LVDS_SEQ_DE_TO_BL:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info->ucPowerSequenceDEtoBLOnin10Ms;
		    break;
		case     ATOM_LVDS_MISC:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info->ucLVDS_Misc;
		    break;
		default:
		    return ATOM_NOT_IMPLEMENTED;
	    }
	    break;
	case 2:
	    switch (func) {
		case ATOM_LVDS_SUPPORTED_REFRESH_RATE:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info_v12->usSupportedRefreshRate;
		    break;
		case ATOM_LVDS_OFF_DELAY:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info_v12->usOffDelayInMs;
		    break;
		case ATOM_LVDS_SEQ_DIG_ONTO_DE:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info_v12->ucPowerSequenceDigOntoDEin10Ms;
		    break;
		case ATOM_LVDS_SEQ_DE_TO_BL:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info_v12->ucPowerSequenceDEtoBLOnin10Ms;
		    break;
		case ATOM_LVDS_MISC:
		    *val = atomDataPtr->LVDS_Info
			.LVDS_Info_v12->ucLVDS_Misc;
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

static AtomBiosResult
rhdAtomBIOSFirmwareInfoQuery(atomBIOSHandlePtr handle, AtomBiosFunc func, CARD32 *val)
{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;

    RHDFUNC(handle);

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

#define Limit(n,max,name) ((n >= max) ? ( \
     xf86DrvMsg(handle->scrnIndex,X_ERROR,"%s: %s %i exceeds maximum %i\n", \
		__func__,name,n,max), TRUE) : FALSE)

const static struct _rhd_connector_objs
{
    char *name;
    rhdConnectorType con;
} rhd_connector_objs[] = {
    { "NONE", RHD_CONNECTOR_NONE },
    { "SINGLE_LINK_DVI_I", RHD_CONNECTOR_DVI },
    { "DUAL_LINK_DVI_I", RHD_CONNECTOR_DVI_DUAL },
    { "SINGLE_LINK_DVI_D", RHD_CONNECTOR_DVI },
    { "DUAL_LINK_DVI_D", RHD_CONNECTOR_DVI_DUAL },
    { "vga", RHD_CONNECTOR_VGA },
    { "COMPOSITE", RHD_CONNECTOR_TV },
    { "SVIDEO", RHD_CONNECTOR_TV, },
    { "D_CONNECTOR", RHD_CONNECTOR_NONE, },
    { "9PIN_DIN", RHD_CONNECTOR_NONE },
    { "scart", RHD_CONNECTOR_TV },
    { "HDMI_TYPE_A", RHD_CONNECTOR_NONE },
    { "HDMI_TYPE_B", RHD_CONNECTOR_NONE },
    { "HDMI_TYPE_B", RHD_CONNECTOR_NONE },
    { "LVDS", RHD_CONNECTOR_PANEL },
    { "7PIN_DIN", RHD_CONNECTOR_TV },
    { "PCIE_CONNECTOR", RHD_CONNECTOR_NONE },
    { "CROSSFIRE", RHD_CONNECTOR_NONE },
    { "HARDCODE_DVI", RHD_CONNECTOR_NONE },
    { "DISPLAYPORT", RHD_CONNECTOR_NONE}
};
const static int n_rhd_connector_objs = sizeof (rhd_connector_objs) / sizeof(struct _rhd_connector_objs);

const static struct _rhd_encoders
{
    char *name;
    rhdOutputType ot;
} rhd_encoders[] = {
    { "NONE", RHD_OUTPUT_NONE },
    { "INTERNAL_LVDS", RHD_OUTPUT_LVDS },
    { "INTERNAL_TMDS1", RHD_OUTPUT_TMDSA },
    { "INTERNAL_TMDS2", RHD_OUTPUT_TMDSB },
    { "INTERNAL_DAC1", RHD_OUTPUT_DACA },
    { "INTERNAL_DAC2", RHD_OUTPUT_DACB },
    { "INTERNAL_SDVOA", RHD_OUTPUT_NONE },
    { "INTERNAL_SDVOB", RHD_OUTPUT_NONE },
    { "SI170B", RHD_OUTPUT_NONE },
    { "CH7303", RHD_OUTPUT_NONE },
    { "CH7301", RHD_OUTPUT_NONE },
    { "INTERNAL_DVO1", RHD_OUTPUT_NONE },
    { "EXTERNAL_SDVOA", RHD_OUTPUT_NONE },
    { "EXTERNAL_SDVOB", RHD_OUTPUT_NONE },
    { "TITFP513", RHD_OUTPUT_NONE },
    { "INTERNAL_LVTM1", RHD_OUTPUT_LVTMA },
    { "VT1623", RHD_OUTPUT_NONE },
    { "HDMI_SI1930", RHD_OUTPUT_NONE },
    { "HDMI_INTERNAL", RHD_OUTPUT_NONE },
    { "INTERNAL_KLDSCP_TMDS1", RHD_OUTPUT_TMDSA },
    { "INTERNAL_KLSCP_DVO1", RHD_OUTPUT_NONE },
    { "INTERNAL_KLDSCP_DAC1", RHD_OUTPUT_DACA },
    { "INTERNAL_KLDSCP_DAC2", RHD_OUTPUT_DACB },
    { "SI178", RHD_OUTPUT_NONE },
    {"MVPU_FPGA", RHD_OUTPUT_NONE },
    { "INTERNAL_DDI", RHD_OUTPUT_NONE },
    { "VT1625", RHD_OUTPUT_NONE },
    { "HDMI_SI1932", RHD_OUTPUT_NONE },
    { "AN9801", RHD_OUTPUT_NONE },
    { "DP501",  RHD_OUTPUT_NONE },
};
const static int n_rhd_encoders = sizeof (rhd_encoders) / sizeof(struct _rhd_encoders);

const static struct _rhd_connectors
{
    char *name;
    rhdConnectorType con;
    Bool dual;
} rhd_connectors[] = {
    {"NONE", RHD_CONNECTOR_NONE, FALSE },
    {"VGA", RHD_CONNECTOR_VGA, FALSE },
    {"DVI-I", RHD_CONNECTOR_DVI, TRUE },
    {"DVI-D", RHD_CONNECTOR_DVI, FALSE },
    {"DVI-A", RHD_CONNECTOR_DVI, FALSE },
    {"SVIDIO", RHD_CONNECTOR_TV, FALSE },
    {"COMPOSITE", RHD_CONNECTOR_TV, FALSE },
    {"PANEL", RHD_CONNECTOR_PANEL, FALSE },
    {"DIGITAL_LINK", RHD_CONNECTOR_NONE, FALSE },
    {"SCART", RHD_CONNECTOR_TV, FALSE },
    {"HDMI Type A", RHD_CONNECTOR_NONE, FALSE },
    {"HDMI Type B", RHD_CONNECTOR_NONE, FALSE },
    {"UNKNOWN", RHD_CONNECTOR_NONE, FALSE },
    {"UNKNOWN", RHD_CONNECTOR_NONE, FALSE },
    {"DVI+DIN", RHD_CONNECTOR_NONE, FALSE }
};
const static int n_rhd_connectors = sizeof(rhd_connectors) / sizeof(struct _rhd_connectors);

const static struct _rhd_devices
{
    char *name;
    rhdOutputType ot;
} rhd_devices[] = {
    {" CRT1", RHD_OUTPUT_NONE },
    {" LCD1", RHD_OUTPUT_LVTMA },
    {" TV1", RHD_OUTPUT_NONE },
    {" DFP1", RHD_OUTPUT_TMDSA },
    {" CRT2", RHD_OUTPUT_NONE },
    {" LCD2", RHD_OUTPUT_LVTMA },
    {" TV2", RHD_OUTPUT_NONE },
    {" DFP2", RHD_OUTPUT_LVTMA },
    {" CV", RHD_OUTPUT_NONE },
    {" DFP3", RHD_OUTPUT_LVTMA }
};
const static int n_rhd_devices = sizeof(rhd_devices) / sizeof(struct _rhd_devices);

const static rhdDDC hwddc[] = { RHD_DDC_0, RHD_DDC_1, RHD_DDC_2, RHD_DDC_3 };
const static int n_hwddc = sizeof(hwddc) / sizeof(rhdDDC);

const static rhdOutputType acc_dac[] = { RHD_OUTPUT_NONE, RHD_OUTPUT_DACA,
				  RHD_OUTPUT_DACB, RHD_OUTPUT_DAC_EXTERNAL };
const static int n_acc_dac = sizeof(acc_dac) / sizeof (rhdOutputType);

/*
 *
 */
static Bool
rhdInterpretObjectID(atomBIOSHandlePtr handle,
		     CARD16 id, CARD8 *obj_type, CARD8 *obj_id,
		     CARD8 *num, char **name)
{
    *obj_id = (id & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
    *num = (id & ENUM_ID_MASK) >> ENUM_ID_SHIFT;
    *obj_type = (id & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

    *name = NULL;

    switch (*obj_type) {
	case GRAPH_OBJECT_TYPE_CONNECTOR:
	    if (!Limit(*obj_id, n_rhd_connector_objs, "obj_id"))
		*name = rhd_connector_objs[*obj_id].name;
	    break;
	case GRAPH_OBJECT_TYPE_ENCODER:
	    if (!Limit(*obj_id, n_rhd_encoders, "obj_id"))
		*name = rhd_encoders[*obj_id].name;
	    break;
	default:
	    break;
    }
    return TRUE;
}

/*
 *
 */
static void
rhdDDCFromI2CRecord(atomBIOSHandlePtr handle,
		    ATOM_I2C_RECORD *Record, rhdDDC *DDC)
{
    RHDDebug(handle->scrnIndex,
	     "   %s:  I2C Record: %s[%x] EngineID: %x I2CAddr: %x\n",
	     __func__,
	     Record->sucI2cId.bfHW_Capable ? "HW_Line" : "GPIO_ID",
	     Record->sucI2cId.bfI2C_LineMux,
	     Record->sucI2cId.bfHW_EngineID,
	     Record->ucI2CAddr);

    if (!*(unsigned char *)&(Record->sucI2cId))
	*DDC = RHD_DDC_NONE;
    else {

	if (Record->ucI2CAddr != 0)
	    return;

	if (Record->sucI2cId.bfHW_Capable) {

	    *DDC = (rhdDDC)Record->sucI2cId.bfI2C_LineMux;
	    if (*DDC >= RHD_DDC_MAX)
		*DDC = RHD_DDC_NONE;

	} else {
	    *DDC = RHD_DDC_GPIO;
	    /* add GPIO pin parsing */
	}
    }
}

/*
 *
 */
static void
rhdParseGPIOLutForHPD(atomBIOSHandlePtr handle,
		CARD8 pinID, rhdHPD *HPD)
{
    atomDataTablesPtr atomDataPtr;
    ATOM_GPIO_PIN_LUT *gpio_pin_lut;
    int i;

    RHDFUNC(handle);

    atomDataPtr = handle->atomDataPtr;

    *HPD = RHD_HPD_NONE;

    if (!rhdGetAtomBiosTableRevisionAndSize(
	    &atomDataPtr->GPIO_Pin_LUT->sHeader, NULL, NULL, NULL)) {
	xf86DrvMsg(handle->scrnIndex, X_ERROR,
		   "%s: No valid GPIO pin LUT in AtomBIOS\n",__func__);
	return;
    }
    gpio_pin_lut = atomDataPtr->GPIO_Pin_LUT;

    /* we count up to 10 because we don't know better :{ */
    for (i = 0; i < 10; i++) {
	if (gpio_pin_lut->asGPIO_Pin[i].ucGPIO_ID  == pinID) {

	    RHDDebug(handle->scrnIndex,
		     "   %s: GPIO PinID: %i Index: %x Shift: %i\n",
		     __func__,
		     pinID,
		     gpio_pin_lut->asGPIO_Pin[i].usGpioPin_AIndex,
		     gpio_pin_lut->asGPIO_Pin[i].ucGpioPinBitShift);

	    /* grr... map backwards: register indices -> line numbers */
	    if (gpio_pin_lut->asGPIO_Pin[i].usGpioPin_AIndex
		== (DC_GPIO_HPD_A >> 2)) {
		switch (gpio_pin_lut->asGPIO_Pin[i].ucGpioPinBitShift) {
		    case 0:
			*HPD = RHD_HPD_0;
			break;
		    case 8:
			*HPD = RHD_HPD_1;
			break;
		    case 16:
			*HPD = RHD_HPD_2;
			break;
		}
	    }
	}
    }
}

/*
 *
 */
static void
rhdHPDFromRecord(atomBIOSHandlePtr handle,
		   ATOM_HPD_INT_RECORD *Record, rhdHPD *HPD)
{
    RHDDebug(handle->scrnIndex,
	     "   %s:  HPD Record: GPIO ID: %x Plugged_PinState: %x\n",
	     __func__,
	     Record->ucHPDIntGPIOID,
	     Record->ucPluggged_PinState);
    rhdParseGPIOLutForHPD(handle, Record->ucHPDIntGPIOID, HPD);
}

/*
 *
 */
static char *
rhdDeviceTagsFromRecord(atomBIOSHandlePtr handle,
			ATOM_CONNECTOR_DEVICE_TAG_RECORD *Record)
{
    int i, j, k;
    char *devices;

    RHDFUNC(handle);

    RHDDebug(handle->scrnIndex,"   NumberOfDevice: %i\n",
	     Record->ucNumberOfDevice);

    if (!Record->ucNumberOfDevice) return NULL;

    devices = (char *)xcalloc(Record->ucNumberOfDevice * 4 + 1,1);

    for (i = 0; i < Record->ucNumberOfDevice; i++) {
	k = 0;
	j = Record->asDeviceTag[i].usDeviceID;

	while (!(j & 0x1)) { j >>= 1; k++; };

	if (!Limit(k,n_rhd_devices,"usDeviceID"))
	    strcat(devices, rhd_devices[k].name);
    }

    RHDDebug(handle->scrnIndex,"   Devices:%s\n",devices);

    return devices;
}

/*
 *
 */
static AtomBiosResult
rhdConnectorTableFromObjectHeader(atomBIOSHandlePtr handle,
			      rhdConnectorTablePtr *ptr)
{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;
    ATOM_CONNECTOR_OBJECT_TABLE *con_obj;
    rhdConnectorTablePtr cp;
    unsigned long object_header_end;
    int ncon = 0;
    int i,j;
    unsigned short object_header_size;

    RHDFUNC(handle);

    atomDataPtr = handle->atomDataPtr;

    if (!rhdGetAtomBiosTableRevisionAndSize(
	    &atomDataPtr->Object_Header->sHeader,
	    &crev,&frev,&object_header_size)) {
	return ATOM_NOT_IMPLEMENTED;
    }

    if (crev < 2) /* don't bother with anything below rev 2 */
	return ATOM_NOT_IMPLEMENTED;

    if (!(cp = (rhdConnectorTablePtr)xcalloc(sizeof(struct rhdConnectorTable),
					 RHD_CONNECTORS_MAX)))
	return ATOM_FAILED;

    object_header_end =
	atomDataPtr->Object_Header->usConnectorObjectTableOffset
	+ object_header_size;

    RHDDebug(handle->scrnIndex,"ObjectTable - size: %u, BIOS - size: %u "
	     "TableOffset: %u object_header_end: %u\n",
	     object_header_size, handle->BIOSImageSize,
	     atomDataPtr->Object_Header->usConnectorObjectTableOffset,
	     object_header_end);

    if ((object_header_size > handle->BIOSImageSize)
	|| (atomDataPtr->Object_Header->usConnectorObjectTableOffset
	    > handle->BIOSImageSize)
	|| object_header_end > handle->BIOSImageSize) {
	xf86DrvMsg(handle->scrnIndex, X_ERROR,
		   "%s: Object table information is bogus\n",__func__);
	return ATOM_FAILED;
    }

    if (((unsigned long)&atomDataPtr->Object_Header->sHeader
	 + object_header_end) >  ((unsigned long)handle->BIOSBase
		     + handle->BIOSImageSize)) {
	xf86DrvMsg(handle->scrnIndex, X_ERROR,
		   "%s: Object table extends beyond BIOS Image\n",__func__);
	return ATOM_FAILED;
    }

    con_obj = (ATOM_CONNECTOR_OBJECT_TABLE *)
	((char *)&atomDataPtr->Object_Header->sHeader +
	 atomDataPtr->Object_Header->usConnectorObjectTableOffset);

    for (i = 0; i < con_obj->ucNumberOfObjects; i++) {
	ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT *SrcDstTable;
	ATOM_COMMON_RECORD_HEADER *Record;
	int record_base;
	CARD8 obj_type, obj_id, num;
	char *name;
	int nout = 0;

	rhdInterpretObjectID(handle, con_obj->asObjects[i].usObjectID,
			     &obj_type, &obj_id, &num, &name);

	RHDDebug(handle->scrnIndex, "Object: ID: %x name: %s type: %x id: %x\n",
		 con_obj->asObjects[i].usObjectID, name ? name : "",
		 obj_type, obj_id);


	if (obj_type != GRAPH_OBJECT_TYPE_CONNECTOR)
	    continue;

	SrcDstTable = (ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT *)
	    ((char *)&atomDataPtr->Object_Header->sHeader
	     + con_obj->asObjects[i].usSrcDstTableOffset);

	if (con_obj->asObjects[i].usSrcDstTableOffset
	    + (SrcDstTable->ucNumberOfSrc
	       * sizeof(ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT))
	    > handle->BIOSImageSize) {
	    xf86DrvMsg(handle->scrnIndex, X_ERROR, "%s: SrcDstTable[%i] extends "
		       "beyond Object_Header table\n",__func__,i);
	    continue;
	}

	cp[ncon].Type = rhd_connector_objs[obj_id].con;
	cp[ncon].Name = name;

	for (j = 0; j < SrcDstTable->ucNumberOfSrc; j++) {
	    CARD8 stype, sobj_id, snum;
	    char *sname;

	    rhdInterpretObjectID(handle, SrcDstTable->usSrcObjectID[j],
				 &stype, &sobj_id, &snum, &sname);

	    RHDDebug(handle->scrnIndex, " * SrcObject: ID: %x name: %s\n",
		     SrcDstTable->usSrcObjectID[j], sname);

	    cp[ncon].Output[nout] = rhd_encoders[sobj_id].ot;
	    if (++nout >= MAX_OUTPUTS_PER_CONNECTOR)
		break;
	}

	Record = (ATOM_COMMON_RECORD_HEADER *)
	    ((char *)&atomDataPtr->Object_Header->sHeader
	     + con_obj->asObjects[i].usRecordOffset);

	record_base = con_obj->asObjects[i].usRecordOffset;

	while (Record->ucRecordType != 0xff) {
	    char *taglist;

	    if ((record_base += Record->ucRecordSize)
		> object_header_size) {
		xf86DrvMsg(handle->scrnIndex, X_ERROR,
			   "%s: Object Records extend beyond Object Table\n",
			   __func__);
		break;
	    }

	    RHDDebug(handle->scrnIndex, " - Record Type: %x\n",
		     Record->ucRecordType);

	    switch (Record->ucRecordType) {

		case ATOM_I2C_RECORD_TYPE:
		    rhdDDCFromI2CRecord(handle,
					(ATOM_I2C_RECORD *)Record,
					&cp[ncon].DDC);
		    break;

		case ATOM_HPD_INT_RECORD_TYPE:
		    rhdHPDFromRecord(handle,
				     (ATOM_HPD_INT_RECORD *)Record,
				     &cp[ncon].HPD);
		    break;

		case ATOM_CONNECTOR_DEVICE_TAG_RECORD_TYPE:
		    taglist = rhdDeviceTagsFromRecord(handle,
						      (ATOM_CONNECTOR_DEVICE_TAG_RECORD *)Record);
		    if (taglist) {
			cp[ncon].Name = RhdCombineStrings(cp[ncon].Name,taglist);
			xfree(taglist);
		    }
		    break;

		default:
		    break;
	    }

	    Record = (ATOM_COMMON_RECORD_HEADER*)
		((char *)Record + Record->ucRecordSize);

	}

	if ((++ncon) == RHD_CONNECTORS_MAX)
	    break;
    }
    *ptr = cp;

    RhdPrintConnectorTable(handle->scrnIndex, cp);

    return ATOM_SUCCESS;
}

/*
 *
 */
static AtomBiosResult
rhdConnectorTableFromSupportedDevices(atomBIOSHandlePtr handle,
				      rhdConnectorTablePtr *ptr)
{
    atomDataTablesPtr atomDataPtr;
    CARD8 crev, frev;
    rhdConnectorTablePtr cp;
    struct {
	rhdOutputType ot;
	rhdConnectorType con;
	rhdDDC ddc;
	rhdHPD hpd;
	Bool dual;
	char *name;
	char *outputName;
    } devices[ATOM_MAX_SUPPORTED_DEVICE];
    int ncon = 0;
    int n;

    RHDFUNC(handle);

    atomDataPtr = handle->atomDataPtr;

    if (!rhdGetAtomBiosTableRevisionAndSize(
	    &(atomDataPtr->SupportedDevicesInfo.SupportedDevicesInfo->sHeader),
	    &crev,&frev,NULL)) {
	return ATOM_NOT_IMPLEMENTED;
    }

    if (!(cp = (rhdConnectorTablePtr)xcalloc(RHD_CONNECTORS_MAX,
					 sizeof(struct rhdConnectorTable))))
	return ATOM_FAILED;

    for (n = 0; n < ATOM_MAX_SUPPORTED_DEVICE; n++) {
	ATOM_CONNECTOR_INFO_I2C ci
	    = atomDataPtr->SupportedDevicesInfo.SupportedDevicesInfo->asConnInfo[n];

	devices[n].ot = RHD_OUTPUT_NONE;

	if (Limit(ci.sucConnectorInfo.sbfAccess.bfConnectorType,
		  n_rhd_connectors, "bfConnectorType"))
	    continue;

	devices[n].con
	    = rhd_connectors[ci.sucConnectorInfo.sbfAccess.bfConnectorType].con;
	if (devices[n].con == RHD_CONNECTOR_NONE)
	    continue;

	devices[n].dual
	    = rhd_connectors[ci.sucConnectorInfo.sbfAccess.bfConnectorType].dual;
	devices[n].name
	    = rhd_connectors[ci.sucConnectorInfo.sbfAccess.bfConnectorType].name;

	RHDDebug(handle->scrnIndex,"AtomBIOS Connector[%i]: %s Device:%s ",n,
		 rhd_connectors[ci.sucConnectorInfo
				.sbfAccess.bfConnectorType].name,
		 rhd_devices[n].name);

	devices[n].outputName = rhd_devices[n].name;

	if (!Limit(ci.sucConnectorInfo.sbfAccess.bfAssociatedDAC,
		   n_acc_dac, "bfAssociatedDAC")) {
	    if ((devices[n].ot
		 = acc_dac[ci.sucConnectorInfo.sbfAccess.bfAssociatedDAC])
		== RHD_OUTPUT_NONE) {
		devices[n].ot = rhd_devices[n].ot;
	    }
	} else
	    devices[n].ot = RHD_OUTPUT_NONE;

	RHDDebugCont("Output: %x ",devices[n].ot);

	if (ci.sucI2cId.sbfAccess.bfHW_Capable) {

	    RHDDebugCont("HW DDC %i ",
			 ci.sucI2cId.sbfAccess.bfI2C_LineMux);

	    if (Limit(ci.sucI2cId.sbfAccess.bfI2C_LineMux,
		      n_hwddc, "bfI2C_LineMux"))
		devices[n].ddc = RHD_DDC_NONE;
	    else
		devices[n].ddc = hwddc[ci.sucI2cId.sbfAccess.bfI2C_LineMux];

	} else if (ci.sucI2cId.sbfAccess.bfI2C_LineMux) {

	    RHDDebugCont("GPIO DDC ");
	    devices[n].ddc = RHD_DDC_GPIO;

	    /* add support for GPIO line */
	} else {

	    RHDDebugCont("NO DDC ");
	    devices[n].ddc = RHD_DDC_NONE;

	}

	if (crev > 1) {
	    ATOM_CONNECTOR_INC_SRC_BITMAP isb
		= atomDataPtr->SupportedDevicesInfo
		.SupportedDevicesInfo_HD->asIntSrcInfo[n];

	    switch (isb.ucIntSrcBitmap) {
		case 0x4:
		    RHDDebugCont("HPD 0\n");
		    devices[n].hpd = RHD_HPD_0;
		    break;
		case 0xa:
		    RHDDebugCont("HPD 1\n");
		    devices[n].hpd = RHD_HPD_1;
		    break;
		default:
		    RHDDebugCont("NO HPD\n");
		    devices[n].hpd = RHD_HPD_NONE;
		    break;
	    }
	} else {
	    RHDDebugCont("NO HPD\n");
	    devices[n].hpd = RHD_HPD_NONE;
	}
    }
    /* sort devices for connectors */
    for (n = 0; n < ATOM_MAX_SUPPORTED_DEVICE; n++) {
	int i;

	if (devices[n].ot == RHD_OUTPUT_NONE)
	    continue;
	if (devices[n].con == RHD_CONNECTOR_NONE)
	    continue;

	cp[ncon].DDC = devices[n].ddc;
	cp[ncon].HPD = devices[n].hpd;
	cp[ncon].Output[0] = devices[n].ot;
	cp[ncon].Output[1] = RHD_OUTPUT_NONE;
	cp[ncon].Type = devices[n].con;
	cp[ncon].Name = RhdCombineStrings(devices[n].name,devices[n].outputName);

	if (devices[n].dual) {
	    if (devices[n].ddc == RHD_DDC_NONE)
		xf86DrvMsg(handle->scrnIndex, X_ERROR,
			   "No DDC channel for device %s found."
			   " Cannot find matching device.\n",devices[n].name);
	    else {
		for (i = n + 1; i < ATOM_MAX_SUPPORTED_DEVICE; i++) {
		    char *tmp;

		    if (!devices[i].dual)
			continue;

		    if (devices[n].ddc != devices[i].ddc)
			continue;

		    if (((devices[n].ot == RHD_OUTPUT_DACA
			  || devices[n].ot == RHD_OUTPUT_DACB)
			 && (devices[i].ot == RHD_OUTPUT_LVTMA
			     || devices[i].ot == RHD_OUTPUT_TMDSA))
			|| ((devices[i].ot == RHD_OUTPUT_DACA
			     || devices[i].ot == RHD_OUTPUT_DACB)
			    && (devices[n].ot == RHD_OUTPUT_LVTMA
				|| devices[n].ot == RHD_OUTPUT_TMDSA))) {

			cp[ncon].Output[1] = devices[i].ot;

			if (cp[ncon].HPD == RHD_HPD_NONE)
			    cp[ncon].HPD = devices[i].hpd;

			tmp = RhdCombineStrings(cp[ncon].Name,devices[i].outputName);
			xfree(cp[ncon].Name); cp[ncon].Name = tmp;

			devices[i].ot = RHD_OUTPUT_NONE; /* zero the device */
		    }
		}
	    }
	}

	if ((++ncon) == RHD_CONNECTORS_MAX)
	    break;
    }
    *ptr = cp;

    RhdPrintConnectorTable(handle->scrnIndex, cp);

    return ATOM_SUCCESS;
}

/*
 *
 */
static AtomBiosResult
rhdConnectorTable(atomBIOSHandlePtr handle, rhdConnectorTablePtr *conp)
{

    if (rhdConnectorTableFromObjectHeader(handle,conp) == ATOM_SUCCESS)
	return ATOM_SUCCESS;
    else
	return rhdConnectorTableFromSupportedDevices(handle,conp);
}

# ifdef ATOM_BIOS_PARSER
static Bool
rhdAtomExec (atomBIOSHandlePtr handle, int index, void *pspace,  pointer *dataSpace)
{
    RHDPtr rhdPtr = RHDPTRI(handle);
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
# endif

AtomBiosResult
RHDAtomBIOSFunc(int scrnIndex, atomBIOSHandlePtr handle, AtomBiosFunc func,
		AtomBIOSArgPtr data)
{
    AtomBiosResult ret = ATOM_NOT_IMPLEMENTED;

# define do_return(x) { \
        if (func < sizeof(AtomBIOSFuncStr)) \
	  xf86DrvMsgVerb(scrnIndex, (x == ATOM_SUCCESS) ? 7 : 1,	\
		           (x == ATOM_SUCCESS) ? X_INFO : X_WARNING,	\
		           "Call to %s %s\n", (func < ATOM_QUERY_FUNCS) \
                           ? AtomBIOSFuncStr[func] : "AtomBIOSQuery", \
		           (x == ATOM_SUCCESS) ? "succeeded" : "FAILED"); \
	    return (x); \
    }
    RHDFUNCI(scrnIndex);
    assert (sizeof(AtomBIOSQueryStr) == (FUNC_END - ATOM_QUERY_FUNCS + 1));

    if (func == ATOMBIOS_INIT) {
	if (!(data->atomp = rhdInitAtomBIOS(scrnIndex)))
	    do_return(ATOM_FAILED);
	do_return(ATOM_SUCCESS);
    }
    if (!handle)
	do_return(ATOM_FAILED);
    if (func == ATOMBIOS_ALLOCATE_FB_SCRATCH) {
        if (rhdAtomBIOSAllocateFbScratch(handle, &data->fb.start, &data->fb.size)) {
	    do_return(ATOM_SUCCESS);
	} else {
	    do_return(ATOM_FAILED);
	}
    }
    if (func == ATOMBIOS_TEARDOWN) {
	rhdTearDownAtomBIOS(handle);
	do_return(ATOM_SUCCESS);
    }
    if (func == ATOMBIOS_GET_CONNECTORS) {
	rhdConnectorTablePtr ptr = NULL;
	ret = rhdConnectorTable(handle, &ptr);
	data->ptr = ptr;
	do_return(ret);
    }
    if (func == ATOMBIOS_GET_PANEL_TIMINGS) {
	ret = rhdLvdsGetTimings(handle, &data->panel);
	do_return(ret);
    }
# ifdef ATOM_BIOS_PARSER
    if (func == ATOMBIOS_EXEC) {
	if (!rhdAtomExec(handle, data->exec.index,
			 data->exec.pspace, data->exec.dataSpace)) {
	    do_return(ATOM_FAILED);
	} else {
	    do_return(ATOM_SUCCESS);
	}
    } else
# endif
	if (func >= ATOM_QUERY_FUNCS && func < ATOM_VRAM_QUERIES) {
	    ret = rhdAtomBIOSFirmwareInfoQuery(handle, func, &data->val);
	} else if (func < ATOM_TMDS_QUERIES) {
	    ret = rhdAtomBIOSVramInfoQuery(handle, func, &data->val);
	} else if (func < ATOM_LVDS_QUERIES) {
	    ret = rhdAtomBIOSTmdsInfoQuery(handle, func, &data->val);
	} else if (func < FUNC_END ){
	    ret = rhdAtomBIOSLvdsInfoQuery(handle, func, &data->val);
	} else {
	    xf86DrvMsg(scrnIndex,X_INFO,"%s: Received unknown query\n",__func__);
	    return ATOM_NOT_IMPLEMENTED;
	}

    if (ret == ATOM_SUCCESS)
	xf86DrvMsg(scrnIndex,X_INFO,"%s: %i\n",
		   AtomBIOSQueryStr[func - ATOM_QUERY_FUNCS], (unsigned int)data->val);
    else
	xf86DrvMsg(scrnIndex,X_INFO,"Query for %s: %s\n",
		   AtomBIOSQueryStr[func - ATOM_QUERY_FUNCS],
		   ret == ATOM_FAILED ? "failed" : "not implemented");
    return ret;

}

# ifdef ATOM_BIOS_PARSER
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

    DEBUGP(xf86DrvMsg(((atomBIOSHandlePtr)CAIL)->scrnIndex,X_INFO,"Delay %i usec\n",delay));
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
	    RHDPTRI((atomBIOSHandlePtr)CAIL)->FbBase;
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
	    RHDPTRI((atomBIOSHandlePtr)CAIL)->FbBase;
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

# endif

#else /* ATOM_BIOS */

AtomBiosResult
RHDAtomBIOSFunc(int scrnIndex, atomBIOSHandlePtr handle, AtomBiosFunc func,
		AtomBIOSArgPtr data)
{
    assert (sizeof(AtomBIOSQueryStr) == (FUNC_END - ATOM_QUERY_FUNCS + 1));

    if (func < ATOM_QUERY_FUNCS) {
	if (func >= 0 && func < sizeof(AtomBIOSFuncStr))
	    xf86DrvMsgVerb(scrnIndex, 5, X_WARNING,
			   "AtomBIOS support not available, cannot execute %s\n",
			   AtomBIOSFuncStr[func]);
	else
	    xf86DrvMsg(scrnIndex, X_ERROR,"Invalid AtomBIOS func %x\n",func);
    } else {

	if (func < FUNC_END)
	    xf86DrvMsgVerb(scrnIndex, 5, X_WARNING,
			    "AtomBIOS not available, cannot get %s\n",
			   AtomBIOSQueryStr[func - ATOM_QUERY_FUNCS]);
	else
	    xf86DrvMsg(scrnIndex, X_ERROR, "Invalid AtomBIOS query %x\n",func);
    }
    return ATOM_NOT_IMPLEMENTED;
}

#endif /* ATOM_BIOS */

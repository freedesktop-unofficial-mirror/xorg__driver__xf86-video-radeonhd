/*
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

#include "xf86.h"
#include "xf86_OSproc.h"
#include "rhd.h"
#include "rhd_macros.h"
#include "rhd_atombios.h"

#include "xf86.h"

/* these types are used in ATI's atombios.h */
# ifndef ULONG
typedef CARD32 ULONG;
# endif
# ifndef USHORT
typedef CARD16 USHORT;
# endif
# ifndef UCHAR
typedef CARD8 UCHAR;
# endif

# include "atombios.h"


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
    unsigned char *BIOSBase;
    atomDataTablesPtr atomDataPtr;
} atomBIOSHandle, *atomBIOSHandlePtr;

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

static pointer
rhdInitAtomBIOS(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    unsigned char *ptr;
    atomDataTablesPtr atomDataPtr;
    atomBIOSHandlePtr handle;

    if (!xf86IsEntityPrimary(rhdPtr->entityIndex)) {
	int length = 1 << rhdPtr->PciInfo->biosSize;
	int read_len;
	if (!(ptr = xcalloc(length, 1))) {
	    xf86DrvMsg(pScrn->scrnIndex,X_ERROR,
		       "Cannot allocate %i bytes of memory "
		       "for BIOS image\n",length);
	    return NULL;
	}
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

    return handle;

 error1:
    xfree(atomDataPtr);
 error:
    xfree(ptr);
    return NULL;
}

void
rhdUninitAtomBIOS(ScrnInfoPtr pScrn, pointer handle)
{
    atomBIOSHandlePtr myhandle = (atomBIOSHandlePtr) handle;
    if (!myhandle)
	return;
    xfree(myhandle->BIOSBase);
    xfree(myhandle->atomDataPtr);
    xfree(myhandle);
}

int
RhdAtomBIOSFunc(ScrnInfoPtr pScrn, pointer handle, driverFunc func,
		AtomBIOSArgPtr data)
{
    atomDataTablesPtr atomDataPtr;

    if (func == ATOMBIOS_INIT) {
	if (!(data->ptr = rhdInitAtomBIOS(pScrn)))
	    return FAILED;
	return SUCCESS;
    }
    if (!handle)
	return FAILED;
    if (func <= ATOMBIOS_UNINIT) {
	rhdUninitAtomBIOS(pScrn, handle);
	return SUCCESS;
    }

    if (func < FUNC_END) {
	CARD8 cref, fref;

	atomDataPtr = ((atomBIOSHandlePtr)handle)->atomDataPtr;
	if (!rhdGetAtomBiosTableRevisionAndSize(
		(ATOM_COMMON_TABLE_HEADER *)(atomDataPtr->FirmwareInfo.base),
					     &cref,&fref,NULL)) {
	    return FAILED;
	}
	switch (cref) {
	    case 1:
		switch (func) {
		    case GET_MAX_PLL_CLOCK:
			data->card32 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo->ulMaxPixelClockPLL_Output;
			break;
		    case GET_MIN_PLL_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo->usMinPixelClockPLL_Output;
			    break;
		    case GET_MAX_PIXEL_CLK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo->usMaxPixelClock;
			break;
		    case GET_REF_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo->usReferenceClock;
			break;
		    default:
			return NOT_IMPLEMENTED;
		}
	    case 2:
		switch (func) {
		    case GET_MAX_PLL_CLOCK:
			data->card32 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_2->ulMaxPixelClockPLL_Output;
			break;
		    case GET_MIN_PLL_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_2->usMinPixelClockPLL_Output;
			    break;
		    case GET_MAX_PIXEL_CLK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_2->usMaxPixelClock;
			break;
		    case GET_REF_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_2->usReferenceClock;
			break;
		    default:
			return NOT_IMPLEMENTED;
		}
		break;
	    case 3:
		switch (func) {
		    case GET_MAX_PLL_CLOCK:
			data->card32 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_3->ulMaxPixelClockPLL_Output;
			break;
		    case GET_MIN_PLL_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_3->usMinPixelClockPLL_Output;
			    break;
		    case GET_MAX_PIXEL_CLK:
			data->card16= atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_3->usMaxPixelClock;
			break;
		    case GET_REF_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_3->usReferenceClock;
			break;
		    default:
			return NOT_IMPLEMENTED;
		}
		break;
	    case 4:
		switch (func) {
		    case GET_MAX_PLL_CLOCK:
			data->card32 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_4->ulMaxPixelClockPLL_Output;
			break;
		    case GET_MIN_PLL_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_4->usMinPixelClockPLL_Output;
			    break;
		    case GET_MAX_PIXEL_CLK:
			data->card16= atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_4->usMaxPixelClock;
			break;
		    case GET_REF_CLOCK:
			data->card16 = atomDataPtr->FirmwareInfo
			    .FirmwareInfo_V_1_4->usReferenceClock;
			break;
		    default:
			return NOT_IMPLEMENTED;
		}
		break;
	    default:
		data->ptr = NULL;
		return NOT_IMPLEMENTED;
	}
	switch (func) {
	    case GET_MAX_PLL_CLOCK:
		xf86DrvMsg(pScrn->scrnIndex,X_INFO,"MAX_PLL_CLOCK: %lu\n",
			   data->card32);
		break;
	    case GET_MIN_PLL_CLOCK:
		xf86DrvMsg(pScrn->scrnIndex,X_INFO,"MIN_PLL_CLOCK: %i\n",
			   data->card16);
		break;
	    case GET_MAX_PIXEL_CLK:
		xf86DrvMsg(pScrn->scrnIndex,X_INFO,"MAX_PIXEL_CLK: %i\n",
			   data->card16);
		break;
	    case GET_REF_CLOCK:
		xf86DrvMsg(pScrn->scrnIndex,X_INFO,"REF_CLK: %i\n",
			   data->card16);
		break;
	    default:
		;
		break;
	}
	return SUCCESS;
    }
    return NOT_IMPLEMENTED;
}

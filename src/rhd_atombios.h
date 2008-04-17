/*
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
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


#ifndef RHD_ATOMBIOS_H_
# define RHD_ATOMBIOS_H_

# ifdef ATOM_BIOS

typedef enum _AtomBiosRequestID {
    ATOMBIOS_INIT,
    ATOMBIOS_TEARDOWN,
# ifdef ATOM_BIOS_PARSER
    ATOMBIOS_EXEC,
#endif
    ATOMBIOS_ALLOCATE_FB_SCRATCH,
    ATOMBIOS_GET_CONNECTORS,
    ATOMBIOS_GET_PANEL_MODE,
    ATOMBIOS_GET_PANEL_EDID,
    ATOMBIOS_GET_CODE_DATA_TABLE,
    GET_DEFAULT_ENGINE_CLOCK,
    GET_DEFAULT_MEMORY_CLOCK,
    GET_MAX_PIXEL_CLOCK_PLL_OUTPUT,
    GET_MIN_PIXEL_CLOCK_PLL_OUTPUT,
    GET_MAX_PIXEL_CLOCK_PLL_INPUT,
    GET_MIN_PIXEL_CLOCK_PLL_INPUT,
    GET_MAX_PIXEL_CLK,
    GET_REF_CLOCK,
    GET_FW_FB_START,
    GET_FW_FB_SIZE,
    ATOM_TMDS_MAX_FREQUENCY,
    ATOM_TMDS_PLL_CHARGE_PUMP,
    ATOM_TMDS_PLL_DUTY_CYCLE,
    ATOM_TMDS_PLL_VCO_GAIN,
    ATOM_TMDS_PLL_VOLTAGE_SWING,
    ATOM_LVDS_SUPPORTED_REFRESH_RATE,
    ATOM_LVDS_OFF_DELAY,
    ATOM_LVDS_SEQ_DIG_ONTO_DE,
    ATOM_LVDS_SEQ_DE_TO_BL,
    ATOM_LVDS_SPATIAL_DITHER,
    ATOM_LVDS_TEMPORAL_DITHER,
    ATOM_LVDS_DUALLINK,
    ATOM_LVDS_24BIT,
    ATOM_LVDS_GREYLVL,
    ATOM_LVDS_FPDI,
    ATOM_GPIO_QUERIES,
    ATOM_GPIO_I2C_CLK_MASK,
    ATOM_DAC1_BG_ADJ,
    ATOM_DAC1_DAC_ADJ,
    ATOM_DAC1_FORCE,
    ATOM_DAC2_CRTC2_BG_ADJ,
    ATOM_DAC2_CRTC2_DAC_ADJ,
    ATOM_DAC2_CRTC2_FORCE,
    ATOM_DAC2_CRTC2_MUX_REG_IND,
    ATOM_DAC2_CRTC2_MUX_REG_INFO,
    ATOM_ANALOG_TV_MODE,
    ATOM_ANALOG_TV_DEFAULT_MODE,
    ATOM_ANALOG_TV_SUPPORTED_MODES,
    ATOM_GET_CONDITIONAL_GOLDEN_SETTINGS,
    ATOM_GET_PCIENB_CFG_REG7,
    ATOM_GET_CAPABILITY_FLAG,
    ATOM_GET_PCIE_LANES,
    FUNC_END
} AtomBiosRequestID;

typedef enum _AtomBiosResult {
    ATOM_SUCCESS,
    ATOM_FAILED,
    ATOM_NOT_IMPLEMENTED
} AtomBiosResult;

typedef struct AtomExec {
    int index;
    pointer pspace;
    pointer *dataSpace;
} AtomExecRec, *AtomExecPtr;

typedef struct AtomFb {
    unsigned int start;
    unsigned int size;
} AtomFbRec, *AtomFbPtr;

typedef enum AtomTVMode {
    ATOM_TVMODE_NTSC = 1 << 0,
    ATOM_TVMODE_NTSCJ = 1 << 1,
    ATOM_TVMODE_PAL = 1 << 2,
    ATOM_TVMODE_PALM = 1 << 3,
    ATOM_TVMODE_PALCN = 1 << 4,
    ATOM_TVMODE_PALN = 1 << 5,
    ATOM_TVMODE_PAL60 = 1 << 6,
    ATOM_TVMODE_SECAM = 1 << 7,
    ATOM_TVMODE_CV = 1 << 8
} AtomTVMode;

enum atomPCIELanes {
    atomPCIELaneNONE,
    atomPCIELane0_3,
    atomPCIELane0_7,
    atomPCIELane4_7,
    atomPCIELane8_11,
    atomPCIELane8_15,
    atomPCIELane12_15
};

typedef struct AtomGoldenSettings
{
    unsigned char *BIOSPtr;
    unsigned char *End;
    unsigned int value;

} AtomGoldenSettings;

typedef union AtomBiosArg
{
    CARD32 val;
    struct rhdConnectorInfo	*connectorInfo;
    enum RHD_CHIPSETS		chipset;
    struct AtomGoldenSettings	GoldenSettings;
    unsigned char*		EDIDBlock;
    struct {
	unsigned char *loc;
	unsigned short size;
    } CommandDataTable;
    struct
    {
	enum atomPCIELanes	Chassis;
	enum atomPCIELanes	Docking;
    } pcieLanes;
    atomBiosHandlePtr		atomhandle;
    DisplayModePtr		mode;
    AtomExecRec			exec;
    AtomFbRec			fb;
    enum AtomTVMode		tvMode;
} AtomBiosArgRec, *AtomBiosArgPtr;

extern AtomBiosResult RHDAtomBiosFunc(int scrnIndex,
				      atomBiosHandlePtr handle,
				      AtomBiosRequestID id, AtomBiosArgPtr data);
extern Bool rhdAtomSetTVEncoder(atomBiosHandlePtr handle, Bool enable, int mode);

enum atomEncoderMode {
    atomDVI_1Link,
    atomDVI_2Link,
    atomDP,
    atomDP_8Lane,
    atomLVDS,
    atomLVDS_DUAL,
    atomHDMI,
    atomSDVO,
    atomTVComposite,
    atomTVSVideo,
    atomTVComponent,
    atomCRTC
};

enum atomTransmitter {
    atomTransmitterLVTMA,
    atomTransmitterUNIPHY,
    atomTransmitterPCIEPHY,
    atomTransmitterDIG1,
    atomTransmitterDIG2
};

enum atomEncoder {
    atomEncoderDACA,
    atomEncoderDACB,
    atomEncoderTV,
    atomEncoderTMDS1,
    atomEncoderTMDS2,
    atomEncoderLVDS,
    atomEncoderDVO,
    atomEncoderDIG1,
    atomEncoderDIG2,
    atomEncoderExternal
};

enum atomOutput {
    atomDVOOutput,
    atomLCDOutput,
    atomCVOutput,
    atomTVOutput,
    atomLVTMAOutput,
    atomTMDSAOutput,
    atomDAC1Output,
    atomDAC2Output
};

enum atomOutputAction {
    atomEnable,
    atomDisable,
    atomLcdOn,
    atomLcdOff,
    atomLcdBrightnessControl,
    atomLcdSelftestStart,
    atomLcdSelftestStop,
    atomEncoderInit
};

enum atomTransmitterAction {
    atomTransDisable,
    atomTransEnable,
    atomTransEnableOutput,
    atomTransDisableOutput,
    atomTransSetup
};

enum atomTransmitterLink {
    atomTransLinkA,
    atomTransLinkB,
    atomTransLinkAB
};

enum atomDVODeviceType {
    atomLCD,
    atomCRT,
    atomDFP,
    atomTV,
    atomCV
};

enum atomDACStandard {
    atomDAC_VGA,
    atomDAC_CV,
    atomDAC_NTSC,
    atomDAC_PAL
};

enum atomDVORate {
    ATOM_DVO_RATE_SDR,
    ATOM_DVO_RATE_DDR
};

enum atomDVOOutput {
    ATOM_DVO_OUTPUT_LOW12BIT,
    ATOM_DVO_OUTPUT_HIGH12BIT,
    ATOM_DVO_OUTPUT_24BIT
};

struct atomTransmitterConfig
{
    int pixelClock;
    enum atomEncoder encoder;
    enum atomPCIELanes lanes;
    enum atomEncoderMode mode;
    enum atomTransmitterLink link;
    Bool coherent;
};

enum atomScaler {
    atomScaler1,
    atomScaler2
};

enum atomEncoderAction {
    atomEncoderOn,
    atomEncoderOff
};

enum atomScaleMode {
    atomScaleNone,
    atomScaleCenter,
    atomScaleExpand,
    atomScaleMulti
};

enum atomTemporalGreyLevels {
    TEMPORAL_DITHER_0,
    TEMPORAL_DITHER_4,
    TEMPRAL_DITHER_2
};

struct atomEncoderConfig
{
    int pixelClock;
    enum atomEncoderAction action;
    union {
	struct {
	    enum atomDACStandard Standard;
	} dac;
	struct {
	    enum AtomTVMode Standard;
	} tv;
	struct {
	    Bool dual;
	    Bool is24bit;
	} lvds;
	struct {
	    Bool dual;
	    Bool is24bit;
	    Bool coherent;
	    Bool linkB;
	    Bool hdmi;
	    Bool spatialDither;
	    enum atomTemporalGreyLevels temporalGrey;
	} lvds2;
	struct {
	    enum atomTransmitterLink link;
	    enum atomTransmitter transmitter;
	    enum atomEncoderMode encoderMode;
	} dig;
	struct {
	    enum atomDVODeviceType deviceType;
	    int encoderID;
	} dvo;
	struct{
	    enum atomDVORate rate;
	    enum atomDVOOutput output;
	} dvo3;
    } u;
};

Bool rhdAtomDigTransmitterControl(atomBiosHandlePtr handle, enum atomTransmitter id,
				  enum atomTransmitterAction action, struct atomTransmitterConfig *config);
extern Bool rhdAtomSetScaler(atomBiosHandlePtr handle, enum atomScaler scaler,
			     enum atomScaleMode mode);

# endif

#endif /*  RHD_ATOMBIOS_H_ */

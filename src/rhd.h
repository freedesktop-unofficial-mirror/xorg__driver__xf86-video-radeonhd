/*
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
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

#ifndef _RHD_H
# define _RHD_H

# if defined HAVE_CONFIG_H && !defined _CONFIG_H
#  error "config.h missing!"
# endif

#define RHD_DRIVER

#ifdef USE_EXA
#include "exa.h"
#endif
#ifdef USE_XAA
#include "xaa.h"
#endif

#ifdef DAMAGE
#include "damage.h"
#endif

#include "xf86xv.h"

#ifdef USE_DRI
#define _XF86DRI_SERVER_
#include "dri.h"
#include "GL/glxint.h"
#ifdef DAMAGE
#include "damage.h"
#include "globals.h"
#endif
#endif

#define RHD_MAJOR_VERSION (PACKAGE_VERSION_MAJOR)
#define RHD_MINOR_VERSION (PACKAGE_VERSION_MINOR)
#define RHD_PATCHLEVEL    (PACKAGE_VERSION_PATCHLEVEL)

/* It does not seem to matter how exactly we construct this int for
    + * DriverRec.driverVersion. So... choose 20/10/0 bitshift. */
#define RHD_VERSION            \
    ((RHD_MAJOR_VERSION<<20) | \
     (RHD_MINOR_VERSION<<10) | \
           (RHD_PATCHLEVEL))

#define RHD_NAME "RADEONHD"
#define RHD_DRIVER_NAME "radeonhd"


enum RHD_CHIPSETS {
    RHD_UNKNOWN = 0,
    /* R500 */
    RHD_RV505,
    RHD_RV515,
    RHD_RV516,
    RHD_R520,
    RHD_RV530,
    RHD_RV535,
    RHD_RV550,
    RHD_RV560,
    RHD_RV570,
    RHD_R580,
    /* R500 Mobility */
    RHD_M52,
    RHD_M54,
    RHD_M56,
    RHD_M58,
    RHD_M62,
    RHD_M64,
    RHD_M66,
    RHD_M68,
    RHD_M71,
    /* R500 integrated */
    RHD_RS600,
    RHD_RS690,
    RHD_RS740,
    /* R600 */
    RHD_R600,
    RHD_RV610,
    RHD_RV630,
    /* R600 Mobility */
    RHD_M72,
    RHD_M74,
    RHD_M76,
    /* RV670 came into existence after RV6x0 and M7x */
    RHD_RV670,
    RHD_M88,
    RHD_R680,
    RHD_RV620,
    RHD_M82,
    RHD_RV635,
    RHD_M86,
    RHD_RS780,
    RHD_RV770,
    RHD_CHIP_END
};

enum RHD_HPD_USAGE {
    RHD_HPD_USAGE_AUTO = 0,
    RHD_HPD_USAGE_OFF,
    RHD_HPD_USAGE_NORMAL,
    RHD_HPD_USAGE_SWAP,
    RHD_HPD_USAGE_AUTO_SWAP,
    RHD_HPD_USAGE_AUTO_OFF
};

enum RHD_TV_MODE {
    RHD_TV_NONE = 0,
    RHD_TV_NTSC = 1,
    RHD_TV_NTSCJ = 1 << 2,
    RHD_TV_PAL = 1 << 3,
    RHD_TV_PALM = 1 << 4,
    RHD_TV_PALCN = 1 << 5,
    RHD_TV_PALN = 1 << 6,
    RHD_TV_PAL60 = 1 << 7,
    RHD_TV_SECAM = 1 << 8,
    RHD_TV_CV = 1 << 9
};

enum rhdPropertyAction {
    rhdPropertyCheck,
    rhdPropertyGet,
    rhdPropertySet
};

union rhdPropertyData
{
    CARD32 integer;
    char *string;
    Bool Bool;
};

#define RHD_CONNECTORS_MAX 6

/* Just define where which PCI BAR lives for now. Will deal with different
 * locations as soon as cards with a different BAR layout arrives.
 */
#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2

/* More realistic powermanagement */
#define RHD_POWER_ON       0
#define RHD_POWER_RESET    1   /* off temporarily */
#define RHD_POWER_SHUTDOWN 2   /* long term shutdown */
#define RHD_POWER_UNKNOWN  3   /* initial state */

#define RHD_VBIOS_SIZE 0x10000

#ifndef XSERVER_LIBPCIACCESS
# define PCI_BUS(x)	((x)->bus)
# define PCI_DEV(x)	((x)->device)
# define PCI_FUNC(x)	((x)->func)
#else
# define PCI_BUS(x)	(((x)->domain << 8) | (x)->bus)
# define PCI_DEV(x)	((x)->dev)
# define PCI_FUNC(x)	((x)->func)
typedef struct pci_device *pciVideoPtr;
#endif

enum rhdCardType {
    RHD_CARD_NONE,
    RHD_CARD_AGP,
    RHD_CARD_PCIE
};

enum {
    RHD_PCI_CAPID_AGP    = 0x02,
    RHD_PCI_CAPID_PCIE   = 0x10
};


typedef struct _rhdI2CRec *rhdI2CPtr;
typedef struct _atomBiosHandle *atomBiosHandlePtr;
typedef struct _rhdShadowRec *rhdShadowPtr;

typedef struct _RHDopt {
    Bool set;
    union  {
        Bool bool;
        int integer;
        unsigned long uslong;
        double real;
        double freq;
        char *string;
    } val;
} RHDOpt, *RHDOptPtr;

/* Some more intelligent handling of chosing which acceleration to use */
enum AccelMethod {
    RHD_ACCEL_NONE = 0, /* ultra slow, but might be desired for debugging. */
    RHD_ACCEL_SHADOWFB = 1, /* cache in main ram. */
    RHD_ACCEL_XAA = 2, /* "old" X acceleration architecture. */
    RHD_ACCEL_EXA = 3, /* not done yet. */
    RHD_ACCEL_DEFAULT = 4 /* keep as highest. */
};

struct rhdAccel {
    int               fifo_slots;       /* Free slots in the FIFO (64 max)   */

				/* Computed values for Radeon */
    uint32_t          dp_gui_master_cntl;
    uint32_t          dp_gui_master_cntl_clip;
    uint32_t          trans_color;

				/* Saved values for ScreenToScreenCopy */
    int               xdir;
    int               ydir;

    uint32_t          dst_pitch_offset;

    int               num_gb_pipes;

    unsigned short    texW[2];
    unsigned short    texH[2];

    uint32_t         surface_cntl;
#ifdef USE_XAA
				/* ScanlineScreenToScreenColorExpand support */
    unsigned char     *scratch_buffer[1];
    unsigned char     *scratch_save;
    int               scanline_x;
    int               scanline_y;
    int               scanline_w;
    int               scanline_h;
    int               scanline_h_w;
    int               scanline_words;
    int               scanline_direct;
    int               scanline_bpp;     /* Only used for ImageWrite */
    int               scanline_fg;
    int               scanline_bg;
    int               scanline_hpass;
    int               scanline_x1clip;
    int               scanline_x2clip;
				/* Saved values for DashedTwoPointLine */
    int               dashLen;
    uint32_t          dashPattern;
    int               dash_fg;
    int               dash_bg;
#endif
#if USE_EXA
    int               exaSyncMarker;
    int               exaMarkerSynced;
    int               engineMode;
#define EXA_ENGINEMODE_UNKNOWN 0
#define EXA_ENGINEMODE_2D      1
#define EXA_ENGINEMODE_3D      2
#endif

    /* X itself has the 3D context */
    Bool             XHas3DEngineState;


};


typedef struct RHDRec {
    int                 scrnIndex;

    int                 ChipSet;
#ifdef XSERVER_LIBPCIACCESS
    struct pci_device   *PciInfo;
    struct pci_device   *NBPciInfo;
#else
    pciVideoRec         *PciInfo;
    PCITAG              PciTag;
    PCITAG		NBPciTag;
#endif
    unsigned int	PciDeviceID;
    enum rhdCardType	cardType;
    int			entityIndex;
    EntityInfoPtr       pEnt;
    struct rhdCard      *Card;
    OptionInfoPtr       Options;
    enum AccelMethod    AccelMethod;
    RHDOpt              swCursor;
    RHDOpt		shadowFB;
    RHDOpt		forceReduced;
    RHDOpt              forceDPI;
    RHDOpt		noRandr;
    RHDOpt		rrUseXF86Edid;
    RHDOpt		rrOutputOrder;
    RHDOpt		useDRI;
    RHDOpt		tvModeName;
    RHDOpt		scaleTypeOpt;
    RHDOpt		unverifiedFeatures;
    enum RHD_HPD_USAGE	hpdUsage;
    unsigned int        FbMapSize;
    pointer             FbBase;   /* map base of fb   */
    unsigned int        FbPhysAddress; /* card PCI BAR address of FB */
    unsigned int        FbIntAddress; /* card internal address of FB */
    unsigned int        FbPCIAddress; /* physical address of FB */

    /* Some simplistic memory handling */
#define ALIGN(x,align)	(((x)+(align)-1)&~((align)-1))
    /* Use this macro to always chew up 4096byte aligned pieces. */
#define RHD_FB_CHUNK(x)     ALIGN((x),0x1000)
    unsigned int        FbFreeStart;
    unsigned int        FbFreeSize;

    /* visible part of the framebuffer */
    unsigned int        FbScanoutStart;
    unsigned int        FbScanoutSize;

    /* for 2d acceleration: pixmapcache and such */
    RHDOpt              OffscreenOption;
    unsigned int        FbOffscreenStart;
    unsigned int        FbOffscreenSize;

    unsigned int        MMIOMapSize;
    pointer             MMIOBase; /* map base of mmio */
    unsigned int        MMIOPCIAddress; /* physical address of mmio */

    struct _xf86CursorInfoRec  *CursorInfo;
    struct rhd_Cursor_Bits     *CursorBits; /* ARGB if NULL */
    CARD32              CursorColor0, CursorColor1;
    CARD32             *CursorImage;

    CloseScreenProcPtr  CloseScreen;

    struct _I2CBusRec	**I2C;  /* I2C bus list */
    atomBiosHandlePtr   atomBIOS; /* handle for AtomBIOS */
    /*
     * BIOS copy - kludge that should go away
     * once we know how to read PCI BIOS on
     * POSTed hardware
     */
    unsigned char*	BIOSCopy;

    struct rhdMC       *MC;  /* Memory Controller */
    struct rhdVGA      *VGA; /* VGA compatibility HW */
    struct rhdCrtc     *Crtc[2];
    struct rhdPLL      *PLLs[2]; /* Pixelclock PLLs */

    struct rhdLUTStore  *LUTStore;
    struct rhdLUT       *LUT[2];

    /* List of output devices:
     * we can go up to 5: DACA, DACB, TMDS, shared LVDS/TMDS, DVO.
     * Will also include displayport when this happens. */
    struct rhdOutput   *Outputs;

    struct rhdConnector *Connector[RHD_CONNECTORS_MAX];
    struct rhdHPD      *HPD; /* Hot plug detect subsystem */

    /* don't ignore the Monitor section of the conf file */
    struct rhdMonitor  *ConfigMonitor;
    enum RHD_TV_MODE   tvMode;
    rhdShadowPtr       shadowPtr;

    /* RandR compatibility layer */
    struct rhdRandr    *randr;
    /* log verbosity - store this for convenience */
    int			verbosity;

    /* DRI */
    struct rhdDri      *dri;

#ifdef USE_EXA
    ExaDriverPtr      exa;
#endif
#ifdef USE_XAA
    XAAInfoRecPtr     xaa;
#endif
    Bool              allowColorTiling;
    Bool              tilingEnabled; /* mirror of sarea->tiling_enabled */

    struct rhdAccel   *accel_state;

#ifdef USE_DRI
    Bool              directRenderingEnabled;
    struct rhdCP      *cp;
    Bool              DMAForXv;
#endif /* USE_DRI */
    /* chips with PVS/TCL hw (used for EXA render) */
    Bool             has_tcl;

    /* Xv */
    XF86VideoAdaptorPtr adaptor;
} RHDRec, *RHDPtr;

#define RHDPTR(p) 	((RHDPtr)((p)->driverPrivate))
#define RHDPTRI(p) 	(RHDPTR(xf86Screens[(p)->scrnIndex]))

#if defined(__GNUC__)
#  define NORETURN __attribute__((noreturn))
#  define CONST    __attribute__((pure))
#else
#  define NORETURN
#  define CONST
#endif

#define PIXEL_CODE(x) (x->bitsPerPixel != 16 ? x->bitsPerPixel : x->depth)

/* rhd_driver.c */
/* Some handy functions that makes life so much more readable */
extern unsigned int RHDReadPCIBios(RHDPtr rhdPtr, unsigned char **prt);
extern Bool RHDScalePolicy(struct rhdMonitor *Monitor, struct rhdConnector *Connector);
extern void RHDAllIdle(ScrnInfoPtr pScrn);
extern CARD32 _RHDRegRead(int scrnIndex, CARD16 offset);
#define RHDRegRead(ptr, offset) _RHDRegRead((ptr)->scrnIndex, (offset))
extern void _RHDRegWrite(int scrnIndex, CARD16 offset, CARD32 value);
#define RHDRegWrite(ptr, offset, value) _RHDRegWrite((ptr)->scrnIndex, (offset), (value))
extern void _RHDRegMask(int scrnIndex, CARD16 offset, CARD32 value, CARD32 mask);
#define RHDRegMask(ptr, offset, value, mask) _RHDRegMask((ptr)->scrnIndex, (offset), (value), (mask))
extern CARD32 _RHDReadMC(int scrnIndex, CARD32 addr);
#define RHDReadMC(ptr,addr) _RHDReadMC((ptr)->scrnIndex,(addr))
extern void _RHDWriteMC(int scrnIndex, CARD32 addr, CARD32 data);
#define RHDWriteMC(ptr,addr,value) _RHDWriteMC((ptr)->scrnIndex,(addr),(value))
extern CARD32 _RHDReadPLL(int scrnIndex, CARD16 offset);
#define RHDReadPLL(ptr, off) _RHDReadPLL((ptr)->scrnIndex,(off))
extern void _RHDWritePLL(int scrnIndex, CARD16 offset, CARD32 data);
#define RHDWritePLL(ptr, off, value) _RHDWritePLL((ptr)->scrnIndex,(off),(value))
extern unsigned int RHDAllocFb(RHDPtr rhdPtr, unsigned int size, const char *name);

/* rhd_id.c */
Bool RHDIsIGP(enum RHD_CHIPSETS chipset);

/* rhd_helper.c */
void RhdGetOptValBool(const OptionInfoRec *table, int token,
                      RHDOptPtr optp, Bool def);
void RhdGetOptValInteger(const OptionInfoRec *table, int token,
                         RHDOptPtr optp, int def);
void RhdGetOptValULong(const OptionInfoRec *table, int token,
                       RHDOptPtr optp, unsigned long def);
void RhdGetOptValReal(const OptionInfoRec *table, int token,
                      RHDOptPtr optp, double def);
void RhdGetOptValFreq(const OptionInfoRec *table, int token,
                      OptFreqUnits expectedUnits, RHDOptPtr optp, double def);
void RhdGetOptValString(const OptionInfoRec *table, int token,
                        RHDOptPtr optp, char *def);
char *RhdAppendString(char *s1, const char *s2);
void RhdAssertFailed(const char *str,
		     const char *file, int line, const char *func) NORETURN;
void RhdAssertFailedFormat(const char *str,  const char *file, int line,
			   const char *func, const char *format, ...) NORETURN;

/* Extra debugging verbosity: decimates gdb usage */

/* __func__ is really nice, but not universal */
#if !defined(__GNUC__) && !defined(C99)
#define __func__ "unknown"
#endif

#ifndef NO_ASSERT
#  define ASSERT(x) do { if (!(x)) RhdAssertFailed \
			 (#x, __FILE__, __LINE__, __func__); } while(0)
#  define ASSERTF(x,f...) do { if (!(x)) RhdAssertFailedFormat \
			    (#x, __FILE__, __LINE__, __func__, ##f); } while(0)
#else
#  define ASSERT(x) ((void)0)
#  define ASSERTF(x,...) ((void)0)
#endif

#define LOG_DEBUG 7
void RHDDebug(int scrnIndex, const char *format, ...);
void RHDDebugCont(const char *format, ...);
void RHDDebugVerb(int scrnIndex, int verb, const char *format, ...);
void RHDDebugContVerb(int verb, const char *format, ...);
#define RHDFUNC(ptr) RHDDebug((ptr)->scrnIndex, "FUNCTION: %s\n", __func__)
#define RHDFUNCI(scrnIndex) RHDDebug(scrnIndex, "FUNCTION: %s\n", __func__)
void RhdDebugDump(int scrnIndex, unsigned char *start, int size);

#ifdef RHD_DEBUG
CARD32 _RHDRegReadD(int scrnIndex, CARD16 offset);
# define RHDRegReadD(ptr, offset) _RHDRegReadD((ptr)->scrnIndex, (offset))
void _RHDRegWriteD(int scrnIndex, CARD16 offset, CARD32 value);
# define RHDRegWriteD(ptr, offset, value) _RHDRegWriteD((ptr)->scrnIndex, (offset), (value))
void _RHDRegMaskD(int scrnIndex, CARD16 offset, CARD32 value, CARD32 mask);
# define RHDRegMaskD(ptr, offset, value, mask) _RHDRegMaskD((ptr)->scrnIndex, (offset), (value), (mask))
# define DEBUGP(x) {x;}
#else
# define RHDRegReadD(ptr, offset) RHDRegRead(ptr, offset)
# define RHDRegWriteD(ptr, offset, value) RHDRegWrite(ptr, offset, value)
# define RHDRegMaskD(ptr, offset, value, mask) RHDRegMask(ptr, offset, value, mask)
# define DEBUGP(x)
#endif

#define IS_R300_3D ((info->ChipSet == RHD_RS600) || (info->ChipSet == RHD_RS690) || (info->ChipSet == RHD_RS740))
#define IS_R500_3D ((info->ChipSet >= RHD_RV505) && (info->ChipSet <= RHD_M71))

#define RADEON_TIMEOUT    2000000 /* Fall out of wait loops after this count */
#define RADEON_LOGLEVEL_DEBUG 4
#define RADEON_BUFFER_ALIGN 0x00000fff
#define RADEON_IDLE_RETRY      16 /* Fall out of idle loops after this count */

#define ADDRREG(addr)       ((volatile uint32_t *)((unsigned char*)info->MMIOBase + (addr)))

#define xFixedToFloat(f) (((float) (f)) / 65536)

#define RADEON_ALIGN(x,bytes) (((x) + ((bytes) - 1)) & ~((bytes) - 1))

#define RADEONWaitForFifo(pScrn, entries)                               \
do {                                                                    \
    if (info->accel_state->fifo_slots < entries)                        \
        RADEONWaitForFifoFunction(pScrn, entries);                      \
    info->accel_state->fifo_slots -= entries;                           \
} while (0)

/* radeon_video.c */
extern void RADEONInitVideo(ScreenPtr pScreen);

/* radeon_accel.c */
extern Bool RADEONAccelInit(ScreenPtr pScreen);
extern void RADEONEngineFlush(ScrnInfoPtr pScrn);
extern void RADEONEngineInit(ScrnInfoPtr pScrn);
extern void RADEONEngineReset(ScrnInfoPtr pScrn);
extern void RADEONEngineRestore(ScrnInfoPtr pScrn);
extern uint8_t *RADEONHostDataBlit(ScrnInfoPtr pScrn, unsigned int cpp,
				   unsigned int w, uint32_t dstPitchOff,
				   uint32_t *bufPitch, int x, int *y,
				   unsigned int *h, unsigned int *hpass);
extern void RADEONHostDataBlitCopyPass(ScrnInfoPtr pScrn,
                                       unsigned int bpp,
                                       uint8_t *dst, uint8_t *src,
                                       unsigned int hpass,
                                       unsigned int dstPitch,
                                       unsigned int srcPitch);
extern void  RADEONCopySwap(uint8_t *dst, uint8_t *src, unsigned int size, int swap);
extern void RADEONHostDataParams(ScrnInfoPtr pScrn, uint8_t *dst,
                                 uint32_t pitch, int cpp,
                                 uint32_t *dstPitchOffset, int *x, int *y);
extern void RADEONInit3DEngine(ScrnInfoPtr pScrn);
extern void RADEONWaitForFifoFunction(ScrnInfoPtr pScrn, int entries);
#ifdef USE_DRI
extern drmBufPtr RADEONCPGetBuffer(ScrnInfoPtr pScrn);
extern void RADEONCPFlushIndirect(ScrnInfoPtr pScrn, int discard);
extern void RADEONCPReleaseIndirect(ScrnInfoPtr pScrn);
extern int RADEONCPStop(ScrnInfoPtr pScrn,  RHDPtr info);
#endif

#ifdef USE_XAA
/* radeon_accelfuncs.c */
extern void RADEONAccelInitMMIO(ScreenPtr pScreen, XAAInfoRecPtr a);
extern Bool RADEONSetupMemXAA(int scrnIndex, ScreenPtr pScreen);
#endif

/* radeon_commonfuncs.c */
#ifdef USE_DRI
extern void RADEONWaitForIdleCP(ScrnInfoPtr pScrn);
#endif
extern void RADEONWaitForIdleMMIO(ScrnInfoPtr pScrn);

#ifdef USE_EXA
/* radeon_exa.c */
extern Bool RADEONSetupMemEXA(ScreenPtr pScreen);

/* radeon_exa_funcs.c */
extern void RADEONCopyCP(PixmapPtr pDst, int srcX, int srcY, int dstX,
                         int dstY, int w, int h);
extern void RADEONCopyMMIO(PixmapPtr pDst, int srcX, int srcY, int dstX,
                           int dstY, int w, int h);
extern Bool RADEONDrawInitCP(ScreenPtr pScreen);
extern Bool RADEONDrawInitMMIO(ScreenPtr pScreen);
extern void RADEONDoPrepareCopyCP(ScrnInfoPtr pScrn,
                                  uint32_t src_pitch_offset,
                                  uint32_t dst_pitch_offset,
                                  uint32_t datatype, int rop,
                                  Pixel planemask);
extern void RADEONDoPrepareCopyMMIO(ScrnInfoPtr pScrn,
                                    uint32_t src_pitch_offset,
                                    uint32_t dst_pitch_offset,
                                    uint32_t datatype, int rop,
                                    Pixel planemask);
#endif

#if defined(USE_DRI) && defined(USE_EXA)
/* radeon_exa.c */
extern Bool RADEONGetDatatypeBpp(int bpp, uint32_t *type);
extern Bool RADEONGetPixmapOffsetPitch(PixmapPtr pPix,
                                       uint32_t *pitch_offset);
extern unsigned long long RADEONTexOffsetStart(PixmapPtr pPix);
#endif


#endif /* _RHD_H */

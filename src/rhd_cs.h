/*
 * Copyright 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2008  Egbert Eich   <eich@novell.com>
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

/*
 * Command Submission backend.
 */
#ifndef _HAVE_RHD_CS_
#define _HAVE_RHD_CS_ 1

/*
 * Enable tracking of buffer usage.
 */
#if 0
#define RHD_CS_DEBUG 1
#endif

enum RhdCSType {
    RHD_CS_NONE = 0,
    RHD_CS_MMIO,
    RHD_CS_CP, /* CP but without the GART (Direct CP) */
    RHD_CS_CPDMA /* CP with kernel support (DRM or indirect CP) */
};

struct RhdCS {
    int scrnIndex;

    enum RhdCSType Type;

    Bool Active;

#define RHD_CS_CLEAN_DIRTY   0
#define RHD_CS_CLEAN_QUEUED  1
#define RHD_CS_CLEAN_DONE    2
    CARD8 Clean;

    /* track the ring state. */
    CARD32 *Buffer;
    CARD32 Flushed;
    CARD32 Wptr;
    CARD32 Size;
    CARD32 Mask;

#ifdef RHD_CS_DEBUG
    CARD32 Grabbed;
    const char *Func;
#endif

    /* callbacks, set up according to MMIO, direct or indirect CP */

    void (*Grab) (struct RhdCS *CS, CARD32 Count);

    void (*Flush) (struct RhdCS *CS);
    Bool AdvanceFlush; /* flush the buffer all the time? */
    void (*Idle) (struct RhdCS *CS);

    void (*Start) (struct RhdCS *CS);
    void (*Reset) (struct RhdCS *CS);
    void (*Stop) (struct RhdCS *CS);

    void (*Destroy) (struct RhdCS *CS);

    void *Private; /* holds MMIO or direct/indirect CP specific information */
};

/* some interface bleedover from rhd_dri.c */
#ifdef USE_DRI
int RHDDRMFDGet(int scrnIndex);
struct _drmBuf *RHDDRMCPBuffer(int scrnIndex);
#endif

/*
 * Some CP defines.
 */
#define CP_PACKET0(reg, n)  (((n - 1) << 16) | ((reg) >> 2))
#define CP_PACKET2()        (0x80000000)
#define CP_PACKET3(pkt, n)  (0xC0000000 | (pkt) | ((n) << 16))

#define R5XX_CP_PACKET3_CNTL_HOSTDATA_BLT         0x00009400

/*
 * CS Calls and macros.
 */
void RHDCSFlush(struct RhdCS *CS);
void RHDCSIdle(struct RhdCS *CS);
void RHDCSStart(struct RhdCS *CS);
void RHDCSReset(struct RhdCS *CS);
void RHDCSStop(struct RhdCS *CS);

void RHDCSInit(ScrnInfoPtr pScrn);
void RHDCSDestroy(ScrnInfoPtr pScrn);

/*
 * I seriously dislike big macros. They make code unreadable and they invite
 * others to make it even more unreadable. But i also cannot deny the numbers
 * from the benchmarks.
 */
#define _RHDCSGrab(CS, Count) \
do { \
    if (CS->Clean == RHD_CS_CLEAN_QUEUED) \
	CS->Clean = RHD_CS_CLEAN_DONE; \
    CS->Grab(CS, Count); \
} while (0)

#ifdef RHD_CS_DEBUG
#define RHDCSGrab(CS, Count)
do { \
   if ((CS)->Wptr != (((CS)->Flushed + (CS)->Grabbed) & (CS)->Mask)) \
	xf86DrvMsg((CS)->scrnIndex, X_ERROR, \
		   "%s: Wptr != Flushed + Grabbed (%d vs %d + %d) (%s -> %s)\n", \
		   __func__, (unsigned int) (CS)->Wptr, (unsigned int) (CS)->Flushed, \
		   (unsigned int) (CS)->Grabbed, (CS)->Func, __func__); \
    _RHDCSGrab((CS), (Count)); \
    (CS)->Grabbed += (Count); \
    (CS)->Func = __func__; \
} while (0)
#else
#define RHDCSGrab(CS, Count) _RHDCSGrab((CS), (Count))
#endif

#define RHDCSWrite(CS, Value) \
do { \
    (CS)->Buffer[(CS)->Wptr] = (Value); \
    (CS)->Wptr = ((CS)->Wptr + 1) & (CS)->Mask; \
} while (0)

#define RHDCSRegWrite(CS, Reg, Value) \
do { \
    (CS)->Buffer[(CS)->Wptr] = CP_PACKET0((Reg), 1); \
    (CS)->Buffer[((CS)->Wptr + 1) & (CS)->Mask] = (Value); \
    (CS)->Wptr = ((CS)->Wptr + 2) & (CS)->Mask; \
} while (0)

#define RHDCSAdvance(CS) \
do { \
    if ((CS)->AdvanceFlush) \
	RHDCSFlush((CS)); \
} while (0)

#ifdef USE_DRI
/*
 * Some extra handling for those who want to use separate indirect buffers.
 */
CARD8 *RHDDRMIndirectBufferGet(int scrnIndex, unsigned int *IntAddress, CARD32 *Size);
void RHDDRMIndirectBufferDiscard(int scrnIndex, CARD8 *Buffer);
#endif

#endif /* _HAVE_RHD_CS_ */

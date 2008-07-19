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

/*
 * Some CP defines.
 */
#define CP_PACKET0(reg, n)  (((n - 1) << 16) | ((reg) >> 2))
#define CP_PACKET3(pkt, n)  (0xC0000000 | (pkt) | ((n) << 16))

#define R5XX_CP_PACKET3_CNTL_HOSTDATA_BLT         0x00009400

/*
 * CS Calls and macros.
 */
#ifndef RHD_CS_DEBUG
void RHDCSGrab(struct RhdCS *CS, int Count);
#else
void _RHDCSGrab(struct RhdCS *CS, int Count, const char *func);

#define RHDCSGrab(CS, Count) _RHDCSGrab((CS), (Count), __func__)
#endif

static inline void
RHDCSWrite(struct RhdCS *CS, CARD32 Value)
{
    CS->Buffer[CS->Wptr] = Value;
    CS->Wptr = (CS->Wptr + 1) & CS->Mask;
}

static inline void
RHDCSRegWrite(struct RhdCS *CS, CARD16 Reg, CARD32 Value)
{
    CS->Buffer[CS->Wptr] = CP_PACKET0(Reg, 1);
    CS->Buffer[(CS->Wptr + 1) & CS->Mask] = Value;
    CS->Wptr = (CS->Wptr + 2) & CS->Mask;
}

void RHDCSFlush(struct RhdCS *CS);
void RHDCSAdvance(struct RhdCS *CS);
void RHDCSIdle(struct RhdCS *CS);
void RHDCSStart(struct RhdCS *CS);
void RHDCSReset(struct RhdCS *CS);
void RHDCSStop(struct RhdCS *CS);

void RHDCSInit(ScrnInfoPtr pScrn);
void RHDCSDestroy(ScrnInfoPtr pScrn);

#endif /* _HAVE_RHD_CS_ */

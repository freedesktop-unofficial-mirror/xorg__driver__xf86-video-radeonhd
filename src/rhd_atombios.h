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


#ifndef RHD_ATOMBIOS_H_
# define RHD_ATOMBIOS_H_

#include "rhd.h"

typedef enum {
    ATOMBIOS_INIT,
    ATOMBIOS_TEARDOWN,
    ATOMBIOS_EXEC,
    ATOMBIOS_ALLOCATE_FB_SCRATCH,
    ATOM_QUERY_FUNCS = 0x1000,
    GET_DEFAULT_ENGINE_CLOCK = ATOM_QUERY_FUNCS,
    GET_DEFAULT_MEMORY_CLOCK,
    GET_MAX_PIXEL_CLOCK_PLL_OUTPUT,
    GET_MIN_PIXEL_CLOCK_PLL_OUTPUT,
    GET_MAX_PIXEL_CLOCK_PLL_INPUT,
    GET_MIN_PIXEL_CLOCK_PLL_INPUT,
    GET_MAX_PIXEL_CLK,
    GET_REF_CLOCK,
    ATOM_VRAM_QUERIES,
    GET_FW_FB_START = ATOM_VRAM_QUERIES,
    GET_FW_FB_SIZE,
    ATOM_TMDS_QUERIES,
    ATOM_TMDS_FREQUENCY = ATOM_TMDS_QUERIES,
    ATOM_TMDS_PLL_CHARGE_PUMP,
    ATOM_TMDS_PLL_DUTY_CYCLE,
    ATOM_TMDS_PLL_VCO_GAIN,
    ATOM_TMDS_PLL_VOLTAGE_SWING,
    FUNC_END
} AtomBiosFunc;

typedef enum {
    ATOM_SUCCESS,
    ATOM_FAILED,
    ATOM_NOT_IMPLEMENTED
} AtomBiosResult;

typedef struct {
    int index;
    pointer pspace;
    pointer *dataSpace;
} AtomExec, *AtomExecPtr;

typedef struct {
    unsigned int start;
    unsigned int size;
} AtomFb, *AtomFbPtr;

typedef union
{
    CARD32 val;
  
    pointer ptr;
    atomBIOSHandlePtr atomp;
    AtomExec exec;
    AtomFb fb;
} AtomBIOSArg, *AtomBIOSArgPtr;


extern AtomBiosResult
RHDAtomBIOSFunc(ScrnInfoPtr pScrn, atomBIOSHandlePtr handle, AtomBiosFunc func,
		    AtomBIOSArgPtr data);

/* only for testing */
void rhdTestAtomBIOS(ScrnInfoPtr pScrn);
Bool rhdTestAsicInit(ScrnInfoPtr pScrn);

#endif /*  RHD_ATOMBIOS_H_ */

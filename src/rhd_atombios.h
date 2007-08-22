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


#ifndef RHD_ATOMBIOS_H_
# define RHD_ATOMBIOS_H_

typedef enum {
    ATOMBIOS_INIT,
    ATOMBIOS_UNINIT,
    GET_MAX_PLL_CLOCK,
    GET_MIN_PLL_CLOCK,
    GET_MAX_PIXEL_CLK,
    GET_REF_CLOCK,
    FUNC_END
} AtomBiosFunc;

typedef enum {
    SUCCESS,
    FAILED,
    NOT_IMPLEMENTED
} AtomBiosResult;

typedef union 
{
    CARD8 card8;
    CARD16 card16;
    CARD32 card32;
    pointer ptr;
} AtomBIOSArg, *AtomBIOSArgPtr;


AtomBiosResult
RhdAtomBIOSFunc(ScrnInfoPtr pScrn, pointer handle, AtomBiosFunc func,
		    AtomBIOSArgPtr data);

#endif /*  RHD_ATOMBIOS_H_ */

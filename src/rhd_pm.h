/*
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _RHD_PM_H
# define _RHD_PM_H

struct rhdPmState {
    /* All entries: 0 means unknown / unspecified / dontchange */
    unsigned long EngineClock;
    unsigned long MemoryClock;
    unsigned long Voltage;
};

struct rhdPm {
    int               scrnIndex;

    struct rhdPmState Forced;
    struct rhdPmState Stored;
    Bool              IsStored;
};

void RHDPmInit(RHDPtr rhdPtr);
void RHDPmSetClock(RHDPtr rhdPtr);
void RHDPmSave(RHDPtr rhdPtr);
void RHDPmRestore(RHDPtr rhdPtr);

struct rhdPmState RHDGetPmState(RHDPtr rhdPtr);
struct rhdPmState RHDGetDefaultPmState(RHDPtr rhdPtr);

Bool RHDSetPmState(RHDPtr rhdPtr, struct rhdPmState state);

#endif /* _RHD_PM_H */

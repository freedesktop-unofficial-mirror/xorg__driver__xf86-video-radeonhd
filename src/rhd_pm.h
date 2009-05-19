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

struct rhdPm {
	int scrnIndex;

	unsigned long ForcedEngineClock;
	unsigned long StoredEngineClock;

	Bool EnableForced;
	Bool Stored;
};

void RHDPmInit(RHDPtr rhdPtr);
void RHDPmSetClock(RHDPtr rhdPtr);
void RHDPmSave(RHDPtr rhdPtr);
void RHDPmRestore(RHDPtr rhdPtr);

unsigned long RHDGetEngineClock(RHDPtr rhdPtr);
unsigned long RHDGetDefaultEngineClock(RHDPtr rhdPtr);
unsigned long RHDGetMemoryClock(RHDPtr rhdPtr);

Bool RHDSetEngineClock(RHDPtr rhdPtr, unsigned long clk);
Bool RHDSetMemoryClock(RHDPtr rhdPtr, unsigned long clk);

#endif /* _RHD_PM_H */

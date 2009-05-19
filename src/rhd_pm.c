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

/*
 * Authors:
 *   Yang Zhao <yang@yangman.ca>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "rhd.h"
#include "rhd_pm.h"

#include "rhd_atombios.h"

void
RHDPmInit(RHDPtr rhdPtr)
{
    RHDFUNC(rhdPtr);
#ifdef ATOM_BIOS
    struct rhdPm *Pm = (struct rhdPm *) xnfcalloc(sizeof(struct rhdPm), 1);

    Pm->scrnIndex = rhdPtr->scrnIndex;
    Pm->ForcedEngineClock = 0;
    Pm->EnableForced = FALSE;

    if (rhdPtr->lowPowerMode.val.bool) {
        if (!rhdPtr->lowPowerModeEngineClock.val.integer) {
            Pm->ForcedEngineClock = RHDGetDefaultEngineClock(rhdPtr) / 2;
            Pm->EnableForced = TRUE;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Force low power mode: calculated engine clock at %ldHz\n",
                       Pm->ForcedEngineClock);
        }

        if (rhdPtr->lowPowerModeEngineClock.val.integer) {
            Pm->ForcedEngineClock = rhdPtr->lowPowerModeEngineClock.val.integer;
            Pm->EnableForced = TRUE;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "Force low power mode: forced engine clock at %ldHz\n",
                       Pm->ForcedEngineClock);
        } else {
            xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING,
                       "ForceLowPowerMode disabled: could not determine default engine clock\n");
        }
    }

    rhdPtr->Pm = Pm;
#else
    rhdPtr->Pm = NULL;
#endif
}

/*
 * set engine clock
 */
void
RHDPmSetClock(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;
    if (!Pm || !Pm->EnableForced) return;

    RHDFUNC(Pm);

    RHDSetEngineClock(rhdPtr, Pm->ForcedEngineClock);

    /* Induce logging of new engine clock */
    RHDGetEngineClock(rhdPtr);
}

/*
 * save current engine clock
 */
void
RHDPmSave(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;
    if (!Pm) return;

    RHDFUNC(Pm);

    Pm->StoredEngineClock = RHDGetEngineClock(rhdPtr);
    Pm->Stored = TRUE;
}

/*
 * restore saved engine clock
 */
void
RHDPmRestore(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;
    if (!Pm) return;

    RHDFUNC(Pm);

    if (!Pm->Stored) {
        xf86DrvMsg(Pm->scrnIndex, X_ERROR, "%s: trying to restore "
                   "uninitialized values.\n", __func__);
        return;
    }

    RHDSetEngineClock(rhdPtr, Pm->StoredEngineClock);

    /* Induce logging of new engine clock */
    RHDGetEngineClock(rhdPtr);
}

unsigned long
RHDGetEngineClock(RHDPtr rhdPtr) {
#ifdef ATOM_BIOS
    union AtomBiosArg data;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_ENGINE_CLOCK, &data) == ATOM_SUCCESS) {
        return data.clockValue;
    } else
#endif
        return 0;
}

unsigned long
RHDGetDefaultEngineClock(RHDPtr rhdPtr) {
#ifdef ATOM_BIOS
    union AtomBiosArg data;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_DEFAULT_ENGINE_CLOCK, &data) == ATOM_SUCCESS) {
        return data.clockValue;
    } else
#endif
        return 0;
}

unsigned long
RHDGetMemoryClock(RHDPtr rhdPtr) {
#ifdef ATOM_BIOS
    union AtomBiosArg data;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_MEMORY_CLOCK, &data) == ATOM_SUCCESS) {
        return data.clockValue;
    } else
#endif
        return 0;
}

Bool
RHDSetEngineClock(RHDPtr rhdPtr, unsigned long clk) {
#ifdef ATOM_BIOS
    union AtomBiosArg data;

    /* TODO: Idle first; find which idles are needed and expose them */
    data.clockValue = clk;
    return RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                           SET_ENGINE_CLOCK, &data) == ATOM_SUCCESS;
#else
    xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "AtomBIOS required to set engine clock\n");
#endif
}

Bool
RHDSetMemoryClock(RHDPtr rhdPtr, unsigned long clk) {
#ifdef ATOM_BIOS
    union AtomBiosArg data;

    /* TODO: Idle first; find which idles are needed and expose them */
    data.clockValue = clk;
    return RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                           SET_MEMORY_CLOCK, &data) == ATOM_SUCCESS;
#else
    xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "AtomBIOS required to set engine clock\n");
#endif
}

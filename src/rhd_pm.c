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
 *   Matthias Hopf <mhopf@suse.de>
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
#ifdef ATOM_BIOS
    struct rhdPm *Pm = (struct rhdPm *) xnfcalloc(sizeof(struct rhdPm), 1);
    RHDFUNC(rhdPtr);

    Pm->scrnIndex = rhdPtr->scrnIndex;

    memset (&Pm->Forced, 0, sizeof(Pm->Forced));
    memset (&Pm->Stored, 0, sizeof(Pm->Stored));
    Pm->IsStored = FALSE;

    if (rhdPtr->lowPowerMode.val.bool) {
	struct rhdPmState state = RHDGetDefaultPmState(rhdPtr);
        if (!rhdPtr->lowPowerModeEngineClock.val.integer) {
            if (state.EngineClock) {
                Pm->Forced.EngineClock = state.EngineClock / 2;
                xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "ForceLowPowerMode: "
                           "calculated engine clock at %ldkHz\n", 
                           Pm->Forced.EngineClock);
            }
            else {
                xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "ForceLowPowerMode: "
                           "downclocking engine disabled: could not determine "
                           "default engine clock\n");
            }
        }
        else {
            Pm->Forced.EngineClock = rhdPtr->lowPowerModeEngineClock.val.integer;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "ForceLowPowerMode: forced "
                       "engine clock at %ldkHz\n", Pm->Forced.EngineClock);
        }

        if (state.MemoryClock) {
            Pm->Forced.MemoryClock = state.MemoryClock / 2;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "ForceLowPowerMode: "
                        "calculated memory clock at %ldkHz\n", 
                        Pm->Forced.MemoryClock);
        }
        else {
            xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "ForceLowPowerMode: "
                        "downclocking memory disabled: could not determine "
                        "default memory clock\n");
        }

	/* FIXME: voltage */
    }

    rhdPtr->Pm = Pm;
#else
    rhdPtr->Pm = NULL;
#endif
}

/*
 * set engine and memory clocks
 */
void
RHDPmSetClock(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;
    if (!Pm) return;

    RHDFUNC(Pm);

#ifdef ATOM_BIOS
    /* ATM unconditionally enable power management features
     * if low power mode requested */
    if (rhdPtr->atomBIOS) {
	union AtomBiosArg data;

	data.val = 1;
	RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_PM_SETUP, &data);
	if (rhdPtr->ChipSet < RHD_R600) {
	    data.val = 1;
	    RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_PM_CLOCKGATING_SETUP, &data);
	}
    }
#endif

    RHDSetPmState(rhdPtr, Pm->Forced);
    /* Induce logging of new engine clock */
    RHDGetPmState(rhdPtr);
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

    Pm->Stored   = RHDGetPmState(rhdPtr);
    Pm->IsStored = TRUE;
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

    if (!Pm->IsStored) {
        xf86DrvMsg(Pm->scrnIndex, X_ERROR, "%s: trying to restore "
                   "uninitialized values.\n", __func__);
        return;
    }
    RHDSetPmState(rhdPtr, Pm->Stored);
    /* Induce logging of new engine clock */
    RHDGetPmState(rhdPtr);

#ifdef ATOM_BIOS
    /* Don't know how to save state yet - unconditionally disable */
    if (rhdPtr->atomBIOS) {
	union AtomBiosArg data;

	data.val = 0;
	RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_PM_SETUP, &data);
	if (rhdPtr->ChipSet < RHD_R600) {
	    data.val = 0;
	    RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    ATOM_PM_CLOCKGATING_SETUP, &data);
	}
    }
#endif
}

struct rhdPmState
RHDGetPmState(RHDPtr rhdPtr)
{
    struct rhdPmState state;
    memset(&state, 0, sizeof(state));

#ifdef ATOM_BIOS
    union AtomBiosArg data;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
        state.EngineClock = data.clockValue;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_MEMORY_CLOCK, &data) == ATOM_SUCCESS)
        state.MemoryClock = data.clockValue;
    /* FIXME: Voltage */
#endif
    return state;
}

struct rhdPmState
RHDGetDefaultPmState(RHDPtr rhdPtr) {
    struct rhdPmState state;
    memset(&state, 0, sizeof(state));

#ifdef ATOM_BIOS
    union AtomBiosArg data;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_DEFAULT_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
        state.EngineClock = data.clockValue;
    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
                        GET_DEFAULT_MEMORY_CLOCK, &data) == ATOM_SUCCESS)
        state.MemoryClock = data.clockValue;
    /* FIXME: Voltage */
#endif
    return state;
}

Bool
RHDSetPmState(RHDPtr rhdPtr, struct rhdPmState state)
{
#ifdef ATOM_BIOS
    union AtomBiosArg data;
    Bool ret = TRUE;

    /* TODO: Idle first; find which idles are needed and expose them */
    /* FIXME: Voltage */
    /* FIXME: If Voltage is to be rised, then do that first, then change frequencies.
     *        If Voltage is to be lowered, do it the other way round. */
    if (state.EngineClock) {
	data.clockValue = state.EngineClock;
	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    SET_ENGINE_CLOCK, &data) != ATOM_SUCCESS)
	    ret = FALSE;
    }
#if 0	/* don't do for the moment */
    if (state.MemoryClock) {
	data.clockValue = state.MemoryClock;
	if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			    SET_MEMORY_CLOCK, &data) != ATOM_SUCCESS)
	    ret = FALSE;
    }
#endif
    return ret;
#else
    xf86DrvMsg(rhdPtr->scrnIndex, X_WARNING, "AtomBIOS required to set clocks\n");
    return FALSE;
#endif
}


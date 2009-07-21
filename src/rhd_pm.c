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


#ifdef ATOM_BIOS

static char *PmLevels[] = {
    "Off", "Idle", "Slow2D", "Fast2D", "Slow3D", "Fast3D", "Max3D", "User"
} ;


/* Certain clocks require certain voltage settings */
/* TODO: So far we only know few safe points. Interpolate? */
static void rhdPmValidateSetting (struct rhdPm *Pm, struct rhdPmState *setting)
{
    uint32_t compare = setting->Voltage ? setting->Voltage : Pm->Current.Voltage;
    /* Only set to lower Voltages than compare if 0 */
    /* TODO: logic missing */
    if (! setting->Voltage)
	setting->Voltage = Pm->Default.Voltage;
}

static void rhdPmCopySetting (struct rhdPm *Pm, struct rhdPmState *to, struct rhdPmState *from)
{
    if (from->EngineClock)
	to->EngineClock = from->EngineClock;
    if (from->MemoryClock)
	to->MemoryClock = from->MemoryClock;
    if (from->Voltage)
	to->Voltage     = from->Voltage;
    rhdPmValidateSetting (Pm, to);
}

/* Have: a list of possible power settings, eventual minimum and maximum settings.
 * Want: all rhdPmState_e settings */
static void rhdPmSelectSettings (RHDPtr rhdPtr)
{
    int i;
    struct rhdPm *Pm = rhdPtr->Pm;

    /* Initialize with default; STORED state is special */
    for (i = 0; i < RHD_PM_NUM_STATES; i++)
	memcpy (&Pm->States[i], &Pm->Default, sizeof(struct rhdPmState));

    /* TODO: This still needs a lot of work */

    /* RHD_PM_OFF: minimum if available, otherwise take lowest config,
     * recalc Voltage */
    /* TODO: copy lowest config  (Q: do that for setting Minimum?) */
    Pm->States[RHD_PM_OFF].Voltage = 0;
    rhdPmCopySetting (Pm, &Pm->States[RHD_PM_OFF], &Pm->Minimum);

    /* Idle: !!!HACK!!! Take half the default */
    /* TODO: copy lowest config with default Voltage/Mem setting? */
    /* TODO: this should actually set the user mode */
    Pm->States[RHD_PM_IDLE].EngineClock = Pm->Default.EngineClock / 2;
    if (rhdPtr->lowPowerMode.val.bool) {
        if (!rhdPtr->lowPowerModeEngineClock.val.integer) {
	    Pm->States[RHD_PM_IDLE].EngineClock = Pm->Default.EngineClock / 2;
                xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
			   "ForceLowPowerMode: calculated engine clock at %dkHz\n",
			   Pm->States[RHD_PM_IDLE].EngineClock);
        } else {
            Pm->States[RHD_PM_IDLE].EngineClock = rhdPtr->lowPowerModeEngineClock.val.integer;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
		       "ForceLowPowerMode: forced engine clock at %dkHz\n",
		       Pm->States[RHD_PM_IDLE].EngineClock);
        }

        if (!rhdPtr->lowPowerModeMemoryClock.val.integer) {
	    Pm->States[RHD_PM_IDLE].MemoryClock = Pm->Default.MemoryClock / 2;
                xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
			   "ForceLowPowerMode: calculated memory clock at %dkHz\n",
			   Pm->States[RHD_PM_IDLE].MemoryClock);
        } else {
            Pm->States[RHD_PM_IDLE].MemoryClock = rhdPtr->lowPowerModeMemoryClock.val.integer;
            xf86DrvMsg(rhdPtr->scrnIndex, X_INFO,
		       "ForceLowPowerMode: forced memory clock at %dkHz\n",
		       Pm->States[RHD_PM_IDLE].MemoryClock);
        }

	/* TODO: force voltage */
	Pm->States[RHD_PM_IDLE].Voltage = 0;
	rhdPmValidateSetting (Pm, &Pm->States[RHD_PM_IDLE]);
    }

    rhdPmCopySetting (Pm, &Pm->States[RHD_PM_MAX_3D], &Pm->Maximum);

    xf86DrvMsg (rhdPtr->scrnIndex, X_INFO,
		"Power management: used engine clock / memory clock / voltage:\n");
    ASSERT (sizeof(PmLevels) / sizeof(char *) == RHD_PM_NUM_STATES);
    for (i = 0; i < RHD_PM_NUM_STATES; i++) {
	xf86DrvMsg(rhdPtr->scrnIndex, X_INFO, "  %s: %d / %d / %.3f\n",
		   PmLevels[i],
		   Pm->States[i].EngineClock, Pm->States[i].MemoryClock,
		   Pm->States[i].Voltage / 1000.0);
    }
}

static struct rhdPmState
rhdGetDefaultPmState (RHDPtr rhdPtr) {
    struct rhdPmState state;
    union AtomBiosArg data;

    memset (&state, 0, sizeof(state));
    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_DEFAULT_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
	state.EngineClock = data.clockValue;
    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_DEFAULT_MEMORY_CLOCK, &data) == ATOM_SUCCESS)
        state.MemoryClock = data.clockValue;
    /* FIXME: Voltage */

    return state;
}

static struct rhdPmState
rhdPmGetRawState (RHDPtr rhdPtr)
{
    union AtomBiosArg data;

    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
        rhdPtr->Pm->Current.EngineClock = data.clockValue;
    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_MEMORY_CLOCK, &data) == ATOM_SUCCESS)
        rhdPtr->Pm->Current.MemoryClock = data.clockValue;
    /* FIXME: Voltage */

    return rhdPtr->Pm->Current;
}

static Bool
rhdPmSetRawState (RHDPtr rhdPtr, struct rhdPmState state)
{
    union AtomBiosArg data;
    Bool ret = TRUE;

    /* TODO: Idle first; find which idles are needed and expose them */
    /* FIXME: Voltage */
    /* FIXME: If Voltage is to be rised, then do that first, then change frequencies.
     *        If Voltage is to be lowered, do it the other way round. */
    if (state.EngineClock && state.EngineClock != rhdPtr->Pm->Current.EngineClock) {
	data.clockValue = state.EngineClock;
	if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_SET_ENGINE_CLOCK, &data) != ATOM_SUCCESS)
	    ret = FALSE;
    }
#if 0	/* don't do for the moment */
    if (state.MemoryClock && state.MemoryClock != rhdPtr->Pm->Current.MemoryClock) {
	data.clockValue = state.MemoryClock;
	if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_SET_MEMORY_CLOCK, &data) != ATOM_SUCCESS)
	    ret = FALSE;
    }
#endif
    rhdPmGetRawState (rhdPtr);
    return ret;
}


/*
 * API
 */

static Bool
rhdPmSelectState (RHDPtr rhdPtr, enum rhdPmState_e num)
{
    return rhdPmSetRawState (rhdPtr, rhdPtr->Pm->States[num]);
}

static Bool
rhdPmDefineState (RHDPtr rhdPtr, enum rhdPmState_e num, struct rhdPmState state)
{
    ASSERT(0);
}

void RHDPmInit(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = (struct rhdPm *) xnfcalloc(sizeof(struct rhdPm), 1);
    RHDFUNC(rhdPtr);

    rhdPtr->Pm = Pm;

    Pm->scrnIndex   = rhdPtr->scrnIndex;
    Pm->SelectState = rhdPmSelectState;
    Pm->DefineState = rhdPmDefineState;

    rhdPmGetRawState (rhdPtr);			/* Sets Pm->Current */
    Pm->Default     = rhdGetDefaultPmState (rhdPtr);

    /* TODO: Get all settings */

    if (! Pm->Default.Voltage)
	rhdPmValidateSetting (Pm, &Pm->Default);
    if (! Pm->Default.EngineClock || ! Pm->Default.MemoryClock)
	memcpy (&Pm->Default, &Pm->Current, sizeof (Pm->Current));
    if (! Pm->Default.EngineClock || ! Pm->Default.MemoryClock) {
	/* Not getting the default setting is fatal */
	xfree (Pm);
	rhdPtr->Pm = NULL;
	return;
    }

    rhdPmCopySetting (Pm, &Pm->Minimum, &Pm->Default);
    rhdPmCopySetting (Pm, &Pm->Maximum, &Pm->Default);
    /* TODO: get min/max settings if possible from AtomBIOS */

    rhdPmSelectSettings (rhdPtr);
}

#else		/* ATOM_BIOS */

void
RHDPmInit (RHDPtr rhdPtr)
{
    rhdPtr->Pm = NULL;
}

#endif		/* ATOM_BIOS */


/*
 * save current engine clock
 */
void
RHDPmSave (RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;
    RHDFUNC(rhdPtr);

#ifdef ATOM_BIOS
    /* ATM unconditionally enable power management features
     * if low power mode requested */
    if (rhdPtr->atomBIOS) {
	union AtomBiosArg data;

	data.val = 1;
	RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_PM_SETUP, &data);
	if (rhdPtr->ChipSet < RHD_R600) {
	    data.val = 1;
	    RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_PM_CLOCKGATING_SETUP, &data);
	}
    }
#endif

    if (!Pm) return;

    Pm->Stored   = rhdPmGetRawState (rhdPtr);
}

/*
 * restore saved engine clock
 */
void
RHDPmRestore (RHDPtr rhdPtr)
{
    struct rhdPm *Pm = rhdPtr->Pm;

    RHDFUNC(rhdPtr);

#ifdef ATOM_BIOS
    /* Don't know how to save state yet - unconditionally disable */
    if (rhdPtr->atomBIOS) {
	union AtomBiosArg data;

	data.val = 0;
	RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_PM_SETUP, &data);
	if (rhdPtr->ChipSet < RHD_R600) {
	    data.val = 0;
	    RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_PM_CLOCKGATING_SETUP, &data);
	}
    }
#endif

    if (!Pm)
	return;

    if (! Pm->Stored.EngineClock && ! Pm->Stored.MemoryClock) {
        xf86DrvMsg (Pm->scrnIndex, X_ERROR, "%s: trying to restore "
		    "uninitialized values.\n", __func__);
        return;
    }
    rhdPmSetRawState (rhdPtr, Pm->Stored);
    /* Induce logging of new engine clock */
    rhdPmGetRawState (rhdPtr);
}


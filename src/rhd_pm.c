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

#define COMPARE_MIN_ENGINE_CLOCK	100000
#define SAVE_MIN_ENGINE_CLOCK		200000
#define COMPARE_MAX_ENGINE_CLOCK	3000000
#define COMPARE_MIN_MEMORY_CLOCK	100000
#define SAVE_MIN_MEMORY_CLOCK		200000
#define COMPARE_MAX_MEMORY_CLOCK	3000000
#define COMPARE_MIN_VOLTAGE		500
#define COMPARE_MAX_VOLTAGE		2000

static char *PmLevels[] = {
    "Off", "Idle", "Slow2D", "Fast2D", "Slow3D", "Fast3D", "Max3D", "User"
} ;


static void
rhdPmPrint (struct rhdPm *Pm, char *name, struct rhdPmState *state)
{
    xf86DrvMsg(Pm->scrnIndex, X_INFO, "  %-8s %8d kHz / %8d kHz / %6.3f V\n",
	       name, state->EngineClock, state->MemoryClock,
	       state->Voltage / 1000.0);
}

/* Certain clocks require certain voltage settings */
/* TODO: So far we only know few safe points. Interpolate? */
static void rhdPmValidateSetting (struct rhdPm *Pm, struct rhdPmState *setting, int forceVoltage)
{
    uint32_t compare = setting->Voltage ? setting->Voltage : Pm->Current.Voltage;
    if (! setting->EngineClock)
	setting->EngineClock = Pm->Current.EngineClock;
    if (setting->EngineClock < Pm->Minimum.EngineClock)
	setting->EngineClock = Pm->Minimum.EngineClock;
    if (setting->EngineClock < COMPARE_MIN_ENGINE_CLOCK)
	setting->EngineClock = SAVE_MIN_ENGINE_CLOCK;
    if (setting->EngineClock > Pm->Maximum.EngineClock)
	setting->EngineClock = Pm->Maximum.EngineClock;
    if (setting->EngineClock > COMPARE_MAX_ENGINE_CLOCK)
	setting->EngineClock = Pm->Default.EngineClock;
    if (setting->EngineClock > COMPARE_MAX_ENGINE_CLOCK)
	setting->EngineClock = 0;
    if (! setting->MemoryClock)
	setting->MemoryClock = Pm->Current.MemoryClock;
    if (setting->MemoryClock < Pm->Minimum.MemoryClock)
	setting->MemoryClock = Pm->Minimum.MemoryClock;
    if (setting->MemoryClock < COMPARE_MIN_MEMORY_CLOCK)
	setting->MemoryClock = SAVE_MIN_MEMORY_CLOCK;
    if (setting->MemoryClock > Pm->Maximum.MemoryClock)
	setting->MemoryClock = Pm->Maximum.MemoryClock;
    if (setting->MemoryClock > COMPARE_MAX_MEMORY_CLOCK)
	setting->MemoryClock = Pm->Default.MemoryClock;
    if (setting->MemoryClock > COMPARE_MAX_MEMORY_CLOCK)
	setting->MemoryClock = 0;
    if (! setting->Voltage)
	setting->Voltage     = Pm->Current.Voltage;
    if (setting->Voltage     < Pm->Minimum.Voltage)
	setting->Voltage     = Pm->Minimum.Voltage;
    if (setting->Voltage     < COMPARE_MIN_VOLTAGE)
	setting->Voltage     = Pm->Current.Voltage;
    if (setting->Voltage     < COMPARE_MIN_VOLTAGE)
	setting->Voltage     = 0;
    if (setting->Voltage     > Pm->Maximum.Voltage)
	setting->Voltage     = Pm->Maximum.Voltage;
    if (setting->Voltage     > COMPARE_MAX_VOLTAGE)
	setting->Voltage     = Pm->Default.Voltage;
    if (setting->Voltage     > COMPARE_MAX_VOLTAGE)
	setting->Voltage     = 0;
    /* TODO: voltage adaption logic missing */
    /* Only set to lower Voltages than compare if 0 */
}

static void rhdPmCopySetting (struct rhdPm *Pm, struct rhdPmState *to, struct rhdPmState *from)
{
    if (from->EngineClock)
	to->EngineClock = from->EngineClock;
    if (from->MemoryClock)
	to->MemoryClock = from->MemoryClock;
    if (from->Voltage)
	to->Voltage     = from->Voltage;
    rhdPmValidateSetting (Pm, to, 0);
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

	rhdPmValidateSetting (Pm, &Pm->States[RHD_PM_IDLE], 1);
    }

    rhdPmCopySetting (Pm, &Pm->States[RHD_PM_MAX_3D], &Pm->Maximum);

    xf86DrvMsg (rhdPtr->scrnIndex, X_INFO,
		"Power management: used engine clock / memory clock / voltage:\n");
    ASSERT (sizeof(PmLevels) / sizeof(char *) == RHD_PM_NUM_STATES);
    for (i = 0; i < RHD_PM_NUM_STATES; i++)
	rhdPmPrint (Pm, PmLevels[i], &Pm->States[i]);
}

static void
rhdPmGetRawState (RHDPtr rhdPtr, struct rhdPmState *state)
{
    union AtomBiosArg data;

    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
        state->EngineClock = data.clockValue;
    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_MEMORY_CLOCK, &data) == ATOM_SUCCESS)
        state->MemoryClock = data.clockValue;
    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_VOLTAGE, &data) == ATOM_SUCCESS)
        state->Voltage = data.val;
}

static Bool
rhdPmSetRawState (RHDPtr rhdPtr, struct rhdPmState *state)
{
    union AtomBiosArg data;
    Bool ret = TRUE;
    struct rhdPmState dummy;

    /* TODO: Idle first; find which idles are needed and expose them */
    /* FIXME: Voltage */
    /* FIXME: If Voltage is to be rised, then do that first, then change frequencies.
     *        If Voltage is to be lowered, do it the other way round. */
    if (state->EngineClock && state->EngineClock != rhdPtr->Pm->Current.EngineClock) {
	data.clockValue = state->EngineClock;
	if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_SET_ENGINE_CLOCK, &data) == ATOM_SUCCESS)
	    rhdPtr->Pm->Current.EngineClock = state->EngineClock;
	else
	    ret = FALSE;
    }
#if 0	/* don't do for the moment */
    if (state->MemoryClock && state->MemoryClock != rhdPtr->Pm->Current.MemoryClock) {
	data.clockValue = state->MemoryClock;
	if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_SET_MEMORY_CLOCK, &data) != ATOM_SUCCESS)
	    rhdPtr->Pm->Current.MemoryClock = state->MemoryClock;
	else
	    ret = FALSE;
    }
#endif
#if 0	/* don't do for the moment */
    if (state->Voltage && state->Voltage != rhdPtr->Pm->Current.Voltage) {
	data.val = state->Voltage;
	if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			     ATOM_SET_VOLTAGE, &data) != ATOM_SUCCESS)
	    rhdPtr->Pm->Current.Voltage = state->Voltage;
	else
	    ret = FALSE;
    }
#endif

    /* AtomBIOS might change values, so that later comparisons would fail, even
     * if re-setting wouldn't change the actual values. So don't save real
     * state in Current, but update only to current values. */
    rhdPmGetRawState (rhdPtr, &dummy);
    return ret;
}


/*
 * API
 */

static Bool
rhdPmSelectState (RHDPtr rhdPtr, enum rhdPmState_e num)
{
    return rhdPmSetRawState (rhdPtr, &rhdPtr->Pm->States[num]);
}

static Bool
rhdPmDefineState (RHDPtr rhdPtr, enum rhdPmState_e num, struct rhdPmState *state)
{
    ASSERT(0);
}

void RHDPmInit(RHDPtr rhdPtr)
{
    struct rhdPm *Pm = (struct rhdPm *) xnfcalloc(sizeof(struct rhdPm), 1);
    union AtomBiosArg data;
    RHDFUNC(rhdPtr);

    rhdPtr->Pm = Pm;

    Pm->scrnIndex   = rhdPtr->scrnIndex;
    Pm->SelectState = rhdPmSelectState;
    Pm->DefineState = rhdPmDefineState;

    if (RHDAtomBiosFunc (rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			 ATOM_GET_CHIP_LIMITS, &data) != ATOM_SUCCESS) {
	/* Not getting the information is fatal */
	xfree (Pm);
	rhdPtr->Pm = NULL;
	return;
    }
    Pm->Minimum.EngineClock = data.chipLimits.MinEngineClock;
    Pm->Minimum.MemoryClock = data.chipLimits.MinMemoryClock;
    Pm->Minimum.Voltage     = data.chipLimits.MinVDDCVoltage;
    Pm->Maximum.EngineClock = data.chipLimits.MaxEngineClock;
    Pm->Maximum.MemoryClock = data.chipLimits.MaxMemoryClock;
    Pm->Maximum.Voltage     = data.chipLimits.MaxVDDCVoltage;
    Pm->Default.EngineClock = data.chipLimits.DefaultEngineClock;
    Pm->Default.MemoryClock = data.chipLimits.DefaultMemoryClock;
    Pm->Default.Voltage     = data.chipLimits.DefaultVDDCVoltage;

    memcpy (&Pm->Current, &Pm->Default, sizeof (Pm->Default));
    rhdPmGetRawState (rhdPtr, &Pm->Current);

    xf86DrvMsg (rhdPtr->scrnIndex, X_INFO, "Power management: Raw Ranges\n");
    rhdPmPrint (Pm, "Global Minimum", &Pm->Minimum);
    rhdPmPrint (Pm, "Global Maximum", &Pm->Maximum);
    rhdPmPrint (Pm, "Global Default", &Pm->Default);

    /* TODO: Get all settings */

    /* Validate */
    if (! Pm->Default.EngineClock || ! Pm->Default.MemoryClock)
	memcpy (&Pm->Default, &Pm->Current, sizeof (Pm->Current));
    rhdPmValidateSetting (Pm, &Pm->Default, 1);
    rhdPmValidateSetting (Pm, &Pm->Minimum, 1);
    rhdPmValidateSetting (Pm, &Pm->Maximum, 1);
    if (Pm->Maximum.EngineClock < Pm->Default.EngineClock)
	Pm->Maximum.EngineClock = Pm->Default.EngineClock;
    if (Pm->Maximum.MemoryClock < Pm->Default.MemoryClock)
	Pm->Maximum.MemoryClock = Pm->Default.MemoryClock;
    if (Pm->Maximum.Voltage     < Pm->Default.Voltage)
	Pm->Maximum.Voltage     = Pm->Default.Voltage;
    if (Pm->Minimum.EngineClock > Pm->Default.EngineClock && Pm->Default.EngineClock)
	Pm->Minimum.EngineClock = Pm->Default.EngineClock;
    if (Pm->Minimum.MemoryClock > Pm->Default.MemoryClock && Pm->Default.MemoryClock)
	Pm->Minimum.MemoryClock = Pm->Default.MemoryClock;
    if (Pm->Minimum.Voltage     > Pm->Default.Voltage     && Pm->Default.Voltage)
	Pm->Minimum.Voltage     = Pm->Default.Voltage;
    if (! Pm->Minimum.Voltage || ! Pm->Maximum.Voltage)
	Pm->Minimum.Voltage = Pm->Maximum.Voltage = Pm->Default.Voltage = 0;

    xf86DrvMsg (rhdPtr->scrnIndex, X_INFO, "Power management: Validated Ranges\n");
    rhdPmPrint (Pm, "Global Minimum", &Pm->Minimum);
    rhdPmPrint (Pm, "Global Maximum", &Pm->Maximum);
    rhdPmPrint (Pm, "Global Default", &Pm->Default);

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

    memcpy (&Pm->Stored, &Pm->Default, sizeof (Pm->Default));
    rhdPmGetRawState (rhdPtr, &Pm->Stored);
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
    rhdPmSetRawState (rhdPtr, &Pm->Stored);
}


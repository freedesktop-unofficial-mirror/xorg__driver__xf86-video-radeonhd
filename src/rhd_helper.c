#include "xf86.h"
#include "rhd.h"

void
RhdGetOptValBool(const OptionInfoRec *table, int token,
                 RHDOptPtr optp, Bool def)
{
    if (!(optp->set = xf86GetOptValBool(table, token, &optp->val.bool))){
	optp->set = FALSE;
        optp->val.bool = def;
    } else
	optp->set = TRUE;
}

void
RhdGetOptValInteger(const OptionInfoRec *table, int token,
                 RHDOptPtr optp, int def)
{
    if (!(optp->set = xf86GetOptValInteger(table, token, &optp->val.integer))){
	optp->set = FALSE;
        optp->val.integer = def;
    } else
	optp->set = TRUE;
}

void
RhdGetOptValULong(const OptionInfoRec *table, int token,
                 RHDOptPtr optp, unsigned long def)
{
    if (!(optp->set = xf86GetOptValULong(table, token, &optp->val.uslong))) {
	optp->set = FALSE;
        optp->val.uslong = def;
    } else
	optp->set = TRUE;
}

void
RhdGetOptValReal(const OptionInfoRec *table, int token,
                 RHDOptPtr optp, double def)
{
    if (!(optp->set = xf86GetOptValReal(table, token, &optp->val.real))) {
	optp->set = FALSE;
        optp->val.real = def;
    } else
	optp->set = TRUE;
}

void
RhdGetOptValFreq(const OptionInfoRec *table, int token,
                 OptFreqUnits expectedUnits, RHDOptPtr optp, double def)
{
    if (!(optp->set = xf86GetOptValFreq(table, token, expectedUnits,
                                        &optp->val.freq))) {
	optp->set = FALSE;
        optp->val.freq = def;
    } else
	optp->set = TRUE;
}

void
RhdGetOptValString(const OptionInfoRec *table, int token,
                   RHDOptPtr optp, char *def)
{
    if (!(optp->val.string = xf86GetOptValString(table, token))) {
	optp->set = FALSE;
        optp->val.string = def;
    } else
	optp->set = TRUE;
}

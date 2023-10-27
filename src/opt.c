#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include "opt.h"

const char *argp_program_version = "vac 0.2";
const char *argp_program_bug_address = "<killingrain@gmail.com>";
static char doc[] =
	"VAC -- a program for measuring I-V curves\v"
	"TODO: This part of the documentation comes *after* the options; "
	"note that the text is automatically filled, but it's possible "
	"to force a line-break, e.g.\n<-- here.";
static char args_doc[] = "SAMPLE_NAME";

#define OPT_V_START 1 // --V_start
#define OPT_V_STOP  2 // --V_stop
#define OPT_V_STEP  3 // --V_step
#define OPT_I_MAX   4 // --I_max
#define OPT_RF      5 // --Rf
#define OPT_DELAY   6 // --Delay

// The options we understand
static struct argp_option options[] =
{
	{0,0,0,0, "I-V parameters:", 0},
	{"V_start" , OPT_V_START, "double", 0, "Start voltage, V   (-60.0 - 60.0, default  0.0 )", 0},
	{"V_stop"  , OPT_V_STOP , "double", 0, "Stop voltage, V    (-60.0 - 60.0, default  1.0 )", 0},
	{"V_step"  , OPT_V_STEP , "double", 0, "Voltage step, V    (0.001 -  1.0, default  0.1 )", 0},
	{"I_max"   , OPT_I_MAX  , "double", 0, "Maximum current, A (0.001 -  0.1, default  0.01)", 0},
	{0,0,0,0, "Required:", 0},
	{"Rf"      , OPT_RF     , "double", 0, "Feedback resistance of TIA, Ohm (+1.0e+03 - +1.0e07)", 0},
	{"delay"   , OPT_DELAY  , "double", 0, "Scanning delay time, s          (+1.0e-03 - +1.0e03)", 0},
	{0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *a = state->input;
	double t;

	switch (key)
	{
		case OPT_V_START:
			t = atof(arg);
			if ((t < -60.0) || (t > 60.0))
			{
				fprintf(stderr, "# E: <V_start> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->V_start = t;
			break;

		case OPT_V_STOP:
			t = atof(arg);
			if ((t < -60.0) || (t > 60.0))
			{
				fprintf(stderr, "# E: <V_stop> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->V_stop = t;
			break;

		case OPT_V_STEP:
			t = atof(arg);
			if ((t < 0.001) || (t > 1.0))
			{
				fprintf(stderr, "# E: <V_step> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->V_step = t;
			break;

		case OPT_I_MAX:
			t = atof(arg);
			if ((t < 0.001) || (t > 0.1))
			{
				fprintf(stderr, "# E: <I_max> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->I_max = t;
			break;

		case OPT_RF:
			t = atof(arg);
			if ((t < 1.0e3) || (t > 1.0e7))
			{
				fprintf(stderr, "# E: <Rf> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->Rf = t;
			a->Rf_flag = 1;
			break;

		case OPT_DELAY:
			t = atof(arg);
			if ((t < 1.0e-3) || (t > 1.0e3))
			{
				fprintf(stderr, "# E: <Delay> is out of range. See \"vac --help\"\n");
				return ARGP_ERR_UNKNOWN;
			}
			a->Delay = t;
			a->Delay_flag = 1;
			break;

		case ARGP_KEY_ARG:
			a->sample_name = arg;
			a->sample_name_flag = 1;
			break;

		case ARGP_KEY_NO_ARGS:
			fprintf(stderr, "# E: <sample_name> has not specified. See \"vac --help\"\n");
			a->sample_name_flag = 0;
			//argp_usage (state);
			return ARGP_ERR_UNKNOWN;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};

int parse_arguments(int argc, char **argv, struct arguments *arg)
{
	return argp_parse(&argp, argc, argv, 0, NULL, arg);
}

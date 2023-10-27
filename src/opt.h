#pragma once

struct arguments
{
	int    sample_name_flag;
	char  *sample_name;
	double V_start;
	double V_stop;
	double V_step;
	double I_max;
	int    Rf_flag;
	double Rf;
	int    Delay_flag;
	double Delay;
};

int parse_arguments(int argc, char **argv, struct arguments *arg);

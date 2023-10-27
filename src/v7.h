#pragma once

struct v7
{
	int dev;
	int status;
	int V_range;
};

int v7_open        (struct v7 *v);
int v7_close       (struct v7 *v);
int v7_init        (struct v7 *v);
int v7_get_voltage (struct v7 *v, double *voltage);

#pragma once

struct st
{
	int dev;
	int status;
};

int st_open        (struct st *v);
int st_close       (struct st *v);
int st_init        (struct st *v);
int st_deinit      (struct st *v);
int st_get_voltage (struct st *v, double *voltage);

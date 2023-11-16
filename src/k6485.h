#pragma once

struct k6485
{
	int dev;
	int status;
};

int k6485_open        (struct k6485 *k);
int k6485_close       (struct k6485 *k);
int k6485_init        (struct k6485 *k);
int k6485_get_current (struct k6485 *k, double *current);

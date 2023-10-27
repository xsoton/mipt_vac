#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "gpib.h"
#include "v7.h"

// === [GPIB] ===
#define V7_GPIB_NAME "AKIP-V7-78/1"

// === [RANGE CHECKER] ===
#define V_100mV 0
#define V_1V    1
#define V_10V   2
#define V_100V  3

static double V_hi[4] = {0.095, 0.95, 9.5, 95.0};
static double V_lo[4] = {0.085, 0.85, 8.5, 85.0};

// returns new range
static int V_check_range (int range, double V)
{
	if ((range < 0) || (range > 3))
	{
		return -1;
	}

	if (range == 0)
	{
		if (fabs(V) <= V_hi[range])
		{
			return range;
		}
		else
		{
			range++;
		}
	}

	while((range >= 1) && (range <= 3))
	{
		if (fabs(V) < V_lo[range-1])
		{
			range--;
			continue;
		}

		if (fabs(V) > V_hi[range])
		{
			range++;
			continue;
		}

		break;
	}

	return range;
}

int v7_open (struct v7 *v)
{
	int r;
	
	r = gpib_open(V7_GPIB_NAME);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to open v7 voltmeter (%d)\n", r);
		return -1;
	}
	
	v->dev     = r;
	v->status  = 1;
	v->V_range = V_100mV;

	return 0;
}

int v7_close (struct v7 *v)
{
	int r = 0;

	if (v->status == 0)
	{
		fprintf(stderr, "# E: Unable to close v7 voltmeter\n");
		return -1;
	}

	r = gpib_close(v->dev);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to close v7 voltmeter (%d)\n", r);
		return -1;
	}

	return 0;
}

int v7_init (struct v7 *v)
{
	int r;

	r = gpib_write(v->dev, "function \"voltage:dc\"");   if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "voltage:dc:range:auto off"); if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "voltage:dc:range 0.1");      if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "voltage:dc:nplcycles 10");   if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "trigger:source immediate");  if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "trigger:delay:auto off");    if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "trigger:delay 0");           if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "trigger:count 1");           if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "sample:count 1");            if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "sense:zero:auto on");        if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}

	v->V_range = V_100mV;

	return 0;
}

int v7_get_voltage (struct v7 *v, double *voltage)
{
	int r;
	char buf[300] = {0};
	int new_V_range;

	r = gpib_write(v->dev, "read?"); if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n",  r); return -1;}
	r = gpib_read(v->dev, buf, 300); if(r < 0){fprintf(stderr, "# E: Unable to read from v7 voltmeter (%d)\n", r); return -1;}
	*voltage = atof(buf);

	new_V_range = V_check_range(v->V_range, *voltage);
	if (new_V_range == -1)
	{
		fprintf(stderr, "# E: Unable to check range of v7 voltmeter (%d)\n",  r);
		return -1;
	}

	if (new_V_range != v->V_range)
	{
		v->V_range = new_V_range;
		switch(v->V_range)
		{
			case V_100mV:
				r = gpib_write(v->dev, "voltage:dc:range 0.1"); if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
				break;
			case V_1V:
				r = gpib_write(v->dev, "voltage:dc:range 1");   if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
				break;
			case V_10V:
				r = gpib_write(v->dev, "voltage:dc:range 10");  if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
				break;
			case V_100V:
				r = gpib_write(v->dev, "voltage:dc:range 100"); if(r < 0){fprintf(stderr, "# E: Unable to write to v7 voltmeter (%d)\n", r); return -1;}
				break;
		}
	}

	return 0;
}

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "gpib.h"
#include "pps.h"

// === [GPIB] ===
#define PPS_GPIB_NAME "AKIP-1142/3G"

int pps_open (struct pps *p)
{
	int r;
	
	r = gpib_open(PPS_GPIB_NAME);
	if(r == -1)
	{
		fprintf(stderr, "# E: Unable to open power supply (%d)\n", r);
		return -1;
	}
	
	p->dev = r;
	p->status = 1;

	return 0;
}

int pps_close (struct pps *p)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	r = gpib_close(p->dev);
	if(r == -1)
	{
		fprintf(stderr, "# E: Unable to close pps (%d)\n", r);
		return -1;
	}

	p->dev = 0;
	p->status = 0;

	return 0;
}

int pps_init (struct pps *p)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	r = gpib_write(p->dev, "output 0");             if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 3"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage:limit 5.1V");   if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 2"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage:limit 60.1V");  if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 1"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage:limit 60.1V");  if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	usleep(1e6);
	r = gpib_write(p->dev, "instrument:nselect 2"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "channel:output 1");     if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 1"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "channel:output 1");     if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}

	return 0;
}

int pps_deinit (struct pps *p)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	r = gpib_write(p->dev, "instrument:nselect 1"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 2"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage 0");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	usleep(1e6);
	r = gpib_write(p->dev, "instrument:nselect 1"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "channel:output 0");     if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "instrument:nselect 2"); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "channel:output 0");     if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_write(p->dev, "system:beeper");        if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}

	return 0;
}

int pps_set_voltage (struct pps *p, int chan, double voltage)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	if ((chan < 0) || (chan > 3))
	{
		fprintf(stderr, "# E: pps chan is out of range (chan = %d)\n", chan);
		return -1;
	}

	if ((voltage < 0.0) || (voltage > 60.0))
	{
		fprintf(stderr, "# E: pps voltage is out of range\n");
		return -1;
	}

	r = gpib_print(p->dev, "instrument:nselect %d", chan); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "voltage %.3lf", voltage);      if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}

	return 0;
}

int pps_set_current (struct pps *p, int chan, double current)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	if ((chan < 0) || (chan > 3))
	{
		fprintf(stderr, "# E: pps chan is out of range (chan = %d)\n", chan);
		return -1;
	}

	if ((current < 0.0) || (current > 60.0))
	{
		fprintf(stderr, "# E: pps current is out of range\n");
		return -1;
	}

	r = gpib_print(p->dev, "instrument:nselect %d", chan); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}
	r = gpib_print(p->dev, "current %.3lf", current);      if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n", r); return -1;}

	return 0;
}

int pps_get_voltage (struct pps *p, int chan, double *voltage)
{
	int r;
	char buf[300] = {0};

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	if ((chan < 0) || (chan > 3))
	{
		fprintf(stderr, "# E: pps chan is out of range (chan = %d)\n", chan);
		return -1;
	}

	if (voltage == NULL)
	{
		fprintf(stderr, "# E: pps voltage is NULL\n");
		return -1;
	}

	r = gpib_print(p->dev, "instrument:nselect %d", chan); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n",  r); return -1;}
	r = gpib_print(p->dev, "measure:voltage?");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n",  r); return -1;}
	r = gpib_read(p->dev, buf, 300);                       if(r == -1){fprintf(stderr, "# E: Unable to read from pps (%d)\n", r); return -1;}
	*voltage = atof(buf);

	return 0;
}

int pps_get_current (struct pps *p, int chan, double *current)
{
	int r;
	char buf[300] = {0};

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	if ((chan < 0) || (chan > 3))
	{
		fprintf(stderr, "# E: pps chan is out of range (chan = %d)\n", chan);
		return -1;
	}

	if (current == NULL)
	{
		fprintf(stderr, "# E: pps current is NULL\n");
		return -1;
	}

	r = gpib_print(p->dev, "instrument:nselect %d", chan); if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n",  r); return -1;}
	r = gpib_print(p->dev, "measure:current?");            if(r == -1){fprintf(stderr, "# E: Unable to write to pps (%d)\n",  r); return -1;}
	r = gpib_read(p->dev, buf, 300);                       if(r == -1){fprintf(stderr, "# E: Unable to read from pps (%d)\n", r); return -1;}
	*current = atof(buf);

	return 0;
}

int pps_set_v12 (struct pps *p, double voltage)
{
	int r;

	if (p->status == 0)
	{
		fprintf(stderr, "# E: pps status is zero\n");
		return -1;
	}

	if ((voltage < -60.0) || (voltage > 60.0))
	{
		fprintf(stderr, "# E: pps voltage is out of range\n");
		return -1;
	}

	if (voltage >= 0)
	{
		r = pps_set_voltage(p, 1, voltage); if(r == -1){fprintf(stderr, "# E: pps set voltage (%d)\n",  r); return -1;}
		r = pps_set_voltage(p, 2, 0);       if(r == -1){fprintf(stderr, "# E: pps set voltage (%d)\n",  r); return -1;}
	}
	else
	{
		r = pps_set_voltage(p, 1, 0);       if(r == -1){fprintf(stderr, "# E: pps set voltage (%d)\n",  r); return -1;}
		r = pps_set_voltage(p, 2, voltage); if(r == -1){fprintf(stderr, "# E: pps set voltage (%d)\n",  r); return -1;}
	}

	return 0;
}

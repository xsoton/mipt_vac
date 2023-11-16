#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "gpib.h"
#include "k6485.h"

#define K6485_GPIB_NAME "KEITHLEY_6485"

int k6485_open (struct k6485 *k)
{
	int r;
	
	r = gpib_open(K6485_GPIB_NAME);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to open k6485 voltmeter (%d)\n", r);
		return -1;
	}
	
	k->dev     = r;
	k->status  = 1;

	r = gpib_write(k->dev, "*rst"); if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}

	usleep(500000);

	return 0;
}

int k6485_close (struct k6485 *k)
{
	int r = 0;

	if (k->status == 0)
	{
		fprintf(stderr, "# E: Unable to close k6485 voltmeter\n");
		return -1;
	}

	r = gpib_write(k->dev, "*rst"); if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}

	usleep(500000);

	r = gpib_close(k->dev);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to close k6485 voltmeter (%d)\n", r);
		return -1;
	}

	return 0;
}

int k6485_init (struct k6485 *k)
{
	int r;

	r = gpib_write(k->dev, "system:clear");             if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "configure:current");        if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "current:nplcycles 5.0");    if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "system:zcheck on");         if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "current:range 2e-9");       if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "init");                     if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	usleep(500000);
	r = gpib_write(k->dev, "system:zcorrect:acquire");  if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "current:range:auto on");    if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "system:zcheck off");        if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "arm:source immediate");     if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "arm:count 1");              if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "arm:timer 0.001");          if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "trigger:source immediate"); if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "trigger:count 1");          if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	// r = gpib_write(k->dev, "trigger:delay 0");          if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "format ascii");             if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	r = gpib_write(k->dev, "format:elements reading"); if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n", r); return -1;}
	usleep(500000);

	return 0;
}

int k6485_get_current (struct k6485 *k, double *current)
{
	int r;
	char buf[300] = {0};

	r = gpib_write(k->dev, "read?"); if(r<0){fprintf(stderr, "# E: Unable to write to k6485 voltmeter (%d)\n",  r); return -1;}
	r = gpib_read(k->dev, buf, 300); if(r<0){fprintf(stderr, "# E: Unable to read from k6485 voltmeter (%d)\n", r); return -1;}
	*current = atof(buf);

	return 0;
}

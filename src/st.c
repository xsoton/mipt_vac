#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "gpib.h"
#include "st.h"

// === [GPIB] ===
#define ST_GPIB_NAME "SOLARTRON_7150"

int st_open (struct st *v)
{
	int r;
	
	r = gpib_open(ST_GPIB_NAME);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to open st voltmeter (%d)\n", r);
		return -1;
	}
	
	v->dev     = r;
	v->status  = 1;

	return 0;
}

int st_close (struct st *v)
{
	int r = 0;

	if (v->status == 0)
	{
		fprintf(stderr, "# E: Unable to close st voltmeter\n");
		return -1;
	}

	r = gpib_close(v->dev);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to close st voltmeter (%d)\n", r);
		return -1;
	}

	return 0;
}

int st_init (struct st *v)
{
	int r;

	if (v->status == 0)
	{
		fprintf(stderr, "# E: Unable to init st voltmeter\n");
		return -1;
	}

	r = gpib_write(v->dev, "M0R0I3N1T0U0"); if(r < 0){fprintf(stderr, "# E: Unable to write to st voltmeter (%d)\n", r); return -1;}
	r = gpib_write(v->dev, "D1");           if(r < 0){fprintf(stderr, "# E: Unable to write to st voltmeter (%d)\n", r); return -1;}

	return 0;
}

int st_deinit (struct st *v)
{
	int r;

	if (v->status == 0)
	{
		fprintf(stderr, "# E: Unable to deinit st voltmeter\n");
		return -1;
	}

	r = gpib_write(v->dev, "D0"); if(r < 0){fprintf(stderr, "# E: Unable to write to st voltmeter (%d)\n", r); return -1;}

	return 0;
}

int st_get_voltage (struct st *v, double *voltage)
{
	int r;
	char buf[300] = {0};

	if (v->status == 0)
	{
		fprintf(stderr, "# E: Unable to get voltage from st voltmeter\n");
		return -1;
	}

	r = gpib_write(v->dev, "G");     if(r < 0){fprintf(stderr, "# E: Unable to write to st voltmeter (%d)\n",  r); return -1;}
	r = gpib_read(v->dev, buf, 300); if(r < 0){fprintf(stderr, "# E: Unable to read from st voltmeter (%d)\n", r); return -1;}
	*voltage = atof(buf);

	return 0;
}

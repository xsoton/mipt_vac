struct pps
{
	int dev;
	int status;
}

int pps_open        (struct pps *p);
int pps_close       (struct pps *p);
int pps_init        (struct pps *p);
int pps_set_voltage (struct pps *p, int chan, double  voltage);
int pps_set_current (struct pps *p, int chan, double  current);
int pps_get_voltage (struct pps *p, int chan, double *voltage);
int pps_get_current (struct pps *p, int chan, double *current);

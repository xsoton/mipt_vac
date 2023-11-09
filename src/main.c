#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <argp.h>
#include <error.h>
#include "gpib.h"
#include "st.h"
#include "v7.h"
#include "pps.h"
#include "opt.h"

// === [DATE] ===
struct tm start_time_struct;

// === [SOURCE] ===
#define V_START +1e+00
#define V_STOP  -1e+00
#define V_STEP  +1e-01
#define I_MAX   +1e-02
#define RF      +1e+06
#define DELAY   +1e-02

// === threads ====
static void *commander(void *);
static void *worker(void *);

// === utils ===
static int    get_run();
static void   set_run(int run_new);
static int    get_next();
static void   set_next(int next_new);
static double get_time();

static int direction(double start, double stop);

// === global variables
static char dir_str[200];
static pthread_rwlock_t run_lock;
static int run;
static pthread_rwlock_t next_lock;
static int next;
static char filename_vac[250];
struct arguments arg = {0};

// === measurements ===
enum meas_state
{
	M_BEFORE = 0,
	M_STAGE1,
	M_STAGE2,
	M_STAGE3,
	M_AFTER,
	M_STOP
};

// #define DEBUG

// === program entry point
int main(int argc, char **argv)
{
	int ret = 0;
	int status;

	time_t start_time;
	struct tm start_time_struct;

	pthread_t t_commander;
	pthread_t t_worker;

	// === parse input parameters
	arg.sample_name_flag = 0;
	arg.sample_name      = NULL;
	arg.V_start          = V_START;
	arg.V_stop           = V_STOP;
	arg.V_step           = V_STEP;
	arg.I_max            = I_MAX;
	arg.Rf_flag          = 0;
	arg.Rf               = RF;
	arg.Delay_flag       = 0;
	arg.Delay            = DELAY;

	status = parse_arguments(argc, argv, &arg);
	if ((status != 0) ||
		(arg.sample_name_flag != 1) ||
		(arg.Delay_flag != 1))
	{
		fprintf(stderr, "# E: Error while parsing. See \"vac --help\"\n");
		ret = -1;
		goto main_exit;
	}

	#ifdef DEBUG
		fprintf(stderr, "# sample_name_flag = %d\n" , arg.sample_name_flag);
		fprintf(stderr, "# sample_name      = %s\n" , arg.sample_name);
		fprintf(stderr, "# V_start          = %le\n", arg.V_start);
		fprintf(stderr, "# V_stop           = %le\n", arg.V_stop);
		fprintf(stderr, "# V_step           = %le\n", arg.V_step);
		fprintf(stderr, "# I_max            = %le\n", arg.I_max);
		fprintf(stderr, "# Rf_flag          = %d\n" , arg.Rf_flag);
		fprintf(stderr, "# Rf               = %le\n", arg.Rf);
		fprintf(stderr, "# Delay_flag       = %d\n" , arg.Delay_flag);
		fprintf(stderr, "# Delay            = %le\n", arg.Delay);
	#endif

	// === get start time of experiment ===
	start_time = time(NULL);
	localtime_r(&start_time, &start_time_struct);

	// === we need actual information w/o buffering
	setlinebuf(stdout);
	setlinebuf(stderr);

	// === initialize run state variable
	pthread_rwlock_init(&run_lock, NULL);
	run = 1;

	// === initialize next state variable
	pthread_rwlock_init(&next_lock, NULL);
	next = 0;

	// === create dirictory in "20191012_153504_<experiment_name>" format
	snprintf(dir_str, 200, "%04d-%02d-%02d_%02d-%02d-%02d_%s",
		start_time_struct.tm_year + 1900,
		start_time_struct.tm_mon + 1,
		start_time_struct.tm_mday,
		start_time_struct.tm_hour,
		start_time_struct.tm_min,
		start_time_struct.tm_sec,
		arg.sample_name
	);
	status = mkdir(dir_str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (status == -1)
	{
		fprintf(stderr, "# E: unable to create experiment directory (%s)\n", strerror(errno));
		ret = -2;
		goto main_exit;
	}

	// === create file names
	snprintf(filename_vac, 250, "%s/vac.dat", dir_str);
	// printf("filename_vac \"%s\"\n", filename_vac);

	// === now start threads
	pthread_create(&t_commander, NULL, commander, NULL);
	pthread_create(&t_worker, NULL, worker, NULL);

	// === and wait ...
	pthread_join(t_worker, NULL);

	// === cancel commander thread becouse we don't need it anymore
	// === and wait for cancelation finish
	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

	fprintf(stdout, "\r\n");

	main_exit:
	return ret;
}

// === commander function
static void *commander(void *a)
{
	(void) a;

	char str[100];
	char *s;
	int ccount;

	while(get_run())
	{
		fprintf(stdout, "> ");

		s = fgets(str, 100, stdin);
		if (s == NULL)
		{
			fprintf(stderr, "# E: Exit\n");
			set_run(0);
			break;
		}

		switch(str[0])
		{
			case 'h':
				printf(
					"Help:\n"
					"\th -- this help;\n"
					"\tn -- next stage;\n"
					"\tq -- exit the program;\n");
				break;
			case 'n':
				set_next(1);
				break;
			case 'q':
				set_run(0);
				break;
			default:
				ccount = strlen(str)-1;
				fprintf(stderr, "# E: Unknown command (%.*s)\n", ccount, str);
				break;
		}
	}

	return NULL;
}

// === worker function
static void *worker(void *a)
{
	(void) a;

	int r;

	struct pps   pps = {0};
	struct st    vm  = {0};
	struct v7    am  = {0};

	int    vac_index;
	double vac_time;
	double vac_voltage;
	double vac_current;

	double pps_voltage1;
	double pps_voltage2;
	double pps_current1;
	double pps_current2;
	double vm_voltage;
	double am_voltage;

	double voltage;
	int dir;

	FILE  *vac_fp;
	FILE  *gp;
	char   buf[300];

	enum meas_state state = M_BEFORE;
	int i1, i2, i3;

	double V_start, V_stop, V_step;

	V_start = arg.V_start;
	V_stop  = arg.V_stop;
	V_step  = arg.V_step;

	r = pps_open(&pps);
	if(r < 0)
	{
		fprintf(stderr, "# E: pps open (%d)\n", r);
		goto worker_pps_close;
	}

	r = pps_init(&pps);
	if(r < 0)
	{
		fprintf(stderr, "# E: pps intit (%d)\n", r);
		goto worker_pps_close;
	}

	r = v7_open(&am);
	if(r < 0)
	{
		fprintf(stderr, "# E: v7 open (%d)\n", r);
		goto worker_am_close;
	}

	r = v7_init(&am);
	if(r < 0)
	{
		fprintf(stderr, "# E: v7 init (%d)\n", r);
		goto worker_am_close;
	}

	r = st_open(&vm);
	if(r < 0)
	{
		fprintf(stderr, "# E: st open (%d)\n", r);
		goto worker_vm_close;
	}

	r = st_init(&vm);
	if(r < 0)
	{
		fprintf(stderr, "# E: st init (%d)\n", r);
		goto worker_vm_close;
	}

	// === create vac file
	vac_fp = fopen(filename_vac, "w+");
	if(vac_fp == NULL)
	{
		fprintf(stderr, "# E: Unable to open file \"%s\" (%s)\n", filename_vac, strerror(ferror(vac_fp)));
		goto worker_vac_fopen;
	}
	setlinebuf(vac_fp);

	// fprintf(stderr, "1\n");

	// === write vac header
	r = fprintf(vac_fp,
		"# Measuring of I-V curve\n"
		"# I vs V\n"
		"# Date: %04d.%02d.%02d %02d:%02d:%02d\n"
		"# Start parameters:\n"
		"#   sample_name = %s\n"
		"#   V_start     = %le\n"
		"#   V_stop      = %le\n"
		"#   V_step      = %le\n"
		"#   I_max       = %le\n"
		"#   Rf          = %le\n"
		"#   Ts          = %le\n"
		"#  1: index\n"
		"#  2: time, s\n"
		"#  3: V, V\n"
		"#  4: I, A\n"
		"#  5: pps:ch1 voltage, V\n"
		"#  6: pps:ch1 current, A\n"
		"#  7: pps:ch2 voltage, V\n"
		"#  8: pps:ch2 current, A\n"
		"#  9: vm voltage, V\n"
		"# 10: am voltage, V\n",
		start_time_struct.tm_year + 1900,
		start_time_struct.tm_mon + 1,
		start_time_struct.tm_mday,
		start_time_struct.tm_hour,
		start_time_struct.tm_min,
		start_time_struct.tm_sec,
		arg.sample_name,
		arg.V_start,
		arg.V_stop,
		arg.V_step,
		arg.I_max,
		arg.Rf,
		arg.Delay
	);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", filename_vac, strerror(r));
		goto worker_vac_header;
	}

	// === open gnuplot
	snprintf(buf, 300, "gnuplot > %s/gnuplot.log 2>&1", dir_str);
	gp = popen(buf, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: Unable to open gnuplot pipe (%s)\n", strerror(errno));
		goto worker_gp_popen;
	}
	setlinebuf(gp);

	// === prepare gnuplot
	r = fprintf(gp,
		"set term qt noraise\n"
		"set xzeroaxis lt -1\n"
		"set yzeroaxis lt -1\n"
		"set grid\n"
		"set key right bottom\n"
		// "set xrange [%le:%le]\n"
		"set xlabel \"Bias, V\"\n"
		"set ylabel \"Current, A\"\n"
		"set format y \"%%.3s%%c\"\n"
	);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to print to gp (%s)\n", strerror(r));
		goto worker_gp_settings;
	}

	// === let the action begins!
	vac_index = 0;

	r = pps_set_v12(&pps, 0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set init voltage\n");
		goto worker_set_init_voltage;
	}

	if (fabs(V_start) < V_step)
		state = M_STAGE2;
	else
		state = M_STAGE1;

	i1 = i2 = i3 = 0;


// set_run(0);
	while(get_run())
	{
		switch(state)
		{
			case M_STAGE1:
				dir = direction(0.0, V_start);
				if (get_next())
				{
					V_start = voltage + V_step * dir;
					state = M_STAGE2;
					set_next(0);
				}
				else
				{
					voltage = 0.0 + i1 * V_step * dir;
					if (((dir > 0) && (voltage >= V_start)) || ((dir < 0) && (voltage <= V_start)))
						state = M_STAGE2;
					else
					{
						i1++;
						vac_index++;
						break;
					}
				}
			case M_STAGE2:
				dir = direction(V_start, V_stop);
				if (get_next())
				{
					V_stop = voltage + V_step * dir;
					state = M_STAGE3;
					set_next(0);
				}
				else
				{
					voltage = V_start + i2 * V_step * dir;
					if (((dir > 0) && (voltage >= V_stop)) || ((dir < 0) && (voltage <= V_stop)))
						state = M_STAGE3;
					else
					{
						i2++;
						vac_index++;
						break;
					}
				}
			case M_STAGE3:
				if (get_next())
				{
					state = M_AFTER;
					set_next(0);
					break;
				}
				else
				{
					dir = direction(V_stop, 0.0);
					voltage = V_stop + i3 * V_step * dir;
					if (((dir > 0) && (voltage >= 0.0)) || ((dir < 0) && (voltage <= 0.0)))
						state = M_AFTER;
					else
					{
						i3++;
						vac_index++;
						break;
					}
				}
			default:
				state = M_STOP;
		}

		if (state > M_STAGE3)
		{
			set_run(0);
			break;
		}

		// fprintf(stderr, "\n");
		// fprintf(stderr, "# state = %d\n", state);
		fprintf(stderr, "\r# voltage = %lf", voltage);
		fflush(stderr);

		// fprintf(stderr, "# pps_set_v12 %lf\n", voltage);
		r = pps_set_v12(&pps, voltage);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to set voltage (%d)\n", r);
			set_run(0);
			break;
		}

		// fprintf(stderr, "# delay\n");
		usleep(arg.Delay * 1e6);

		// fprintf(stderr, "# get_time\n");
		vac_time = get_time();
		if (vac_time < 0)
		{
			fprintf(stderr, "# E: Unable to get time (%lf)\n", vac_time);
			set_run(0);
			break;
		}

		// fprintf(stderr, "# pps_get_voltage 1\n");
		r = pps_get_voltage(&pps, 1, &pps_voltage1);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get pps 1 voltage (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# pps_voltage1 = %lf\n", pps_voltage1);

		// fprintf(stderr, "# pps_get_current 1\n");
		r = pps_get_current(&pps, 1, &pps_current1);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get pps 1 current (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# pps_current1 = %lf\n", pps_current1);

		// fprintf(stderr, "# pps_get_voltage 2\n");
		r = pps_get_voltage(&pps, 2, &pps_voltage2);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get pps 2 voltage (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# pps_voltage2 = %lf\n", pps_voltage2);

		// fprintf(stderr, "# pps_get_current 2\n");
		r = pps_get_current(&pps, 2, &pps_current2);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get pps 2 current (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# pps_current2 = %lf\n", pps_current2);

		// fprintf(stderr, "# v7_get_voltage\n");
		r = v7_get_voltage(&am, &am_voltage);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get am voltage (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# am_voltage = %lf\n", am_voltage);

		// fprintf(stderr, "# st_get_voltage\n");
		r = st_get_voltage(&vm, &vm_voltage);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to get vm voltage (%d)\n", r);
			set_run(0);
			break;
		}
		// fprintf(stderr, "# vm_voltage = %lf\n", vm_voltage);

		// vm_voltage = 0;

		// vac_voltage = pps_voltage1 - pps_voltage2;
		vac_voltage = vm_voltage;
		vac_current = am_voltage / arg.Rf;

		fprintf(stderr, "\tV = %lf, I = %le", vac_voltage, vac_current);
		fflush(stderr);

		// fprintf(stderr, "# vac_voltage = %lf\n", vac_voltage);
		// fprintf(stderr, "# vac_current = %le\n", vac_current);

		r = fprintf(vac_fp, "%d\t%+le\t%+le\t%+le\t%+le\t%+le\t%+le\t%+le\t%+le\t%+le\n",
			vac_index,
			vac_time,
			vac_voltage,
			vac_current,
			pps_voltage1,
			pps_voltage2,
			pps_current1,
			pps_current2,
			vm_voltage,
			am_voltage
		);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", filename_vac, strerror(r));
			set_run(0);
			break;
		}

		r = fprintf(gp, "set title \"i = %d, t = %.3lf s\"\n", vac_index, vac_time);
		r = fprintf(gp, "plot \"%s\" u 3:4 w l lw 1 title \"V = %.3lf V, I = %le A\"\n",
			filename_vac,
			vac_voltage,
			vac_current
		);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to gp (%s)\n", strerror(r));
			set_run(0);
			break;
		}

		vac_index++;
	}

	// fprintf(stderr, "# pps_deinit\n");
	pps_deinit(&pps);

	r = fprintf(gp, "exit;\n");
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to print to gp (%s)\n", strerror(r));
	}

	worker_set_init_voltage:

	worker_gp_settings:

	r = pclose(gp);
	if (r == -1)
	{
		fprintf(stderr, "# E: Unable to close gnuplot pipe (%s)\n", strerror(errno));
	}
	worker_gp_popen:


	worker_vac_header:

	r = fclose(vac_fp);
	if (r == EOF)
	{
		fprintf(stderr, "# E: Unable to close file \"%s\" (%s)\n", filename_vac, strerror(errno));
	}
	worker_vac_fopen:

	st_deinit(&vm);
	st_close(&vm);
	worker_vm_close:

	v7_close(&am);
	worker_am_close:

	pps_close(&pps);
	worker_pps_close:

	return NULL;
}

// === utils
static int get_run()
{
	int run_local;
	pthread_rwlock_rdlock(&run_lock);
		run_local = run;
	pthread_rwlock_unlock(&run_lock);
	return run_local;
}

static void set_run(int run_new)
{
	pthread_rwlock_wrlock(&run_lock);
		run = run_new;
	pthread_rwlock_unlock(&run_lock);
}

static int get_next()
{
	int next_local;
	pthread_rwlock_rdlock(&next_lock);
		next_local = next;
	pthread_rwlock_unlock(&next_lock);
	return next_local;
}

static void set_next(int next_new)
{
	pthread_rwlock_wrlock(&next_lock);
		next = next_new;
	pthread_rwlock_unlock(&next_lock);
}

static double get_time()
{
	static int first = 1;
	static struct timeval t_first = {0};
	struct timeval t = {0};
	double ret;
	int r;

	if (first == 1)
	{
		r = gettimeofday(&t_first, NULL);
		if (r == -1)
		{
			fprintf(stderr, "# E: unable to get time (%s)\n", strerror(errno));
			ret = -1;
		}
		else
		{
			ret = 0.0;
			first = 0;
		}
	}
	else
	{
		r = gettimeofday(&t, NULL);
		if (r == -1)
		{
			fprintf(stderr, "# E: unable to get time (%s)\n", strerror(errno));
			ret = -2;
		}
		else
		{
			ret = (t.tv_sec - t_first.tv_sec) * 1e6 + (t.tv_usec - t_first.tv_usec);
			ret /= 1e6;
		}
	}

	return ret;
}

static int direction(double start, double stop)
{
	return (stop >= start) ? 1 : -1;
}

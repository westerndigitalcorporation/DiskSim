/*
 * A Driver for disksim that generates block requests to match
 * the seek parameters in the [device].seek file. 
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
//#include "syssim_interface.h"
#include "disksim_interface.h"
#include "disksim_rand48.h"
#include "skippy_opts.h"

typedef	struct	{
  int n;
  double sum;
  double sqr;
} Stat;

static double now = 0;		/* current time */
static double next_event = -1;	/* next event */
static int completed = 0;	/* last request was completed */
static Stat st;

void
panic(const char *s)
{
  perror(s);
  exit(1);
}


void
add_statistics(Stat *s, double x)
{
  s->n++;
  s->sum += x;
  s->sqr += x*x;
}


void
print_statistics(Stat *s, const char *title)
{
  double avg, std;

  avg = s->sum/s->n;
  std = sqrt((s->sqr - 2*avg*s->sum + s->n*avg*avg) / s->n);
  printf("%s: n=%d average=%f std. deviation=%f\n", title, s->n, avg, std);
}


void
syssim_schedule_callback(disksim_interface_callback_t fn, 
			 double t, 
			 void *ctx)
{
  next_event = t;
}


/*
 * de-scehdule a callback.
 */
void
syssim_deschedule_callback(double t, void *ctx)
{
  next_event = -1;
}

void
syssim_report_completion(double t, struct disksim_request *r, void *ctx)
{
  completed = 1;
  now = t;
  add_statistics(&st, t - r->start);
}

int seek_sim(
		struct disksim_interface *disksim,
		const int measurements,
		const size_t sector_size,
		const int write_flag)
{
    double t; 
    double sum;
    unsigned long pos=0;
    int nbytes; 
    int i; 
    struct disksim_request r;
    int base = 0;

    fprintf(stdout, "# Seek distances measured: %d\n", measurements);

    sum = 0;
    //    for ( base = 0; base < sector_size*measurements; base += sector_size )
    //    for ( base = 0; base < sector_size*1000; base += 4096 )
    {
      pos = 0;
	for (i=0; i<measurements; i++) 
	{
	  r.start = now;
	  //	  r.flags = DISKSIM_READ;
	  r.flags = DISKSIM_WRITE;
	  r.devno = 0;
	  r.bytecount = sector_size;
	  //	  r.blkno = base;	  r.blkno = base;
	  r.blkno = 0;
	  
	  /* reset the disk position */
	  if (write_flag) {
	    r.flags = DISKSIM_WRITE;
	  }
	  
	  disksim_interface_request_arrive(disksim, now, &r);

	  /* Process events until this I/O is completed */
	  while(next_event >= 0) {
	    now = next_event;
	    next_event = -1;
	    disksim_interface_internal_event(disksim, now, 0);
	  }

	  pos = 0;

	  /* save the current time */
	  
	  /* lseek(fd, i*sector_size, SEEK_CUR) */
	  r.start = now;
	  r.flags = DISKSIM_WRITE;
	  r.devno = 0;
	  r.bytecount = sector_size;
	  r.blkno = i+base/sector_size;
	  
	  /* reset the disk position */
	  if (write_flag) {
	    r.flags = DISKSIM_WRITE;
	  }
	  
	  t = now;
	  disksim_interface_request_arrive(disksim, now, &r);

	  /* Process events until this I/O is completed */
	  while(next_event >= 0) {
	    now = next_event;
	    next_event = -1;
	    disksim_interface_internal_event(disksim, now, 0);
	  }
	  
	  fprintf(stdout, "%d %lf\n", i, now-t);
	}

      }

    return 0; 
}



int
main(int argc, char *argv[])
{
	int nsectors;
	//	struct stat; 
	//	struct disksim *disksim, *disksim2;
	struct stat buf;
	int len = 8192000;
	int test_encapsulation;
	struct disksim_interface* disksim;

	struct gengetopt_args_info args_info; 

	if (argc != 4 || (nsectors = atoi(argv[3])) <= 0) {
	  fprintf(stderr, "usage: %s <param file> <output file> <#nsectors>\n",
		  argv[0]);
	  exit(1);
	}

	if (stat(argv[1], &buf) < 0)
      	  panic(argv[1]);

  //	nsectors = args_info.sectors_arg; 
	test_encapsulation = 0;//args_info.test_encapsulation_flag; 

	disksim = disksim_interface_initialize(argv[1], 
					       argv[2],
					       syssim_report_completion,
					       syssim_schedule_callback,
					       syssim_deschedule_callback,
					       0,
					       0,
					       0);

	/* This simulates the skippy algorithm */
	seek_sim(disksim, 
		 nsectors,
		 512,
		 0); 



	//	if (test_encapsulation) {
	//		disksim_interface_shutdown(disksim2, now);
	//	}

	exit(0);
}

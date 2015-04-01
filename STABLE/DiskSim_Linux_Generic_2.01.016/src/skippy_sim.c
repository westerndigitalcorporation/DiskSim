/*
 * A Driver for disksim that generates block requests based on the 
 * skippy algorithm from~\cite{talagala:extract}
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
//#include "syssim_interface.h"
#include "disksim_interface.h"
/*#include "disksim_rand48.h"*/
#include "disksim_interface_private.h"
#include "skippy_opts.h"


#define	BLOCK	4096
#define	SECTOR	512 
#define	BLOCK2SECTOR	(BLOCK/SECTOR)

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

int
main(int argc, char *argv[])
{
  int i;
  int nsectors;
  struct stat buf;
  struct disksim_request r;
  struct disksim_interface *disksim;
  unsigned long pos=0;

  int measurements;
  struct stat; 
  int len = 8192000;
  int test_encapsulation;
  struct disksim_interface* iface;  
  struct gengetopt_args_info args_info; 

  double t;
  
  if (argc != 4 || (measurements = atoi(argv[3])) <= 0) {
    fprintf(stderr, "usage: %s <param file> <output file> <#measurements>\n",
	    argv[0]);
    exit(1);
  }

  if (stat(argv[1], &buf) < 0)
    panic(argv[1]);

  disksim = disksim_interface_initialize(argv[1], 
					 argv[2],
					 syssim_report_completion,
					 syssim_schedule_callback,
					 syssim_deschedule_callback,
					 0,
					 0,
					 0);

  /* NOTE: it is bad to use this internal disksim call from external... */

  for (i=1; i < measurements; i++) {
    r.start = now;
    //    r.flags = DISKSIM_READ;
    r.flags = DISKSIM_WRITE;
    r.devno = 0;
    
    pos = pos+i*SECTOR;

    /* NOTE: it is bad to use this internal disksim call from external... */
    r.blkno =  pos/SECTOR;

    r.bytecount = SECTOR;
    completed = 0;

    t = now;
    disksim_interface_request_arrive(disksim, now, &r);

    /* Process events until this I/O is completed */
    while(next_event >= 0) {
      now = next_event;
      next_event = -1;
      disksim_interface_internal_event(disksim, now, 0);
    }

    fprintf(stdout, "%d %g\n",i,(now-t)*1000.0);

    if (!completed) {
      fprintf(stderr,
	      "%s: internal error. Last event not completed %d\n",
	      argv[0], i);
      exit(1);
    }
    pos += SECTOR;
  }

  disksim_interface_shutdown(disksim, now);

  print_statistics(&st, "response time");

  exit(0);
}

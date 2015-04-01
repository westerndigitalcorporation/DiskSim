/* diskmodel (version 1.1)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2003-2005
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
 */

// The g4 skew analysis works by generating a trace of requests to
// issue to the disk, and then analyzing the result of that to derive
// the skew values.

// The trace follows the structure of the g4 layout search tree for
// the disk's layout mapping.  We effectively do a depth-first search
// from the top down with a recursive algorithm.  Operating at each
// node, we take the lowest LBN in that node as the "zero point" and
// measure the angular offset of the first lbn in each entry in the
// node from the first entry of the whole node.  Then we recursively
// expand the structure of the lbn range of each index entry.

// The implementation does both passes in the same code path.

#include <libddbg/libddbg.h>
#include <libparam/libparam.h>

#include <disksim_interface.h>
#include "../layout_g4.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define MAX_OUT 4

struct dsstuff {
  struct disksim_interface *iface;
  double now;
  double next;

  // see the completion callback
  double compl[MAX_OUT];
  int compl_next;
};

struct trace {
  FILE *fp;
};

// Disksim parameters to disable the disk cache.
static char *nocache_over[] = {
  "disk0", "Enable caching in buffer", "0",
  "disk0", "Fast write level", "0", 
  "disk0", "Buffer continuous read", "0", 
  "disk0", "Read any free blocks", "0", 
  "disk0", "Minimum read-ahead (blks)", "0", 
  "disk0", "Maximum read-ahead (blks)", "0"
};

// forward progress -- max times to loop
#define FP 40

enum { GENTRACE, CALIB } mode = CALIB;

void
schedule_callback(disksim_interface_callback_t fn, 
		  double t, 
		  void *ctx) {
  struct dsstuff *ds = (struct dsstuff *)ctx;
  ds->next = t;
}

void
deschedule_callback(double t, void *ctx) {
  struct dsstuff *ds = (struct dsstuff *)ctx;
  ds->next = -1;
}

// Disksim interface request completion callback.
void cb(double t, struct disksim_request *r, void *ctx) {
  //  double *d = (double *)ctx;
  struct dsstuff *ds = (struct dsstuff *)ctx;

  // This is a dumb hack to deal with ds iface being wrong -- the
  // ctx here *should* be the per-req ctx (->reqctx) but it doesn't
  // have the demux code.  
  ds->compl[ds->compl_next++] = t;
}

// Read lbns l1 and l2 back-to-back, timing the second request.  In
// trace generation mode, we just print out the IOs we would do.  In
// calibration mode, we inject both IOs into an instance of disksim.
double
time_second_request(int l1, int l2, struct dsstuff *ds, struct trace *t,
		    struct dm_disk_if *d) {
  struct disksim_request *r1, *r2;

  ddbg_assert(l1 < d->dm_sectors);
  ddbg_assert(l2 < d->dm_sectors);

  if(mode == GENTRACE) {
    fprintf(t->fp, "%f 0 %d 1 1\n", ds->now, l1);
    fprintf(t->fp, "%f 0 %d 1 1\n", ds->now + 0.1, l2);
    ds->now += 20.0;
  } else {
    r1 = calloc(1, sizeof(*r1));
    r2 = calloc(1, sizeof(*r2));

    r1->blkno = l1;
    r1->bytecount = 512;
    r1->flags |= DISKSIM_READ;
    r1->start = ds->now;
    //    r1->reqctx = &t1;
  
    r2->blkno = l2;
    r2->bytecount = 512;
    r2->flags |= DISKSIM_READ;
    r2->start = ds->now + 0.001;
    //    r2->reqctx = &t2;
  
    disksim_interface_request_arrive(ds->iface, ds->now, r1);
    disksim_interface_request_arrive(ds->iface, ds->now, r2);
  
    // pump disksim

    do {
      ds->now = ds->next;
      ds->next = -1;
      disksim_interface_internal_event(ds->iface, ds->now, 0);
    } while(ds->next != -1);

    // roll now forward slightly, some small ~1ms int-arr
    ds->now += 1.0;

    free(r1);
    free(r2);
    r1 = NULL;
    r2 = NULL;

    //    printf("%d -> %d : %f\n", l1, l2, ds->compl[1] - ds->compl[0]);

    ds->compl_next = 0;
  }

  return ds->compl[1] - ds->compl[0]; 
}

// Open the trace file for reading or writing according to the mode.
struct trace *
setup_trace(char *name) {
  struct trace *t = calloc(1, sizeof(*t));

  if(mode == GENTRACE) {
    t->fp = fopen(name, "w");
  } else {
    t->fp = fopen(name, "r");
  }

  return t;
}

// Parse the next request in the input trace in disksim "validate"
// format.
int
trace_get_next(struct trace *t, int *lbn, double *result) {
  int rv;

  rv = fscanf(t->fp, "%*s %*s %d %*d %lf %*f", lbn, result) == 2;
  *result /=  1000.0;
  
  return rv;
}

// Get the timing for accessing lbn2 after lbn1 from the trace.
double
get_tracetime(struct trace *t, int lbn1, int lbn2) {
  double result, t1;
  int l1, l2;
  
  do { // try to resync
    ddbg_assert(trace_get_next(t, &l1, &t1));
  } while(l1 != lbn1);
  ddbg_assert(trace_get_next(t, &l2, &result));

  ddbg_assert(l1 == lbn1);
  ddbg_assert(l2 == lbn2);
  return result;
}


// We actually want the last lbn on the source track.  If you use the
// first lbn on the source track, some disks will make it every time,
// some will make it about half of the time and have a rotation miss
// the other half.
int
adjust_lbns(struct dm_disk_if *d, int *l1, int *l2) {
  struct dm_pbn pbn;
  int l0, ln;

  if(l1) {
    d->layout->dm_translate_ltop(d, *l1, MAP_FULL, &pbn, 0);
    d->layout->dm_get_track_boundaries(d, &pbn, &l0, &ln, 0);
    *l1 = ln;
  }

  if(l2) {
    d->layout->dm_translate_ltop(d, *l2, MAP_FULL, &pbn, 0);
    d->layout->dm_get_track_boundaries(d, &pbn, &l0, &ln, 0);
    *l2 = l0;
  }

  return 0;
}

// The following functions are used to do a linear least-squares fit
// following the article at mathworld.wolfram.com
static double
mean(double *x, int n) {
  int i;
  double sum = 0.0;
  double mean;
  for(i = 0; i < n; i++) {
    sum += x[i];
  }
  
  mean = sum / n;
  return mean;
}

// sum of squares
static double
ss(double *x, int n) {
  int i;
  double sum = 0.0;
  double res;
  for(i = 0; i < n; i++) {
    sum += (x[i] * x[i]);
  }
  res = sum - n * mean(x,n) * mean(x,n);
  return res;
}

static double
ssxy(double *x, double *y, int n) {
  int i;
  double sum = 0.0;
  double res;
  for(i = 0; i < n; i++) {
    sum += (x[i] * y[i]);
  }
  res = sum - (n * mean(x,n) * mean(y,n));
  return res;
}

// variance
double vari(double *x, int n) {
  double res = ss(x,n) / n;
  return res;
}

double covar(double *x, double *y, int n) {
  double res = ssxy(x,y,n) / n;
  return res;
}

// y = a + bx
// The data is the collection of pairs (x[i], y[i]) for i in [0,n)
// Returns the correlation coeffecient, r
double
linreg(double *x, double *y, int n, double *a, double *b) {
  double r;
  *b = ssxy(x,y,n) / ss(x,n);
  *a = mean(y,n) - *b * mean(x,n);

  r = sqrt(ssxy(x,y,n) * ssxy(x,y,n) / (ss(x,n) * ss(y,n)));

  return r;
}

// Compute the difference between the samples in d1 and d2, returning
// the slope of a line through the points, (i, d1[i] - d2[i])
// for i in [0,n)
// Returns the y intercept in aa.
// Return value is the slope of the fitted line.  If it decides we're
// done, returns 0.0
double
find_slope(double *d1, double *d2, int n, double *aa,
	   double period) {
  int i;
  double a,b,r;
  double yi;
  double *err = calloc(n, sizeof(*err));
  double *x = calloc(n, sizeof(*x));
  double fit_err, tot_err;
  double var;
  double thresh = period / 2.0;

  int addct = 0;
  int n2 = 0;
  
  if(n == 1) {
    *aa = d1[0] - d2[0];
    return 0.0;
  }

  if(n == 2) {
    *aa = 0.0;
    return d1[1] - d2[1];
  }

  for(i = 1; i < n; i++) {
    yi = d1[i] - d2[i];

    if(-thresh <= yi && yi <= thresh) {
      //      printf("%3d\t%f\n", i, yi);
      err[n2] = yi;
      x[n2] = i;
      n2++;
    }
  }

  r = linreg(x, err, n2, &a, &b);

  for(i = 0; i < n2; i++) {
    printf("%3d\t%f\n", (int)x[i], err[i]);
  }

  var = vari(err,n);

  printf("%s() a = %f, b = %f, r = %f, var = %f\n", __func__, a, b, r, var);

  *aa = a;

  free(err);
  free(x);
  err = NULL;
  x = NULL;

  return b;
}

// Make an angle be within [0,1)
double fix_angle(double a) {
  if(a < 0) {
    return a - (int)a + 1;
  } else {
    return a - (int)a;
  }
}


// Time accessing lbn li immediately after accessing lbn l0 and convert the
// time into an angle in [0,1) according to the rotational period of
// the disk.
double 
measure_one_skew(struct dm_disk_if *d,
		 struct dsstuff *ds,
		 struct trace *trace,
		 int l0,
		 int li) {
  double t, tt, a = 0.0;
  double period = dm_time_itod(d->mech->dm_period(d));

  adjust_lbns(d, &l0, &li);

  t = time_second_request(l0, li, ds, trace, d);
  if(mode == CALIB) {
    tt = get_tracetime(trace, l0, li);
    a = fix_angle(-(t - tt) / period);
  }

  return a;
}


// do_idx_ent() does the hard work to measure the skews for a single
// index entry.  We need to determine e->off, the angular offset of
// the first lbn of this instance taking the first lbn of parent as
// the 0 point and e->alen, the angular offset of the i+1st instance
// of e from the ith.
void
do_idx_ent(struct dm_disk_if *d,        // diskmodel
	   struct dm_layout_g4 *l,      // root of g4 layout
	   struct idx_ent *e,           // the entry in question
	   struct idx *parent,          // The index containing e
	   struct idx_ent *parent_e,    // The entry for parent in its parent
	   int parent_off,              // Which entry we are in parent
	   struct dsstuff *ds,          // Disksim instance
	   struct trace *t,             // io trace
	   int lbn) {                   // The first lbn of parent
  int i;
  int l0;
  int dist;

  double yi;

  double *times;
  double *tracetimes;

  // for bootstrapping off and len
  double off0time, len0time;
  int off0lbn[2], len0lbn[2];

  // Work internally in floating-point, then convert back to the
  // integer representation at the end.
  double aoff = dm_angle_itod(e->off);
  double alen =  dm_angle_itod(e->alen);
  
  double period = dm_time_itod(d->mech->dm_period(d));

  // Number of instances of this entry.
  int quot = e->runlen / e->len;

  // times[i] is disksim's prediction of the amount of time to access the 
  // first lbn of the ith instance after accessing the first instance.
  times = calloc(quot, sizeof(double));
  // Actual times from the trace replay against the real disk.
  tracetimes = calloc(quot, sizeof(double));
  // li[i] contains the first lbn of the ith instance of e.
  int *li = calloc(quot, sizeof(*li));

  printf("%s() lbn %d e->lbn %d alen %f off %f quot %d\n", 
	 __func__, lbn, e->lbn, alen, aoff, quot);

  if(e->alen != 0 || e->off != 0) {
    printf("%s() already done, apparently\n", __func__);
    return;
  }

  // Bootstrap offset by measuring the first instance.
  if(e->lbn != 0) {
    aoff = measure_one_skew(d, ds, t, lbn, lbn + e->lbn);
    e->off = dm_angle_dtoi(aoff);
  }

  // Check for the end of the lbn space.
  if(lbn + e->lbn + e->len >= d->dm_sectors) {
    e->alen = dm_angle_dtoi(0.0);
    return;
  }

  // Bootstrap alen by measuring the skew from the first to the second
  // instance.
  alen = measure_one_skew(d, ds, t, lbn + e->lbn, 
			  lbn + e->lbn + e->len);
  e->alen = dm_angle_dtoi(alen);    

  printf("first off %f len %f\n", aoff, alen);

  // Expand out the lbns of all of the instances.  For calibration
  // (second pass), read in the values from the trace.  To generate
  // the trace (first pass), time how long it takes to read the first
  // lbn of the ith instance after reading the last lbn on the first
  // track of the first instance.
  for(i = 1; i < quot; i++) {
    l0 = lbn;
    li[i] = l0 + e->lbn + e->len * i;

    adjust_lbns(d, &l0, &li[i]);

    if(mode == CALIB) {
      tracetimes[i] = get_tracetime(t, l0, li[i]);

      printf("%d (%d,%d) trace %f pred %f\n", i, 
 	     l0, li[i], 
 	     tracetimes[i], times[i]);
    }
    else {
      time_second_request(l0, li[i], ds, t, d);
    }
  }

  if(mode == GENTRACE) {
    return;
  }

  // Now calibrate the value.  We first look at the first 2 instances,
  // then 4, then 8, etc, refining our estimate at each step.
  for(dist = 2; dist < quot ; ) {
    // y = a + bx; r is the correlation coeffecient.
    double a, b;
    for(i = 1; i < dist; i++) {
      times[i] = time_second_request(l0, li[i], ds, t, d);
      printf("%d (%d,%d) trace %f pred %f\n", i, 
	     l0, li[i], 
	     tracetimes[i], times[i]);
    }

    // Do a linear least squares fit of the difference between the
    // predicted and actual service times.  The slope (b) of that line
    // corresponds to the error in our estimate of alen and the y
    // intercept corresponds to the error in our estimate of off.
    b = find_slope(times, tracetimes, dist, &a, period);

    // Adjust the instance-to-instance skew according to the fit.    
    alen = fix_angle(alen - b / period);
    e->alen = dm_angle_dtoi(alen);
    
    // If e is the first instance in its parent index, its offset is
    // defined to be 0.  Otherwise, correct according to the fit.
    if(e->lbn > lbn) {
      printf("fix aoff (%d, %d)\n", lbn, e->lbn);
      aoff = fix_angle(aoff - a / period);
      e->off = dm_angle_dtoi(aoff);
    }

    printf("alen -> %f, off -> %f (dist %d)\n", alen, aoff, dist);

    if(dist == quot) {
      break;
    } else if(dist * 2 >= quot) {
      dist = quot;
    } else {
      dist *= 2;
    }
  }

  free(li);
  free(tracetimes);
  free(times);

  li = NULL;
  tracetimes = NULL;
  times = NULL;

  return;
}

// do_idx() is the main recursive function which measures the skews
// for all of the entries of a given index node.  Each entry has 2
// parameters to measure: the offset of the first instance of that
// entry from the beginning of the index containing it and the
// "length" of each instance, i.e. the offset of the i+1st instance
// relative to the ith.  For each index entry, it first measures the
// offset of the entry relative to the start of the index
// (do_idx_ent()) and then recurses into the structure of that entry.
int do_idx(struct dm_disk_if *d,    // diskmodel
	   struct idx_ent *ie,      // The entry for idx in its parent
	   struct idx *idx,         // The index node in question
	   struct dm_layout_g4 *l,  // g4 layout
	   struct dsstuff *ds,      // disksim instance
	   struct trace *t,         // io trace
	   int lbn) {               // First lbn of idx
  int i;
  struct idx_ent *e;

  fprintf(stderr, "%s(lbn %d)\n", __func__, lbn);

  for(i = 0, e = &idx->ents[0]; i < idx->ents_len; i++, e++) {
    // Measure the angular offset of this entry from the beginning of
    // the index node.
    do_idx_ent(d, l, &idx->ents[i], idx, ie, i, ds, t, lbn);

    // Recursively expand the structure of this entry.
    if(e->childtype == IDX) {
      do_idx(d, e, e->child.i, l, ds, t, lbn + e->lbn);
    }
  }

  for(i = 0, e = &idx->ents[0]; i < idx->ents_len; i++, e++) {
    printf("%s() off %f alen %f\n", __func__, 
	   dm_angle_itod(idx->ents[i].off),
	   dm_angle_itod(idx->ents[i].alen));
  }

  return 0;
}
	 

int main(int argc, char **argv) {
  int c;
  static struct option opts[] = {
    { "parv", 1, 0, 0 },    { "outv", 1, 0, 0 },
    { "model", 1, 0, 0 },
    { "mode", 1, 0, 0 },
    { "trace", 1, 0, 0 },
    {0,0,0,0}
  };

  enum optt { 
    PARV = 0,
    OUTV = 1,
    MODEL = 2,
    MODE = 3,
    TRACE = 4
  };

  struct dm_disk_if *d;
  struct dm_layout_g4 *l;

  struct trace *t;
  struct dsstuff *ds = calloc(1, sizeof(*ds));

  struct lp_block *unm;

  setlinebuf(stdout);
  FILE *outfile;
  int optind;

  char *parv = 0;
  char *outv = 0;
  char *model = 0;
  char *trace = 0;

  while((c = getopt_long(argc, argv, "q", opts, &optind)) != -1) {
    switch(c) {
    case -1:
      break;
    case 0:
      switch(optind) {
      case PARV:
	parv = strdup(optarg);
	break;
      case OUTV:
	outv = strdup(optarg);
	break;
      case MODEL:
	model = strdup(optarg);
	break;
      case MODE:
	if(!strcmp(optarg, "calib")) {
	  mode = CALIB;
	}
	else if(!strcmp(optarg, "gentrace")) {
	  mode = GENTRACE;
	}
	else {
	  fprintf(stderr, "*** bad mode %s\n", optarg);
	  exit(1);
	}
	break;

      case TRACE:
	trace = strdup(optarg);
	break;

      default:
	ddbg_assert(0);
	break;
      }
      break;
    default:
      ddbg_assert(0);
      break;
    }
  }

  t = setup_trace(trace);
  if(mode == CALIB) {
    outfile = fopen(model, "w");
    ddbg_assert(outfile);
  }

  ds->iface = disksim_interface_initialize(parv, 
					   outv,
					   cb,
					   schedule_callback,
					   deschedule_callback,
					   ds,
					   18,  // argc
					   nocache_over);

  d = disksim_getdiskmodel(ds->iface, 0);
  l = (struct dm_layout_g4 *)d->layout;

  do_idx(d, 0, l->root, l, ds, t, 0);

  extern struct lp_block* marshal_layout_g4(struct dm_layout_g4 *);

  if(mode == CALIB) {
    unm = marshal_layout_g4(l);
    unparse_block(unm, outfile);
    fclose(outfile);
  }

  free(ds);
  ds = NULL;

  fclose(t->fp);
  free(t);
  t = NULL;

  return 0;
}

/*
 * DIXtrac - Automated Disk Extraction Tool
 * Authors: Jiri Schindler, John Bucy, Greg Ganger
 *
 * Copyright (c) of Carnegie Mellon University, 1999-2005.
 *
 * Permission to reproduce, use, and prepare derivative works of
 * this software for internal use is granted provided the copyright
 * and "No Warranty" statements are included with all reproductions
 * and derivative works. This software may also be redistributed
 * without charge provided that the copyright and "No Warranty"
 * statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 */

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libparam/libparam.h>
#include <diskmodel/dm.h>
#include <diskmodel/modules/modules.h>
#include <libddbg/libddbg.h>

#include "mechanics.h"
#include "extract_layout.h"
#include "dixtrac.h"


extern struct dm_disk_if *adisk;

void 
seekout(void *ctx, int dist, float time, float stddev )
{
  fprintf((FILE *)ctx, "%d, %f, %f\n", dist, time, stddev);
}

// seekfd for the seeks
int
do_seeks(FILE *seekfd, int delay_usecs)
{
  float singlecyl, fullstrobe;
  int seeks_len;

  disable_caches();

  seeks_len = dx_seek_curve_len();
  fprintf(seekfd, "Seek distances measured: %d\n", seeks_len);
  get_seek_times(&singlecyl, &fullstrobe, 0, seekout, seekfd, delay_usecs);
  return 0;
}



void
do_degradation(FILE *outfile, int cyl)
{
  int i;
  float stmax, stmean, stvar;

  for(i = 100; i < 10000; i += 100) {
    find_seek_time(cyl, &stmax, &stmean, &stvar, i);
    fprintf(outfile, "%d %f %f\n", i, stmean, sqrt(stvar));
  }

  for(i = 10000; i < 100000; i += 1000) {
    find_seek_time(cyl, &stmax, &stmean, &stvar, i);
    fprintf(outfile, "%d %f %f\n", i, stmean, sqrt(stvar));
  }
}


void
test_mtbrc(int cyl, 
	   int step,
	   int points,
	   int max,
	   FILE *seekfd) 
{
  float stmax, stmean, stvar;
  int count = 0;

  int i;
  //  float seeks[10];

  if(step == -1) { step = 1; }

  if(max == -1) { max = adisk->dm_cyls - 1; }


  disable_caches();

  for(i = cyl; 
      (i < max) && (points <= 0 || count < points); 
      i += step, count++) 
  {
    double stddev;
    int dist = i;
    find_seek_time_mtbrc(dist, &stmax, &stmean, &stvar, 0);
    //    printf("*** dist %d time %f\n", i, stmean);
    //   seeks[i] = stmean;

    stddev = stvar;
    stddev = sqrt(stddev);
    
    // convert from usecs to msecs
    stmean /= 1000.0;
    stddev /= 1000.0;

    seekout(seekfd, dist, stmean, (float)stddev);
  }

/*   for(i = 0; i < 10; i++) { */
/*     printf("%d %f\n", (i+1) * 1000, seeks[i]); */
/*   } */

/*   printf("dist %d mean %f max %f stdev %f\n", */
/* 	 cyl, stmean, stmax, sqrt(stvar)); */
}


void
linear_backwards(FILE *seekfd) 
{
  int max;
  float stmax[2], stmean[2], stvar[2];
  float slope, si;
  int p[2];

  max = adisk->dm_cyls - 1;

  disable_caches();

  p[0] = max - 5000;
  p[1] = max / 2;

  find_seek_time_mtbrc(p[0], &stmax[0], &stmean[0], &stvar[0], 0);
  find_seek_time_mtbrc(p[1], &stmax[1], &stmean[1], &stvar[1], 0);

  slope = (stmean[0] - stmean[1]) / (p[0] - p[1]);

  printf("%d %f, %d %f, slope %f\n", p[0], stmean[0], p[1], stmean[1], slope);


  p[1] /= 2;

  do {
    find_seek_time_mtbrc(p[1], &stmax[1], &stmean[1], &stvar[1], 0);
    si = (stmean[0] - stmean[1]) / (p[0] - p[1]);
    printf("%d %f, slope %f\n", p[1], stmean[1], si);
    p[1] /= 2;
  } while(fabs(slope - si) < 0.01);


}



void 
usage(void)
{
  fprintf(stderr, "dx_seeks -d <disk dev> -l <layout model> -s <seekfile> "
	  "\t [--mode={degradation|normal|mtbrc}]\n"
	  "\t [--cyl=<n>]\n"
	  "\t [--delay=<d>]\n"
	  "\t [--points=<n>]\n"
	  "\t [--cylstep=<n>]\n"
	  "\t [--cylmax=<n>]\n");
  


  // --trials <n> -- n trials per point
}


int 
main(int argc, char **argv)
{
  static struct option opts[] = { 
    {"delay", 1, 0, 0 },
    {"mode", 1, 0, 0 },
    {"cyl", 1, 0, 0},
    {"cylstep", 1, 0, 0},
    {"cylmax", 1, 0, 0},
    {"points", 1, 0, 0},
    {0,0,0,0} };

  int opts_idx;
  int c;

  int cyl = -1;
  int step = -1;
  int points = -1;
  int max = -1;

  int delay_us = 1000;
  FILE *seekfile, *l;
  enum { NORM, DEGRAD, MTBRC } mode = NORM;

  char l_name[1024];
  char seekfile_name[1024];
  char dev[1024];

  struct lp_tlt **tlts;
  int tlts_len;

  seekfile_name[0] = 0;
  dev[0] = 0;
  l_name[0] = 0;
 

  setlinebuf(stdout);
  
  while((c = getopt_long(argc, argv, "d:l:s:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 0:
      if(!strcmp("delay", opts[opts_idx].name)) {
	delay_us = strtol(optarg, 0, 0);
      }
      else if(!strcmp("mode", opts[opts_idx].name)) {
	if(!strcmp(optarg, "degradation")) {
	  mode = DEGRAD;
	}
	else if(!strcmp(optarg, "mtbrc")) {
	  mode = MTBRC;
	}
	else {
	  mode = NORM;
	}
      }
      else if(!strcmp("cyl", opts[opts_idx].name)) {
	cyl = strtol(optarg, 0, 0);
      }
      else if(!strcmp("cylstep", opts[opts_idx].name)) {
	step = strtol(optarg, 0, 0);
      }
      else if(!strcmp("cylmax", opts[opts_idx].name)) {
	max = strtol(optarg, 0, 0);
      }
      else if(!strcmp("points", opts[opts_idx].name)) {
	points = strtol(optarg, 0, 0);
      }
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    case 's':
      strncpy(seekfile_name, optarg, sizeof(seekfile_name));
      break;
    case 'l':
      // layout model
      strncpy(l_name, optarg, sizeof(l_name));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }

  ddbg_assert_setfile(stderr);

  srand(time(0) % 71999);

  if(!*seekfile_name || !*dev || !*l_name) { 
    usage();
    exit(1);
  }

  if(mode == DEGRAD && cyl == -1) {
    fprintf(stderr, "*** need cylinder number for degradation mode\n");
    usage();
    exit(1);
  }

  if(mode == NORM) {
    fprintf(stderr, "*** using a delay of %d usecs\n", delay_us);
  }

  // set up SCSI stuff
  disk_init(dev, 1);

  // set up seek output
  if(!strcmp(seekfile_name, "-")) {
    seekfile = stdout;
  }
  else {
    seekfile = fopen(seekfile_name, "w");
    if(!seekfile) {
      perror("fopen seek file");
      exit(1);
    }
  }

  setlinebuf(seekfile);

  // load model
  l = fopen(l_name, "r");
  if(!l) {
    perror("fopen layout\n");
    exit(1);
  }

  for(c = 0; c <= DM_MAX_MODULE; c++) {
    lp_register_module(dm_mods[c]);
  }

  lp_init_typetbl();
  lp_loadfile(l, &tlts, &tlts_len, l_name, 0, 0);
  adisk = (struct dm_disk_if *)lp_instantiate("foo", 0);

  switch(mode) {
  case NORM:
    do_seeks(seekfile, delay_us);
    break;
  case DEGRAD:
    do_degradation(seekfile, cyl);
    break;

  case MTBRC:
    test_mtbrc(cyl, step, points, max, seekfile);
    //    linear_backwards(seekfile);
    break;

  default:
    break;
  }

  exit(0);
  return 0; // UNREACHED
}


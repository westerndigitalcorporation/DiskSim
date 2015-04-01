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
#include "build_scsi.h"
#include "mtbrc.h"

extern struct dm_disk_if *adisk;

void usage(void) {
  fprintf(stderr, "usage: dx_misc -d <dev>\n");
}

extern char *scsi_buf;

void
mtbrcfoo(void) {
  extern int ustop, ustart;
  double acc = 0.0;
  int i;
  int count = 10;

  for(i = 0; i < count; i++) {
    int hdel, ireqdist, irqd_time, mx;
    int val = mtbrc(B_WRITE, B_READ, MTBRC_SAME_TRACK, &hdel,&ireqdist,&irqd_time,&mx);
    
    acc += val;
    printf("%d\n", val);
  }

  acc /= count;
  printf("%f\n", acc);

}

int 
main(int argc, char **argv)
{
  static struct option opts[] = { 
    {0,0,0,0} };
  int opts_idx;
  int c;
  int cyl = -1;
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
  
  while((c = getopt_long(argc, argv, "d:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }

  ddbg_assert_setfile(stderr);

  srand(time(0) % 71999);

  if(!*dev) { 
    usage();
    exit(1);
  }

  // set up SCSI stuff
  disk_init(dev, 1);

  mtbrcfoo();

  //  dump_notches();

  return 0; // UNREACHED
}


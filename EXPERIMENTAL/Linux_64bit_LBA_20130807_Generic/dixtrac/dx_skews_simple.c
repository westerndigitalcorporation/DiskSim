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

#include "extract_params.h"
#include "extract_layout.h"


void skewout(void *ctx,
	     int zone,
	     float cylskew,
	     float trackskew)
{
  FILE *outfile = (FILE *)ctx;
  fprintf(outfile, "Zone #%d\n\tTrack Skew: %f\n\tCylinder Skew: %f\n",
	  zone, trackskew, cylskew);
}

void usage(void) {
  fprintf(stderr, "usage: ./dx_skews_simple -d dev -o outfile -m model\n");
}

// XXX
extern struct dm_disk_if *adisk;

int 
main(int argc, char **argv)
{
  int c;

  static struct option opts[] = { {0,0,0,0} };
  int opts_idx;

  char dev[1024];
  char m_name[1024];
  char outfile_name[1024];
  FILE *outfile, *model;

  struct lp_tlt **tlts;
  int tlts_len;

  dev[0] = 0;
  m_name[0] = 0;
  outfile_name[0] = 0;

  while((c = getopt_long(argc, argv, "d:o:m:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    case 'o':
      strncpy(outfile_name, optarg, sizeof(outfile_name));
      break;
    case 'm':
      // model file
      strncpy(m_name, optarg, sizeof(m_name));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }


  if(!*dev || !*outfile_name || !*m_name) {
    usage();
    exit(1);
  }

  srand(time(0) % 71999);


  ddbg_assert_setfile(stdout);

  disk_init(dev, 1);

  
  if(!strcmp(outfile_name, "-")) {
    outfile = stdout;
  }
  else {
    outfile = fopen(outfile_name, "w");
    if(!outfile) {
      perror("fopen() outfile");
      exit(1);
    }
  }

  setlinebuf(outfile);


  for(c = 0; c <= DM_MAX_MODULE; c++) {
    lp_register_module(dm_mods[c]);
  }

  lp_init_typetbl();

  model = fopen(m_name, "r");
  if(!model) {
    perror("fopen model\n");
    exit(1);
  }

  lp_loadfile(model, &tlts, &tlts_len, m_name, 0, 0);

  adisk = (struct dm_disk_if *)lp_instantiate("foo", 0);


  disable_caches();

  get_skew_information(skewout, outfile);


  exit(0);
  return 0;
}

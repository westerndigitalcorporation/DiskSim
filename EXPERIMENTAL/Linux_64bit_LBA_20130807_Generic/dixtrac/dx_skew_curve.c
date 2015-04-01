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

#include "extract_params.h"

extern struct dm_disk_if *adisk;


int measure_pt(int start, int lbn) {
  scsi_command_t c1, c2;
  int result;

  c1 = *(SCSI_rw_command(B_READ, start, 1));
  c2 = *(SCSI_rw_command(B_READ, lbn, 1));
  
  result = time_second_command(&c1,&c2);

  return result;
}

// Get points from lbn 0 to each track across the entire disk.
// Not so useful ... measure per zone and measure zone skew separately.
int
whole_disk(FILE *out)
{
  int lbn = 0;
  long time;
  int l1, l2;
  int cyl, head, sector;
  //  int maxlbn, blocksize;
  int samples = 10;
  int i;

  
  //  get_capacity(&maxlbn, &blocksize);

  while(lbn < adisk->dm_sectors) {
    time = 0;
    for(i = 0; i < samples; i++) {
      time = measure_pt(0, lbn);
      fprintf(out, "LBN %d : %ld\n", lbn, time);
    }

    diskinfo_physical_address(lbn, &cyl, &head, &sector);
    diskinfo_track_boundaries(cyl, head, &l1, &l2);

    lbn = l2 + 1;
  }

  return 0;
}


// Get points from the beginning of the zone to each track in the zone.
int
byzone(FILE *out, struct dm_disk_if *d, int zone, int noop)
{
  int lbn = 0;
  long time;
  int l1, l2;
  int cyl, head, sector;
  //  int maxlbn, blocksize;
  int samples = 10; 
  int i, j;
  int z0, zn;

  struct dm_layout_zone z;

  int zones_len = d->layout->dm_get_numzones(d); 

  //  get_capacity(&maxlbn, &blocksize);

  if(zone >= 0) {
    z0 = zn = zone;
  }
  else {
    z0 = 0;
    zn = zones_len-1;
  }
 
  for(j = z0; j <= zn; j++) {
    int spt;
    int lbn0;
    d->layout->dm_get_zone(d, j, &z);
    lbn0 = z.lbn_low;

    spt = d->layout->dm_get_sectors_lbn(d, lbn);
    lbn0 = lbn0 + spt - 1;

    while(lbn <= z.lbn_high) {
      time = 0;

      if(!noop) {
	for(i = 0; i < samples; i++) {
	  time = measure_pt(lbn0, lbn);
	  fprintf(out, "LBN %d : %f\n", lbn, (double)time);
	}
      }
      else {
	fprintf(out, "LBN %d : %f\n", lbn, 0.0);
      }

      diskinfo_physical_address(lbn, &cyl, &head, &sector);
      diskinfo_track_boundaries(cyl, head, &l1, &l2);
      
      ddbg_assert((l2+1) > lbn);
      lbn = l2 + 1;
    }
  }

  return 0;
}

// Points from lbn 0 to the first lbn of each zone.
// Not so useful, either.
int
perzone(FILE *out, struct dm_disk_if *d, int noop)
{
  int lbn = 0;

  int time;
  int samples = 50; 
  int i, j;

  struct dm_layout_zone z;

  int zones_len = d->layout->dm_get_numzones(d); 

  for(j = 0; j < zones_len; j++) {
    d->layout->dm_get_zone(d, j, &z);
    lbn = z.lbn_low;
    time = 0;

    if(!noop) {
      for(i = 0; i < samples; i++) {
	time = measure_pt(0, z.lbn_low);
	fprintf(out, "Zone %d (LBN %d) : %d\n", j, lbn, time);
      }
    }
    else {
      fprintf(out, "Zone %d (LBN %d) : %d\n", j, lbn, 0);
    }
  }

  return 0;
}

void oneshot(int start, int lbn) {
  int samples = 10;
  int time = 0;
  int i;
  for(i = 0; i < samples; i++) {
    time += measure_pt(start, lbn);
  }

  printf("(%d,%d) : %f\n", start, lbn, (double)time / samples);
}



void 
usage(void)
{
  fprintf(stderr, "dx_skew_curve "
	  "-d <disk dev> "
	  "-o <output file> "
	  "-l <layout model> "
	  "[-z <zone>]"
	  "[-l0 <lbn>]"
	  "[-ln <lbn>]"
	  "[-n]\n");
}

extern struct dm_disk_if *adisk;

int 
main(int argc, char **argv)
{
  static struct option opts[] = { 
    {"l0", 1, 0, 0},
    {"ln", 1, 0, 0},
    {0,0,0,0} };

  int opts_idx;
  int c;
  int zone = -1;
  int noop = 0;
  FILE *outfile, *l;

  int l0 = -1;
  int ln = -1;


  char l_name[1024];
  char outfile_name[1024];
  char dev[1024];

  struct lp_tlt **tlts;
  int tlts_len;

  outfile_name[0] = 0;
  dev[0] = 0;
  l_name[0] = 0;
 
  
  while((c = getopt_long(argc, argv, "d:o:l:z:n", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 0:
      if(opts_idx == 0) {
	l0 = atoi(optarg);
      }
      else if(opts_idx == 1) {
	ln = atoi(optarg);
      }
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    case 'o':
      strncpy(outfile_name, optarg, sizeof(outfile_name));
      break;
    case 'l':
      // layout model
      strncpy(l_name, optarg, sizeof(l_name));
      break;
    case 'n':
      noop = 1;
      break;
    case 'z':
      zone = atoi(optarg);
      break;

    default:
      usage();
      exit(1);
      break;
    }
  }

  ddbg_assert_setfile(stderr);

  srand(time(0) % 71999);

  if(!*outfile_name || (!noop && !*dev) || !*l_name) { 
    usage();
    exit(1);
  }
  
  if(!noop) {
    // set up SCSI stuff
    disk_init(dev, 1);
  }

  // set up output file
  if(!strcmp(outfile_name, "-")) {
    outfile = stdout;
  }
  else {
    outfile = fopen(outfile_name, "w");
    if(!outfile) {
      perror("fopen outfile");
      exit(1);
    }
  }

  setlinebuf(outfile);


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

  if(!noop) {
    disable_caches();
  }

  if(l0 > -1 && ln > -1) {
    oneshot(l0, ln);
  }
  else {
    byzone(outfile, adisk, zone, noop);
//     if(zone == -1) {
//       perzone(outfile, adisk, noop);
//     }
  }

  exit(0);
  return 0; // UNREACHED
}


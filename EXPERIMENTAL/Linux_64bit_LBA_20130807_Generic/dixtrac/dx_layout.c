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


// XXX move to config.h or something
#include <diskmodel/dm.h>

#include "extract_params.h"
#include "extract_layout.h"


// fp is the output file
// if lbnhigh == -1, extracts the entire disk
void
extract_ltop_map(FILE *fp,
		 int lbnlow,
		 int lbnhigh,
		 int defects_only)
{
  int i;
  int lbn, cyl, head, sect;
  int cyl2, head2, sect2;
  int seqcnt;
  int blocksize;
  int numcyls, rot, numsurfaces, dummy;

  char label[80];

  defect_t *defects;
  int defects_len;

  int prevseqcnt = 0;
  int transcnt = 0;
  
  get_disk_label (label);

  fprintf (fp, "Info extracted from a %s\n", label);

  get_capacity (&lbn, &blocksize);

  fprintf (fp, "maxlbn %d, blocksize %d\n", lbn, blocksize);

  get_geometry (&numcyls, &rot, &numsurfaces, &dummy);

  fprintf (fp, "%d cylinders, %d rot, %d heads\n", numcyls, rot, numsurfaces);


  if(lbnhigh == -1) {
    lbnhigh = lbn;
  }

  if(!defects_only) {

    for (lbn = lbnlow; lbn <= lbnhigh; lbn++)
    {
      get_physical_address (lbn, &cyl, &head, &sect);
      transcnt++;
      seqcnt = 0;
      // Begin optimization for skipping address translations 
      if (prevseqcnt > 0) {
	  get_physical_address((lbn + prevseqcnt), &cyl2, &head2, &sect2);
	  transcnt++;
	  if ((lbn + prevseqcnt) <= lbnhigh
	      && (cyl == cyl2)
	      && (head == head2)
	      && (sect2 == (sect + prevseqcnt))) {
	      /* printf("Translated LBN %d, skipped %d address translations\n",
		 (lbn + prevseqcnt), prevseqcnt); */
	      seqcnt = prevseqcnt - 1;
	  }
	  else {
	      /* printf("Translation speedup didn't work at head %d, cyl %d\n",
		 head, cyl); */
	      seqcnt = 0;
	  }
      }
      // End optimization

      get_physical_address ((lbn + 1 + seqcnt), &cyl2, &head2, &sect2);
      transcnt++;
      while (lbn+1+seqcnt <= lbnhigh
	     && (cyl == cyl2)
	     && (head == head2)
	     && (sect2 == (sect + seqcnt + 1)))
      {
	seqcnt++;
	get_physical_address ((lbn + 1 + seqcnt), &cyl2, &head2, &sect2);
	transcnt++;
      }
      prevseqcnt = seqcnt;
      
      fprintf (fp, "lbn %d --> cyl %d, head %d, sect %d, count %d\n",
	       lbn, cyl, head, sect, seqcnt+1);
      
      lbn += seqcnt;
    }
#ifndef SILENT
  printf("Executed %d address translations, for %d LBNs (%.2fx speedup)\n", 
	 transcnt, (lbnhigh- lbnlow), 
	 (double) (lbnhigh - lbnlow) / (double) transcnt);
#endif
  }

  read_defect_list(0, &defects, &defects_len, 0);
  for(i = 0; i < defects_len; i++) {
    fprintf(fp, "Defect at cyl %d, head %d, sect %d\n",
	    defects[i].cyl,
	    defects[i].head,
	    defects[i].sect);
  }
}


void 
usage(void)
{
  fprintf(stderr, "Usage: dx_layout [options] -d <disk dev> -o <output file>\n");
  fprintf(stderr, "   where options is one or more of:\n");
  fprintf(stderr, "       --defects-only      extraction of defects only\n");
  fprintf(stderr, "       --start-lbn <lbn>   start translation at LBN <lbn>\n");
  fprintf(stderr, "       --end-lbn <lbn>     end translation at LBN <lbn>\n");
}


int 
main(int argc, char **argv)
{
  int start_lbn = 0, end_lbn = -1;

  static struct option opts[] = { 
    {"start-lbn", 1, 0, 0},
    {"end-lbn", 1, 0, 0},
    {"defects-only", 0, 0, 0},
    {0,0,0,0} 
  };
  int opts_idx;
  int c;
  int defects_only = 0;
  FILE *outfile;

  char outfile_name[1024];
  char dev[1024];
  
  outfile_name[0] = 0;
  dev[0] = 0;
  
  while((c = getopt_long(argc, argv, "d:o:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 0:
      switch(opts_idx) {
      case 0:
	start_lbn = atoi(optarg);
	break;
      case 1:
	end_lbn = atoi(optarg);
	break;
      case 2:
	defects_only = 1;
	break;
      default:
	usage();
	exit(1);
	break;
      }
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    case 'o':
      strncpy(outfile_name, optarg, sizeof(outfile_name));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }

  if(!*outfile_name || !*dev) { 
    usage();
    exit(1);
  }

  disk_init(dev, 1);


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

  extract_ltop_map(outfile, start_lbn, end_lbn, defects_only);


  exit(0);
  return 0;
}


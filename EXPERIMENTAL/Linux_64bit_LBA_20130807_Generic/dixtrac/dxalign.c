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

#include "dixtrac.h"
#include "print_bscsi.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "scsi_conv.h"
#include "read_diskspec.h"
#include "mtbrc.h"
#include "mechanics.h"
#include "trace_gen.h"
#include "paramfile_gen.h"
#include "cache.h"
#include "handle_error.h"
#include "dxopts.h"

static int trace_source = SYNTH_DISKSIM_TRACE;

#define TRACESTATS_EXT      "tracestats"
#define ALIGNED_EXT         "aligned"

/* #include "disksim_global.h" */
#define WRITE		0x00000000
#define READ		0x00000001

#if HAVE_GETOPT_H
static struct option const longopts[] =
{
  DX_COMMON_OPTS_LIST
  {"produce-stats",    no_argument, NULL, 'p'},
  {"only-reads",       no_argument, NULL, 'n'},
  {"split-requests",   no_argument, NULL, 's'},
  {"dont-align-small", no_argument, NULL, 'd'},
  {"lltrace"         , no_argument, NULL, 't'},
  {"zone",             no_argument, NULL, 'z'},
  {0,0,0,0}
};
#endif

static char const * const option_help[] = {
"-p  --produce-stats     Write out stats to a file.",
"-n  --only-reads        Use only READS in trace.",
"-s  --split-requests    Split aligned requests to track-sized ones.",
"-d  --dont-align-small  Do not align I/Os that are smaller than track-size.",
"-z  --zone              Print out the 1st zone's last LBN.",
"-r  --raw-layout        Use raw layout.",                   
"-t  --lltrace           Use the (idle-time annotated) LLT trace.",
"-m  --model-name        String used as a base for created files.",          
"-h  --help              Output this help.",                                 

0
};

#define OPTIONS_STRING   "hu:g::rm:lfnzpsdt"

static char *program_name;
extern diskinfo_t *thedisk;

typedef struct {
  int reqs;        /* total number of requests in the trace */
  int misaligned;  /* number requests not aligned on track boundary */
  int orig_tcross; /* number of track-crossing reqs in the orginal trace */
  int align_tcross;/* number of track-crossing reqs in the aligned trace */
  int split_reqs;  /* number of split requests in track-aligned trace */
} stats_t;

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
usage(FILE *fp,char *program_name) 
{
   fprintf(fp,"Usage: %s [-pwzrfgsd] [-t] -m <model_name>\n",
	  program_name);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
print_help(FILE *fp,char *program_name) 
{
  char const * const *p;
  usage(fp,program_name);
  for (p = option_help;  *p;  p++)
    fprintf(fp,"  %s\n", *p);
  fprintf(fp,"<disk> is the corresponding SCSI raw device.\n");

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
main(int argc, char * argv[]) {

  char c;
  int use_files = 0;
  int reads_only = 0;
  int zone_only = 0;
  int split = 0;
  int align_smallio = 1;


  char line[201];
   
  int cyl,head,sector;
  int startlbn,endlbn,lbn;

  double dummy2;
  int    dummy3,thinktime;
     
  double time; 
  int devno,bcount,flags;

  FILE *tracefile = stdin;
  FILE *newfile = stdout;
  FILE *statfile; 

  stats_t statistics;
   /* 
    * start of common options 
    */
   DX_COMMON_VARIABLES;
   /* 
    * end of common options
    */

   program_name = argv[0];
   while (strpbrk(program_name++,"/")) ;
   program_name--;


  memset(&statistics,0,sizeof(statistics));

   while ((c = GETOPT(argc, argv,OPTIONS_STRING)) != EOF) {
    switch (c) {
     case 'p':
       use_files = 1;
        break;
     case 'n':
       reads_only = 1;
        break;
     case 'd':
       align_smallio = 0;
        break;
     case 's':
       split = 1;
        break;
     case 't':
       trace_source = LLT_TRACE;
       break;
     case 'z':
       zone_only = 1;
        break;
     /* 
      * start of common options 
      */
      DX_COMMON_OPTIONS;
     /* 
      * end of common options
      */
     }
   }
   extractlayout = 0;
   gen_complete_mappings = 0;
   run_tests = 0;
   use_scsi_device = 0;
   use_layout_file = 1;

   argc = 0;  argv[optind] = "hi";  /* make it believe that we provided device */

   /* 
    * start of common options check
    */
   DX_COMMON_OPTS_CHK;
   /* 
    * end of common options check
    */

   if ((zone_only) && (reads_only || use_files)) {
     error_handler("Cannot combine -z with other options!\n");
     exit(1);
   }

   dxobtain_layout(extractlayout,use_layout_file,use_raw_layout,
		   gen_complete_mappings,use_mapping_file,model_name,
		   start_lbn,mapping_file);

   if (zone_only) {
     lbn = 0;
     /* 
     disk_translate_lbn_to_pbn(thedisk,lbn,MAP_FULL,&cyl,&head,&sector);
     currband = diskinfo_get_band(cyl);
     printf("%d\n",disk_compute_blksinband(thedisk,currband));
     */
     printf("%d\n",diskinfo_blks_in_zone(lbn));
     exit(0);
   }
  /* open the trace file */
  if (use_files) {
    char tracext[30];
    if (trace_source == SYNTH_DISKSIM_TRACE) {
      strncpy(tracext,SYNTHTRACE_EXT,20);
    }    
    else { 
      strncpy(tracext,LLTRACE_EXT,20);
    }
    sprintf(line,"%s.%s",model_name,tracext);
    if (!(tracefile = fopen(line, "r"))) {
      fprintf(stderr,"Error opening '%s' for reading!\n",line);
      exit(1);
    }
    sprintf(line,"%s.%s.%s",model_name,tracext,ALIGNED_EXT);
    if (!(newfile = fopen(line, "w"))) {
      fprintf(stderr,"Error opening '%s' for writing!\n",line);
      exit(1);
    }
  }

  while (fgets(line, 200, tracefile)) {
    if (trace_source == SYNTH_DISKSIM_TRACE) {
      if (sscanf(line,
		 "%lf %d %d %d %x\n",&time,&devno,&lbn,&bcount,&flags) != 5) {
	fprintf(stderr, "Wrong number of arguments for I/O trace event\n");
	fprintf(stderr, "line: %s", line);
	exit(1);
      }
    }
    else {
      if (sscanf(line,"%lf %d %d %d %d %lf %u %d\n",&time,&devno,
		 &lbn,&bcount,&flags,&dummy2,&dummy3,&thinktime) != 8) {
	fprintf(stderr, "Wrong number of arguments for I/O trace event\n");
	fprintf(stderr, "line: %s", line);
	exit(1);
      }
    }
    if ((!reads_only) || (reads_only && (flags & READ))) {
      /* adjust the LBNs to be aligned on track boundary */
      /* 
      disk_translate_lbn_to_pbn(thedisk,lbn,MAP_FULL,&cyl,&head,&sector);
      currband = diskinfo_get_band(cyl);
      disk_get_lbn_boundaries_for_track(thedisk,currband,cyl,head,
					&startlbn,&endlbn);
      */
      diskinfo_physical_address(lbn,&cyl,&head,&sector);
      diskinfo_track_boundaries(cyl, head, &startlbn, &endlbn);

      /* adjust endlbn to point to the first LBN of the next track */
      endlbn++;

      /* collect some statistics */
      statistics.reqs++;
      
      if (lbn != startlbn) {
	statistics.misaligned++;
      }
      if (lbn+bcount > endlbn) {
	statistics.orig_tcross++;
      }

      if (startlbn+bcount > endlbn) {
	if (split) {
	  int first_split = 1;
	  int residual_bcount = bcount;
	  while (startlbn+bcount > endlbn) {
	    if (align_smallio) {
	      /* set the lbn to the begining of the track */
	      lbn = startlbn;
	    }
	    /* set bcount to include all sectors to the end of the track */
	    bcount = endlbn - lbn;
	    /* record how many blocks still need to be handled */
	    residual_bcount -=bcount;
	    /* write out the trace file */
	    if (trace_source == SYNTH_DISKSIM_TRACE) {
	      fprintf(newfile,"%13f %4d %10d %4d %x\n",time,devno,lbn,
		      bcount,flags);
	    }
	    else {
	      if (!first_split) {
		thinktime = 0;
	      }
	      fprintf(newfile,"%10.6f %2d %8d %5d %1d %10.6f %u %d\n",time,
		     devno,lbn,bcount,flags,dummy2,dummy3,thinktime);
	      first_split = 0;
	    }
	    /* count # of I/Os split */
	    statistics.split_reqs++;
	    /* advance to the next track which will create another I/O */
	    lbn = endlbn;
	    bcount = residual_bcount;
	    diskinfo_physical_address(lbn,&cyl,&head,&sector);
	    diskinfo_track_boundaries(cyl, head, &startlbn, &endlbn);
	    /* 
	    disk_translate_lbn_to_pbn(thedisk,lbn,MAP_FULL,&cyl,&head,&sector);
	    currband = diskinfo_get_band(cyl);
	    disk_get_lbn_boundaries_for_track(thedisk,currband,cyl,head,
					      &startlbn,&endlbn);
	    */
	  }
	  lbn = startlbn;
	}
	else {
	  statistics.align_tcross++;
	}
      }
      else {
	if (align_smallio) {
	  /* set the lbn to the begining of the track */
	  lbn = startlbn;
	}
      }
      /* write out the trace file */
      if (trace_source == SYNTH_DISKSIM_TRACE) {
	fprintf(newfile,"%13f %4d %10d %4d %x\n",time,devno,lbn,
		bcount,flags);
      }
      else {
	fprintf(newfile,"%10.6f %2d %8d %5d %1d %10.6f %u %d\n",time,
		devno,lbn,bcount,flags,dummy2,dummy3,thinktime);
      }
    }
  }

  if (fclose(newfile)) {
    fprintf(stderr,"Could not close the input data file!\n");
  }
  if (fclose(tracefile)) {
    fprintf(stderr,"Could not close the output data file!\n");
  }

  /* write the statistics */
  sprintf(line,"%s.%s",model_name,TRACESTATS_EXT);
  if (!(statfile = fopen(line, "w"))) {
    fprintf(stderr,"Error opening '%s'\n",line);
    exit(1);
    }
  fprintf(statfile,"Total number of requests:                   %d\n",
	  statistics.reqs);
  fprintf(statfile,"Number of misaligned requests:              %d\n",
	  statistics.misaligned);
  fprintf(statfile,"Number of track-crossing original requests: %d\n",
	  statistics.orig_tcross);
  fprintf(statfile,"Number of track-crossing aligned requests:  %d\n",
	  statistics.align_tcross);
  if (split) {
    fprintf(statfile,"Number of split track-crossing requests:    %d\n",
	    statistics.split_reqs);
    fprintf(statfile,"Total # of requests in track-aligned trace: %d\n",
	    statistics.split_reqs+ statistics.reqs);
  }

 if (fclose(statfile)) {
    fprintf(stderr,"Could not close the trace stats file!\n");
  }

  exit(0);
}

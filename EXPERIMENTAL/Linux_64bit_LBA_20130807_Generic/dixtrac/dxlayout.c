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

extern diskinfo_t *thedisk;
diskinfo_t *rawdisk;

extern char *scsi_buf;

#ifdef ADDR_TRANS_COUNTER
extern int transaddr_cntr;
#endif

#if HAVE_GETOPT_H
static struct option const longopts[] =
{

  DX_COMMON_OPTS_LIST

  {"translate-address", no_argument, NULL, 't'},
  {"track-boundries",    no_argument, NULL, 'e'},
  {"defects",           no_argument, NULL, 'j'},
  {"pad-defects",       no_argument, NULL, 'd'},
  {0,0,0,0}
};
#endif

static char const * const option_help[] = {
 "-t  --translate-address Measures time of one address translation.",
 "-e  --track-boundries   Determine track boundries",
 "-j  --defects           Print the disk's defect list.",
 "-d  --pad-defects       Pads the defect list and prints it out to stdout.",

 DX_COMMON_OPTS_HELP

0
};

/*  #define OPTIONS_STRING   "ilfrau:vbsn:dp:tsrhc:g::rm:xw:ej" */
#define OPTIONS_STRING   "ahtu:g::rm:lfejd"

static void
pad_defect_list(defect_t * dlist)
{
  int count = dlist[0].cyl;
  int i,j,s;
  int lbn,head,cyl,sector;

  for(i=1 ; i < count ; i++) {
    s = dlist[i].sect;
    if (s < 0) {
      printf("Pad entire track\n");
    }
    else {
      do { diskinfo_lbn_address(dlist[i].cyl,dlist[i].head,--s,&lbn);
      } while((lbn < 0) && (s >= 0));
    }
    diskinfo_physical_address(lbn+1,&cyl,&head,&sector);

    if ((dlist[i].cyl == cyl) && (dlist[i].head == head)) {
      int size = sizeof(defect_t)*(count-i);
      int shift = sector - dlist[i].sect-1;
      if (shift) {
	if (dlist[i+shift].sect != (dlist[i].sect+1)) {
	  printf("Inserted a new defect\n");
	  memmove(&dlist[i+shift+1],&dlist[i+1],size);
	  for(j=1 ; j <= shift ; j++) {
	    dlist[i+j].cyl = cyl;
	    dlist[i+j].head = head;
	    dlist[i+j].sect = sector+j;
	  }
	}
	else {
	  printf("Two consecutive defects, but both are on the defect list\n");
	}
      }
    }
    else {
      printf("next LBN on the next track (LBN %d [cyl %d,head %d])\n",lbn,cyl,head);
    }
  }

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
usage(FILE *fp,char *program_name) 
{
   fprintf(fp,"Usage: %s [-hdtalgfr] [-u <file>] -m <model_name> <disk>\n",
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

/****************************************************************************
 *
 *
 ****************************************************************************/
int 
main (int argc, char *argv[]) 
{
   char c;
   char *program_name;
   int track_boundries = 0;
   int print_defects = 0;
   int pad_defects = 0;
   int transaddr_mode = 0;
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

   while ((c = GETOPT(argc, argv,OPTIONS_STRING)) != EOF) {
     switch (c) {
     case 'e':
       track_boundries = 1;
       break;
     case 'j':
       print_defects = 1;
       break;
     case 'd':
       pad_defects = 1;
       break;
     case 't':
       transaddr_mode = 1;
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
   /* 
    * start of common options check
    */
   DX_COMMON_OPTS_CHK;
   /* 
    * end of common options check
    */

   disk_init(argv[optind],use_scsi_device);

   if (!(thedisk = read_layout_file(model_name))) {
     error_handler("Error reading layout file %s.%s!\n",
		   model_name,LAYOUT_FULL_EXT);
   }
   {
     char rlfilename[255];
     sprintf(rlfilename,"%s.%s",model_name,LAYOUT_MAP_EXT);
     rawdisk = get_diskinfo(1);
     read_raw_layout(rawdisk,rlfilename);
   }
   { 
     int lbn = 0;
     int lbn2;
     int rllbn1,rllbn2;
     int cyl,head,sector;
     int rlcyl,rlhead,rlsector;

     rllbn1 = 0;
     while (lbn < thedisk->numblocks) {
       diskinfo_physical_address(lbn,&cyl,&head,&sector);
       diskinfo_track_boundaries(cyl, head, &lbn, &lbn2);

       printf("G1  LBN = %d [%d,%d,%d]\t%d\n",lbn,cyl,head,sector,lbn2);
       lbn = lbn2;

       diskinfo_physical_address(rllbn1,&rlcyl,&rlhead,&rlsector);
       diskinfo_track_boundaries(rlcyl, rlhead, &rllbn1, &rllbn2);
       printf("RAW LBN = %d [%d,%d,%d]\t%d\n",rllbn1,rlcyl,rlhead,rlsector,rllbn2);
       if ((head != rlhead) || (cyl != rlcyl)) {
	 printf("discrepancy: %d,%d\t%d,%d\n",head,cyl,rlhead,rlhead);
       }
       rllbn1 = rllbn2;
       printf("LBN: %d\t%d\n",lbn,rllbn1);
     }
   }
   disk_shutdown(use_scsi_device);
   exit (0);

   /* 
    * start of common options handling
    */
   DX_COMMON_OPTS_HANDLE;
   /* 
    * end of common options handling
    */

   dxobtain_layout(extractlayout,use_layout_file,use_raw_layout,
		   gen_complete_mappings,use_mapping_file,model_name,
		   start_lbn,mapping_file);

   if (pad_defects) {
     if (!use_raw_layout) {
       char rlfilename[255];
       sprintf(rlfilename,"%s.%s",model_name,LAYOUT_MAP_EXT);
       thedisk = get_diskinfo(0);
       read_raw_layout(thedisk,rlfilename);
     }
     pad_defect_list(thedisk->defect_lists);
   }

   if (print_defects) {
     defect_t *defect_lists;
     FILE *fp;
     char filename[80];
     
     sprintf(filename, "%s.defect_list", model_name);
     fp = fopen(filename, "w");
     if (fp == NULL) {
       error_handler("writing out defect list - could not open %s for writing\n", filename);
     }
     if (!thedisk->defect_lists) {
       printf("No defect list obtained, reading defect list from the disk.\n");
       defect_lists = read_defect_list(NULL);
     }
     writeout_defect_list(defect_lists, fp);
   }

   if (run_tests) {
#ifdef MEASURE_TIME
     elapsed_time();
#endif
     disable_caches();
     if (use_raw_layout) {
       get_skew_information();
     }

#ifdef MEASURE_TIME
     elapsed_time();
#endif
   }
   if (transaddr_mode) {
     trans_addr_delay();
#ifdef MEASURE_TIME
     printf("Translate Address delay: %.3f seconds\n",elapsed_time());
#endif
   }
   if(track_boundries){
#ifdef MEASURE_TIME
     elapsed_time();
#endif
     find_track_boundries();
#ifdef MEASURE_TIME
     printf("Track boundaries detection run time: %.3f seconds\n",
	    elapsed_time());
#endif
   }
   exit (0);
}


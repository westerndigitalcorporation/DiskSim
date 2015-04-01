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

#ifndef __EXTRACT_LAYOUT_H
#define __EXTRACT_LAYOUT_H


// XXX not here
#include <diskmodel/dm.h>

#include <stdio.h>
#include <sys/types.h>
#include "disksim_diskmap.h"

#define MAXBANDS	64


/* LBN to PBN mapping types (also for getting cylinder mappings) */

#define MAP_NONE                0
#define MAP_IGNORESPARING       1
#define MAP_ZONEONLY            2
#define MAP_ADDSLIPS            3
#define MAP_FULL                4
#define MAP_FROMTRACE           5
#define MAP_AVGCYLMAP           6
#define MAXMAPTYPE              6

#ifdef RAWLAYOUT_SUPPORT
/* type definitions for raw layout support */
#define MAXSURFACES    25

#ifdef DETAILED_RAW_LAYOUT
#define MAXSEQ         20
#endif

typedef struct surface {
  int firstlbn,lastlbn;
#ifdef DETAILED_RAW_LAYOUT
  int sect[MAXSEQ], seqcnt[MAXSEQ];
#endif
  int valid;
} surface_t;

typedef struct cyl_rawlayout {
  int firstlbn,lastlbn;
  int validcyl;
  surface_t surface[MAXSURFACES];
} cyl_rawlayout_t;

typedef struct lbn_rawlayout  {
  int firstlbn,lastlbn;
  int cyl;
} lbn_rawlayout_t;
/* end of type definitions for raw layout support */
#endif

typedef struct defect {
   int cyl;
   int head;
   int sect;
} defect_t;

typedef struct transcacheentry {
   int cyl;
   int sect;
   int head;
   int lbn;
   struct transcacheentry *next;
} transcacheentry_t;

struct query_results {
   char drivename[80];
   int maxlbn;
   int blocksize;
   int numcyls;
   int rot;
   int numsurfaces;
   defect_t *defect_lists;
   int rangecnt;
#define MAX_RANGES 256*1024
   struct range {
      int lbn;
      int lastlbn;
      int cyl;
      int head;
      int sect;
   } ranges[MAX_RANGES];
};

typedef struct band {
   int    startcyl;
   int    endcyl;
   int    blkspertrack;
   int    deadspace;
   double firstblkno;
   double cylskew;
   double trackskew;
   int    blksinband;
   int    sparecnt;
   int    numslips;
   int    numdefects;
   int    slip[MAXSLIPS];
   int    defect[MAXDEFECTS];
   int    remap[MAXDEFECTS];
} band_t;


typedef struct disk {
   uint   devno;
   struct disk *next;
   int   numsurfaces;
   int   numblocks;
   int   numcyls;
   int   numbands;
   int   sectsize;
   int   sparescheme;
   int  rangesize;
   int   mapping;
   band_t bands[MAXBANDS];
   int   sectpercyl;
   /*  void * defect_lists; */
   defect_t* defect_lists;
   struct query_results *query_results;
#define TRANSCACHESIZE 1024
   struct transcacheentry *translation_cache[TRANSCACHESIZE];
   double rotatetime;
  
  /* data structures for support of raw layout format */
#ifdef RAWLAYOUT_SUPPORT
   int numlbnranges;
   cyl_rawlayout_t *rlcyls;
   lbn_rawlayout_t *lbnrange;
#endif
} diskinfo_t;

diskinfo_t * extract_layout (uint devno);

void write_layout_file (diskinfo_t *diskinfo, FILE *fp, int sansdefects);

diskinfo_t * generate_layout_info (uint devno, char *modelname);

void 
writeout_all_lbntopbn_mappings (uint devno, 
				uint maxlbn, 
				char *modelname, 
				int start_lbn);

void readin_all_lbntopbn_mappings (uint devno, char *mapping_file);

int count_defects_on_track (struct dm_disk_if *, uint cyl, uint head);

int count_defects_on_cylinder (struct dm_disk_if *, uint cyl);

int 
find_defectfree_cylinder_range(struct dm_disk_if *,
			       int firsttry, 
			       int count);

defect_t *
read_defect_list (diskinfo_t *diskinfo,
		  defect_t **defects,
		  int *defects_len,
		  int *format);

void writeout_defect_list (defect_t *defect_lists, FILE *fp);

diskinfo_t * get_diskinfo (uint devno);

/* disksim_diskmap.c functions */



extern double disk_map_pbn_skew();
extern void disk_get_lbn_boundaries_for_track();
extern void disk_check_numblocks();

#ifdef RAWLAYOUT_SUPPORT
void read_raw_layout(diskinfo_t *currdisk,char *rlfilename);
#endif

#endif /* __EXTRACT_LAYOUT_H */


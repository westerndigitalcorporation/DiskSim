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

#include <diskmodel/dm.h>
// XXX
extern struct dm_disk_if *adisk;

#include "dixtrac.h"
#include "build_scsi.h"
#include "print_bscsi.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "mechanics.h"
#include "mtbrc.h"
#include "handle_error.h"
#include "cache.h"

#define CYL_OFFSET 2

// #ifndef SILENT
#undef VERBOSE_MTBRC 
// #endif

/* print some more debugging info */
// #define DEBUG_MTBRC 1

// extern diskinfo_t *thedisk;

extern char *scsi_buf;

static int compl_overhead_dist[CYL_DISTANCES] = { CYL_DIST_1, CYL_DIST_2,
						  CYL_DIST_3, CYL_DIST_4};
static float seek_times[CYL_DISTANCES];


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
find_seeks_for_distances(void) {
    static int retval = -1;
    int d,distance;
    float dummy;
    if (retval == -1) {
      // printf("Running MTBRC seek times extraction...\n");
	for(d=0;d<CYL_DISTANCES;d++) {
	    distance = compl_overhead_dist[d];
	    find_seek_time(distance,&dummy,&seek_times[d], 0, 0);
	    /* printf("Seek distance %d cyls: %f\n",distance,seek_times[d]); */
	}
    retval = 0;
    }
}

/*---------------------------------------------------------------------------*
 * If cyldist is non-zero, then startcyl must be an LBN-mapped cylinder as   *
 * well as startcyl+cyldist is also a cylinder with some mapped LBNs.        *
 *---------------------------------------------------------------------------*/
static unsigned int
internal_mtbrc(int mtbrc_type,
	       int rw1,
	       int rw2,
	       int req_type,
	       int startcyl,
	       int cyldist,
	       float *hostdelay,
	       float *ireqdist,
	       float *ireqdist_time, 
	       float *med_xfer)
{
  // XXX
  double rotatetime = 0.0;
/* how many locations for determining partial MTBRCs */
#define MAX_LOCATIONS    CYL_DISTANCES

  int i,tmp_ird = -1,sectors = 0;

  //  long mtbrc_vals[MAX_LOCATIONS];
  //  long host_delays[MAX_LOCATIONS];
  //  long ireq_dist[MAX_LOCATIONS];
  //  long ireq_dist_time[MAX_LOCATIONS];

  long *mtbrc_vals = calloc(MAX_LOCATIONS, sizeof(*mtbrc_vals));
  long *host_delays = calloc(MAX_LOCATIONS, sizeof(*host_delays));
  long *ireq_dist = calloc(MAX_LOCATIONS, sizeof(*ireq_dist));
  long *ireq_dist_time = calloc(MAX_LOCATIONS, sizeof(*ireq_dist_time));

  // find_mean_max() will not count the 0 entries
  //  memset(mtbrc_vals, 0, sizeof(mtbrc_vals));
  //  memset(host_delays, 0, sizeof(host_delays));
  //  memset(ireq_dist, 0, sizeof(ireq_dist));
  //  memset(ireq_dist_time, 0, sizeof(ireq_dist_time));

  float max_mtbrc, mean_mtbrc;
  float max_hdel, mean_hdel;
  float max_irdist, mean_irdist;
  float max_irdist_time, mean_irdist_time;
  int d, trials, locations = MAX_LOCATIONS;

  u_int8_t dra_origval = 0;

  if (req_type == MTBRC_READ_MISS) {
    dra_origval = get_cache_bit(CACHE_DRA);
    set_cache_bit(CACHE_DRA,1);
  }
  switch(mtbrc_type) {
  case COMPL_OVERHEAD_MTBRC:
    find_seeks_for_distances();
    locations = CYL_DISTANCES;
    break;
  case SEEKDIST_MTBRC:
  case REGULAR_MTBRC:
    locations = 1;
    break;
  default:
    internal_error("internal_mtbrc: wrong mtbrc_type!\n");
  }
#ifdef VERBOSE_MTBRC
   setbuf(stdout,NULL);
   printf("Finding MTBRC ");
#endif

  for(i = 0; i < locations; i += CYL_DISTANCES) {
    tmp_ird = -1;
    switch(mtbrc_type) {

    case REGULAR_MTBRC:
      /* leave space for the next cylinder */
      startcyl = random_cylinder(40,ANY_DEFECT,ANY_BAND);
      mtbrc_vals[i] = find_one_mtbrc(startcyl,req_type,rw1,rw2,TRIALS,
				     0,&host_delays[i], &tmp_ird);
      break;

    case SEEKDIST_MTBRC:
      mtbrc_vals[i] = find_one_mtbrc_bins(startcyl,
					  MTBRC_CYL_DIST,
					  rw1,rw2,
					  TRIALS,
					  cyldist,
					  &host_delays[i],
					  &tmp_ird);
      if(mtbrc_vals[i] == -1) {

  free(mtbrc_vals);
  free(host_delays);
  free(ireq_dist);
  free(ireq_dist_time);


	return -1;
      }
      
      break;

    case COMPL_OVERHEAD_MTBRC:
      do {
	/* leave space for the next cylinder */
	startcyl = random_cylinder(40,ANY_DEFECT,ANY_BAND);
	for(d=0;d<CYL_DISTANCES;d++) {
	  cyldist = compl_overhead_dist[d];
	  if (diskinfo_cyl_firstlbn(startcyl+cyldist,0) < 0) {
	    printf("Broke out of the loop!\n");
	    break;
	    /* break only if the cylinder is not mapped -> find new startcyl */
	    /* then d < CYL_DISTANCES */
	  }
	}
      } while ((diskinfo_cyl_firstlbn(startcyl,0) < 0) || (d < CYL_DISTANCES));

      startcyl = random_cylinder(40,ANY_DEFECT,ANY_BAND);
      /* completion overheads */
      trials = TRIALS / CYL_DISTANCES;
      for(d=0;d<CYL_DISTANCES;d++) {
	mtbrc_vals[i+d]=
	  find_one_mtbrc(startcyl,MTBRC_CYL_DIST,rw1,rw2,trials,
			 compl_overhead_dist[d],&host_delays[i+d],&tmp_ird)
	  - seek_times[d];
      }
      break;
    }

    assert(tmp_ird != -1);

    ireq_dist[i] = (long) tmp_ird;
    /* printf("Inter-request distance = %ld\n",ireq_dist[i]); */

    sectors = diskinfo_sect_per_track(startcyl);
    ireq_dist_time[i] = (long) ((float) ireq_dist[i] ) * 
      (1000 * rotatetime / ((float) sectors));

//      printf("Cylinder: %d,sectors: %d\n",startcyl,sectors);
//      printf("rev: %ld\tTime (irdist) = %ld \n",
//  	   ((long) (1000 * rotatetime)),
//  	   (long) (tmp_ird * (1000 * rotatetime / sectors)));

  }

  if (req_type == MTBRC_READ_MISS)
    set_cache_bit(CACHE_DRA,dra_origval);

  find_mean_max(locations, 1, (long*)&host_delays[0], &max_hdel, &mean_hdel, 0);
  find_mean_max(locations, 1, (long*)&ireq_dist[0], &max_irdist, &mean_irdist, 0);
  find_mean_max(locations, 1, (long*)&mtbrc_vals[0], &max_mtbrc, &mean_mtbrc, 0);
  find_mean_max(locations, 1, (long*)&ireq_dist_time[0], &max_irdist_time, &mean_irdist_time, 0); 

  *med_xfer = ONE_REVOLUTION / (float) sectors;
  *ireqdist_time = mean_irdist_time;
  *hostdelay = mean_hdel;
  *ireqdist = mean_irdist;

#ifdef VERBOSE_MTBRC
  if (mtbrc_type == REGULAR_MTBRC) {
    printf("MTBRC");
  }
  else {
    printf("Compl Overhead");
  }

  printf("(1-sector %s, 1-sector %s %s) = %.2f usec\n",
	 ((rw1 == B_WRITE) ? "write" : "read"),
	 ((rw2 == B_WRITE) ? "write" : "read"),
	 ((req_type == MTBRC_SAME_TRACK) ? "same track" : 
	  ((req_type == MTBRC_NEXT_TRACK) ? "next track" : 
	   ((req_type == MTBRC_READ_MISS) ? "miss same track" : 
	    ((req_type == MTBRC_READ_HIT) ? "hit" : 
	     ((req_type == MTBRC_WRITE_HIT) ? "hit" : "another cylinder"))))),
	 mean_mtbrc);

  printf("\tFinal host delay:   \t %6.2f usec\n",mean_hdel);

  /* printf("\tFinal ireq distance:\t %6.2f (sectors)\n",mean_irdist); */
  /* printf("\tFinal ireq distance:\t %6.2f usec (time)\n",mean_irdist_time);*/
  printf("\tFinal media xfer:   \t %6.2f usec (time)\n",*med_xfer);
#endif


  free(mtbrc_vals);
  free(host_delays);
  free(ireq_dist);
  free(ireq_dist_time);


  return ((uint) mean_mtbrc);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
seek_mtbrc(int rw1,
	   int rw2,
	   int startcyl,
	   int cyldist,
	   float *hostdelay,
	   float *ireqdist,
	   float *ireqdist_time, 
	   float *med_xfer)
{
  return internal_mtbrc(SEEKDIST_MTBRC, //REGULAR_MTBRC,         // mtbrc_type
			rw1,
			rw2,
			SEEKDIST_MTBRC,        // req type
			startcyl,
			cyldist,
			hostdelay,
			ireqdist,
			ireqdist_time,
			med_xfer);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
unsigned int
mtbrc(int rw1,int rw2,int req_type,float *hostdelay,
      float *ireqdist,float *ireqdist_time, float *med_xfer)
{
  return(internal_mtbrc(REGULAR_MTBRC,rw1,rw2,req_type,0,0,hostdelay,
	       ireqdist,ireqdist_time,med_xfer));
}

/*---------------------------------------------------------------------------*
 * Returns completion overhead.                                              * 
 *---------------------------------------------------------------------------*/
unsigned int
compl_overhead(int rw1,int rw2,int req_type,float *hostdelay, float *med_xfer)
{
  float ireqdist,ireqdist_time;
  return(internal_mtbrc(COMPL_OVERHEAD_MTBRC,rw1,rw2,req_type,0,0,hostdelay,
	       &ireqdist,&ireqdist_time,med_xfer));
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
uint 
mtbrc_run_commands(int req_type,
		   int rw1,
		   int rw2,
		   int lbn1,
		   int lbn2,
		   int *hdel,
		   long *cmd2_time)
{
/* #define TEST_DELAY */
  int dummy,cyl;
  //  long con;

  //  long start_sec[2], start[2],
  //    end_sec[2], end[2], 
  //    command[2];

  extern long ustop, ustart, ustart_sec, ustop_sec;

  int discard_read = cd_discard_req_after_read();

  double start[2] = {0.0, 0.0}, stop[2] = {0.0, 0.0};

  /* printf("Discard read : %d\n",discard_read); */
  /* read the data to the cache */
  if ((0 /* (req_type == MTBRC_WRITE_HIT) */
       || (req_type == MTBRC_READ_HIT))
      // && (!cd_share_rw_segment()) 
      && cd_discard_req_after_read()) 
    {

    exec_scsi_command(scsi_buf,SCSI_rw_command(rw2,lbn2,1));
    /* printf("Data read to the cache...\n"); */
    if (discard_read) {
      lbn2 += 1;
      busy_wait_loop(ONE_REVOLUTION);
    }
  }

  exec_scsi_command(scsi_buf,SCSI_rw_command(rw1,lbn1,1));

//   printf("%s (%d,%d) (%d,%d)\n", __PRETTY_FUNCTION__,
// 	 ustart_sec, ustart,
// 	 ustop_sec, ustop);

  start[0] = ustart + (double)ustart_sec * 1000000.0; // (double)(1 << 20);
  stop[0]  = ustop  + (double)ustop_sec  * 1000000.0; // (double)(1 << 20);

//   end[0] = ustop;
//   start[0] = ustart;
//   start_sec[0] = ustart_sec;
//   command[0] = ustop - ustart;

  if (req_type == MTBRC_WRITE_HIT)
    busy_wait_loop(2*ONE_REVOLUTION);

#ifdef TEST_DELAY
  busy_wait_loop(10000);
#endif 

  exec_scsi_command(scsi_buf,SCSI_rw_command(rw2,lbn2,1));

//   printf("%s() (%d,%d) (%d,%d)\n", __PRETTY_FUNCTION__,
// 	 ustart_sec, ustart,
// 	 ustop_sec, ustop);


  start[1] = ustart + (double)ustart_sec * 1000000.0; //(double)(1 << 20);
  stop[1]  = ustop  + (double)ustop_sec  * 1000000.0; //(double)(1 << 20);

//   command[1] = ustop - ustart;
//   end[1] = ustop;
//   start[1] = ustart;
//   start_sec[1] = ustart_sec;

#ifdef DEBUG_MTBRC
  printf("0: %08f %08f %08f\n", start[0], stop[0], stop[0] - start[0]);
  printf("1: %08f %08f %08f\n", start[1], stop[1], stop[1] - start[1]);
#endif

  diskinfo_physical_address(lbn2,&cyl,&dummy,&dummy);

  //  con =  ONE_REVOLUTION * 8 / diskinfo_sect_per_track(cyl);
  // printf("Con: %ld\n",con); 

  *cmd2_time = (int)(stop[1] - start[1]);
  //  *hdel = ((start_sec[1] - start_sec[0]) * 1000000) + start[1] - stop[0];
  *hdel = start[1] - stop[0];

  /* printf("Time %ld\n", */

  //  return ((start_sec[1] - start_sec[0]) * 1000000) + stop[1] - stop[0];

  {
    unsigned int result = (stop[1] - stop[0]);

    if(result > (5 * 6000)) {
      fprintf(stderr, "*** %s -- result way too big %d (%f,%f)\n",
	      __PRETTY_FUNCTION__, result, stop[1], stop[0]);
      return 0;
    }
    else {
      return result;
    }
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
void
mtbrc_lbn_pair(int startcyl,
	       int sect_offset,
	       int req_type,
	       int cyldist,

	       int *lbn1,
	       int *lbn2) 
{  
  int head = 0;
  int sector = 0;
  int cyl2,head2,sector2;
  int retry;
  int lbns,lbne;

  do {
    retry = 0;
    /* get a LBN at the "begining" of the cylinder */
    *lbn1 = diskinfo_cyl_firstlbn(startcyl,MAX_SECT_OFFSET);
    diskinfo_physical_address(*lbn1,&cyl2,&head,&sector);

    switch (req_type) {
    case MTBRC_WRITE_HIT:
      sect_offset+=2;

    case MTBRC_READ_MISS:
    case MTBRC_READ_HIT:
    case MTBRC_SAME_TRACK:
      /* find appropriate lbn2 */
      if (sect_offset >= 0) 
	*lbn2 = *lbn1 + sect_offset; 
      else {
	internal_error("mtbrc_lbn_pair: sect_offset is negative!\n");
      }

      diskinfo_physical_address(*lbn2,&cyl2,&head2,&sector2);

      /* make sure we didn't choose remapped LBNs */
      if ((cyl2 != startcyl) || (head2 != head) || 
	  (sector2 != (sector+sect_offset))) 
	retry = 1;
      break;

    case MTBRC_NEXT_TRACK:
      /* get the head address of a sector which is on the next track */
      /* i.e. take LBN1 and add to it the number of sectors/track */
      diskinfo_physical_address(*lbn1+diskinfo_sect_per_track(startcyl),
				&cyl2,&head,&sector2);

      diskinfo_track_boundaries(startcyl, head, &lbns, &lbne);


      // disk_get_lbn_boundaries_for_track(thedisk,diskinfo_get_band(startcyl),
      //			startcyl,head,&lbns,&lbne);


      *lbn2 = ((sect_offset<0)? lbne+1 : lbns) + sect_offset;
      diskinfo_physical_address(*lbn2,&cyl2,&head2,&sector2);
      /* make sure we didn't choose remapped LBNs */
      if ((cyl2 != startcyl) || (head2 != (head)) || (*lbn2 < 0)) {
	retry = 1;
      }
      break;

//     case  (MTBRC_READ_HIT+10) :  
//       do {
// 	*lbn2 = rand() % (thedisk->numblocks);
// 	diskinfo_physical_address(*lbn2,&cyl2,&head2,&sector2);
//       } while (cyl2 == startcyl);
//       break;

    case MTBRC_NEXT_CYL:
    case MTBRC_CYL_DIST:
      if (req_type == MTBRC_NEXT_CYL) {
	cyl2 = startcyl + CYL_OFFSET;
      }
      else {
	cyl2 = startcyl + cyldist;
      }

      sector2 = diskinfo_sect_per_track(cyl2);

      // cyl2 may be unmapped ... give up and try another pair?
      // XXX semantics???
      if(sector2 == -1) {
	*lbn2 = -1;
	return;
      }

      /* printf("Sector offset: %d\n",sect_offset); */

      if (sect_offset > sector2) {
	sector = abs(sector2 - sect_offset);
      }
      else {
	if (sect_offset < 0) {
	  sector = sector2 + sect_offset;
	}
	else {
	  sector += sect_offset;
	}
      }

      assert(sector <= sector2);

      diskinfo_lbn_address(cyl2,head,sector,lbn2);

      if (*lbn2 <0) {
	// error!
	return;
      }
      break;

    default:
      internal_error("mtbrc_lbn_pair: MTBRC type not specified!\n");
    }

  } while (retry);

  //  printf("%s() cyl %d -> %d cyl %d -> %d\n", __func__, 
  //	 startcyl, startcyl + cyldist, *lbn1, *lbn2);

}

/*---------------------------------------------------------------------------*
 * Determines the MTBRC for a given startcyl and requests rw1 and rw2.       *
 *---------------------------------------------------------------------------*/
long 
find_one_mtbrc(int startcyl,
	       int req_type,
	       int rw1,
	       int rw2,
	       int trials,
	       int cyldist,
	       long* host_delay,
	       int* ireq_dist)
{
   long cmd2_time;
   int i,t,hits,percent_hit,hdel;
   uint tbrc;
   int lbn1,lbn2;
   float mean_hdel,mean_mtbrc;
   int overshot = 0;
   int sect_offset = 0;                 /* initial value */
   int repeats = COARSE_REPEATS;        /* initial value */
   int first = 1;

   do {
#ifdef DEBUG_MTBRC
     printf("Sector offset: %d\n",sect_offset);
#endif

     mean_mtbrc = 0;
     mean_hdel = 0;
     hits = percent_hit = 0;

     mtbrc_lbn_pair(startcyl,
		    sect_offset,
		    req_type,
		    cyldist,
		    &lbn1,
		    &lbn2);
     

     for(t = 0; t < trials; t++) {
       for(i = 0; i < repeats; i++) {
	 tbrc = mtbrc_run_commands(req_type,rw1,rw2,lbn1,lbn2,
				   &hdel,&cmd2_time);

	 mean_hdel += hdel;

	 if (cmd2_time < (long) (MISS_THRESHOLD * ONE_REVOLUTION)) {  
	   /* hit */
	   hits++;
	   mean_mtbrc += tbrc;

#ifdef DEBUG_MTBRC
	   printf("\tMTBRC = %d\t host delay = %d\n",tbrc,hdel);
#endif
	 }
	 else {
#ifdef DEBUG_MTBRC
	     printf("\tMissed threshold: %ld\n",cmd2_time);
#endif
	 }
       }
     }
     
     mean_hdel = mean_hdel / (float) (repeats * trials);
     percent_hit = (hits * 100) / (repeats * trials);
     if (hits > 0) 
       mean_mtbrc = mean_mtbrc / hits;


#ifdef VERBOSE_MTBRC
     /* printf("Hit: %d%% [%d]\tMTBRC = %6.2f\th_del: %d\toffset: %d\n", */
     /*        percent_hit,hits,mean_mtbrc,(int) mean_hdel,sect_offset); */

     //     printf(".");

    /* printf("first: %d\tovershot:%d\t repeats: %s\n",first,overshot, */
    /*       ((repeats==REPETITIONS)?"REPETITIONS":"COARSE_REPEATS")); */
#endif



    if (percent_hit >= HIT_PERCENTILE) {
      if (first) {
	/* first time and got hit, decrement sector_offset */
	if ((req_type == MTBRC_SAME_TRACK) 
	    || (req_type == MTBRC_READ_HIT) 
	    || (req_type == MTBRC_WRITE_HIT)) 
	{
	  first = 0;
	  repeats = REPETITIONS;
	}
	else {
	  sect_offset -= COARSE_OFFSET_INCR; 
	}
      } 
      else if (repeats == COARSE_REPEATS) {
	/* overshot, start incrementing only by one to find MTBRC */
	overshot = 1;
	repeats = REPETITIONS;
	sect_offset += (1 - COARSE_OFFSET_INCR);
      } 
      else {
	/* OK, we landed on MTBRC */
	break;
      }
      /* decrement percent_hit to loop back */
      percent_hit = HIT_PERCENTILE-1;
    }
    else { 
      /* percent_hit < HIT_PRECENTILE */
      first = 0;
      if (overshot) {   
	repeats = REPETITIONS;
	sect_offset += 1;
      }
      else {
	repeats = COARSE_REPEATS;
	sect_offset += COARSE_OFFSET_INCR;
      }
    }
  } while(percent_hit < HIT_PERCENTILE);
  /* until we get HIT_PERCENTILE of hits */



#ifdef VERBOSE_MTBRC
   //  printf("\n");
#endif


  *host_delay = (long) mean_hdel;
  *ireq_dist = sect_offset;
  return((long) mean_mtbrc);
}

// XXX 
#define MAX_SPT 2048

long 
find_one_mtbrc_dumb(int startcyl,
		    int req_type,
		    int rw1,
		    int rw2,
		    int trials,
		    int cyldist,
		    long* host_delay,
		    int* ireq_dist)
{
   long cmd2_time;
   int i,hits,hdel; // t
   uint tbrc;
   int lbn1,lbn2;
   float mean_hdel,mean_mtbrc;
   int sect_offset = 0;                 /* initial value */
   int repeats = COARSE_REPEATS;        /* initial value */

   float tbrcs[MAX_SPT];
   float hdels[MAX_SPT];
   int min = 0;

   int spt;
   struct dm_pbn pbn;
   
   pbn.cyl = startcyl + cyldist - 1;
   pbn.head = 0;
   pbn.sector = 0;
   spt = adisk->layout->dm_get_sectors_pbn(adisk, &pbn);


   for(sect_offset = 0; sect_offset < spt; sect_offset++) {
     hits = 0;

     mean_mtbrc = 0.0;
     mean_hdel = 0.0;

     mtbrc_lbn_pair(startcyl,
		    sect_offset,
		    req_type,
		    cyldist,
		    &lbn1,
		    &lbn2);
       
     //     for(t = 0; t < trials; t++) {
     //       for(i = 0; i < repeats; i++) {
     for(i = 0; i < 10; i++) {
	 tbrc = mtbrc_run_commands(req_type,
				   rw1,rw2,
				   lbn1,lbn2,
				   &hdel,
				   &cmd2_time);
	 if(tbrc > 0) {
	   mean_hdel += hdel;
	   
	   hits++;
	   mean_mtbrc += tbrc;
	   
	   //	   printf("tbrc %d mean_mtbrc %f\n", tbrc, mean_mtbrc);
	 }
     }

     
     mean_hdel = mean_hdel / (float) (repeats * trials);
     
     mean_mtbrc = mean_mtbrc / hits;

     printf("%s() sect_offset: %d %10f\n", 
 	    __PRETTY_FUNCTION__, sect_offset, mean_mtbrc);

     tbrcs[sect_offset] = mean_mtbrc;
     hdels[sect_offset] = mean_hdel;

   }

   for(i = 0; i < spt; i++) {
     printf("%d %f\n", i, tbrcs[i]);
     if(tbrcs[i] < tbrcs[min]) {
       min = i;
     }
   }
   


  *host_delay = (long)hdels[min];
  *ireq_dist = min;
  return (long)(tbrcs[min]);
}





/*---------------------------------------------------------------------------*
 * Determines the MTBRC for a given startcyl and requests rw1 and rw2.       *
 *---------------------------------------------------------------------------*/


static double
testpt(int startcyl,
       int cyldist,
       int sect,
       int req_type,
       int rw1,
       int rw2,
       double *acc) 
{
  if(acc[sect] != 0) {
    return acc[sect];
  }
  else {
    double tbrc, result = 0.0;
    int lbn1, lbn2;

    mtbrc_lbn_pair(startcyl,
		   sect,
		   req_type,
		   cyldist,
		   &lbn1,
		   &lbn2);

    if(lbn2 == -1) {
      return 0.0;
    }

    do {
      int hdel;
      long cmd2_time;

      tbrc = mtbrc_run_commands(req_type,
				rw1,rw2,
				lbn1,lbn2,
				&hdel,&cmd2_time);

      // printf("%s sect %4d, tbrc %f\n", __PRETTY_FUNCTION__, sect, tbrc);

      result += tbrc;
      if(result == 0.0) {
	fprintf(stderr, "*** %s tbrc = 0.0?! (%d,%d)\n",
		__PRETTY_FUNCTION__, lbn1, lbn2);
      }
    } while(result == 0.0);

    acc[sect] = result;

    assert(result > 0.0);

    return result;
  }
}

long 
find_one_mtbrc_bins_real(int req_type,
			 int rw1,
			 int rw2,
			 int trials,
			 int startcyl, 
			 int cyldist,
			 long* host_delay,
			 int* ireq_dist,

			 int pl, // left point
			 int pr, // right point

			 int spt,
			 double *acc)
{
  int pmid;
  double tpl, tpr, tpmid;

  tpl = testpt(startcyl, cyldist, pl, req_type, rw1, rw2, acc);
  if(tpl == 0.0) { return -1; }

  tpr = testpt(startcyl, cyldist, pr, req_type, rw1, rw2, acc);
  if(tpr == 0.0) { return -1; }

  if((pr - pl) < 40) {
    // test everything between pl and pr
    int i;
    double min = tpl, tpi;
    *ireq_dist = pl;
    for(i = pl; i <= pr; i++) {
      tpi = testpt(startcyl, cyldist, i, req_type, rw1, rw2, acc);
      if(tpi < min) {
	min = tpi;
	*ireq_dist = i;
      }
      //      printf("%s %d %f\n", __func__, i, tpi);
    }
    
    //    printf("%s min %f\n", __func__, min);
    return min;
  }

  //   printf("%s pl %d pr %d\n", __PRETTY_FUNCTION__, pl, pr);

  pmid = pl + (pr - pl)/2;
  
  tpmid = testpt(startcyl, cyldist, pmid, req_type, rw1, rw2, acc);
  if(tpmid == 0.0) { return -1; }

  if(tpmid > tpl) {
    pl = pmid;
  }
  else if(tpmid < tpr) {
    pr = pmid;
  }
  else {
    // ssh ... hack
    pl++;
  }

  
  //   printf("%s tpl %f tpmid %f tpr %f\n", __PRETTY_FUNCTION__, tpl, tpmid, tpr);
  


    // tail-recursive
    return find_one_mtbrc_bins_real(req_type,rw1,rw2,trials,startcyl,cyldist,
				    host_delay, ireq_dist,
				    pl, pr, spt, acc);
}



long 
find_one_mtbrc_bins(int startcyl,
		    int req_type,
		    int rw1,
		    int rw2,
		    int trials,
		    int cyldist,
		    long* host_delay,
		    int* ireq_dist)
{
  int spt;
  struct dm_pbn p;
  double result;
  double *acc = calloc(MAX_SPT, sizeof(double));


  p.cyl = startcyl + cyldist;
  p.head = 0;
  p.sector = 0;
  spt = adisk->layout->dm_get_sectors_pbn(adisk, &p);

  result = find_one_mtbrc_bins_real(req_type,rw1,rw2,trials,startcyl,cyldist,
				    host_delay, ireq_dist, 
				    0, spt-1, spt, acc);

//   for(i = 0; i < spt; i++) {
//     if(acc[i] != 0.0) {
//       printf("%d %f\n", i, acc[i]);
//     }
//   }

//   printf("ireq_dist %d\n", *ireq_dist);

//  printf("%s() %f\n", __PRETTY_FUNCTION__, result);

  free(acc);

  return result;
}





/*---------------------------------------------------------------------------*
 * Finds the various completion times.                                       * 
 * rc   - Read Completion                                                    *
 * wc   - Write Completion                                                   *
 * rmaw - Read Miss After Write                                              *
 * waw  - Write After Write                                                  *
 * war  - Write after Read                                                   *
 * rmar - Read Miss After Read                                               *
 * rhaw - Read Hit After Write                                               *
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
void
find_completion_times(float *rc,
		      float *wc,
		      float *rmaw,
		      float *wmaw,
		      float *wmar,
		      float *rmar,
		      float *rhar,
		      float *rhaw,
		      float *whaw,
		      float *whar) 
{
#define RC_INDEX     1
#define WC_INDEX     2
  long m;
  float hdel,irqdist,irqd_time,bx,mx; 
  int i;
  float b[MATRIX_ROWS][2];
  /* coefficiens for bus transfer */
  float bx_const[MATRIX_ROWS] = {9.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
  float mx_const[MATRIX_ROWS] = {9.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
/* char *labels[] = {" ","RC","WC","RMAW","WAW","WAR","RMAR","RHAR","RHAW"}; */
  float seek_time;
  int mtbrc_placement = MTBRC_NEXT_CYL;

  disable_caches();
  warmup_disk();

  seek_time = 0;

  bx = get_bulk_transfer_time() * 1000;

//  m = mtbrc(B_WRITE,B_READ,mtbrc_placement,&hdel,&irqdist,&irqd_time,&mx);
  m = compl_overhead(B_WRITE,B_READ,mtbrc_placement,&hdel,&mx);
  if (mtbrc_placement == MTBRC_NEXT_CYL) {
    m -= (long) seek_time;
  }
  b[1][1] = m - hdel - mx_const[1]*mx;

//  m = mtbrc(B_WRITE,B_WRITE,mtbrc_placement,&hdel,&irqdist,&irqd_time,&mx);
  m = compl_overhead(B_WRITE,B_WRITE,mtbrc_placement,&hdel,&mx);
  if (mtbrc_placement == MTBRC_NEXT_CYL) {
    m -= (long) seek_time;
  }
  b[2][1] = m - hdel - mx_const[2]*mx;

//  m = mtbrc(B_READ,B_WRITE,mtbrc_placement,&hdel,&irqdist,&irqd_time,&mx);
  m = compl_overhead(B_READ,B_WRITE,mtbrc_placement,&hdel,&mx);
  if (mtbrc_placement == MTBRC_NEXT_CYL) {
    m -= (long) seek_time;
  }
  b[3][1] = m - hdel - mx_const[3]*mx;

  if (mtbrc_placement == MTBRC_NEXT_CYL) {
//    m = mtbrc(B_READ,B_READ,mtbrc_placement,&hdel,&irqdist,&irqd_time,&mx);
    m = compl_overhead(B_READ,B_READ,mtbrc_placement,&hdel,&mx);
    m -= (long) seek_time;
  }
  else 
    m = mtbrc(B_READ,B_READ,MTBRC_READ_MISS,&hdel,&irqdist,&irqd_time,&mx);
  b[4][1] = m - hdel - mx_const[4]*mx;

  set_cache(READ_CACHE_ENABLE | WRITE_CACHE_DISABLE);
  m = mtbrc(B_READ,B_READ,MTBRC_READ_HIT,&hdel,&irqdist,&irqd_time,&mx);
  b[5][1] = m - hdel - mx_const[5]*mx;

  m = mtbrc(B_WRITE,B_READ,MTBRC_READ_HIT,&hdel,&irqdist,&irqd_time,&mx);
  b[6][1] = m - hdel - mx_const[6]*mx;

  /* extracting the write hit overheads */
  set_cache(READ_CACHE_ENABLE | WRITE_CACHE_ENABLE);
  m = mtbrc(B_WRITE,B_WRITE,MTBRC_WRITE_HIT,&hdel,&irqdist,&irqd_time,&mx);
  b[7][1] = m - hdel - mx_const[7]*mx;

  m = mtbrc(B_READ,B_WRITE,MTBRC_WRITE_HIT,&hdel,&irqdist,&irqd_time,&mx);
  b[8][1] = m - hdel - mx_const[8]*mx;

  /* subtract the bus transfers from the times*/
  for(i=1;i<MATRIX_ROWS;i++) {
    b[i][1] -= bx_const[i]*bx;
  }
  *rmaw = ( b[1][1] > 0 ? b[1][1] / 1000 : 0);
  /* printf("Read Miss After Write = %.3f\n",b[1][1] / 1000); */

  *wmaw = ( b[2][1] > 0 ? b[2][1] / 1000 : 0);
  /* printf("Write Miss After Write = %.3f\n",b[2][1] / 1000); */

  *wmar = ( b[3][1] > 0 ? b[3][1] / 1000 : 0);
  /* printf("Write After Read = %.3f\n",b[3][1] / 1000); */

  *rmar = ( b[4][1] > 0 ? b[4][1] / 1000 : 0);
  /* printf("Read Miss After Read = %.3f\n",b[4][1] / 1000); */

  *rhar = ( b[5][1] > 0 ? b[5][1] / 1000 : 0);
  /* printf("Read Hit After Read = %.3f\n",b[5][1] / 1000); */

  *rhaw = ( b[6][1] > 0 ? b[6][1] / 1000 : 0);
  /* printf("Read Hit After Write = %.3f\n",b[6][1] / 1000); */

  *whaw = ( b[7][1] > 0 ? b[7][1] / 1000 : 0);
  /* printf("Write Hit After Write = %.3f\n",b[7][1] / 1000); */

  *whar = ( b[8][1] > 0 ? b[8][1] / 1000 : 0);
  /* printf("Write Hit After Read = %.3f\n",b[8][1] / 1000); */

  *rc = *wc = 0;
}

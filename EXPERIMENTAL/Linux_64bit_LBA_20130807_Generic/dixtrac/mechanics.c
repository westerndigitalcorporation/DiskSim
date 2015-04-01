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
#include "handle_error.h"
#include "extract_layout.h"
#include "extract_params.h"
#include "mtbrc.h"
#include "mechanics.h"




// extern diskinfo_t *thedisk;

extern char *scsi_buf;

#undef min
#define min(a,b) (((a)<(b))?(a):(b))



#define MAXLBN     (thedisk->numblocks-1)
#define fetch_lbn(s)           diskinfo_cyl_firstlbn(s,0)

static inline int add_cyls(int s, int d) {
  return ((s+d) < adisk->dm_cyls) ? diskinfo_cyl_firstlbn((s+d), 0) : -1;
}
 


#define MINCYL      diskinfo_get_first_valid_cyl()
#define MAXCYL      diskinfo_get_last_valid_cyl()
#define MAXDIST     (MAXCYL-MINCYL)

#define SEEK_SAMPLES     10
#define SEEK_SETS        1

/* time is in microseconds */
#define MAX_SEEK_TIME    (1000*1000*1000)

#define USE_SEEK


int 
find_seek_time_mtbrc(int distance, 
		     float *max, 
		     float *mean,
		     float *variance,
		     int delay_us)
{
  long tset[SEEK_SETS][SEEK_SAMPLES];
  int startlbn,i,j,lbn,startcyl;
  long min_time = MAX_SEEK_TIME;
  
  int maxdist = MAXDIST;
  float f1,f2,f3,f4; // don't-cares for mtbrc

  /* clear the array with seek times */
  memset((void *) &tset, 0, sizeof(tset));

  if (distance>maxdist)
    return(-1);


  j = 0;

  // why is this necessary?
  //    exec_scsi_command(scsi_buf,SCSI_rezero_command());
//   startcyl = -1;
//   do {

//     startcyl = random_cylinder(distance, NO_CYL_DEFECT, ANY_BAND);


//     startlbn = fetch_lbn(startcyl);
//     /* choose the lbn of the seek target */
//     lbn = add_cyls(startcyl,distance);
//   } while ((startcyl <0) || (lbn<0));

  for(i = 0; i < SEEK_SAMPLES / 2; i++) {
    // for(i = 0; i < 1; i++) {
    startcyl = -1;
    do {
      startcyl = random_cylinder(distance, NO_CYL_DEFECT, ANY_BAND);

      //      printf("%s() startcyl %d\n", __func__, startcyl);
	
      startlbn = fetch_lbn(startcyl);
      /* choose the lbn of the seek target */
      lbn = add_cyls(startcyl,distance);
    } while ((startcyl < 0) || (lbn < 0));

    assert(startcyl != -1);

    // seek to the desired cylinder 

    // SEEK out

    tset[j][2*i+1] = 
      seek_mtbrc(B_READ,
		 B_READ,
		 startcyl,
		 distance,
		 &f1,&f2,&f3,&f4);

    // unlucky guess for cyls
    if(tset[j][2*i+1] == -1) {
	/*
	  printf("%s() (%s:%d) unlucky guess startcyl:%d dist:%d\n", 
	     __PRETTY_FUNCTION__, __FILE__, __LINE__, startcyl, distance);
	*/
      i--; continue;
    }


    min_time = min(min_time,tset[j][2*i+1]);
      
    /* seek back to original cylinder - opposite arm movement */

    tset[j][2*i] = 
      seek_mtbrc(B_READ,
		 B_READ,
		 startcyl + distance,
		 -distance,
		 &f1,&f2,&f3,&f4);

    // unlucky guess for cyls
    if(tset[j][2*i] == -1) {
	/* 
	printf("%s() (%s:%d) unlucky guess %d %d\n", 
	       __PRETTY_FUNCTION__, __FILE__, __LINE__, startcyl, distance);
	*/
      i--; continue;
    }

    //      printf("mtbrc #1 %ld\n", tset[j][2*i + 1]);
    //      printf("mtbrc #2 %ld\n", tset[j][2*i]);

    min_time = min(min_time,tset[j][2*i]);
  } // SEEK_SAMPLES loop


  find_mean_max(SEEK_SETS,SEEK_SAMPLES,&tset[0][0],max,mean,variance);
  return(0);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int 
find_seek_time(int distance, 
	       float *max, 
	       float *mean,
	       float *variance,
	       int delay_us)
{
  extern long ustart,ustop;
  long tset[SEEK_SETS][SEEK_SAMPLES];
  int startlbn,i,j,lbn,startcyl,section;
  long min_time = MAX_SEEK_TIME;
  
  int maxcyl  = MAXCYL;
  int maxdist = MAXDIST;
  int mincyl = MINCYL;

  /* clear the array with seek times */
  memset((void *) &tset, 0, sizeof(tset));

  if (distance>maxdist)
    return(-1);

  section = (maxcyl/SEEK_SETS);

  for(j = 0; j < SEEK_SETS; j++) {
    exec_scsi_command(scsi_buf,SCSI_rezero_command());
    startcyl = -1;
    do {
      if (distance < section) {
	startcyl = mincyl + (rand() % section) + section*j;
      }
      else {
	startcyl = mincyl + (rand() % (maxdist - distance+1));
      }

      while ((startcyl + distance) >maxcyl) {
	startcyl--;
      }

      startlbn = fetch_lbn(startcyl);
      /* choose the lbn of the seek target */
      lbn = add_cyls(startcyl,distance);
    } while ((startcyl <0) || (lbn<0));

    /* printf("%d]: %d\t%d\n",distance,startlbn,lbn); */

    /* printf("Seek at cylinder %d for distance %d\n",startcyl,distance); */


    for(i = 0; i < SEEK_SAMPLES / 2; i++) {
      startcyl = -1;
      do {
	if (distance < section)
	  startcyl = mincyl + (rand() % section) + section*j;
	else {
	  startcyl = mincyl + (rand() % (maxdist - distance+1));
	}

	while ((startcyl + distance) >maxcyl) {
	  startcyl--;
	}
	
	startlbn = fetch_lbn(startcyl);
	/* choose the lbn of the seek target */
	lbn = add_cyls(startcyl,distance);
      } while ((startcyl <0) || (lbn<0));

      //      printf("Start at cylinder %d [%d]\n",startcyl,startlbn);

      /* seek to the desired cylinder */

#ifdef USE_SEEK
      exec_scsi_command(scsi_buf,SCSI_seek_command(startlbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,startlbn,1));
#endif

      // some disks will delay SEEKs if we send them too fast
      busy_wait_loop(delay_us);

      /* seek inwards */
      /* printf("Seek in to cylinder %d [%d]\n",startcyl+distance,lbn); */
#ifdef USE_SEEK
      exec_scsi_command(scsi_buf,SCSI_seek_command(lbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn,1));
#endif

      tset[j][2*i+1] = (ustop-ustart) % 1000000;
      min_time = min(min_time,tset[j][2*i+1]);
      
      /* seek back to original cylinder - opposite arm movement */

      // some disks will delay SEEKs if we send them too fast
      busy_wait_loop(delay_us);

#ifdef USE_SEEK
      exec_scsi_command(scsi_buf, SCSI_seek_command(startlbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,startlbn,1));
#endif

      tset[j][2*i] = (ustop-ustart) % 1000000;
      min_time = min(min_time,tset[j][2*i]);

      //      printf("Time [%d]:\t  %ld\n",distance,tset[j][2*i]);
      /* printf("Time [%d]:\t  %ld\n",distance,tset[j][2*i+1]); */

    }
  }

  /* filter out values that are too high to be real seek times */
  /* an artifact of host activity?                             */
/*    for(j=0;j<SEEK_SETS;j++) */
/*      for(i=0;i<SEEK_SAMPLES;i++)  */
/*        if (tset[j][i] > (3/2 * min_time)) */
/*  	tset[j][i] = min_time; */
  

  find_mean_max(SEEK_SETS,SEEK_SAMPLES,&tset[0][0],max,mean,variance);
  return(0);
}


int dx_seek_curve_len(void) {
  // XXX magic ... see skips in get_seek_times()
  return (MAXDIST / 100) - 5 + 1 + 1 + 99 + 20 + 12;
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
get_seek_times(float *single_cyl,
	       float *full_seek,
	       float *ten_seeks,
	       void(*seekout)(void *ctx, int dist, float time, float stddev),
	       void *seekout_ctx,
	       int delay_us)
{
  // XXX change dx_seek_curve_len if you change this
  int skips[4][2] = {{99,1},{199,5},{499,25},{MAXDIST,100}};
  float mean, max;
  int i = 1;
  int idx = 0;
  int counter = 0;

  double m1, m2, overhead;
  float hdel,irqdist,irqd_time,mx;

  m1 = (double) 
    mtbrc(B_WRITE,B_READ,MTBRC_SAME_TRACK,&hdel,&irqdist,&irqd_time,&mx);
  m2 = (double)
    mtbrc(B_WRITE,B_READ,MTBRC_NEXT_CYL,&hdel,&irqdist,&irqd_time,&mx);

  find_seek_time(1, &max, &mean, 0, delay_us);
  // XXX
  //  overhead = (mean - (m2 - m1)) / 1000;
  overhead = 0;
  printf("mtbrc single cyl seek %f %f %f %f %f\n", m2, m1, mean, max, overhead);


  while(i <= MAXDIST) {
    if (i > skips[idx][0]) {
      idx++;
    }
    i += skips[idx][1];
    counter++;
  }
  counter++;


  i = 1;
  idx = 0;
  while(i < MAXDIST) {
    find_seek_time(i, &max, &mean, 0, delay_us);
    mean = mean / 1000;

    seekout(seekout_ctx, i, mean - overhead, 0);

    if (i > skips[idx][0]) {
      idx++;
    }

    if (i < 11) {
      if(ten_seeks) {
	*ten_seeks = mean;
	ten_seeks++;
      }
    }

    if (i==1) {
      *single_cyl = mean;
    }

    i += skips[idx][1];
  }
  /* determine a full strobe seek cylinder range */
  i = MAXDIST;

  find_seek_time(i, &max, &mean, 0, delay_us);
  *full_seek = (mean / 1000);

  seekout(seekout_ctx, i, *full_seek - overhead, 0);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
get_rotation_speed(int *rot, float *error) 
{
  int i,j,lbn;
  float max,mean;
  long tset[N_SETS][N_SAMPLES];
  int nominal_rot,dummy[3];

  disable_caches();
  get_geometry(&dummy[0],&nominal_rot,&dummy[1],&dummy[2]);
  for(j = 0; j < (N_SETS); j++) {
    for(i = 0; i < (N_SAMPLES); i++) {
      lbn = rand() % adisk->dm_sectors;
      exec_scsi_command(scsi_buf,SCSI_seek_command(lbn));
      tset[j][i] = time_second_command(SCSI_rw_command(B_WRITE,lbn,1),
				       SCSI_rw_command(B_WRITE,lbn,1));
    }
  }

  find_mean_max(N_SETS,N_SAMPLES,&tset[0][0],&max,&mean,0);
  *error = (1/mean) * (60*1000*1000);
  *rot = (int) *error;

  if (*rot == nominal_rot) {
    *error = 0.0;
  }
  else {
    *error = fabs (*error - nominal_rot) / nominal_rot * 100;
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_write_settle(double head_switch, double *result)
{
  double m1,m2;
  float hdel,irqdist,irqd_time,mx;

  m1 = (double)
    mtbrc(B_WRITE,B_WRITE,MTBRC_SAME_TRACK,&hdel,&irqdist,&irqd_time,&mx);
  m2 = (double)
    mtbrc(B_WRITE,B_WRITE,MTBRC_NEXT_TRACK,&hdel,&irqdist,&irqd_time,&mx);

  *result = ((m2-m1) / 1000.0) - head_switch;

  fprintf(stderr, "*** get_write_settle %f %f %f\n", m1, m2, *result);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_head_switch(double *result) 
{
  double m1,m2;
  float hdel,irqdist,irqd_time,mx;

  m1 = (double) 
    mtbrc(B_WRITE,B_READ,MTBRC_SAME_TRACK,&hdel,&irqdist,&irqd_time,&mx);

  m2 = (double)
    mtbrc(B_WRITE,B_READ,MTBRC_NEXT_TRACK,&hdel,&irqdist,&irqd_time,&mx);

  *result = (m2-m1) / 1000.0;

  fprintf(stderr, "*** get_head_switch %f %f %f\n", m1, m2, *result);
}


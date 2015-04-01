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
#include "handle_error.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "print_bscsi.h"
#include "read_diskspec.h"

#ifndef DISKINFO_ONLY
#include <libddbg/libddbg.h>
#include <diskmodel/modules/modules.h>

defect_t *defect_lists = NULL;
struct dm_disk_if *adisk = NULL;
#endif

diskinfo_t *thedisk = NULL;

char *scsi_buf;

#ifdef ADDR_TRANS_COUNTER
int transaddr_cntr;
#endif

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
disk_init(char *device, int use_scsi_device)
{
  int diskno = -1;
 
// I'm deprecating some of this mess.  bucy 20030916
#if 0
  char devname[20];
 
#ifdef SG_TIMER 
#ifndef SILENT
  printf("OK, using /dev/sg kernel timer!\n");
#endif
#endif

  //  sprintf(devname,"%s%c",DEVNAME,'a'+diskno);

  /* interpret the disknum/raw device file argument, it could be just a # */
  if (strncmp(DEVNAME, device, sizeof(DEVNAME))) {
      // OK, the argument is not of the form /dev/...
      diskno = (int) atoi((const char *) devname);
      sprintf(devname,"%s%c",DEVNAME,'a'+diskno);
      device = devname;
  }

  if (diskno < 0) {
    error_handler("'%s' is invalid disk specification!\n",device);
  }
#endif // deprecated code

  /* done processing arguments, do some real work */  
  if (use_scsi_device) {
    scsi_init(device);
    scsi_buf = scsi_alloc_buffer();
    
#if 0
    /* set definition of disk to SCSI-3 */
    exec_scsi_command(scsi_buf,SCSI_chg_definition(SCSI_3_DEF));
      printf("SCSI definition set to SCSI-3.\n");
#endif
#ifndef SILENT
      /* print basic identification of the drive */
      exec_scsi_command( scsi_buf, SCSI_inq_command());
      print_scsi_inquiry_data(scsi_buf,device);
#endif
  }

  return(diskno);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
disk_shutdown(int use_scsi_device)
{
  if (use_scsi_device) {
    scsi_dealloc_buffer();
    close_scsi_disk();
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
internal_error(const char *fmt, ...)
{
#ifdef DEBUG
  int internal_error = 1;
#endif
  va_list args;

  fprintf(stderr," ------- Internal error -------\n");
  va_start(args, fmt);
  vfprintf(stderr,fmt,args);
  va_end(args);
#ifdef DEBUG
  assert(internal_error == 0);
#endif
  fprintf(stderr," ------- Internal error -------\n");
  exit(2);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
error_handler(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr,fmt,args);
  va_end(args);
  exit(1);
}

#ifdef MEASURE_TIME
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
float
elapsed_time(void) 
{
  static struct timeval last_tv;
  float retval;
  long t1,t2;
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv,&tz);
  t1 = tv.tv_sec;
  t2 = tv.tv_usec;

  if (t2 < last_tv.tv_usec) {
    t2 += 1000000;
    t1--;
  }
  retval = (float) (t2 - last_tv.tv_usec) / 1000000;
  retval += (float) (t1 - last_tv.tv_sec);

  last_tv.tv_sec = t1;
  last_tv.tv_usec = t2;
  return(retval);
}
#endif

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
long 
time_two_commands(scsi_command_t *c1,scsi_command_t *c2)
{

/*   extern long ustop,ustart; */
/*   long end[2],command[2]; */

  long time,hdelay;

  long start_sec[2],start[2],end[2],command[2];
  extern long ustop,ustart,ustart_sec;

  exec_scsi_command(scsi_buf,c1);
  end[0] = ustop;
  start[0] = ustart;
  start_sec[0] = ustart_sec;
  command[0] = ustop - ustart;

  exec_scsi_command(scsi_buf,c2);
  command[1] = ustop - ustart;
  end[1] = ustop;
  start[1] = ustart;
  start_sec[1] = ustart_sec;

  time = ((((start_sec[1]-start_sec[0])*1000000)+end[1])-start[0]);
  hdelay = ((((start_sec[1]-start_sec[0])*1000000)+start[1])-end[0]);

  return((long) (time - hdelay));
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
long 
time_second_command(scsi_command_t *c1,scsi_command_t *c2)
{
  long time,hdelay;

  long start_sec[2],start[2],end[2],command[2];
  extern long ustop,ustart,ustart_sec,ustop_sec;

  exec_scsi_command(scsi_buf,c1);
  end[0] = ustop;
  start[0] = ustart;
  start_sec[0] = ustart_sec;
  command[0] = ustop - ustart;

  exec_scsi_command(scsi_buf,c2);
  command[1] = ustop - ustart;
  end[1] = ustop;
  start[1] = ustart;
  start_sec[1] = ustart_sec;

  time = ((start_sec[1]-start_sec[0])*1000000) + (end[1] - end[0]);
  hdelay = ((((start_sec[1]-start_sec[0])*1000000)+start[1])-end[0]);

  return ((ustop_sec - ustart_sec) * 1000000) + (end[1] - start[1]);
}

/*---------------------------------------------------------------------------*
 * Busy loops for 'howlong' microseconds.                                    * 
 *---------------------------------------------------------------------------*/
void
busy_wait_loop(long howlong)
{
  long t1,t2;
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv,&tz);
  t1 = tv.tv_sec;
  t2 = tv.tv_usec;
  do {
    gettimeofday(&tv,&tz);    
  } while ((tv.tv_usec + 1000000*(tv.tv_sec-t1)) < (t2 + howlong));
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
// find_mean_max() takes in a 2d array (tset) of sets*samples and
// computes stats across each set, performing some noise reduction,
// then crunches this down to the aggregate over the whole thing.  I
// can't speak for the validity of this computation; AFAIK, Jiri made
// it up.  (bucy 200506)
void 
find_mean_max(int sets,
	      int samples,
	      long *tset, 

	      // result params
	      float *max, 
	      float *mean,
	      float *variance)
{
/* discard the values more than PERCENTAGE away from the median value */
#define PERCENTAGE    0.15
  int i,j,k;
  float temp;
  long *oset;
  float *total,*count,*avrg,*median,*maxval, *var;

  if((sets == samples) && (samples == 1)) {
    if(max) {
      *max = (float)tset[0];
    }
    if(mean) {
      *mean = (float)tset[0];
    }
    if(variance) {
      *variance = 0.0;
    }
    return;
  }

  // noise-reduced data
  oset = calloc(sets * samples, sizeof(long));
  assert(oset!=NULL);

  // popcount of each set
  count = calloc(sets, sizeof(float));
  assert(count!=NULL);

  avrg = calloc(sets, sizeof(float));
  assert(avrg!=NULL);

  median = calloc(sets, sizeof(float));
  assert(median!=NULL);

  total = calloc(sets, sizeof(float));
  assert(total!=NULL);

  maxval = calloc(sets, sizeof(float));
  assert(maxval!=NULL);

  var = calloc(sets, sizeof(float));
  assert(var);

  memcpy(oset, tset, (sizeof(long) * sets * samples));

  for(j = 0; j < sets; j++) {
    count[j] = 0;
    total[j] = 0;

    for(i = 0; i < samples; i++) {
      if (tset[j*samples+i] != 0) {
	total[j] += tset[j*samples+i];
	count[j]++;
      }

      /* sort the elements */
      // XXX use lib
      for(k = 0; k < (samples-1); k++) {
	if(oset[j*samples+k] > oset[j*samples+k+1]) {
	  temp = oset[j*samples+k+1];
	  oset[j*samples+k+1] = oset[j*samples+k];
	  oset[j*samples+k] = temp;
	}
      }
    }

    /* find median of set j */
    i = 0;
    // invariant: not all 0 ??
    while (oset[j*samples+i] == 0) { i++; }
    median[j] = oset[j*samples+((samples-i)/2)+i];

    /*
     * printf("j=%d, i=%d\t median: %d index: %d\n",j,i,(int) median[j], 
     *        i+((samples-i)/2));
     */	   

    /* discard the values from set j more than PERCENTAGE away from
       the median */
    for(i = 0; i < samples; i++) {
      if ((tset[j*samples+i] != 0) 
	  && ((tset[j*samples+i] > median[j]*(1+PERCENTAGE)) 
	      || (tset[j*samples+i] < median[j]*(1-PERCENTAGE)))) 
      {
	count[j]--;
	total[j] -= tset[j*samples+i];
	/* printf("Discarded: %d \n",(int) tset[j*samples+i]); */
	tset[j*samples+i] = 0;
      }
    }

    /* copy to oset the trimmed dataset */
    memcpy((void *)oset, (void *)tset, (sizeof(float)*sets*samples));

    /* find the max value ?? */

    for(i = 0; i < samples; i++) {
      oset[j*samples+i] = tset[j*samples+i];
    }

    // sort oset?? (use lib!)
    for(i = 0; i < samples; i++) {
      for(k = 0; k < samples-1; k++) {
	if(oset[j*samples+k] > oset[j*samples+k+1]) {
	  temp = oset[j*samples+k+1];
	  oset[j*samples+k+1] = oset[j*samples+k];
	  oset[j*samples+k] = temp;
	}
      }
    }


    /*
     * printf("\nMeasured: ");
     * for(i=0;i<samples;i++) 
     *   printf("%d ",(int) tset[j*samples+i]);
     * printf("\nSorted: ");
     * for(k=0;k<(samples);k++) {
     *   printf("%d ",(int) oset[j*samples+k]);
     * }
     * printf("\n");
     */

    maxval[j] = oset[j * samples + samples - 1]; 

    if (count[j]) {
      avrg[j] = total[j] / count[j]; 
    }
    else {
      avrg[j] = 0;
    }

    
    // printf("[j=%d] maxval: %d, total: %d, count: %d\n", j,
    // (int)maxval[j], (int) total[j], (int) count[j]);
     	   
  } //   for(j = 0; j < sets; j++) {

  *max = 0;
  *mean = 0;

  for(j = 0; j < sets; j++) {
    /* printf("maxval: %f avrg: %f\n",maxval[j],avrg[j]); */
    *max += maxval[j];
    *mean += avrg[j];
  }


  *max = *max / sets;
  *mean = *mean / sets;


  if(variance) {
    // Compute the variance, first across rows and then average rows together.
    // Only include samples within PERCENTAGE of the median.

    for(j = 0; j < sets; j++) {
      for(i = 0; i < samples; i++) {
	if ((tset[j*samples+i]!=0) 
	    && ((tset[j*samples+i] <= median[j]*(1+PERCENTAGE)) 
		|| (tset[j*samples+i] >= median[j]*(1-PERCENTAGE)))) 
	{
	  double tmp;
	  tmp = tset[j*samples+i] - avrg[j];
	  //	  printf("%s %d %d %f\n", __PRETTY_FUNCTION__, i, j, tmp);
	  var[j] += pow(tmp, 2.0);
	}
      }

      var[j] /= count[j];
    }

    *variance = 0;
    for(j = 0; j < sets; j++) {
      *variance += var[j];
    }
    *variance /= sets;
  }


  /*
    printf("Final Mean: %d \n",(int) *mean);
    printf("Final Max: %d \n",(int) *max);
  */

  free(oset);
  free(count);
  free(avrg);
  free(median);
  free(total);
  free(maxval);
}
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
float 
get_host_delay(void) 
{
  extern long ustart,ustop;
  int i,j;
  float max,mean;
  long tset[N_SETS][N_SAMPLES];
  for(j=0; j < (N_SETS); j++) {
    for(i=0; i < (N_SAMPLES); i++) {
      exec_scsi_command(scsi_buf,SCSI_read_capacity(0,0));
      tset[j][i] = ustop-ustart;
    }
  }
  find_mean_max(N_SETS,N_SAMPLES,&tset[0][0],&max,&mean,0);
  return (mean / 1000);
}
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
float
trans_addr_delay(void) 
{
  extern long ustop,ustart;
  int i,j,lbn,len;
  float max,mean;
  long c_start,c_stop;
  long tset[N_SETS][N_SAMPLES];

  for(j=0; j < (N_SETS); j++) {
    for(i=0; i < (N_SAMPLES); i++) {
      lbn = rand() % 2500000;
      len = create_lba_to_phys_page(scsi_buf,lbn);
      exec_scsi_command(scsi_buf,SCSI_send_diag_command(len));
      c_start = ustart;
      exec_scsi_command(scsi_buf,SCSI_recv_diag_command(len));
      c_stop = ustop;
      tset[j][i] = (long) (c_stop - c_start);
    }
  }
  find_mean_max(N_SETS,N_SAMPLES,&tset[0][0],&max,&mean,0);
  mean = mean/1000;
  printf("Translate Address delay: %.5f ms\n",mean); 
  return(mean);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
warmup_disk(void) 
{
/* number of random read requests for warm up */
#define WARMUPS       50

  int i;
  int lbn;
  exec_scsi_command(scsi_buf,SCSI_rezero_command());
  for(i = 0; i < WARMUPS; i++) {
    lbn = rand() % adisk->dm_sectors;
    exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn,1));
  }
}


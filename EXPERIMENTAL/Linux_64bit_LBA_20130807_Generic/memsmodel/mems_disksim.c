/*
 * DiskSim Storage Subsystem Simulation Environment (Version 4.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty" statements
 * are included with all reproductions and derivative works and associated
 * documentation. This software may also be redistributed without charge
 * provided that the copyright and "No Warranty" statements are included
 * in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
 * OR DOCUMENTATION.
 *
 */


/* mems_disksim.c
 *
 * Functions integrating the memsdevice simulator with disksim.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mems_global.h"
#include "mems_internals.h"
#include "mems_mapping.h"
//#include "disksim_global.h"
//#include "disksim_iosim.h"	/* Provides MAXINBUSES */
#include "disksim_device.h"

//#include "disksim_ioqueue.h"	/* Provides ioqueue_cleanstats() */
//#include "disksim_bus.h"	/* Provides bus_get_transfer_time() */

#include "config.h"

#include "modules/modules.h"

double bus_get_transfer_time(int busno, int bcount, int read);
void stat_initialize (FILE *statdef_file, char *statdesc, statgen *statptr);

/* read-only global vars used during mems_statinit */
static char *statdesc_request_energy_uj = "Per-request energy (uJ)";
static char *statdesc_seek_time		= "Seek time";
static char *statdesc_x_seek_time	= "X Seek time";
static char *statdesc_y_seek_time	= "Y Seek time";
static char *statdesc_turnaround_time	= "Initial turnaround time";
static char *statdesc_turnaround_number	= "Initial turnaround number";
static char *statdesc_stream_turnaround_time   = "Streaming turnaround time";
static char *statdesc_stream_turnaround_number = "Streaming turnaround number";
static char *statdesc_subtrack_accesses = "Subtrack accesses";
static char *statdesc_tips_per_access   = "Average tips per access";
static char *statdesc_inactive_time	= "Inactive time";
static char *statdesc_prefetched_blocks = "Prefetched blocks";

struct device_header mems_hdr_initializer;

/************************************************************************
 * Functions related to mapping physical organization
 ************************************************************************/

/* Return the inbus number for the device */
int
mems_get_busno (ioreq_event *curr)
{
  mems_t *dev = getmems(curr->devno);
  intchar busno;
  int depth;

  busno.value = curr->busno;
  depth = dev->depth[0];	/* Only one bus connection allowed */
  return busno.byte[depth];
}


/* How far down the bus hierarchy is this device? */
int
mems_get_depth (int devno)
{
  mems_t *dev = getmems(devno);
  return dev->depth[0];		/* Only one bus connection allowed */
}


/* Which inbus connects to the device? */
int
mems_get_inbus (int devno)
{
  mems_t *dev = getmems(devno);
  return dev->inbuses[0];	/* Only one bus connection allowed */
}


/* In which slot on the bus is the device attached? */
int
mems_get_slotno (int devno)
{
  mems_t *dev = getmems(devno);
  return dev->slotno[0];
}


/* Set the inbus information (where, which slot, how deep) */
int
mems_set_depth (int devno, int inbusno, int depth, int slotno)
{
  mems_t *dev = getmems(devno);
  int count;

  count = dev->numinbuses;
  dev->numinbuses++;
  if ((count + 1) > MAXINBUSES) {
    fprintf(stderr,"Too many inbuses specified for memsdevice %d--%d\n", devno, count + 1);
    exit(1);
  }
  dev->inbuses[count] = inbusno;
  dev->depth  [count] = depth;
  dev->slotno [count] = slotno;

  return 0;
}


/************************************************************************
 * ioqueue support functions
 ************************************************************************/

/* Get the number of sectors per cylinder */
int
mems_get_avg_sectpercyl (int devno)
{
  mems_t *dev = getmems(devno);
  return mems_get_blocks_per_cylinder(&dev->sled[0]);
}


/* Find the cylinder, surface, and block mapping of a block */
void
mems_get_mapping (int maptype, int devno, int blkno, 
		   int *cylptr, int *surfaceptr, int *blkptr)
{
  mems_t *dev = getmems(devno);
  mems_sled_t *sled = mems_lbn_to_sled(dev, blkno);
  tipsector_coord_set_t up, dn;
  tipset_t tipset;

  if ((blkno < 0) || (blkno >= dev->numblocks)) {
    fprintf(stderr, "Invalid blkno at mems_get_mapping (%d)\n", blkno);
    exit(1);
  }

  mems_lbn_to_position(blkno, sled, &up, &dn, &tipset, cylptr, surfaceptr, blkptr);
}


/* Get the number of cylinders (e.g., X distance bits) in the device */
int
mems_get_numcyls (int devno)
{
  mems_t *dev = getmems(devno);
  return mems_get_number_of_cylinders(dev);
}


double
mems_get_acctime (int devno, ioreq_event *req, double maxtime)
{
  // fprintf(stderr, "Entering mems_get_acctime()\n");

  /* This function can be extended to calculate the access time
   * for any request.  It's not extended yet.  We shouldn't ever
   * get here, so just die. */
  assert(FALSE);

  return 0.0;
}


double 
mems_get_seektime (int devno, ioreq_event *req, int checkcache, double maxtime)
{
  mems_t *dev = getmems(req->devno);
  mems_sled_t *sled = mems_lbn_to_sled(dev, req->blkno);
  double up_time;
  double dn_time;
  tipsector_coord_set_t up;
  tipsector_coord_set_t dn;
  tipset_t tipset;

  // fprintf(stderr, "Entering mems_get_seektime()\n");

  mems_lbn_to_position(req->blkno, sled, &up, &dn, &tipset, NULL, NULL, NULL);
  up_time = mems_seek_time(sled, &sled->pos, &up.servo_start, NULL, NULL, NULL, NULL);
  dn_time = mems_seek_time(sled, &sled->pos, &dn.servo_start, NULL, NULL, NULL, NULL);
  return min(up_time, dn_time);
}

void
mems_get_average_position (tipsector_coord_set_t *up,
			   tipsector_coord_set_t *down,
			   coord_t *average)
{
  assert(up->servo_start.x_pos == down->servo_start.x_pos);

  average->x_pos = up->servo_start.x_pos;
  average->y_pos = (up->servo_start.y_pos + down->servo_start.y_pos) / 2;
  average->y_vel = 0;
}

/*
 * mems_get_distance
 *
 * return the distance from the current position, or a given lbn, to the position
 *  of req.
 *
 * devno - the number of the device in question.
 *
 * req - a pointer to the request, which gives the destination position.  we're
 *       trying to find the distance to this point.
 *
 * exact - if equal to -1, then use the sled's current position as the starting
 *         point.  else, this gives the lbn to use as the starting position.
 *
 * direction - ignored for now.
 *
 */

// #define DISTANCE_DEBUG
int
mems_get_distance (int devno, ioreq_event *req, int exact, int direction)
{
  coord_t *start_position;
  coord_t *dest_position;

  mems_t *dev = getmems(devno);
  mems_sled_t *sled = mems_lbn_to_sled(dev, req->blkno);

  tipsector_coord_set_t up, down;
  tipset_t tipset;

  int distance1, distance2;

  int x1, x2, y1, y2;

#ifdef DISTANCE_DEBUG
  fprintf(outputfile, "mems_get_distance -- devno = %d, req->blkno = %d, exact = %d, direction = %d\n",
	  devno, req->blkno, exact, direction);
#endif

  if (exact == -1)
    {
      start_position = mems_get_current_position(sled);
    }
  else
    {
      // find the starting position
      mems_lbn_to_position(exact,
			   sled,
			   &up, &down,
			   &tipset,
			   NULL, NULL, NULL);
      
      start_position = (coord_t *)malloc(sizeof(coord_t));
      
      mems_get_average_position(&up, &down, start_position);
    }

  // dest_position = (coord_t *)malloc(sizeof(coord_t));
  
  // find the destination position
  mems_lbn_to_position(req->blkno,
		       sled,
		       &up, &down,
		       &tipset,
		       NULL, NULL, NULL);

  // mems_get_average_position(&up, &down, dest_position);

  x1 = start_position->x_pos;
  x2 = up.servo_start.x_pos;
  y1 = start_position->y_pos;
  y2 = up.servo_start.y_pos;

#ifdef DISTANCE_DEBUG
  fprintf(outputfile, "mems_get_distance -- UP - x1 = %d, y1 = %d, x2 = %d, y2 = %d\n",
	  x1, y1, x2, y2);
#endif

  distance1 = (int)(sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));

  x1 = start_position->x_pos;
  x2 = down.servo_start.x_pos;
  y1 = start_position->y_pos;
  y2 = down.servo_start.y_pos;

#ifdef DISTANCE_DEBUG
  fprintf(outputfile, "mems_get_distance -- DOWN - x1 = %d, y1 = %d, x2 = %d, y2 = %d\n",
	  x1, y1, x2, y2);
#endif

  distance2 = (int)(sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
  
  if (exact != -1)
    {
      free(start_position);
    }

#ifdef DISTANCE_DEBUG
  fprintf(outputfile, "mems_get_distance -- distance1 = %d, distance2 = %d\n",
	  distance1, distance2);
#endif

  if (distance1 < distance2)
    {
#ifdef DISTANCE_DEBUG
      fprintf(outputfile, "returning distance1 = %d\n", distance1);
#endif
      return (distance1);
    }
  else
    {
#ifdef DISTANCE_DEBUG
      fprintf(outputfile, "returning distance2 = %d\n", distance2);
#endif
      return (distance2);
    }
}
#undef DISTANCE_DEBUG

double
mems_get_servtime (int devno, ioreq_event *req, int checkcache, double maxtime)
{
  // fprintf(stderr, "Entering mems_get_servtime() --");
  return mems_get_seektime(devno, req, checkcache, maxtime);
}


/************************************************************************
 * General / informational for disksim components
 ************************************************************************/

/* Return the bulk sector transfer time (the greater of the device bulk
 * sector transfer time and the bus bulk sector transfer time) */
double
mems_get_blktranstime (ioreq_event *curr)
{
  mems_t *dev = getmems(curr->devno);
  double tmptime;

  /* return min(bus_blktranstime,dev_blktranstime) */
//  tmptime = bus_get_transfer_time(mems_get_busno(curr), 1, (int)(curr->flags & READ));
  tmptime = bus_get_transfer_time( 0,0,0 );
  if (tmptime < dev->blktranstime) {
    /* FIXME: Should there be a distinction between read/write times? */
    tmptime = dev->blktranstime;
  }

  return tmptime;
}


/* What's the maximum number of outstanding requests on the device? */
int
mems_get_maxoutstanding (int devno)
{
  mems_t *dev = getmems(devno);
  return dev->maxqlen;
}


/* How many blocks are available across all sleds on the device? */
int
mems_get_number_of_blocks (int devno)
{
  mems_t *dev = getmems(devno);
  if (dev->numblocks == 0) mems_check_numblocks(dev);
  return dev->numblocks;
}


/************************************************************************
 * Statistics stuff
 ************************************************************************/

static void
mems_statinit (int devno, int firsttime)
{
  mems_t *dev = getmems(devno);
  int j;

  /* First initialize device statistics */
  if (firsttime) {
    stat_initialize (statdeffile, statdesc_request_energy_uj, 
		     &dev->stat.request_energy_uj);
    stat_initialize (statdeffile, statdesc_seek_time,
		     &dev->stat.seek_time);
    stat_initialize (statdeffile, statdesc_x_seek_time,
		     &dev->stat.x_seek_time);
    stat_initialize (statdeffile, statdesc_y_seek_time,
		     &dev->stat.y_seek_time);
    stat_initialize (statdeffile, statdesc_turnaround_time,
		     &dev->stat.turnaround_time);
    stat_initialize (statdeffile, statdesc_turnaround_number,
		     &dev->stat.turnaround_number);

    stat_initialize (statdeffile, statdesc_stream_turnaround_time,
		     &dev->stat.stream_turnaround_time);
    stat_initialize (statdeffile, statdesc_stream_turnaround_number,
		     &dev->stat.stream_turnaround_number);

    stat_initialize (statdeffile, statdesc_subtrack_accesses,
		     &dev->stat.subtrack_accesses);
    stat_initialize (statdeffile, statdesc_tips_per_access,
		     &dev->stat.tips_per_access);
    stat_initialize (statdeffile, statdesc_inactive_time,
		     &dev->stat.inactive_time);
    stat_initialize (statdeffile, statdesc_prefetched_blocks,
		     &dev->stat.prefetched_blocks);
  } else {
    stat_reset (&dev->stat.request_energy_uj);
    stat_reset (&dev->stat.seek_time);
    stat_reset (&dev->stat.x_seek_time);
    stat_reset (&dev->stat.y_seek_time);
    stat_reset (&dev->stat.turnaround_time);
    stat_reset (&dev->stat.turnaround_number);

    stat_reset (&dev->stat.stream_turnaround_time);
    stat_reset (&dev->stat.stream_turnaround_number);

    stat_reset (&dev->stat.subtrack_accesses);
    stat_reset (&dev->stat.tips_per_access);
    stat_reset (&dev->stat.inactive_time);
    stat_reset (&dev->stat.prefetched_blocks);
  }
  dev->stat.total_energy_j = 0.0;
  dev->stat.servicing_energy_j = 0.0;
  dev->stat.startup_energy_j   = 0.0;
  dev->stat.idle_energy_j      = 0.0;
  dev->stat.inactive_energy_j  = 0.0;
  dev->stat.num_spinups    = 0;
  dev->stat.num_spindowns  = 0;
  dev->stat.num_buffer_accesses = 0;
  dev->stat.num_buffer_hits     = 0;
  dev->stat.num_initial_turnarounds = 0;
  dev->stat.num_stream_turnarounds  = 0;

  /* Now initialize sled statistics */
  for (j=0; j < dev->num_sleds; j++) {
    if (firsttime) {
      stat_initialize (statdeffile, statdesc_request_energy_uj,
		       &dev->sled[j].stat.request_energy_uj);
      stat_initialize (statdeffile, statdesc_seek_time,
		       &dev->sled[j].stat.seek_time);
      stat_initialize (statdeffile, statdesc_x_seek_time,
		       &dev->sled[j].stat.x_seek_time);
      stat_initialize (statdeffile, statdesc_y_seek_time,
		       &dev->sled[j].stat.y_seek_time);
      stat_initialize (statdeffile, statdesc_turnaround_time,
		       &dev->sled[j].stat.turnaround_time);
      stat_initialize (statdeffile, statdesc_turnaround_number,
		       &dev->sled[j].stat.turnaround_number);

      stat_initialize (statdeffile, statdesc_stream_turnaround_time,
		       &dev->sled[j].stat.stream_turnaround_time);
      stat_initialize (statdeffile, statdesc_stream_turnaround_number,
		       &dev->sled[j].stat.stream_turnaround_number);

      stat_initialize (statdeffile, statdesc_subtrack_accesses,
		       &dev->sled[j].stat.subtrack_accesses);
      stat_initialize (statdeffile, statdesc_tips_per_access,
		       &dev->sled[j].stat.tips_per_access);
      stat_initialize (statdeffile, statdesc_inactive_time,
		       &dev->sled[j].stat.inactive_time);
      stat_initialize (statdeffile, statdesc_prefetched_blocks,
		       &dev->sled[j].stat.prefetched_blocks);
    } else {
      stat_reset (&dev->sled[j].stat.request_energy_uj);
      stat_reset (&dev->sled[j].stat.seek_time);
      stat_reset (&dev->sled[j].stat.x_seek_time);
      stat_reset (&dev->sled[j].stat.y_seek_time);
      stat_reset (&dev->sled[j].stat.turnaround_time);
      stat_reset (&dev->sled[j].stat.turnaround_number);

      stat_reset (&dev->sled[j].stat.stream_turnaround_time);
      stat_reset (&dev->sled[j].stat.stream_turnaround_number);

      stat_reset (&dev->sled[j].stat.subtrack_accesses);
      stat_reset (&dev->sled[j].stat.tips_per_access);
      stat_reset (&dev->sled[j].stat.inactive_time);
      stat_reset (&dev->sled[j].stat.prefetched_blocks);
    }
    dev->sled[j].stat.total_energy_j = 0.0;
    dev->sled[j].stat.servicing_energy_j = 0.0;
    dev->sled[j].stat.startup_energy_j   = 0.0;
    dev->sled[j].stat.idle_energy_j      = 0.0;
    dev->sled[j].stat.inactive_energy_j  = 0.0;
    dev->sled[j].stat.num_spinups    = 0;
    dev->sled[j].stat.num_spindowns  = 0;
    dev->sled[j].stat.num_buffer_accesses = 0;
    dev->sled[j].stat.num_buffer_hits     = 0;
    dev->sled[j].stat.num_initial_turnarounds = 0;
    dev->sled[j].stat.num_stream_turnarounds  = 0;
  }
}


/* Reset device and ioqueue statistics */
void 
mems_cleanstats (void)
{
  mems_t *dev;
  int i;		/* loop counter [which device] */
  int j;		/* loop counter [which sled on device] */

  if (disksim->memsinfo == NULL) {
    return;
  }

  /* FIXME: Should this be (i=0; i<disksim->memsinfo->nummdevices) ? */
  for (i = 0; i < disksim->memsinfo->devices_len; i++) {
    dev = getmems(i);
    if (dev) {
      ioqueue_cleanstats(dev->queue);
      /* FIXME: Should this be (j=0; j<dev->numsleds; j++) ? */
      for (j = 0; j < dev->num_sleds; j++) {
	ioqueue_cleanstats(dev->sled[j].queue);
	/* FIXME: Should we also clean mems stat structures? */
      }
    }
  }
  return;
}


/* Uh, reset the statistics. */
void
mems_resetstats (void)
{
  mems_t *dev;
  int i;
  int j;

  if (disksim->memsinfo == NULL) {
    return;
  }

  for (i = 0; i < disksim->memsinfo->devices_len; i++) {
    dev = getmems(i);
    if (dev) {
      ioqueue_resetstats(dev->queue);
      for (j=0; j<MEMS_MAXSLEDS; j++) {
	ioqueue_resetstats(dev->sled[j].queue);
      }
      mems_statinit(i, FALSE);
    }
  }
}


static void
mems_prefetch_printstats (int *set, int setsize, char *prefix)
{
  int i;
  int j;
  statgen *statset[MAXDEVICES];
  mems_t *dev = 0;

  /************************************************************************
   * prefetched_blocks
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.prefetched_blocks;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.prefetched_blocks;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.prefetched_blocks;
      stat_print_set(statset, 1, newprefix);
    }
  }
}


static void
mems_seektime_printstats (int *set, int setsize, char *prefix)
{
  int i;
  int j;
  statgen *statset[MAXDEVICES];
  mems_t *dev = 0;

  /************************************************************************
   * seek_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.seek_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.seek_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.seek_time;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * x_seek_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.x_seek_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.x_seek_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.x_seek_time;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * y_seek_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.y_seek_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.y_seek_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.y_seek_time;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * turnaround_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.turnaround_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.turnaround_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.turnaround_time;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * turnaround_number
   ************************************************************************/
  
  /* Overall statistics */

  fprintf(outputfile, "%sTotal initial turnarounds: %d\n",
	  prefix, dev->stat.num_initial_turnarounds);

  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.turnaround_number;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);

    fprintf(outputfile, "%sTotal initial turnarounds: %d\n",
	    newprefix, dev->stat.num_initial_turnarounds);

    statset[0] = &dev->stat.turnaround_number;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.turnaround_number;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * stream_turnaround_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.stream_turnaround_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.stream_turnaround_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.stream_turnaround_time;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * stream_turnaround_number
   ************************************************************************/
  
  /* Overall statistics */
  fprintf(outputfile, "%sTotal streaming turnarounds: %d\n",
	  prefix, dev->stat.num_stream_turnarounds);

  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.stream_turnaround_number;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);

    fprintf(outputfile, "%sTotal streaming turnarounds: %d\n",
	    newprefix, dev->stat.num_stream_turnarounds);

    statset[0] = &dev->stat.stream_turnaround_number;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.stream_turnaround_number;
      stat_print_set(statset, 1, newprefix);
    }
  }
}


static void
mems_energy_printstats (int *set, int setsize, char *prefix)
{
  int i;
  int j;
  statgen *statset[MAXDEVICES];
  mems_t *dev;

  /************************************************************************
   * Per-request energy (request_energy_uj)
   ************************************************************************/

  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.request_energy_uj;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.request_energy_uj;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.request_energy_uj;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * Total energy dissipated
   ************************************************************************/

  {
    double total_energy = 0.0;
    double servicing_energy = 0.0;
    double startup_energy   = 0.0;
    double idle_energy      = 0.0;
    double inactive_energy  = 0.0;
    for (i=0; i < setsize; i++) {
      dev = getmems(set[i]);
      total_energy += dev->stat.total_energy_j;
      servicing_energy += dev->stat.servicing_energy_j;
      startup_energy   += dev->stat.startup_energy_j;
      idle_energy      += dev->stat.idle_energy_j;
      inactive_energy  += dev->stat.inactive_energy_j;
    }
    fprintf(outputfile, "%sTotal energy (J): %f\n", prefix, total_energy);
    fprintf(outputfile, 
	    "%sServicing energy (J): %f\n", prefix, servicing_energy);
    fprintf(outputfile, 
	    "%sStartup energy   (J): %f\n", prefix, startup_energy);
    fprintf(outputfile, 
	    "%sIdle energy      (J): %f\n", prefix, idle_energy);
    fprintf(outputfile, 
	    "%sInactive energy  (J): %f\n", prefix, inactive_energy);
    fprintf(outputfile, "\n");
  }

  /* Finally, total energy dissipated by each device and sled */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    fprintf(outputfile, "%sTotal energy (J) for device %d: %f\n", 
	    prefix, set[i], dev->stat.total_energy_j);
    fprintf(outputfile, "%sServicing energy (J) for device %d: %f\n", 
	    prefix, set[i], dev->stat.servicing_energy_j);
    fprintf(outputfile, "%sStartup energy   (J) for device %d: %f\n",
	    prefix, set[i], dev->stat.startup_energy_j);
    fprintf(outputfile, "%sIdle energy      (J) for device %d: %f\n", 
	    prefix, set[i], dev->stat.idle_energy_j);
    fprintf(outputfile, "%sInactive energy  (J) for device %d: %f\n", 
	    prefix, set[i], dev->stat.inactive_energy_j);

    for (j=0; j < dev->num_sleds; j++) {
      fprintf(outputfile, "%sTotal energy (J) for device %d sled %d: %f\n", 
	      prefix, set[i], j, dev->sled[j].stat.total_energy_j);
      fprintf(outputfile, 
	      "%sServicing energy (J) for device %d sled %d: %f\n", 
	      prefix, set[i], j, dev->sled[j].stat.servicing_energy_j);
      fprintf(outputfile,
	      "%sStartup energy   (J) for device %d sled %d: %f\n",
	      prefix, set[i], j, dev->sled[j].stat.startup_energy_j);
      fprintf(outputfile, 
	      "%sIdle energy      (J) for device %d sled %d: %f\n", 
	      prefix, set[i], j, dev->sled[j].stat.idle_energy_j);
      fprintf(outputfile, 
	      "%sInactive energy  (J) for device %d sled %d: %f\n", 
	      prefix, set[i], j, dev->sled[j].stat.inactive_energy_j);
    }
    fprintf(outputfile, "\n");
  }
}


static void
mems_buffer_printstats (int *set, int setsize, char *prefix)
{
  int i;
  mems_t *dev;
  int total_accesses = 0;
  int total_hits     = 0;
  double hit_ratio;

  /* Overall statistics */

  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    total_accesses += dev->stat.num_buffer_accesses;
    total_hits += dev->stat.num_buffer_hits;
  }
  hit_ratio = (double)total_hits / (double)total_accesses;

  fprintf(outputfile, "%sNumber of buffer accesses: %d\n",
	  prefix, total_accesses);
  fprintf(outputfile, "%sNumber of hits:            %d\n",
	  prefix, total_hits);
  fprintf(outputfile, "%sBuffer hit ratio:          %f\n",
	  prefix, hit_ratio);
  fprintf(outputfile, "%sBuffer miss ratio:         %f\n", 
	  prefix, 1.0 - hit_ratio);
  fprintf(outputfile, "\n");

  /* Per-device statistics */

  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);

    if (dev->stat.num_buffer_accesses) {
      hit_ratio = (double)dev->stat.num_buffer_hits / 
	(double)dev->stat.num_buffer_accesses;
    fprintf(outputfile, "%sNumber of buffer accesses for device %d: %d\n",
	    prefix, i, dev->stat.num_buffer_accesses);
    fprintf(outputfile, "%sNumber of buffer hits for device %d:     %d\n",
	    prefix, i, dev->stat.num_buffer_hits);
    fprintf(outputfile, "%sBuffer hit ratio for device %d:          %f\n",
	    prefix, i, hit_ratio);
    fprintf(outputfile, "%sBuffer miss ratio for device %d:         %f\n",
	    prefix, i, 1.0 - hit_ratio);
    fprintf(outputfile, "\n");
    } else {
      fprintf(outputfile, "%sNo buffer accesses for device %d\n",
	      prefix, i);
    }
  }
}


static void
mems_inactive_printstats (int *set, int setsize, char *prefix)
{
  int i;
  int j;
  statgen *statset[MAXDEVICES];
  mems_t *dev;

  /************************************************************************
   * num_spinups
   ************************************************************************/

  {
    int total_spinups = 0;
    for (i=0; i < setsize; i++) {
      dev = getmems(set[i]);
      total_spinups += dev->stat.num_spinups;
    }
    fprintf(outputfile, "%sNumber of spinups: %d\n\n", 
	    prefix, total_spinups);
  }

  /* Spinups for each device and sled */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    fprintf(outputfile, "%sNumber of spinups for device %d: %d\n",
	    prefix, set[i], dev->stat.num_spinups);
    for (j=0; j < dev->num_sleds; j++) {
      fprintf(outputfile, "%sNumber of spinups for device %d sled %d: %d\n",
	      prefix, set[i], j, dev->sled[j].stat.num_spinups);
    }
    fprintf(outputfile, "\n");
  }

  /************************************************************************
   * num_spindowns
   ************************************************************************/

  {
    int total_spindowns = 0;
    for (i=0; i < setsize; i++) {
      dev = getmems(set[i]);
      total_spindowns += dev->stat.num_spindowns;
    }
    fprintf(outputfile, "%sNumber of spindowns: %d\n\n", 
	    prefix, total_spindowns);
  }

  /* Spindowns for each device and sled */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    fprintf(outputfile, "%sNumber of spindowns for device %d: %d\n",
	    prefix, set[i], dev->stat.num_spindowns);
    for (j=0; j < dev->num_sleds; j++) {
      fprintf(outputfile, "%sNumber of spindowns for device %d sled %d: %d\n",
	      prefix, set[i], j, dev->sled[j].stat.num_spindowns);
    }
    fprintf(outputfile, "\n");
  }

  /************************************************************************
   * inactive_time
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.inactive_time;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.inactive_time;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.inactive_time;
      stat_print_set(statset, 1, newprefix);
    }
  }
}


static void
mems_other_printstats (int *set, int setsize, char *prefix)
{
  int i;
  int j;
  statgen *statset[MAXDEVICES];
  mems_t *dev;

  /************************************************************************
   * subtrack_accesses
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.subtrack_accesses;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.subtrack_accesses;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.subtrack_accesses;
      stat_print_set(statset, 1, newprefix);
    }
  }

  /************************************************************************
   * tips_per_access
   ************************************************************************/
  
  /* Overall statistics */
  for (i=0; i < setsize; i++) {
    dev = getmems(set[i]);
    statset[i] = &dev->stat.tips_per_access;
  }
  stat_print_set(statset, setsize, prefix);

  /* Per-device and per-sled statistics */
  for (i=0; i < setsize; i++) {
    char newprefix[80];
    dev = getmems(set[i]);

    sprintf(newprefix, "%sdevice %d ", prefix, i);
    statset[0] = &dev->stat.tips_per_access;
    stat_print_set(statset, 1, newprefix);

    for (j=0; j < dev->num_sleds; j++) {
      sprintf(newprefix, "%sdevice %d sled %d ", prefix, i, j);
      statset[0] = &dev->sled[j].stat.tips_per_access;
      stat_print_set(statset, 1, newprefix);
    }
  }
}


/* Print set statistics */
void
mems_printsetstats (int *set, int setsize, char *sourcestr)
{
  int i;
  struct ioq *queueset[MAXDEVICES];
  int reqcnt = 0;
  char prefix[80];

  sprintf(prefix, "%smems ", sourcestr);
  for (i=0; i<setsize; i++) {
    mems_t *dev = getmems(set[i]);
    queueset[i] = dev->queue;
    reqcnt += ioqueue_get_number_of_requests(dev->queue);
  }
  if (reqcnt == 0) {
    fprintf(outputfile, "\nNo mems requests for members of this set\n\n");
    return;
  }
  ioqueue_printstats(queueset, setsize, prefix);

  mems_prefetch_printstats(set, setsize, prefix);
  mems_seektime_printstats(set, setsize, prefix);
  mems_other_printstats(set, setsize, prefix);
  mems_inactive_printstats(set, setsize, prefix);
  mems_buffer_printstats(set, setsize, prefix);
  mems_energy_printstats(set, setsize, prefix);
}


/* Print statistics */
void
mems_printstats (void)
{
  struct ioq *queueset[MAXDEVICES];
  int set[MAXDEVICES];
  int i;
  int reqcnt = 0;
  char prefix[80];
  int devcnt;

  mems_t *dev;

  fprintf(outputfile, "\nMEMS STATISTICS\n----------------\n\n");
  sprintf(prefix, "Mems ");

  if (!disksim->memsinfo) {
    fprintf(outputfile, "No mems requests encountered (1)\n");
    return;
  }    

  devcnt = 0;
  for (i = 0; i < disksim->memsinfo->devices_len; i++) {
    dev = getmems(i);
    if (dev) {
      // SCHLOS - this has been hard-coded to the first sled since
      //  mems_event doesn't use the device queues anymore.
      queueset[devcnt] = dev->sled[0].queue;
      reqcnt += ioqueue_get_number_of_requests(dev->queue);
      devcnt++;
    }
  }
  assert(devcnt == disksim->memsinfo->numdevices);

  /*
  if (reqcnt == 0) {
    fprintf(outputfile, "No mems requests encountered (2)\n");
    return;
  }
  */

  ioqueue_printstats(queueset, devcnt, prefix);

  devcnt = 0;
  for (i=0; i < disksim->memsinfo->devices_len; i++) {
    dev = getmems(i);
    if (dev) {
      set[devcnt] = i;
      devcnt++;
    }
  }
  assert(devcnt == disksim->memsinfo->numdevices);

  mems_prefetch_printstats(set, devcnt, prefix);
  mems_seektime_printstats(set, devcnt, prefix);
  mems_other_printstats(set, devcnt, prefix);
  mems_inactive_printstats(set, devcnt, prefix);
  mems_buffer_printstats(set, devcnt, prefix);
  mems_energy_printstats(set, devcnt, prefix);
  fprintf(outputfile, "\n\n");
}


/************************************************************************
 * Initialization foo
 ************************************************************************/


struct mems *memsmodel_mems_loadparams(struct lp_block *b,
				       int *numptr)
{
  int c;
  int num;
  struct mems *result;
  mems_sled_t tmpsled;

  device_initialize_deviceinfo();

  bzero(&tmpsled, sizeof(tmpsled));

  result = (struct mems *)calloc(1, sizeof(mems_t));
  if(!result) return 0;

  ((struct device_header *)result)->device_type = DEVICETYPE_MEMS;

  /* Initialize disksim's memsinfo structures */
  if (disksim->memsinfo == NULL) {
    disksim->memsinfo = (struct mems_info *)calloc(1, sizeof(struct mems_info));
    ASSERT(disksim->memsinfo != NULL);

/*      disksim->memsinfo->devices = malloc(MAXDEVICES * sizeof(mems_t)); */
/*      ASSERT(disksim->memsinfo->devices != NULL); */

  }

  for(c = 0; c < disksim->memsinfo->devices_len; c++) {
    if(!disksim->memsinfo->devices[c]) {
      break;
    }
  }

  if(c == disksim->memsinfo->devices_len) {
    int newlen = disksim->memsinfo->devices_len ?
      disksim->memsinfo->devices_len * 2 : 2;
    int zerolen = newlen == 2 ? 2 : newlen / 2;
     
    disksim->memsinfo->devices = 
      (mems_t **)realloc(disksim->memsinfo->devices, 
	      newlen * sizeof(mems_t *));

    bzero(&(disksim->memsinfo->devices[c]), zerolen * sizeof(mems_t *));
    disksim->memsinfo->devices_len = newlen;
  }
  
  disksim->memsinfo->devices[c] = result;
  num = c;
  if(numptr) *numptr = num;
  result->hdr = mems_hdr_initializer;
  result->hdr.device_name = strdup(b->name);
  
  disksim->memsinfo->numdevices++;

    
  //#include "modules/disksim_mems_param.c"
  lp_loadparams(result, b, &memsmodel_mems_mod);

  for (c = 0; c < MAXINBUSES; c++) {
    result->inbuses[c] = -1;
    result->depth[c]   = -1;
    result->slotno[c]  = -1;
  }


  for(c = 0; c < result->num_sleds; c++) {
    memcpy(&result->sled[c], &result->sled[0], result->num_sleds * sizeof(mems_sled_t));
    result->sled[c].queue = ioqueue_copy(result->queue);
  }

  
  device_add((struct device_header *)result, num);
  return result;
}

mems_t *mems_copy(mems_t *src) {
  mems_t *result = (mems_t *)calloc(1, sizeof(mems_t));
  memcpy(result, src, sizeof(mems_t));
  result->queue = ioqueue_copy(src->queue);
  memcpy(&result->sled, src->sled, src->num_sleds * sizeof(mems_sled_t));
  return result;
}




/* Initialize device structures */
void
mems_initialize (void)
{
  int i;
  int j;
  int k;
  mems_t *dev;

  if (disksim->memsinfo == NULL) return;

  ioqueue_setcallbacks();

  /* Initialize all devices that have been inited (huh?) */
  for (i = 0; i < disksim->memsinfo->devices_len; i++) {
    dev = getmems(i);
    if (dev) {
      dev->busowned = -1;
      addlisttoextraq((event **)&dev->buswait);
      ioqueue_initialize(dev->queue, i);
      dev->dataxfer_req = NULL;
      dev->dataxfer_queue = NULL;

      /* Segment initialization stuff taken in part from disksim_disk.c */
      if ((dev->numsegs > 0) && (dev->seglist == NULL)) {
	dev->seglist = (struct mems_segment *)DISKSIM_malloc(sizeof(struct mems_segment));
	dev->seglist->next = NULL;
	dev->seglist->prev = NULL;
	dev->seglist->time = 0.0;
	dev->seglist->startblkno = 0;
	dev->seglist->endblkno   = 0;
	for (k = 1; k < dev->numsegs; k++) {
	  struct mems_segment *tmpseg = (struct mems_segment *)DISKSIM_malloc(sizeof(struct mems_segment));
	  tmpseg->next = dev->seglist;
	  dev->seglist = tmpseg;
	  tmpseg->next->prev = tmpseg;
	  tmpseg->prev = NULL;
	  tmpseg->startblkno = 0;
	  tmpseg->endblkno   = 0;
	}
      }
      if (dev->numsegs == 0) addlisttoextraq((event **)&dev->seglist);

      mems_check_numblocks(dev);

      for (j = 0; j < dev->num_sleds; j++) {
	dev->sled[j].dev = dev;

	ioqueue_initialize(dev->sled[j].queue, i);

	dev->sled[j].active = MEMS_SLED_INACTIVE;
	dev->sled[j].active_request = NULL;
	dev->sled[j].prefetch_info  = NULL;
	dev->sled[j].lastreq_lbn = mems_centermost_lbn(&dev->sled[j]);
	dev->sled[j].lastreq_comptime = 0.0;

	/* Initialize sled position, velocity at centermost LBN */
	mems_lbn_to_position(dev->sled[j].lastreq_lbn, &dev->sled[j], &dev->sled[j].coordset_up, &dev->sled[j].coordset_dn, &dev->sled[j].tipset, NULL, NULL, NULL);
	mems_coord_t_copy(&dev->sled[j].coordset_up.servo_start, &dev->sled[j].pos);

      }
      mems_statinit(i, TRUE);
      if (dev->precompute_seek_count > 0) {
	mems_precompute_seek_curve(dev);
      }
    }
  }
}


void mems_unimplemented(void) {
  fprintf(stderr, "*** error: mems method unsupported\n");
  assert(0);
}


extern void mems_event_arrive (ioreq_event *curr);
extern void mems_bus_delay_complete (int devno, ioreq_event *curr, int sentbusno);
extern void mems_bus_ownership_grant (int devno, ioreq_event *curr, int busno, double arbdelay);


/* default mems dev header */
struct device_header mems_hdr_initializer = { 
  DEVICETYPE_MEMS,
  sizeof(struct mems),
  "unnamed mems",
  (void *)mems_copy,
  mems_set_depth,
  mems_get_depth,
  mems_get_inbus,
  mems_get_busno,
  mems_get_slotno,
  mems_get_number_of_blocks,
  mems_get_maxoutstanding,
  mems_get_numcyls,
  mems_get_blktranstime,
  mems_get_avg_sectpercyl,
  mems_get_mapping,
  mems_event_arrive,
  mems_get_distance,
  mems_get_servtime,
  mems_get_seektime,
  (void *)mems_unimplemented,
  mems_bus_delay_complete,
  mems_bus_ownership_grant
};

// End of file mems_disksim.c

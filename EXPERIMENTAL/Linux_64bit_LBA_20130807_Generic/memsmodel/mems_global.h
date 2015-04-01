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


#ifndef _MEMS_GLOBAL_H_
#define _MEMS_GLOBAL_H_

//#include "disksim_global.h"
//#include "disksim_iosim.h"	/* Provides MAXINBUSES */
#include "disksim_device.h"
#include "disksim_stat.h"

#define MEMS_MAXSLEDS		9
#define MEMS_SEEKCACHE		2	/* Number of entries in seekcache */

#define getmems(d) (disksim->memsinfo->devices[d])

#define MEMS_SLED_INACTIVE     0       /*  sled states  */
#define MEMS_SLED_IDLE         1
#define MEMS_SLED_ACTIVE       2

#define MEMS_SEEK_PIECEWISE    0
#define MEMS_SEEK_HONG         1

typedef struct coord {
  int x_pos;  /*  bit position offsets  */
  int y_pos;
  int y_vel;
} coord_t;

typedef struct tipsector_coord_set {
  coord_t servo_start;
  coord_t tipsector_start;
  coord_t tipsector_end;
} tipsector_coord_set_t;

typedef struct tipset {
  int tip_start;
  int tip_end;
  int num_tips;
} tipset_t;


/* Informational structure for prefetching support */
struct mems_prefetch_info {
  int firstblock;
  int lastblock;

  int completed_block;
  int next_block_start;
  int next_block_end;
};


/* Marshalling structure for mems_seek_time_checkcache() */
struct mems_seekcache {
  double  time;		/* total time returned by mems_seek_time() */
  coord_t begin;	/* begin coordinate */
  coord_t end;		/* end coordinate */
  double  x_seek_time;		/* stat returned for X seek time */
  double  y_seek_time;		/* stat returned for Y seek time */
  double  turnaround_time;	/* stat returned for turnaround time */
  int     turnaround_number;	/* stat returned for num turnarounds */
};


struct mems_segment {
  double time;
  struct mems_segment *next;
  struct mems_segment *prev;
  int    startblkno;
  int    endblkno;
};


struct mems_stat {
  statgen request_energy_uj;	/* Average energy dissipated per request */
  double  total_energy_j;	/* Total sled dissipated energy over run */
  double  servicing_energy_j;	/* Energy expended servicing requests */
  double  startup_energy_j;	/* Energy expended while transitioning */
  double  idle_energy_j;	/* Energy expended while idle */
  double  inactive_energy_j;	/* Energy expended while inactive */

  statgen seek_time;		/* Initial positioning time */
  statgen x_seek_time;		/* Initial positioning time in X */
  statgen y_seek_time;		/* Initial positioning time in Y */

  int num_initial_turnarounds;  /* Number of initial turnarounds */

  statgen turnaround_time;	/* Initial turnaround time to position */      
  statgen turnaround_number;	/* Initial number of to's to position */

  int num_stream_turnarounds;   /* Number of streaming turnarounds  */

  statgen stream_turnaround_time; /* "Streaming" turnaround time */
  statgen stream_turnaround_number; /* "Streaming turnaround number */

  statgen subtrack_accesses;	/* Sectors accessed per request */
  statgen tips_per_access;	/* Average number of tips per sector access */

  int num_spinups;		/* Number of times sled spins up */
  int num_spindowns;		/* Number of times sled spins down */
  statgen inactive_time;	/* Time spent inactive before spinup */

  int num_buffer_accesses;	/* Number of times buffer is checked */
  int num_buffer_hits;		/* Number of hits in the buffer */

  statgen prefetched_blocks;	/* Number of blocks prefetched each request */
  statgen batch_response_time;  /* Response time for batches */

};


typedef struct {
  struct mems *dev;	/* Sled is component of this mems device */

  int active;		/* true if sled is active (e.g., not sleeping) */
  int numblocks;      	/* number of blocks per sled */

  struct mems_stat stat;	/* Statistics gathering */

  struct ioq 	*queue;
  ioreq_event 	*active_request;	/* Req currently being processed */
  struct mems_prefetch_info *prefetch_info;	/* Info on prefetching */
  int 		prefetch_depth;		/* Blocks to prefetch per req */

  double lastreq_comptime;	/* Time the last request completed	*/
  double inactive_delay_ms;	/* Idle delay before sled goes inactive	*/
  double startup_time_ms;	/* Inactive-to-active (startup) time	*/
  int lastreq_lbn;	/* blkno of last completed request */

  coord_t pos;		/* last known valid position of sled */
  tipsector_coord_set_t coordset;
  tipsector_coord_set_t coordset_up;
  tipsector_coord_set_t coordset_dn;
  struct tipset         tipset;

  int layout_policy;	/* How data are divided among multiple sleds */
  int x_length_nm;	/* Sled mobility in X */
  int y_length_nm;	/* Sled mobility in Y */

  int bit_length_nm;	/* length of side of the bit cell */
  int tip_sector_length_bits;	/* Length of tipsector */
  int servo_burst_length_bits;	/* Length of pre-tipsector servo burst */
  int tip_sectors_per_lbn;	/* Each sector striped across how many tips */

  int tips_usable;		/* number of usable (good) tips */
  int tips_simultaneous;	/* number of simultaneously active tips */
  int    bidirectional_access;	/* =1 if bidirectional OK */
  double x_accel_nm_s2;         /* available acceleration in X direction */
  double y_accel_nm_s2;		/* available acceleration in Y direction */

  int    y_access_speed_bit_s;	/* max read/write speed of sled */
  int     sled_resonant_freq_hz;/* sled resonant frequency */
  double num_time_constants;	/* number of time constants for settling */
  double spring_factor; /* percentage of actuator acceleration the springs 
			 * provide at the full throw of the media.  see 
			 * memsdevice_mapping.c  */

  /* Variables used by mems_seek_time_seekcache */
  struct mems_seekcache seekcache[MEMS_SEEKCACHE];
  int seekcache_next;		/* Next available cache entry */

  double active_power_mw;	/* Per-sled power when sled active */
  double inactive_power_mw;	/* Per-sled power when sled inactive */
  double tip_power_mw;		/* Per-tip access power */
} mems_sled_t;


typedef struct mems {
  struct device_header hdr;
  int    devno;		/* Disksim internal device number 		*/
  int    inited;	/* Set when disksim initialize routine called 	*/

  struct mems_stat stat;	/* Statistics gathering */

  double overhead;	/* Per-request device overhead time		*/
  double blktranstime;	/* Minimum 1-sector transfer time 		*/
  int    printstats;	/* Print statistics for this device?		*/
  int    maxqlen;	/* Maximum internal request queue length 	*/
  int    numblocks;	/* Size of device (in 512 byte blocks) 		*/

  int    depth[MAXINBUSES];	/* How far down the bus tree is device 	*/
  int    inbuses[MAXINBUSES];	/* Which inbuses connect to device 	*/
  int    slotno[MAXINBUSES];	/* To which bus slot device connects 	*/
  int    numinbuses;		/* How many inbuses connect to device 	*/

  int         busowned;       	/* Bus number owned by device */
  ioreq_event *buswait;		/* Sleds waiting on bus access */
  struct ioq  *queue;
  ioreq_event *dataxfer_req;	/* Request currently owning bus for xfer */
  ioreq_event *dataxfer_queue;	/* Requests waiting to transfer data */

  mems_sled_t *sled;
  int num_sleds;

  int numsegs;			/* Number of buffer segments */
  int segsize;			/* Segment size (in blks) */
  struct mems_segment *seglist;	/* Buffer segments */

  int seek_function;

  int precompute_seek_count;    /* Number of points in the pre-computed seek curve */
  int *precompute_seek_distances;  /* Seek distances in precomputed seek curve */
  double *precompute_x_seek_times; /* X seek times in precomputed seek curve */
  double *precompute_y_seek_times; /* Y seek times in precomputed seek curve */

} mems_t;


struct mems_info {
  mems_t **devices;	/* Array of all mems devices in simulator */
  int numdevices;	/* Count of above devices */
  int devices_len;      /* allocated size of devices */
};

typedef struct mems_x {
  int firstblock;
  int lastblock;

  tipsector_coord_set_t coordset_up;
  tipsector_coord_set_t coordset_dn;
  tipset_t tipset;
  int firstblock_cylinder, firstblock_surface, firstblock_block;

  tipsector_coord_set_t lastblock_up_coord_set;
  tipsector_coord_set_t lastblock_dn_coord_set;
  tipset_t lastblock_tipset;
  int lastblock_cylinder, lastblock_surface, lastblock_block;

  int completed_block_media;
  int completed_block_bus;
  int next_block_start;
  int next_block_end;

  int bus_done;
  int media_done;

  struct mems_x *next;
  struct mems_x *prev;
  ioreq_event *request;
} mems_extent_t;

typedef struct {
  int firstblock;	/* First block of request (curr->blkno) */
  int lastblock;	/* Last block of request (blkno + bcount - 1) */
  
  double batch_arrival_time;
  
  mems_extent_t *extents;
  int num_extents;
  
  mems_extent_t *bus_extent;

  int bus_pending;	/* TRUE when request on dataxfer_queue */
  int bus_done;		/* TRUE when busxfer done */
  int media_done;	/* TRUE when media access done */

  int completed_block_media;	/* last block completed by media subsystem */
  int completed_block_bus;	/* last block completed by bus subsystem */
  int next_block_start;
  int next_block_end;

  /* Statistics support */
  double request_energy_uj;	/* Energy consumed by this request */
  int    firstseek;		/* TRUE when first seek hasn't happened yet */
  int    subtrack_access_num;	/* Number of subtrack accesses */
  int    subtrack_access_tips;	/* Sum of all tips used during accesses */
} mems_reqinfo_t;

struct mems* memsmodel_mems_loadparams(struct lp_block* b, int* numptr);

#endif

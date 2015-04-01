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



/*
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
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

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */

#ifndef DISKSIM_DISK_H
#define DISKSIM_DISK_H

using namespace std;

#include <map>


#include "disksim_stat.h"
#include "disksim_iosim.h"
#include "disksim_ioqueue.h"
#include "config.h"

#include <diskmodel/dm.h>

// Disk states
typedef enum {
  DISK_IDLE                = 1,
  DISK_TRANSFERING         = 2,
  DISK_WAIT_FOR_CONTROLLER = 3
} disk_state_t;

// Disk buffer states -- status of the data in the cache
typedef enum {
  BUFFER_EMPTY             = 1,  // buffer empty - segment descriptor available
  BUFFER_CLEAN             = 2,  // buffer data same as data on disk
  BUFFER_DIRTY             = 3,  // buffer data is modified and not written to disk
  BUFFER_READING           = 4,  // buffer activity is READING
  BUFFER_WRITING           = 5,  // buffer activity is WRITING
} disk_buffer_state_t;

// bus xfer to/from seg
typedef enum {
  BUFFER_PREEMPT           = 6,
  BUFFER_IDLE              = 7,
  BUFFER_CONTACTING        = 8,
  BUFFER_TRANSFERING       = 9
} disk_buffer_outstate_t;

// Disk buffer content match types 
typedef enum {
  BUFFER_COLLISION         = -1,
  BUFFER_NOMATCH           = 0,
  BUFFER_WHOLE             = 1,    // read or write
  BUFFER_PARTIAL           = 2,    // read
  BUFFER_PREPEND           = 3,    // write - buffer partial front
  BUFFER_APPEND            = 4,    // write - buffer partial end
  BUFFER_OVERLAP           = 5     // write - data fits within segment
} disk_cache_hit_t;


/* Disk buffer continuous read types */
typedef enum {
  BUFFER_NO_READ_AHEAD          = 0,
  BUFFER_READ_UNTIL_TRACK_END   = 1,
  BUFFER_READ_UNTIL_CYL_END     = 2,
  BUFFER_READ_UNTIL_SEG_END     = 3,
  BUFFER_DEC_PREFETCH_SCHEME    = 4
} disk_cont_read_t;

/* Disk request flags */
#define SEG_OWNED                       0x00000001      // has cache segment attached
#define HDA_OWNED                       0x00000002      // has HDA attached
#define EXTRA_WRITE_DISCONNECT          0x00000004
#define COMPLETION_SENT                 0x00000008
#define COMPLETION_RECEIVED             0x00000010
#define FINAL_WRITE_RECONNECTION_1      0x00000020
#define FINAL_WRITE_RECONNECTION_2      0x00000040


/* Disk preseeking levels */
typedef enum {
  NO_PRESEEK                     = 0,
  PRESEEK_DURING_COMPLETION      = 1,
  PRESEEK_BEFORE_COMPLETION      = 2        /* implies 1 as well */
} disk_preseek_t;

/* Disk fastwrites levels OBSOLETE */
typedef enum {
  NO_FASTWRITE          = 0,
  LIMITED_FASTWRITE     = 1,
  FULL_FASTWRITE        = 2
} disk_fastwrite_t;


/* Disk write cache enable levels */
typedef enum {
  WCD     = 0,
  WCE     = 1
} disk_write_cache_enable_t;

/* aliases */
#define ioreq_hold_disk tempptr1
#define ioreq_hold_diskreq tempptr2

/* free blocks added may change later */
#define MAXSTATS 10
#define MAXDISKS 100


typedef struct seg {
   double       time;
   disk_buffer_state_t state;
   struct seg  *next;
   struct seg  *prev;
   int         startblkno;
   int         endblkno;
   disk_buffer_outstate_t   outstate;
   int         outbcount;
   int         minreadaheadblkno;      /* min prefetch blkno + 1 */
   int         maxreadaheadblkno;      /* max prefetch blkno + 1 */

   struct diskreq_t *diskreqlist;       /* sorted by ascending blkno */
   int               size;
                                        /* diskreqlist normally starts
                                         * as a single request, more
                                         * can be added by read hits
                                         * on the segment and write combining */

   ioreq_event *access;                 /* copy of the active ioreq using this seg -rcohen */
   int               hold_blkno;        /* used for prepending */
   int               hold_bcount;       /* sequential writes   */
   struct diskreq_t *recyclereq;        /* diskreq to recycle this seg */
   struct dm_mech_acctimes mech_acctimes; // storage for RPO calculation
} segment;

typedef std::map <double, segment *> CACHE_LBA_MAP;


typedef struct diskreq_t {
   int                 flags;
   ioreq_event         *ioreqlist;  /* sorted by ascending blkno */
   struct diskreq_t    *seg_next;   /* list attached to a segment */
   struct diskreq_t    *bus_next;
   int                 outblkno;
   int                 inblkno;
   segment             *seg;       /* associated cache segment */
   int                 watermark;  // used by read and write
   disk_cache_hit_t    hittype;    /* for cache use */
   double              overhead_done;
   char                space[20];
} diskreq;


typedef struct {
   double  seektime;
   double  latency;
   double  xfertime;
   int     seekdistance;
   int     zeroseeks;
   int     zerolatency;
   int     highblkno;
   statgen seekdiststats;
   statgen seektimestats;
   statgen rotlatstats;
   statgen xfertimestats;
   statgen postimestats;
   statgen acctimestats;
   int     writecombs;
   int     readmisses;
   int     writemisses;
   int     fullreadhits;
   int     appendhits;
   int     prependhits;
   int     readinghits;
   double  runreadingsize;
   double  remreadingsize;
   int     parthits;
   double  runpartsize;
   double  rempartsize;
   int     interfere[2];
   double  requestedbus;
   double  waitingforbus;
   int     numbuswaits;
} diskstat;


// replaces currangle/currtime/... foo
struct disk_currstate {
  struct dm_mech_state s;
  dm_time_t t;
};

typedef struct disk {
  struct device_header hdr;
  struct dm_disk_if    *model;
  struct dm_mech_state mech_state;

  int track_low, track_high; // track boundaries for the current track

  int	               devno;

//****************************************************************
// Used by SIMPLE_DISK_MODEL
  // if true, uses constant access-time model,
  // bypasses some of mech model
  int       const_acctime; 
  // constant access time; only meaningful if const_acctime is true
  double          acctime;
//****************************************************************


  // if true, uses constant seek-time model,
  // bypasses some of mech model
  int       const_seektime;
  // constant seek time; only meaningful if const_seektime set
  double          seektime;

  // "Per-request overhead time"
  double       overhead;
  double       timescale;
  
  // which syncset we're in; only really used to sync up disks in
  // syncsets once at beginning of simulation
  int          syncset;

  // list of io's not completed 
  struct ioq *queue;	       
  // "Avg" value used in suboptimal schedulers 
  int	       sectpercyl;    

  // controller stuff
  int		hold_bus_for_whole_read_xfer;
  int		hold_bus_for_whole_write_xfer;

  // cache stuff
  int		almostreadhits;
  int		sneakyfullreadhits;
  int		sneakypartialreadhits;
  int		sneakyintermediatereadhits;
  int		readhitsonwritedata;
  int		writeprebuffering;

  // diskctlr
  int		preseeking;
  // ctlr/cache
  int		neverdisconnect;

  // doesn't appear to be used
  // int           qlen;
  int           maxqlen;

  int           busy;

  // cache/controller 
  int		prev_readahead_min;
  int		write_hit_stop_readahead;
  int		read_direct_to_buffer;
  int		immedtrans_any_readhit;
  int		readanyfreeblocks;

  int		numsegs;
  int		numwritesegs;
  segment *	dedicatedwriteseg;
  int		segsize;
  double	writelowwatermark;
  double	readhighwatermark;
  int		reqwater;           // Set watermark by reqsize flag (from parameter file)
  int		sectbysect;
  int		enablecache;
  int       contread;           // Buffer Continuous Read (from parameter file)
  int		minreadahead;
  int		maxreadahead;
  int		keeprequestdata;
  int		readaheadifidle; // obsolete
  int		fastwrites; // obsolete
  int       writeCacheEnable;
  int		numdirty;

  // "Combine seq writes"
  int		writecomb;

  // "Stop prefetch in sector"
  int		stopinsector;

  // "Disconnect write if seek"
  int		disconnectinseek;

  // ctlr
  int		immedstart;
  int		immedend;

  // list of all the cache segments(?)
  segment      *seglist;

  // overheads -- mostly in controller code
  double	overhead_command_readhit_afterread;
  double	overhead_command_readhit_afterwrite;

  double	overhead_command_readmiss_afterread;
  double	overhead_command_readmiss_afterwrite;

  double	overhead_command_writehit_afterread;
  double	overhead_command_writehit_afterwrite;

  double	overhead_command_writemiss_afterread;
  double	overhead_command_writemiss_afterwrite;

  double	overhead_complete_read;
  double	overhead_complete_write;

  double	overhead_data_prep;
  double	overhead_reselect_first;
  double	overhead_reselect_other;

  double	overhead_disconnect_read_afterread;
  double	overhead_disconnect_read_afterwrite;

  double	overhead_disconnect_write;


  // "extra" overhead/delay/latency ... cache/ctlr
  struct diskreq_t *extradisc_diskreq;
  int		extra_write_disconnect;
  double	extradisc_command;
  double	extradisc_disconnect1;
  double	extradisc_inter_disconnect;
  double	extradisc_disconnect2;
  double	extradisc_seekdelta;

  double	minimum_seek_delay;

  // looks like more globals
  //  int		firstblkontrack;
  //  int		endoftrack;

  // passes into global_currtime, involved in global_currangle computation
  double       currtime;
  dm_time_t    currtime_i;

  // only appears in cache code -- disk_interferestats()
  int          lastgen;

  // flags on previous request to current (?)
  int	       lastflags;

  // time at which some transaction was started; only appears in 
  // controller code
  double       starttrans;

  // "Bulk sector transfer time"
  double       blktranstime;

  disk_state_t outstate;   // current disk state

  // cache/controller
  diskreq     *pendxfer;
  diskreq     *currenthda;
  diskreq     *effectivehda;
  diskreq     *currentbus;
  diskreq     *effectivebus;

  // number of blocks transferred for this request (?)
  // only appears in controller code
  int	       blksdone;

  int	       busowned;    // set to the bus number connected to this disk
  ioreq_event *buswait;
  ioreq_event *outwait;

  int          numinbuses;
  int          inbuses[MAXINBUSES];
  int          depth[MAXINBUSES];
  int          slotno[MAXINBUSES];


  diskstat     stat;
  int          printstats;


  // zero-latency parameters
  int immed;
  int immedread;
  int immedwrite;

  
  // This is a forward progress check.  There have been a number of
  // bugs over the years that cause the controller to get into an
  // infinite loop, reading the same sector or track over and over.
  // Every time we do a reposition, this is reset (currently
  // statically, should be set to the track length) and
  // disk_buffer_sector_done() decrements it and asserts that it
  // hasn't gotten to 0.
  // int fpcheck;

  double    writeCacheFlushIdleDelay;
  double    writeCacheFlushPeriod;
} disk;


typedef struct disk_info {
   disk **disks;
   int disks_len; /* allocated size of disks */
   int numdisks;
   int disk_printhack;   /* disk_printhack = 0, 1, or 2 */
   double disk_printhacktime;
   int numsyncsets;
   int extra_write_disconnects;
/* From disksim_diskctlr.c */
   int bandstart;
   int swap_forward_only;
   double addtolatency;
   double disk_seek_stoptime;
  disk *lastdisk;
/* From disksim_diskcache.c */
   int LRU_at_seg_list_head;
/* "globals" are used instead of locals. values may be set and passed */
/* do NOT assume these have useful values at any point in time        */
/* unless they have just been set                                     */
  //   int global_currcylno;
  //   int global_currsurface;
  //   double global_currtime;
  //   double global_currangle;
/* *ESTIMATED* command processing overheads for buffer cache hits.  These */
/* values are not actually used for determining request service times.    */
   double buffer_partial_servtime;
   double reading_buffer_partial_servtime;
   double buffer_whole_servtime;
   double reading_buffer_whole_servtime;
} disk_info_t;

/* one remapping #define for each variable in disk_info_t */
//#define disks                     (disksim->diskinfo->disks)
//#define multi_disk_last_seektime  (disksim->diskinfo->multi_disk_last_seektime)
//#define multi_disk_last_latency   (disksim->diskinfo->multi_disk_last_latency)
//#define multi_disk_last_cylno     (disksim->diskinfo->multi_disk_last_cylno)
//#define multi_disk_last_surface   (disksim->diskinfo->multi_disk_last_surface)
//#define multi_disk_last_angle     (disksim->diskinfo->multi_disk_last_angle)
//#define multi_disk_last_xfertime  (disksim->diskinfo->multi_disk_last_xfertime)
//#define multi_disk_last           (disksim->diskinfo->multi_disk_last)
//#define do_free_blocks            (disksim->diskinfo->do_free_blocks)
//#define disk_last_angle           (disksim->diskinfo->disk_last_angle)
//#define lastdisk                  (disksim->diskinfo->lastdisk)
#define NUMDISKS                  (disksim->diskinfo->numdisks)
#define disk_printhack            (disksim->diskinfo->disk_printhack)
#define disk_printhacktime        (disksim->diskinfo->disk_printhacktime)
#define NUMSYNCSETS                 (disksim->diskinfo->numsyncsets)
#define EXTRA_WRITE_DISCONNECTS   (disksim->diskinfo->extra_write_disconnects)
//#define remapsector               (disksim->diskinfo->remapsector)
//#define bandstart                 (disksim->diskinfo->bandstart)
#define SWAP_FORWARD_ONLY         (disksim->diskinfo->swap_forward_only)
// #define trackstart                (disksim->diskinfo->trackstart)
#define ADDTOLATENCY              (disksim->diskinfo->addtolatency)
//#define global_seekdistance       (disksim->diskinfo->global_seekdistance)
#define DISK_SEEK_STOPTIME        (disksim->diskinfo->disk_seek_stoptime)
//#define disk_last_distance        (disksim->diskinfo->disk_last_distance)
//#define disk_last_seektime        (disksim->diskinfo->disk_last_seektime)
//#define disk_last_latency         (disksim->diskinfo->disk_last_latency)
//#define DISK_LAST_XFERTIME        (disksim->diskinfo->disk_last_xfertime)
//#define disk_last_acctime         (disksim->diskinfo->disk_last_acctime)
//#define DISK_LAST_CYLNO           (disksim->diskinfo->disk_last_cylno)
//#define disk_last_surface         (disksim->diskinfo->disk_last_surface)
#define LRU_AT_SEG_LIST_HEAD      (disksim->diskinfo->LRU_at_seg_list_head)
//#define global_currcylno          (disksim->diskinfo->global_currcylno)
//#define global_currsurface        (disksim->diskinfo->global_currsurface)
//#define global_currtime           (disksim->diskinfo->global_currtime)
//#define global_currangle          (disksim->diskinfo->global_currangle)
#define BUFFER_PARTIAL_SERVTIME         (disksim->diskinfo->buffer_partial_servtime)
#define READING_BUFFER_PARTIAL_SERVTIME (disksim->diskinfo->reading_buffer_partial_servtime)
#define BUFFER_WHOLE_SERVTIME           (disksim->diskinfo->buffer_whole_servtime)
#define READING_BUFFER_WHOLE_SERVTIME   (disksim->diskinfo->reading_buffer_whole_servtime)



/* 
 * disksim_diskctlr.c functions 
 */

int  disk_buffer_stopable_access (disk *currdisk, diskreq *currdiskreq);
int  disk_enablement_function (ioreq_event *);


/* 
 * disksim_diskcache.c functions 
 */

void dump_disk_buffer_seqment ( FILE *fp, segment *seg, int index );
void dump_disk_buffer_seqments ( FILE *fp, segment *seglist, const char *msg );
segment * disk_buffer_select_segment(disk *currdisk, diskreq *currdiskreq, int set_extradisc);

segment * disk_buffer_recyclable_segment(disk *currdisk, int isread);
diskreq * disk_buffer_seg_owner(segment *seg, int effective);
int  disk_buffer_attempt_seg_ownership(disk *currdisk, diskreq *currdiskreq);

int  disk_buffer_get_max_readahead(disk *currdisk, segment *seg, ioreq_event *curr);

int disk_buffer_block_available (disk *currdisk, segment *seg, int blkno);
int  disk_buffer_reusable_segment_check(disk *currdisk, segment *currseg);
int  disk_buffer_overlap(segment *seg, ioreq_event *curr);
int  disk_buffer_check_segments(disk *currdisk, ioreq_event *currioreq, int *buffer_reading);

void disk_buffer_set_segment(disk *currdisk, diskreq *currdiskreq);
void disk_buffer_segment_wrap(segment *seg, int endblkno);
void disk_buffer_remove_from_seg(diskreq *currdiskreq);
void disk_interferestats(disk *currdisk, ioreq_event *curr);


/* 
 * externalized disksim_disk*.c functions (should be here?) 
 */

INLINE struct disk *getdisk (int diskno);
struct disk *getdiskbyname(char *name);

void    disk_set_syncset (int setstart, int setend);

void    disk_setcallbacks(void);
void    disk_initialize(void);
void    disk_resetstats(void);
void    disk_printstats(void);
void    disk_printsetstats(int *set, int setsize, char *sourcestr);
void    disk_cleanstats(void);

int     disk_set_depth(int diskno, int inbusno, int depth, int slotno);
int     disk_get_depth(int diskno);
int     disk_get_inbus(int diskno);
INLINE int     disk_get_busno(ioreq_event *curr);
int     disk_get_slotno(int diskno);


int     disk_get_maxoutstanding(int diskno);

int     disk_get_numdisks(void);


double  disk_get_blktranstime(ioreq_event *curr);
int     disk_get_avg_sectpercyl(int devno);


/* No longer exported - bucy 20030227 */
/*  int     disk_get_number_of_blocks(int diskno); */
/*  int     disk_get_numcyls(int diskno); */
/*  void    disk_get_mapping(int maptype,  */
/*  			 int diskno,  */
/*  			 int blkno,  */
/*  			 int *cylptr,  */
/*  			 int *surfaceptr,  */
/*  			 int *blkptr); */


void    disk_event_arrive(ioreq_event *curr);

int     disk_get_distance(int diskno, 
			  ioreq_event *req, 
			  int exact, 
			  int direction);

double  disk_get_servtime(int diskno, 
			  ioreq_event *req, 
			  int checkcache, 
			  double maxtime);

double  disk_get_seektime(int diskno, 
			  ioreq_event *req, 
			  int checkcache, 
			  double maxtime);

double  disk_get_acctime(int diskno, 
			 ioreq_event *req, 
			 double maxtime);



void    disk_bus_delay_complete(int devno, 
				ioreq_event *curr, 
				int sentbusno);

void    disk_bus_ownership_grant(int devno, 
				 ioreq_event *curr, 
				 int busno, 
				 double arbdelay);

/* default disk dev header */
extern struct device_header disk_hdr_initializer;

int disk_load_syncsets(struct lp_block *b);


int disk_setup_mapping(struct disk *d);


void disk_acctimestats (disk *currdisk, int distance, double seektime,
		        double latency, double xfertime, double acctime);

int disk_load_syncsets(struct lp_block *b);

static void disk_read_arrive( disk *currdisk, ioreq_event *curr, diskreq *new_diskreq, segment *seg, ioreq_event *intrp);
static void disk_write_arrive(disk *currdisk, ioreq_event *curr, diskreq *new_diskreq, segment *seg, ioreq_event *intrp);
char * disk_buffer_decode_disk_cache_hit_t( disk_cache_hit_t hittype );
void dump_disk_buffer_seqments ( segment *seg, const char *msg );

// write cache flush routines
void disk_write_cache_periodic_flush (timer_event *timereq);
void disk_write_cache_idletime_detected (void *idleworkparam, int idledevno);
CACHE_LBA_MAP disk_buffer_sort_cache_segments_by_LBA( disk *currdisk );
double disk_calc_rpo_time( disk *currdisk, ioreq_event *curr, struct dm_mech_acctimes *mech_acctimes );

#endif   /* DISKSIM_DISK_H */


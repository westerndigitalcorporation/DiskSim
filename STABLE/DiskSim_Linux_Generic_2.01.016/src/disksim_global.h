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

#ifndef DISKSIM_GLOBAL_H
#define DISKSIM_GLOBAL_H

#include "disksim_rand48.h"
#include "disksim_malloc.h"
#include "disksim_bitstring.h"
#include "inline.h"

#include <sys/types.h>
#include <stdio.h>

#define LBA_SIZE_64_BIT 0    // 0 = 32 bit LBN's, 1 = 64 bit LBN's

#define VERSION "2.01.016"

//*********************************************************************************************
// 2.01.016 hurst_r
//     1) Reverted function disk_buffer_block_available
//     2) Removed WCE code change
// 2.01.015 hurst_r
//     1) WCD NULL segment fixes in function disk_buffer_select_write_segment:
//       a) NULL segment returned for second command if back to back writes to the same block address and range
//       b) NULL segment returned if read segment in-use and write command bumped up against the read block range.
//          i.e. Read command received on block 108 for 8 blocks and then write command received on block 100 for 8 blocks.
//     2) Added routines: disk_flush_write_cache() and disk_buffer_sort_dirty_cache_segments_by_RPO()
//       1) detect a cache full condition and search for DIRTY write segments, choose best segment for next disk operation
// 2.01.014 hurst_r
//     1) Merged most WCE changes
//     2) Additional debug statements to evaluate WCE feature
// 2.01.013 hurst_r
//     1) Fixed command logging - bcount value was incorrect when passed to disk_initiate_seek routine
//     2) Additional debug statements to evaluate WCE feature
// 2.01.012 hurst_r
//     1) Redesigned command tagging and logging for WCE logging
//      a) added data structure to log one more disk I/O within the command
//      b) command tagging and logging totally enclosed in disk controller
//      c) removed code from SynthIO, IOTrace, and IODriver so CacheMem and CacheDev now functions.
//     2) runvalid script - disabled HPL trace support
//     3) Additional debug statements to evaluate WCE feature
// 2.01.011 hurst_r
//     1) 64 bit OS ready
//     2) iotrace.c: disabled raw and hpl input formats.
//     3) Additional debug statements to evaluate post read and write cache algorithms
//     4) Added post read abort on new command in command queue
// 2.01.010 hurst_r
//     1) added three new parameters to the parametric file : diskBlocksXferred, block_first, block_last
//     2) Additional debug statements to evaluate post read and write cache algorithms
// 2.01.009 hurst_r
//     1) disksim_synthio: fixed tag commands. (caused segmentation faults)
//     2) disksim: commented out printf (caused segmentation fault).
// 2.01.008 hurst_r
//     1) Converted macro warmuptime to uppercase WARMUPTIME.
//     2) Added a unique tag ID to each command so it can be identified for adding new metrics to the command.
//     3) Added two new files, disksim_simresult.h and disksim_simresult.c, to manage DiskSim paramteric data.
//     4) log_diskio.txt: added diskctlr cache info to the file
// 2.01.007 hurst_r
//     1) Additional debug statements to evaluate IOQueue and diskctlr
//     2) Added code to produce an event log of the simulation run.
//     3) disksim_diskctlr.c - disk_write_arrive: Fixed FastWrite issue when writes were cached
//        The diskctlr state machine would failed to set it's next state causing it ot go idle.
//        This fix generates the next state allowing the state machine to finish the command.
//        Moved out of if/else statement:
//          disk_send_event_up_path(intrp, (delay * currdisk->timescale));
//          disk_activate_write(currdisk, new_diskreq, TRUE, TRUE);
// 2.01.006 hurst_r
//     1) Additional debug statements to evaluate IOQueue and diskctlr
// 2.01.005 hurst_r
//     1) Additional debug statements to evaluate IOQueue and diskctlr
// 2.01.004 hurst_r
//     1) Additional debug statements to evaluate IOQueue and diskctlr
// 2.01.003 hurst_r
//     1) Additional debug statements
// 2.01.002 hurst_r
//     1) Added statistics to outputfile for Event block usage.
// 2.01.001 hurst_r
//     1) disksim_synthio.c: moved the incrementing of the Synthio IOCount to the function
//         synthio_appendio. The function synthio_initialize_generator was being called to
//         initialize each generator and generate an initial time event. However, the IOCount
//         was not being incremented. Number of time events generated is now equal to the requested
//         number of time events.
// 2.01.000 hurst_r
//     1) Added additional debug statements in preparation for evaluating the ioqueue.
// 2.0 hurst_r
//     1) minor fixes to debug code.
// 1.9 hurst_r
//     2) Modified to always output read and write statistics even if those operations were not performed.
//        Allows for automated data collection.
//     3) Changes to reduce debugger issue with symbol ambiguities.
//        This is an ongoing process and changes are made when encountered.
//        Original authors defined macros with the same name as the variable.
//        Code:
//          #define numdisks                  (disksim->diskinfo->numdisks)
//        Changed to:
//          #define NUMDISKS                  (disksim->diskinfo->numdisks)
//     4) disk_buffer_estimate_seektime: removed another instance where a constant was added to a variable.
//          tmptime = maxtime + 1.0;   <- effects seek time calculation.
//     5) Added simulation start, end and elapsed time to the simulation output file.
//     6) disksim_sythio.c: Added debug code and additional comments
//     7) disksim.c: Added code to not output synthio and disk log if the file does not exist.
//        This allows the user to choose if synthio and/or disk data is collected.
//     Bug was copied from the cachemem and cachedev devices. Fixed by testing for flush delay > 0.0
// 1.7 hurst_r Fixed G4 layout
//     The author of the G4 layout code for Linux used a designated initializer list to initialize
//     the dm_layout_if data structure. The struct designated initializer list is supported by
//     but Microsoft C does not support them. The designated initializer list is not supported
//     by the C++ standard. The author of the Windows port used the same methods as in the G1
//     and G2 layout code but did not include all of the function place holders required to make
//     it work.
// 1.6 hurst_r Removed debug messages from statistics file
//*********************************************************************************************

// DEBUG conditionals
// Notes:
//   DEBUG_DISKCACHE_HIT and DEBUG_DISKCTLR_SEEK should give you a trace of everything in the Disk controller

#define DISK_QUEUE_LOG_ON   // logs entries into the Disk Controller IOQueue
#define DISKLOG_ON
//#define DEBUGLOG_ON

//#define DEBUG_ON
//#define DEBUG_EXTRAQ
//#define DEBUG_INTQ
//#define DEBUG_BUS
//#define DEBUG_CACHEMEM
//#define DEBUG_CONTROLLER
//#define DEBUG_CTLRDUMB
//#define DEBUG_CTLRSMART
//#define DEBUG_DISK
//#define DEBUG_DISKCACHE
//#define DEBUG_DISKCTLR
//#define DEBUG_INTERFACE
//#define DEBUG_IODRIVER
//#define DEBUG_IOQUEUE
//#define DEBUG_IOSIM
//#define DEBUG_PFSIM
//#define DEBUG_SIMPLEDISK
//#define DEBUG_SYNTHIO
//#define DEBUG_STAT
//#define DEBUG_MODEL_G4

//typedef int64_t LBA_t;

#ifdef _WIN32
#define u_int       unsigned int
#define u_int64_t   unsigned __int64
#endif

/* must enable this on Suns and Alphas */
#ifndef _WIN32
#define u_int32_t	unsigned int
#define int32_t		int
#else
#define u_int32_t       unsigned long
#define int32_t         long
#endif

#ifndef _WIN32
#define SUPPORT_CHECKPOINTS
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define DISKSIM_time()		time(NULL)

#ifndef MAXINT
#define MAXINT	0x7FFFFFFF
#endif

#include "disksim_assertlib.h"

#ifdef __cplusplus
extern "C" {
typedef      unsigned u_int;
#endif

#define ALLOCSIZE	(8192)

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#define BITS_PER_INT_MASK	0x0000001F
#define INV_BITS_PER_INT_MASK	0xFFFFFFE0

#define _BIG_ENDIAN	1
#define _LITTLE_ENDIAN	2

/* Trace Formats */

#define ASCII		1
#define HPL		2
#define DEC		3
#define VALIDATE	4
#define RAW		5
#define ATABUS          6
#define IPEAK           7
#define POSTGRES        8
#define EMCSYMM         9
#define EMCBACKEND      10
#define BATCH           11
#define DEFAULT		ASCII

/* Time conversions */

#define MILLI	1000
#define MICRO	1000000
#define NANO	1000000000

#define SECONDS_PER_MINUTE	60
#define SECONDS_PER_HOUR	3600
#define SECONDS_PER_DAY		86400

/* Flags components */

#include "disksim_reqflags.h"

#define READ_WRITE_MASK DISKSIM_READ
#define WRITE           DISKSIM_WRITE
#define READ            DISKSIM_READ
#define TIME_CRITICAL   DISKSIM_TIME_CRITICAL
#define TIME_LIMITED    DISKSIM_TIME_LIMITED
#define TIMED_OUT       DISKSIM_TIMED_OUT
#define HALF_OUT        DISKSIM_HALF_OUT
#define MAPPED          DISKSIM_MAPPED
#define READ_AFTR_WRITE DISKSIM_READ_AFTR_WRITE
#define SYNCHRONOUS	    DISKSIM_SYNC
#define ASYNCHRONOUS    DISKSIM_ASYNC
#define IO_FLAG_PAGEIO  DISKSIM_IO_FLAG_PAGEIO
#define SEQ             DISKSIM_SEQ
#define LOCAL           DISKSIM_LOCAL
#define BATCH_COMPLETE  DISKSIM_BATCH_COMPLETE

/* Event Type Ranges */

#define NULL_EVENT          0
#define PF_MIN_EVENT        1
#define PF_MAX_EVENT        97
#define INTR_EVENT          98
#define INTEND_EVENT        99
#define IO_MIN_EVENT        100
#define IO_MAX_EVENT        120
#define TIMER_EXPIRED       121
#define CHECKPOINT          122
#define STOP_SIM            123
#define EXIT_DISKSIM        124
#define MEMS_MIN_EVENT      200
#define MEMS_MAX_EVENT      220
#define SSD_MIN_EVENT       300
#define SSD_MAX_EVENT       320

/* Interrupt vector types */

#define CLOCK_INTERRUPT		100
#define SUBCLOCK_INTERRUPT	101
#define IO_INTERRUPT    	102
#define CPUSWAP_INTERRUPT    	103

#define MSECS_PER_MIN		60000

/* 
   this threshold allows some difference between disksim's
   internal time and simos' time.  it was determined by
   trial and error and may require some adjustment.
*/
#define DISKSIM_TIME_THRESHOLD  0.0013

typedef union {
    u_int32_t   value;
    char        byte[4];
} intchar;

#define StaticAssert(c) switch (c) case 0: case (c):

typedef struct foo {
    double time;
    int    type;
    struct ev *next;
    struct ev *prev;
//    int    temp;  // hurst_r commented out because other structures don't adhere to this
} foo;

#define DISKSIM_EVENT_SIZE	256
#define DISKSIM_EVENT_SPACESIZE	(DISKSIM_EVENT_SIZE - sizeof(struct foo))

typedef struct ev {
    double time;
    int    type;
    struct ev *next;
    struct ev *prev;
//    int    temp;
    char   space[DISKSIM_EVENT_SPACESIZE];
} event;

typedef struct ioreq_ev {
   double time;
   int    type;
   struct ioreq_ev *next;
   struct ioreq_ev *prev;
   int    tagID;        // unique value that identifies this IO request;
   int    bcount;
   int    blkno;
   u_int  flags;
   u_int  busno;
   u_int  slotno;
   int    devno;
   int    opid;
   void  *buf;
   int    cause;
   int    tempint1;
   int    tempint2;
   void  *tempptr1;
   void  *tempptr2;
   void  *mems_sled;	 /* mems sled associated with a particular event */
   void  *mems_reqinfo; /* per-request info for mems subsystem */
   int    ssd_elem_num;	 /* SSD: element to which this request went */
   int    ssd_gang_num ; /* SSD: gang to which this request went */
   double start_time;    /* temporary; used for memulator timing */
   int    batchno;
   int    batch_complete;
   int    batch_size;
   struct ioreq_ev *batch_next;
   struct ioreq_ev *batch_prev;
} ioreq_event;

typedef struct timer_ev {
   double time;
   int type;
   struct timer_ev *next;
   struct timer_ev *prev;
   void (**func)(struct timer_ev *);
   int    val;
   void  *ptr;
} timer_event;

typedef struct intr_ev {
   double time;
   int    type;
   struct intr_ev * next;
   struct intr_ev * prev;
   int    vector;
   int    oldstate;
   int    flags;
   event  *eventlist;
   event  *infoptr;
   double runtime;
} intr_event;


/* place-holding definitions for structure types; placed here because */
/* some compilers can't handle them being inside other structure defs */

struct cacheevent;
struct ioq;
struct iosim_info;
struct device_info;
struct disk_info;
struct simpledisk_info;
struct mems_info;
struct iodriver_info;
struct businfo;
struct ctlrinfo;
struct pf_info;
struct synthio_info;
struct iotrace_info;
struct rand48_info;

typedef event*(*disksim_iodone_notify_t)(ioreq_event *, void *ctx);
enum trace_mode_t { DISKSIM_MASTER, DISKSIM_SLAVE, DISKSIM_NONE };

typedef struct disksim {
   int    tagIDBegin;    // used to track when a command is created (command tagging and logging)
   int    tagIDEnd;      // used to track when a command has completed (command tagging and logging)
   void * startaddr;
   int    totallength;
   int    curroffset;
   int    event_count;   // total events simulated
   int    totalreqs;
   int    closedios;
   double closedthinktime;
   int    warmup_iocnt;
   double warmuptime;
   timer_event *warmup_event;
   double simtime;
   int    checkpoint_disable;
   int    checkpoint_iocnt;
   double checkpoint_interval;
   event *checkpoint_event;
   int    traceformat;
   int    endian;
   int    traceendian;
   int    traceheader;
   int    iotrace;
   int    synthgen;
   int    external_control;

   disksim_iodone_notify_t external_io_done_notify;
   void *notify_ctx;

   FILE * diskqueuelog;     // hurst_r used to log Disk Controller IOQueue activity
   FILE * disklog;          // hurst_r used to log Disk Seek activity
   FILE * debuglog;         // hurst_r
   FILE * eventlog;         // hurst_r used to log events
   FILE * simlog;           // hurst_r used to log sim results
   FILE * synthiolog;       // hurst_r used to log SynthIO activity
   FILE * parfile;
   FILE * iotracefile;
   FILE * statdeffile;
   FILE * outputfile;
   FILE * outios;
   char   iotracefilename[256];
   char   outputfilename[256];
   char   outiosfilename[256];
   char   checkpointfilename[256];
   fpos_t iotracefileposition;
   fpos_t outputfileposition;
   fpos_t outiosfileposition;
   event *intq;
   event *intqhint;
   event *extraq;
   int    intqlen;
   int    extraqlen;
   int    stop_sim;
   int    seedval;
   double lastphystime;

/* call-back indirections for allowing checkpoint restores to deal with */
/* functions whose addresses change on recompilation.                   */
   void         (*issuefunc_ctlrsmart)     (void *, ioreq_event *);
   struct ioq * (*queuefind_ctlrsmart)     (void *, int);
   void         (*wakeupfunc_ctlrsmart)    (void *, struct cacheevent *);
   void         (*donefunc_ctlrsmart_read) (void *, ioreq_event *);
   void         (*donefunc_ctlrsmart_write)(void *, ioreq_event *);
   void         (*donefunc_cachemem_empty) (void *, ioreq_event *);
   void         (*donefunc_cachedev_empty) (void *, ioreq_event *);
   void         (*idlework_cachemem)       (void *, int);
   void         (*idlework_cachedev)       (void *, int);
   void         (*idlework_diskcltr)       (void *, int);
   int          (*concatok_cachemem)       (void *, int, int, int, int);
   int          (*enablement_disk)         (ioreq_event *);
   void         (*timerfunc_disksim)       (timer_event *);
   void         (*timerfunc_ioqueue)       (timer_event *);
   void         (*timerfunc_cachemem)      (timer_event *);
   void         (*timerfunc_cachedev)      (timer_event *);
   void         (*timerfunc_diskctlr)      (timer_event *);

/* opaque structures for different modules */
   struct iosim_info *iosim_info;
   struct device_info *deviceinfo;
   struct disk_info *diskinfo;
   struct simpledisk_info *simplediskinfo;
   struct mems_info *memsinfo;
   struct ssd_info *ssdinfo;  /* SSD: ssd specific plugin */
   struct iodriver_info *iodriver_info;
   struct businfo *businfo;
   struct ctlrinfo *ctlrinfo;
   struct pf_info *pf_info;
   struct synthio_info *synthio_info;
   struct iotrace_info *iotrace_info;
   struct rand48_info *rand48_info;

   char **overrides;
   int overrides_len;
   int verbosity;

  int tracepipes[2];
  enum trace_mode_t trace_mode;

  FILE *exectrace;
  char *exectrace_fn;

} disksim_t;

extern disksim_t *disksim;


/* remapping #defines for some of the variables in disksim_t */
#define WARMUPTIME              (disksim->warmuptime)
#define simtime                 (disksim->simtime)
#define statdeffile             (disksim->statdeffile)
#define outputfile              (disksim->outputfile)
#define OUTIOS                  (disksim->outios)
#define DISKSIM_DISK_LOG        (disksim->disklog)
#define DISKSIM_DEBUG_LOG       (disksim->debuglog)
#define DISKSIM_DISK_QUEUE_LOG  (disksim->diskqueuelog)
#define DISKSIM_EVENT_LOG       (disksim->eventlog)
#define DISKSIM_SIM_LOG         (disksim->simlog)
#define DISKSIM_SYNTHIO_LOG     (disksim->synthiolog)

#ifndef _WIN32
#define	min(x,y)	((x) < (y) ? (x) : (y))

#define	max(x,y)	((x) < (y) ? (y) : (x))
#endif

#define	wrap(x,y)	((y) < (x) ? 1 : 0)

#define diff(x,y)	((x) < (y) ? (y)-(x) : (x)-(y))

#define rounduptomult(val,mult)	((val) + ((mult) - (((val)-1) % (mult))) - 1)

/* translate from the deprecated but convenient bzero function to memset */
#define bzero(ptr,size)  memset(ptr,0,size)


/* Global disksim_intr.c functions */

void intr_request(event *curr);
void intr_acknowledge (event *intrp);


/* Global disksim.c functions */

void resetstats (void);
void disksim_simstop (void);
void disksim_register_checkpoint (double atsimtime);
INLINE void addtoextraq (event *temp);
void addlisttoextraq (event **headptr);
INLINE event * getfromextraq (void);
event * event_copy (event *orig);
INLINE void addtointq (event *temp);
int removefromintq (event *curr);
void scanparam_int (char *parline, char *parname, int *parptr, int parchecks, int parminval, int parmaxval);
void getparam_int (FILE *parfile, char *parname, int *parptr, int parchecks, int parminval, int parmaxval);
void getparam_double (FILE *parfile, char *parname, double *parptr, int parchecks, double parminval, double parmaxval);
void getparam_bool (FILE *parfile, char *parname, int *parptr);
event * io_done_notify (ioreq_event *curr);


/* disksim.c functions used for external control */

int disksim_initialize_disksim_structure (struct disksim *);
int disksim_loadparams(char *inputfile, int synthgen);
void disksim_setup_disksim (int argc, char **argv);
void disksim_set_external_io_done_notify (disksim_iodone_notify_t);
void disksim_cleanup_and_printstats ( time_t startTime, time_t endTime );
void disksim_cleanstats (void);
void disksim_printstats2 (void);
void disksim_simulate_event (int);
void disksim_restore_from_checkpoint (char *filename);
void disksim_run_simulation ();

void disksim_printstats(void);
void dumpIOReq( const char * msg, ioreq_event * curr );

// destructor
void disksim_cleanup(void);

void disksim_exectrace(char *fmt,...);

char * getEventString( int eventType );

#ifdef __cplusplus
}
#endif

#endif  /* DISKSIM_GLOBAL_H */


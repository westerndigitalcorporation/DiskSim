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

#ifndef DISKSIM_IOQUEUE_H
#define DISKSIM_IOQUEUE_H

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_disk.h"
#include "config.h"


struct ioq;
extern void		ioqueue_setcallbacks (void);
extern void		ioqueue_initialize (struct ioq *queue, int devno);
extern void		ioqueue_resetstats (struct ioq *queue);
extern void		ioqueue_printstats (struct ioq **set, int setsize, char *sourcestr);
extern void		ioqueue_cleanstats (struct ioq *queue);
extern struct ioq *	ioqueue_createdefaultqueue ();
extern struct ioq *	ioqueue_readparams (FILE *parfile, int printqueuestats, int printcritstats, int printidlestats, int printintarrstats, int printsizestats);
extern void		ioqueue_param_override (struct ioq *queue, char *paramname, char *paramval);
extern struct ioq *	ioqueue_copy (struct ioq *queue);
extern double		ioqueue_tick (struct ioq *queue);
extern int		ioqueue_raise_priority (struct ioq *queue, int opid);
extern double		ioqueue_add_new_request (struct ioq *queue, ioreq_event *new_event);
extern ioreq_event *	ioqueue_get_next_request (struct ioq *queue);
extern ioreq_event *	ioqueue_show_next_request (struct ioq *queue);
extern ioreq_event *   ioqueue_set_starttime (struct ioq *queue, ioreq_event *target);
extern ioreq_event *	ioqueue_get_specific_request (struct ioq *queue, ioreq_event *wanted);
extern ioreq_event *	ioqueue_physical_access_done (struct ioq *queue, ioreq_event *curr);
extern int		ioqueue_get_number_pending (struct ioq *queue);
extern int		ioqueue_get_number_in_queue (struct ioq *queue);
extern int		ioqueue_get_number_of_requests (struct ioq *queue);
extern int		ioqueue_get_number_of_requests_initiated (struct ioq *queue);
extern int		ioqueue_get_reqoutstanding (struct ioq *queue);
extern int		ioqueue_get_dist (struct ioq *queue, int blkno);
extern void		ioqueue_set_concatok_function (struct ioq *queue, int (**concatok)(void *,int,int,int,int), void *concatokparam);
extern void		ioqueue_set_idlework_function (struct ioq *queue, void (**idlework)(void *,int), void *idleworkparam, double idledelay);
extern void		ioqueue_set_enablement_function (struct ioq *queue, int (**enablement)(ioreq_event *));
extern void		ioqueue_reset_idledetecter (struct ioq *queue, int timechange);
extern void		ioqueue_print_contents (struct ioq *queue);


/* Request scheduling algorithms */

#define MINSCHED	 1
#define FCFS             1
#define ELEVATOR_LBN     2
#define CYCLE_LBN        3
#define SSTF_LBN         4
#define ELEVATOR_CYL     5
#define CYCLE_CYL        6
#define SSTF_CYL         7
#define SPTF_OPT         8
#define SPCTF_OPT        9
#define SATF_OPT         10
#define WPTF_OPT         11
#define WPCTF_OPT        12
#define WATF_OPT         13
#define ASPTF_OPT        14
#define ASPCTF_OPT       15
#define ASATF_OPT        16
#define VSCAN_LBN        17
#define VSCAN_CYL        18
#define PRI_VSCAN_LBN    19
#define PRI_ASPTF_OPT    20
#define PRI_ASPCTF_OPT   21
#define SDF_APPROX       22
#define SDF_EXACT        23
#define SPTF_ROT_OPT     24
#define SPTF_ROT_WEIGHT  25
#define SPTF_SEEK_WEIGHT 26
#define TSPS             27
#define BATCH_FCFS       28
#define MAXSCHED         28


typedef struct iob {
   double    starttime;
   int       state;
   struct iob *next;
   struct iob *prev;
   int       totalsize;
   int       tagID;         // unique integer attached to this IO request
   int       devno;
   int       blkno;
   int       flags;
   int       batchno;
   int       batch_size;
   int       batch_complete;
   union {
      struct {
 	 int       waittime;
         ioreq_event *concat;
      } pend;
      double time;
   } iob_un;
   ioreq_event *iolist;
   ioreq_event *batch_list;
   int       reqcnt;
   int       cylinder;
   int       surface;
   int       opid;
} iobuf;

struct ioq;

typedef struct subq {
    struct ioq * bigqueue;
    int     sched_alg;
    int     surfoverforw;
    int     force_absolute_fcfs;
    int     (**enablement)(ioreq_event *);
    iobuf * list;
    iobuf * current;
    int     prior;
    int     dir;
    double  vscan_value;
    int     vscan_cyls;
    int     lastblkno;
    int		lastsurface;
    int		lastcylno;
    int		optcylno;
    int		optsurface;
    int		sstfupdown;
    int		sstfupdowncnt;
    int		numreads;
    int		numwrites;
    int		switches;
    int		maxqlen;
    double	lastalt;
    int		listlen;
    int		iobufcnt;
    int		maxlistlen;
    double	runlistlen;
    int		readlen;
    int		maxreadlen;
    double	runreadlen;
    int		maxwritelen;
    int		numcomplete;
    int		reqoutstanding;
    int		numoutstanding;
    int		maxoutstanding;
    double	runoutstanding;
    int          num_sptf_sdf_different;
    int          num_scheduling_decisions;
    statgen	outtimestats;
    statgen	critreadstats;
    statgen	critwritestats;
    statgen	nocritreadstats;
    statgen	nocritwritestats;
    statgen	qtimestats;
    statgen	accstats;
    statgen	instqueuelen;
    statgen	infopenalty;
} subqueue;

typedef struct ioq {
   subqueue	base;
   subqueue	timeout;
   subqueue	priority;
   int		devno;
   void		(**idlework)(void *,int);
   void	*	idleworkparam;
   int		idledelay;
   timer_event *idledetect;
   int		(**coalesceok)(void *,int,int,int,int);
   void *	coalesceokparam;
   int		(**concatok)(void *,int,int,int,int);
   void *	concatokparam;
   int		concatmax;
   int		comboverlaps;
   int		seqscheme;
   int 		seqstreamdiff;
   int		to_scheme;
   int		to_time;
   int		pri_scheme;
   int      priority_mix;
   int		cylmaptype;
   double	writedelay;
   double	readdelay;
   int		sectpercyl;
   int		lastsubqueue;
   int		timeouts;
   int		timeoutreads;
   int		halfouts;
   int		halfoutreads;
   double   idlestart;
   double	lastarr;
   double	lastread;
   double	lastwrite;
   double       latency_weight;
   statgen	idlestats;
   statgen	intarrstats;
   statgen	readintarrstats;
   statgen	writeintarrstats;
   statgen	reqsizestats;
   statgen	readsizestats;
   statgen	writesizestats;
   statgen      batchsizestats;
   int          numbatches;
   int		maxlistlen;
   int		maxqlen;
   int		maxoutstanding;
   int		maxreadlen;
   int		maxwritelen;
   int		overlapscombed;
   int		readoverlapscombed;
   int		seqblkno;
   int		seqflags;
   int		seqreads;
   int		seqwrites;
   int		printqueuestats;
   int		printcritstats;
   int		printidlestats;
   int		printintarrstats;
   int		printsizestats;
   char        *name;
} ioqueue;


#endif    /* DISKSIM_IOQUEUE_H */


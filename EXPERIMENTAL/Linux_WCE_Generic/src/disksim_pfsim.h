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

#ifndef DISKSIM_PFSIM_H
#define DISKSIM_PFSIM_H

#include "disksim_stat.h"

/* Process-flow event types */

#define SLEEP_EVENT             3
#define WAKEUP_EVENT            5
#define IOREQ_EVENT             9
#define IOACC_EVENT             10

#define CPU_EVENT	        50
#define SYNTHIO_EVENT		51
#define IDLELOOP_EVENT		52
#define CSWITCH_EVENT		53

#define MAXCPUS 20

#define PF_SLEEP_FLAG  0x00000002
#define PF_WAKED       0x00000010

#define PF_BUF_WANTED  0x00000040

/* CPU states */

#define CPU_IDLE	0
#define CPU_PROCESS	1
#define CPU_IDLE_WORK	2
#define CPU_INTR	3

/* Process status flags */

#define PROC_SLEEP	1
#define PROC_RUN	2
#define PROC_ONPROC	3

/* event times */

#define PF_INTER_CLOCK_TIME		10.0
#define PF_INTER_LONG_CLOCKS		100
#define PF_LONG_CLOCK_TIME		0.0
#define PF_SHORT_CLOCK_TIME		0.0
#define PF_CSWITCH_TIME			0.0
#define PF_IDLE_CHECK_DISPQ_TIME	1000.0
#define PF_IO_WAKEUP_TIME		0.0

typedef struct process {
   int pid;
   int idler;
   u_int pfflags;
   u_int stat;
   u_int runcpu;
   u_int flags;
   void *chan;
   int  ios;
   int  ioreads;
   int  cswitches;
   int	active;
   int  sleeps;
   int  iosleep;
   int  iosleeps;
   double runiosleep;
   double lastsleep;
   double runsleep;
   double runtime;
   double falseidletime;
   double lasteventtime;
   statgen readtimelimitstats;
   statgen writetimelimitstats;
   statgen readmisslimitstats;
   statgen writemisslimitstats;
   ioreq_event *ioreq;
   event *eventlist;
   char *space;
   struct process *livelist;
   struct process *link;
   struct process *next;
} process;

typedef struct {
   double      time;
   int         type;
   event      *next;
   event      *prev;
   int         cpunum;
   process    *procp;
   intr_event *intrp;
} cpu_event;

typedef struct {
   double     scale;
   int        state;
   cpu_event *current;
   int        runpreempt;
   event     *idleevents;
   double     idlestart;
   double     idletime;
   double     falseidletime;
   double     idleworktime;
   int        intrs;
   int        iointrs;
   int        clockintrs;
   int        tickintrs;
   int        extintrs;
   double     runintrtime;
   double     runiointrtime;
   double     runclockintrtime;
   double     runtickintrtime;
   double     runextintrtime;
   int        cswitches;
   double     runswitchtime;
} cpu;

typedef struct {
   double time;
   int    type;
   event * next;
   event * prev;
   int    vector;
} intend_event;

typedef struct {
   double time;
   int    type;
   event * next;
   event * prev;
   process *newprocp;
} cswitch_event;

typedef struct {
   double time;
   int    type;
   event * next;
   event * prev;
} idleloop_event;

typedef struct {
   double time;
   int    type;
   event * next;
   event * prev;
   int    info;
   void  *chan;
   int    iosleep;
   int    sleeptime;
} sleep_event;

typedef struct {
   double time;
   int    type;
   event * next;
   event * prev;
   int    info;
   void  *chan;
   int    dropit;
} wakeup_event;

typedef struct pf_info {
   /* stuff originating in disksim_pfsim.c */
   cpu *cpus;
   int  numcpus;
   process *process_livelist;
   process *extra_process_q;
   int  extra_process_qlen;
   int  curlbolt;                   // signed int that counts the number of clock ticks since the system was booted?? hurst_r
   ioreq_event *pendiolist;
   ioreq_event *doneiolist;
   process *synthlist;
   double  pfscale;
   double lastuser;
   double idlein;
   int  idlereset;
   int pf_printhack;
   statgen timecritrespstats;
   statgen timelimitrespstats;
   statgen timenoncritrespstats;
   int pf_print_perprocessstats;
   int pf_print_percpustats;
   int pf_print_intrstats;
   int pf_print_sleepstats;
   /* stuff originating in disksim_pfdisp.c */
   process *pf_dispq;
   process *sleepqueue;
} pf_info_t;


/* one remapping #define for each variable in pf_info_t */
#define cpus                      (disksim->pf_info->cpus)
#define numcpus                   (disksim->pf_info->numcpus)
#define process_livelist          (disksim->pf_info->process_livelist)
#define extra_process_q           (disksim->pf_info->extra_process_q)
#define extra_process_qlen        (disksim->pf_info->extra_process_qlen)
#define curlbolt                  (disksim->pf_info->curlbolt)
#define pendiolist                (disksim->pf_info->pendiolist)
#define doneiolist                (disksim->pf_info->doneiolist)
#define synthlist                 (disksim->pf_info->synthlist)
#define pfscale                   (disksim->pf_info->pfscale)
#define lastuser                  (disksim->pf_info->lastuser)
#define idlein                    (disksim->pf_info->idlein)
#define idlereset                 (disksim->pf_info->idlereset)
#define pf_printhack              (disksim->pf_info->pf_printhack)
#define timecritrespstats         (disksim->pf_info->timecritrespstats)
#define timelimitrespstats        (disksim->pf_info->timelimitrespstats)
#define timenoncritrespstats      (disksim->pf_info->timenoncritrespstats)
#define pf_print_perprocessstats  (disksim->pf_info->pf_print_perprocessstats)
#define pf_print_percpustats      (disksim->pf_info->pf_print_percpustats)
#define pf_print_intrstats        (disksim->pf_info->pf_print_intrstats)
#define pf_print_sleepstats       (disksim->pf_info->pf_print_sleepstats)
#define pf_dispq                  (disksim->pf_info->pf_dispq)
#define sleepqueue                (disksim->pf_info->sleepqueue)


/* disksim_pfsim.c functions */

int       pf_how_many_cpus ();
process * pf_getfromextra_process_q (void);
void      pf_idle_cpu_recheck_dispq (int cpunum);
process * pf_new_process (void);

/* disksim_pfdisp.c functions */

void      pf_dispatcher_init (process *startprocs);
process * pf_disp_get_from_sleep_queue (void *chan);
process * pf_disp_get_specific_from_sleep_queue (u_int pid);
void      pf_disp_put_on_sleep_queue (process *procp);
process * pf_dispatch (int cpunum);
process * pf_disp_sleep (process *procp);
void      pf_disp_wakeup (process *procp);

#endif /* DISKSIM_PFSIM_H */


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

#ifndef DISKSIM_IODRIVER_H
#define DISKSIM_IODRIVER_H

#include "config.h"
#include "disksim_iosim.h" // for device 
#include "disksim_stat.h"  // for statgen


/* Device driver types */

#define STANDALONE	1
/* & NOTSTANDALONE, which corresponds to a driver that is actually part of */
/* the process-flow model (i.e., a system-level simulation rather than a   */
/* standalone storage subsystem simulation). */

/* Device driver ctlr flags */

#define DRIVER_C700		0x2
#define DRIVER_CTLR_BUSY	0x4

/* Device driver computation times */

#define IODRIVER_IMMEDSCHED_TIME		0.0
#define IODRIVER_WRITE_DISCONNECT_TIME		0.0
#define IODRIVER_READ_DISCONNECT_TIME		0.0
#define IODRIVER_RECONNECT_TIME			0.0
#define IODRIVER_BASE_COMPLETION_TIME		0.0

#define IODRIVER_COMPLETION_RESPTODEV_TIME	0.0
#define IODRIVER_COMPLETION_RESPTOSCHED_TIME	0.0
#define IODRIVER_COMPLETION_SCHEDTOEND_TIME	0.0
#define IODRIVER_COMPLETION_SCHEDTOWAKEUP_TIME	0.0
#define IODRIVER_COMPLETION_WAKEUPTOEND_TIME	0.0
#define IODRIVER_COMPLETION_RESPTOWAKEUP_TIME	0.0
#define IODRIVER_COMPLETION_RESPTOREQ_TIME	0.0    /* e.g., Software RAID */

/* Special Constant Queue/Access times */

#define IODRIVER_TRACED_ACCESS_TIMES	-1.0
#define IODRIVER_TRACED_QUEUE_TIMES	-2.0
#define IODRIVER_TRACED_BOTH_TIMES	-3.0


typedef struct iodriver {
  int		type;
  int		usequeue;
  int		numoutbuses;
  int		outbus;
  double	consttime;     /* Use value if positive, or traced if neg. */
  double	scale;
  int		numdevices;
  int		numctlrs;
  struct ioq *	queue;
  device *	devices;
  ctlr *	ctlrs;
  char        *name;
} iodriver;


typedef struct iodriver_info {
   int numiodrivers;
   iodriver **iodrivers;
   int iodrivers_len;
   int numsysorgs;
   struct logorg **sysorgs;
   int sysorgs_len;
   struct ioq *overallqueue;
   int drv_printsizestats;
   int drv_printlocalitystats;
   int drv_printblockingstats;
   int drv_printinterferestats;
   int drv_printqueuestats;
   int drv_printcritstats;
   int drv_printidlestats;
   int drv_printintarrstats;
   int drv_printstreakstats;
   int drv_printstampstats;
   int drv_printperdiskstats;
   statgen emptyqueuestats;
   statgen initiatenextstats;
} iodriver_info_t;


/* one remapping #define for each variable in iodriver_info_t */
#define numiodrivers            (disksim->iodriver_info->numiodrivers)
#define iodrivers               (disksim->iodriver_info->iodrivers)
#define numsysorgs              (disksim->iodriver_info->numsysorgs)
#define sysorgs                 (disksim->iodriver_info->sysorgs)
#define OVERALLQUEUE            (disksim->iodriver_info->overallqueue)
#define drv_printsizestats      (disksim->iodriver_info->drv_printsizestats)
#define drv_printlocalitystats  (disksim->iodriver_info->drv_printlocalitystats)
#define drv_printblockingstats  (disksim->iodriver_info->drv_printblockingstats)
#define drv_printinterferestats (disksim->iodriver_info->drv_printinterferestats)
#define drv_printqueuestats     (disksim->iodriver_info->drv_printqueuestats)
#define drv_printcritstats      (disksim->iodriver_info->drv_printcritstats)
#define drv_printidlestats      (disksim->iodriver_info->drv_printidlestats)
#define drv_printintarrstats    (disksim->iodriver_info->drv_printintarrstats)
#define drv_printstreakstats    (disksim->iodriver_info->drv_printstreakstats)
#define drv_printstampstats     (disksim->iodriver_info->drv_printstampstats)
#define drv_printperdiskstats   (disksim->iodriver_info->drv_printperdiskstats)
#define emptyqueuestats         (disksim->iodriver_info->emptyqueuestats)
#define initiatenextstats       (disksim->iodriver_info->initiatenextstats)




/* exported disksim_iodriver.c functions */

void    iodriver_read_toprints (FILE *parfile);
void    iodriver_read_specs (FILE *parfile);
void    iodriver_read_physorg (FILE *parfile);
void    iodriver_read_logorg (FILE *parfile);
void    iodriver_param_override (char *paramname, char *paramval, int first, int last);
void    iodriver_sysorg_param_override (char *paramname, char *paramval, int first, int last);
void    iodriver_setcallbacks (void);
void    iodriver_initialize (int standalone);
void    iodriver_resetstats (void);
void    iodriver_printstats (void);
void    iodriver_cleanstats (void);
event * iodriver_request (int iodriverno, ioreq_event *curr);
void    iodriver_schedule (int iodriverno, ioreq_event *curr);
double  iodriver_tick (void);
double  iodriver_raise_priority (int iodriverno, int opid, int devno, int blkno, void *chan);
void    iodriver_interrupt_arrive (int iodriverno, intr_event *intrp);
void    iodriver_access_complete (int iodriverno, intr_event *intrp);
void    iodriver_respond_to_device (int iodriverno, intr_event *intrp);
void    iodriver_interrupt_complete (int iodriverno, intr_event *intrp);
void    iodriver_trace_request_start (int iodriverno, ioreq_event *curr);

int iodriver_load_logorg(struct lp_block *b);
int load_iodriver_topo(struct lp_topospec *t, int len);

void iodriver_cleanup(void);

#endif /* DISKSIM_IODRIVER_H */


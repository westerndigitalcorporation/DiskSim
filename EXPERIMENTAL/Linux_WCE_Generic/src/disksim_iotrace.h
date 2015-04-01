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


#ifndef DISKSIM_IOTRACE_H
#define DISKSIM_IOTRACE_H

#include <stdio.h>

/* really need to clean interface between iotrace.c and iosim.c, such */
/* that this stuff can be local to just iotrace.c ...                 */

typedef struct iotrace_info {
   double tracebasetime;
   int syncreads;
   int syncwrites;
   int asyncreads;
   int asyncwrites;
   int hpreads;
   int hpwrites;
   int firstio;
   int basehighshort;
   int basehighshort2;
   int lasttime1;
   double lasttime;
   int baseyear;
   int baseday;
   int basesecond;
   int basebigtime;
   int basesmalltime;
   double basesimtime;
   double validate_lastserv;
   int validate_lastblkno;
   int validate_lastbcount;
   int validate_lastread;
   double validate_nextinter;
   char validate_buffaction[20];
   double accumulated_event_time;
   double lastaccesstime;
} iotrace_info_t;


/* one remapping #define for each variable in iotrace_info_t */
#define tracebasetime           (disksim->iotrace_info->tracebasetime)
#define syncreads               (disksim->iotrace_info->syncreads)
#define syncwrites              (disksim->iotrace_info->syncwrites)
#define asyncreads              (disksim->iotrace_info->asyncreads)
#define asyncwrites             (disksim->iotrace_info->asyncwrites)
#define hpreads                 (disksim->iotrace_info->hpreads)
#define hpwrites                (disksim->iotrace_info->hpwrites)
#define firstio                 (disksim->iotrace_info->firstio)
#define basehighshort           (disksim->iotrace_info->basehighshort)
#define basehighshort2          (disksim->iotrace_info->basehighshort2)
#define lasttime1               (disksim->iotrace_info->lasttime1)
#define lasttime                (disksim->iotrace_info->lasttime)
#define baseyear                (disksim->iotrace_info->baseyear)
#define baseday                 (disksim->iotrace_info->baseday)
#define basesecond              (disksim->iotrace_info->basesecond)
#define basebigtime             (disksim->iotrace_info->basebigtime)
#define basesmalltime           (disksim->iotrace_info->basesmalltime)
#define basesimtime             (disksim->iotrace_info->basesimtime)
#define validate_lastserv       (disksim->iotrace_info->validate_lastserv)
#define validate_lastblkno      (disksim->iotrace_info->validate_lastblkno)
#define validate_lastbcount     (disksim->iotrace_info->validate_lastbcount)
#define validate_lastread       (disksim->iotrace_info->validate_lastread)
#define validate_nextinter      (disksim->iotrace_info->validate_nextinter)
#define validate_buffaction     (disksim->iotrace_info->validate_buffaction)
#define accumulated_event_time  (disksim->iotrace_info->accumulated_event_time)
#define lastaccesstime          (disksim->iotrace_info->lastaccesstime)


/* exported disksim_iotrace.c functions */

void iotrace_set_format (char *formatname);
void iotrace_initialize_file (FILE *tracefile, int traceformat, int print_tracefile_header);
ioreq_event * iotrace_get_ioreq_event (FILE *tracefile, int traceformat, ioreq_event *temp);
ioreq_event * iotrace_validate_get_ioreq_event(FILE *tracefile, ioreq_event *new_event);
void iotrace_printstats (FILE *outfile);

#endif    /* DISKSIM_IOTRACE_H */


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

#ifndef DISKSIM_IOSIM_H
#define DISKSIM_IOSIM_H


#include "disksim_ioface.h"


/* I/O Subsystem restrictions */

#define MAXDEVICES      100
#define MAXDEPTH        4     /* Bus hierarchy can be up to four levels deep */
#define MAXINBUSES      1
#define MAXOUTBUSES     4
#define MAXSLOTS        15
#define MAXLOGORGS      100


#include "disksim_device.h"


/* I/O Event types */

/* #define IO_REQUEST_ARRIVE        100 *//* actually in ioface.h */
#define IO_ACCESS_ARRIVE                    101
#define IO_INTERRUPT_ARRIVE                 102
#define IO_RESPOND_TO_DEVICE                103
#define IO_ACCESS_COMPLETE                  104
#define IO_INTERRUPT_COMPLETE               105
#define DEVICE_OVERHEAD_COMPLETE            106
#define DEVICE_ACCESS_COMPLETE              107
#define DEVICE_PREPARE_FOR_DATA_TRANSFER    108
#define DEVICE_DATA_TRANSFER_COMPLETE       109
#define DEVICE_BUFFER_SEEKDONE              110
#define DEVICE_BUFFER_TRACKACC_DONE         111
#define DEVICE_BUFFER_SECTOR_DONE           112
#define DEVICE_GOT_REMAPPED_SECTOR          113
#define DEVICE_GOTO_REMAPPED_SECTOR         114
#define BUS_OWNERSHIP_GRANTED               115
#define BUS_DELAY_COMPLETE                  116
#define CONTROLLER_DATA_TRANSFER_COMPLETE   117
#define TIMESTAMP_LOGORG                    118
#define IO_TRACE_REQUEST_START              119
#define IO_QLEN_MAXCHECK                    120

/* mems event types */

#define MEMS_SLED_SCHEDULE  201
#define MEMS_SLED_SEEK      202
#define MEMS_SLED_SERVO     203
#define MEMS_SLED_DATA      204
#define MEMS_SLED_UPDATE    205
#define MEMS_BUS_INITIATE   206
#define MEMS_BUS_TRANSFER   207
#define MEMS_BUS_UPDATE     208

/* SSD: ssd event types -- keep this between SSD_MIN_EVENT and SSD_MAX_EVENT */

#define SSD_CLEAN_ELEMENT   301
#define SSD_CLEAN_GANG      302

/* I/O Interrupt cause types */

typedef enum {
  COMPLETION        = -1,
  RECONNECT         = -2,
  DISCONNECT        = -3,
  READY_TO_TRANSFER = -4
} disksim_int_t;

#ifndef IOFACE_H

/* Device types */

#define IODRIVER    2
#define DEVICE        4
#define CONTROLLER    5

#endif   /* IOFACE_H */

/* Call types for diskacctime() */

#define DISKACCESS    1
#define DISKACCTIME    2
#define DISKPOS        3
#define DISKPOSTIME    4
#define DISKSEEK    5
#define DISKSEEKTIME    6
#define DISKSERVTIME    7

/* LBN to PBN mapping types (also for getting cylinder mappings) */

/* no idea why this was ever here, now lives in diskmodel.  bucy */
/*  typedef enum { */
/*    MAP_NONE        = 0, */
/*    MAP_IGNORESPARING    = 1, */
/*    MAP_ZONEONLY        = 2, */
/*    MAP_ADDSLIPS        = 3, */
/*    MAP_FULL        = 4, */
/*    MAP_FROMTRACE        = 5, */
/*    MAP_AVGCYLMAP        = 6 */
/*  } disksim_map_t; */

/*  #define    MAXMAPTYPE MAP_AVGCYLMAP */

/* Convenient to define here, so can be used both by controllers and drivers */
/* These are just simple structures used in higher-level components to track */
/* important state (e.g., access paths and outstanding requests).            */

typedef struct {
   int          ctlno;
   int          flags;
   intchar      slotpath;
   intchar      buspath;
   int          maxoutstanding;
   int          numoutstanding;
   int          maxreqsize;
   ioreq_event *pendio;
   ioreq_event *oversized;
} ctlr;

/* NOTE: this is *not* the same as what disksim_device.[ch] cares about. */
typedef struct {
   double       lastevent;
   int          flag;
   int          devno;
   int          busy;
   intchar      slotpath;
   intchar      buspath;
   int          maxoutstanding;
   int          queuectlr;
   ctlr         *ctl;
   struct ioq   *queue;
} device;


/* exported disksim_iosim.c functions */

void io_validate_do_stats1 ();
void io_validate_do_stats2 (ioreq_event *new_event);
ioreq_event * ioreq_copy (ioreq_event *old);
int ioreq_compare (ioreq_event *first, ioreq_event *second);
void iosim_get_path_to_controller (int iodriverno, int ctlno, intchar *buspath, intchar *slotpath);
void iosim_get_path_to_device (int iodriverno, int devno, intchar *buspath, intchar *slotpath);
int iosim_load_mappings(struct lp_list *l);
char * iosim_decodeInterruptEvent( ioreq_event *ioReqEvent );

#include "disksim_global.h"
#include "disksim_ioface.h"
#include "disksim_orgface.h"
#include "disksim_iotrace.h"
#include "disksim_stat.h"
#include "disksim_iodriver.h"
#include "disksim_bus.h"
#include "disksim_controller.h"
#include "config.h"

#define TRACEMAPPINGS    MAXDEVICES

#define PRINTTRACESTATS        TRUE


/* read-only globals used during readparams phase */
static char *statdesc_tracequeuestats =        "Trace queue time";
static char *statdesc_tracerespstats =         "Trace response time";
static char *statdesc_traceaccstats =          "Trace access time";
static char *statdesc_traceqlenstats =         "Trace queue length";
static char *statdesc_tracenoqstats =          "Trace non-queue time";
static char *statdesc_traceaccdiffstats =      "Trace access diff time";
static char *statdesc_traceaccwritestats =     "Trace write access time";
static char *statdesc_traceaccdiffwritestats = "Trace write access diff time";


typedef struct iosim_info {
   event * io_extq;
   int     io_extqlen;
   int     io_extq_type;
   double  ioscale;
   double  last_request_arrive;
   double  constintarrtime;
   int     validatebuf[10];
   int     tracemappings;
   int     tracemap[TRACEMAPPINGS];
   int     tracemap1[TRACEMAPPINGS];
   int     tracemap2[TRACEMAPPINGS];
   int     tracemap3[TRACEMAPPINGS];
   int     tracemap4[TRACEMAPPINGS];
   statgen *tracestats;
   statgen *tracestats1;
   statgen *tracestats2;
   statgen *tracestats3;
   statgen *tracestats4;
   statgen *tracestats5;
} iosim_info_t;


/* one remapping #define for each variable in iosim_info_t */
#define io_extq                  (disksim->iosim_info->io_extq)
#define io_extqlen               (disksim->iosim_info->io_extqlen)
#define io_extq_type             (disksim->iosim_info->io_extq_type)
#define ioscale                  (disksim->iosim_info->ioscale)
#define last_request_arrive      (disksim->iosim_info->last_request_arrive)
#define constintarrtime          (disksim->iosim_info->constintarrtime)
#define validatebuf              (disksim->iosim_info->validatebuf)
#define tracemappings            (disksim->iosim_info->tracemappings)
#define tracemap                 (disksim->iosim_info->tracemap)
#define tracemap1                (disksim->iosim_info->tracemap1)
#define tracemap2                (disksim->iosim_info->tracemap2)
#define tracemap3                (disksim->iosim_info->tracemap3)
#define tracemap4                (disksim->iosim_info->tracemap4)
#define tracestats               (disksim->iosim_info->tracestats)
#define tracestats1              (disksim->iosim_info->tracestats1)
#define tracestats2              (disksim->iosim_info->tracestats2)
#define tracestats3              (disksim->iosim_info->tracestats3)
#define tracestats4              (disksim->iosim_info->tracestats4)
#define tracestats5              (disksim->iosim_info->tracestats5)


#endif   /* DISKSIM_IOSIM_H */


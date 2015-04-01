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


#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_ioqueue.h"
#include "disksim_bus.h"
#include "config.h"

#ifndef DISKSIM_SIMPLEDISK_H
#define DISKSIM_SIMPLEDISK_H

/* externalized disksim_simpledisk.c functions */

void    simpledisk_read_toprints (FILE *parfile);
void    simpledisk_read_specs (FILE *parfile, int devno, int copies);
void    simpledisk_set_syncset (int setstart, int setend);
void    simpledisk_param_override (char *paramname, char *paramval, int first, int last);
void    simpledisk_setcallbacks (void);
void    simpledisk_initialize (void);
void    simpledisk_resetstats (void);
void    simpledisk_printstats (void);
void    simpledisk_printsetstats (int *set, int setsize, char *sourcestr);
void    simpledisk_cleanstats (void);
int     simpledisk_set_depth (int devno, int inbusno, int depth, int slotno);
int     simpledisk_get_depth (int devno);
int     simpledisk_get_inbus (int devno);
int     simpledisk_get_busno (ioreq_event *curr);
int     simpledisk_get_slotno (int devno);
int     simpledisk_get_number_of_blocks (int devno);
int     simpledisk_get_maxoutstanding (int devno);
int     simpledisk_get_numdisks (void);
int     simpledisk_get_numcyls (int devno);
double  simpledisk_get_blktranstime (ioreq_event *curr);
int     simpledisk_get_avg_sectpercyl (int devno);
void    simpledisk_get_mapping (int maptype, int devno, int blkno, int *cylptr, int *surfaceptr, int *blkptr);
void    simpledisk_event_arrive (ioreq_event *curr);
int     simpledisk_get_distance (int devno, ioreq_event *req, int exact, int direction);
double  simpledisk_get_servtime (int devno, ioreq_event *req, int checkcache, double maxtime);
double  simpledisk_get_acctime (int devno, ioreq_event *req, double maxtime);
void    simpledisk_bus_delay_complete (int devno, ioreq_event *curr, int sentbusno);
void    simpledisk_bus_ownership_grant (int devno, ioreq_event *curr, int busno, double arbdelay);

struct simpledisk *getsimpledisk (int devno);

/* default simpledisk dev header */
extern struct device_header simpledisk_hdr_initializer;



typedef struct {
   statgen acctimestats;
   double  requestedbus;
   double  waitingforbus;
   int     numbuswaits;
} simpledisk_stat_t;


typedef struct simpledisk {
  struct device_header hdr;
   double acctime;
   double overhead;
   double bus_transaction_latency;
   int numblocks;
   int devno;
   int inited;
   struct ioq *queue;
   int media_busy;
   int reconnect_reason;

   double blktranstime;
   int maxqlen;
   int busowned;
   ioreq_event *buswait;
   int neverdisconnect;
   int numinbuses;
   int inbuses[MAXINBUSES];
   int depth[MAXINBUSES];
   int slotno[MAXINBUSES];

   int printstats;
   simpledisk_stat_t stat;
} simpledisk_t;


typedef struct simpledisk_info {
   struct simpledisk **simpledisks;
   int numsimpledisks;
  int simpledisks_len; /* allocated size of simpledisks */
} simplediskinfo_t;


#endif   /* DISKSIM_SIMPLEDISK_H */


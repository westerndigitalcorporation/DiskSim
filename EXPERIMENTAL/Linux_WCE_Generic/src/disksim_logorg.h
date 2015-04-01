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

#ifndef DISKSIM_LOGORG_H
#define DISKSIM_LOGORG_H

#include "disksim_global.h"
#include "disksim_stat.h"
#include "disksim_iosim.h"
#include "disksim_orgface.h"
#include "disksim_ioqueue.h"
#include "config.h"


/* Mapping types */

#define ASIS            1
#define IDEAL           2
#define RANDOM          3
#define STRIPED         4

/* Redundancy types */

#define NO_REDUN		1
#define SHADOWED        	2
#define PARITY_DISK     	3
#define PARITY_ROTATED  	4
#define PARITY_TABLE		5

/* Shadow disk read choices */

#define SHADOW_PRIMARY		1
#define SHADOW_RANDOM		2
#define SHADOW_ROUNDROBIN	3
#define SHADOW_SHORTDIST	4
#define SHADOW_SHORTQUEUE	5
#define SHADOW_SHORTQUEUE2	6

/* Parity rotation types */

#define PARITY_LEFT_SYM		1
#define PARITY_LEFT_ASYM	2
#define PARITY_RIGHT_ASYM	3
#define PARITY_RIGHT_SYM	4

#define LOGORG_PARITY_SEQGIVE	32
#define MAXCOPIES	10
#define NUMGENS 50
#define HASH_OUTSTAND   20

#define BLOCKINGMAX	128
#define INTERFEREMAX	32
#define INTDISTMAX	8

typedef struct dep {
   int    devno;
   int    blkno;
   int    numdeps;
   struct dep *next;
   struct dep *cont;
   ioreq_event *deps[10];
} depends;

typedef struct os {
   double arrtime;
   int    type;
   struct os *next;
   struct os *prev;
   u_int  bcount;
   u_int  blkno;
   u_int  flags;
   u_int  busno;
   int    numreqs;
   u_int  devno;
   int    opid;
   int    reqopid;
   void  *buf;
   depends *depend;
} outstand;

typedef struct {
   double	outtime;
   double	runouttime;
   int		outstanding;
   int		readoutstanding;
   int		maxoutstanding;
   double	nonzeroouttime;
   int		reads;
   int		gens[NUMGENS];
   int		seqdiskswitches;
   int		locdiskswitches;
   int		numlocal;
   int		critreads;
   int		critwrites;
   int		seqreads;
   int		seqwrites;
   int		distavgdiff[10];
   int		distmaxdiff[10];
   double       idlestart;
   double       lastarr;
   double       lastread;
   double       lastwrite;
   int          *blocked;
   int          *aligned;
   int		*lastreq;
   int		*intdist;
   statgen      resptimestats;
   statgen	idlestats;
   statgen      sizestats;
   statgen	readsizestats;
   statgen	writesizestats;
   statgen      intarrstats;
   statgen      readintarrstats;
   statgen      writeintarrstats;
} logorgstat;

typedef struct {
   int    devno;
   int    startblkno;
   struct ioq * queue;
   int    lastblkno;
   int    seqreads;
   int    seqwrites;
   int    lastblkno2;
   int    intreads;
   int    intwrites;
   int    numout;
   int    distnumout[10];
   int    curstreak;
   statgen streakstats;
   statgen localitystats;
} logorgdev;

typedef struct {
   int    devno;
   int    blkno;
} tableentry;

typedef struct logorg {
  char *name;
   outstand * hashoutstand[HASH_OUTSTAND];
   int    outstandqlen;
   int    opid;
   int    addrbyparts;
   int    maptype;
   int    reduntype;
   int    numdisks;
   int    actualnumdisks;
   int    arraydisk;
   int    writesync;
   int    copies;
   int    copychoice;
   double rmwpoint;
   int    parityunit;
   int    rottype;
   int    blksperpart;
   int    actualblksperpart;
   int    stripeunit;
   int    sectionunit;
   int    tablestripes;
   tableentry *table;
   int    tablesize;
   int    partsperstripe;
   int    idealno;
   int    reduntoggle;
   int    lastdiskaccessed;
   int    numfull;
   int   *sizes;
   int   *redunsizes;
   int    printlocalitystats;
   int    printblockingstats;
   int    printinterferestats;
   int    printidlestats;
   int    printintarrstats;
   int    printstreakstats;
   int    printstampstats;
   int    printsizestats;
   double stampinterval;
   double stampstart;
   double stampstop;
   FILE * stampfile;
   logorgdev *devs;
   logorgstat stat;
   /* rcohen's additions */
   int    startdev;
   int    enddev;
} logorg;

/* exported disksim_logorg.c functions */

/* exported disksim_redun.c functions */

int  logorg_shadowed (logorg *currlogorg, ioreq_event *curr, int numreqs);
int  logorg_parity_disk (logorg *currlogorg, ioreq_event *curr, int numreqs);
int  logorg_parity_rotate (logorg *currlogorg, ioreq_event *curr, int numreqs);
int  logorg_parity_table (logorg *currlogorg, ioreq_event *curr, int numreqs);
void logorg_create_table (logorg *currlogorg);
int  logorg_tabular_rottype (int maptype, int reduntype, int rottype, int stripeunit);
int  logorg_check_dependencies (logorg *currlogorg, outstand *req, ioreq_event *curr);

// get the logorg (and number in *n) of the logorg called <name>
// or 0 if it doesn't exist
logorg *getlogorgbyname(logorg **, int, char *name, int *n);

INLINE void logorg_set_arraydisk(struct logorg *l, int n);

logorg *getlogorgbyname(logorg **logorgs, 
			int numlogorgs, 
			char *name, 
			int *n);

void logorg_cleanup(logorg *);
int logorg_addr(logorg *result, char *s);
int logorg_distn(logorg *result, char *s);
int logorg_redun(logorg *result, char *s);
int getlogorgdevs(logorg *result, struct lp_list *l);

#endif  /* DISKSIM_LOGORG_H */


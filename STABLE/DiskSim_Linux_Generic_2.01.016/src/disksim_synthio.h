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

#include "config.h"
#include "disksim_global.h"
#include "disksim_pfsim.h"
#include "disksim_iotrace.h"
#include "disksim_device.h"
#include "disksim_iodriver.h"
#include "disksim_logorg.h"


#ifndef DISKSIM_SYNTHIO_H
#define DISKSIM_SYNTHIO_H

/* functions exported by disksim_synthio.c */

struct process;


typedef struct dist {
   double base;
   double mean;
   double var;
   int    type;
} synthio_distr;


typedef struct gen {
   FILE          *tracefile;
   double         probseq;
   double         probloc;
   double         probread;
   double         probtmcrit;
   double         probtmlim;
   int            number;
   ioreq_event *  pendio;
   sleep_event *  limits;
   int            numdisks;
   int            *devno;
   int            numblocks;
   int            sectsperdisk;   // total sectors per disk
   int            blksperdisk;    // number of blocks per disk (block size = blocksize )
   int            blocksize;      // blocking factor
   synthio_distr  tmlimit;
   synthio_distr  genintr;
   synthio_distr  seqintr;
   synthio_distr  locintr;
   synthio_distr  locdist;
   synthio_distr  sizedist;
} synthio_generator;


int  loaddistn( struct lp_list *l, struct dist *result );
void synthio_readparams (FILE *parfile);
void synthio_initialize_generator (struct process *procp);
void synthio_resetstats (void);
void synthio_cleanstats (void);
void synthio_printstats (void);
void synthio_generate_io_activity (struct process *procp);
void synthio_param_override (char *paramname, char *paramval, int first, int last);
int  loadsynthdevs( synthio_generator * result, struct lp_list *l );
int  loadsynthgenerators( synthio_generator * junk, struct lp_list *l );


typedef struct synthio_info {
   synthio_generator **synthio_gens;
   int    synthio_gencnt;
   int    synthio_gens_len; /* allocated size of array */

   int    synthio_iocnt;
   int    synthio_endiocnt;
   double synthio_endtime;
   int    synthio_syscalls;
   double synthio_syscall_time;
   double synthio_sysret_time;
} synthio_info_t;


/* one remapping #define for each variable in synthio_info_t */
#define SYNTHIO_GENS            (disksim->synthio_info->synthio_gens)
#define SYNTHIO_GENCNT          (disksim->synthio_info->synthio_gencnt)
#define SYNTHIO_IOCNT           (disksim->synthio_info->synthio_iocnt)
#define SYNTHIO_ENDIOCNT        (disksim->synthio_info->synthio_endiocnt)
#define SYNTHIO_ENDTIME         (disksim->synthio_info->synthio_endtime)
#define SYNTHIO_SYSCALLS        (disksim->synthio_info->synthio_syscalls)
#define SYNTHIO_SYSCALL_TIME    (disksim->synthio_info->synthio_syscall_time)
#define SYNTHIO_SYSRET_TIME     (disksim->synthio_info->synthio_sysret_time)


#endif    /* DISKSIM_SYNTHIO_H */


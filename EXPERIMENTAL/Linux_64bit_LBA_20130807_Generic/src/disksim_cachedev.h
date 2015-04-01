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

#ifndef DISKSIM_CACHEDEV_H
#define DISKSIM_CACHEDEV_H

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_ioqueue.h"
#include "config.h"
#include "disksim_cache.h"

// Note: hurst_r maybe need to move these defines to the top level h file and rewrite code

/* cache event types */

#define CACHE_EVENT_IOREQ               0
#define CACHE_EVENT_READ                1
#define CACHE_EVENT_POPULATE_ONLY       2
#define CACHE_EVENT_POPULATE_ALSO       3
#define CACHE_EVENT_WRITE               4
#define CACHE_EVENT_FLUSH               5
#define CACHE_EVENT_IDLEFLUSH_READ      6
#define CACHE_EVENT_IDLEFLUSH_FLUSH     7

// cache write schemes (policies)
// When a system writes a datum to cache, it must at some point write that datum to backing store as well.
// The timing of this write is controlled by what is known as the write policy.
//
// There are two basic writing approaches:
//
//    Write-through - Write is done synchronously both to the cache and to the backing store.
//    Write-back (or Write-behind) - Initially, writing is done only to the cache. The write to the backing store is postponed until the cache blocks containing the data are about to be modified/replaced by new content.
//
// Write-back cache is more complex to implement, since it needs to track which of its locations have been written over, and mark them as dirty for later writing to the backing store. The data in these locations are written back to the backing store only when they are evicted from the cache, an effect referred to as a lazy write. For this reason, a read miss in a write-back cache (which requires a block to be replaced by another) will often require two memory accesses to service: one to write the replaced data from the cache back to the store, and then one to retrieve the needed datum.
// Other policies may also trigger data write-back. The client may make many changes to a datum in the cache, and then explicitly notify the cache to write back the datum.
// Since on write operations, no actual data are needed back, there are two approaches for situations of write-misses:
//
//    Write allocate (aka Fetch on write) - Datum at the missed-write location is loaded to cache, followed by a write-hit operation. In this approach, write misses are similar to read-misses.
//    No-write allocate (aka Write-no-allocate, Write around) - Datum at the missed-write location is not loaded to cache, and is written directly to the backing store. In this approach, only system reads are being cached.
//
// Both write-through and write-back policies can use either of these write-miss policies, but usually they are paired in this way:[2]
//
//    A write-back cache uses write allocate, hoping for subsequent writes (or even reads) to the same location, which is now cached.
//    A write-through cache uses no-write allocate. Here, subsequent writes have no advantage, since they still need to be written directly to the backing store.
//
// Entities other than the cache may change the data in the backing store, in which case the copy in the cache may become out-of-date or stale. Alternatively, when the client updates the data in the cache, copies of those data in other caches will become stale. Communication protocols between the cache managers which keep the data consistent are known as coherency protocols.

#define CACHE_WRITE_MIN         1
#define CACHE_WRITE_SYNCONLY    1
#define CACHE_WRITE_THRU        2
#define CACHE_WRITE_BACK        3
#define CACHE_WRITE_MAX         3

/* cache background flush types */

#define CACHE_FLUSH_MIN         0
#define CACHE_FLUSH_DEMANDONLY  0
#define CACHE_FLUSH_PERIODIC    1
#define CACHE_FLUSH_MAX         1



struct cache_dev_event {
   double time;
   int type;
   struct cache_dev_event *next;
   struct cache_dev_event *prev;
   void (**donefunc)(void *,ioreq_event *);	/* Function to call when complete */
   void *doneparam;		/* parameter for donefunc */
   int flags;
   ioreq_event *req;
   struct cache_dev_event *waitees;
   int validpoint;
};

struct cache_dev_stats {
   int reads;
   int readblocks;
   int readhitsfull;
   int readmisses;
   int popwrites;
   int popwriteblocks;
   int writes;
   int writeblocks;
   int writehitsfull;
   int writemisses;
   int destagereads;
   int destagereadblocks;
   int destagewrites;
   int destagewriteblocks;
   int maxbufferspace;
};


struct cache_dev {
  struct cache_if hdr;
   void (**issuefunc)(void *,ioreq_event *);	/* to issue a disk access    */
   void *issueparam;				/* first param for issuefunc */
   struct ioq * (**queuefind)(void *,int);	/* to get ioqueue ptr for dev*/
   void *queuefindparam;			/* first param for queuefind */
   // void (**wakeupfunc)(void *,void *);	/* to re-activate slept proc */
   void (**wakeupfunc)(void *, struct cacheevent *);	/* to re-activate slept proc */
   void *wakeupparam;				/* first param for wakeupfunc */
   int size;					/* in 512B blks  */
   int cache_devno;				/* device used for cache */
   int real_devno;				/* device for which cache is used */
   int maxreqsize;
   int writescheme;
   int flush_policy;
   double flush_period;
   double flush_idledelay;
   int bufferspace;
   struct cache_dev_event *ongoing_requests;
   bitstr_t *validmap;
   bitstr_t *dirtymap;
   struct cache_dev_stats stat;
   char *name;
};

void cachedev_setcallbacks(void);


#endif // DISKSIM_CACHEDEV_H




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


/***************************************************************************
  This is a fairly simple module for managing a device that is being used
  as a cache for another device.  It does not have many functionalities
  that a real device-caching-device would have, such as
	1. No actual "cache"-iness.  That is, it simply maps given locations
	   to the same locations on the cache device.  This limits the max
	   amount of real device space that can be cached, and misses
	   opportunities for creating cache-device locality where none
	   exists in the original workload.
	2. No prefetching.
	3. No optimization.  For example, see #1.  Also, no grouping of
	   writes to cache-device.  Also, no going to original device when
	   cache-device is busy.
	4. No locking.  Specifically, requests for the same space can be in
	   progress in parallel.  This could cause inconsistencies, with
	   unfortunate sequencing.
	5. No buffer space limitations.  Specifically, there is no limit on
	   the amount of buffer space used at once for managing the cache
	   and real devices.
***************************************************************************/


#define CACHE_DEVICE    2

#include "modules/modules.h"
#include "disksim_cachedev.h"






static int 
cachedev_get_maxreqsize (struct cache_if *c)
{
  struct cache_dev *cache = (struct cache_dev *)c;

#ifdef DEBUG_CACHEDEV
  fprintf(outputfile, "*** %f: Entered cachedev::cachedev_get_maxreqsize \tcache->maxreqsize=%d\n", simtime, cache->maxreqsize );
  fflush(outputfile);
#endif

  return(cache->maxreqsize);
}


static void cachedev_empty_donefunc (void *doneparam, ioreq_event *req)
{
#ifdef DEBUG_CACHEDEV
  fprintf(outputfile, "*** %f: Entered cachedev::cachedev_empty_donefunc - adding to extraq type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, req->type, req->devno, req->blkno, req->bcount, req->flags );
  fflush(outputfile);
#endif

  addtoextraq((event *) req);
}


//*****************************************************************************
// Function: cachedev_add_ongoing_request
//	Add cache device to the double linked list
//
// Parameters:
//	struct cache_dev *cache		pointer to a cache device
//	void *crq					pointer to a double link list
//
// Returns: void
//*****************************************************************************

static void 
cachedev_add_ongoing_request (struct cache_dev *cache, void *crq)
{
  struct cache_dev_event *cachereq = (struct cache_dev_event *)crq;

#ifdef DEBUG_CACHEDEV
  fprintf(outputfile, "*** %f: Entered cachedev::cachedev_add_ongoing_request", simtime );
  if( NULL != cachereq )
  {
      fprintf(outputfile, "  crq %p", cachereq );
      if( NULL != cachereq->req )
	  {
		 fprintf(outputfile, ", type %d, devno %d, blkno %d, bcount %d, flags 0x%x", cachereq->req->type, cachereq->req->devno, cachereq->req->blkno, cachereq->req->bcount, cachereq->req->flags  );
	  }
  }
  fprintf(outputfile, "\n" );
  fflush(outputfile);
#endif

   cachereq->next = cache->ongoing_requests;
   cachereq->prev = NULL;
   if (cachereq->next) {
      cachereq->next->prev = cachereq;
   }
   cache->ongoing_requests = cachereq;
}


static void 
cachedev_remove_ongoing_request (struct cache_dev *cache, struct cache_dev_event *cachereq)
{
#ifdef DEBUG_CACHEDEV
  fprintf(outputfile, "*** %f: Entered cachedev::cachedev_remove_ongoing_request", simtime );
  if( NULL != cachereq )
  {
      fprintf(outputfile, "  crq %p", cachereq );
  }
  fprintf(outputfile, "\n" );
  fflush(outputfile);
#endif

   if (cachereq->next) {
      cachereq->next->prev = cachereq->prev;
   }
   if (cachereq->prev) {
      cachereq->prev->next = cachereq->next;
   }
   if (cache->ongoing_requests == cachereq) {
      cache->ongoing_requests = cachereq->next;
   }
}


static struct cache_dev_event * cachedev_find_ongoing_request (struct cache_dev *cache, ioreq_event *req)
{
   struct cache_dev_event *tmp = cache->ongoing_requests;

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_find_ongoing_request, type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, req->type, req->devno, req->blkno, req->bcount, req->flags );
   fflush(outputfile);
#endif

   /* Is this enough to ensure equivalence?? */
   while ((tmp != NULL) && ((req->opid != tmp->req->opid) || (req->blkno != tmp->req->blkno) || (req->bcount != tmp->req->bcount) || (req->buf != tmp->req->buf))) {
      tmp = tmp->next;
   }

   return (tmp);
}


//*****************************************************************************
// Function: cachedev_count_dirty_blocks
//	Return a count of how many dirty blocks are in the cachedev
//  Searches dirtymap for set bits
//
// Parameters:
//	struct cache_dev *cache					pointer to a cache device
//
// Returns: void
//*****************************************************************************

static int cachedev_count_dirty_blocks (struct cache_dev *cache)
{
   int dirtyblocks = 0;
   int i;

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_count_dirty_blocks (dirtymap)\n", simtime );
   fflush(outputfile);
#endif

   for (i=0; i<cache->size; i++) {
      if (bit_test(cache->dirtymap,i)) {
         dirtyblocks++;
      }
   }
   return (dirtyblocks);
}


//*****************************************************************************
// Function: cachedev_setbits
//	Sets the bits in the bitmap corresponding to the LBA's in the ioreq_event
//
// Parameters:
//	bitstr_t *bitmap		pointer to a bitmap
//	ioreq_event *req		pointer to an ioreq_event
//
// Returns: void
//*****************************************************************************

static void cachedev_setbits (bitstr_t *bitmap, ioreq_event *req)
{
#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_setbits from block %d to %d\n", simtime, req->blkno, req->blkno+req->bcount-1  );
   fflush(outputfile);
#endif

   bit_nset (bitmap, req->blkno, (req->blkno+req->bcount-1));
}


//*****************************************************************************
// Function: cachedev_clearbits
//	Clears the bits in the bitmap corresponding to the LBA's in the ioreq_event
//
// Parameters:
//	bitstr_t *bitmap		pointer to a bitmap
//	ioreq_event *req		pointer to an ioreq_event
//
// Returns: void
//*****************************************************************************

static void cachedev_clearbits (bitstr_t *bitmap, ioreq_event *req)
{
#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_clearbits from block %d to %d\n", simtime, req->blkno, req->blkno+req->bcount-1 );
   fflush(outputfile);
#endif

   bit_nclear (bitmap, req->blkno, (req->blkno+req->bcount-1));
}


//*****************************************************************************
// Function: cachedev_isreadhit
//	Checks if the corresponding LBA's in the ioreq_event are stored in the cache device
//  A set bit in the validmap bitmap indicates data valid (no actual data stored)
//
// Parameters:
//	bitstr_t *bitmap		pointer to a bitmap
//	ioreq_event *req		pointer to an ioreq_event
//
// Returns: void
//*****************************************************************************

static int cachedev_isreadhit (struct cache_dev *cache, ioreq_event *req)
{
   int lastblk = req->blkno + req->bcount;
   int i;

   int readHit = 1;
   if (lastblk >= cache->size) {
      readHit = 0;
   }
   else
   {
	   for (i=req->blkno; i<lastblk; i++) {
		  if (bit_test(cache->validmap,i) == 0) {
			 readHit = 0;
			 break;
		  }
	   }
   }

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_isreadhit (%s)\n", simtime, readHit ? "yes" : "no" );
   fflush(outputfile);
#endif

   return readHit;
}


//*****************************************************************************
// Function: cachedev_iswritehit
//	Checks if the corresponding LBA's in the ioreq_event can be written to the cache device
//
// Parameters:
//	bitstr_t *bitmap		pointer to a bitmap
//	ioreq_event *req		pointer to an ioreq_event
//
// Returns: int
//   0		if LBA range is greater or equal to the maximum capacity of the disk
//   1      if LBA range is less than the maximum capacity of the disk
//*****************************************************************************

static int cachedev_iswritehit (struct cache_dev *cache, ioreq_event *req)
{
   int writeHit = 1;

   if ((req->blkno+req->bcount) >= cache->size) {
      writeHit = 0;
   }

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_iswritehit (%s)\n", simtime, writeHit ? "yes" : "no" );
   fflush(outputfile);
#endif

   return writeHit;
}


static int cachedev_find_dirty_cache_blocks (struct cache_dev *cache, int *blknoPtr, int *bcountPtr)
{
   int blkno;

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_find_dirty_cache_blocks\n", simtime );
   fflush(outputfile);
#endif

   bit_ffs (cache->dirtymap, cache->size, blknoPtr);
   if (*blknoPtr == -1) {
      return (0);
   }

   blkno = *blknoPtr;
   while (bit_test (cache->dirtymap, blkno) != 0) {
      blkno++;
   }
   *bcountPtr = blkno - *blknoPtr;

   return (1);
}


static void cachedev_periodic_callback (timer_event *timereq)
{
   fprintf (stderr, "cachedev_periodic_callback not yet supported\n");
   ASSERT (0);
   exit (0);
}


static void cachedev_idlework_callback (void *idleworkparam, int idledevno)
{

   struct cache_dev *cache = (struct cache_dev *)idleworkparam;
   struct cache_dev_event *flushdesc;
   ioreq_event *flushreq;
   int blkno, bcount;
   struct ioq *queue;

   ASSERT (idledevno == cache->real_devno);

#ifdef DEBUG_CACHEDEV
   fprintf(outputfile, "*** %f: Entered cachedev::cachedev_idlework_callback\n", simtime );
   fflush(outputfile);
#endif

   queue = (*cache->queuefind)(cache->queuefindparam, cache->real_devno);
   if (ioqueue_get_number_in_queue (queue) != 0) {
      return;
   }

   queue = (*cache->queuefind)(cache->queuefindparam, cache->cache_devno);
   if (ioqueue_get_number_in_queue (queue) != 0) {
      return;
   }

   if (cachedev_find_dirty_cache_blocks (cache, &blkno, &bcount) == 0) {
      return;
   }

   /* Just assume that bufferspace is available */
   cache->bufferspace += bcount;
   if (cache->bufferspace > cache->stat.maxbufferspace) {
      cache->stat.maxbufferspace = cache->bufferspace;
   }

   flushdesc = (struct cache_dev_event *) getfromextraq();
   flushdesc->type = CACHE_EVENT_IDLEFLUSH_READ;
   flushreq = (ioreq_event *) getfromextraq();
   flushreq->buf = flushdesc;
   flushreq->devno = cache->cache_devno;
   flushreq->blkno = blkno;
   flushreq->bcount = bcount;
   flushreq->type = IO_ACCESS_ARRIVE;
   flushreq->flags = READ;
   (*cache->issuefunc)(cache->issueparam, flushreq);
   cache->stat.destagereads++;
   cache->stat.destagereadblocks += bcount;
}


/* Gets the appropriate block, locked and ready to be accessed read or write */

static int 
cachedev_get_block (struct cache_if *c, 
		    ioreq_event *req, 
		    void (**donefunc)(void *, ioreq_event *), 
		    void *doneparam)
{
   struct cache_dev *cache = (struct cache_dev *)c;
   struct cache_dev_event *rwdesc = (struct cache_dev_event *) getfromextraq();
   ioreq_event *fillreq;
   int devno;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** cachedev::totalreqs = %d\n", disksim->totalreqs);
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_get_block: rw %d, devno %d, blkno %d, size %d\n", simtime, (req->flags & READ), req->devno, req->blkno, req->bcount);
#endif

   if (req->devno != cache->real_devno) {
      fprintf (stderr, "cachedev_get_block trying to cache blocks for wrong device (%d should be %d)\n", req->devno, cache->real_devno);
      ASSERT(0);
      exit(1);
   }

   /* Ignore request overlap and locking issues for now.  */
   /* Also ignore buffer space limitation issues for now. */
   cache->bufferspace += req->bcount;
   if (cache->bufferspace > cache->stat.maxbufferspace) {
      cache->stat.maxbufferspace = cache->bufferspace;
   }

   rwdesc->type = (req->flags & READ) ? CACHE_EVENT_READ : CACHE_EVENT_WRITE;
   rwdesc->donefunc = donefunc;
   rwdesc->doneparam = doneparam;
   rwdesc->req = req;
   req->next = NULL;
   req->prev = NULL;
   rwdesc->flags = 0;
   cachedev_add_ongoing_request (cache, rwdesc);

   if (req->flags & READ) {
      cache->stat.reads++;
      cache->stat.readblocks += req->bcount;

      /* Send read straight to whichever device has it (preferably cachedev). */
      if (cachedev_isreadhit (cache, req)) {
         devno = cache->cache_devno;
         cache->stat.readhitsfull++;
      } else {
         devno = cache->real_devno;
         cache->stat.readmisses++;
      }
      /* For now, just assume both device's store bits at same LBNs */
      fillreq = ioreq_copy (req);
      fillreq->buf = rwdesc;
      fillreq->type = IO_ACCESS_ARRIVE;
      fillreq->devno = devno;

#ifdef DEBUG_CACHEDEV
      fprintf (outputfile, "*** %f: cachedev::fillreq (inserts into ioqueue?): devno %d, blkno %d, bcount %d, flags %x, buf %p\n", simtime, fillreq->devno, fillreq->blkno, fillreq->bcount, fillreq->flags, fillreq->buf);
#endif

      (*cache->issuefunc)(cache->issueparam, fillreq);
      return (1);

   } else {
      /* Grab buffer space and let the controller fill in data to be written. */
      /* (for now, just assume that there is buffer space available)          */

      (*donefunc)(doneparam, req);
      return (0);
   }
}


/* frees the block after access complete, block is clean so remove locks */
/* and update lru                                                        */

static void 
cachedev_free_block_clean (struct cache_if *c, 
			   ioreq_event *req)
{
  struct cache_dev *cache = (struct cache_dev *)c;
   struct cache_dev_event *rwdesc;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cache_free_block_clean: blkno %d, bcount %d, devno %d\n", simtime, req->blkno, req->bcount, req->devno);
#endif

   /* For now, just find relevant rwdesc and free it.                       */
   /* Later, write it to the cache device (and update the cache map thusly. */

   rwdesc = cachedev_find_ongoing_request (cache, req);
   ASSERT (rwdesc != NULL);

   if (rwdesc->type == CACHE_EVENT_READ) {
      cache->bufferspace -= req->bcount;
      cachedev_remove_ongoing_request (cache, rwdesc);
      addtoextraq ((event *) rwdesc);
   } else {
      ASSERT (rwdesc->type == CACHE_EVENT_POPULATE_ALSO);
      rwdesc->type = CACHE_EVENT_POPULATE_ONLY;
   }
}


/* a delayed write - set dirty bits, remove locks and update lru.        */
/* If cache doesn't allow delayed writes, forward this to async          */

static int 
cachedev_free_block_dirty (struct cache_if *c, 
			   ioreq_event *req, 
			   void (**donefunc)(void *, ioreq_event *), 
			   void *doneparam)
{
  struct cache_dev *cache = (struct cache_dev *)c;
   ioreq_event *flushreq;
   struct cache_dev_event *writedesc;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f, Entered cachedev::cache_free_block_dirty: devno %d blkno %d, size %d\n", simtime, req->devno, req->blkno, req->bcount);
#endif

   cache->stat.writes++;
   cache->stat.writeblocks += req->bcount;

   writedesc = cachedev_find_ongoing_request (cache, req);
   ASSERT (writedesc != NULL);
   ASSERT (writedesc->type == CACHE_EVENT_WRITE);

   writedesc->donefunc = donefunc;
   writedesc->doneparam = doneparam;
   writedesc->req = req;
   req->type = IO_REQUEST_ARRIVE;
   req->next = NULL;
   req->prev = NULL;

   /* For now, just assume both device's store bits at same LBNs */
   flushreq = ioreq_copy(req);
   flushreq->type = IO_ACCESS_ARRIVE;
   flushreq->buf = writedesc;
   if (cachedev_iswritehit (cache, req)) {
      flushreq->devno = cache->cache_devno;
      cache->stat.writehitsfull++;
   } else {
      cache->stat.writemisses++;
   }

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: cachedev::cachedev_free_block_dirty flushreq: devno %d, blkno %d, bcount %d, buf %p\n", simtime, flushreq->devno, flushreq->blkno, flushreq->bcount, flushreq->buf);
#endif

   (*cache->issuefunc)(cache->issueparam, flushreq);

#if 0
   if (cache->flush_idledelay >= 0.0) {
      ioqueue_reset_idledetecter((*cache->queuefind)(cache->queuefindparam, req->devno), 0);
   }
#endif

   return(1);
}


int cachedev_sync (struct cache_if *c)
{
#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %lf: cachedev::cachedev_sync (does nothing)\n", simtime );
#endif

  return(0);
}


static void *
cachedev_disk_access_complete (struct cache_if *c,
			       ioreq_event *curr)
{
  struct cache_dev *cache = (struct cache_dev *)c;
   struct cache_dev_event *rwdesc = (struct cache_dev_event *)curr->buf;
   struct cache_dev_event *tmp = NULL;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cache_disk_access_complete: cacheDevEventType %d, buf %p, type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, rwdesc->type, curr->buf, curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags);
#endif

   switch(rwdesc->type) {
   case CACHE_EVENT_READ:
      /* Consider writing same buffer to cache_devno, in order to populate it.*/
      /* Not clear whether it is more appropriate to do it from here or from  */
      /* "free_block_clean" -- do it here for now to get more overlap.        */
      if (curr->devno == cache->real_devno) {
         ioreq_event *flushreq = ioreq_copy(rwdesc->req);
         flushreq->type = IO_ACCESS_ARRIVE;
         flushreq->buf = rwdesc;
         flushreq->flags = WRITE;
         flushreq->devno = cache->cache_devno;
         rwdesc->type = CACHE_EVENT_POPULATE_ALSO;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cache_disk_access_complete - flushing memory cache to disk: type %d, devno %d, blkno %d, bcount %d, flags 0x%x, buf %p\n", simtime, flushreq->type, flushreq->devno, flushreq->blkno, flushreq->bcount, flushreq->flags, flushreq->buf);
#endif
         (*cache->issuefunc)(cache->issueparam, flushreq);
         cache->stat.popwrites++;
         cache->stat.popwriteblocks += rwdesc->req->bcount;
      }

      /* Ongoing read request can now proceed, so call donefunc from get_block*/
      (*rwdesc->donefunc)(rwdesc->doneparam,rwdesc->req);

      break;
   case CACHE_EVENT_WRITE:

      /* finished writing to cache-device */
      if (curr->devno == cache->cache_devno) {
         cachedev_setbits (cache->validmap, curr);
         cachedev_setbits (cache->dirtymap, curr);
         if (cache->writescheme == CACHE_WRITE_THRU) {
            ioreq_event *flushreq = ioreq_copy(rwdesc->req);
            flushreq->type = IO_ACCESS_ARRIVE;
            flushreq->buf = rwdesc;
            flushreq->flags = WRITE;
            flushreq->devno = cache->real_devno;
            rwdesc->type = CACHE_EVENT_FLUSH;
            (*cache->issuefunc)(cache->issueparam, flushreq);
            cache->stat.destagewrites++;
            cache->stat.destagewriteblocks += rwdesc->req->bcount;
         }
      }
      (*rwdesc->donefunc)(rwdesc->doneparam,rwdesc->req);

      if (rwdesc->type != CACHE_EVENT_FLUSH) {
         cachedev_remove_ongoing_request (cache, rwdesc);
         addtoextraq ((event *) rwdesc);
         cache->bufferspace -= curr->bcount;
      }

      break;

   case CACHE_EVENT_POPULATE_ONLY:
     cachedev_setbits (cache->validmap, curr);
     cachedev_remove_ongoing_request (cache, rwdesc);
      addtoextraq ((event *) rwdesc);
      cache->bufferspace -= curr->bcount;
      break;
      
   case CACHE_EVENT_POPULATE_ALSO:
     cachedev_setbits (cache->validmap, curr);
     rwdesc->type = CACHE_EVENT_READ;
     break;

   case CACHE_EVENT_FLUSH:
      cachedev_clearbits (cache->dirtymap, curr);
      cachedev_remove_ongoing_request (cache, rwdesc);
      addtoextraq ((event *) rwdesc);
      cache->bufferspace -= curr->bcount;
      break;

   case CACHE_EVENT_IDLEFLUSH_READ:
     {
       ioreq_event *flushreq = ioreq_copy (curr);
       flushreq->type = IO_ACCESS_ARRIVE;
       flushreq->flags = WRITE;
       flushreq->devno = cache->real_devno;
       rwdesc->type = CACHE_EVENT_IDLEFLUSH_FLUSH;
       (*cache->issuefunc)(cache->issueparam, flushreq);
       cache->stat.destagewrites++;
       cache->stat.destagewriteblocks += curr->bcount;
     } break;

   case CACHE_EVENT_IDLEFLUSH_FLUSH:
     cachedev_clearbits (cache->dirtymap, curr);
     cachedev_remove_ongoing_request (cache, rwdesc);
     addtoextraq ((event *) rwdesc);
     cachedev_idlework_callback (cache, curr->devno);
     cache->bufferspace -= curr->bcount;
     break;

   default:
     ddbg_assert2(0, "Unknown cachedev event type");
     break;
   }

   addtoextraq((event *) curr);

   /* returned cacheevent will get forwarded to cachedev_wakeup_continue... */
   return(tmp);
}


static void 
cachedev_wakeup_complete (struct cache_if *c, 
			  void *d) // really struct cache_dev_event
{
  struct cache_dev_event *desc = (struct cache_dev_event *)d;
  struct cache_dev *cache = (struct cache_dev *)c;
  ASSERT (0);

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_wakeup_complete (does nothing)\n", simtime );
#endif

  // ???

#if 0
   switch(desc->type) {
   case CACHE_EVENT_READ:
      cache_read_continue(cache, desc);
      break;
   case CACHE_EVENT_WRITE:
     cache_write_continue(cache, desc);
     break;
   case CACHE_EVENT_FLUSH:
      (*desc->donefunc)(desc->doneparam, desc->req);
      addtoextraq((event *) desc);
      break;

   default:
     ddbg_assert2(0, "Unknown cachedev event type");
     break;
   }
#endif
}


static void 
cachedev_resetstats (struct cache_if *c)
{
  struct cache_dev *cache = (struct cache_dev *)c;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_resetstats\n", simtime );
#endif

  cache->stat.reads = 0;
  cache->stat.readblocks = 0;
  cache->stat.readhitsfull = 0;
  cache->stat.readmisses = 0;
  cache->stat.popwrites = 0;
  cache->stat.popwriteblocks = 0;
  cache->stat.writes = 0;
  cache->stat.writeblocks = 0;
  cache->stat.writehitsfull = 0;
  cache->stat.writemisses = 0;
  cache->stat.destagereads = 0;
  cache->stat.destagereadblocks = 0;
  cache->stat.destagewrites = 0;
  cache->stat.destagewriteblocks = 0;
  cache->stat.maxbufferspace = 0;
}


void 
cachedev_setcallbacks(void)
{
#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_setcallbacks\n", simtime );
#endif

  disksim->donefunc_cachedev_empty = cachedev_empty_donefunc;
  disksim->idlework_cachedev       = cachedev_idlework_callback;
  disksim->timerfunc_cachedev      = cachedev_periodic_callback;
}


static void 
cachedev_initialize (struct cache_if *c, 
		     void (**issuefunc)(void *,ioreq_event *), 
		     void *issueparam, 
		     struct ioq * (**queuefind)(void *,int), 
		     void *queuefindparam, 
		     void (**wakeupfunc)(void *, struct cacheevent *), 
		     void *wakeupparam, 
		     int numdevs)
{
  struct cache_dev *cache = (struct cache_dev *)c;
  StaticAssert (sizeof(struct cache_dev_event) <= DISKSIM_EVENT_SIZE);

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_initialize\n", simtime );
#endif

   cache->issuefunc = issuefunc;
   cache->issueparam = issueparam;
   cache->queuefind = queuefind;
   cache->queuefindparam = queuefindparam;
   cache->wakeupfunc = wakeupfunc;
   cache->wakeupparam = wakeupparam;
   cache->bufferspace = 0;
   cache->ongoing_requests = NULL;
   bzero (cache->validmap, bitstr_size(cache->size));
   bzero (cache->dirtymap, bitstr_size(cache->size));
   cachedev_resetstats(c);

   if (cache->flush_idledelay > 0.0) {
      struct ioq *queue = (*queuefind)(queuefindparam,cache->real_devno);
      ASSERT (queue != NULL);
      ioqueue_set_idlework_function (queue, 
				     &disksim->idlework_cachedev, 
				     cache, 
				     cache->flush_idledelay);
   }

   if (device_get_number_of_blocks(cache->cache_devno) < cache->size) {
      fprintf (stderr, "Size of cachedev exceeds that of actual cache device (devno %d): %d > %d\n", cache->cache_devno, cache->size, device_get_number_of_blocks(cache->cache_devno));
      ddbg_assert(0);
   }
}


static void 
cachedev_cleanstats (struct cache_if *cache)
{
#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_cleanstats (does nothing)\n", simtime );
#endif
}


static void 
cachedev_printstats (struct cache_if *c, char *prefix)
{
  struct cache_dev *cache = (struct cache_dev *)c;
   int reqs = cache->stat.reads + cache->stat.writes;
   int blocks = cache->stat.readblocks + cache->stat.writeblocks;

   fprintf (outputfile, "%scache requests:             %6d\n", prefix, reqs);
   if (reqs == 0) {
      return;
   }

   fprintf (outputfile, "%scache read requests:        %6d  \t%6.4f\n", prefix, cache->stat.reads, ((double) cache->stat.reads / (double) reqs));

   if (cache->stat.reads) {
     fprintf(outputfile, "%scache blocks read:           %6d  \t%6.4f\n", prefix, cache->stat.readblocks, ((double) cache->stat.readblocks / (double) blocks));
     fprintf(outputfile, "%scache read misses:          %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.readmisses, ((double) cache->stat.readmisses / (double) reqs), ((double) cache->stat.readmisses / (double) cache->stat.reads));
    
 fprintf(outputfile, "%scache read full hits:       %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.readhitsfull, ((double) cache->stat.readhitsfull / (double) reqs), ((double) cache->stat.readhitsfull / (double) cache->stat.reads));

     fprintf(outputfile, "%scache population writes:         %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.popwrites, ((double) cache->stat.popwrites / (double) reqs), ((double) cache->stat.popwrites / (double) cache->stat.reads));

     fprintf(outputfile, "%scache block population writes:    %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.popwriteblocks, ((double) cache->stat.popwriteblocks / (double) blocks), ((double) cache->stat.popwriteblocks / (double) cache->stat.readblocks));
   }

   fprintf(outputfile, "%scache write requests:       %6d  \t%6.4f\n", prefix, cache->stat.writes, ((double) cache->stat.writes / (double) reqs));

   if (cache->stat.writes) {
     fprintf(outputfile, "%scache blocks written:        %6d  \t%6.4f\n", prefix, cache->stat.writeblocks, ((double) cache->stat.writeblocks / (double) blocks));
     fprintf(outputfile, "%scache write misses:         %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.writemisses, ((double) cache->stat.writemisses / (double) reqs), ((double) cache->stat.writemisses / (double) cache->stat.writes));

      fprintf(outputfile, "%scache full write hits:   %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.writehitsfull, ((double) cache->stat.writehitsfull / (double) reqs), ((double) cache->stat.writehitsfull / (double) cache->stat.writes));

      fprintf(outputfile, "%scache destage pre-reads:     %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.destagereads, ((double) cache->stat.destagereads / (double) reqs), ((double) cache->stat.destagereads / (double) cache->stat.writes));
      fprintf(outputfile, "%scache block destage pre-reads: %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.destagereadblocks, ((double) cache->stat.destagereadblocks / (double) blocks), ((double) cache->stat.destagereadblocks / (double) cache->stat.writeblocks));

      fprintf(outputfile, "%scache destages (write):     %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.destagewrites, ((double) cache->stat.destagewrites / (double) reqs), ((double) cache->stat.destagewrites / (double) cache->stat.writes));

      fprintf(outputfile, "%scache block destages (write): %6d  \t%6.4f  \t%6.4f\n", prefix, cache->stat.destagewriteblocks, ((double) cache->stat.destagewriteblocks / (double) blocks), ((double) cache->stat.destagewriteblocks / (double) cache->stat.writeblocks));

      fprintf(outputfile, "%scache end dirty blocks:      %6d  \t%6.4f\n", prefix, cachedev_count_dirty_blocks(cache), ((double) cachedev_count_dirty_blocks(cache) / (double) cache->stat.writeblocks));
   }

   fprintf (outputfile, "%scache bufferspace use end:             %6d\n", prefix, cache->bufferspace);

   fprintf (outputfile, "%scache bufferspace use max:             %6d\n", prefix, cache->stat.maxbufferspace);
}


//*****************************************************************************
// Function: cachedev_copy
//   Creates a new copy of cache_if in memory and returns a pointer to it
//
// Parameters:
//   struct cache_if *c		data we wish to copy 
//
// Returns: struct cache_if *
//*****************************************************************************

static struct cache_if * 
cachedev_copy (struct cache_if *c)
{
  struct cache_dev *cache    = (struct cache_dev *)c;
  struct cache_dev *newCache = (struct cache_dev *) DISKSIM_malloc(sizeof(struct cache_dev));

  ASSERT(newCache != NULL);

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::cachedev_copy\n", simtime );
#endif

  bzero (newCache, sizeof(struct cache_dev));
  
  newCache->issuefunc = cache->issuefunc;
  newCache->issueparam = cache->issueparam;
  newCache->queuefind = cache->queuefind;
  newCache->queuefindparam = cache->queuefindparam;
  newCache->wakeupfunc = cache->wakeupfunc;
  newCache->wakeupparam = cache->wakeupparam;
  newCache->size = cache->size;
  newCache->maxreqsize = cache->maxreqsize;
  
  return (struct cache_if *)newCache;
}

static struct cache_if disksim_cache_dev = {
  cachedev_setcallbacks,
  cachedev_initialize,
  cachedev_resetstats,
  cachedev_printstats,
  cachedev_cleanstats,
  cachedev_copy,
  cachedev_get_block,
  cachedev_free_block_clean,
  cachedev_free_block_dirty,
  cachedev_disk_access_complete,
  cachedev_wakeup_complete,
  cachedev_sync,
  cachedev_get_maxreqsize
};

struct cache_if *disksim_cachedev_loadparams(struct lp_block *b)
{
  int c;
  struct cache_dev *result;

#ifdef DEBUG_CACHEDEV
   fprintf (outputfile, "*** %f: Entered cachedev::disksim_cachedev_loadparams\n", simtime );
#endif

  result = (struct cache_dev *)calloc(1, sizeof(struct cache_dev));
  result->hdr = disksim_cache_dev;
 

  result->name = b->name ? strdup(b->name) : 0;

    
  //#include "modules/disksim_cachedev_param.c"
  lp_loadparams(result, b, &disksim_cachedev_mod);

  return (struct cache_if *)result;
}

// #End of file: disksim_cachedev.c


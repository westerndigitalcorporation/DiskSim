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


/* TODO
 *
 * Replace curr->time with simtime?  Is this a problem?
 * Remove lastreq_lbn?
 */

#include <stdio.h>
#include <stdlib.h>

#include "mems_global.h"
#include "mems_disksim.h"
#include "mems_mapping.h"
#include "mems_internals.h"
#include "mems_buffer.h"

#include "disksim_bus.h"
#include "disksim_ioqueue.h"

// #define VERBOSE_EVENTLOOP

unsigned int mems_reqinfo_mallocs = 0;
unsigned int mems_reqinfo_frees   = 0;

unsigned int mems_extent_mallocs  = 0;
unsigned int mems_extent_frees    = 0;

/************************************************************************
 * Bus stuff
 ************************************************************************/

static void
mems_send_event_up_path (ioreq_event * curr,
			 double delay)
{
  /* Acquire the bus (if not already acquired), then use bus_delay
   * to send the event up the path.
   */

  mems_t *dev   = getmems(curr->devno);
  int     busno  = mems_get_busno(curr);
  int     slotno = mems_get_slotno(curr->devno);

  /* Put new request at head of buswait queue */
  curr->next     = dev->buswait;
  dev->buswait   = curr;
  curr->tempint1 = busno;
  curr->time     = delay;

  if (dev->busowned == -1)
    {
      /* Must get ownership of bus first */
      if (bus_ownership_get(busno, slotno, curr) == FALSE) {
	/* FIXME: dev->stat.requestedbus = simtime */
      }
      else
	{
	  bus_delay(busno, DEVICE, curr->devno, delay, curr);
	}
    }
  else if (dev->busowned == busno)
    {
      /* Already own the bus, so send it on up! */
      bus_delay(busno, DEVICE, curr->devno, delay, curr);
    }
  else
    {
      fprintf(stderr, "Wrong bus owned for transfer desired!\n");
      exit(1);
    }
}

void
mems_bus_ownership_grant (int devno,
			  ioreq_event *curr,
			  int busno,
			  double arbdelay)
{
  mems_t *dev = getmems(devno);
  ioreq_event *tmp = dev->buswait;
  
#ifdef VERBOSE_EVENTLOOP
  printf("Hi! I'm in mems_bus_ownership_grant: %f\n", simtime);
  fflush(stdout);
#endif

  while ((tmp != NULL) && (tmp != curr)) tmp = tmp->next;

  if (!tmp)
    {
      fprintf(stderr,
	      "Bus ownership granted to unknown request -- devno %d, busno %d\n",
	      devno, busno);
      exit(1);
    }

  dev->busowned = busno;

  /* FIXME: Do we _really_ want to keep this statistic? Is it valid? */
  // dev->stat.waitingforbus += simtime - dev->stat.requestedbus;
  // dev->stat.numbuswaits++;

  bus_delay(busno, DEVICE, dev->devno, tmp->time, curr);
}

void
mems_bus_delay_complete (int devno,
			 ioreq_event *curr,
			 int sentbusno)
{
  mems_t *dev = getmems(devno);
  intchar slotno;
  intchar busno;
  int     depth;

#ifdef VERBOSE_EVENTLOOP
  printf("Hello! I'm in mems_bus_delay_complete: %f\n", simtime);
  fflush(stdout);
#endif

  busno.value  = curr->busno;
  slotno.value = curr->slotno;
  depth = mems_get_depth(curr->devno);
  slotno.byte[depth] = slotno.byte[depth] >> 4;

  curr->time = (double)0.0;

  if (curr == dev->buswait)
    {
      dev->buswait = curr->next;
    }
  else
    {
      ioreq_event *tmp = dev->buswait;
      while ((tmp->next != NULL) && (tmp->next != curr)) tmp = tmp->next;
      if (tmp->next != curr)
	{
	  fprintf(stderr,
		  "Bus delay complete for unknown request -- devno %d, busno %d\n",
		  devno, busno.value);
	  exit(1);
	}
    }

  if (depth == 0)
    {
      intr_request ((event *)curr);
    }
  else
    {
      bus_deliver_event (busno.byte[depth], slotno.byte[depth], curr);
    }
}

/************************************************************************
 * Other support functions
 ************************************************************************/

mems_extent_t *
mems_get_new_extent(ioreq_event *curr)
{
  mems_extent_t *new_extent = (mems_extent_t *)malloc(sizeof(mems_extent_t));
  // mems_extent_t *new_extent = (mems_extent_t *)getfromextraq();
  mems_extent_mallocs++;

  new_extent->firstblock = curr->blkno;
  new_extent->lastblock  = curr->blkno + curr->bcount - 1;
  new_extent->completed_block_media = -1;
  new_extent->completed_block_bus = -1;
  new_extent->bus_done = FALSE;
  new_extent->media_done = FALSE;
  new_extent->next = NULL;
  new_extent->prev = NULL;
  new_extent->request = curr;
  new_extent->next_block_start = -1;
  new_extent->next_block_end = -1;

  return new_extent;
}

void
mems_update_reqinfo(ioreq_event *curr)
{
  mems_reqinfo_t *r = (mems_reqinfo_t *)curr->mems_reqinfo;
  mems_extent_t *extent_ptr = r->extents;
  ioreq_event *event_ptr;
  ioreq_event *next_event_ptr;

#ifdef VERBOSE_EVENTLOOP
  printf("mems_update_reqinfo -- entry\n");
#endif

  event_ptr = curr;

  while(event_ptr)
    {
      next_event_ptr = event_ptr->batch_next;
      free(((mems_reqinfo_t *)event_ptr->mems_reqinfo)->extents);
      mems_reqinfo_frees++;
      if (event_ptr->mems_reqinfo != r) 
	{
	  free(event_ptr->mems_reqinfo);
	  mems_reqinfo_frees++;
	  //addtoextraq(event_ptr->mems_reqinfo);
	}
      event_ptr = next_event_ptr;
    }

  r->extents = mems_get_new_extent(curr);
  r->num_extents = 1;

  r->batch_arrival_time = curr->time;

#ifdef VERBOSE_EVENTLOOP
  printf("mems_update_reqinfo -- curr->flags = %d\n", curr->flags);
#endif

  /*
  if ((curr->flags & 0x1) == DISKSIM_WRITE)
    {
#ifdef VERBOSE_EVENTLOOP
      printf("mems_update_reqinfo -- this is a write\n");
#endif
      r->extents->completed_block_bus = r->extents->lastblock;
      r->extents->bus_done = TRUE;
#ifdef VERBOSE_EVENTLOOP
      printf("mems_update_reqinfo -- completed_block_bus = %d\n",
	     r->extents->completed_block_bus);
#endif
    }
  */

  extent_ptr = r->extents;
  event_ptr = curr->batch_next;

  while (event_ptr != NULL)
    {
      extent_ptr->next = mems_get_new_extent(event_ptr);
      extent_ptr->next->prev = extent_ptr;

      if ((event_ptr->flags & 0x1) == DISKSIM_WRITE)
	{
#ifdef VERBOSE_EVENTLOOP
	  printf("mems_update_reqinfo -- this is a write\n");
#endif
	  extent_ptr->next->completed_block_bus = extent_ptr->next->lastblock;
	  extent_ptr->next->bus_done = TRUE;
#ifdef VERBOSE_EVENTLOOP
	  printf("mems_update_reqinfo -- completed_block_bus = %d\n",
		 extent_ptr->next->completed_block_bus);
#endif
	}
      r->num_extents++;
      if (curr->time < r->batch_arrival_time)
	{
	  r->batch_arrival_time = curr->time;
	}
      event_ptr->mems_reqinfo = NULL;
      event_ptr = event_ptr->batch_next;
      extent_ptr = extent_ptr->next;
    }
}

static mems_reqinfo_t *
mems_get_new_reqinfo (ioreq_event *curr)
{
  ioreq_event *event_ptr;
  mems_extent_t *extent_ptr;

  // mems_reqinfo_t *r = (mems_reqinfo_t *)getfromextraq();
  mems_reqinfo_t *r = (mems_reqinfo_t *)malloc(sizeof(mems_reqinfo_t));
  mems_reqinfo_mallocs++;

  extent_ptr = r->extents;

  r->extents = mems_get_new_extent(curr);
  r->num_extents = 1;

  extent_ptr = r->extents;
  event_ptr = curr;
  // if there are more in the batch  (there shouldn't be)
  while (event_ptr->batch_next != NULL)
    {
      event_ptr = event_ptr->batch_next;
      extent_ptr->next = mems_get_new_extent(event_ptr);
      r->num_extents++;
    }

  r->bus_pending = FALSE;
  r->bus_done    = FALSE;
  r->media_done  = FALSE;

  r->bus_extent = NULL;

  r->request_energy_uj = 0.0;
  r->firstseek            = 1;	/* TRUE: first seek hasn't yet completed */
  r->subtrack_access_num  = 0;
  r->subtrack_access_tips = 0;

  return r;
}

static struct mems_prefetch_info *
mems_get_prefetch_info (ioreq_event *curr)
{
  mems_sled_t    *sled    = (mems_sled_t *)curr->mems_sled;
  mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)curr->mems_reqinfo;
  struct mems_prefetch_info *p;

  if (!sled->prefetch_depth) return NULL;

  p = (struct mems_prefetch_info *)getfromextraq();

  p->firstblock = reqinfo->lastblock + 1;
  p->lastblock  = p->firstblock + sled->prefetch_depth - 1;

  p->completed_block  = -1;
  p->next_block_start = -1;
  p->next_block_end   = -1;

  return p;
}


static void
send_disconnect (ioreq_event *curr,
		 double latency)
{
  ioreq_event *intrp = ioreq_copy(curr);
  intrp->type = IO_INTERRUPT_ARRIVE;
  intrp->cause = DISCONNECT;
  mems_send_event_up_path(intrp, latency);
}

static void
send_completion (ioreq_event *curr,
		 double latency)
{
  ioreq_event *intrp = ioreq_copy(curr);
  intrp->type = IO_INTERRUPT_ARRIVE;
  intrp->cause = COMPLETION;
  mems_send_event_up_path(intrp, latency);
}

static void
send_reconnect (ioreq_event *curr,
		double latency)
{
  ioreq_event *intrp = ioreq_copy(curr);
  intrp->type = IO_INTERRUPT_ARRIVE;
  intrp->cause = RECONNECT;
  mems_send_event_up_path(intrp, latency);
}

static void
mems_request_complete (ioreq_event *curr,
		       double latency)
{
  mems_sled_t    *sled    = ( mems_sled_t *)curr->mems_sled;
  mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)curr->mems_reqinfo;
  ioreq_event *batch_ptr;
  ioreq_event *next_ptr;

#ifdef VERBOSE_EVENTLOOP
  printf("Entering mems_request_complete() - curr->blkno = %d, time = %f, latency = %f\n",
	 curr->blkno, simtime, latency);
  fflush(stdout);
#endif

  {	/* Verify that we're not insane here */
    mems_reqinfo_t *r = (mems_reqinfo_t *)curr->mems_reqinfo;
    // assert ((r->bus_done == TRUE) && (r->media_done == TRUE));
  }

  if (!ioqueue_physical_access_done(sled->queue, curr))
    {
      fprintf(stderr, "mems_request_complete: sled ioreq_event not found by ioqueue_physical_access_done call\n");
      exit(1);
    }
  /*
    if (!ioqueue_physical_access_done(sled->dev->queue, curr)) {
    fprintf(stderr, "mems_request_complete: device ioreq_event not found by ioqueue_physical_access_done call\n");
    exit(1);
  }
  */

  send_completion(curr, latency);

  
  if (reqinfo && reqinfo->subtrack_access_num)
    {
      /* Don't keep statistics for requests that hit entirely in the buffer
       * WARNING: If you _want_ to keep these, note that the 
       * stat.tips_per_access statistic will die with a divide-by-zero
       * error unless you do something else here */
      
      stat_update(&sled->dev->stat.request_energy_uj, 
		  reqinfo->request_energy_uj);
      stat_update(&sled->stat.request_energy_uj, 
		  reqinfo->request_energy_uj);
      
      stat_update(&sled->dev->stat.subtrack_accesses, 
		  reqinfo->subtrack_access_num);
      stat_update(&sled->stat.subtrack_accesses, 
		  reqinfo->subtrack_access_num);
      
      stat_update(&sled->dev->stat.tips_per_access,
		  reqinfo->subtrack_access_tips / reqinfo->subtrack_access_num);
      stat_update(&sled->stat.tips_per_access,
		  reqinfo->subtrack_access_tips / reqinfo->subtrack_access_num); 
    }
}

void
print_extent_info(mems_reqinfo_t *reqinfo)
{
  int i;
  mems_extent_t *extent_ptr;

  extent_ptr = reqinfo->extents;

  i = 0;

  while(extent_ptr != NULL)
    {
      printf(" extent %d, firstblock = %d, lastblock = %d\n",
	     i, extent_ptr->firstblock, extent_ptr->lastblock);
      printf(" next_block_start = %d, next_block_end = %d\n",
	     extent_ptr->next_block_start, extent_ptr->next_block_end);
      printf(" completed_block_media = %d, completed_block_bus = %d\n",
	     extent_ptr->completed_block_media, extent_ptr->completed_block_bus);
      printf(" media_done = %d, bus_done = %d\n",
	     extent_ptr->media_done, extent_ptr->bus_done);
      extent_ptr = extent_ptr->next;
      i++;
    }
}

/************************************************************************
 * Energy updating functions
 ************************************************************************/

void
mems_energy_update_device_overhead_complete_inactive(mems_sled_t *sled)
{
  double energy_j;
  
  /* Account for both energy consumed during inactive period... */
  energy_j = 
    (sled->inactive_power_mw * (simtime - sled->lastreq_comptime)) / 
    1000000.0;
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  
  sled->dev->stat.inactive_energy_j += energy_j;
  sled->stat.inactive_energy_j      += energy_j;
  
  /* ...and energy consumed during sled startup. */
  energy_j = 
    (sled->active_power_mw * sled->startup_time_ms) / 1000000.0;
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  
  sled->dev->stat.startup_energy_j += energy_j;
  sled->stat.startup_energy_j      += energy_j;
  /* FIXME: Should request_energy be updated with startup energy? */

}

void
mems_energy_update_device_overhead_complete_active(mems_sled_t *sled,
						   double elapsed_time)
{
  double energy_j = (sled->active_power_mw * elapsed_time) / 1000000.0;
  
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  
  sled->dev->stat.idle_energy_j += energy_j;
  sled->stat.idle_energy_j      += energy_j;
}

void
mems_energy_update_sled_seek(mems_sled_t *sled,
			     double timedelta)
{
  double energy_j = (sled->active_power_mw * timedelta) / 1000000.0;
  
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      reqinfo->request_energy_uj += energy_j * 1000000.0;
    }
  
  if (sled->active_request || sled->prefetch_info)
    {
      sled->dev->stat.servicing_energy_j += energy_j;
      sled->stat.servicing_energy_j += energy_j;
    }
  else
    {
      sled->dev->stat.idle_energy_j += energy_j;
      sled->stat.idle_energy_j      += energy_j;
    }
}

void
mems_energy_update_sled_servo(mems_sled_t *sled,
			      double timedelta)
{
  /* Account for both sled positioning energy and tip dissipation. */
  // int num_tips = sled->tipset.tip_end - sled->tipset.tip_start + 1;
  int num_tips = sled->tipset.num_tips;
  double energy_j =
    ((sled->active_power_mw * timedelta) +
     (sled->tip_power_mw * num_tips * timedelta)) / 1000000.0;
  
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      int num_tips_req = sled->tip_sectors_per_lbn *
	(reqinfo->next_block_end - reqinfo->next_block_start + 1);
      double energy_j_req =
	((sled->active_power_mw * timedelta) +
	 (sled->tip_power_mw * num_tips_req * timedelta)) / 1000000.0;
      
      reqinfo->request_energy_uj += energy_j_req * 1000000.0;
    }
  
  if (sled->active_request || sled->prefetch_info)
    {
      sled->dev->stat.servicing_energy_j += energy_j;
      sled->stat.servicing_energy_j      += energy_j;
    }
  else
    {
      sled->dev->stat.idle_energy_j += energy_j;
      sled->stat.idle_energy_j      += energy_j;
    }
}

void
mems_energy_update_sled_data(mems_sled_t *sled,
			     double timedelta)
{
  /* Account for both sled positioning energy and tip dissipation. */
  // int num_tips = sled->tipset.tip_end - sled->tipset.tip_start + 1;
  int num_tips = sled->tipset.num_tips;
  double energy_j =
    ((sled->active_power_mw * timedelta) +
     (sled->tip_power_mw * num_tips * timedelta)) / 1000000.0;
  
  sled->dev->stat.total_energy_j += energy_j;
  sled->stat.total_energy_j += energy_j;
  
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      int num_tips_req = sled->tip_sectors_per_lbn *
	(reqinfo->next_block_end - reqinfo->next_block_start + 1);
      double energy_j_req =
	((sled->active_power_mw * timedelta) +
	 (sled->tip_power_mw * num_tips_req * timedelta)) / 1000000.0;
      
      reqinfo->request_energy_uj += energy_j_req * 1000000.0;
    }
  
  sled->dev->stat.servicing_energy_j += energy_j;
  sled->stat.servicing_energy_j      += energy_j;
}

/************************************************************************
 * Event handling functions
 ************************************************************************/

void
mems_io_access_arrive(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);

#ifdef VERBOSE_EVENTLOOP
  printf("\nIO_ACCESS_ARRIVE: %f -- %s for %d(%d), batchno = %d, batch_complete = %d\n",
	 simtime, (curr->flags & READ) ? "R" : "W", curr->blkno, curr->bcount, curr->batchno, curr->batch_complete);
  fflush(stdout);
#endif
 
  /* A new request!
   * - Initialize the per-request mems info structure.
   * - Map the request to the appropriate sled. */

  curr->batch_next = NULL;
  curr->batch_prev = NULL;
  curr->batch_size = 0;

  curr->mems_sled = mems_lbn_to_sled(dev, curr->blkno);
  curr->mems_reqinfo = mems_get_new_reqinfo(curr);
  
  dev->busowned = mems_get_busno(curr);
  
  curr->time = simtime + dev->overhead;
  curr->type = DEVICE_OVERHEAD_COMPLETE;
  addtointq((event *)curr);
}

void
mems_device_overhead_complete(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);

#ifdef VERBOSE_EVENTLOOP
  printf("\nDEVICE_OVERHEAD_COMPLETE: %f\n", simtime);
  fflush(stdout);
#endif

  /* - Add the request to sled's/device's incoming request queue. */
  ioqueue_add_new_request(sled->queue, curr);
  // ioqueue_add_new_request(sled->dev->queue, curr);

  // if (curr->batchno == 0)
  if (1)
    {
      /* If it's a write request, go ahead and keep the bus to start
       * transferring blocks into the buffer */
      if (!(curr->flags & READ))
	{
	  ioreq_event *busreq = ioreq_copy(curr);
#ifdef VERBOSE_EVENTLOOP
	  printf("\n adding a MEMS_BUS_INITIATE event to the intq: %f\n", simtime);
	  fflush(stdout);
#endif
	  busreq->type = MEMS_BUS_INITIATE;
	  busreq->time = simtime;
	  addtointq((event *)busreq);
	  ((mems_reqinfo_t *)(curr->mems_reqinfo))->bus_pending = TRUE;
	}
      else
	{
	  /* Special case: If all the blocks for this request are already
	   * buffered, no need to add request to sled active queue (or even
	   * notify sled at all) -- instead, just put the busxfer event on
	   * the queue. */

	  //  I need to decide whether I can respond to a single request
	  //  of a batch if it is buffered, or if I have to check the whole
	  //  thing.

	  //  A case which would cause problems would be if only the last
	  //  request of the batch was cached, but other requests in the
	  //  batch were not.  In this case, the batch would never come out
	  //  of the ioqueue, since it would never be completed.  The hope
	  //  is that if one request of the batch is cached, then the rest
	  //  are as well.  I may need to make that explicit.

	  mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)curr->mems_reqinfo;
	  mems_extent_t *extent_ptr = reqinfo->extents;

	  if (mems_buffer_check(extent_ptr->firstblock,
				extent_ptr->lastblock, sled->dev)) {
	    // ioqueue_get_specific_request(sled->dev->queue, curr);
	    ioqueue_get_specific_request(sled->queue, curr);
	    extent_ptr->completed_block_media = extent_ptr->lastblock;
	    extent_ptr->media_done = TRUE;
	    {
	      ioreq_event *busreq = ioreq_copy(curr);
	      busreq->type = MEMS_BUS_INITIATE;
	      busreq->time = simtime;
	      addtointq((event *)busreq);
	      reqinfo->bus_pending = TRUE;
	    }
	    addtoextraq((event *)curr);  // I think curr gets freed later...
	    return;
	  }

	  /* Release the bus if there's no dataxfer_req holding it */
	  if (!sled->dev->dataxfer_req)
	    {
	      send_disconnect(curr, 0.0);
	    }
	}
    }
  /*
  else if ((curr->batchno != 0) && (curr->batch_complete == TRUE))
    {
      if (!(curr->flags & READ))
	{
	  ioreq_event *busreq = ioreq_copy(curr);
	  busreq->type = MEMS_BUS_INITIATE;
	  busreq->time = simtime;
	  addtointq((event *)busreq);
	  ((mems_reqinfo_t *)(curr->mems_reqinfo))->bus_pending = TRUE;    
	}
    }
  */
  else
    {
      if (!sled->dev->dataxfer_req)
	{
	  send_disconnect(curr, 0.0);
	}
    }

  // don't move to the scheduling state unless a batch is complete.
  // in theory, this should work even when issuing interleaved
  // requests from multiple batches.  when a batch (any batch) is
  // complete, then MEMS_SLED_SCHEDULE will pull it out of the ioqueue
  // and start crunching on it.
  if ((curr->batchno == 0) || (curr->batch_complete == TRUE))
    {

      /* If the sled is in the inactive power-saving state, reactivate it */
      if (sled->active == MEMS_SLED_INACTIVE)
	{
	  ioreq_event *r = (ioreq_event *)getfromextraq();
	  // fflush(stdout);
	  // printf("GETFROMEXTRAQ - first\n");
	  r->devno = curr->devno;
	  /* FIXME: Crashes without above line. Why? Use ioreq_copy? */
	  r->type = MEMS_SLED_SCHEDULE;
	  r->time = simtime + sled->startup_time_ms;
	  /* FIXME: curr->time doesn't work (for simtime).  Why? */
	  r->mems_sled = sled;
	  addtointq((event *)r);
	
	  sled->active = MEMS_SLED_ACTIVE;
	
	  /* Spindown/spinup statistics */
	  sled->stat.num_spinups++;
	  sled->dev->stat.num_spinups++;
	  stat_update(&sled->stat.inactive_time, 
		      (simtime - sled->lastreq_comptime));
	  stat_update(&sled->dev->stat.inactive_time,
		      (simtime - sled->lastreq_comptime));
	
	  /* Energy statistics -- only if resuming from inactive */
	  mems_energy_update_device_overhead_complete_inactive(sled);
	}
      else if (sled->active == MEMS_SLED_IDLE)
	{
	  /*  the sled is idle, but not spun down  */
	  coord_t *update_coord;
	  double elapsed_time;
	  double sweep_time;
	  double leftover_time;
	  double extra_sweeps;
	  int extra_distance;
	  int sled_length_bits;
	
	  ioreq_event *r = (ioreq_event *)getfromextraq();
	  // printf("GETFROMEXTRAQ - second\n");
	  r->devno = curr->devno;
	  /* FIXME: Crashes without above line. Why? Use ioreq_copy? */
	  r->type = MEMS_SLED_SCHEDULE;
	  r->time = simtime;
	  /* FIXME: curr->time doesn't work (for simtime).  Why? */
	
	  removefromintq((event *)sled->active_request);
	  sled->active_request = NULL;
	
	  /*  figure out where the sled should be at this point  */
	  update_coord = mems_get_current_position(sled);
	
	  elapsed_time = simtime - sled->lastreq_comptime;
	  sweep_time =
	    find_turnaround_time((sled->y_length_nm / 2),
				 sled->y_access_speed_bit_s * sled->bit_length_nm,
				 sled->x_accel_nm_s2,
				 sled->spring_factor,
				 sled->y_length_nm);
	  sweep_time +=
	    sled->y_length_nm / sled->y_access_speed_bit_s * sled->bit_length_nm;
	
	  leftover_time = fmod(elapsed_time, sweep_time);
	  extra_sweeps = (double)(leftover_time / sweep_time);
	
	  sled_length_bits = sled->y_length_nm / sled->bit_length_nm;
	
	  extra_distance =
	    (int)(extra_sweeps * (double)(sled->y_length_nm)) / sled->bit_length_nm;
	
	  if (update_coord->y_vel > 0)
	    {
	      /*  if we're moving up  */
	      update_coord->y_pos += extra_distance;
	    
	      if (abs(update_coord->y_pos) > (sled_length_bits / 2))
		{
		  /*  if we've gone too far...  */
		  update_coord->y_pos =
		    ((sled_length_bits / 2)
		     -
		     (update_coord->y_pos - (sled_length_bits / 2)));
		  update_coord->y_vel = -update_coord->y_vel;
		}
	    
	    }
	  else
	    {
	      /*  we're moving down  */
	      update_coord->y_pos -= extra_distance;
	    
	      if (abs(update_coord->y_pos) > (sled_length_bits / 2))
		{
		  /*  if we've gone too far...  */
		  update_coord->y_pos =
		    ((-sled_length_bits / 2)
		     +
		     (abs(update_coord->y_pos) - (sled_length_bits / 2)));
		  update_coord->y_vel = -update_coord->y_vel;
		}
	    }
	  mems_commit_move(sled, update_coord);
	
	  r->mems_sled = sled;
	  addtointq((event *)r);
	  sled->active = MEMS_SLED_ACTIVE;
	
	  mems_energy_update_device_overhead_complete_active(sled,
							     elapsed_time);
	}
    }
}

void
mems_sled_schedule(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);
  ioreq_event *old_active_request;
  
#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_SCHEDULE: %f\n", simtime);
  fflush(stdout);
#endif

  if ((sled->active == MEMS_SLED_IDLE)
      &&
      (curr->time >= sled->lastreq_comptime + sled->inactive_delay_ms))
    {
      /*  the sled has been idle and should now spin down  */
      // printf("Sled was idle, now it's spun down.\n");
      sled->lastreq_comptime = curr->time;
      // sled->lastreq_lbn = mems_centermost_lbn(sled);
      
      /* Initialize sled position, velocity at centermost LBN */
      mems_lbn_to_position(mems_centermost_lbn(sled),
			   sled,
			   &sled->coordset_up, 
			   &sled->coordset_dn,
			   &sled->tipset, 
			   NULL, NULL, NULL);
      mems_coord_t_copy(&sled->coordset_up.servo_start, &sled->pos);
      sled->active = MEMS_SLED_INACTIVE;
      sled->active_request = NULL;
      addtoextraq((event *)curr);
      
      /* Spindown/spinup statistics */
      sled->stat.num_spindowns++;
      sled->dev->stat.num_spindowns++;
      
      // printf("Hello!!  curr->type = %d, curr->time = %f\n", curr->type, curr->time);
      return;
    }
  else if ((sled->active == MEMS_SLED_IDLE)
	   &&
	   (curr->time < sled->lastreq_comptime + sled->inactive_delay_ms))
    {
      /*  the sled is idle, but now there's a new request  */
      // printf("Sled was idle, but there's a new request coming in.\n");
      sled->active = MEMS_SLED_ACTIVE;
      
      removefromintq((event *)sled->active_request);
      sled->active_request = NULL;
    }
  
  /* First check if there's no active request, and try to find one. */
  if (!sled->active_request)
    {
      sled->active_request = ioqueue_get_next_request(sled->queue);
      if (sled->active_request)
	{
	  // ioqueue_get_specific_request(sled->dev->queue, sled->active_request); // i think that this is only done for stats.  -schlos
	  
	  // a single ioreq_event can have multiple ioreq_events chained off of it
	  // in the extents.  ioqueue_get_next_request returns a pointer to a chain
	  // of ioreq_events, if the request is part of a batch.  those requests get
	  // put into the mems_reqinfo extents, each with a pointer to the ioreq_event
	  // which originally generated that extent.  however, ioqueue_get_next_request
	  // returns the ioreq_event of the first request in the batch.  so if that
	  // request finishes before the others in the batch, it will hose the entire
	  // batch and all of its extents.  i need to make a copy of the ioreq_event
	  // and use that to execute this batch.  and make sure i free it when all of
	  // the extents are complete.
	  //
	  // i think...
	  
	  if (sled->active_request->batch_next != NULL)
	    {
	      // printf("more than one request was returned!\n");
	      mems_update_reqinfo(sled->active_request);
#ifdef VERBOSE_EVENTLOOP	  
	      print_extent_info(sled->active_request->mems_reqinfo);
#endif
	    }
	  
	  old_active_request = sled->active_request;
	  sled->active_request = ioreq_copy(sled->active_request);
	  old_active_request->mems_reqinfo = NULL;
	  
	  /* Just got a new active request, so throw out old prefetch info */
	  /*
	    if (sled->prefetch_info)
	    {
	    if (sled->prefetch_info->completed_block != -1)
	    {
	    int fetched_blocks = sled->prefetch_info->completed_block -
	    sled->prefetch_info->firstblock + 1;
	    stat_update(&sled->stat.prefetched_blocks, fetched_blocks);
		stat_update(&sled->dev->stat.prefetched_blocks, fetched_blocks); 
	      }
	    // addtoextraq((event *)sled->prefetch_info);
	    sled->prefetch_info = NULL;
	    }
	  */
	  /* If new request is a read, get new prefetch info */
	  if (sled->active_request->flags & READ)
	    {
	      // sled->prefetch_info = mems_get_prefetch_info(sled->active_request);
	    }
	}
    }
  
  /* Now, if there's an active request, figure out which position to seek
   * to, and which blocks to access once we're there. */
  if (sled->active_request)
    {
      int extent;
      int block;
      int lastblock;
      mems_reqinfo_t *reqinfo;
      int started_mapping = FALSE;
      tipsector_coord_set_t tmp_coordset_up, tmp_coordset_dn;
      struct tipset tmp_tipset;
      tipsector_coord_set_t first_coordset_up, first_coordset_dn;
      struct tipset first_tipset;
      mems_extent_t *extent_ptr;
      
      reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      
#ifdef VERBOSE_EVENTLOOP
      printf("MEMS_SLED_SCHEDULE:: before\n");
      print_extent_info(reqinfo);
#endif
      
      // go through the extents and assign the next blocks
      extent_ptr = reqinfo->extents;
      
      for (extent = 0; extent < reqinfo->num_extents; extent++)
	{
	  if (extent_ptr->media_done == TRUE)
	    {
	      extent_ptr = extent_ptr->next;
	      continue;
	    }

	  if (extent_ptr->completed_block_media == -1)
	    {
	      extent_ptr->next_block_start = extent_ptr->firstblock;
	    }
	  else
	    {
	      extent_ptr->next_block_start = extent_ptr->completed_block_media + 1;
	    }
	  
	  // set the first lbn's position - this is what we compare to the
	  // rest of the way.
	  if (started_mapping == FALSE)
	    {
	      mems_lbn_to_position(extent_ptr->next_block_start,
				   sled,
				   &sled->coordset_up,
				   &sled->coordset_dn,
				   &sled->tipset,
				   NULL, NULL, NULL);
	      // this is a hack since the first block gets mapped twice
	      sled->tipset.num_tips = 0;  
	      started_mapping = TRUE;
	    }

	  // find the position for this extent's next block
	  mems_lbn_to_position(extent_ptr->next_block_start,
			       sled,
			       &extent_ptr->coordset_up,
			       &extent_ptr->coordset_dn,
			       &extent_ptr->tipset,
			       NULL, NULL, NULL);
	  
	  if (mems_equal_coord_sets(&sled->coordset_up,
				    &extent_ptr->coordset_up) == FALSE)
	    {
	      extent_ptr->next_block_start = -1;
	      extent_ptr->next_block_end = -1;
	      extent_ptr = extent_ptr->next;
	      continue;
	    }
	  
	  if (sled->tipset.num_tips + extent_ptr->tipset.num_tips
	      >
	      sled->tips_simultaneous)
	    {
	      extent_ptr->next_block_start = -1;
	      extent_ptr->next_block_end = -1;
	      extent_ptr = extent_ptr->next;
	      continue;
	    }
	  
	  sled->tipset.num_tips += extent_ptr->tipset.num_tips;
	  extent_ptr->next_block_end = extent_ptr->next_block_start;
	  
	  // go through this extent's blocks and find the last that can be
	  // accessed in parallel
	  for (block = extent_ptr->next_block_end + 1;
	       block <= extent_ptr->lastblock;
	       block++)
	    {
	      // if this is a write and we haven't transferred this block over the bus,
	      // it can't be written yet.
	      if (!(sled->active_request->flags & READ) &&
		  (block > extent_ptr->completed_block_bus))
		{
		  break;
		}
	      
	      mems_lbn_to_position(block,
				   sled,
				   &tmp_coordset_up,
				   &tmp_coordset_dn,
				   &tmp_tipset,
				   NULL, NULL, NULL);
	      
	      // if this block's coordset is not equal to first_coordset,
	      // then they are in different positions and can't be accessed
	      // in parallel and we're done
	      if (mems_equal_coord_sets(&sled->coordset_up,
					&tmp_coordset_up) == FALSE)
		{
		  break;
		}
	      
	      // if adding this block exceeds the number of available tips,
	      // then break.
	      if (sled->tipset.num_tips + tmp_tipset.num_tips
		  >
		  sled->tips_simultaneous)
		{
		  break;
		}
	
	      sled->tipset.num_tips += tmp_tipset.num_tips;
	      extent_ptr->next_block_end = block;
	    }
	  extent_ptr = extent_ptr->next;
	}
      /* If we've started prefetching, update prefetch info */
      /*
	if (reqinfo->next_block_end > reqinfo->lastblock) {
	sled->prefetch_info->next_block_end = reqinfo->next_block_end;
	sled->prefetch_info->next_block_start = reqinfo->lastblock + 1;
	reqinfo->next_block_end = reqinfo->lastblock;
	}
      */
    }
  else
    {
      /* No active request, no prefetch information, nothing to do, so
	 set the sled to be idle and schedule an event for when it
	 should actually spin down. */
      if (curr->time < sled->lastreq_comptime + sled->inactive_delay_ms)
	{
	  // printf("I think it's idle, but not spun down\n");
	  sled->lastreq_comptime = curr->time;
	  sled->active = MEMS_SLED_IDLE;
	  sled->active_request = curr;   /* keep a pointer to this event in case */
	  
	  curr->type = MEMS_SLED_SCHEDULE;
	  curr->time = simtime + sled->inactive_delay_ms;
	  addtointq((event *)curr);
	  return;
	}
      else if (curr->time
	       >=
	       sled->lastreq_comptime + sled->inactive_delay_ms)
	{
	  /* If enough time has passed, switch sled to inactive state */
	  // printf("Now I've spun down.\n");
	  sled->lastreq_comptime = curr->time;
	  // sled->lastreq_lbn = mems_centermost_lbn(sled);
	  
	  /* Initialize sled position, velocity at centermost LBN */
	  mems_lbn_to_position(mems_centermost_lbn(sled), sled, &sled->coordset_up, 
			       &sled->coordset_dn, &sled->tipset, 
			       NULL, NULL, NULL);
	  mems_coord_t_copy(&sled->coordset_up.servo_start, &sled->pos);
	  sled->active = MEMS_SLED_INACTIVE;
	  addtoextraq((event *)curr);
	  
	  /* Spindown/spinup statistics */
	  sled->stat.num_spindowns++;
	  sled->dev->stat.num_spindowns++;
	  
	  return;
	}
    }
  
  // printf("I DID THIS!!\n");
#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_SCHEDULE:: after\n");
  print_extent_info(sled->active_request->mems_reqinfo);
#endif

  curr->type = MEMS_SLED_SEEK;
  addtointq((event *)curr);
}

void
mems_update_seek_stats(double seek_time,
		       double x_seek_time,
		       double y_seek_time,
		       double turnaround_time,
		       int num_turnarounds,
		       mems_sled_t *sled)
{
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      
      reqinfo->subtrack_access_num++;
      // printf("subtrack_access_num = %d\n", reqinfo->subtrack_access_num);
      reqinfo->subtrack_access_tips += sled->tip_sectors_per_lbn *
	(reqinfo->next_block_end - reqinfo->next_block_start + 1);
      
      if (reqinfo->firstseek)
	{
	  stat_update(&sled->dev->stat.seek_time, seek_time);
	  stat_update(&sled->stat.seek_time, seek_time);
	  stat_update(&sled->dev->stat.x_seek_time, x_seek_time);
	  stat_update(&sled->stat.x_seek_time, x_seek_time);
	  stat_update(&sled->dev->stat.y_seek_time, y_seek_time);
	  stat_update(&sled->stat.y_seek_time, y_seek_time);
	  
	  stat_update(&sled->dev->stat.turnaround_time, turnaround_time);
	  stat_update(&sled->stat.turnaround_time, turnaround_time);
	  stat_update(&sled->dev->stat.turnaround_number, num_turnarounds);
	  stat_update(&sled->stat.turnaround_number, num_turnarounds);
	  
	  sled->dev->stat.num_initial_turnarounds += num_turnarounds;
	  sled->stat.num_initial_turnarounds += num_turnarounds;
	  
	  /*
	    fprintf(stderr, "MEMS_SLED_SEEK - added %d initial turnarounds\n",
	    num_turnarounds);
	    fprintf(stderr, "MEMS_SLED_SEEK - num_initial_turnarounds = %d\n",
	    sled->dev->stat.num_initial_turnarounds);
	  */
	  
	  reqinfo->firstseek = 0;
	}
      else if (num_turnarounds != 0)
	{  /*  we must be streaming  */
	  stat_update(&sled->dev->stat.stream_turnaround_time, turnaround_time);
	  stat_update(&sled->stat.stream_turnaround_time, turnaround_time);
	  stat_update(&sled->dev->stat.stream_turnaround_number, num_turnarounds);
	  stat_update(&sled->stat.stream_turnaround_number, num_turnarounds);
	  
	  sled->dev->stat.num_stream_turnarounds += num_turnarounds;
	  sled->stat.num_stream_turnarounds += num_turnarounds;
	  /*
	    fprintf(stderr, "MEMS_SLED_SEEK - added %d streaming turnarounds\n",
	    num_turnarounds);
	    fprintf(stderr, "MEMS_SLED_SEEK - num_streaming_turnarounds = %d\n",
	    sled->dev->stat.num_streaming_turnarounds);
	  */
	}
    }
}

void
mems_sled_seek(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  enum direction_enum { UP, DOWN } direction;
  double time_up, x_up, y_up, to_up;
  double time_dn = 0.0;
  double x_dn, y_dn, to_dn;
  int num_up, num_dn;
  double timedelta;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_SEEK: %f\n", simtime);
  fflush(stdout);
#endif

  time_up = mems_seek_time_seekcache(sled, &sled->pos, 
				     &sled->coordset_up.servo_start, 
				     &x_up, &y_up, &to_up, &num_up);
  /*
    time_up = mems_seek_time(sled, &sled->pos, 
    &sled->coordset_up.servo_start, 
    &x_up, &y_up, &to_up, &num_up);
  */
  /* This call can be expensive, so don't do it if unnecessary */
  if (sled->bidirectional_access)
    {
      time_dn = mems_seek_time_seekcache(sled, &sled->pos, 
					 &sled->coordset_dn.servo_start, 
					 &x_dn, &y_dn, &to_dn, &num_dn);
    }
  
  if (!sled->bidirectional_access)
    {
      direction = UP;
    }
  else if (time_up < time_dn)
    {
      direction = UP;
    }
  else
    {
      direction = DOWN;
    }
  
  /* FIXME: If the request spans multiple adjacent sectors,
   * we want zero seek time between the adjacent sectors.  To ensure
   * this, we want the _initial_ seek to go to the position farthest
   * from the _second_ sector, so when it finishes accessing the
   * first sector it can just keep on trucking in the same
   * direction.  (Am I making sense here?) */
  
  if (direction == UP)
    {
      timedelta = time_up;
      mems_tipsector_coord_set_t_copy(&sled->coordset_up, &sled->coordset);
      mems_update_seek_stats(time_up, x_up, y_up, to_up, num_up, sled);    
    }
  else
    {
      timedelta = time_dn;
      mems_tipsector_coord_set_t_copy(&sled->coordset_dn, &sled->coordset);
      mems_update_seek_stats(time_dn, x_dn, y_dn, to_dn, num_dn, sled);    
    }

  /*
    fprintf(stderr, "MEMS_SLED_SEEK - timedelta = %f\n", timedelta);
  */
  curr->type = MEMS_SLED_SERVO;
  curr->time += timedelta;
  addtointq((event *)curr);
  mems_commit_move(sled, &sled->coordset.servo_start);
  
  /* Energy statistics */
  mems_energy_update_sled_seek(sled, timedelta);
}

void
mems_sled_servo(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  double timedelta;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_SERVO: %f\n", simtime);
  fflush(stdout);
  /*
    fprintf(stderr, "x_pos = %d, y_pos = %d\n",
    sled->pos.x_pos, sled->pos.y_pos);
  
    fprintf(stderr, "tip_start = %d, tip_end = %d\n",
    sled->tipset.tip_start, sled->tipset.tip_end);
  */
#endif

  /* Calculate servo access time */
  timedelta = mems_media_access_time(sled, &sled->pos, 
				     &sled->coordset.tipsector_start);
  
  /* Only access data if there is an active request or prefetching */
  if (sled->active_request || sled->prefetch_info)
    {
      curr->type = MEMS_SLED_DATA;
    }
  else
    {
      curr->type = MEMS_SLED_SCHEDULE;
    }

  /* Weird special case:  If this is a write request, and we
   * haven't busxfered ANY of the blocks we're trying to write, then
   * we should return to MEMS_SLED_SCHEDULE immediately.  It
   * was easier to put in this small hack than do a lot of
   * hacking above. 
   *
   * Note I don't expect this to ever actually happen, but
   * we're building a quality simulator here. */
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      mems_extent_t *extent_ptr = reqinfo->extents;
      int i;
      int data_to_write = TRUE;
      
      for (i = 0; i < reqinfo->num_extents; i++)
	{
	  if (!(extent_ptr->request->flags & READ) &&
	      (extent_ptr->next_block_start > extent_ptr->completed_block_bus))
	    {
	      data_to_write = FALSE;
	    }
	  else
	    {
	      data_to_write = TRUE;
	    }
	  extent_ptr = extent_ptr->next;
	}
      if (data_to_write == FALSE)
	{
	  curr->type = MEMS_SLED_SCHEDULE;
	  /* Turn only one tip on during the access (minimize energy) */
	  // sled->tipset.tip_end = sled->tipset.tip_start;
	  sled->tipset.num_tips = 1;
	  
	  // fprintf(stderr,"Woah! The weird special case is here!\n");
	}
    }
  
  curr->time += timedelta;
  addtointq((event *)curr);
  mems_commit_move(sled, &sled->coordset.tipsector_start);
  
  /* Energy statistics */
  mems_energy_update_sled_servo(sled, timedelta);
}

void
mems_sled_data(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  double timedelta;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_DATA: %f\n", simtime);
  /*
  fprintf(stderr, "x_pos = %d, y_pos = %d\n",
	  sled->pos.x_pos, sled->pos.y_pos);
  
  fprintf(stderr, "tip_start = %d, tip_end = %d\n",
	  sled->tipset.tip_start, sled->tipset.tip_end);
  */
#endif
  
  /* Calculate data access time */
  timedelta = mems_media_access_time(sled, &sled->pos, 
				     &sled->coordset.tipsector_end);
  
  curr->type = MEMS_SLED_UPDATE;
  curr->time += timedelta;
  addtointq((event *)curr);
  mems_commit_move(sled, &sled->coordset.tipsector_end);
  
  /* Energy statistics */
  mems_energy_update_sled_data(sled, timedelta);
}

void
mems_sled_update(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_SLED_UPDATE: %f\n", simtime);
  fflush(stdout);
#endif
  
  if (sled->active_request)
    {
      mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)sled->active_request->mems_reqinfo;
      int extent;
      mems_extent_t *extent_ptr;
      mems_extent_t *next_extent;
      int all_media_done = TRUE;
      int num_initial_extents = reqinfo->num_extents;

#ifdef VERBOSE_EVENTLOOP
      printf("MEMS_SLED_UPDATE:: before\n");
      print_extent_info(reqinfo);
#endif
      
      extent_ptr = reqinfo->extents;
      
      for (extent = 0; extent < num_initial_extents; extent++)
	{
	  double latency = 0.0;
	  next_extent = extent_ptr->next;
	  extent_ptr->completed_block_media = extent_ptr->next_block_end;
	  /* BUFFER - insert the blocks into the buffer */
	  if (extent_ptr->request->flags & READ) {
	    mems_buffer_insert(extent_ptr->next_block_start, 
			       extent_ptr->next_block_end, sled->dev);      
	  }
	  
	  // if this extent is a read and the bus is not pending,
	  // then initiate a bus access - but only one!
	  if ((extent_ptr->request->flags & READ) &&
	      (reqinfo->bus_pending == FALSE))
	    {
	      ioreq_event *busreq = ioreq_copy(sled->active_request);
	      busreq->type = MEMS_BUS_INITIATE;
	      busreq->time = simtime;
	      addtointq((event *)busreq);
	      reqinfo->bus_pending = TRUE;
	      assert(extent_ptr->bus_done == FALSE);
	    }
	  
	  if (extent_ptr->completed_block_media >= extent_ptr->lastblock)
	    {
	      extent_ptr->media_done = TRUE;
	      
	      // if the bus transfer is done as well, then pull this extent out and
	      // respond to its request
	      if (extent_ptr->bus_done == TRUE)
		{
		  if (extent_ptr->prev)
		    {
		      extent_ptr->prev->next = extent_ptr->next;
		    }
		  else
		    {
		      reqinfo->extents = extent_ptr->next;
		    }
		  if (extent_ptr->next)
		    {
		      extent_ptr->next->prev = extent_ptr->prev;
		    }
		  reqinfo->num_extents--;
		  if (reqinfo->num_extents == 0)
		    {
		      // printf("completing batch! simtime = %f, arrival time = %f, diff = %f\n",
		      // simtime, reqinfo->batch_arrival_time, (simtime - reqinfo->batch_arrival_time));
		      stat_update(&sled->dev->stat.batch_response_time,
				  (simtime - reqinfo->batch_arrival_time));
		      stat_update(&sled->stat.batch_response_time,
				  (simtime - reqinfo->batch_arrival_time));
		    }
		  // SCHLOS - this is an ugly hack to make the write requests not complete
		  //  simultaneously.  If they do, then the Atropos threads break!
		  // latency += 0.000;
		  latency += 0.001;
		  mems_request_complete(extent_ptr->request, latency);
		  addtoextraq((event *)extent_ptr->request);
		  free(extent_ptr);
		  extent_ptr = NULL;
		  mems_extent_frees++;
		  // addtoextraq(extent_ptr);
		}
	    }
	  else
	    {
	      all_media_done = FALSE;
	    }
	  
#ifdef VERBOSE_EVENTLOOP
	  printf("media completed = %d, last = %d\n",
		 extent_ptr->completed_block_media, extent_ptr->lastblock);
	  fflush(stdout);
#endif
	  
	  extent_ptr = next_extent;
	}

#ifdef VERBOSE_EVENTLOOP
      printf("MEMS_SLED_UPDATE:: after\n");
      print_extent_info(reqinfo);
#endif

      // if all of the extents are done with media transfers, then we can free 
      // the active request and the bus unit will clean everything else up
      if (all_media_done == TRUE)
	{
	  addtoextraq((event *)sled->active_request);
	  sled->lastreq_comptime = curr->time;
	  sled->active_request   = NULL;
	}
    }
  // ELSE??
  
  curr->type = MEMS_SLED_SCHEDULE;
  addtointq((event *)curr);
}

void
mems_io_interrupt_complete(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);

#ifdef VERBOSE_EVENTLOOP
  if (curr->cause==DISCONNECT) printf("IO_INTERRUPT_DISCONNECT: %f\n", simtime);
  if (curr->cause==COMPLETION) printf("IO_INTERRUPT_COMPLETION: %f\n", simtime);
  if (curr->cause==RECONNECT)  printf("IO_INTERRUPT_RECONNECT: %f\n", simtime);
  fflush(stdout);
#endif
        
  switch (curr->cause)
    {
      
    case DISCONNECT:
      addtoextraq((event *)curr);
      if (dev->busowned != -1)
	{
	  bus_ownership_release(dev->busowned);
	  dev->busowned = -1;
	}
      break;
      
    case COMPLETION:
      // SCHLOS - this is where it messes up the mems_reqinfo!!
      //  mems_reqinfo is somehow getting set as the ioreq_event
      if (curr->mems_reqinfo)
	{
	  free(curr->mems_reqinfo);
	  curr->mems_reqinfo = NULL;
	  mems_reqinfo_frees++;
	  //addtoextraq((event *)curr->mems_reqinfo);
	}
      addtoextraq((event *)curr);
      if (dev->busowned != -1)
	{
	  bus_ownership_release(dev->busowned);
	  dev->busowned = -1;
	}
      break;
    
    case RECONNECT:
      addtoextraq((event *)curr);
      dev->dataxfer_req->type = MEMS_BUS_TRANSFER;
      dev->dataxfer_req->time = simtime;
      addtointq((event *)dev->dataxfer_req);
      break;
      
    default:
      fprintf(stderr,
	      "Unknown cause in mems_io_interrupt_complete (%d)\n",
	      curr->cause);
      exit(1);
    }
}

void
mems_bus_initiate(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);
  mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)curr->mems_reqinfo;
  mems_extent_t *extent_ptr;
  int num_extents;
  int i;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_BUS_INITIATE: %f\n", simtime);
  fflush(stdout);
#endif

  // choose an extent to start bxfering
  if (reqinfo->bus_extent == NULL)
    {
      extent_ptr = reqinfo->extents;
      num_extents = reqinfo->num_extents;
      
      for (i = 0; i < num_extents; i++)
	{
	  if (extent_ptr->completed_block_bus == -1)
	    {
	      reqinfo->bus_extent = extent_ptr;
	      // printf("bus_initiate chose this extent - %d\n",
	      //        extent_ptr->firstblock);
	      break;
	    }
	  extent_ptr = extent_ptr->next;
	}
    }
  /* else {
     printf("bus initiate stuck with this extent - %d\n",
     reqinfo->bus_extent->firstblock);
     }
  */

  if (!dev->dataxfer_req)
    {
      /* This request has the bus: grab it and start transferring */
      dev->dataxfer_req = curr;
      send_reconnect(curr, 0.0);
    }
  else
    {
      /* Another request is waiting to transfer, so add to tail of
       * dataxfer queue */
      ioreq_event *tmp = dev->dataxfer_queue;
      if (!dev->dataxfer_queue)
	{ /* If list is empty... */
	  dev->dataxfer_queue = curr;
	  curr->prev = NULL;
	  curr->next = NULL;
	}
      else
	{
	  while (tmp->next) tmp = tmp->next;	/* Find last node in list */
	  tmp->next  = curr;	/* Make curr the new last node in list */
	  curr->prev = tmp;
	  curr->next = NULL;
	}
    }
}

void
mems_bus_transfer(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_BUS_TRANSFER: %f\n", simtime);
  fflush(stdout);
#endif
  curr->type = MEMS_BUS_UPDATE;
  // device_get_blktranstime deals with transferring blocks from
  // multiple extents
  curr->time = simtime + device_get_blktranstime(curr);
  addtointq((event *)curr);
}

void
mems_bus_update(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  mems_t *dev = getmems(curr->devno);
  mems_reqinfo_t *reqinfo = (mems_reqinfo_t *)curr->mems_reqinfo;
  mems_extent_t *extent_ptr;
  mems_extent_t *bus_extent;
  int extent;
  int num_extents;
  int isread;

#ifdef VERBOSE_EVENTLOOP
  printf("MEMS_BUS_UPDATE: %f\n", simtime);
  fflush(stdout);
#endif

  bus_extent = reqinfo->bus_extent;
  isread = (bus_extent->request->flags & READ);

  /* Update last block transferred */
  if (bus_extent->completed_block_bus == -1)
    {
      bus_extent->completed_block_bus = bus_extent->firstblock;
    }
  else
    {
      bus_extent->completed_block_bus++;
    }

  /* BUFFER - insert blocks into buffer */
  if (!isread) {
    mems_buffer_insert(bus_extent->completed_block_bus,
                       bus_extent->completed_block_bus, dev);
  }

#ifdef VERBOSE_EVENTLOOP
  printf("bus completed = %d, last = %d\n",
	 bus_extent->completed_block_bus,
	 bus_extent->lastblock);
  fflush(stdout);
#endif
  
  /* Check if bus portion of request is complete */
  if (bus_extent->completed_block_bus >= bus_extent->lastblock)
    {
      bus_extent->bus_done = TRUE;
      reqinfo->bus_extent = NULL;
      
#ifdef VERBOSE_EVENTLOOP
      printf("set bus_extent->bus_done to be TRUE\n");
      fflush(stdout);
#endif
      
      /* If media access is also complete, do final request processing
       * Else free(req) -- sled will do final request processing */
      if (bus_extent->media_done == TRUE)
	{
	  if (bus_extent->prev)
	    {
	      bus_extent->prev->next = bus_extent->next;
	    }
	  else
	    {
	      reqinfo->extents = bus_extent->next;
	    }
	  if (bus_extent->next)
	    {
	      bus_extent->next->prev = bus_extent->prev;
	    }
	  reqinfo->num_extents--;
	  if (reqinfo->num_extents == 0)
	    {
	      // printf("completing batch! simtime = %f, arrival time = %f, diff = %f\n",
	      // simtime, reqinfo->batch_arrival_time, (simtime - reqinfo->batch_arrival_time));
	      stat_update(&sled->dev->stat.batch_response_time,
			  (simtime - reqinfo->batch_arrival_time));
	      stat_update(&sled->stat.batch_response_time,
			  (simtime - reqinfo->batch_arrival_time));
	    }
	  mems_request_complete(bus_extent->request, 0.0);
	  addtoextraq((event *)bus_extent->request);  // schlos
	  free(bus_extent);
	  bus_extent = NULL;
	  mems_extent_frees++;
	  // addtoextraq(bus_extent);
	}

      // check if there are other extents that need bus transfers
#ifdef VERBOSE_EVENTLOOP
	  printf("reqinfo->num_extents = %d\n",
		 reqinfo->num_extents);
	  fflush(stdout);
#endif
      extent_ptr = reqinfo->extents;

      while (extent_ptr != NULL)
	{
#ifdef VERBOSE_EVENTLOOP
	  printf("firstblock = %d, completed_block_bus = %d, request_type = %s, completed_block_media = %d\n",
		 extent_ptr->firstblock,
		 extent_ptr->completed_block_bus,
		 (((extent_ptr->request->flags & 0x1) == DISKSIM_READ) ? ("R") : ("W")),
		 extent_ptr->completed_block_media);
	  fflush(stdout);
#endif	  
	  if (extent_ptr->completed_block_bus == -1)
	    {
	      if (((extent_ptr->request->flags & 0x1) == DISKSIM_READ)
		  &&
		  (extent_ptr->completed_block_media != -1))
		{
		  break;
		}
	      else if ((extent_ptr->request->flags & 0x1) == DISKSIM_WRITE)
		{
		  break;
		} 
	    }
	  extent_ptr = extent_ptr->next;
	}

#ifdef VERBOSE_EVENTLOOP
      if (extent_ptr)
	{
	  printf("selected extent for block %d as the next bus transfer\n",
		 extent_ptr->firstblock);
	  fflush(stdout);
	}
      else
	{
	  printf("extent_ptr is NULL!\n");
	  fflush(stdout);
	}
#endif
      
      if (!extent_ptr)
	{
	  dev->dataxfer_req = NULL;
	  reqinfo->bus_pending = FALSE;
	  if (isread && (reqinfo->extents == NULL))
	    {
	      free(reqinfo);
	      reqinfo = NULL;
	      mems_reqinfo_frees++;
	      // addtoextraq((event *)reqinfo);  // schlos - I fixed a leak here, i think
	    }
	}
      else
	{
	  reqinfo->bus_extent = extent_ptr;
	}
    
      // chucks curr because the request is totally finished
      /* addtoextraq((event *)curr); // moved to below */
    }
  else if ((bus_extent->request->flags & READ) &&
	   (bus_extent->completed_block_bus >= 
	    bus_extent->completed_block_media))
    {
      /* If this is a read request but the media hasn't finished
       * enough blocks for us to transfer, give up the bus until
       * such time as we have enough blocks to transfer */
      dev->dataxfer_req = NULL;
      reqinfo->bus_pending = FALSE;
      // chucks curr because there is no bus transfer to be done
      /* addtoextraq((event *)curr); // moved to below */
    }
  
  /* If we set (dataxfer_req = NULL) above, check if other requests
   * are waiting to transfer & give them the bus */
  if (!dev->dataxfer_req)
    {
      if (dev->dataxfer_queue)
	{
	  /* Pop first node off dataxfer_queue */
	  dev->dataxfer_req = dev->dataxfer_queue;
	  dev->dataxfer_queue = dev->dataxfer_queue->next;
	  if (dev->dataxfer_queue) dev->dataxfer_queue->prev = NULL;
	  dev->dataxfer_req->next = NULL;
      
	  /* FIXME: The 0.000001 is to ensure the RECONNECT or DISCONNECT
	   * interrupts occur after the COMPLETION (if any, above) --
	   * for some reason they're not being sequenced correctly. */
	  send_reconnect(dev->dataxfer_req, 0.000001);
	}
      else
	{
	  send_disconnect(curr, 0.000001);
	}
      addtoextraq((event *)curr);  // SCHLOS - this is where it messes up the intq!  curr is already there.
      return;
    }
  curr->type = MEMS_BUS_TRANSFER;
  addtointq((event *)curr);
}

void
mems_io_qlen_maxcheck(ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
      /* Used only at initialization time to set up queue stuff */

  curr->tempint1 = -1;
  curr->tempint2 = mems_get_maxoutstanding(curr->devno);
  curr->bcount   = 0;
}


/************************************************************************
 * Event dispatch point
 ************************************************************************/

void
mems_event_arrive (ioreq_event *curr)
{
  mems_sled_t *sled = (mems_sled_t *)curr->mems_sled;
  double timedelta;

  switch (curr->type)
    {
      
    case IO_ACCESS_ARRIVE:
      mems_io_access_arrive(curr);
      break;
      
    case DEVICE_OVERHEAD_COMPLETE:
      mems_device_overhead_complete(curr);
      break;
      
    case MEMS_SLED_SCHEDULE:
      mems_sled_schedule(curr);
      break;
      
    case MEMS_SLED_SEEK:
      mems_sled_seek(curr);
      break;
      
    case MEMS_SLED_SERVO:
      mems_sled_servo(curr);
      break;
      
    case MEMS_SLED_DATA:
      mems_sled_data(curr);
      break;
      
    case MEMS_SLED_UPDATE:
      mems_sled_update(curr);
      break;
      
    case IO_INTERRUPT_COMPLETE:
      mems_io_interrupt_complete(curr);
      break;
      
    case MEMS_BUS_INITIATE:
      mems_bus_initiate(curr);
      break;
      
    case MEMS_BUS_TRANSFER:
      mems_bus_transfer(curr);
      break;
      
    case MEMS_BUS_UPDATE:
      mems_bus_update(curr);
      break;
      
    case IO_QLEN_MAXCHECK:
      mems_io_qlen_maxcheck(curr);
      break;
      
    default:
      fprintf(stderr, "Unknown type encountered in mems_event_arrive (%d)\n", curr->type);
      exit(1);
    }
}

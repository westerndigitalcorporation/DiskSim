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

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_controller.h"
#include "disksim_ctlr.h"
#include "disksim_orgface.h"
#include "disksim_ioqueue.h"
#include "disksim_cache.h"


 //***************************************************************************
 // Function: controller_smart_queuefind
 //   Return the queue of the given device
 //
 // Parameters:
 //   void *queuefindparam      pointer to a controller object
 //   int devno                 device number attached to the controller
 //
 // Returns: struct ioq *
 //   Returns a pointer to an ioq for the given device
 //***************************************************************************

static struct ioq * controller_smart_queuefind (void *queuefindparam, int devno)
{
   controller *currctlr = (controller *)queuefindparam;
   ASSERT1((devno >= 0) && (devno < device_get_numdevices()), "devno", devno);
   return(currctlr->devices[devno].queue);
}


static void controller_smart_wakeup (void *wakeupfuncparam, struct cacheevent *cacheevent)
{
   controller *currctlr = (controller *)wakeupfuncparam;
   if (cacheevent) {
      currctlr->cache->cache_wakeup_complete(currctlr->cache, cacheevent);
   }
}


static void controller_smart_issue_access (void *issuefuncparam, ioreq_event *curr)
{
   controller *currctlr = (controller *)issuefuncparam;
   struct ioq *queue = currctlr->devices[curr->devno].queue;
   int numout = ioqueue_get_reqoutstanding(queue);

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_issue_access busno %x, buspath %x, slotno %x, slotpath %x\n", simtime, curr->busno, currctlr->devices[curr->devno].buspath.value, curr->slotno, currctlr->devices[curr->devno].slotpath.value);
   fprintf (outputfile, "*** %f: controller_smart_issue_access type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   /* in case the cache changes to which device the request is sent */
   curr->busno = currctlr->devices[curr->devno].buspath.value;
   curr->slotno = currctlr->devices[curr->devno].slotpath.value;

   ioqueue_add_new_request(queue, curr);
   if (numout < currctlr->devices[curr->devno].maxoutstanding) {
      ioreq_event *sched = ioqueue_get_next_request(queue);
      controller_send_event_down_path(currctlr, sched, currctlr->ovrhd_disk_request);
   }
}


static void controller_smart_disk_data_transfer (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *tmp = (ioreq_event *) getfromextraq();

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_disk_data_transfer: devno %d, bcount %d\n", simtime, curr->devno, curr->bcount);
   fflush( outputfile );
#endif

   curr->time = max(device_get_blktranstime(curr), currctlr->blktranstime);
   tmp->time = simtime + ((double) curr->bcount * curr->time);
   tmp->type = CONTROLLER_DATA_TRANSFER_COMPLETE;
   tmp->devno = curr->devno;
   tmp->blkno = curr->blkno;
   tmp->bcount = curr->bcount;
   tmp->tempint2 = currctlr->ctlno;
   tmp->tempptr1 = curr;

   /* want to use the tempptr1 value for something else! */
   curr->tempptr1 = tmp;
   curr->next = currctlr->datatransfers;
   curr->prev = NULL;
   if (curr->next) {
      curr->next->prev = curr;
   }
   currctlr->datatransfers = curr;

   addtointq((event *) tmp);
}


static void controller_smart_disk_data_transfer_complete (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *tmp = (ioreq_event *) curr->tempptr1;

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_disk_data_transfer_complete: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   tmp->bcount -= curr->bcount;
   addtoextraq((event *) curr);
   ASSERT(tmp->bcount >= 0);
   if (tmp->bcount == 0) {
      if (tmp->next) {
         tmp->next->prev = tmp->prev;
      }
      if (tmp->prev) {
         tmp->prev->next = tmp->next;
      } else {
         currctlr->datatransfers = tmp->next;
      }
      tmp->time = simtime;
      addtointq((event *) tmp);
   } else {
      fprintf(stderr, "Haven't required less than all out transfer at controller_smart_disk_data_transfer_complete\n");
      exit(1);
   }
}


static void controller_smart_request_complete (void *donefuncparam, ioreq_event *curr)
{
   controller *currctlr = (controller *)donefuncparam;

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_request_complete Request completed at smart controller: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags);
   fflush( outputfile );
#endif

   curr->type = IO_INTERRUPT_ARRIVE;
   curr->cause = COMPLETION;
   controller_send_event_up_path(currctlr, curr, currctlr->ovrhd_complete);
}


static void controller_smart_host_data_transfer_complete (controller *currctlr, ioreq_event *curr)
{
   /* DMA to/from host complete */

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_host_data_transfer_complete: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags);
   fflush( outputfile );
#endif

   if (curr->flags & READ) {
      currctlr->cache->cache_free_block_clean(currctlr->cache, curr);
      controller_smart_request_complete(currctlr, curr);
   } else {
      /* cache will call "done" function if request doesn't block */
      currctlr->cache->cache_free_block_dirty(currctlr->cache, curr, &disksim->donefunc_ctlrsmart_write, currctlr);
   }
   if (currctlr->hostwaiters) {
      curr = currctlr->hostwaiters->next;
      if (curr->next == curr) {
	 currctlr->hostwaiters = NULL;
      } else {
	 currctlr->hostwaiters->next = curr->next;
      }
      curr->next = NULL;
                                       /* Time for DMA */
      curr->time = simtime + ((double) curr->bcount * currctlr->blktranstime);
      addtointq((event *) curr);
   } else {
      currctlr->hosttransfer = FALSE;
   }
}


static void controller_smart_host_data_transfer (void *donefuncparam, ioreq_event *curr)
{
   controller *currctlr = (controller *)donefuncparam;

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_host_data_transfer: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   /* DMA data to/from host */

   curr->type = CONTROLLER_DATA_TRANSFER_COMPLETE;
   curr->tempint2 = currctlr->ctlno;
   curr->tempptr1 = NULL;

   if (currctlr->hosttransfer) {
      if (currctlr->hostwaiters) {
         curr->next = currctlr->hostwaiters->next;
         currctlr->hostwaiters->next = curr;
      } else {
         curr->next = curr;
      }
      currctlr->hostwaiters = curr;
      return;
   }
                                       /* Time for DMA */
   curr->time = simtime + ((double) curr->bcount * currctlr->blktranstime);
   currctlr->hosttransfer = TRUE;
   addtointq((event *) curr);
}


static void controller_smart_request_arrive (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_request_arrive Request arrived at smart controller: devno %d, blkno %d, flags %x\n", simtime, curr->devno, curr->blkno, curr->flags);
   fflush( outputfile );
#endif

   /* Cache will call "done" function if cache access doesn't block */

   currctlr->cache->cache_get_block(currctlr->cache, curr, &disksim->donefunc_ctlrsmart_read, currctlr);
}


static void controller_smart_access_complete (controller *currctlr, ioreq_event *curr)
{
   struct ioq *queue = currctlr->devices[curr->devno].queue;
   ioreq_event *done = ioreq_copy(curr);
   int devno = curr->devno;
   int numout;

#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_access_complete: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   /* Responds to completion interrupt */

   done->type = IO_INTERRUPT_COMPLETE;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, done, NULL);
   controller_send_event_down_path(currctlr, done, currctlr->ovrhd_disk_complete);
   currctlr->outbusowned = -1;

   /* Handles request completion, including call-backs into cache */

   curr = ioqueue_physical_access_done(queue, curr);
   while ((done = curr)) {
      curr = curr->next;
      /* call back into cache with completion -- let it do request_complete */
      controller_smart_wakeup(currctlr, (struct cacheevent *)currctlr->cache->cache_disk_access_complete(currctlr->cache, done));
   }

   /* Initiate another request, if any pending */

   numout = ioqueue_get_reqoutstanding(queue);
   if ((numout < currctlr->devices[devno].maxoutstanding) && (curr = ioqueue_get_next_request(queue))) {
      controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_request);
   }
}


static void controller_smart_ready_to_transfer (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_ready_to_transfer: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   curr->type = IO_INTERRUPT_COMPLETE;
   curr->cause = RECONNECT;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_ready);
   currctlr->outbusowned = -1;
}


static void controller_smart_disconnect (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_disconnect: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   curr->type = IO_INTERRUPT_COMPLETE;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_disconnect);
   currctlr->outbusowned = -1;
}


static void controller_smart_reconnect_to_transfer (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_reconnect_to_transfer: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   curr->type = IO_INTERRUPT_COMPLETE;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_reconnect);
   currctlr->outbusowned = -1;
}


static void controller_smart_interrupt_arrive (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_interrupt_arrive: cause %s, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, getCauseString( curr->cause ), curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   switch (curr->cause) {

      case COMPLETION:
         controller_smart_access_complete(currctlr, curr);
         break;

      case RECONNECT:
         controller_smart_reconnect_to_transfer(currctlr, curr);
         break;

      case DISCONNECT:
         controller_smart_disconnect(currctlr, curr);
         break;

      case READY_TO_TRANSFER:
         controller_smart_ready_to_transfer(currctlr, curr);
         break;

      default:
         fprintf(stderr, "Unknown interrupt cause in smart_interrupt_arrive: %d\n", curr->cause);
         exit(1);
   }
}


static void controller_smart_interrupt_complete (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_interrupt_complete: devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   switch (curr->cause) {

       case COMPLETION:
          addtoextraq((event *) curr);
          break;

      default:
         fprintf(stderr, "Unknown interrupt cause in smart_interrupt_complete: %d\n", curr->cause);
         exit(1);
   }
}


void controller_smart_event_arrive (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRSMART
   fprintf (outputfile, "*** %f: controller_smart_event_arrive: type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   switch (curr->type) {

      case IO_ACCESS_ARRIVE:
         controller_smart_request_arrive(currctlr, curr);
         break;

      case IO_INTERRUPT_ARRIVE:
         controller_smart_interrupt_arrive(currctlr, curr);
         break;

      case IO_INTERRUPT_COMPLETE:
         controller_smart_interrupt_complete(currctlr, curr);
         break;

      case DEVICE_DATA_TRANSFER_COMPLETE:
         controller_smart_disk_data_transfer(currctlr, curr);
         break;

      case CONTROLLER_DATA_TRANSFER_COMPLETE:
         if (curr->tempptr1) {
            controller_smart_disk_data_transfer_complete(currctlr, curr);
         } else {
            controller_smart_host_data_transfer_complete(currctlr, curr);
         }
         break;

      case IO_QLEN_MAXCHECK:
         curr->tempint1 = currctlr->ctlno;
         curr->tempint2 = currctlr->maxoutstanding;
         curr->bcount = currctlr->cache->cache_get_maxreqsize(currctlr->cache);
         break;

      default:
         fprintf(stderr, "Unknown event type arriving at SMART controller: %d\n", curr->type);
         exit(1);
   }
}


void controller_smart_setcallbacks ()
{
   disksim->issuefunc_ctlrsmart = controller_smart_issue_access;
   disksim->queuefind_ctlrsmart = controller_smart_queuefind;
   disksim->wakeupfunc_ctlrsmart = controller_smart_wakeup;
   disksim->donefunc_ctlrsmart_read = controller_smart_host_data_transfer;
   disksim->donefunc_ctlrsmart_write = controller_smart_request_complete;
   ioqueue_setcallbacks ();
   cache_setcallbacks ();
}


void controller_smart_initialize (controller *currctlr)
{
   int numdevs = device_get_numdevices();
   device *currdev;
   int i;

   controller_smart_setcallbacks ();
   currctlr->numdevices = numdevs;
   currctlr->devices = (device *) DISKSIM_malloc(numdevs * sizeof(device));
   ASSERT(currctlr->devices != NULL);
   for (i=0; i<numdevs; i++) {
      currdev = &currctlr->devices[i];
      currdev->devno = i;
      currdev->busy = FALSE;
      currdev->flag = 0;
      currdev->queue = ioqueue_copy(currctlr->queue);
      ioqueue_initialize(currdev->queue, i);
      iosim_get_path_to_device (0, i, &currdev->buspath, &currdev->slotpath);
      currdev->maxoutstanding = currctlr->maxdiskqsize;
   }
   currctlr->cache->cache_initialize(currctlr->cache, &disksim->issuefunc_ctlrsmart, currctlr, &disksim->queuefind_ctlrsmart, currctlr, &disksim->wakeupfunc_ctlrsmart, currctlr, numdevs);
}


void controller_smart_resetstats (controller *currctlr)
{
   int numdevs = device_get_numdevices();
   int i;

   for (i=0; i<numdevs; i++) {
      ioqueue_resetstats(currctlr->devices[i].queue);
   }
   currctlr->cache->cache_resetstats(currctlr->cache);
}




void controller_smart_printstats (controller *currctlr, char *prefix)
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   int devno;
   char devprefix[181];
   struct ioq **queueset = (struct ioq **)DISKSIM_malloc (currctlr->numdevices * sizeof(void *));

   if (ctlrinfo->ctl_printcachestats) {
      currctlr->cache->cache_printstats(currctlr->cache, prefix);
   }

   for (devno=0; devno<currctlr->numdevices; devno++) {
      queueset[devno] = currctlr->devices[devno].queue;
   }
   sprintf (devprefix, "%sdevices ", prefix);
   ioqueue_printstats (queueset, currctlr->numdevices, devprefix);

   if ((ctlrinfo->ctl_printperdiskstats) && (currctlr->numdevices > 1)) {
      for (devno=0; devno<currctlr->numdevices; devno++) {
         struct ioq *queue = currctlr->devices[devno].queue;
         if (ioqueue_get_number_of_requests(queue)) {
            sprintf(devprefix, "%sdevice #%d ", prefix, devno);
            ioqueue_printstats(&queue, 1, devprefix);
         }
      }
   }
}

// End of file


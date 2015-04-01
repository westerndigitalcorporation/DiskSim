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


#include "disksim_simpledisk.h"


#include "modules/disksim_simpledisk_param.h"


/* read-only globals used during readparams phase */
static char *statdesc_acctimestats	=	"Access time";



/* private remapping #defines for variables from device_info_t */
#define numsimpledisks         (disksim->simplediskinfo->numsimpledisks)
//#define simpledisks            (disksim->simplediskinfo->simpledisks)



struct simpledisk *getsimpledisk (int devno)
{
   ASSERT1((devno >= 0) && (devno < MAXDEVICES), "devno", devno);
   return (disksim->simplediskinfo->simpledisks[devno]);
}


int simpledisk_set_depth (int devno, int inbusno, int depth, int slotno)
{
   simpledisk_t *currdisk;
   int cnt;

   currdisk = getsimpledisk (devno);
   assert(currdisk);
   cnt = currdisk->numinbuses;
   currdisk->numinbuses++;
   if ((cnt + 1) > MAXINBUSES) {
      fprintf(stderr, "Too many inbuses specified for simpledisk %d - %d\n", devno, (cnt+1));
      exit(1);
   }
   currdisk->inbuses[cnt] = inbusno;
   currdisk->depth[cnt] = depth;
   currdisk->slotno[cnt] = slotno;
   return(0);
}


int simpledisk_get_depth (int devno)
{
   simpledisk_t *currdisk;
   currdisk = getsimpledisk (devno);
   return(currdisk->depth[0]);
}


int simpledisk_get_slotno (int devno)
{
   simpledisk_t *currdisk;
   currdisk = getsimpledisk (devno);
   return(currdisk->slotno[0]);
}


int simpledisk_get_inbus (int devno)
{
   simpledisk_t *currdisk;
   currdisk = getsimpledisk (devno);
   return(currdisk->inbuses[0]);
}


int simpledisk_get_maxoutstanding (int devno)
{
   simpledisk_t *currdisk;
   currdisk = getsimpledisk (devno);
   return(currdisk->maxqlen);
}


double simpledisk_get_blktranstime (ioreq_event *curr)
{
   simpledisk_t *currdisk;
   double tmptime;

   currdisk = getsimpledisk (curr->devno);
   tmptime = bus_get_transfer_time(simpledisk_get_busno(curr), 1, (curr->flags & READ));
   if (tmptime < currdisk->blktranstime) {
      tmptime = currdisk->blktranstime;
   }
   return(tmptime);
}


int simpledisk_get_busno (ioreq_event *curr)
{
   simpledisk_t *currdisk;
   intchar busno;
   int depth;

   currdisk = getsimpledisk (curr->devno);
   busno.value = curr->busno;
   depth = currdisk->depth[0];
   return(busno.byte[depth]);
}


/*
 * simpledisk_send_event_up_path()
 *
 * Acquires the bus (if not already acquired), then uses bus_delay to
 * send the event up the path.
 *
 * If the bus is already owned by this device or can be acquired
 * immediately (interleaved bus), the event is sent immediately.
 * Otherwise, disk_bus_ownership_grant will later send the event.
 */
  
static void simpledisk_send_event_up_path (ioreq_event *curr, double delay)
{
   simpledisk_t *currdisk;
   int busno;
   int slotno;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_send_event_up_path - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   currdisk = getsimpledisk (curr->devno);

   busno = simpledisk_get_busno(curr);
   slotno = currdisk->slotno[0];

   /* Put new request at head of buswait queue */
   curr->next = currdisk->buswait;
   currdisk->buswait = curr;

   curr->tempint1 = busno;
   curr->time = delay;
   if (currdisk->busowned == -1) {

      // fprintf (outputfile, "Must get ownership of the bus first\n");

      if (curr->next) {
         //fprintf(stderr,"Multiple bus requestors detected in disk_send_event_up_path\n");
         /* This should be ok -- counting on the bus module to sequence 'em */
      }
      if (bus_ownership_get(busno, slotno, curr) == FALSE) {
         /* Remember when we started waiting (only place this is written) */
	 currdisk->stat.requestedbus = simtime;
      } else {
         bus_delay(busno, DEVICE, curr->devno, delay, curr); /* Never for SCSI */
      }
   } else if (currdisk->busowned == busno) {

      //fprintf (outputfile, "Already own bus - so send it on up\n");

      bus_delay(busno, DEVICE, curr->devno, delay, curr);
   } else {
      fprintf(stderr, "Wrong bus owned for transfer desired\n");
      exit(1);
   }
}


/*
  **-simpledisk_bus_ownership_grant

  Calls bus_delay to handle the event that the disk has been granted the bus.  I believe
  this is always initiated by a call to disk_send_even_up_path.

  */

void simpledisk_bus_ownership_grant (int devno, ioreq_event *curr, int busno, double arbdelay)
{
   simpledisk_t *currdisk;
   ioreq_event *tmp;

   currdisk = getsimpledisk (devno);

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_bus_ownership_grant - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   tmp = currdisk->buswait;
   while ((tmp != NULL) && (tmp != curr)) {
      tmp = tmp->next;
   }
   if (tmp == NULL) {
      fprintf(stderr, "Bus ownership granted to unknown simpledisk request - devno %d, busno %d\n", devno, busno);
      exit(1);
   }
   currdisk->busowned = busno;
   currdisk->stat.waitingforbus += arbdelay;
   //ASSERT (arbdelay == (simtime - currdisk->stat.requestedbus));
   currdisk->stat.numbuswaits++;
   bus_delay(busno, DEVICE, devno, tmp->time, tmp);
}


void simpledisk_bus_delay_complete (int devno, ioreq_event *curr, int sentbusno)
{
   simpledisk_t *currdisk;
   intchar slotno;
   intchar busno;
   int depth;

   currdisk = getsimpledisk (devno);

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_bus_delay_complete - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   if (curr == currdisk->buswait) {
      currdisk->buswait = curr->next;
   } else {
      ioreq_event *tmp = currdisk->buswait;
      while ((tmp->next != NULL) && (tmp->next != curr)) {
         tmp = tmp->next;
      }
      if (tmp->next != curr) {
         fprintf(stderr, "Bus delay complete for unknown simpledisk request - devno %d, busno %d\n", devno, busno.value);
         exit(1);
      }
      tmp->next = curr->next;
   }
   busno.value = curr->busno;
   slotno.value = curr->slotno;
   depth = currdisk->depth[0];
   slotno.byte[depth] = slotno.byte[depth] >> 4;
   curr->time = 0.0;
   if (depth == 0) {
      intr_request ((event *)curr);
   } else {
      bus_deliver_event(busno.byte[depth], slotno.byte[depth], curr);
   }
}


/* send completion up the line */

static void simpledisk_request_complete(ioreq_event *curr)
{
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_request_complete - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   currdisk = getsimpledisk (curr->devno);

   if ((curr = ioqueue_physical_access_done(currdisk->queue,curr)) == NULL) {
      fprintf(stderr, "simpledisk_request_complete:  ioreq_event not found by ioqueue_physical_access_done call\n");
      exit(1);
   }

   /* send completion interrupt */
   curr->type = IO_INTERRUPT_ARRIVE;
   curr->cause = COMPLETION;
   simpledisk_send_event_up_path(curr, currdisk->bus_transaction_latency);
}


static void simpledisk_bustransfer_complete (ioreq_event *curr)
{
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_bustransfer_complete - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   currdisk = getsimpledisk (curr->devno);

   if (curr->flags & READ) {
      simpledisk_request_complete (curr);

   } else {
      simpledisk_t *currdisk = getsimpledisk (curr->devno);

      if (currdisk->neverdisconnect == FALSE) {
         /* disconnect from bus */
         ioreq_event *tmp = ioreq_copy (curr);
         tmp->type = IO_INTERRUPT_ARRIVE;
         tmp->cause = DISCONNECT;
         simpledisk_send_event_up_path (tmp, currdisk->bus_transaction_latency);
      }

      /* do media access */
      currdisk->media_busy = TRUE;
      stat_update (&currdisk->stat.acctimestats, currdisk->acctime);
      curr->time = simtime + currdisk->acctime;
      curr->type = DEVICE_ACCESS_COMPLETE;
      addtointq ((event *) curr);
   }
}


static void simpledisk_reconnect_done (ioreq_event *curr)
{
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_reconnect_done - devno %d, type %d, cause %d, blkno %d\n", simtime, curr->devno, curr->type, curr->cause, curr->blkno);
#endif

   currdisk = getsimpledisk (curr->devno);

   if (curr->flags & READ) {
      if (currdisk->neverdisconnect) {
         /* Just holding on to bus; data transfer will be initiated when */
         /* media access is complete.                                    */
         addtoextraq((event *) curr);

      } else {
         /* data transfer: curr->bcount, which is still set to original */
         /* requested value, indicates how many blks to transfer.       */
         curr->type = DEVICE_DATA_TRANSFER_COMPLETE;
         simpledisk_send_event_up_path(curr, (double) 0.0);
      }

   } else {
      if (currdisk->reconnect_reason == DEVICE_ACCESS_COMPLETE) {
         simpledisk_request_complete (curr);

      } else {
         /* data transfer: curr->bcount, which is still set to original */
         /* requested value, indicates how many blks to transfer.       */
         curr->type = DEVICE_DATA_TRANSFER_COMPLETE;
         simpledisk_send_event_up_path(curr, (double) 0.0);
      }
   }
}


static void simpledisk_request_arrive (ioreq_event *curr)
{
   ioreq_event *intrp;
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_request_arrive - devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
#endif

   currdisk = getsimpledisk(curr->devno);

   /* verify that request is valid. */
   if ((curr->blkno < 0) || (curr->bcount <= 0) ||
       ((curr->blkno + curr->bcount) > currdisk->numblocks)) {
      fprintf(stderr, "Invalid set of blocks requested from simpledisk - blkno %d, bcount %d, numblocks %d\n", curr->blkno, curr->bcount, currdisk->numblocks);
      exit(1);
   }

   /* create a new request, set it up for initial interrupt */
   currdisk->busowned = simpledisk_get_busno(curr);

   if (ioqueue_get_reqoutstanding (currdisk->queue) == 0) {
      ioqueue_add_new_request(currdisk->queue, curr);
      curr = ioqueue_get_next_request (currdisk->queue);
      intrp = curr;

      /* initiate media access if request is a READ */
      if (curr->flags & READ) {
         ioreq_event *tmp = ioreq_copy (curr);
         currdisk->media_busy = TRUE;
         stat_update (&currdisk->stat.acctimestats, currdisk->acctime);
         tmp->time = simtime + currdisk->acctime;
         tmp->type = DEVICE_ACCESS_COMPLETE;
         addtointq ((event *)tmp);
      }

      /* if not disconnecting, then the READY_TO_TRANSFER is like a RECONNECT */
      currdisk->reconnect_reason = IO_INTERRUPT_ARRIVE;
      if (curr->flags & READ) {
         intrp->cause = (currdisk->neverdisconnect) ? READY_TO_TRANSFER : DISCONNECT;
      } else {
         intrp->cause = READY_TO_TRANSFER;
      }

   } else {
      intrp = ioreq_copy(curr);
      ioqueue_add_new_request(currdisk->queue, curr);
      intrp->cause = DISCONNECT;
   }

   intrp->type = IO_INTERRUPT_ARRIVE;
   simpledisk_send_event_up_path(intrp, currdisk->bus_transaction_latency);
}


static void simpledisk_access_complete (ioreq_event *curr)
{
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_access_complete - devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
#endif

   currdisk = getsimpledisk (curr->devno);
   currdisk->media_busy = FALSE;

   if (currdisk->neverdisconnect) {
      /* already connected */
      if (curr->flags & READ) {
         /* transfer data up the line: curr->bcount, which is still set to */
         /* original requested value, indicates how many blks to transfer. */
         curr->type = DEVICE_DATA_TRANSFER_COMPLETE;
         simpledisk_send_event_up_path(curr, (double) 0.0);

      } else {
         simpledisk_request_complete (curr);
      }

   } else {
      /* reconnect to controller */
      curr->type = IO_INTERRUPT_ARRIVE;
      curr->cause = RECONNECT;
      simpledisk_send_event_up_path (curr, currdisk->bus_transaction_latency);
      currdisk->reconnect_reason = DEVICE_ACCESS_COMPLETE;
   }
}


/* intermediate disconnect done */

static void simpledisk_disconnect_done (ioreq_event *curr)
{
   simpledisk_t *currdisk;

   currdisk = getsimpledisk (curr->devno);

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_disconnect_done - devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
#endif

   addtoextraq((event *) curr);

   if (currdisk->busowned != -1) {
      bus_ownership_release(currdisk->busowned);
      currdisk->busowned = -1;
   }
}


/* completion disconnect done */

static void simpledisk_completion_done (ioreq_event *curr)
{
   simpledisk_t *currdisk = getsimpledisk (curr->devno);

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_completion_done - devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
#endif

   addtoextraq((event *) curr);

   if (currdisk->busowned != -1) {
      bus_ownership_release(currdisk->busowned);
      currdisk->busowned = -1;
   }

   /* check for and start next queued request, if any */
   curr = ioqueue_get_next_request(currdisk->queue);
   if (curr != NULL) {
      ASSERT (currdisk->media_busy == FALSE);
      if (curr->flags & READ) {
         currdisk->media_busy = TRUE;
         stat_update (&currdisk->stat.acctimestats, currdisk->acctime);
         curr->time = simtime + currdisk->acctime;
         curr->type = DEVICE_ACCESS_COMPLETE;
         addtointq ((event *)curr);

      } else {
         curr->type = IO_INTERRUPT_ARRIVE;
         curr->cause = RECONNECT;
         simpledisk_send_event_up_path (curr, currdisk->bus_transaction_latency);
         currdisk->reconnect_reason = IO_INTERRUPT_ARRIVE;
      }
   }
}


static void simpledisk_interrupt_complete (ioreq_event *curr)
{
#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_interrupt_complete - devno %d, blkno %d, bcount %d, flags 0x%x, cause %d\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, curr->cause );
#endif

   switch (curr->cause) {

      case RECONNECT:
         simpledisk_reconnect_done(curr);
	 break;

      case DISCONNECT:
	 simpledisk_disconnect_done(curr);
	 break;

      case COMPLETION:
	 simpledisk_completion_done(curr);
	 break;

      default:
         ddbg_assert2(0, "bad event type");
         break;
   }
}


void simpledisk_event_arrive (ioreq_event *curr)
{
   simpledisk_t *currdisk;

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_event_arrive - devno %d, blkno %d, bcount %d, flags 0x%x, cause %d\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, curr->cause );
   // fprintf (outputfile, "Entered simpledisk_event_arrive: time %f (simtime %f)\n", curr->time, simtime);
   // fprintf (outputfile, " - devno %d, blkno %d, type %d, cause %d, read = %d\n", curr->devno, curr->blkno, curr->type, curr->cause, curr->flags & READ);
#endif

   currdisk = getsimpledisk (curr->devno);

   switch (curr->type) {

      case IO_ACCESS_ARRIVE:
         curr->time = simtime + currdisk->overhead;
         curr->type = DEVICE_OVERHEAD_COMPLETE;
         addtointq((event *) curr);
         break;

      case DEVICE_OVERHEAD_COMPLETE:
         simpledisk_request_arrive(curr);
         break;

      case DEVICE_ACCESS_COMPLETE:
         simpledisk_access_complete (curr);
         break;

      case DEVICE_DATA_TRANSFER_COMPLETE:
         simpledisk_bustransfer_complete(curr);
         break;

      case IO_INTERRUPT_COMPLETE:
         simpledisk_interrupt_complete(curr);
         break;

      case IO_QLEN_MAXCHECK:
         /* Used only at initialization time to set up queue stuff */
         curr->tempint1 = -1;
         curr->tempint2 = simpledisk_get_maxoutstanding(curr->devno);
         curr->bcount = 0;
         break;

      default:
         fprintf(stderr, "Unrecognized event type at simpledisk_event_arrive\n");
         exit(1);
   }

   // fprintf (outputfile, "Exiting simpledisk_event_arrive\n");
}


int simpledisk_get_number_of_blocks (int devno)
{
   simpledisk_t *currdisk = getsimpledisk (devno);
   return (currdisk->numblocks);
}


int simpledisk_get_numcyls (int devno)
{
   simpledisk_t *currdisk = getsimpledisk (devno);
   return (currdisk->numblocks);
}


void simpledisk_get_mapping (int maptype, int devno, int blkno, int *cylptr, int *surfaceptr, int *blkptr)
{
   simpledisk_t *currdisk = getsimpledisk (devno);

   if ((blkno < 0) || (blkno >= currdisk->numblocks)) {
      fprintf(stderr, "Invalid blkno at simpledisk_get_mapping: %d\n", blkno);
      exit(1);
   }

   if (cylptr) {
      *cylptr = blkno;
   }
   if (surfaceptr) {
      *surfaceptr = 0;
   }
   if (blkptr) {
      *blkptr = 0;
   }
}


int simpledisk_get_avg_sectpercyl (int devno)
{
   return (1);
}


int simpledisk_get_distance (int devno, ioreq_event *req, int exact, int direction)
{
   /* just return an arbitrary constant, since acctime is constant */
   return 1;
}


double  simpledisk_get_servtime (int devno, ioreq_event *req, int checkcache, double maxtime)
{
   simpledisk_t *currdisk = getsimpledisk (devno);
   return (currdisk->acctime);
}


double  simpledisk_get_acctime (int devno, ioreq_event *req, double maxtime)
{
   simpledisk_t *currdisk = getsimpledisk (devno);
   return (currdisk->acctime);
}


int simpledisk_get_numdisks (void)
{
   return(numsimpledisks);
}


void simpledisk_cleanstats (void)
{
   int i;

   for (i=0; i<MAXDEVICES; i++) {
      simpledisk_t *currdisk = getsimpledisk (i);
      if (currdisk) {
         ioqueue_cleanstats(currdisk->queue);
      }
   }
}




static void simpledisk_statinit (int devno, int firsttime)
{
   simpledisk_t *currdisk;

   currdisk = getsimpledisk (devno);
   if (firsttime) {
      stat_initialize(statdeffile, statdesc_acctimestats, &currdisk->stat.acctimestats);
   } else {
      stat_reset(&currdisk->stat.acctimestats);
   }

   currdisk->stat.requestedbus = 0.0;
   currdisk->stat.waitingforbus = 0.0;
   currdisk->stat.numbuswaits = 0;
}


static void simpledisk_postpass (void)
{
}


void simpledisk_setcallbacks ()
{
   ioqueue_setcallbacks();
}


static void simpledisk_initialize_diskinfo ()
{
   disksim->simplediskinfo = (struct simpledisk_info *)calloc (1, sizeof(simplediskinfo_t));
   disksim->simplediskinfo->simpledisks = (struct simpledisk **)calloc(1, MAXDEVICES * (sizeof(simpledisk_t)));
   disksim->simplediskinfo->simpledisks_len = MAXDEVICES;
}


void simpledisk_initialize (void)
{
   int i;

   if (disksim->simplediskinfo == NULL) {
      simpledisk_initialize_diskinfo ();
   }

#ifdef DEBUG_SIMPLEDISK
   fprintf (outputfile, "*** %f: simpledisk_initialize - numsimpledisks %d\n", simtime, numsimpledisks );
#endif

   simpledisk_setcallbacks();
   simpledisk_postpass();

   for (i=0; i<MAXDEVICES; i++) {
      simpledisk_t *currdisk = getsimpledisk (i);
      if(!currdisk) continue;
/*        if (!currdisk->inited) { */
         currdisk->media_busy = FALSE;
         currdisk->reconnect_reason = -1;
         addlisttoextraq ((event **) &currdisk->buswait);
         currdisk->busowned = -1;
         ioqueue_initialize (currdisk->queue, i);
         simpledisk_statinit(i, TRUE);
/*        } */
   }
}


void simpledisk_resetstats (void)
{
   int i;

   for (i=0; i<MAXDEVICES; i++) {
      simpledisk_t *currdisk = getsimpledisk (i);
      if (currdisk) {
         ioqueue_resetstats(currdisk->queue);
         simpledisk_statinit(i, 0);
      }
   }
}



int simpledisk_add(struct simpledisk *d) {
  int c;

  if(!disksim->simplediskinfo) simpledisk_initialize_diskinfo();
  
  for(c = 0; c < disksim->simplediskinfo->simpledisks_len; c++) {
    if(!disksim->simplediskinfo->simpledisks[c]) {
      disksim->simplediskinfo->simpledisks[c] = d;
      numsimpledisks++;
      return c;
    }
  }

  /* note that numdisks must be equal to diskinfo->disks_len */
  disksim->simplediskinfo->simpledisks = 
    (struct simpledisk **)realloc(disksim->simplediskinfo->simpledisks,
	    2 * c * sizeof(struct simpledisk *));

  bzero(&(disksim->simplediskinfo->simpledisks[numsimpledisks]), 
	numsimpledisks*sizeof(void*));

  disksim->simplediskinfo->simpledisks[c] = d;
  numsimpledisks++;
  disksim->simplediskinfo->simpledisks_len *= 2;
  return c;
}


struct simpledisk *disksim_simpledisk_loadparams(struct lp_block *b)
{
  /* temp vars for parameters */
  struct simpledisk *result;
  int num;

  if(!disksim->simplediskinfo) simpledisk_initialize_diskinfo();

  result = (struct simpledisk *)calloc(1, sizeof(struct simpledisk));
  if(!result) return 0;
  
  num = simpledisk_add(result);

  result->hdr = simpledisk_hdr_initializer; 
  if(b->name)
    result->hdr.device_name = strdup(b->name);

  //#include "modules/disksim_simpledisk_param.c"
  lp_loadparams(result, b, &disksim_simpledisk_mod);



  device_add((struct device_header *)result, num);
  return result;
}


struct device_header *simpledisk_copy(struct device_header *orig) {
  struct simpledisk *result = (struct simpledisk *)calloc(1, sizeof(struct simpledisk));
  memcpy(result, orig, sizeof(struct simpledisk));
  result->queue = ioqueue_copy(((struct simpledisk *)orig)->queue);

  return (struct device_header *)result;
}

void simpledisk_set_syncset (int setstart, int setend)
{
}


static void simpledisk_acctime_printstats (int *set, int setsize, char *prefix)
{
   int i;
   statgen * statset[MAXDEVICES];

   if (device_printacctimestats) {
      for (i=0; i<setsize; i++) {
         simpledisk_t *currdisk = getsimpledisk (set[i]);
         statset[i] = &currdisk->stat.acctimestats;
      }
      stat_print_set(statset, setsize, prefix);
   }
}


static void simpledisk_other_printstats (int *set, int setsize, char *prefix)
{
   int i;
   int numbuswaits = 0;
   double waitingforbus = 0.0;

   for (i=0; i<setsize; i++) {
      simpledisk_t *currdisk = getsimpledisk (set[i]);
      numbuswaits += currdisk->stat.numbuswaits;
      waitingforbus += currdisk->stat.waitingforbus;
   }

   fprintf(outputfile, "%sTotal bus wait time: %f\n", prefix, waitingforbus);
   fprintf(outputfile, "%sNumber of bus waits: %d\n", prefix, numbuswaits);
}


void simpledisk_printsetstats (int *set, int setsize, char *sourcestr)
{
   int i;
   struct ioq * queueset[MAXDEVICES];
   int reqcnt = 0;
   char prefix[80];

   sprintf(prefix, "%ssimpledisk ", sourcestr);
   for (i=0; i<setsize; i++) {
      simpledisk_t *currdisk = getsimpledisk (set[i]);
      queueset[i] = currdisk->queue;
      reqcnt += ioqueue_get_number_of_requests(currdisk->queue);
   }
   if (reqcnt == 0) {
      fprintf (outputfile, "\nNo simpledisk requests for members of this set\n\n");
      return;
   }
   ioqueue_printstats(queueset, setsize, prefix);

   simpledisk_acctime_printstats(set, setsize, prefix);
   simpledisk_other_printstats(set, setsize, prefix);
}


void simpledisk_printstats (void)
{
   struct ioq * queueset[MAXDEVICES];
   int set[MAXDEVICES];
   int i;
   int reqcnt = 0;
   char prefix[80];
   int diskcnt;

   fprintf(outputfile, "\nSIMPLEDISK STATISTICS\n");
   fprintf(outputfile, "---------------------\n\n");

   sprintf(prefix, "Simpledisk ");

   diskcnt = 0;
   for (i=0; i<MAXDEVICES; i++) {
      simpledisk_t *currdisk = getsimpledisk (i);
      if (currdisk) {
         queueset[diskcnt] = currdisk->queue;
         reqcnt += ioqueue_get_number_of_requests(currdisk->queue);
         diskcnt++;
      }
   }
   assert (diskcnt == numsimpledisks);

   if (reqcnt == 0) {
      fprintf(outputfile, "No simpledisk requests encountered\n");
      return;
   }

   ioqueue_printstats(queueset, numsimpledisks, prefix);

   diskcnt = 0;
   for (i=0; i<MAXDEVICES; i++) {
      simpledisk_t *currdisk = getsimpledisk (i);
      if (currdisk) {
         set[diskcnt] = i;
         diskcnt++;
      }
   }
   assert (diskcnt == numsimpledisks);

   simpledisk_acctime_printstats(set, numsimpledisks, prefix);
   simpledisk_other_printstats(set, numsimpledisks, prefix);
   fprintf (outputfile, "\n\n");

   if (numsimpledisks <= 1) {
      return;
   }

   for (i=0; i<numsimpledisks; i++) {
      simpledisk_t *currdisk = getsimpledisk (set[i]);
      if (currdisk->printstats == FALSE) {
	 continue;
      }
      if (ioqueue_get_number_of_requests(currdisk->queue) == 0) {
	 fprintf(outputfile, "No requests for simpledisk #%d\n\n\n", set[i]);
	 continue;
      }
      fprintf(outputfile, "Simpledisk #%d:\n\n", set[i]);
      sprintf(prefix, "Simpledisk #%d ", set[i]);
      ioqueue_printstats(&currdisk->queue, 1, prefix);
      simpledisk_acctime_printstats(&set[i], 1, prefix);
      simpledisk_other_printstats(&set[i], 1, prefix);
      fprintf (outputfile, "\n\n");
   }
}


double simpledisk_get_seektime (int devno, 
				ioreq_event *req, 
				int checkcache, 
				double maxtime)
{
  fprintf(stderr, "device_get_seektime not supported for simpledisk devno %d\n",  devno);
  return 0.0;
}

/* default simpledisk dev header */
struct device_header simpledisk_hdr_initializer = { 
  DEVICETYPE_SIMPLEDISK,
  sizeof(struct simpledisk),
  "unnamed simpledisk",
  simpledisk_copy,
  simpledisk_set_depth,
  simpledisk_get_depth,
  simpledisk_get_inbus,
  simpledisk_get_busno,
  simpledisk_get_slotno,
  simpledisk_get_number_of_blocks,
  simpledisk_get_maxoutstanding,
  simpledisk_get_numcyls,
  simpledisk_get_blktranstime,
  simpledisk_get_avg_sectpercyl,
  simpledisk_get_mapping,
  simpledisk_event_arrive,
  simpledisk_get_distance,
  simpledisk_get_servtime,
  simpledisk_get_seektime,
  simpledisk_get_acctime,
  simpledisk_bus_delay_complete,
  simpledisk_bus_ownership_grant
};

// End of file


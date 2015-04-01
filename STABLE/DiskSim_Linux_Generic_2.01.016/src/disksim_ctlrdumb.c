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
#include "disksim_bus.h"


/* Times for passthru controller */

#define PASSTHRU_DELAY		1.0


static void controller_53c700_reset_complete (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *tmp;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_reset_complete - devno %d, blkno %d, bcount %d, flags %X\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush( outputfile);
#endif

   switch (currctlr->state) {
      case COMPLETION_PENDING:
      case DISCONNECT_PENDING:
         break;
      default:
         fprintf(stderr, "Controller not in appropriate state for reset completion\n");
         exit(1);
   }
   currctlr->state = FREE;
   addtoextraq((event *) curr);
   if (currctlr->connections != NULL) {
/*
      fprintf (outputfile, "Pending reconnection for controller %d\n", ctlno);
*/
      tmp = currctlr->connections;
      currctlr->connections = tmp->next;
      currctlr->state = RECONNECTING;
      controller_send_event_up_path(currctlr, tmp, currctlr->ovrhd_reset);
   }
}


static void controller_53c700_reconnection_complete (controller *currctlr, ioreq_event *curr)
{
   int read = curr->flags & READ;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_reconnection_complete - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush( outputfile );
#endif

    ASSERT(currctlr->state == RECONNECTING);
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_reconnect);
   currctlr->outbusowned = -1;
   currctlr->state = (read) ? READ_DATA_TRANSFER : WRITE_DATA_TRANSFER;
}


static void controller_53c700_reconnect_to_transfer (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_reconnect_to_transfer - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush( outputfile );
#endif

    switch (currctlr->state) {

      case COMPLETION_PENDING:
      case DISCONNECT_PENDING:
         curr->next = currctlr->connections;
         currctlr->connections = curr;
         break;

      case REQUEST_PENDING:
	 controller_remove_type_from_buswait(currctlr, IO_ACCESS_ARRIVE);

      case FREE:
         currctlr->state = RECONNECTING;
         controller_send_event_up_path(currctlr, curr, currctlr->ovrhd_reconnect);
	 break;

      default:
	 fprintf(stderr, "Received reconnection request in invalid state - ctlno %d, state %d\n", currctlr->ctlno, currctlr->state);
	 exit(1);
   }
}


static void controller_53c700_request_complete (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *ret;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_request_complete - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
	fflush( outputfile );
#endif

   switch (currctlr->state) {
      case READ_DATA_TRANSFER:
      case WRITE_DATA_TRANSFER:
         break;
      default:
         fprintf(stderr, "Completing request not transfering in controller_53c700_request_complete\n");
         exit(1);
   }
   ret = ioreq_copy(curr);
   ret->type = IO_INTERRUPT_COMPLETE;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, ret, currctlr->ovrhd_disk_complete);
   currctlr->outbusowned = -1;
   currctlr->state = COMPLETION_PENDING;
   controller_send_event_up_path(currctlr, curr, currctlr->ovrhd_complete);
}


static void controller_53c700_disconnect (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *ret;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_disconnect - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
#endif

   switch (currctlr->state) {
      case READ_DATA_TRANSFER:
      case WRITE_DATA_TRANSFER:
      case REQUEST_PENDING:
         break;
      default:
         fprintf(stderr, "Completing request not transfering in controller_53c700_request_complete\n");
         exit(1);
   }
   ret = ioreq_copy(curr);
   ret->type = IO_INTERRUPT_COMPLETE;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, ret, currctlr->ovrhd_disk_disconnect);
   currctlr->outbusowned = -1;
   currctlr->state = DISCONNECT_PENDING;
   controller_send_event_up_path(currctlr, curr, currctlr->ovrhd_disconnect);
}


static void controller_53c700_ready_to_transfer (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_ready_to_transfer - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush( outputfile );
#endif

    ASSERT(currctlr->state == REQUEST_PENDING);
   currctlr->state = (curr->flags & READ) ? READ_DATA_TRANSFER : WRITE_DATA_TRANSFER;
   curr->type = IO_INTERRUPT_COMPLETE;
   curr->cause = RECONNECT;
   currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_ready);
   /* shouldn't we also be sending a message up the path?? ovrhd_ready?? */
   currctlr->outbusowned = -1;
}


static void controller_53c700_request_arrive (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_request_arrive - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush(outputfile );
#endif

    switch (currctlr->state) {
      case FREE:
		           break;
      case RECONNECTING:
			   addtoextraq((event *) curr);
		           return;
      default:
                   fprintf(stderr, "Request arriving at non-FREE controller\n");
                   exit(1);
   }
   controller_send_event_down_path(currctlr, curr, currctlr->ovrhd_disk_request);
   currctlr->state = REQUEST_PENDING;
}


static void controller_53c700_interrupt_arrive (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRDUMB
    dumpIOReq( "controller_53c700_interrupt_arrive", curr );
    fflush(outputfile );
#endif

    switch (curr->cause) {

      case COMPLETION:
	 controller_53c700_request_complete(currctlr, curr);
	 break;

      case RECONNECT:
	 controller_53c700_reconnect_to_transfer(currctlr, curr);
	 break;

      case DISCONNECT:
	 controller_53c700_disconnect(currctlr, curr);
	 break;

      case READY_TO_TRANSFER:
	 controller_53c700_ready_to_transfer(currctlr, curr);
	 break;

      default:
         fprintf(stderr, "Unknown interrupt cause in 53c700_interrupt_arrive\n");
         exit(1);
   }
}


static void controller_53c700_interrupt_complete (controller *currctlr, ioreq_event *curr)
{
#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_interrupt_complete - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush(outputfile );
#endif

    switch (curr->cause) {

      case COMPLETION:
      case DISCONNECT:
	 controller_53c700_reset_complete(currctlr, curr);
	 break;

      case RECONNECT:
	 controller_53c700_reconnection_complete(currctlr, curr);
	 break;

      default:
         fprintf(stderr, "Unknown interrupt cause in 53c700_interrupt_complete\n");
         exit(1);
   }
}


static void controller_53c700_data_transfer (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *tmp;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_data_transfer - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush(outputfile );
/*
fprintf (outputfile, "Entered controller_53c700_data_transfer - ctlno %d, devno %d, blkno %d, bcount %d\n", ctlno, curr->devno, curr->blkno, curr->bcount);
*/
#endif

   if (curr->type == DEVICE_DATA_TRANSFER_COMPLETE) {
      curr->time = max(device_get_blktranstime(curr), currctlr->blktranstime);
   } else if (curr->type == CONTROLLER_DATA_TRANSFER_COMPLETE) {
      fprintf(stderr, "Not yet equiped to handle controller to controller transfers\n");
      exit(1);
   } else {
      fprintf(stderr, "Illegal event type at controller_53c700_data_transfer\n");
      exit(1);
   }
   curr->next = currctlr->datatransfers;
   curr->prev = NULL;
   if (curr->next) {
      curr->next->prev = curr;
   }
   currctlr->datatransfers = curr;
   tmp = (ioreq_event *) getfromextraq();
   tmp->time = simtime + (curr->time * (double) curr->bcount);
   tmp->type = CONTROLLER_DATA_TRANSFER_COMPLETE;
   tmp->blkno = curr->blkno;
   tmp->bcount = curr->bcount;
   tmp->tempint2 = currctlr->ctlno;
   tmp->tempptr1 = curr;
   curr->buf = tmp;
   addtointq((event *) tmp);
}


static void controller_53c700_data_transfer_complete (controller *currctlr, ioreq_event *curr)
{
   ioreq_event *tmp;

#ifdef DEBUG_CTLRDUMB
    fprintf (outputfile, "*** %f: controller_53c700_data_transfer_complete - devno %d, blkno %d\n, bcount %d, flags 0x%x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush(outputfile );
#endif

   tmp = (ioreq_event *) curr->tempptr1;
   tmp->bcount -= curr->bcount;
   addtoextraq((event *) curr);
   if (tmp->bcount < 0) {
      fprintf(stderr, "Transfered more than requested at controller_data_transfer_done\n");
      exit(1);
   } else if (tmp->bcount == 0) {
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
      fprintf(stderr, "Haven't required less than all out transfer at controller_data_transfer_done\n");
      exit(1);
   }
}


void controller_53c700_event_arrive (controller *currctlr, ioreq_event *curr)
{
   int busno;
   int slotno;

#ifdef DEBUG_CTLRDUMB
    dumpIOReq("controller_53c700_event_arrive", curr );
    fflush(outputfile );
#endif

   switch (curr->type) {

      case IO_ACCESS_ARRIVE:
	 controller_53c700_request_arrive(currctlr, curr);
	 break;

      case IO_INTERRUPT_ARRIVE:
	 controller_53c700_interrupt_arrive(currctlr, curr);
	 break;

      case IO_INTERRUPT_COMPLETE:
	 controller_53c700_interrupt_complete(currctlr, curr);
	 break;

      case DEVICE_DATA_TRANSFER_COMPLETE:
	 controller_53c700_data_transfer(currctlr, curr);
	 break;

      case CONTROLLER_DATA_TRANSFER_COMPLETE:
	 controller_53c700_data_transfer_complete(currctlr, curr);
	 break;

      case IO_QLEN_MAXCHECK:
	 busno = controller_get_downward_busno(currctlr, curr, &slotno);
	 bus_deliver_event(busno, slotno, curr);
	 break;

      default:
         fprintf(stderr, "Unknown event type arriving at 53c700 controller: %d\n", curr->type);
         exit(1);
   }
}


void controller_passthru_event_arrive (controller *currctlr, ioreq_event *curr)
{
   int busno;
   int slotno;

#ifdef DEBUG_CTLRDUMB
    dumpIOReq("controller_passthru_event_arrive", curr );
    fflush(outputfile );
#endif

   switch (curr->type) {

      case IO_INTERRUPT_COMPLETE:
	 currctlr->outbusowned = controller_get_downward_busno(currctlr, curr, NULL);
	 controller_send_event_down_path(currctlr, curr, PASSTHRU_DELAY);
	 currctlr->outbusowned = -1;
	 break;

      case IO_ACCESS_ARRIVE:
/*
fprintf (outputfile, "Following passthru_event path, and passing access down\n");
*/
	 controller_send_event_down_path(currctlr, curr, PASSTHRU_DELAY);
	 break;

      case IO_INTERRUPT_ARRIVE:
	 if (curr->cause == READY_TO_TRANSFER) {
	    curr->cause = RECONNECT;
         }
	 controller_send_event_up_path(currctlr, curr, PASSTHRU_DELAY);
	 break;

      case DEVICE_DATA_TRANSFER_COMPLETE:
	 controller_send_event_up_path(currctlr, curr, (double) 0.0);
	 break;

      case IO_QLEN_MAXCHECK:
	 busno = controller_get_downward_busno(currctlr, curr, &slotno);
	 bus_deliver_event(busno, slotno, curr);
	 break;

      default:
         fprintf(stderr, "Unknown event type arriving at passthru controller: %d\n", curr->type);
         exit(1);
   }
}


void controller_passthru_printstats (controller *currctlr, char *prefix)
{
         fprintf(outputfile, "NCR 53C700 controller statistics not implemented\n" );
}


void controller_53c700_printstats (controller *currctlr, char *prefix)
{
         fprintf(outputfile, "NCR 53C700 controller statistics not implemented\n" );
}

// End of file


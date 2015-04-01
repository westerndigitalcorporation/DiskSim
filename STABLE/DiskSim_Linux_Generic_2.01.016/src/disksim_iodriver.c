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
#include "disksim_stat.h"
#include "disksim_iosim.h"
#include "disksim_iotrace.h"
#include "disksim_iodriver.h"
#include "disksim_logorg.h"
#include "disksim_orgface.h"
#include "disksim_ioqueue.h"
#include "disksim_bus.h"
#include "disksim_controller.h"
#include "disksim_simresult.h"
#include "config.h"

#include "modules/modules.h"

/* read-only globals used during readparams phase */
static char *statdesc_emptyqueue	=  "Empty queue delay";
static char *statdesc_initiatenext	=  "Initiate next delay";


struct iodriver *getiodriverbyname(char *name, int *num) {
  int c;
  if(!disksim->iodriver_info) return 0;

  for(c = 0; c < numiodrivers; c++) {
    if(iodrivers[c])
      if(!strcmp(iodrivers[c]->name, name)) {
	if(num) *num = c;
	return iodrivers[c];
      }
  }

  return 0;
}


static void iodriver_send_event_down_path (ioreq_event *curr)
{
   intchar busno;
   intchar slotno;

   busno.value = curr->busno;
   slotno.value = curr->slotno;
   slotno.byte[0] = slotno.byte[0] & 0x0F;
#ifdef DEBUG_IODRIVER
   fprintf (outputfile, "%f: iodriver_send_event_down_path: busno %d, slotno %d, type=%d\n", simtime, busno.byte[0], slotno.byte[0], curr->type);
#endif
   bus_deliver_event(busno.byte[0], slotno.byte[0], curr);
}


double iodriver_raise_priority (int iodriverno, int opid, int devno, int blkno, void *chan)
{
   logorg_raise_priority(sysorgs, numsysorgs, opid, devno, blkno, chan);
   return(0.0);
}


static void schedule_disk_access (iodriver *curriodriver, ioreq_event *curr)
{
   int queuectlr = curriodriver->devices[(curr->devno)].queuectlr;
   if (queuectlr != -1) {
      curriodriver->ctlrs[queuectlr].numoutstanding++;
   }
   curriodriver->devices[(curr->devno)].busy = TRUE;
}


static int check_send_out_request (iodriver *curriodriver, int devno)
{
   int numout;

   if ((curriodriver->consttime == IODRIVER_TRACED_QUEUE_TIMES) || (curriodriver->consttime == IODRIVER_TRACED_BOTH_TIMES)) {
      return(FALSE);
   }
   numout = ioqueue_get_reqoutstanding(curriodriver->devices[devno].queue);
   if (curriodriver->usequeue == TRUE) {
      int queuectlr = curriodriver->devices[devno].queuectlr;
      if (queuectlr != -1) {
         numout = curriodriver->ctlrs[queuectlr].numoutstanding;
/*
fprintf (outputfile, "Check send_out_req: queuectlr %d, numout %d, maxout %d, send %d\n", queuectlr, numout, curriodriver->ctlrs[queuectlr].maxoutstanding, (numout < curriodriver->ctlrs[queuectlr].maxoutstanding));
*/
         return(numout < curriodriver->ctlrs[queuectlr].maxoutstanding);
      } else {
         return(numout < curriodriver->devices[devno].maxoutstanding);
      }
   } else {
      return(!numout);
   }
}


static ioreq_event * handle_new_request (iodriver *curriodriver, ioreq_event *curr)
{
   struct ioq *queue = curriodriver->devices[(curr->devno)].queue;
   ioreq_event *ret = NULL;

   /*
   fprintf(outputfile, "\n***  handle_new_request::  time %f, devno %d, blkno %d, bcount %d\n\n",
	   simtime, curr->devno, curr->blkno, curr->bcount);

   fprintf(outputfile, "handle_new_request::  calling ioqueue_add_new_request\n");
   */

   ioqueue_add_new_request(queue, curr);
   if (check_send_out_request(curriodriver, curr->devno)) {
     /*
     fprintf(outputfile, "handle_new_request::  calling ioqueue_get_next_request\n");
     */
      ret = ioqueue_get_next_request(queue);
      if (ret != NULL) {
         schedule_disk_access(curriodriver, ret);
         ret->time = IODRIVER_IMMEDSCHED_TIME * curriodriver->scale;
      }
   }
   return(ret);
}


void iodriver_schedule (int iodriverno, ioreq_event *curr)
{
   ctlr *ctl;

#ifdef DEBUG_IODRIVER
   fprintf (outputfile, "%f: iodriver_schedule - devno %d, blkno %d, bcount %d, read %d\n", simtime, curr->devno, curr->blkno, curr->bcount, (curr->flags & READ));
#endif

   ASSERT1(curr->type == IO_ACCESS_ARRIVE, "curr->type", curr->type);

   if ((iodrivers[iodriverno]->consttime != 0.0) 
       && (iodrivers[iodriverno]->consttime != IODRIVER_TRACED_QUEUE_TIMES)) 
   {
      curr->type = IO_INTERRUPT;
      if (iodrivers[iodriverno]->consttime > 0.0) {
         curr->time = iodrivers[iodriverno]->consttime;
      } 
      else {
         curr->time = ((double) curr->tempint2 / (double) 1000);
      }
      curr->cause = COMPLETION;
      intr_request((event *) curr);
      return;
   }

   ctl = iodrivers[iodriverno]->devices[(curr->devno)].ctl;

   if ((ctl) && (ctl->flags & DRIVER_C700)) {
      if ((ctl->pendio) 
	  && ((curr->devno != ctl->pendio->next->devno) 
	      || (curr->opid != ctl->pendio->next->opid) 
	      || (curr->blkno != ctl->pendio->next->blkno))) 
      {
         curr->next = ctl->pendio->next;
         ctl->pendio->next = curr;
         ctl->pendio = curr;
         return;
      } 
      else if (ctl->pendio == NULL) {
         ctl->pendio = ioreq_copy(curr);
         ctl->pendio->next = ctl->pendio;
      }
      if (ctl->flags & DRIVER_CTLR_BUSY) {
         addtoextraq((event *) curr);
         return;
      }
   }
   curr->busno = iodrivers[iodriverno]->devices[(curr->devno)].buspath.value;
   curr->slotno = iodrivers[iodriverno]->devices[(curr->devno)].slotpath.value;
   if (iodrivers[iodriverno]->devices[(curr->devno)].queuectlr != -1) {
      int ctlrno = iodrivers[iodriverno]->devices[(curr->devno)].queuectlr;
      ctl = &iodrivers[iodriverno]->ctlrs[ctlrno];
      if ((ctl->maxreqsize) && (curr->bcount > ctl->maxreqsize)) {
         ioreq_event *totalreq = ioreq_copy(curr);
/*
fprintf (outputfile, "%f, oversized request: opid %d, blkno %d, bcount %d, maxreqsize %d\n", simtime, curr->opid, curr->blkno, curr->bcount, ctl->maxreqsize);
*/
         curr->bcount = ctl->maxreqsize;
         if (ctl->oversized) {
            totalreq->next = ctl->oversized->next;
            ctl->oversized->next = totalreq;
         } else {
            totalreq->next = totalreq;
            ctl->oversized = totalreq;
         }
      }
   }
   iodriver_send_event_down_path(curr);
/*
fprintf (outputfile, "Leaving iodriver_schedule\n");
*/
}


static void update_iodriver_statistics()
{
}


static void iodriver_check_c700_based_status (iodriver *curriodriver, int devno, int cause, int type, int blkno)
{
   ctlr *ctl;
   ioreq_event *tmp;

   ctl = curriodriver->devices[devno].ctl;
   if ((ctl == NULL) || (!(ctl->flags & DRIVER_C700))) {
      return;
   }
   if (type == IO_INTERRUPT_ARRIVE) {
      if (ctl->pendio != NULL) {
         tmp = ctl->pendio->next;
         if ((tmp->devno == devno) && (tmp->blkno == blkno)) {
            if (ctl->pendio == tmp) {
               ctl->pendio = NULL;
            } else {
               ctl->pendio->next = tmp->next;
            }
            addtoextraq((event *) tmp);
         }
      }
      switch (cause) {
         case COMPLETION:
         case DISCONNECT:
         case RECONNECT:
         case READY_TO_TRANSFER:
                 ctl->flags |= DRIVER_CTLR_BUSY;
                 break;
         default:
                 fprintf(stderr, "Unknown interrupt cause at iodriver_check_c700_based_status - cause %d\n", cause);
                 exit(1);
      }
   } else {
      switch (cause) {
         case COMPLETION:
         case DISCONNECT:
                           ctl->flags &= ~DRIVER_CTLR_BUSY;
                           if (ctl->pendio != NULL) {
                              tmp = ioreq_copy(ctl->pendio->next);
                              tmp->time = simtime;
                              addtointq((event *) tmp);
                           }
                           break;
         case RECONNECT:
         case READY_TO_TRANSFER:
                           ctl->flags |= DRIVER_CTLR_BUSY;
                           break;
         default:
                           fprintf(stderr, "Unknown interrupt cause at iodriver_check_c700_based_status - cause %d\n", cause);
                           exit(1);
      }
   }
}


/* Should only be called for completion interrupts!!!!! */

static void iodriver_add_to_intrp_eventlist (intr_event *intrp, event *curr, double scale)
{
   if (curr) {
      event *tmp = intrp->eventlist;
      int lasttype = curr->type;
      intrp->eventlist = curr;
      if (curr->type == IO_ACCESS_ARRIVE) {
         curr->time = IODRIVER_COMPLETION_RESPTOSCHED_TIME;
      } else if (curr->type == IO_REQUEST_ARRIVE) {
         curr->time = IODRIVER_COMPLETION_RESPTOREQ_TIME;
      } else {       /* Assume wakeup */
         curr->time = IODRIVER_COMPLETION_RESPTOWAKEUP_TIME;
      }
      curr->time *= scale;
      while (curr->next) {
         curr = curr->next;
	 curr->time = 0.0;
      }
      curr->next = tmp;
      if (lasttype != IO_REQUEST_ARRIVE) {
         while (tmp) {
            if (tmp->type == INTEND_EVENT) {
               ASSERT(tmp->next == NULL);
               tmp->time = (lasttype == IO_ACCESS_ARRIVE) ? IODRIVER_COMPLETION_SCHEDTOEND_TIME : IODRIVER_COMPLETION_WAKEUPTOEND_TIME;
               tmp->time *= scale;
            } else if (tmp->type != IO_REQUEST_ARRIVE) {    /* assume wakeup */
               tmp->time = (lasttype == IO_ACCESS_ARRIVE) ? IODRIVER_COMPLETION_SCHEDTOWAKEUP_TIME : 0.0;
               lasttype = tmp->type;
               tmp->time *= scale;
            }
            tmp = tmp->next;
         }
      }
   }
}


void iodriver_access_complete (int iodriverno, intr_event *intrp)
{
   int i;
   int numreqs;
   ioreq_event *tmp;
   ioreq_event *del;
   ioreq_event *req;
   int devno;
   int skip = 0;
   ctlr *ctl = NULL;
   time_t now;

   if (iodrivers[iodriverno]->type == STANDALONE) {
      req = ioreq_copy((ioreq_event *) intrp->infoptr);
   } 
   else {
      req = (ioreq_event *) intrp->infoptr;
   }

#ifdef DEBUG_IODRIVER
   fprintf (outputfile, "*** %f: iodriver_access_complete - devno %d, blkno %d, bcount %d, read %d\n", simtime, req->devno, req->blkno, req->bcount, (req->flags & READ));
   fflush(outputfile);
#endif

   time( & now );
   disksim_exectrace( "%.6f,%d,%d,%d,%d,%d,%s", simtime, req->tagID, req->devno, req->blkno, req->bcount, req->flags, asctime( localtime(& now)) );

   if( NULL != DISKSIM_SIM_LOG )
   {
       simresult_dumpSimResult( DISKSIM_SIM_LOG, req->tagID - 1 );
   }

   if (iodrivers[iodriverno]->devices[(req->devno)].queuectlr != -1) 
   {
      int ctlrno = iodrivers[iodriverno]->devices[(req->devno)].queuectlr;
      ctl = &iodrivers[iodriverno]->ctlrs[ctlrno];
      tmp = ctl->oversized;
      numreqs = 1;


      while (((numreqs) || (tmp != ctl->oversized)) 
	     && (tmp) 
	     && (tmp->next) 
	     && ((tmp->next->devno != req->devno) 
		 || (tmp->next->opid != req->opid) 
		 || (req->blkno < tmp->next->blkno) 
		 || (req->blkno >= (tmp->next->blkno + tmp->next->bcount)))) 
      {
	
	// fprintf (outputfile, "oversized request in list: opid %d, blkno %d, bcount %d\n", tmp->opid, tmp->blkno, tmp->bcount);
	
	numreqs = 0;
	tmp = tmp->next;
      }


      if ((tmp) 
	  && (tmp->next->devno == req->devno) 
	  && (tmp->next->opid == req->opid) 
	  && (req->blkno >= tmp->next->blkno) 
	  && (req->blkno < (tmp->next->blkno + tmp->next->bcount))) 
      {
	fprintf (outputfile, "%f, part of oversized request completed: opid %d, blkno %d, bcount %d, maxreqsize %d\n", simtime, req->opid, req->blkno, req->bcount, ctl->maxreqsize);

	if ((req->blkno + ctl->maxreqsize) < (tmp->next->blkno + tmp->next->bcount)) 
	{
	  fprintf (outputfile, "more to go\n");
	  req->blkno += ctl->maxreqsize;
	  req->bcount = min(ctl->maxreqsize, 
			    (tmp->next->blkno + tmp->next->bcount - req->blkno));
	  goto schedule_next;
	} 
	else {
	  fprintf (outputfile, "done for real\n");

	  addtoextraq((event *) req);
	  req = tmp->next;
	  tmp->next = tmp->next->next;

	  if (ctl->oversized == req) {
	    ctl->oversized = (req != req->next) ? req->next : NULL;
	  }

	  req->next = NULL;
	}
      }
   }




   devno = req->devno;
   req = ioqueue_physical_access_done(iodrivers[iodriverno]->devices[devno].queue, req);

   if (ctl) {
      ctl->numoutstanding--;
   }

   // special case for validate:
   if (disksim->traceformat == VALIDATE) {
      tmp = (ioreq_event *) getfromextraq();
      io_validate_do_stats1();
      tmp = iotrace_validate_get_ioreq_event(disksim->iotracefile, tmp);
      if (tmp) {
         io_validate_do_stats2(tmp);
         tmp->type = IO_REQUEST_ARRIVE;
         addtointq((event *) tmp);

	 disksim_exectrace("Request issue: simtime %f, devno %d, blkno %d, time %f\n", 
			   simtime, tmp->devno, tmp->blkno, tmp->time);
      } 
      else {
         disksim_simstop();
      }
  } 
   else if (disksim->closedios) {
      tmp = (ioreq_event *) io_get_next_external_event(disksim->iotracefile);
      if (tmp) {
         io_using_external_event ((event *)tmp);
         tmp->time = simtime + disksim->closedthinktime;
         tmp->type = IO_REQUEST_ARRIVE;
         addtointq((event *) tmp);
      } else {
         disksim_simstop();
      }
   }

 

   while (req) {
      tmp = req;
      req = req->next;
      tmp->next = NULL;
      update_iodriver_statistics();
      if ((numreqs = logorg_mapcomplete(sysorgs, numsysorgs, tmp)) == COMPLETE) {

	/* update up overall I/O system stats for this completed request */
	ioreq_event *temp = ioqueue_get_specific_request (OVERALLQUEUE, tmp);
	ioreq_event *temp2 = ioqueue_physical_access_done (OVERALLQUEUE, temp);
	ASSERT (temp2 != NULL);
	addtoextraq((event *)temp);
	temp = NULL;
	
	if (iodrivers[iodriverno]->type != STANDALONE) {
	  iodriver_add_to_intrp_eventlist(intrp, 
					  io_done_notify(tmp), 
					  iodrivers[iodriverno]->scale);
         } else {
            io_done_notify (tmp);
         }
      } 
      else if (numreqs > 0) {
	for (i = 0; i < numreqs; i++) {
	  del = tmp->next;
	  tmp->next = del->next;
	  del->next = NULL;
	  del->type = IO_REQUEST_ARRIVE;
	  del->flags |= MAPPED;
	  skip |= (del->devno == devno);

	  if (iodrivers[iodriverno]->type == STANDALONE) 
	  {
	    del->time += simtime + 0.0000000001; /* to affect an ordering */
	    addtointq((event *) del);
	  } 
	  else {
	    iodriver_add_to_intrp_eventlist(intrp, 
					    (event *) del, 
					    iodrivers[iodriverno]->scale);
	  }
	}
      }
      addtoextraq((event *) tmp);
   }

   if ((iodrivers[iodriverno]->consttime == IODRIVER_TRACED_QUEUE_TIMES) 
       || (iodrivers[iodriverno]->consttime == IODRIVER_TRACED_BOTH_TIMES)) 
   {
     if (ioqueue_get_number_in_queue(iodrivers[iodriverno]->devices[devno].queue) > 0) 
     {
       iodrivers[iodriverno]->devices[devno].flag = 1;
       iodrivers[iodriverno]->devices[devno].lastevent = simtime;
     }
     return;
   }
   if (skip) {
     return;
   }


   // fprintf(outputfile, "iodriver_access_complete::  calling ioqueue_get_next_request\n");


   req = ioqueue_get_next_request(iodrivers[iodriverno]->devices[devno].queue);

   
   // fprintf (outputfile, "next scheduled: req %p, req->blkno %d, req->flags %x\n", req, ((req) ? req->blkno : 0), ((req) ? req->flags : 0));


schedule_next:
   if (req) {
      req->type = IO_ACCESS_ARRIVE;
      req->next = NULL;

      if (ctl) {
         ctl->numoutstanding++;
      }

      if (iodrivers[iodriverno]->type == STANDALONE) {
         req->time = simtime;
         addtointq((event *) req);
      } 
      else {
         iodriver_add_to_intrp_eventlist(intrp, 
					 (event *) req, 
					 iodrivers[iodriverno]->scale);
      }
   }
}


void iodriver_respond_to_device (int iodriverno, intr_event *intrp)
{
    ioreq_event *req = NULL;
    int devno;
    int cause;

    if ((iodrivers[iodriverno]->consttime != 0.0) && (iodrivers[iodriverno]->consttime != IODRIVER_TRACED_QUEUE_TIMES)) {
        if (iodrivers[iodriverno]->type == STANDALONE) {
            addtoextraq((event *) intrp->infoptr);
        }
        return;
    }
    req = (ioreq_event *) intrp->infoptr;
    /*
fprintf (outputfile, "%f, Responding to device - cause = %d, blkno %lld\n", simtime, req->cause, req->blkno);
     */
    req->type = IO_INTERRUPT_COMPLETE;
    devno = req->devno;
    cause = req->cause;
    switch (cause) {
    case COMPLETION:
        if (iodrivers[iodriverno]->type != STANDALONE) {
            req = ioreq_copy((ioreq_event *) intrp->infoptr);
        }
    case DISCONNECT:
    case RECONNECT:
        iodriver_send_event_down_path(req);
        break;
    case READY_TO_TRANSFER:
        addtoextraq((event *) req);
        break;
    default:
        fprintf(stderr, "Unknown io_interrupt cause - %d\n", req->cause);
        exit(1);
    }
    iodriver_check_c700_based_status(iodrivers[iodriverno], devno, cause, IO_INTERRUPT_COMPLETE, 0);
}


void iodriver_interrupt_complete (int iodriverno, intr_event *intrp)
{
/*
fprintf (outputfile, "%f, Interrupt completing - cause = %d, blkno %d\n", simtime, ((ioreq_event *) intrp->infoptr)->cause, ((ioreq_event *) intrp->infoptr)->blkno);
*/
   if (iodrivers[iodriverno]->type == STANDALONE) {
      if (((ioreq_event *) intrp->infoptr)->cause == COMPLETION) {
         iodriver_access_complete(iodriverno, intrp);
      }
      iodriver_respond_to_device(iodriverno, intrp);
   }
   addtoextraq((event *) intrp);
}


static double iodriver_get_time_to_handle_interrupt (iodriver *curriodriver, int cause, int read)
{
   double retval = 0.0;
   if (cause == DISCONNECT) {
      retval = (read) ? IODRIVER_READ_DISCONNECT_TIME : IODRIVER_WRITE_DISCONNECT_TIME;
   } else if (cause == RECONNECT) {
      retval = IODRIVER_RECONNECT_TIME;
   } else if (cause == COMPLETION) {
      retval = (IODRIVER_BASE_COMPLETION_TIME - IODRIVER_COMPLETION_RESPTODEV_TIME) * (double) 5;   /* because it gets divided by five later */
   } else if (cause != READY_TO_TRANSFER) {
      fprintf(stderr, "Unknown interrupt at time_to_handle_interrupt: %d\n", cause);
      exit(1);
   }
   return(curriodriver->scale * retval);
}


static double iodriver_get_time_to_respond_to_device (iodriver *curriodriver, int cause, double fifthoftotal)
{
   double retval;
   if (cause == COMPLETION) {
      retval = IODRIVER_COMPLETION_RESPTODEV_TIME;
   } else {
			/* arbitrarily chosen based on some observations */
      retval = fifthoftotal * (double) 4;
   }
   return(curriodriver->scale * retval);
}


void iodriver_interrupt_arrive (int iodriverno, intr_event *intrp)
{
   event *tmp;
   ioreq_event *infoptr = (ioreq_event *) intrp->infoptr;

#ifdef DEBUG_IODRIVER
   fprintf (outputfile, "%f, iodriver_interrupt_arrive - cause %d, blkno %d\n", simtime, infoptr->cause, infoptr->blkno);
#endif

   if ((iodrivers[iodriverno]->consttime == 0.0) || (iodrivers[iodriverno]->consttime == IODRIVER_TRACED_QUEUE_TIMES)) {
      intrp->time = iodriver_get_time_to_handle_interrupt(iodrivers[iodriverno], infoptr->cause, (infoptr->flags & READ));
      iodriver_check_c700_based_status(iodrivers[iodriverno], infoptr->devno, infoptr->cause, IO_INTERRUPT_ARRIVE, infoptr->blkno);
   } else {
      intrp->time = 0.0;
   }
   if (iodrivers[iodriverno]->type == STANDALONE) {
      intrp->time += simtime;
      intrp->type = IO_INTERRUPT_COMPLETE;
      addtointq((event *) intrp);
   } else {
      tmp = getfromextraq();
      intrp->time /= (double) 5;
      tmp->time = intrp->time;
      tmp->type = INTEND_EVENT;
      tmp->next = NULL;
      ((intr_event *)tmp)->vector = IO_INTERRUPT;
      intrp->eventlist = tmp;
      if (infoptr->cause == COMPLETION) {
         tmp = getfromextraq();
         tmp->time = 0.0;
         tmp->type = IO_ACCESS_COMPLETE;
         tmp->next = intrp->eventlist;
         ((ioreq_event *)tmp)->tempptr1 = intrp;
         intrp->eventlist = tmp;
      }
      tmp = getfromextraq();
      tmp->time = iodriver_get_time_to_respond_to_device(iodrivers[iodriverno], infoptr->cause, intrp->time);
      tmp->type = IO_RESPOND_TO_DEVICE;
      tmp->next = intrp->eventlist;
      ((ioreq_event *)tmp)->tempptr1 = intrp;
      intrp->eventlist = tmp;
   }
}


double iodriver_tick ()
{
   int i, j;
   double ret = 0.0;

   for (i = 0; i < numiodrivers; i++) {
      for (j = 0; j < iodrivers[i]->numdevices; j++) {
         ret += ioqueue_tick(iodrivers[i]->devices[j].queue);
      }
   }
   return(0.0);
}


event * iodriver_request (int iodriverno, ioreq_event *curr)
{
   ioreq_event *temp = NULL;
   ioreq_event *ret = NULL;
   ioreq_event *retlist = NULL;
   int numreqs;
/*
   printf ("Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
   fprintf (outputfile, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
   fprintf (stderr, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
*/

   if (NULL != OUTIOS) {
      fprintf(OUTIOS, "%.6f,%d,%d,%d,%x,%d,%d,%p\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, OVERALLQUEUE->base.listlen + 1, curr->tagID, curr );
      fflush(OUTIOS);
   }

#if 0
   fprintf (stderr, "Entered iodriver_request - simtime %f, devno %d, blkno %d, cause %d\n", simtime, curr->devno, curr->blkno, curr->cause);
#endif

   /* add to the overall queue to start tracking */
   ret = ioreq_copy (curr);
   ioqueue_add_new_request (OVERALLQUEUE, ret);
   ret = NULL;
 
   disksim->totalreqs++;
   if ((disksim->checkpoint_iocnt > 0) && ((disksim->totalreqs % disksim->checkpoint_iocnt) == 0)) {
      disksim_register_checkpoint (simtime);
   }
   if (disksim->totalreqs == disksim->warmup_iocnt) {
       WARMUPTIME = simtime;
      resetstats();
   }
   numreqs = logorg_maprequest(sysorgs, numsysorgs, curr);
   temp = curr->next;
   for (; numreqs>0; numreqs--) {
          /* Request list size must match numreqs */
      ASSERT(curr != NULL);
      curr->next = NULL;
      if ((iodrivers[iodriverno]->consttime == IODRIVER_TRACED_QUEUE_TIMES) || (iodrivers[iodriverno]->consttime == IODRIVER_TRACED_BOTH_TIMES)) {
         ret = ioreq_copy(curr);
         ret->time = simtime + (double) ret->tempint1 / (double) 1000;
         ret->type = IO_TRACE_REQUEST_START;
         addtointq((event *) ret);
         ret = NULL;
         if ((curr->slotno == 1) && (ioqueue_get_number_in_queue(iodrivers[iodriverno]->devices[(curr->devno)].queue) == 0)) {
            iodrivers[(iodriverno)]->devices[(curr->devno)].flag = 2;
            iodrivers[(iodriverno)]->devices[(curr->devno)].lastevent = simtime;
         }
      }
      ret = handle_new_request(iodrivers[iodriverno], curr);

      if ((ret) && (iodrivers[iodriverno]->type == STANDALONE) && (ret->time == 0.0)) {
         ret->type = IO_ACCESS_ARRIVE;
         ret->time = simtime;
         iodriver_schedule(iodriverno, ret);
      } else if (ret) {
         ret->type = IO_ACCESS_ARRIVE;
         ret->next = retlist;
         ret->prev = NULL;
         retlist = ret;
      }
      curr = temp;
      temp = (temp) ? temp->next : NULL;
   }
   if (iodrivers[iodriverno]->type == STANDALONE) {
      while (retlist) {
         ret = retlist;
         retlist = ret->next;
         ret->next = NULL;
         ret->time += simtime;
         addtointq((event *) ret);
      }
   }
/*
fprintf (outputfile, "leaving iodriver_request: retlist %p\n", retlist);
*/
   return((event *) retlist);
}


void iodriver_trace_request_start (int iodriverno, ioreq_event *curr)
{
   ioreq_event *tmp;
   device *currdev = &iodrivers[iodriverno]->devices[(curr->devno)];
   double tdiff = simtime - currdev->lastevent;

   if (currdev->flag == 1) {
      stat_update(&initiatenextstats, tdiff);
   } else if (currdev->flag == 2) {
      stat_update(&emptyqueuestats, tdiff);
   }
   currdev->flag = 0;

   tmp = ioqueue_get_specific_request(currdev->queue, curr);
   addtoextraq((event *) curr);
   ASSERT(tmp != NULL);

   schedule_disk_access(iodrivers[iodriverno], tmp);
   tmp->time = simtime;
   tmp->type = IO_ACCESS_ARRIVE;
   tmp->slotno = 0;
   if (tmp->time == simtime) {
      iodriver_schedule(iodriverno, tmp);
   } else {
      addtointq((event *) tmp);
   }
}


static void iodriver_set_ctl_to_device (int iodriverno, device * dev)
{
   int i;
   ctlr *tmp;

   dev->ctl = NULL;
   for (i=0; i < iodrivers[iodriverno]->numctlrs; i++) {
      tmp = &iodrivers[iodriverno]->ctlrs[i];
      if ((tmp->slotpath.value & dev->slotpath.value) == tmp->slotpath.value) {
         if ((tmp->buspath.value & dev->buspath.value) == tmp->buspath.value) {
            if (tmp->flags & DRIVER_C700) {
               dev->ctl = tmp;
               return;
            } else {
               dev->ctl = tmp;
            }
         }
      }
   }
}


static void get_device_maxoutstanding (iodriver *curriodriver, device * dev)
{
   ioreq_event *chk = (ioreq_event *) getfromextraq();

   chk->busno = dev->buspath.value;
   chk->slotno = dev->slotpath.value;
   chk->devno = dev->devno;
   chk->type = IO_QLEN_MAXCHECK;
   iodriver_send_event_down_path(chk);
   dev->queuectlr = chk->tempint1;
   dev->maxoutstanding = (chk->tempint1 == -1) ? chk->tempint2 : -1;
   if (chk->tempint1 != -1) {
      curriodriver->ctlrs[chk->tempint1].maxoutstanding = chk->tempint2;
      curriodriver->ctlrs[chk->tempint1].maxreqsize = chk->bcount;
   }
/*
   fprintf (outputfile, "Maxoutstanding: tempint1 %d, tempint2 %d, bcount %d\n", chk->tempint1, chk->tempint2, chk->bcount);
*/
   addtoextraq((event *) chk);
}


#if 0
static void print_paths_to_devices()
{
   int i,j;

   for (i = 0; i < numiodrivers; i++) {
      fprintf (outputfile, "I/O driver #%d\n", i);
      for (j = 0; j < iodrivers[i]->numdevices; j++) {
         fprintf (outputfile, "Device #%d: buspath = %x, slotpath = %x\n", j, iodrivers[i]->devices[j].buspath.value, iodrivers[i]->devices[j].slotpath.value);
      }
   }
}
#endif


#if 0
static void print_paths_to_ctlrs()
{
   int i,j;

   for (i = 0; i < numiodrivers; i++) {
      fprintf (outputfile, "I/O driver #%d\n", i);
      for (j = 0; j < iodrivers[i]->numctlrs; j++) {
         fprintf (outputfile, "Controller #%d: buspath = %x, slotpath = %x\n", j, iodrivers[i]->ctlrs[j].buspath.value, iodrivers[i]->ctlrs[j].slotpath.value);
      }
   }
}
#endif




void iodriver_setcallbacks ()
{
   ioqueue_setcallbacks();
}


void iodriver_initialize (int standalone)
{
   int numdevs;
   struct ioq * queueset[MAXDEVICES];
   int i, j;
   iodriver *curriodriver;

   i = numiodrivers;

      /* Code will be broken by multiple iodrivers */
   ASSERT1(numiodrivers == 1, "numiodrivers", numiodrivers);

   ioqueue_initialize (OVERALLQUEUE, 0);

   for (i = 0; i < numiodrivers; i++) {
      curriodriver = iodrivers[i];
      curriodriver->type = standalone;
      if (standalone != STANDALONE) {
         curriodriver->scale = 1.0;
      }
      numdevs = controller_get_numcontrollers();
      curriodriver->numctlrs = numdevs;
      curriodriver->ctlrs = (ctlr*) DISKSIM_malloc(numdevs * (sizeof(ctlr)));
      ASSERT(curriodriver->ctlrs != NULL);
      for (j=0; j < numdevs; j++) {
         ctlr *currctlr = &curriodriver->ctlrs[j];
         currctlr->ctlno = j;
         currctlr->flags = 0;
         if (controller_C700_based(j) == TRUE) {
/*
fprintf (outputfile, "This one is c700_based - %d\n", j);
*/
            currctlr->flags |= DRIVER_C700;
         }
         currctlr->buspath.value = 0;
         currctlr->slotpath.value = 0;
         iosim_get_path_to_controller(i, currctlr->ctlno, &currctlr->buspath, &currctlr->slotpath);
         currctlr->numoutstanding = 0;
         currctlr->pendio = NULL;
         currctlr->oversized = NULL;
      }

      numdevs = device_get_numdevices();
      curriodriver->numdevices = numdevs;
      curriodriver->devices = (device*) DISKSIM_malloc(numdevs * (sizeof(device)));
      ASSERT(curriodriver->devices != NULL);
      for (j = 0; j < numdevs; j++) {
         device *currdev = &curriodriver->devices[j];
         currdev->devno = j;
         currdev->busy = FALSE;
         currdev->flag = 0;
         currdev->queue = ioqueue_copy(curriodriver->queue);
         ioqueue_initialize(currdev->queue, j);
         queueset[j] = currdev->queue;
         currdev->buspath.value = 0;
         currdev->slotpath.value = 0;
         iosim_get_path_to_device(i, currdev->devno, &currdev->buspath, &currdev->slotpath);
         iodriver_set_ctl_to_device(i, currdev);
         get_device_maxoutstanding(curriodriver, currdev);
      }
      logorg_initialize(sysorgs, numsysorgs, queueset,
			drv_printlocalitystats, 
			drv_printblockingstats, 
			drv_printinterferestats, 
			drv_printstreakstats, 
			drv_printstampstats, 
			drv_printintarrstats, 
			drv_printidlestats, 
			drv_printsizestats);
/*
fprintf (outputfile, "Back from logorg_initialize\n");
*/
   }
   stat_initialize(statdeffile, statdesc_emptyqueue, &emptyqueuestats);
   stat_initialize(statdeffile, statdesc_initiatenext, &initiatenextstats);

#if 0
   print_paths_to_devices();
   print_paths_to_ctlrs();
#endif

}


void iodriver_resetstats()
{
   int i, j;
   int numdevs = device_get_numdevices();

   ioqueue_resetstats(OVERALLQUEUE);
   for (i=0; i<numiodrivers; i++) {
      for (j=0; j<numdevs; j++) {
         ioqueue_resetstats(iodrivers[i]->devices[j].queue);
      }
   }
   logorg_resetstats(sysorgs, numsysorgs);
   stat_reset(&emptyqueuestats);
   stat_reset(&initiatenextstats);
}




int disksim_iodriver_stats_loadparams(struct lp_block *b) {
  
  if (disksim->iodriver_info == NULL) {
    disksim->iodriver_info = (struct iodriver_info *)DISKSIM_malloc (sizeof(iodriver_info_t));
    bzero ((char *)disksim->iodriver_info, sizeof(iodriver_info_t));
  }
  
  
  /*     unparse_block(b, outputfile); */
  
  //#include "modules/disksim_iodriver_stats_param.c"
  lp_loadparams(0, b, &disksim_iodriver_stats_mod);

  return 1;
}





static iodriver *driver_copy(iodriver *orig);

static void add_driver(iodriver *d) {
  int c, newlen,zerocnt;

  for(c = 0; c < disksim->iodriver_info->iodrivers_len; c++) {
    if(!iodrivers[c]) {
      iodrivers[c] = d;
      return;
    }
  }

  newlen = c ? 2*c : 2;
  disksim->iodriver_info->iodrivers_len = newlen;
  iodrivers = (iodriver **)realloc(iodrivers, newlen * sizeof(iodriver *));
  zerocnt = c ? c : 2;
  bzero(&(iodrivers[c]), zerocnt * sizeof(iodriver *));

  iodrivers[c] = d;
  numiodrivers++;
}

struct iodriver *disksim_iodriver_loadparams(struct lp_block *b) {
  iodriver *result;

  if (disksim->iodriver_info == NULL) {
    disksim->iodriver_info = (struct iodriver_info *)calloc (1, sizeof(iodriver_info_t));
  }
  OVERALLQUEUE = ioqueue_createdefaultqueue ();
   
  result = (iodriver *)calloc(1, sizeof(iodriver));

  add_driver(result);



  result->name = strdup(b->name);

  //#include "modules/disksim_iodriver_param.c"
  lp_loadparams(result, b, &disksim_iodriver_mod);

  result->scale = 0.0;


  return result;
}


int load_iodriver_topo(struct lp_topospec *t, int len) {
  struct iodriver *id;

  assert(!strcmp(t->type, disksim_mods[DISKSIM_MOD_IODRIVER]->name));

  id = getiodriverbyname(t->name, 0);
  if(!id) {
    fprintf(stderr, "*** error: no such iodriver %s\n", t->name);
    return 0;
  }

  if(t->l->values_len < 1) {
    fprintf(stderr, "*** error: currently, exactly one bus must be connected to a device driver.\n");
    return 0;
  }
  else if(t->l->values_len > 1) {
    if(disksim->verbosity > 0) 
      fprintf(stderr, "*** warning: currently, only one bus is supported per device driver.  I'm ignoring the rest of the buses.\n");

  }

  if(t->l->values[0]->t != TOPOSPEC) {
    fprintf(stderr, "*** error: bad bus spec for device driver.\n");
    return 0;
  }


  if(strcmp(lp_lookup_base_type(t->l->values[0]->v.t.l[0].type, 0), 
	    disksim_mods[DISKSIM_MOD_BUS]->name)) 
    {
      fprintf(stderr, "*** error: must attach buses to device driver.\n");
      return 0;
    }

  if(!getbusbyname(t->l->values[0]->v.t.l[0].name, &id->outbus)) {
    fprintf(stderr, "*** error: no such bus %s.\n", 
	    t->l->values[0]->v.t.l[0].name);
    return 0;
  }

  id->numoutbuses = 1;


  /* ghetto! */
  bus_set_to_zero_depth(id->outbus);


  return load_bus_topo(&t->l->values[0]->v.t.l[0], 0);
}


static iodriver *driver_copy(iodriver *orig) {
  iodriver *result = (iodriver *)calloc(1, sizeof(iodriver));
  memcpy(result, orig, sizeof(iodriver));
  result->queue = ioqueue_copy(orig->queue);
  return result;
}




int iodriver_load_logorg(struct lp_block *b) {
  int c, newlen, zeroOff;
  
  struct logorg *l = 0;

  newlen = disksim->iodriver_info->sysorgs_len ? 
    disksim->iodriver_info->sysorgs_len * 2 : 2;
  zeroOff = (newlen == 2) ? 0 : (newlen / 2);


  l = disksim_logorg_loadparams(b); 
  if(!l) return 0;

  for(c = 0; c < disksim->iodriver_info->sysorgs_len; c++) {
    if(!sysorgs[c]) {
      goto foundslot;
    }
  }

  sysorgs = (logorg **)realloc(sysorgs, newlen * sizeof(struct logorg *));

  bzero(&(sysorgs[zeroOff]), 
	(zeroOff ? numsysorgs : 2) * sizeof(struct logorg *));

  disksim->iodriver_info->sysorgs_len = newlen;

 foundslot:
  sysorgs[c] = l;
  logorg_set_arraydisk(l, c);
  numsysorgs++;

  return 1;
}


void iodriver_printstats()
{
   int i;
   int j;
   struct ioq **queueset;
   int setsize = 0;
   char prefix[80];

   fprintf (outputfile, "\nOVERALL I/O SYSTEM STATISTICS\n");
   fprintf (outputfile, "-----------------------------\n\n");
   sprintf (prefix, "Overall I/O System ");
   ioqueue_printstats (&OVERALLQUEUE, 1, prefix);

   fprintf (outputfile, "\nSYSTEM-LEVEL LOGORG STATISTICS\n");
   fprintf (outputfile, "------------------------------\n\n");
   sprintf(prefix, "System ");
   logorg_printstats(sysorgs, numsysorgs, prefix);

   fprintf (outputfile, "\nIODRIVER STATISTICS\n");
   fprintf (outputfile, "-------------------\n\n");
   for (i = 0; i < numiodrivers; i++) {
      setsize += iodrivers[i]->numdevices;
   }

   queueset = (struct ioq **)DISKSIM_malloc(setsize*sizeof(struct ioq *));

   setsize = 0;
   for (i = 0; i < numiodrivers; i++) {
      for (j = 0; j < iodrivers[i]->numdevices; j++) {
         queueset[setsize] = iodrivers[i]->devices[j].queue;
         setsize++;
      }
   }
   sprintf(prefix, "IOdriver ");
   if (stat_get_count(&emptyqueuestats) > 0) {
      stat_print(&emptyqueuestats, prefix);
   }
   if (stat_get_count(&initiatenextstats) > 0) {
      stat_print(&initiatenextstats, prefix);
   }
   ioqueue_printstats(queueset, setsize, prefix);
   if ((drv_printperdiskstats == TRUE) && ((numiodrivers > 1) || (iodrivers[0]->numdevices > 1))) {
      for (i = 0; i < numiodrivers; i++) {
         for (j = 0; j < iodrivers[i]->numdevices; j++) {
            fprintf (outputfile, "\nI/O Driver #%d - Device #%d\n", i, j);
            sprintf(prefix, "IOdriver #%d device #%d ", i, j);
            ioqueue_printstats(&iodrivers[i]->devices[j].queue, 1, prefix);
         }
      }
   }

   free(queueset);
   queueset = NULL;

   simresult_saveSimResults();
}


void iodriver_cleanstats()
{
   int i;
   int j;

   ioqueue_cleanstats (OVERALLQUEUE);
   logorg_cleanstats(sysorgs, numsysorgs);
   for (i = 0; i < numiodrivers; i++) {
      for (j = 0; j < iodrivers[i]->numdevices; j++) {
         ioqueue_cleanstats(iodrivers[i]->devices[j].queue);
      }
   }
}


void iodriver_cleanup(void) {
  int i;

  for(i = 0; i < numiodrivers; i++) {
  }

  for(i = 0; i < numsysorgs; i++) {
    if(sysorgs[i]) {
      logorg_cleanup(sysorgs[i]);
    }
  }
}

// End of file


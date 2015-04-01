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
#include "disksim_bus.h"
#include "config.h"

#include "modules/modules.h"


#ifdef DEBUG_CONTROLLER

char buff[40];
char * debug_decode_controller_type( int ctlrtype )
{
    static char *ctlrTypeMsgs[ ] = { "UNKNOWN", "PASSTHRU", "53C700", "SMART" };
    int index = 0;

    switch( ctlrtype )
    {
      case CTLR_PASSTHRU:
      case CTLR_BASED_ON_53C700:
      case CTLR_SMART:
        index = ctlrtype;
        break;
    }
    sprintf( buff, "%d(%s)", ctlrtype, ctlrTypeMsgs[index] );
    return buff;
}


char * getCauseString( int cause )
{
    static char *causeMsgs[ ] = { "UNKNOWN", "COMPLETION", "RECONNECT", "DISCONNECT", "READY_TO_TRANSFER" };
    int index = 0;

    switch( cause )
    {
      case COMPLETION:
      case RECONNECT:
      case DISCONNECT:
      case READY_TO_TRANSFER:
        index = -cause;
        break;
    }
    sprintf( buff, "%d(%s)", cause, causeMsgs[index] );
    return buff;
}


void
dump_ioreq_event(ioreq_event *event, char *msg )
{
    ioreq_event *myEvent = event;

    while( NULL != myEvent )
    {
        fprintf (outputfile, "\n*** %f: dump_ioreq_event (%s): %p", simtime, msg, myEvent );
        fprintf (outputfile, "\n*** %f:    next        : %p", simtime, myEvent->next );
        fprintf (outputfile, "\n*** %f:    prev        : %p", simtime, myEvent->prev );
        fprintf (outputfile, "\n*** %f:    batch_next  : %p", simtime, myEvent->batch_next );
        fprintf (outputfile, "\n*** %f:    batch_prev  : %p", simtime, myEvent->batch_prev );
        fprintf (outputfile, "\n*** %f:    time        : %f", simtime, myEvent->time );
        fprintf (outputfile, "\n*** %f:    type        : %d", simtime, myEvent->type );
        fprintf (outputfile, "\n*** %f:    cause       : %d", simtime, myEvent->cause );
        fprintf (outputfile, "\n*** %f:    busno       : %u", simtime, myEvent->busno );
        fprintf (outputfile, "\n*** %f:    devno       : %d", simtime, myEvent->devno );
        fprintf (outputfile, "\n*** %f:    blkno       : %d", simtime, myEvent->blkno );
        fprintf (outputfile, "\n*** %f:    bcount      : %d", simtime, myEvent->bcount );
        fprintf (outputfile, "\n*** %f:    flags       : %X\n\n", simtime, myEvent->flags );
        fflush( outputfile );

        if( NULL != myEvent->next )
            myEvent = myEvent->next;
        else if( NULL != myEvent->batch_next )
            myEvent = myEvent->batch_next;
        else
            myEvent = NULL;
    }
}
#endif

/* Currently, controllers can not communicate via ownership-type buses */

INLINE controller * getctlr (int ctlrno)
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   ASSERT1((ctlrno >= 0) && (ctlrno < ctlrinfo->numcontrollers), "ctlrno", ctlrno);
   return (ctlrinfo->controllers[ctlrno]);
}

controller *getctlrbyname(char *name, int *num) {
  int c;
  for(c = 0; c < disksim->ctlrinfo->numcontrollers; c++) {
    if(!strcmp(name, disksim->ctlrinfo->controllers[c]->name)) {
      if(num) *num = c;
      return disksim->ctlrinfo->controllers[c];
    }
  }
  return 0;
}


int controller_C700_based (int ctlno)
{
   controller *currctlr = getctlr(ctlno);
   if (currctlr->type == CTLR_BASED_ON_53C700) {
      return(TRUE);
   }
   return(FALSE);
}


int controller_get_downward_busno (controller *currctlr, ioreq_event *curr, int *slotnoptr)
{
   intchar busno;
   intchar slotno;
   int depth;

#ifdef DEBUG_CONTROLLER
     fprintf (outputfile, "*** %f: controller_get_downward_busno  ctlno %d, busno %d, slotno %d, type %s (%d), cause %s, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, curr->busno, curr->slotno, getEventString( curr->type ), curr->type, getCauseString( curr->cause), curr->blkno, curr->bcount, curr->flags, curr);
     fflush( outputfile );
#endif

   busno.value = curr->busno;
   slotno.value = curr->slotno;
   depth = currctlr->depth[0];
   ASSERT1(depth != MAXDEPTH, "depth", depth);
   depth++;
   if (slotnoptr != NULL) {
      *slotnoptr = slotno.byte[depth] & 0x0F;
   }
   return(busno.byte[depth]);
}


static int controller_get_upward_busno (controller *currctlr, ioreq_event *curr, int *slotnoptr)
{
   intchar busno;
   intchar slotno;
   int depth;

#ifdef DEBUG_CONTROLLER
     fprintf (outputfile, "*** %f: controller_get_upward_busno  ctlno %d, busno %d, slotno %d, type %d, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, curr->busno, curr->slotno, curr->type, curr->blkno, curr->bcount, curr->flags, curr);
     fflush( outputfile );
#endif

   busno.value = curr->busno;
   slotno.value = curr->slotno;
   depth = currctlr->depth[0];
   if (slotnoptr != NULL) {
      *slotnoptr = slotno.byte[depth] >> 4;
   }
   if (depth != 0) {
      return(busno.byte[depth]);
   } else {
      return(-1);
   }
}


void controller_send_event_down_path (controller *currctlr, ioreq_event *curr, double delay)
{
   int busno;
   int slotno;

#ifdef DEBUG_CONTROLLER
     fprintf (outputfile, "*** %f: controller_send_event_down_path - ctlno %d, , type %d, cause %s, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, curr->type, getCauseString( curr->cause ), curr->blkno, curr->bcount, curr->flags, curr);
     fflush( outputfile );
#endif

   busno = controller_get_downward_busno(currctlr, curr, NULL);
   slotno = controller_get_outslot(currctlr->ctlno, busno);
   curr->next = currctlr->buswait;
   currctlr->buswait = curr;
   curr->tempint1 = busno;

#ifdef DEBUG_CONTROLLER
     fprintf (outputfile, "*** %f: controller_send_event_down_path - ctlno %d, busno %d, slotno %d, type %d, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, busno, slotno, curr->type, curr->blkno, curr->bcount, curr->flags, curr);
     fflush( outputfile );
#endif

   curr->time = delay * currctlr->timescale;
   if (currctlr->outbusowned == -1) {
      if (bus_ownership_get(busno, slotno, curr) == FALSE) {

/*
fprintf (outputfile, "Must get ownership of bus\n");
*/
      } else {
         bus_delay(busno, CONTROLLER, currctlr->ctlno, curr->time, curr);
         curr->time = (double) -1;
      }
   } else if (currctlr->outbusowned == busno) {
      bus_delay(busno, CONTROLLER, currctlr->ctlno, curr->time, curr);
      curr->time = (double) -1.0;
   } else {
      fprintf(stderr, "Wrong bus owned for transfer desired\n");
      exit(1);
   }
}


void controller_send_event_up_path (controller *currctlr, ioreq_event *curr, double delay)
{
   int busno;
   int slotno;

   busno = controller_get_upward_busno(currctlr, curr, NULL);
   slotno = currctlr->slotno[0];

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_send_event_up_path - ctlno %d, busno %d, slotno %d, type %d, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, busno, slotno, curr->type, curr->blkno, curr->bcount, curr->flags, curr);
   fflush( outputfile );
#endif

   curr->next = currctlr->buswait;
   currctlr->buswait = curr;
   curr->tempint1 = busno;
   curr->time = delay * currctlr->timescale;
   if (busno == -1) {
      bus_delay(busno, CONTROLLER, currctlr->ctlno, curr->time, curr);
      curr->time = (double) -1.0;
   } else if (currctlr->inbusowned == -1) {
      if (bus_ownership_get(busno, slotno, curr) == FALSE) {
      } else {
         bus_delay(busno, CONTROLLER, currctlr->ctlno, curr->time, curr);
         curr->time = (double) -1.0;
      }
   } else if (currctlr->inbusowned == busno) {
      bus_delay(busno, CONTROLLER, currctlr->ctlno, curr->time, curr);
      curr->time = (double) -1.0;
   } else {
      fprintf(stderr, "Wrong bus owned for transfer desired\n");
      exit(1);
   }
}


int controller_get_data_transfered(int ctlno, int devno)
{
   double tmptime;
   int tmpblks;
   controller *currctlr = getctlr(ctlno);
   ioreq_event *tmp = currctlr->datatransfers;

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: Entered controller_get_data_transfered - ctlno %d, devno %d\n", simtime, currctlr->ctlno, devno);
   fflush( outputfile );
#endif

   while (tmp) {
      if (tmp->devno == devno) {
         tmptime = ((ioreq_event *) tmp->tempptr1)->time;
         tmptime = (tmptime - simtime) / tmp->time;
         tmpblks = ((ioreq_event *) tmp->tempptr1)->blkno - tmp->blkno;
         tmpblks += ((ioreq_event *) tmp->tempptr1)->bcount;
         return((int) ((double) tmpblks - tmptime));
      }
      tmp = tmp->next;
   }
   return(-1);
}


static void controller_disown_busno (controller *currctlr, int busno)
{

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_disown_busno - ctlno %d, busno %d\n", simtime, currctlr->ctlno, busno );
   fflush( outputfile );
#endif

   ASSERT1((busno == currctlr->inbusowned) || (busno == currctlr->outbusowned), "busno", busno);
   if (currctlr->inbusowned == busno) {
      bus_ownership_release(currctlr->inbusowned);
      currctlr->inbusowned = -1;
   } else {
      currctlr->outbusowned = -1;
   }
}


void controller_bus_ownership_grant (int ctlno, ioreq_event *curr, int busno, double arbdelay)
{
   controller *currctlr = getctlr(ctlno);
   ioreq_event *tmp;

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_bus_ownership_grant - ctlno %d, busno %d, arbdelay %f, type %d, cause %s, blkno %d, bcount %d, flags 0x%x, addr %p\n", simtime, currctlr->ctlno, busno, arbdelay, curr->type, getCauseString( curr->cause), curr->blkno, curr->bcount, curr->flags, curr);
   fflush( outputfile );
#endif

   tmp = currctlr->buswait;
   while ((tmp) && (tmp != curr)) {
      tmp = tmp->next;
   }
       /* Bus ownership granted to unknown controller request */
   ASSERT(tmp != NULL);
   switch (tmp->type) {
      case IO_ACCESS_ARRIVE:
      case IO_INTERRUPT_COMPLETE:
         currctlr->outbusowned = busno;
         break;
      case IO_INTERRUPT_ARRIVE:
         currctlr->inbusowned = busno;
         break;
      default:
         fprintf(stderr, "Unknown event type at controller_bus_ownership_grant - %d\n", tmp->type);
         exit(1);
   }
   currctlr->waitingforbus += arbdelay;
   bus_delay(busno, CONTROLLER, currctlr->ctlno, tmp->time, tmp);
   tmp->time = (double) -1.0;
   controller_disown_busno(currctlr, busno);
   //controller_disown_busno(&controllers[ctlno], busno);
}


void controller_remove_type_from_buswait (controller *currctlr, int type)
{
   ioreq_event *tmp;
   ioreq_event *trv;
   int busno;

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_remove_type_from_buswait - ctlno %d, type %d\n", simtime, currctlr->ctlno, type );
   fflush( outputfile );
#endif

   ASSERT(currctlr->buswait != NULL);
   if (currctlr->buswait->type == type) {
      tmp = currctlr->buswait;
      currctlr->buswait = tmp->next;
   } else {
      trv = currctlr->buswait;
      while ((trv->next) && (trv->next->type != type)) {
         trv = trv->next;
      }
      ASSERT1(trv->next != NULL, "type", type);
      tmp = trv->next;
      trv->next = tmp->next;
   }
   if (tmp->time == -1.0) {
      fprintf(stderr, "In remove_type_from_buswait, the event is in transit\n");
      exit(1);
   } else {
      busno = controller_get_downward_busno(currctlr, tmp, NULL);
      bus_remove_from_arbitration(busno, tmp);
      addtoextraq((event *) tmp);
   }
}


void controller_bus_delay_complete(int ctlno, ioreq_event *curr, int busno)
{
   controller *currctlr = getctlr(ctlno);
   ioreq_event *trv;
   int slotno;
   int buswaitdir;

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_bus_delay_complete  Entering\n", simtime );
   dump_ioreq_event( curr, "controller_bus_delay_complete::curr" );
   dump_ioreq_event( currctlr->buswait, "controller_bus_delay_complete::currctlr->buswait" );
#endif

   // search for the ioreq_event in the buswait list
   if (curr == currctlr->buswait) {
      currctlr->buswait = curr->next;
   } else {
      trv = currctlr->buswait;
      while ((trv->next != NULL) && (trv->next != curr)) {
         trv = trv->next;
      }
      ASSERT(trv->next == curr);
      trv->next = curr->next;
   }
/*
fprintf (outputfile, "Controller_bus_delay_complete - ctlno %d, busno %d, blkno %d, type %d, cause %d\n", currctlr->ctlno, curr->tempint1, curr->blkno, curr->type, curr->cause);
*/
   curr->next = NULL;
   curr->time = 0.0;

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: Inside controller_bus_delay_complete: ctlno = %d, type = %d, cause = %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, ctlno, curr->type, curr->cause, curr->devno, curr->blkno, curr->bcount, curr->flags );
   fflush( outputfile );
#endif

   if (busno == controller_get_upward_busno(currctlr, curr, &slotno)) {
      buswaitdir = UP;
      if (busno == -1) {
         intr_request ((event *)curr);
      } else {
         bus_deliver_event(busno, slotno, curr);
      }
      if (currctlr->inbusowned != -1) {
         bus_ownership_release(currctlr->inbusowned);
         currctlr->inbusowned = -1;
      }
   } else if (busno == controller_get_downward_busno(currctlr, curr, &slotno)) {
      buswaitdir = DOWN;
      bus_deliver_event(busno, slotno, curr);
      if (currctlr->outbusowned != -1) {
         currctlr->outbusowned = -1;
      }
   } else {
      fprintf(stderr, "Non-matching busno for request in controller_bus_delay_complete: busno %d\n", busno);
      exit(1);
   }

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_bus_delay_complete  Exiting\n", simtime );
   dump_ioreq_event( currctlr->buswait, "controller_bus_delay_complete::currctlr->buswait" );
#endif
}


void controller_event_arrive (int ctlno, ioreq_event *curr)
{
   controller *currctlr = getctlr(ctlno);

#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_event_arrive: ctlno %d, ctlrtype %s, type %d, cause %s, devno %d, blkno %d, bcount %d, flags %x\n", simtime, ctlno, debug_decode_controller_type( currctlr->type ), curr->type, getCauseString(curr->cause), curr->devno, curr->blkno, curr->bcount, curr->flags);
   fflush( outputfile );
#endif

   switch (currctlr->type) {
      
      case CTLR_PASSTHRU:
               controller_passthru_event_arrive(currctlr, curr);
               break;
      
      case CTLR_BASED_ON_53C700:
               controller_53c700_event_arrive(currctlr, curr);
               break;
      
      case CTLR_SMART:
               controller_smart_event_arrive(currctlr, curr);
               break;

      default:
               fprintf(stderr, "Unknown controller type in controller_interrupt_arrive: %d\n", currctlr->type);
               exit(1);
   }
#ifdef DEBUG_CONTROLLER
   fprintf (outputfile, "*** %f: controller_event_arrive: Exiting\n", simtime );
   fflush( outputfile );
#endif
}


int disksim_ctlr_stats_loadparams(struct lp_block *b) {

  ctlrinfo_t *ctlrinfo;

  if(disksim->ctlrinfo == NULL) {
    disksim->ctlrinfo = (ctlrinfo_t *)calloc(1, sizeof(ctlrinfo_t));
    if(!disksim->ctlrinfo) return 0;
  }
  ctlrinfo = disksim->ctlrinfo;

  //  #include "modules/disksim_ctlr_stats_param.c"
  lp_loadparams(0, b, &disksim_ctlr_stats_mod);

  return 1;
}




controller *controller_copy(controller *orig) {
  controller *result = (controller *)calloc(1, sizeof(controller));
  if(!result) return 0;
  memcpy(result, orig, sizeof(controller));
  result->cache = orig->cache->cache_copy(orig->cache);
  result->queue = ioqueue_copy(orig->queue);

  /* don't copy these */
  result->connections = 0;
  result->datatransfers = 0;
  result->hostwaiters = 0;
  result->buswait = 0;
  result->outbuses = 0;

  return result;
}

struct controller *disksim_ctlr_loadparams(struct lp_block *b)
{
  int c;
  controller *result;
  /* temp vars for parameters */
  



  /* initialize top-level controller info struct if necessary */
  if (disksim->ctlrinfo == NULL) {
    disksim->ctlrinfo = (struct ctlrinfo *)DISKSIM_malloc (sizeof(ctlrinfo_t));
    bzero ((char *)disksim->ctlrinfo, sizeof(ctlrinfo_t));
  }

  
  /* find a free slot in the controller table for the controller
   * we're creating */
  for(c = 0; c < disksim->ctlrinfo->ctlrs_len; c++) {
    if(!disksim->ctlrinfo->controllers[c]) { goto foundslot; } 
  }

  /* didn't find a free slot -- resize the controller ptr array */
  {
    int newlen = c ? 2*c : 2;
    int zerooff = (newlen == 2) ? 0 : c;
    int zerolen = ((newlen == 2) ? 2 : (newlen / 2));
    
    if ( zerooff == 0 )
    {
      disksim->ctlrinfo->controllers = (controller **)calloc(newlen, sizeof(void*));
    }
    else
    {
      disksim->ctlrinfo->controllers = (controller **)realloc(disksim->ctlrinfo->controllers,
					       newlen * sizeof(int *));
      bzero(&(disksim->ctlrinfo->controllers[zerooff]), zerolen*sizeof(int*));
    }

    disksim->ctlrinfo->ctlrs_len = newlen;
  }

 foundslot:
  
  disksim->ctlrinfo->numcontrollers++;

  /* allocate a new controller struct */
  result = (controller *)calloc(1, sizeof(controller));
  if(!result) return 0;

  disksim->ctlrinfo->controllers[c] = result;
  bzero(result, sizeof(controller));

  result->name = strdup(b->name);

  //  #include "modules/disksim_ctlr_param.c"
  lp_loadparams(result, b, &disksim_ctlr_mod);


  /* Overhead for propogating request to lower-level component */
  result->ovrhd_disk_request = 0.5;

  /* Overhead for propogating ready-to-transfer to upper-level component */
  result->ovrhd_ready = 0.5;

  /* Overhead for propogating ready-to-transfer to lower-level component */
  result->ovrhd_disk_ready = 0.5;

  /* Overhead for propogating disconnect to upper-level component */
  result->ovrhd_disconnect = 0.5;

  /* Overhead for propogating disconnect to lower-level component */
  result->ovrhd_disk_disconnect = 0.5;

  /* Overhead for propogating reconnect to upper-level component */
  result->ovrhd_reconnect = 0.5;

  /* Overhead for propogating reconnect to lower-level component */
  result->ovrhd_disk_reconnect = 0.5;

  /* Overhead for propogating complete to upper-level component */
  result->ovrhd_complete = 0.5;

  /* Overhead for propogating complete to lower-level component */
  result->ovrhd_disk_complete = 0.5;

  /* Overhead for propogating disconnect/complete-complete to
     upper-level component */
  result->ovrhd_reset = 0.5;

  return result;
}




void controller_read_logorg (FILE *parfile)
{
  assert(0);
/*     ctlrinfo_t *ctlrinfo = disksim->ctlrinfo; */

/*     getparam_int(parfile, "\n# of controller-level organizations", &ctlrinfo->numctlorgs, 3, 0, MAXLOGORGS); */
/*     fscanf(parfile, "\n"); */

/*     if (ctlrinfo->numctlorgs > 0) { */
/*        fprintf (stderr, "Controller-level logical organizations are not currently supported\n"); */
/*        exit (0); */
/*        ctlrinfo->ctlorgs = logorg_readparams(parfile, ctlrinfo->numctlorgs, */
/*               ctlrinfo->ctl_printlocalitystats, ctlrinfo->ctl_printblockingstats, */
/*               ctlrinfo->ctl_printinterferestats, ctlrinfo->ctl_printstreakstats, */
/*               ctlrinfo->ctl_printstampstats, ctlrinfo->ctl_printintarrstats, */
/*               ctlrinfo->ctl_printidlestats, ctlrinfo->ctl_printsizestats); */
/*     } */
}


int 
controller_set_depth (int ctlno, 
		      int inbusno, 
		      int depth, 
		      int slotno)
{
   controller *currctlr = getctlr (ctlno);
   intchar ret;
   int i;
   int cnt;

/*
fprintf (outputfile, "controller_set_depth %d, inbusno %d, depth %d, slotno %d\n", ctlno, inbusno, depth, slotno);
*/

   cnt = currctlr->numinbuses;
   if ((cnt > 0) && (currctlr->depth[(cnt-1)] < depth)) {
      return 0;
   }
   currctlr->numinbuses++;

   if ((cnt + 1) > MAXINBUSES) {
      fprintf(stderr, 
	      "*** Too many inbuses specified for controller %d : %d (check BUG #30)\n", 
	      ctlno, (cnt+1));
      ddbg_assert(0);
   }

   currctlr->inbuses[cnt] = inbusno;
   currctlr->depth[cnt] = depth;
   currctlr->slotno[cnt] = slotno;
   ret.value = 0;

   for (i = 0; i < currctlr->numoutbuses; i++) {
      ret.byte[i] = (char) currctlr->outbuses[i];
      currctlr->outslot[i] = bus_get_controller_slot(ret.byte[i], ctlno);
   }

   return ret.value;
}


int controller_get_bus_master (int busno)
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   int i;
   int j;

   for (i = 0; i < ctlrinfo->numcontrollers; i++) {
      controller *currctlr = getctlr(i);
      for (j = 0; j < currctlr->numoutbuses; j++) {
         if (currctlr->outbuses[j] == busno) {
            return(i);
         }
      }
   }
   return(-1);
}


int controller_get_outslot (int ctlno, int busno)
{
   int i;
   int slotno = 0;
   controller *currctlr = getctlr(ctlno);

   for (i = 0; i < currctlr->numoutbuses; i++) {
      if (currctlr->outbuses[i] == busno) {
         slotno = currctlr->outslot[i];
         break;
      }
   }
/*     ASSERT(slotno != 0); */
   return(slotno);
}


int controller_get_numcontrollers ()
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   return (ctlrinfo->numcontrollers);
}


int controller_get_depth (int ctlno)
{
   controller *currctlr = getctlr(ctlno);
   return (currctlr->depth[0]);
}


int controller_get_inbus (int ctlno)
{
   controller *currctlr = getctlr(ctlno);
   return (currctlr->inbuses[0]);
}


int controller_get_slotno (int ctlno)
{
   controller *currctlr = getctlr(ctlno);
   return (currctlr->slotno[0]);
}


int controller_get_maxoutstanding (int ctlno)
{
   controller *currctlr = getctlr(ctlno);
   return (currctlr->maxoutstanding);
}


void controller_setcallbacks ()
{
   controller_smart_setcallbacks ();
}


void controller_initialize()
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   int i;

   for (i = 0; i < ctlrinfo->numcontrollers; i++) {
      controller *currctlr = getctlr(i);
      currctlr->ctlno = i;
      currctlr->state = FREE;
      addlisttoextraq((event **) &currctlr->connections);
      addlisttoextraq((event **) &currctlr->buswait);
      addlisttoextraq((event **) &currctlr->datatransfers);
      addlisttoextraq((event **) &currctlr->hostwaiters);
      currctlr->hosttransfer = FALSE;
      currctlr->inbusowned = -1;
      currctlr->outbusowned = -1;
      currctlr->waitingforbus = 0.0;
      if (currctlr->type == CTLR_SMART) {
         controller_smart_initialize(currctlr);
      }
   }
}


void controller_resetstats()
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   int i;

   for (i=0; i<ctlrinfo->numcontrollers; i++) {
      controller *currctlr = getctlr(i);
      currctlr->waitingforbus = 0.0;
      if (currctlr->type == CTLR_SMART) {
         controller_smart_resetstats(currctlr);
      }
   }
}


void controller_printstats()
{
   ctlrinfo_t *ctlrinfo = disksim->ctlrinfo;
   int ctlno;
   char prefix[81];
   double waitingforbus = 0.0;

   fprintf (outputfile, "\nCONTROLLER STATISTICS\n");
   fprintf (outputfile, "---------------------\n\n");

   for (ctlno=0; ctlno < ctlrinfo->numcontrollers; ctlno++) {
      controller *currctlr = getctlr(ctlno);
      waitingforbus += currctlr->waitingforbus;
      if (currctlr->printstats == 0) {
         continue;
      }
      sprintf(prefix, "Controller #%d ", ctlno);
      fprintf (outputfile, "%s\n\n", prefix);
      switch(currctlr->type) {

         case CTLR_PASSTHRU:
            controller_passthru_printstats(currctlr, prefix);
            break;

         case CTLR_BASED_ON_53C700:
            controller_53c700_printstats(currctlr, prefix);
            break;

         case CTLR_SMART:
            controller_smart_printstats(currctlr, prefix);
            break;

         default:
            fprintf(stderr, "Unknown controller type in controller_printstats: %d\n", currctlr->type);
            exit(1);
       }
   }
   fprintf (outputfile, "Total controller bus wait time: %f\n", waitingforbus);
}


void controller_cleanstats()
{
}


int load_ctlr_topo(struct lp_topospec *t, int *inbus) {
  int c;
  controller *ctlr;
  int num;
  struct lp_topospec *ts;
  int slots;
  assert(!strcmp(t->type, disksim_mods[DISKSIM_MOD_CTLR]->name));

  ctlr = getctlrbyname(t->name, &num);
  if(!ctlr) {
    fprintf(stderr, "*** error: no such controller %s\n", t->name);
    return 0;
  }


  if(inbus) {
    ctlr->inbuses[0] = *inbus;
  }

  if(!ctlr->outbuses) {
    ctlr->numoutbuses = 0;
    slots = t->l->values_len;
    ctlr->outbuses = (int *)calloc(1, slots * sizeof(int));
  }

  for(c = 0; c < t->l->values_len; c++) {
    if(!t->l->values[c]) continue;
    assert(t->l->values[c]->v.t.len == 1);
    ts = &t->l->values[c]->v.t.l[0];
    if(!strcmp(ts->type, disksim_mods[DISKSIM_MOD_BUS]->name)) {
      getbusbyname(ts->name, &ctlr->outbuses[c]);

      if(ctlr->outbuses[c] == -1) {
	fprintf(stderr, "*** error: No such bus: %s\n", ts->name);
	return 0;
      }

      if(!load_bus_topo(ts, &num)) {
	fprintf(stderr, "*** error: failed to load bus topology spec for controller %s.\n", t->name);
	return 0;
      }

      ctlr->numoutbuses++;
    }
    else {
      fprintf(stderr, "*** error: can only attach buses to controllers %s\n", t->name);
      return 0;
    }
  }

  return 1;
}


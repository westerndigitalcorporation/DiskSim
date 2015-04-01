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
#include "disksim_stat.h"
#include "disksim_bus.h"
#include "disksim_ctlr.h"
#include "disksim_controller.h"
#include "config.h"

#include "modules/modules.h"
//#include "memsmodel/modules/modules.h"

#include "disksim_device.h"

/* Bus types */

#define BUS_TYPE_MIN	1
#define EXCLUSIVE	1
#define INTERLEAVED	2
#define BUS_TYPE_MAX	2

/* Bus arbitration types */

#define BUS_ARB_TYPE_MIN	1
#define SLOTPRIORITY_ARB	1
#define FIFO_ARB		2
#define BUS_ARB_TYPE_MAX	2

/* Bus states */

#define BUS_FREE	1
#define BUS_OWNED	2


/* INLINE */ static struct bus * getbus (int busno)
{
  /*     ASSERT1((busno >= 0) && (busno < numbuses), "busno",busno); */
  if((busno < 0) || (busno >= numbuses)) { 
    return 0;
  } 
  else {
    return disksim->businfo->buses[busno];
  }
}

struct bus *getbusbyname(char *name, int *num) {
  int c;
  for(c = 0; c < numbuses; c++) {
    if(disksim->businfo->buses[c])
      if(!strcmp(disksim->businfo->buses[c]->name, name)) {
	if(num) *num = c;
	return disksim->businfo->buses[c];
      }
  }

  return 0;
}



void bus_set_to_zero_depth (int busno)
{
   struct bus *currbus = getbus(busno);
   currbus->depth = 0;
}


int 
bus_get_controller_slot (int busno, int ctlno)
{
   int i;
   struct bus *currbus = getbus (busno);

   for (i = 0; i < currbus->numslots; i++) {
     if ((currbus->slots[i].devno == ctlno) && (currbus->slots[i].devtype == CONTROLLER)) {
       /*  	slotno = i; */
       /*  	break; */
       return i;
     }
   }

   fprintf(stderr, "Controller not connected to bus but called bus_get_controller_slot: %d %d\n", ctlno, busno);
   ddbg_assert(0);
   return 0;
}


void bus_set_depths()
{
   int depth;
   int busno;
   int slotno;
   intchar ret;
   int i;
   u_int devno;
   
   for (depth = 0; depth < MAXDEPTH; depth++) {
     /* fprintf (outputfile, "At depth %d\n", depth); */

     for (busno = 0; busno < numbuses; busno++) {
       struct bus *currbus = getbus(busno);
       /* fprintf (outputfile, "At busno %d\n", busno); */

       if (currbus->depth == depth) {
	 /* fprintf (outputfile, "Busno %d is at this depth - %d\n",
	    busno, depth); */

	 for (slotno = 0; slotno < currbus->numslots; slotno++) {
	   /* fprintf (outputfile, "Deal with slotno %d of busno
	      %d\n", slotno, busno); */

	   devno = currbus->slots[slotno].devno;

	   switch (currbus->slots[slotno].devtype) {
	   case CONTROLLER:
	     /* fprintf (outputfile, "Slotno %d contains controller
		number %d\n", slotno, devno); */
	     ret.value = controller_set_depth(devno, busno, depth, slotno);
	     break;

	   case DEVICE:
	     /* fprintf (outputfile, "Slotno %d contains disk number
		%d\n", slotno, devno); */
	     ret.value = device_set_depth(devno, busno, depth, slotno);
	     break;

	   default:         
	     fprintf(stderr, "*** Invalid device type in bus slot\n");
	     ddbg_assert(0);
	     break;
	   }

	   /* fprintf (outputfile, "Back from setting device depth\n"); */
	   if (ret.value != 0) {
	     /* fprintf (outputfile, "Non-zero return - %x\n", ret.value); */
	     for (i = 0; i < MAXDEPTH; i++) {
	       if (ret.byte[i] != 0) {
		 struct bus *currbus2 = getbus(ret.byte[i]);
		 /* fprintf (outputfile, "Set busno %x to have depth
		    %d\n", ret.byte[i], (depth+1));*/
		 currbus2->depth = depth + 1;
	       }
	     }
	   }
	 }
       }
     }
   }
}


double bus_get_transfer_time(int busno, int bcount, int read)
{
   double blktranstime;
   struct bus *currbus = getbus(busno);

   blktranstime = (read) ? currbus->readblktranstime : currbus->writeblktranstime;
   return((double) bcount * blktranstime);
}


/*
 * bus_delay
 * Adds a BUS_DELAY_COMPLETE event to the intq.
 */

void bus_delay(int busno, int devtype, int devno, double delay, ioreq_event *curr)
{
   bus_event *tmp = (bus_event *) getfromextraq();

#ifdef DEBUG_BUS
    fprintf( outputfile, "*** %f: bus_delay  adds this event to delayed_event  type %d, devno %d, blkno %d, bcount %d, flags 0x%x, delay %f msec\n", simtime, curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags, delay );
#endif

   tmp->type = BUS_DELAY_COMPLETE;
   tmp->time = simtime + delay;
   tmp->devno = devno;
   tmp->busno = busno;
   tmp->devtype = devtype;
   tmp->delayed_event = curr;
   addtointq((event *) tmp);
}


void bus_event_arrive(ioreq_event *ptr)
{
   bus_event *curr = (bus_event *) ptr;

#ifdef DEBUG_BUS
   fprintf (outputfile, "*** %f: Entered bus_event_arrive: type = %d, busno = %d, slotno = %d, devtype = %d, devno = %d\n", simtime, curr->type, curr->busno, curr->slotno, curr->devtype, curr->devno );
   if( NULL != curr->delayed_event )
   {
        dumpIOReq( "Entered bus_event_arrive: ioreq_event  ", curr->delayed_event );
   }
#endif

   if (curr->type == BUS_OWNERSHIP_GRANTED) {
      struct bus *currbus = getbus(curr->busno);
      ASSERT(curr == currbus->arbwinner);
      currbus->arbwinner = NULL;
      stat_update (&currbus->arbwaitstats, (simtime - curr->wait_start));
   }

   switch (curr->devtype) {

      case CONTROLLER:
         if (curr->type == BUS_DELAY_COMPLETE) {
	    controller_bus_delay_complete (curr->devno, curr->delayed_event, curr->busno);
         } else {
	    controller_bus_ownership_grant (curr->devno, curr->delayed_event, curr->busno, (simtime - curr->wait_start));
         }
	 break;

      case DEVICE:
         if (curr->type == BUS_DELAY_COMPLETE) {
	    device_bus_delay_complete (curr->devno, curr->delayed_event, curr->busno);
         } else {
	    device_bus_ownership_grant (curr->devno, curr->delayed_event, curr->busno, (simtime - curr->wait_start));
         }
	 break;

      default:
	 fprintf(stderr, "Unknown device type at bus_event_arrive: %d, type %d\n", curr->devtype, curr->type);
	 exit(1);
   }
   addtoextraq((event *)curr);  // deallocate event object to extraq
}


static bus_event * bus_fifo_arbitration(bus *currbus)
{
   bus_event *ret = currbus->owners;
   ASSERT(currbus->owners != NULL);
   currbus->owners = ret->next;
   return(ret);
}


static bus_event * bus_slotpriority_arbitration(bus *currbus)
{
   bus_event *ret = currbus->owners;

   ASSERT(currbus->owners != NULL);
   if (currbus->owners->next == NULL) {
      currbus->owners = NULL;
      return(ret);
   } else {
      bus_event *trv = ret;
      while (trv->next) {
         if (trv->next->slotno > ret->slotno) {
            ret = trv->next;
         }
         trv = trv->next;
      }
      if (ret == currbus->owners) {
         currbus->owners = ret->next;
         ret->next = NULL;
      } else {
	 trv = currbus->owners;
         while ((trv->next != NULL) && (trv->next != ret)) {
            trv = trv->next;
         }
	 ASSERT(trv->next == ret);
         trv->next = ret->next;
         ret->next = NULL;
      }
   }
   return(ret);
}


static bus_event * bus_arbitrate_for_ownership(bus *currbus)
{
   switch (currbus->arbtype) {

      case SLOTPRIORITY_ARB:
          return(bus_slotpriority_arbitration(currbus));

      case FIFO_ARB:
          return(bus_fifo_arbitration(currbus));

      default:
          fprintf(stderr, "Unrecognized bus arbitration type in bus_arbitrate_for_ownership\n");
          exit(1);
   }
}


/*
 * int bus_ownership_get(int busno, int slotno, ioreq_event *curr)
 *
 * Returns TRUE if the bus is immediately acquired, which only happens
 * for an interleaved bus.  In this case, this function does nothing
 * else.
 *
 * Returns FALSE if there is a delay (always true with SCSI). A
 * bus_event of type BUS_OWNERSHIP_GRANTED for this request is created.
 * If the bus is initially BUS_BUSY, the event is added to the
 * buses.owners queue If the bus is initially BUS_FREE, its state is
 * changed to BUS_BUSY and is granted to this request.  (I think:
 * buses.arbwinner is set to this event).  The event is scheduled on
 * the intq for the current time plus buses.arbtime.
 *
 * Note that I don't think this really how SCSI works: If a higher
 * priority device arbitrates for the bus within the arbitration time,
 * it will win.  Maybe this window is small enough to be ignored for
 * this level of simulation.
 *
 */

int bus_ownership_get(int busno, int slotno, ioreq_event *curr)
{
   bus_event *tmp;
   struct bus *currbus = getbus (busno);

   if (currbus->type == INTERLEAVED) {
      return(TRUE);
   }
   tmp = (bus_event *) getfromextraq();
   tmp->type = BUS_OWNERSHIP_GRANTED;
   tmp->devno = currbus->slots[slotno].devno;
   tmp->busno = busno;
   tmp->delayed_event = curr;
   tmp->wait_start = simtime;
   if (currbus->state == BUS_FREE) {
      tmp->devtype = currbus->slots[slotno].devtype;
      tmp->time = simtime + currbus->arbtime;
/*
fprintf (outputfile, "Granting ownership immediately - devno %d, devtype %d, busno %d\n", tmp->devno, tmp->devtype, tmp->busno);
*/
      currbus->arbwinner = tmp;
      addtointq((event *) tmp);
      currbus->state = BUS_OWNED;
      currbus->runidletime += simtime - currbus->lastowned;
      stat_update (&currbus->busidlestats, (simtime - currbus->lastowned));
   } else {
      tmp->slotno = slotno;
/*
fprintf (outputfile, "Must wait for bus to become free - devno %d, slotno %d, busno %d\n", tmp->devno, tmp->slotno, tmp->busno);
*/
      tmp->next = NULL;
      if (currbus->owners) {
         bus_event *tail = currbus->owners;
         while (tail->next) {
            tail = tail->next;
         }
         tail->next = tmp;
      } else {
         currbus->owners = tmp;
      }
   }
   return(FALSE);
}


void bus_ownership_release(int busno)
{
   bus_event *tmp;
   struct bus *currbus = getbus(busno);

#ifdef DEBUG_BUS
   fprintf (outputfile, "%f: bus_ownership_release  busno %d\n", simtime, busno );
#endif

/*
fprintf (outputfile, "Bus ownership being released - %d\n", busno);
*/
   ASSERT(currbus->arbwinner == NULL);
   if (currbus->owners == NULL) {
/*
fprintf (outputfile, "Bus has become free - %d\n", busno);
*/
      currbus->state = BUS_FREE;
      currbus->lastowned = simtime;
   } else {
      tmp = bus_arbitrate_for_ownership(currbus);
      tmp->devtype = currbus->slots[tmp->slotno].devtype;
      tmp->time = simtime + currbus->arbtime;
/*
fprintf (outputfile, "Bus ownership transfered - devno %d, devtype %d, busno %d\n", tmp->devno, tmp->devtype, tmp->busno);
*/
      currbus->arbwinner = tmp;
      addtointq((event *) tmp);
   }
}


void bus_remove_from_arbitration(int busno, ioreq_event *curr)
{
   bus_event *tmp;
   bus_event *trv;
   struct bus *currbus = getbus(busno);

#ifdef DEBUG_BUS
   fprintf (outputfile, "%f: bus_remove_from_arbitration  busno %d\n", simtime, busno );
   dumpIOReq( "bus_deliver_event", curr );
#endif

   if (currbus->arbwinner) {
      if (curr == currbus->arbwinner->delayed_event) {
         if (removefromintq((event *)currbus->arbwinner) != TRUE) {
            fprintf(stderr, "Ongoing arbwinner not in internal queue, at bus_remove_from_arbitration\n");
            exit(1);
         }
         addtoextraq((event *) currbus->arbwinner);
         currbus->arbwinner = NULL;
         bus_ownership_release(busno);
         return;
      }
   }
   if (curr == currbus->owners->delayed_event) {
      tmp = currbus->owners;
      currbus->owners = tmp->next;
   } else {
      trv = currbus->owners;
      while ((trv->next) && (curr != trv->next->delayed_event)) {
         trv = trv->next;
      }
      ASSERT(trv->next != NULL);
      tmp = trv->next;
      trv->next = tmp->next;
   }
   addtoextraq((event *) tmp);
}


void bus_deliver_event(int busno, int slotno, ioreq_event *curr)
{
   int devno, devtype;
   struct bus *currbus = getbus(busno);

   ASSERT2((slotno >= 0) && (slotno < currbus->numslots), 
	   "slotno", slotno, "busno", busno);

   devno   = currbus->slots[slotno].devno;
   devtype = currbus->slots[slotno].devtype;

#ifdef DEBUG_BUS
   fprintf (outputfile, "%f: bus_deliver_event  busno %d, slotno %d, busdevno %d, busdevtype %d\n", simtime, busno, slotno, devno, devtype );
   dumpIOReq( "bus_deliver_event", curr );
#endif

   switch (devtype) {

   case CONTROLLER:  
     controller_event_arrive(devno, curr);
     break;

   case DEVICE:      
     ASSERT(devno == curr->devno);
     device_event_arrive(curr);
     break;

   default:          
     fprintf(stderr, "Invalid device type in bus slot\n");
     assert(0);
     break;
   }
}


int bus_get_data_transfered(ioreq_event *curr, int depth)
{
   intchar slotno;
   intchar busno;
   int checkdevno;
   int ret;
/*
fprintf (outputfile, "Entered bus_get_data_transfered - devno %d, depth %d, busno %x, slotno %x\n", curr->devno, depth, curr->busno, curr->slotno);
*/
   busno.value = curr->busno;
   slotno.value = curr->slotno;
   while (depth) {
      struct bus *currbus = getbus(busno.byte[depth]);
      checkdevno = currbus->slots[(slotno.byte[depth] >> 4)].devno;
      switch (currbus->slots[(slotno.byte[depth] >> 4)].devtype) {
   
         case CONTROLLER:       ret = controller_get_data_transfered(checkdevno, curr->devno);
                                break;
   
         default:       fprintf(stderr, "Invalid device type in bus_get_data_transfered\n");
                        exit(1);
      }
      if (ret != -1) {
         return(ret);
      }
      depth--;
   }
   return(-1);
}


/* temporary global variables */
static int disksim_bus_printidlestats;
static int disksim_bus_printarbwaitstats;



int disksim_bus_stats_loadparams(struct lp_block *b) {
   
  if(!disksim->businfo) {
    disksim->businfo = (struct businfo *)calloc(1, sizeof(businfo_t));
  }


/*    unparse_block(b, outputfile); */

  //#include "modules/disksim_bus_stats_param.c"

  lp_loadparams(0, b, &disksim_bus_stats_mod);

  return 1;
}





void bus_resetstats()
{
   int i;

   for (i=0; i<numbuses; i++) {
      struct bus *currbus = getbus(i);
      currbus->lastowned = simtime;
      currbus->runidletime = 0.0;
      stat_reset (&currbus->busidlestats);
      stat_reset (&currbus->arbwaitstats);
   }
}


void bus_setcallbacks ()
{
}


void bus_initialize()
{
   int i;

   StaticAssert (sizeof(bus_event) <= DISKSIM_EVENT_SIZE);
   for (i=0; i<numbuses; i++) {
      struct bus *currbus = getbus(i);
      currbus->state = BUS_FREE;
      addlisttoextraq((event **) &currbus->owners);
      stat_initialize (statdeffile, "Arbitration wait time", &currbus->arbwaitstats);
      stat_initialize (statdeffile, "Bus idle period length", &currbus->busidlestats);
   }
   bus_resetstats();
}


void bus_printstats()
{
   int i;
   char prefix[81];

   fprintf (outputfile, "\nBUS STATISTICS\n");
   fprintf (outputfile, "--------------\n\n");

   for (i=0; i < disksim->businfo->buses_len; i++) {
     struct bus *currbus = getbus(i); if(!currbus) continue;
      if (currbus->printstats) {
         sprintf (prefix, "Bus #%d (%s) ", i, currbus->name);
         fprintf (outputfile, "Bus #%d\n", i);
         fprintf (outputfile, "Bus #%d Total utilization time: \t%.2f   \t%6.5f\n", i, (simtime - WARMUPTIME - currbus->runidletime), ((simtime - WARMUPTIME - currbus->runidletime) / (simtime - WARMUPTIME)));
         if (disksim->businfo->bus_printidlestats) {
            stat_print (&currbus->busidlestats, prefix);
         }
         if (disksim->businfo->bus_printarbwaitstats) {
            fprintf (outputfile, "Bus #%d Number of arbitrations: \t%d\n", i, stat_get_count (&currbus->arbwaitstats));
            stat_print (&currbus->arbwaitstats, prefix);
         }
         fprintf (outputfile, "\n");
      }
   }
}


void bus_cleanstats()
{
   int i;

   for (i=0; i<numbuses; i++) {
      struct bus *currbus = getbus(i);
      if (currbus->state == BUS_FREE) {
         currbus->runidletime += simtime - currbus->lastowned;
         stat_update (&currbus->busidlestats, (simtime - currbus->lastowned));
      }
   }
}

bus *bus_copy(bus *orig) {
  bus *result = (bus *)malloc(sizeof(bus));
  if(result) return (bus *)memcpy(result, orig, sizeof(bus));
  else return (bus *)0;
}

/* bus constructor using new parser.  Returns null on failure
 * or a pointer to an allocated/initialized bus struct on success 
 * if parent is non-null, it will be copied and the new structure
 * initialized from it, i.e. you only need to provide parameters in 
 * b that you want to override from parent
 */
bus *disksim_bus_loadparams(struct lp_block *b, 
		    int *num)
{
  struct bus *result;
  int c;

  if(!disksim->businfo) {
    disksim->businfo = (struct businfo *)calloc(1, sizeof(businfo_t));
  }

  //   disksim->businfo->bus_printidlestats = disksim_bus_printidlestats;
  //   disksim->businfo->bus_printarbwaitstats = disksim_bus_printarbwaitstats;


   for(c = 0; c < numbuses; c++) {
     if(!disksim->businfo->buses[c]) { break; } 
   }
   /* didn't find a free slot */
   if(c == disksim->businfo->buses_len) {
     int newlen = c ? 2*c : 2;
     
     disksim->businfo->buses = (bus **)realloc(disksim->businfo->buses,
				       newlen * sizeof(void*));
     if(newlen > 2)
       bzero(&(disksim->businfo->buses[c]), (newlen/2)*sizeof(void*));
     else 
       bzero(disksim->businfo->buses, newlen * sizeof(void*));

     disksim->businfo->buses_len = newlen;

   }
   result = (bus *)malloc(sizeof(struct bus));
   if(!result) { return 0; }
   numbuses++;
   
   disksim->businfo->buses[c] = result;
   bzero(result, sizeof(struct bus));
   if(num) *num = c;

   result->name = strdup(b->name);


   //#include "modules/disksim_bus_param.c"
  lp_loadparams(result, b, &disksim_bus_mod);

   result->owners = NULL;
   result->depth = -1;
   
   return result;
}



int load_bus_topo(struct lp_topospec *t, int *parentctlno) {
  int c, d;
  struct bus *b;
  struct lp_topospec *ts = 0;
  int busno;
  int slotnum = 0;
  int type;

  assert(!strcmp(t->type, disksim_mods[DISKSIM_MOD_BUS]->name));

  /* lookup bus */
  assert(b = getbusbyname(t->name, &busno));

  b->numslots = 0;
  for(c = 0; c < t->l->values_len; c++)
    if(!t->l->values[c]) continue;
  /*      else if(t->l->values[c]->t != TOPOSPEC) b->numslots++; */
    else for(d = 0; d < t->l->values[c]->v.t.len; d++)
      if(t->l->values[c]->v.t.l[d].name || t->l->values[c]->v.t.l[d].l) b->numslots++;

  /* if this bus is the child of a controller */
  if(parentctlno) { b->numslots++; }

  b->slots = (slot *)calloc(b->numslots, sizeof(slot));
  //  bzero(b->slots, b->numslots * sizeof(slot));

  if(parentctlno) {
    b->slots[0].devtype = CONTROLLER;
    b->slots[0].devno = *parentctlno;
    slotnum = 1;
  }

  for(c = 0; c < t->l->values_len; c++) {
    if(!t->l->values[c]) continue;
    /* t->l->values[c] should be a topospec */
    if(t->l->values[c]->t != TOPOSPEC) {
      fprintf(stderr, "*** error loading topology spec -- bad child device spec.\n");
      return 0;
    }

    for(d = 0; d < t->l->values[c]->v.t.len; d++) {
      ts = &t->l->values[c]->v.t.l[d];
      if(!ts->type) continue;
    }
 
    type = lp_mod_name(ts->type);
    if(type == lp_mod_name("disksim_ctlr")) {
      b->slots[slotnum].devtype = CONTROLLER;
	
      if(!getctlrbyname(ts->name, &b->slots[slotnum].devno)) {
	fprintf(stderr, "*** error: failed to load controller \"%s\" into slot %d of bus %s: no such controller \"%s\".\n", ts->name, c, t->name, ts->name);
	return 0;
      } 
      	
      if(!load_ctlr_topo(ts, &busno)) {
	fprintf(stderr, "*** error: failed to load controller topology spec for bus %s slot %d\n", t->name, c);
	return 0;
      }
    }
    else if ((type == lp_mod_name("disksim_disk"))
	||
	(type == lp_mod_name("disksim_simpledisk"))
	||
	(type == lp_mod_name("memsmodel_mems"))
	|| /*SSD:*/
        (type == lp_mod_name("ssdmodel_ssd"))
	)
      {
	b->slots[slotnum].devtype = DEVICE;
	
	if(!getdevbyname(ts->name, &b->slots[slotnum].devno, 0, 0)) {
	  fprintf(stderr, "*** error: failed to load device \"%s\" into slot %d of bus %s: no such device \"%s\".\n",
		  ts->name, c, t->name, ts->name);
	  return 0;
	}
      }
    else {
      fprintf(stderr, "Error loading topology spec -- slot must contain a device or a controller (not %s).\n", ts->type);
      return 0;
      break;
    }
    /* eventually want to add pointer to device here to avoid table
     * lookups elsewhere */
      
    slotnum++;
      
  }
  
  return 1;
  
}

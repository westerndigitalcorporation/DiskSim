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


#include "disksim_iosim.h"

#include "modules/modules.h"

char * iosim_decodeInterruptEvent( ioreq_event *ioReqEvent )
{
    char * reasons[] = { "READY_TO_TRANSFER", "DISCONNECT", "RECONNECT", "COMPLETION" };

    char * reason = "";
    if( ioReqEvent->cause < 0 )
    {
    	reason = reasons[ioReqEvent->cause + 4];
    }

    return reason;
}

void iosim_initialize_iosim_info (void)
{
   disksim->iosim_info = (struct iosim_info *)DISKSIM_malloc (sizeof(struct iosim_info));
   bzero ((char *)disksim->iosim_info, sizeof(struct iosim_info));

   /* initializations that get remapped into iosim_info */
   ioscale = 1.0;
   last_request_arrive = 0.0;
   constintarrtime = 0.0;
}


ioreq_event * ioreq_copy (ioreq_event *old)
{
   ioreq_event *new_event = (ioreq_event *) getfromextraq();
   memmove ((char *)new_event, (char *)old, sizeof(ioreq_event));
   /* bcopy ((char *)old, (char *)new_event, sizeof (ioreq_event)); */
   return(new_event);
}


int ioreq_compare (ioreq_event *first, ioreq_event *second)
{
   int diff = 0;
   diff |= first->time != second->time;
   diff |= first->type != second->type;
   //diff |= first->next != second->next;
   //diff |= first->prev != second->prev;
   diff |= first->bcount != second->bcount;
   diff |= first->blkno != second->blkno;
   diff |= first->flags != second->flags;
   diff |= first->busno != second->busno;
   diff |= first->slotno != second->slotno;
   diff |= first->devno != second->devno;
   diff |= first->opid != second->opid;
   diff |= first->buf != second->buf;
   diff |= first->cause != second->cause;
   diff |= first->tempint1 != second->tempint1;
   diff |= first->tempint2 != second->tempint2;
   diff |= first->tempptr1 != second->tempptr1;
   diff |= first->tempptr2 != second->tempptr2;
   return(diff);
}


void io_catch_stray_events (ioreq_event *curr)
{
   switch (curr->type) {
   case DEVICE_DATA_TRANSFER_COMPLETE:
     curr->time = device_get_blktranstime(curr);
     curr->time = simtime + (curr->time * (double) curr->bcount);
     addtointq((event *) curr);
     break;
     
   default:
     fprintf(stderr, "Unknown event type at io_catch_stray_events\n");
     exit(1);
   }
}


event * io_request (ioreq_event *curr)
{
   curr->type = IO_REQUEST_ARRIVE;
   return (iodriver_request(0, curr));
}


void io_schedule (ioreq_event *curr)
{
   curr->type = IO_ACCESS_ARRIVE;
   iodriver_schedule(0, curr);
}


double io_tick()
{
   return (iodriver_tick());
}


double io_raise_priority (int opid, int devno, int blkno, void *chan)
{
   return (iodriver_raise_priority(0, opid, devno, blkno, chan));
}


void io_interrupt_arrive (ioreq_event *intrp)
{
   intrp->type = IO_INTERRUPT_ARRIVE;
   iodriver_interrupt_arrive (0, (intr_event *)intrp);
}


void io_interrupt_complete (ioreq_event *intrp)
{
   intrp->type = IO_INTERRUPT_COMPLETE;
   iodriver_interrupt_complete (0, (intr_event *)intrp);
}


void io_internal_event(ioreq_event *curr)
{
   ASSERT(curr != NULL);

#ifdef DEBUG_IOSIM
   fprintf(outputfile, "%f: io_internal_event, time %f, type %s (%d), ", simtime, curr->time, getEventString(curr->type), curr->type );
   fprintf(outputfile, "busno %d, slotno %d, devno %d, blkno %d, bcount %d, %s(%x)\n", curr->busno, curr->slotno, curr->devno, curr->blkno, curr->bcount, (curr->flags & 1) ? "read" : "write", curr->flags );
   fflush(outputfile);

/*
fprintf (outputfile, "%f: io_internal_event entered with event type %d, %f\n", curr->time, curr->type, simtime);
*/
#endif

   switch (curr->type) {
   case IO_REQUEST_ARRIVE:
     iodriver_request(0, curr);
     break;
     
   case IO_ACCESS_ARRIVE:
     iodriver_schedule(0, curr);
     break;
     
   case IO_INTERRUPT_ARRIVE:
     iodriver_interrupt_arrive(0, (intr_event *) curr);
     break;

   case IO_RESPOND_TO_DEVICE:
     iodriver_respond_to_device(0, (intr_event *) curr->tempptr1);
     addtoextraq((event *) curr);
     break;
     
   case IO_ACCESS_COMPLETE:
     iodriver_access_complete(0, (intr_event *) curr->tempptr1);
     addtoextraq((event *) curr);
     break;
     
   case IO_INTERRUPT_COMPLETE:
     iodriver_interrupt_complete(0, (intr_event *) curr);
     break;

   case DEVICE_OVERHEAD_COMPLETE:
   case DEVICE_ACCESS_COMPLETE:
   case DEVICE_DATA_TRANSFER_COMPLETE:
   case DEVICE_PREPARE_FOR_DATA_TRANSFER:
   case DEVICE_BUFFER_SEEKDONE:
   case DEVICE_BUFFER_TRACKACC_DONE:
   case DEVICE_BUFFER_SECTOR_DONE:
   case DEVICE_GOT_REMAPPED_SECTOR:
   case DEVICE_GOTO_REMAPPED_SECTOR:
   case MEMS_SLED_SCHEDULE:
   case MEMS_SLED_SEEK:
   case MEMS_SLED_SERVO:
   case MEMS_SLED_DATA:
   case MEMS_SLED_UPDATE:
   case MEMS_BUS_INITIATE:
   case MEMS_BUS_TRANSFER:
   case MEMS_BUS_UPDATE:
   case SSD_CLEAN_ELEMENT:
   case SSD_CLEAN_GANG:
     device_event_arrive(curr);
     break;

   case BUS_OWNERSHIP_GRANTED:
   case BUS_DELAY_COMPLETE:
     bus_event_arrive(curr);
     break;

   case CONTROLLER_DATA_TRANSFER_COMPLETE:
     controller_event_arrive(curr->tempint2, curr);
     break;

   case TIMESTAMP_LOGORG:
     logorg_timestamp(curr);
     break;

   case IO_TRACE_REQUEST_START:
     iodriver_trace_request_start(0, curr);
     break;

   default:
     fprintf(stderr, "Unknown event type passed to io_internal_events\n");
     fprintf(outputfile, "Program Exit: Unknown event type passed to io_internal_events\n");
     dumpIOReq( "io_internal_event", curr );
     exit(1);
   }
   /*
     fprintf (outputfile, "Leaving io_internal_event\n");
   */
}


void iosim_get_path_to_controller (int iodriverno, int ctlno, intchar *buspath, intchar *slotpath)
{
   int depth;
   int currbus;
   char inslotno;
   char outslotno;
   int master;

#ifdef DEBUG_IOSIM
   fprintf (outputfile, "%f: iosim_get_path_to_controller, iodriverno %d, ctlno %d, buspath %p, slotpath %p\n", simtime, iodriverno, ctlno, buspath, slotpath );
   fflush(outputfile);
#endif

   depth = controller_get_depth(ctlno);
   currbus = controller_get_inbus(ctlno);
   inslotno = (char) controller_get_slotno(ctlno);
   master = controller_get_bus_master(currbus);
   while (depth > 0) {
/*
fprintf (outputfile, "ctlno %d, depth %d, currbus %d, inslotno %d, master %d\n", ctl->ctlno, depth, currbus, inslotno, master);
*/
      outslotno = (char) controller_get_outslot(master, currbus);
      buspath->byte[depth] = (char) currbus;
      slotpath->byte[depth] = (inslotno & 0x0F) | (outslotno << 4);
      depth--;
      currbus = controller_get_inbus(master);
      inslotno = (char) controller_get_slotno(master);
      master = controller_get_bus_master(currbus);
          /* Bus must have at least one controller */
      ASSERT1((master != -1) || (depth <= 0), "currbus", currbus);
   }
   outslotno = (char) iodriverno;
   buspath->byte[depth] = currbus;
   slotpath->byte[depth] = (inslotno & 0x0F) | (outslotno << 4);
}


void iosim_get_path_to_device (int iodriverno, int devno, intchar *buspath, intchar *slotpath)
{
   int depth;
   int currbus;
   char inslotno;
   char outslotno;
   int master;

#ifdef DEBUG_IOSIM
   fprintf(outputfile, "*** %f: iosim_get_path_to_controller  iodriverno %d, devno %d, buspath %p, slotpath %p\n", simtime, iodriver, devno, buspath, slotno );
   fflush(outputfile);
#endif

   depth = device_get_depth(devno);
   currbus = device_get_inbus(devno);
   inslotno = (char) device_get_slotno(devno);
   master = controller_get_bus_master(currbus);
   while (depth > 0) {
/*
fprintf (outputfile, "devno %d, depth %d, currbus %d, inslotno %d, master %d\n", devno, depth, currbus, inslotno, master);
*/
      outslotno = (char) controller_get_outslot(master, currbus);
      buspath->byte[depth] = (char) currbus;
      slotpath->byte[depth] = (inslotno & 0x0F) | (outslotno << 4);
      depth--;
      currbus = controller_get_inbus(master);
      inslotno = (char) controller_get_slotno(master);
      master = controller_get_bus_master(currbus);
          /* Bus must have at least one controller */
      ASSERT1((master != -1) || (depth <= 0), "currbus", currbus);
   }
   outslotno = (char) iodriverno;
   buspath->byte[depth] = currbus;
   slotpath->byte[depth] = (inslotno & 0x0F) | (outslotno << 4);
}




static int iosim_load_map(struct lp_block *b, int n) {
  int c;
  int i = 0;
  char *s = 0; 

  //#include "modules/disksim_iomap_param.c"
  lp_loadparams((void *)n, b, &disksim_iomap_mod);

  if (tracemap2[n] == 512) {
    tracemap2[n] = 0;
  } 
  else if (tracemap2[n] > 512) {
    tracemap2[n] = -(tracemap2[n] / 512);
  } 
  else {
    tracemap2[n] = 512 / tracemap2[n];
  }
  

  return 1;
}

int iosim_load_mappings(struct lp_list *l) {
  int c; 
  int mapno = 0;

  if(l->values_len > TRACEMAPPINGS) {
    fprintf(stderr, "*** error: too many io mappings.\n");
    return -1;
  }

  for(c = 0; c < l->values_len; c++) {
    if(!l->values[c]) continue;

    if(l->values[c]->t == BLOCK) {
      assert(iosim_load_map(l->values[c]->v.b, mapno));
      tracemappings++;
    }

    mapno++;
  }

  return 1;
}

int disksim_iomap_loadparams(struct lp_block *b) { 
  ddbg_assert2(0, "this is a dummy and isn't supposed to be called");
  return 0;
}


int disksim_iosim_loadparams(struct lp_block *b) {

  if(!disksim->iosim_info) 
    iosim_initialize_iosim_info();

  //#include "modules/disksim_iosim_param.c"
  lp_loadparams(0, b, &disksim_iosim_mod);


  return 1;
}





/* OBSELETE */
void io_readparams (FILE *parfile)
{
  ddbg_assert(0);
/*     fscanf(parfile, "\nI/O Subsystem Input Parameters\n"); */
/*     fprintf(outputfile, "\nI/O Subsystem Input Parameters\n"); */
/*     fscanf(parfile, "------------------------------\n"); */
/*     fprintf(outputfile, "------------------------------\n"); */

/*     fscanf(parfile, "\nPRINTED I/O SUBSYSTEM STATISTICS\n\n"); */
/*     fprintf(outputfile, "\nPRINTED I/O SUBSYSTEM STATISTICS\n\n"); */

/*     iodriver_read_toprints(parfile); */
/*     bus_read_toprints(parfile); */
/*     controller_read_toprints(parfile); */
/*     device_read_toprints(parfile); */

/*     fscanf(parfile, "\nGENERAL I/O SUBSYSTEM PARAMETERS\n\n"); */
/*     fprintf(outputfile, "\nGENERAL I/O SUBSYSTEM PARAMETERS\n\n"); */

/*     io_read_generalparms(parfile); */

/*     fscanf(parfile, "\nCOMPONENT SPECIFICATIONS\n"); */
/*     fprintf(outputfile, "\nCOMPONENT SPECIFICATIONS\n"); */

/*     iodriver_read_specs(parfile); */
/*     bus_read_specs(parfile); */
/*     controller_read_specs(parfile); */
/*     device_read_specs(parfile); */

/*     fscanf(parfile, "\nPHYSICAL ORGANIZATION\n"); */
/*     fprintf(outputfile, "\nPHYSICAL ORGANIZATION\n"); */

/*     iodriver_read_physorg(parfile); */
/*     controller_read_physorg(parfile); */
/*     bus_read_physorg(parfile); */

/*     fscanf(parfile, "\nSYNCHRONIZATION\n"); */
/*     fprintf(outputfile, "\nSYNCHRONIZATION\n"); */

/*     device_read_syncsets(parfile); */

/*     fscanf(parfile, "\nLOGICAL ORGANIZATION\n"); */
/*     fprintf(outputfile, "\nLOGICAL ORGANIZATION\n"); */

/*     iodriver_read_logorg(parfile); */
/*     controller_read_logorg(parfile); */
/*
   bus_print_phys_config();
*/
}


void io_validate_do_stats1 ()
{
   int i;

   if (tracestats2 == NULL) {
      tracestats2 = (statgen *)DISKSIM_malloc(sizeof(statgen));
      tracestats3 = (statgen *)DISKSIM_malloc(sizeof(statgen));
      tracestats4 = (statgen *)DISKSIM_malloc(sizeof(statgen));
      tracestats5 = (statgen *)DISKSIM_malloc(sizeof(statgen));

      stat_initialize(statdeffile, statdesc_traceaccstats, tracestats2);
      stat_initialize(statdeffile, statdesc_traceaccdiffstats, tracestats3);
      stat_initialize(statdeffile, statdesc_traceaccwritestats, tracestats4);
      stat_initialize(statdeffile, statdesc_traceaccdiffwritestats, tracestats5);
      for (i=0; i<10; i++) {
	 validatebuf[i] = 0;
      }
   } else {
      stat_update(tracestats3, (validate_lastserv - disksim->lastphystime));
      if (!validate_lastread) {
         stat_update(tracestats5, (validate_lastserv - disksim->lastphystime));
      }
   }
}


void io_validate_do_stats2 (ioreq_event *new_event)
{
   stat_update(tracestats2, validate_lastserv);
   if (new_event->flags == WRITE) {
      stat_update(tracestats4, validate_lastserv);
   }
   if (strcmp(validate_buffaction, "Doub") == 0) {
      validatebuf[0]++;
   } else if (strcmp(validate_buffaction, "Trip") == 0) {
      validatebuf[1]++;
   } else if (strcmp(validate_buffaction, "Miss") == 0) {
      validatebuf[2]++;
   } else if (strcmp(validate_buffaction, "Hit") == 0) {
      validatebuf[3]++;
   } else {
      fprintf(stderr, "Unrecognized buffaction in validate trace: %s\n", validate_buffaction);
      exit(1);
   }
}


static void io_hpl_do_stats1 ()
{
   int i;

   if (tracestats == NULL) {
      tracestats  = (statgen *)DISKSIM_malloc(tracemappings * sizeof(statgen));
      tracestats1 = (statgen *)DISKSIM_malloc(tracemappings * sizeof(statgen));
      tracestats2 = (statgen *)DISKSIM_malloc(tracemappings * sizeof(statgen));
      tracestats3 = (statgen *)DISKSIM_malloc(tracemappings * sizeof(statgen));
      tracestats4 = (statgen *)DISKSIM_malloc(tracemappings * sizeof(statgen));

      for (i=0; i<tracemappings; i++) {
         stat_initialize(statdeffile, statdesc_tracequeuestats, &tracestats[i]);
         stat_initialize(statdeffile, statdesc_tracerespstats, &tracestats1[i]);
         stat_initialize(statdeffile, statdesc_traceaccstats, &tracestats2[i]);
         stat_initialize(statdeffile, statdesc_traceqlenstats, &tracestats3[i]);
         stat_initialize(statdeffile, statdesc_tracenoqstats, &tracestats4[i]);
      }
   }
}


void io_map_trace_request (ioreq_event *temp)
{
   int i;

   for (i=0; i<tracemappings; i++) {
      if (temp->devno == tracemap[i]) {
#ifdef DEBUG_IOSIM
		 fprintf (outputfile, "*** %f: io_map_trace_request mapping devno number %d to %d\n", simtime, temp->devno, tracemap1[i] );
#endif
		 temp->devno = tracemap1[i];
		 if (tracemap2[i]) {
			if (tracemap2[i] < 1) {
			   temp->blkno *= -tracemap2[i];
			} else {
			   if (temp->blkno % tracemap2[i]) {
				  fprintf(stderr, "Small sector size disk using odd sector number: %d\n", temp->blkno);
				  exit(1);
			   }
#ifdef DEBUG_IOSIM
			   fprintf (outputfile, "*** %f: io_map_trace_request mapping block number %d to %d\n", simtime, temp->blkno, (temp->blkno / tracemap2[i]));
#endif
			   temp->blkno /= tracemap2[i];
			}
		 }
		 temp->bcount *= tracemap3[i];
		 temp->blkno += tracemap4[i];
		 if (tracestats) {
			stat_update(&tracestats[i], ((double) temp->tempint1 / (double) 1000));
			stat_update(&tracestats1[i],((double) (temp->tempint1 + temp->tempint2) / (double) 1000));
			stat_update(&tracestats2[i],((double) temp->tempint2 / (double) 1000));
			stat_update(&tracestats3[i], (double) temp->slotno);
			if (temp->slotno == 1) {
			   stat_update(&tracestats4[i], ((double) temp->tempint1 / (double) 1000));
			}
		 }
		 return;
	  }
   }
/*
   fprintf(stderr, "Requested device not mapped - %x\n", temp->devno);
   exit(1);
*/
}


event * io_get_next_external_event (FILE *iotracefile)
{
   ioreq_event *temp;

   ASSERT(io_extq == NULL);

#ifdef DEBUG_IOSIM
   fprintf (outputfile, "Entering: io_get_next_external_event\n");
#endif

   temp = (ioreq_event *) getfromextraq();

   switch (disksim->traceformat) {
      case VALIDATE: io_validate_do_stats1();
		     break;
      case HPL: io_hpl_do_stats1();
                break;
   }

   temp = iotrace_get_ioreq_event(iotracefile, disksim->traceformat, temp);
   if (temp) {
      switch (disksim->traceformat) {
         case VALIDATE: io_validate_do_stats2 (temp);
		        break;
      }
      temp->type = IO_REQUEST_ARRIVE;
      if (constintarrtime > 0.0) {
	 temp->time = last_request_arrive + constintarrtime;
	 last_request_arrive = temp->time;
      }
      temp->time = (temp->time * ioscale) + tracebasetime;
      if ((temp->time < simtime) && (!disksim->closedios)) {
         fprintf(stderr, "Trace event appears out of time order in trace - simtime %f, time %f\n", simtime, temp->time);
	 fprintf(stderr, "ioscale %f, tracebasetime %f\n", ioscale, tracebasetime);
	 fprintf(stderr, "devno %d, blkno %d, bcount %d, flags %d\n", temp->devno, temp->blkno, temp->bcount, temp->flags);
         exit(1);
      }
      if (tracemappings) {
         io_map_trace_request(temp);
      }
      io_extq = (event *)temp;
      io_extq_type = temp->type;

#ifdef DEBUG_IOSIM
	  fprintf( outputfile, "*** %f: Set io_extq, type %d, devno %d, blkno %d, bcount %d, flags %d\n", simtime, temp->type, temp->devno, temp->blkno, temp->bcount, temp->flags );
#endif
   }

#ifdef DEBUG_IOSIM
   fprintf (outputfile, "Exiting: io_get_next_external_event\n");
#endif

   return ((event *)temp);
}


int io_using_external_event (event *curr)
{
#ifdef DEBUG_IOSIM
   fprintf( outputfile, "*** %f: Entered: io_using_external_event\n", simtime );
#endif

   if (io_extq == curr) {
      curr->type = io_extq_type;
      io_extq = NULL;

#ifdef DEBUG_IOSIM
	  fprintf( outputfile, "*** %f: io_using_external_event conversion, type %d, devno %d, blkno %d, bcount %d, flags %d\n", simtime, ((ioreq_event *)curr)->type, ((ioreq_event *)curr)->devno, ((ioreq_event *)curr)->blkno, ((ioreq_event *)curr)->bcount, ((ioreq_event *)curr)->flags );
#endif

      return(1);
   }
   return(0);
}


void io_printstats()
{
   int i;
   int cnt = 0;
   char prefix[80];

   fprintf (outputfile, "\nSTORAGE SUBSYSTEM STATISTICS\n");
   fprintf (outputfile, "----------------------------\n");

   iotrace_printstats (outputfile);

   if ((tracestats) && (PRINTTRACESTATS)) {
      /* info relevant to HPL traces */
      for (i=0; i<tracemappings; i++) {
         sprintf(prefix, "Mapped disk #%d ", i);
         stat_print(&tracestats[i], prefix);
         stat_print(&tracestats1[i], prefix);
         stat_print(&tracestats2[i], prefix);
         stat_print(&tracestats3[i], prefix);
         stat_print(&tracestats4[i], prefix);
      }
   } else if ((tracestats2) && (PRINTTRACESTATS)) {
      /* info to help with validation */
      fprintf (outputfile, "\n");
      stat_print(tracestats2, "VALIDATE ");
      stat_print(tracestats3, "VALIDATE ");
      stat_print(tracestats4, "VALIDATE ");
      stat_print(tracestats5, "VALIDATE ");
      for (i=0; i<10; i++) {
	 cnt += validatebuf[i];
      }
      fprintf (outputfile, "VALIDATE double disconnects:  %5d  \t%f\n", validatebuf[0], ((double) validatebuf[0] / (double) cnt));
      fprintf (outputfile, "VALIDATE triple disconnects:  %5d  \t%f\n", validatebuf[1], ((double) validatebuf[1] / (double) cnt));
      fprintf (outputfile, "VALIDATE read buffer hits:    %5d  \t%f\n", validatebuf[3], ((double) validatebuf[3] / (double) cnt));
      fprintf (outputfile, "VALIDATE buffer misses:       %5d  \t%f\n", validatebuf[2], ((double) validatebuf[2] / (double) cnt));
   }

   iodriver_printstats();
   device_printstats();
   controller_printstats();
   bus_printstats();
}




void io_setcallbacks ()
{
   device_setcallbacks ();
   bus_setcallbacks ();
   controller_setcallbacks ();
   iodriver_setcallbacks ();
}


void io_initialize (int standalone)
{
   if (disksim->iosim_info == NULL) {
      iosim_initialize_iosim_info ();
   }

   bus_set_depths();
   // fprintf (outputfile, "Back from bus_set_depths\n");

   StaticAssert (sizeof(ioreq_event) <= DISKSIM_EVENT_SIZE);
   device_initialize();
   bus_initialize();
   controller_initialize();
   iodriver_initialize(standalone);
}


void io_resetstats()
{
   iodriver_resetstats();
   device_resetstats();
   bus_resetstats();
   controller_resetstats();
}


void io_cleanstats()
{
   iodriver_cleanstats();
   device_cleanstats();
   bus_cleanstats();
   controller_cleanstats();
}

// End of file


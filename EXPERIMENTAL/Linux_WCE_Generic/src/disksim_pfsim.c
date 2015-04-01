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
#include "disksim_pfface.h"
#include "disksim_pfsim.h"
#include "disksim_ioface.h"
#include "disksim_synthio.h"
#include "config.h"

#include "modules/modules.h"


int pf_how_many_cpus()
{
   return (numcpus);
}


static void pf_allocate_process_structs()
{
   int i;
   process *temp = (process *) DISKSIM_malloc(ALLOCSIZE);

   ASSERT(temp != NULL);
   for (i = 0; i < (int)((ALLOCSIZE/sizeof(process))-1); i++) {
      temp[i].next = &temp[i+1];
   }
   temp[((ALLOCSIZE/sizeof(process))-1)].next = NULL;
   extra_process_q = temp;
   extra_process_qlen = ALLOCSIZE/sizeof(process);
}


#if 0
static void pf_addtoextra_process_q(temp)
process *temp;
{
   temp->next = extra_process_q;
   extra_process_q = temp;
   extra_process_qlen++;
}
#endif


process *pf_getfromextra_process_q()
{
   process *temp = extra_process_q;

   if (extra_process_qlen == 0) {
      pf_allocate_process_structs();
      temp = extra_process_q;
      extra_process_q = extra_process_q->next;
   } else if (extra_process_qlen == 1) {
      extra_process_q = NULL;
   } else {
      extra_process_q = extra_process_q->next;
   }
   extra_process_qlen--;
   temp->pfflags = 0;
   temp->stat = PROC_RUN;
   temp->idler = 0;
   temp->runcpu = 0;
   temp->flags = 0;
   temp->chan = 0;
   temp->ios = 0;
   temp->ioreads = 0;
   temp->cswitches = 0;
   temp->active = 0;
   temp->sleeps = 0;
   temp->iosleep = 0;
   temp->iosleeps = 0;
   temp->runiosleep = 0.0;
   temp->lastsleep = 0.0;
   temp->runsleep = 0.0;
   temp->runtime = 0.0;
   temp->falseidletime = 0.0;
   temp->lasteventtime = -1.0;
   stat_initialize(statdeffile, "Time limit duration", &temp->readtimelimitstats);
   stat_initialize(statdeffile, "Time limit duration", &temp->writetimelimitstats);
   stat_initialize(statdeffile, "Time limit duration", &temp->readmisslimitstats);
   stat_initialize(statdeffile, "Time limit duration", &temp->writemisslimitstats);
   temp->ioreq = NULL;
   temp->eventlist = NULL;
   temp->space = NULL;
   temp->link = NULL;
   temp->next = NULL;
   return(temp);
}


/* This is NOT general.  It is basically only used by synthio.c during */
/* synthio_readparams.                                                 */

process * pf_new_process()
{
   process *procp = pf_getfromextra_process_q();

   procp->pid = (synthlist) ? (synthlist->pid + 1) : 0;
   procp->next = synthlist;
   synthlist = procp;
   return(procp);
}


#if 0
static event * pf_remove_from_process_eventlist (process *procp)
{
   event *old = procp->eventlist;
   procp->eventlist = old->next;
   return(old);
}
#endif


static void pf_add_to_process_eventlist (process *procp, event *new_event)
{
   new_event->next = procp->eventlist;
   procp->eventlist = new_event;
}


static void pf_add_to_intrp_eventlist (intr_event *intrp, event *new_event)
{
   new_event->next = intrp->eventlist;
   intrp->eventlist = new_event;
}


static double pf_add_false_idle_time (double length)
{
   double ret = 0.0;
   process *procp = process_livelist;
   while (procp != NULL) {
      if ((procp->stat == PROC_SLEEP) && (procp->iosleep)) {
         procp->falseidletime += length;
         ret = length;
      } else if ((procp->stat == PROC_RUN) && (procp->pfflags & PF_SLEEP_FLAG) && (procp->iosleep)) {
         procp->falseidletime += length;
         ret = length;
      } else if ((procp->stat == PROC_ONPROC) && (cpus[procp->runcpu].state == CPU_IDLE) && (procp->pfflags & PF_SLEEP_FLAG) && (procp->iosleep)) {
         procp->falseidletime += length;
         ret = length;
      }
      procp = procp->livelist;
   }
   return(ret);
}


static void pf_add_to_pendiolist (process *procp, ioreq_event *curr)
{
   ioreq_event *new_event = (ioreq_event *)getfromextraq();
   new_event->time = simtime;
   new_event->devno = curr->devno;
   new_event->blkno = curr->blkno;
   new_event->buf = curr->buf;
   new_event->opid = curr->opid;
   new_event->tempptr1 = procp;
   new_event->tempint2 = -1;
   new_event->flags = curr->flags & ~PF_BUF_WANTED;
   new_event->prev = NULL;
   new_event->next = pendiolist;
   pendiolist = new_event;
}


event * pf_io_done_notify (ioreq_event *curr, void *ctx)
{
   ioreq_event *tmp = pendiolist;
   ioreq_event *tmp2;
   wakeup_event *tmpwake = NULL;

   if(disksim->external_control){
     disksim->external_io_done_notify (curr, ctx);
   }

   if (pf_printhack) {
     fprintf (outputfile, "pf_io_done_notify: curr->buf %p, curr->opid %x, curr->blkno %d\n", curr->buf, curr->opid, curr->blkno);
   }

   ASSERT(pendiolist != NULL);
   if ((tmp->buf == curr->buf) && (tmp->opid == curr->opid)) {
      pendiolist = tmp->next;
   } 
   else {
     while ((tmp->next) && 
	    ((tmp->next->buf != curr->buf) || 
	     (tmp->next->opid != curr->opid))) 
       {
         tmp = tmp->next;
       }
     
     ASSERT(tmp->next != NULL);
     tmp2 = tmp->next;
     tmp->next = tmp2->next;
     tmp = tmp2;
   }

   if (tmp->flags & PF_BUF_WANTED) {
      tmpwake = (wakeup_event *) getfromextraq();
      tmpwake->type = WAKEUP_EVENT;
      /* tmpwake->time = PF_IO_WAKEUP_TIME;         gets overridden anyway */
      tmpwake->next = NULL;
      tmpwake->chan = tmp->buf;
      tmpwake->dropit = 0;
   }

   if (curr->flags & TIME_CRITICAL) {
      stat_update(&timecritrespstats, (simtime - tmp->time));
   } 
   else if (curr->flags & TIME_LIMITED) {
      stat_update(&timelimitrespstats, (simtime - tmp->time));
   } 
   else {
      stat_update(&timenoncritrespstats, (simtime - tmp->time));
   }

   if (!(curr->flags & TIME_LIMITED) || (tmpwake)) {
      addtoextraq((event *) tmp);
   } 
   else {
      tmp->next = doneiolist;
      doneiolist = tmp;
   }
   return((event *) tmpwake);
}


static int pf_iowait (void *chan, process *procp)
{
   double tmptime;
   ioreq_event *tmp = pendiolist;

   while ((tmp) && (tmp->buf != chan)) {
      tmp = tmp->next;
   }
   if (tmp == NULL) {
      ioreq_event *prev = 0;
      tmp = doneiolist;
      while ((tmp) && (tmp->buf != chan)) {
         prev = tmp;
         tmp = tmp->next;
      }
      if (tmp) {
         if (doneiolist == tmp) {
            doneiolist = tmp->next;
         } else {
            prev->next = tmp->next;
         }
         if (tmp->flags & READ) {
            stat_update(&procp->readtimelimitstats, (simtime - tmp->time));
         } else {
            stat_update(&procp->writetimelimitstats, (simtime - tmp->time));
         }
         addtoextraq((event *) tmp);
      }
      return(FALSE);
   }

#ifdef DEBUG_PFSIM
	fprintf (outputfile, "pf_iowait: chan %p, read %d, crit %d, opid %d, blkno %x\n", tmp->buf, (tmp->flags & READ), (tmp->flags & (TIME_LIMITED|TIME_CRITICAL)), tmp->opid, tmp->blkno);
#endif

   if (tmp->flags & TIME_LIMITED) {
      if (tmp->flags & READ) {
         stat_update(&procp->readtimelimitstats, (simtime - tmp->time));
         stat_update(&procp->readmisslimitstats, (simtime - tmp->time));
      } else {
         stat_update(&procp->writetimelimitstats, (simtime - tmp->time));
         stat_update(&procp->writemisslimitstats, (simtime - tmp->time));
      }
   }
   tmp->flags |= PF_BUF_WANTED;
   tmptime = io_raise_priority(tmp->opid, tmp->devno, tmp->blkno, tmp->buf);
   if (tmptime != 0.0) {
      fprintf(stderr, "Haven't handled non-zero bufferwait time yet\n");
      exit(1);
   }
   return(TRUE);
}


static void pf_add_cswitch_event (process *procp, process *newprocp, cpu_event *cpu_ev)
{
   cpu *currcpu = &cpus[(cpu_ev->cpunum)];
   cswitch_event *new_event = (cswitch_event *) getfromextraq();
   new_event->type = CSWITCH_EVENT;
   new_event->newprocp = newprocp;
   if (procp != newprocp) {
      new_event->time = PF_CSWITCH_TIME;
      currcpu->cswitches++;
   } else {
      new_event->time = 0.0;
   }
   currcpu->runswitchtime += new_event->time;
   if (procp) {
      pf_add_to_process_eventlist(procp, (event *) new_event);
   } else {
      new_event->next = currcpu->idleevents;
      currcpu->idleevents = (event *) new_event;
   }
}


static double pf_get_time_to_next_process_event (process *procp)
{
   double tmptime;

   if (procp->eventlist == NULL) {
      fprintf (outputfile, "Active process at end of eventlist: %d\n", procp->pid);
      disksim_simstop();
      return(0.0);
   }

   tmptime = procp->eventlist->time;
   procp->eventlist->time = simtime;
   return(tmptime);
}


static double pf_get_time_to_next_intr_event (intr_event *intrp)
{
   double tmptime;

   ASSERT(intrp->eventlist != NULL);
   tmptime = intrp->eventlist->time;
   intrp->eventlist->time = simtime;
   return(tmptime);
}


/* called when a process is enabled by another CPU while this CPU is idle */

void pf_idle_cpu_recheck_dispq (int cpunum)
{
  ASSERT(cpus[cpunum].current->procp == NULL);

   if (cpus[cpunum].idleevents == NULL) {
      event *new_event = getfromextraq();
      new_event->type = IDLELOOP_EVENT;
      new_event->time = PF_IDLE_CHECK_DISPQ_TIME;
      new_event->next = cpus[cpunum].idleevents;
      cpus[cpunum].idleevents = new_event;

      if (cpus[cpunum].current->intrp == NULL) {
	cpus[cpunum].idletime += simtime - cpus[cpunum].idlestart;
	cpus[cpunum].falseidletime += pf_add_false_idle_time(simtime - cpus[cpunum].idlestart);
	cpus[cpunum].current->time = simtime + (cpus[cpunum].scale * new_event->time);
	new_event->time = simtime;
	cpus[cpunum].state = CPU_IDLE_WORK;
	addtointq((event *) cpus[cpunum].current);
      }
   }
}


static void pf_handle_cswitch_event (cswitch_event *curr, cpu_event *cpu_ev)
{
   process *procp = NULL;

   if (pf_printhack)
   fprintf (outputfile, "%.3f\tCONTEXT SWITCH cpu=%d pid=%d, newpid=%d\n", simtime, cpu_ev->cpunum, ((cpu_ev->procp) ? cpu_ev->procp->pid : 0), ((curr->newprocp) ? curr->newprocp->pid : 0));

   if (cpu_ev->procp) {
      if (cpu_ev->procp->idler == 0) {
         lastuser = simtime;
      }
      cpu_ev->procp->cswitches++;
   }

   /* Used to identify how long the system has been completely idle   */
   /* Simulation should stop rather than continuing to idle endlessly */
   if ((idlereset == 0) && 
       ((curr->newprocp == NULL) || 
	(curr->newprocp->idler != 0))) {
      idlein = simtime;
      idlereset = 1;
   } 
   else {
      idlereset = 0;
   }

   cpu_ev->procp = curr->newprocp;
   cpus[(cpu_ev->cpunum)].state = (curr->newprocp) ? CPU_PROCESS : CPU_IDLE;
   if (cpu_ev->procp) {
      cpu_ev->procp->pfflags &= ~PF_SLEEP_FLAG;
   }

   if ((cpu_ev->procp == NULL) && (pf_dispq)) {
      procp = pf_dispatch(cpu_ev->cpunum);
      if (procp != cpu_ev->procp) {
         pf_add_cswitch_event(cpu_ev->procp, procp, cpu_ev);
      }
   } else {
      addlisttoextraq((event **) &cpus[(cpu_ev->cpunum)].idleevents);
   }
}


static void pf_handle_idleloop_event (idleloop_event *curr, cpu_event *cpu_ev)
{
   process *procp;

   if (pf_printhack)
   fprintf (outputfile, "%f\tIDLELOOP cpu=%d\n", simtime, cpu_ev->cpunum);

   ASSERT(cpu_ev->procp == NULL);
   procp = pf_dispatch(cpu_ev->cpunum);
   if (procp != cpu_ev->procp) {
      pf_add_cswitch_event(cpu_ev->procp, procp, cpu_ev);
   }
}


static void pf_handle_sleep_event (sleep_event *curr, cpu_event *cpu_ev)
{
   process *procp = cpu_ev->procp;
   process *newprocp;

#ifdef DEBUG_PFSIM
   fprintf (outputfile, "*** %f: pf_handle_sleep_event SLEEP cpu=%d pid=%d chan=%p info=%d iosleep=%d\n", simtime, cpu_ev->cpunum, procp->pid, curr->chan, curr->info, curr->iosleep);
#endif

   if (pf_iowait(curr->chan, procp)) {
      ASSERT(curr->iosleep > 0);
      procp->iosleep = TRUE;
      procp->iosleeps++;
   } else if (curr->iosleep) {
      return;   /* to support synthetic TIME_LIMIT'd requests */
   } else {
      ASSERT (0);
   }
   procp->pfflags |= PF_SLEEP_FLAG;
   procp->chan = curr->chan;
   newprocp = pf_disp_sleep(procp);
   pf_add_cswitch_event(procp, newprocp, cpu_ev);
   procp->lastsleep = simtime;
   procp->sleeps++;
}


static void pf_handle_wakeup_event(wakeup_event *curr, cpu_event *cpu_ev)
{
   process *procp;

   if (pf_printhack) {
     fprintf (outputfile, "%f\tWAKEUP cpu=%d chan=%p info=%d\n", 
	      simtime, cpu_ev->cpunum, curr->chan, curr->info);
   }

   while ((procp = pf_disp_get_from_sleep_queue(curr->chan))) {

     if (pf_printhack) {
       fprintf (outputfile, "Waking up process %d\n", procp->pid);
     }

     procp->runsleep += simtime - procp->lastsleep;
     if (procp->iosleep == 1) {
       procp->runiosleep += simtime - procp->lastsleep;
     }
     pf_disp_wakeup(procp);
   }

   procp = cpu_ev->procp;
   if ((procp) 
       && ((procp->pfflags & PF_SLEEP_FLAG) 
	   && (procp->chan == curr->chan))) 
     {
       procp->pfflags |= PF_WAKED;
     }
}


static void pf_handle_ioreq_event (ioreq_event *curr, cpu_event *cpu_ev)
{
   process *procp = cpu_ev->procp;
   ioreq_event *new_event;
   ioreq_event *tmp;

   io_map_trace_request(curr);
   curr->next = NULL;

#ifdef DEBUG_PFSIM
   fprintf (outputfile, "*** %f: pf_handle_ioreq_event IOREQ cpu=%d opid=%d buf=%p blkno=%x flags=%x, bcount=%d\n", simtime, cpu_ev->cpunum, curr->opid, curr->buf, curr->blkno, curr->flags, curr->bcount);
#endif

   curr->flags &= ~(TIMED_OUT|HALF_OUT); /* hack to help out ioqueue.c */

   pf_add_to_pendiolist(procp, curr);

   if (cpu_ev->intrp == NULL) {
      procp->ios++;
      if (curr->flags & READ) {
         procp->ioreads++;
      }
   }
   curr->cause = 0;   /* To avoid a stupid problem in logorg.c --- TEMP  */
   new_event = (ioreq_event *) io_request(curr);
   if (new_event) {
      tmp = (ioreq_event *) procp->eventlist;
      procp->eventlist = (event *) new_event;
      while (new_event->next) {
         new_event = new_event->next;
      }
      new_event->next = tmp;
   }
}


static void pf_handle_ioacc_event (ioreq_event *curr, cpu_event *cpu_ev)
{
#ifdef DEBUG_PFSIM
//   if (pf_printhack)
   fprintf (outputfile, "*** %f: pf_handle_ioacc_event IOACC cpu=%d opid=%d blkno=%x\n", simtime, cpu_ev->cpunum, curr->opid, curr->blkno);
#endif

   io_schedule(curr);
}


static void pf_handle_io_internal_event (ioreq_event *curr, cpu_event *cpu_ev)
{
#ifdef DEBUG_PFSIM
//	if (pf_printhack)
   fprintf (outputfile, "*** %f: pf_handle_io_internal_event IO INTERNAL cpu=%d type %d opid=%d blkno=%x buf=%p\n", simtime, cpu_ev->cpunum, curr->type, curr->opid, curr->blkno, curr->buf);
#endif

   if (curr->type == IO_REQUEST_ARRIVE) {
      ioreq_event *new_event = (ioreq_event *) io_request (curr);
      if (new_event) {
         intr_event *intrp = cpu_ev->intrp;
         ioreq_event *tmp;
         ASSERT (intrp != NULL);

         tmp = (ioreq_event *) intrp->eventlist;
         intrp->eventlist = (event *) new_event;
         while (new_event->next) {
            new_event = new_event->next;
         }
         new_event->next = tmp;
      }
   } else {
      io_internal_event(curr);
   }
}


static void pf_clock_interrupt_complete (intr_event *intrp)
{
   addtoextraq((event *) intrp);
}


static void pf_handle_intend_event (intend_event *curr, cpu_event *cpu_ev)
{
   intr_event *intrp = cpu_ev->intrp;
   process *procp = cpu_ev->procp;
   cpu *currcpu = &cpus[(cpu_ev->cpunum)];

#ifdef DEBUG_PFSIM
   fprintf (outputfile, "*** %f: pf_handle_intend_event INTEND cpu=%d\n", simtime, cpu_ev->cpunum);
#endif

   cpu_ev->intrp = (intr_event *) intrp->next;
   currcpu->state = intrp->oldstate;
   currcpu->intrs++;
   currcpu->runintrtime += intrp->runtime;
   if (intrp->vector == IO_INTERRUPT) {
      currcpu->iointrs++;
      currcpu->runiointrtime += intrp->runtime;
      io_interrupt_complete ((ioreq_event *) intrp);
   } else if (intrp->vector == CLOCK_INTERRUPT) {
      currcpu->clockintrs++;
      currcpu->runclockintrtime += intrp->runtime;
      pf_clock_interrupt_complete(intrp);
   } else {
      fprintf (stderr, "pf_handle_intend_event: unknown interrupt vector (%d)\n", intrp->vector);
      exit(1);
   }
   intrp = cpu_ev->intrp;
   if (intrp == NULL) {
      if ((procp == NULL) && (pf_dispq)) {
         procp = pf_dispatch(cpu_ev->cpunum);
         assert (procp != NULL);
         pf_add_cswitch_event (NULL, procp, cpu_ev);
      }
      if ((procp == NULL) || (procp->idler != 0)) {
         if ((idlereset) && ((simtime - idlein) > (double) 50000.0)) {
            fprintf (outputfile, "Idle process has been running for more than 50 seconds\n");
            disksim_simstop();
         }
      }
   }
}


static void pf_handle_synthio_event (event *curr, cpu_event *cpu_ev)
{
#ifdef DEBUG_PFSIM
   fprintf (outputfile, "*** %f, SYNTHIO_EVENT\n", simtime);
#endif
   curr->time = 0.0;
   curr->next = cpu_ev->procp->eventlist;
   cpu_ev->procp->eventlist = curr;
   synthio_generate_io_activity(cpu_ev->procp);
}


static void pf_handle_event (event *curr, cpu_event *cpu_ev)
{
#ifdef DEBUG_PFSIM
    fprintf (outputfile, "*** %f: Entered pf_handle_event with event type %d\n", simtime, curr->type);
    fflush(outputfile);
#endif

  switch (curr->type) {
  case IDLELOOP_EVENT:
    pf_handle_idleloop_event((idleloop_event *)curr, cpu_ev);
    break;
  case CSWITCH_EVENT:
    pf_handle_cswitch_event((cswitch_event *)curr, cpu_ev);
    break;
  case SLEEP_EVENT:
    pf_handle_sleep_event((sleep_event *)curr, cpu_ev);
    break;
  case WAKEUP_EVENT:
    pf_handle_wakeup_event((wakeup_event *)curr, cpu_ev);
    break;
  case IOREQ_EVENT:
    pf_handle_ioreq_event((ioreq_event *)curr, cpu_ev);
    curr = NULL;
    break;
  case IOACC_EVENT:
    pf_handle_ioacc_event((ioreq_event *)curr, cpu_ev);
    curr = NULL;
    break;
  case INTEND_EVENT:
    pf_handle_intend_event((intend_event *)curr, cpu_ev);
    break;
  case SYNTHIO_EVENT:
    pf_handle_synthio_event(curr, cpu_ev);
    curr = NULL;
    break;
  default:
    if ((curr->type >= IO_MIN_EVENT) && (curr->type <= IO_MAX_EVENT)) {
      pf_handle_io_internal_event((ioreq_event *)curr, cpu_ev);
      curr = NULL;
      break;
    }
    fprintf(stderr, "Trying to handle unknown pf event - %d\n", curr->type);
    exit(1);
  }
  addtoextraq((event *) curr);
}


static void pf_handle_io_intr_arrive (intr_event *intrp, cpu_event *cpu_ev)
{
#ifdef DEBUG_PFSIM
   ioreq_event *tmp = (ioreq_event *) intrp->infoptr;
    fprintf (outputfile, "%f\tIO_INTERRUPT cpu %d  cause %d  buf %p\n", simtime, cpu_ev->cpunum, tmp->cause, tmp->buf);
#endif

   io_interrupt_arrive ((ioreq_event *)intrp);
}


static void pf_handle_clock_intr_arrive (intr_event *intrp, cpu_event *cpu_ev)
{
   intr_event *nextclock;
   intend_event *new_event;
   curlbolt++;

#ifdef DEBUG_PFSIM
//if (pf_printhack)
   fprintf (outputfile, "*** %f: pf_handle_clock_intr_arrive CLOCK_INTERRUPT - new lbolt %d\n", simtime, curlbolt);
#endif

   ASSERT (intrp->eventlist == NULL);
   new_event = (intend_event *) getfromextraq();
   new_event->type = INTEND_EVENT;
   new_event->vector = intrp->type;
   if ((curlbolt % PF_INTER_LONG_CLOCKS) == 0) {
      io_tick();
      new_event->time = PF_LONG_CLOCK_TIME;
   } else {
      new_event->time = PF_SHORT_CLOCK_TIME;
   }
   pf_add_to_intrp_eventlist(intrp, (event *)new_event);

   nextclock = (intr_event *) getfromextraq();
   nextclock->type = INTR_EVENT;
   nextclock->time = intrp->time + PF_INTER_CLOCK_TIME;
   nextclock->vector = CLOCK_INTERRUPT;
   nextclock->eventlist = NULL;
   addtointq((event *) nextclock);
}


void pf_handle_intr_event (intr_event *intrp, int cpunum)
{
   cpu *currcpu = &cpus[cpunum];
   cpu_event *cpu_ev = currcpu->current;
   process *procp = cpu_ev->procp;

#ifdef DEBUG_PFSIM
//   if (pf_printhack)
   fprintf (outputfile, "*** %f: pf_handle_intr_event INTR cpu=%d vector=%d cpustate=%d\n", simtime, cpunum, intrp->vector, currcpu->state);
#endif

   intrp->runtime = 0.0;
   intrp->flags = 0;
   intrp->next = cpu_ev->intrp;
   if (currcpu->state == CPU_IDLE) {
      currcpu->idletime += simtime - currcpu->idlestart;
      currcpu->falseidletime += pf_add_false_idle_time(simtime - currcpu->idlestart);
   } else {
      if (removefromintq((event *)cpu_ev) == 0) {
         fprintf(stderr, "Event from active CPU not pending on internal queue\n");
         exit(1);
      }
      if (intrp->next) {
         intrp->next->runtime += simtime - intrp->next->eventlist->time;
         intrp->next->eventlist->time = (cpu_ev->time - simtime) / currcpu->scale;
      } else if (procp == NULL) {
         currcpu->idleworktime += simtime - currcpu->idleevents->time;
         currcpu->idleevents->time = (cpu_ev->time - simtime) / currcpu->scale;
      } else {
         procp->runtime += simtime - procp->eventlist->time;
         procp->eventlist->time = (cpu_ev->time - simtime) / currcpu->scale;
      }
   }
   intrp->oldstate = currcpu->state;
   currcpu->state = CPU_INTR;
   cpu_ev->intrp = intrp;
   if (intrp->vector == IO_INTERRUPT) {
      pf_handle_io_intr_arrive(intrp, cpu_ev);
   } else if (intrp->vector == CLOCK_INTERRUPT) {
      pf_handle_clock_intr_arrive(intrp, cpu_ev);
   } else {
      fprintf(stderr, "Unknown interrupt vector: %d\n", intrp->vector);
      exit(1);
   }
   cpu_ev->time = currcpu->scale * pf_get_time_to_next_intr_event(intrp);
   cpu_ev->time += simtime;
   addtointq((event *) cpu_ev);
/*
fprintf (outputfile, "Exit pf_handle_intr_event\n");
*/
}


static void pf_handle_cpu_event (cpu_event *curr)
{
   event *tmp;
   cpu *currcpu = &cpus[(curr->cpunum)];

#ifdef DEBUG_PFSIM
   //if (simtime > 0.0) {
   //  pf_printhack = 1;
   //}
    
//   if(pf_printhack) {
     fprintf (outputfile, "*** %f: Entered pf_handle_cpu_event: type %d, cpu %d\n", simtime, curr->type, curr->cpunum);
     fflush(outputfile);
//   }
#endif

   if(curr->intrp) {
      tmp = curr->intrp->eventlist;
      curr->intrp->runtime += simtime - tmp->time;
      curr->intrp->eventlist = tmp->next;
   } 
   else if(curr->procp) {
      curr->procp->lasteventtime = simtime;
      tmp = curr->procp->eventlist;
      curr->procp->runtime += simtime - tmp->time;
      curr->procp->eventlist = tmp->next;
   } 
   else {
      tmp = currcpu->idleevents;
      currcpu->idleworktime += simtime - tmp->time;
      currcpu->idleevents = tmp->next;
   }

   ASSERT(tmp != NULL);
   tmp->next = NULL;
   pf_handle_event(tmp, curr);

   if(curr->intrp) {
      curr->time = pf_get_time_to_next_intr_event(curr->intrp);
   } 
   else if(curr->procp == NULL) {
     if (currcpu->idleevents == NULL) {
       currcpu->state = CPU_IDLE;
       currcpu->idlestart = simtime;
       return;
     }
     curr->time = currcpu->idleevents->time;
     currcpu->idleevents->time = simtime;
     currcpu->state = CPU_IDLE_WORK;
   } 
   else {
     curr->time = pf_get_time_to_next_process_event(curr->procp);
   }

   curr->time = simtime + (currcpu->scale * curr->time);
   addtointq((event *) curr);

   
   if(pf_printhack) {
     fprintf(outputfile, "Exited handle_cpu_event\n");
     fflush(outputfile);
   }
}


void pf_internal_event (event *curr)
{
   ASSERT(curr != NULL);

#ifdef DEBUG_PFSIM
   fprintf (outputfile, "*** %f: pf_internal_event entered with event type %d\n", curr->time, curr->type);
#endif
   ASSERT1(curr->type == CPU_EVENT, "curr->type", curr->type);
   pf_handle_cpu_event((cpu_event *) curr);
}


static void pf_cpu_resetstats()
{
   int i;

   for (i=0; i<numcpus; i++) {
      cpus[i].idletime = 0.0;
      cpus[i].falseidletime = 0.0;
      cpus[i].idleworktime = 0.0;
      cpus[i].idlestart = simtime;
      cpus[i].intrs = 0;
      cpus[i].iointrs = 0;
      cpus[i].clockintrs = 0;
      cpus[i].runintrtime = 0.0;
      cpus[i].runiointrtime = 0.0;
      cpus[i].runclockintrtime = 0.0;
      cpus[i].cswitches = 0;
      cpus[i].runswitchtime = 0.0;
   }
}


static void pf_cpu_initialize()
{
   int i;

   for (i=0; i<numcpus; i++) {
      cpus[i].scale = pfscale;
      cpus[i].idleevents = NULL;
      cpus[i].current = (cpu_event *) getfromextraq();
      cpus[i].current->type = CPU_EVENT;
      cpus[i].current->cpunum = i;
      cpus[i].current->procp = NULL;
      cpus[i].current->intrp = NULL;
   }
   pf_cpu_resetstats();
}


void pf_resetstats()
{
   pf_cpu_resetstats();
}


void pf_setcallbacks()
{
}


void pf_initialize (int seedvalue)
{
    int i = 0;
    process *procp = NULL;
    intr_event *intrp;

#ifdef DEBUG_PFSIM
    fprintf (outputfile, "*** %f: Entered pf_initialize - numcpus %d, pfscale %f\n", simtime, numcpus, pfscale );
#endif

    StaticAssert (sizeof(cpu_event) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(intend_event) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(cswitch_event) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(idleloop_event) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(sleep_event) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(wakeup_event) <= DISKSIM_EVENT_SIZE);

    stat_initialize(statdeffile, "Response time", &timecritrespstats);
    stat_initialize(statdeffile, "Response time", &timelimitrespstats);
    stat_initialize(statdeffile, "Response time", &timenoncritrespstats);
    pf_cpu_initialize();

    intrp = (intr_event *) getfromextraq();
    intrp->type = INTR_EVENT;
    intrp->time = PF_INTER_CLOCK_TIME;
    intrp->vector = CLOCK_INTERRUPT;
    intrp->eventlist = NULL;
    addtointq((event *) intrp);

    DISKSIM_srand48(seedvalue);
    procp = synthlist;
    while(procp) {
        if(procp->space) {
            synthio_initialize_generator(procp);
        }
        procp = procp->next;
    }

    // initial distribution of processes to cpus
    pf_dispatcher_init(synthlist);
    synthlist = NULL;

    for (i = 0; i < numcpus; i++) {
        fprintf (outputfile, "%f: pf_initialize  Kicking off cpu #%d\n", simtime, i);

        if(cpus[i].current->procp) {
            cpus[i].current->procp->active = 1;
            cpus[i].current->time = cpus[i].scale *
                    pf_get_time_to_next_process_event(cpus[i].current->procp);

            addtointq((event *)cpus[i].current);
            cpus[i].state = CPU_PROCESS;
            fprintf (outputfile, "%f: pf_initialize  Event occurs at time %f, type %s (%d), cpunum %d\n",
                    simtime,
                    cpus[i].current->time,
                    getEventString(cpus[i].current->type),
                    cpus[i].current->type,
                    cpus[i].current->cpunum
            );
        }
        else {
            cpus[i].state = CPU_IDLE;
        }
    }
}



int disksim_pf_loadparams(struct lp_block *b) {
  pf_info_t *pf_info;

  if(!disksim->pf_info) {
    pf_info = (pf_info_t *)DISKSIM_malloc (sizeof(pf_info_t));
    if(!pf_info) return 0;
    bzero ((char *)pf_info, sizeof(pf_info_t));
    disksim->pf_info = pf_info;
  }

/*    unparse_block(b, outputfile); */

  //#include "modules/disksim_pf_param.c"
  lp_loadparams(0, b, &disksim_pf_mod);

  return 1;
}


int disksim_pf_stats_loadparams(struct lp_block *b) {
  pf_info_t *pf_info;

  if(!disksim->pf_info) {
    pf_info = (pf_info_t *)DISKSIM_malloc (sizeof(pf_info_t));
    if(!pf_info) return 0;
    bzero ((char *)pf_info, sizeof(pf_info_t));
    disksim->pf_info = pf_info;
  }


  //#include "modules/disksim_pf_stats_param.c"
  lp_loadparams(0, b, &disksim_pf_stats_mod);


  return 1;
}



void pf_cleanstats()
{
   int i;
   intr_event *intrp;
   process *procp;

   for (i=0; i<numcpus; i++) {
      intrp = cpus[i].current->intrp;
      procp = cpus[i].current->procp;
      switch (cpus[i].state) {
         case CPU_IDLE:
                      cpus[i].idletime += simtime - cpus[i].idlestart;
                      cpus[i].falseidletime += pf_add_false_idle_time(simtime - cpus[i].idlestart);
                      break;
         case CPU_IDLE_WORK:
                      cpus[i].idleworktime += simtime - cpus[i].idleevents->time;
                      break;
         case CPU_PROCESS:
                      if (procp->idler == 0) {
                         lastuser = simtime;
                      }
                      procp->runtime += simtime - procp->eventlist->time;
                      break;
         case CPU_INTR:
                      intrp->runtime += simtime - intrp->eventlist->time;
                      break;
         default:
                      fprintf(stderr, "Unknown CPU state at pf_cleanstats - %d\n", cpus[i].state);
                      exit(1);
      }
   }
   procp = process_livelist;
   while (procp) {
      if (procp->stat == PROC_SLEEP) {
         procp->runsleep += simtime - procp->lastsleep;
         if (procp->iosleep) {
            procp->runiosleep += simtime - procp->lastsleep;
         }
      }
      procp = procp->livelist;
   }
}


static void pf_print_interrupt_stats (int startcpu, int stopcpu)
{
   int i;
   int iointrs = 0;
   int clockintrs = 0;
   double runiointr = 0.0;
   double runclockintr = 0.0;
   char cpustr[81];

   if (startcpu == stopcpu) {
      sprintf(cpustr, "CPU #%d ", startcpu);
   } else {
      sprintf(cpustr, "CPU ");
   }

   if (pf_print_intrstats == FALSE) {
      return;
   }

   for (i=startcpu; i<=stopcpu; i++) {
      iointrs += cpus[i].iointrs;
      clockintrs += cpus[i].clockintrs;
      runiointr += cpus[i].runiointrtime;
      runclockintr += cpus[i].runclockintrtime;
   }
   fprintf (outputfile, "%sNumber of IO interrupts:            %d\n", cpustr, iointrs);
   fprintf (outputfile, "%sTime spent in I/O interrupts:       %f\n", cpustr, runiointr);
   fprintf (outputfile, "%sNumber of clock interrupts:         %d\n", cpustr, clockintrs);
   fprintf (outputfile, "%sTime spent in clock interrupts:     %f\n", cpustr, runclockintr);
}


static void pf_print_sleep_stats()
{
   int sleeps = 0;
   int iosleeps = 0;
   double runsleep = 0.0;
   double runiosleep = 0.0;
   process *procp = process_livelist;

   if (pf_print_sleepstats == FALSE) {
      return;
   }

   while (procp) {
      sleeps += procp->sleeps;
      iosleeps += procp->iosleeps;
      runsleep += procp->runsleep;
      runiosleep += procp->runiosleep;
      procp = procp->livelist;
   }
   fprintf (outputfile, "Number of sleep events:     %d\n", sleeps);
   fprintf (outputfile, "Number of I/O sleep events: %d\n", iosleeps);
   fprintf (outputfile, "Average sleep time:         %f\n", (runsleep / (double) max(sleeps,1)));
   fprintf (outputfile, "Average I/O sleep time:     %f\n", (runiosleep / (double) max(iosleeps,1)));
}


static void pf_print_process_stats()
{
   int i = 0;
   process *procp = process_livelist;
   int proccnt = 0;
   double runtime = 0.0;
   double lasteventtime = 0.0;
   int ios = 0;
   int ioreads = 0;
   int cswitches = 0;
   int sleeps = 0;
   double runsleep = 0.0;
   int iosleeps = 0;
   double runiosleep = 0.0;
   double falseidletime = 0.0;
   statgen * readlimitstats[511];
   statgen * writelimitstats[511];
   statgen * readmissstats[511];
   statgen * writemissstats[511];
   char procstr[81];

   fprintf (outputfile, "\nPROCESS STATISTICS\n");
   while (procp) {
      if ((procp->runtime != 0.0) || (procp->cswitches)) {
         proccnt++;
      }
      procp = procp->livelist;
   }

   ASSERT(proccnt < 511);
   procp = process_livelist;
   i = 0;
   while (procp) {
      if ((procp->runtime != 0.0) || (procp->cswitches)) {
         runtime += procp->runtime;
         lasteventtime = max (procp->lasteventtime, lasteventtime);
         ios += procp->ios;
         ioreads += procp->ioreads;
         cswitches += procp->cswitches;
         sleeps += procp->sleeps;
         runsleep += procp->runsleep;
         iosleeps += procp->iosleeps;
         runiosleep += procp->runiosleep;
         falseidletime += procp->falseidletime;
         readlimitstats[i] = &procp->readtimelimitstats;
         writelimitstats[i] = &procp->writetimelimitstats;
         readmissstats[i] = &procp->readmisslimitstats;
         writemissstats[i] = &procp->writemisslimitstats;
         i++;
      }
      procp = procp->livelist;
   }

   fprintf(outputfile, "Process Total computation time:  %f\n", runtime);
   fprintf(outputfile, "Process Last event time:         %f\n", lasteventtime);
   fprintf(outputfile, "Process Number of I/O requests:  %d\n", ios);
   fprintf(outputfile, "Process Number of read requests: %d\n", ioreads);
   fprintf(outputfile, "Process Number of C-switches:    %d\n", cswitches);
   fprintf(outputfile, "Process Number of sleeps:        %d\n", sleeps);
   fprintf(outputfile, "Process Average sleep time:      %f\n", (runsleep / (double) max(sleeps,1)));
   fprintf(outputfile, "Process Number of I/O sleeps:    %d\n", iosleeps);
   fprintf(outputfile, "Process Average I/O sleep time:  %f\n", (runiosleep / (double) max(iosleeps,1)));
   fprintf(outputfile, "Process False idle time:         %f\n", falseidletime);
   fprintf(outputfile, "Process Read Time limits measured: %d\n", stat_get_count_set(readlimitstats, proccnt));
   stat_print_set(readlimitstats, proccnt, "Process Read ");
   fprintf(outputfile, "Process Write Time limits measured: %d\n", stat_get_count_set(writelimitstats, proccnt));
   stat_print_set(writelimitstats, proccnt, "Process Write ");
   fprintf(outputfile, "Process Read Time limits missed: %d\n", stat_get_count_set(readmissstats, proccnt));
   stat_print_set(readmissstats, proccnt, "Process Missed Read ");
   fprintf(outputfile, "Process Write Time limits missed: %d\n", stat_get_count_set(writemissstats, proccnt));
   stat_print_set(writemissstats, proccnt, "Process Missed Write ");

   if ((pf_print_perprocessstats == FALSE) || (proccnt <= 1)) {
      return;
   }

   procp = process_livelist;
   while (procp) {
      if ((procp->runtime == 0.0) && (procp->cswitches == 0)) {
         procp = procp->livelist;
         continue;
      }
      fprintf(outputfile, "\nProcess %d\n", procp->pid);
      fprintf(outputfile, "Process %d Total computation time:  %f\n", procp->pid, procp->runtime);
      fprintf(outputfile, "Process %d Last event time:         %f\n", procp->pid, procp->lasteventtime);
      fprintf(outputfile, "Process %d Number of I/O requests:  %d\n", procp->pid, procp->ios);
      fprintf(outputfile, "Process %d Number of read requests: %d\n", procp->pid, procp->ioreads);
      fprintf(outputfile, "Process %d Number of C-switches:    %d\n", procp->pid, procp->cswitches);
      fprintf(outputfile, "Process %d Number of sleeps:        %d\n", procp->pid, procp->sleeps);
      fprintf(outputfile, "Process %d Average sleep time:      %f\n", procp->pid, (procp->runsleep / (double) max(procp->sleeps,1)));
      fprintf(outputfile, "Process %d Number of I/O sleeps:    %d\n", procp->pid, procp->iosleeps);
      fprintf(outputfile, "Process %d Average I/O sleep time:  %f\n", procp->pid, (procp->runiosleep / (double) max(procp->iosleeps,1)));
      fprintf(outputfile, "Process %d False idle time:         %f\n", procp->pid, procp->falseidletime);
      fprintf(outputfile, "Process %d Read Time limits measured: %d\n", procp->pid, stat_get_count(&procp->readtimelimitstats));
      sprintf(procstr, "Process %d Read ", procp->pid);
      stat_print(&procp->readtimelimitstats, procstr);
      fprintf(outputfile, "Process %d Write Time limits measured: %d\n", procp->pid, stat_get_count(&procp->writetimelimitstats));
      sprintf(procstr, "Process %d Write ", procp->pid);
      stat_print(&procp->writetimelimitstats, procstr);
      fprintf(outputfile, "Process %d Read Time limits missed: %d\n", procp->pid, stat_get_count(&procp->readmisslimitstats));
      sprintf(procstr, "Process %d Missed Read ", procp->pid);
      stat_print(&procp->readmisslimitstats, procstr);
      fprintf(outputfile, "Process %d Write Time limits missed: %d\n", procp->pid, stat_get_count(&procp->writemisslimitstats));
      sprintf(procstr, "Process %d Missed Write ", procp->pid);
      stat_print(&procp->writemisslimitstats, procstr);
      procp = procp->livelist;
   }
   fprintf(outputfile, "\n");
}


static void pf_print_pf_stats (int startcpu, int stopcpu)
{
   int i;
   double idletime = 0.0;
   double falseidletime = 0.0;
   double idleworktime = 0.0;
   int cswitches = 0;
   int numintrs = 0;
   double runintr = 0.0;
   double runswitchtime = 0.0;
   int cpucnt;
   char cpustr[81];
   char cpustr2[81];

   if (startcpu == stopcpu) {
      sprintf(cpustr, "CPU #%d ", startcpu);
   } else {
      sprintf(cpustr, "CPU ");
   }

   cpucnt = stopcpu - startcpu + 1;
   for (i=startcpu; i<=stopcpu; i++) {
      idletime += cpus[i].idletime;
      falseidletime += cpus[i].falseidletime;
      idleworktime += cpus[i].idleworktime;
      cswitches += cpus[i].cswitches;
      runswitchtime += cpus[i].runswitchtime;
      numintrs += cpus[i].intrs;
      runintr += cpus[i].runintrtime;
   }
   fprintf(outputfile, "%sTotal idle milliseconds:      %f\n", cpustr, idletime);
   fprintf(outputfile, "%sIdle time per processor:      %f\n", cpustr, (idletime / (double) cpucnt));
   fprintf(outputfile, "%sPercentage idle cycles:       %f\n", cpustr, ((double) 100.0 * idletime / ((simtime - WARMUPTIME) * (double) cpucnt)));
   fprintf(outputfile, "%sTotal false idle ms:          %f\n", cpustr, falseidletime);
   fprintf(outputfile, "%sFalse idle time per CPU:      %f\n", cpustr, (falseidletime / (double) cpucnt));
   fprintf(outputfile, "%sPercentage false idle cycles: %f\n", cpustr, ((double) 100.0 * falseidletime / ((simtime - WARMUPTIME) * (double) cpucnt)));
   fprintf(outputfile, "%sTotal idle work ms:           %f\n", cpustr, idleworktime);
   fprintf(outputfile, "%sContext Switches: %d\n", cpustr, cswitches);
   fprintf(outputfile, "%sTime spent context switching: %f\n", cpustr, runswitchtime);
   fprintf(outputfile, "%sPercentage switching cycles:  %f\n", cpustr, ((double) 100.0 * runswitchtime / ((simtime - WARMUPTIME) * (double) cpucnt)));
   fprintf(outputfile, "%sNumber of interrupts: %d\n", cpustr, numintrs);
   fprintf(outputfile, "%sTotal time in interrupts: %.3f\n", cpustr, runintr);
   fprintf(outputfile, "%sPercentage interrupt cycles:  %f\n", cpustr, ((double) 100.0 * runintr / ((simtime - WARMUPTIME) * (double) cpucnt)));
   fprintf(outputfile, "%sTime-Critical request count:      %d\n", cpustr, stat_get_count(&timecritrespstats));
   sprintf(cpustr2, "%sTime-Critical ", cpustr);
   stat_print(&timecritrespstats, cpustr2);
   fprintf(outputfile, "%sTime-Limited request count:       %d\n", cpustr, stat_get_count(&timelimitrespstats));
   sprintf(cpustr2, "%sTime-Limited ", cpustr);
   stat_print(&timelimitrespstats, cpustr2);
   fprintf(outputfile, "%sTime-Noncritical request count:   %d\n", cpustr, stat_get_count(&timenoncritrespstats));
   sprintf(cpustr2, "%sTime-Noncritical ", cpustr);
   stat_print(&timenoncritrespstats, cpustr2);
}


static void pf_print_percpu_stats()
{
   int i;

   if ((numcpus == 1) || (pf_print_percpustats == FALSE)) {
      return;
   }

   fprintf (outputfile, "\nPER-CPU STATISTICS\n");

   for (i=0; i<numcpus; i++) {
      fprintf (outputfile, "\nCPU #%d\n\n", i);
      pf_print_pf_stats(i, i);
      pf_print_interrupt_stats(i, i);
   }
}


void pf_printstats()
{
   fprintf (outputfile, "\nPROCESS FLOW STATISTICS\n");
   fprintf (outputfile, "-----------------------\n\n");

   pf_print_pf_stats(0, (numcpus - 1));
   pf_print_interrupt_stats(0, (numcpus - 1));
   pf_print_sleep_stats();
   pf_print_percpu_stats();
   pf_print_process_stats();
/*
   fprintf (outputfile, "Time since last user process:  %.3f\n", ((simtime - warmuptime) - lastuser));
*/
}

// End of file

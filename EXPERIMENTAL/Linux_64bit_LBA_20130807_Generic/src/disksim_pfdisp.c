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
#include "disksim_pfsim.h"


static void pf_disp_put_at_end_of_queue(process *procp)
{
   process *tmp = pf_dispq;

   procp->link = NULL;
   if (tmp == NULL) {
      pf_dispq = procp;
   } 
   else {
     while (tmp->link) {
       tmp = tmp->link;
     }
     tmp->link = procp;
   }
}


void pf_disp_put_on_sleep_queue (process *procp)
{
   procp->next = sleepqueue;
   sleepqueue = procp;
}


process *pf_disp_get_from_sleep_queue (void *chan)
{
   process *tmp = sleepqueue;
   process *prev = tmp;

   if (tmp == NULL) {
      return(NULL);
   }
   if (tmp->chan == chan) {
      sleepqueue = tmp->next;
      tmp->next = NULL;
      return(tmp);
   }
   tmp = tmp->next;
   while (tmp) {
      if (tmp->chan == chan) {
         prev->next = tmp->next;
         tmp->next = NULL;
         return(tmp);
      }
      prev = tmp;
      tmp = tmp->next;
   }
   return(NULL);
}


#if 0
process *pf_disp_get_specific_from_sleep_queue (int pid)
{
   process *tmp = sleepqueue;
   process *prev = tmp;

   if (tmp == NULL) {
      return (NULL);
   }
   if (tmp->pid == pid) {
      sleepqueue = tmp->next;
      tmp->next = NULL;
      return(tmp);
   }
   tmp = tmp->next;
   while (tmp) {
      if (tmp->pid == pid) {
         prev->next = tmp->next;
         tmp->next = NULL;
         return(tmp);
      }
      prev = tmp;
      tmp = tmp->next;
   }
   return(NULL);
}
#endif


process *pf_dispatch (int cpunum)
{
   process *procp = pf_dispq;

   if (procp) {
      pf_dispq = procp->link;
      procp->runcpu = cpunum;
      procp->stat = PROC_ONPROC;

      //      fprintf (outputfile, "New process chosen for cpu %d is pid %d\n", 
      //	       cpunum, procp->pid);

   } 
   else {
     //     fprintf (outputfile, "CPU %d is going idle\n", cpunum);
     //     cpus[cpunum].state = CPU_IDLE;
     //     pf_idle_cpu_recheck_dispq(cpunum);
   }

   return(procp);
}


void pf_dispatcher_init (process *startprocs)
{
   process *procp;
   process *tmp = startprocs;
   int i;

   /*
    * fprintf (outputfile, "Entering pf_dispatcher_init - "
    * "numprocs %d, numcpus %d\n", numprocs, numcpus);
    */

   while (tmp) {
      procp = tmp;
      tmp = procp->next;

      procp->livelist = process_livelist;
      process_livelist = procp;

      // why this?
      procp->pid += numcpus;
      pf_disp_put_at_end_of_queue(procp);

      /*
       * fprintf (outputfile, "Sticking start processes"
       * " #%d in pf_dispq #%d\n", procp->pid, procp->pri);
       */
   }


   for(i = 0; i < numcpus; i++) {
      if(cpus[i].current->procp == NULL) {
         cpus[i].current->procp = pf_dispatch(i);
      }
   }
}


process * pf_disp_sleep (process *procp)
{
   procp->stat = PROC_SLEEP;
   pf_disp_put_on_sleep_queue(procp);
   
   return(pf_dispatch(procp->runcpu));
}


void pf_disp_wakeup (process *procp)
{
  int c;
  procp->stat = PROC_RUN;
  pf_disp_put_at_end_of_queue(procp);

/*    for(c = 0; c < numcpus; c++) { */
/*      if(cpus[c].state == CPU_IDLE) { */
/*        printf("pf_disp_wakeup() waking up cpu %d\n", c); */
/*        cpus[c].current->procp = pf_dispatch(c); */
/*        cpus[c].state = CPU_PROCESS; */
/*      } */
/*    } */

}


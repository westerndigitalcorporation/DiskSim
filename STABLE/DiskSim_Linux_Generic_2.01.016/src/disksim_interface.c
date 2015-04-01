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


/*
 *
 * This file offers a suggested interface between disksim and a full
 * system simulator.  Using this interface (which assumes only a few
 * characteristics of the system simulator), disksim will act as a
 * slave of the system simulator, providing disk request completion
 * indications in time for an interrupt to be generated in the system
 * simulation.  Specifically, disksim code will only be executed when
 * invoked by one of the procedures in this file.
 *
 * Using ths interface requires only two significant functionalities
 * of the system simulation environment:
 *
 * 1. the ability to function correctly without knowing when a disk
 * request will complete at the time that it is initiated.  Via the
 * code in this file, the system simulation will be informed at some
 * later point in its view of time that the request is completed.  (At
 * this time, the appropriate "disk request completion" interrupt
 * could be inserted into the system simulation.)
 *
 * 2. the ability for disksim to register a callback with the system
 * simulation environment.  That is, this client code must be able to
 * say, "please invoke this function when the simulated time reaches
 * X".  It is also helpful to be able to "deschedule" a callback at a
 * later time -- but lack of this support can certainly be worked
 * around.
 *
 * To use the external interface, your program should link with
 * libdisksim.a and disksim_interface.o.  See syssim.* for a good
 * example.
 *
 */


#include <unistd.h>

#include "disksim_global.h"
#include "disksim_ioface.h"
#include "disksim_interface.h"
#include "disksim_disk.h"

#include "config.h"

#include "disksim_interface_private.h"

/*
struct disksim_interface {
  struct disksim *disksim;
  disksim_interface_complete_t complete_fn;
  disksim_interface_sched_t sched_fn;
  disksim_interface_desched_t desched_fn;
  void *ctx;
};
*/


/* This is the disksim callback for reporting completion of a disk
 * request to the system-level simulation -- the system-level *
 * simulation should incorporate this completion as appropriate *
 * (probably by inserting a simulated "disk completion interrupt" at *
 * the specified simulated time).  Based on the requestdesc pointed to
 * * by "curr->buf" (below), the system-level simulation should be
 * able * to determine which request completed.  (A ptr to the
 * system-level * simulator's request structure is a reasonable use of
 * "curr->buf".)
 */
static event * 
disksim_interface_io_done_notify (ioreq_event *curr, void *ctx)
{
#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_io_done_notify  ctx %p, curr %p, time %f, type %d, devno %d, blkno %d, bcount %d, flags %X\n",
            simtime, ctx, curr, curr->time, curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush( outputfile );
#endif

  struct disksim_interface *iface = (struct disksim_interface *)ctx;
  struct disksim_request *req = (struct disksim_request *) curr->buf;

  // wrong ctx -- should be the per-req one ... how to demux?
  iface->complete_fn(simtime, req, iface->ctx);
  return 0;
}


static int
disksim_interface_initialize_latency (
        struct disksim_interface *iface,
        const char *pfile,
        const char *ofile,
        int latency_weight,
        char *paramval,
        char *paramname,
        int synthio,
        char *sched_alg,
        int over_argc,
        char **over_argv)
{
  char **argv;
  int argc = 6;
  int i = argc;

#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_initialize_latency\n", simtime );
    fflush( outputfile );
#endif

  argc += over_argc;
  
  // fprintf (stder, "disksim_initialize\n");
  
  disksim_initialize_disksim_structure(iface->disksim);

  if(latency_weight) {
    argc += 12;
  }

  argv = (char **)calloc(argc, sizeof(char *));

  if(latency_weight){
    //     iodriver_param_override(paramname,paramval,-1,-1);
    
    
    argv[i++] = "driver*";
    argv[i++] = paramname;
    argv[i++] = paramval;


    /* note: these parameter names must match those from
     * modules/disksim_ioqueue_param.h, etc 
     */
    
    argv[i++] = "driver*";
    argv[i++] = "Scheduler:Scheduling policy";
    argv[i++] = sched_alg;
    
    argv[i++] = "driver*";
    argv[i++] = "Scheduler:Scheduling priority scheme";
    argv[i++] = sched_alg;

    argv[i++] = "driver*";
    argv[i++] = "Scheduler:Timeout scheduling";
    argv[i++] = sched_alg;
  }

  memcpy(argv + i, over_argv, over_argc * sizeof(char *));

  argv[0] = "disksim";
  argv[1] = (char *) pfile;
  argv[2] = (char *) ofile;
  argv[3] = "external";
  argv[4] = "0";
  if(synthio == 1){
    argv[5] = "1";
  }else{
    argv[5] = "0";
  }
  disksim_setup_disksim (argc, argv);
  
  /* Note that this call must be redone anytime a disksim checkpoint is 
   * restored -- this prevents old function addresses from polluting    
   * the restored execution...                                          
   */
  disksim_set_external_io_done_notify(disksim_interface_io_done_notify);
  
  // fprintf (stder, "disksim_initialize done\n");
  
  return 0;
}

/* called once at simulation initialization time */

struct disksim_interface *
disksim_interface_initialize (
        const char *pfile,
        const char *ofile,
        disksim_interface_complete_t comp,
        disksim_interface_sched_t sched,
        disksim_interface_desched_t desched,
        void *ctx,
        int argc,
        char **argv)
{
  struct disksim_interface *iface = (struct disksim_interface *)calloc(1, sizeof(struct disksim_interface));
  ddbg_assert(iface != 0);

#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_initialize\n", simtime );
    fflush( outputfile );
#endif

  iface->complete_fn = comp;
  iface->sched_fn = sched;
  iface->desched_fn = desched;
  iface->ctx = ctx;
  iface->disksim = (disksim_t *)calloc(1, sizeof(struct disksim));

  disksim = iface->disksim;


  disksim_interface_initialize_latency(
          iface,
          pfile,
          ofile,
          0,         // latency_weight
          NULL,      // paramval
          NULL,      // paramname
          0,         // synthio
          NULL,      // sched_alg
          argc,
          argv);

  disksim->notify_ctx = iface;

  return iface;
}



/* called once at simulation shutdown time */

void 
disksim_interface_shutdown (
        struct disksim_interface *iface,
        double syssimtime)
{
   double curtime = syssimtime;

#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_shutdown\n", simtime );
    fflush( outputfile );
#endif

   disksim = iface->disksim;

   // fprintf (stderr, "disksim_shutdown\n");

   if ((curtime + 0.0001) < simtime) {
      fprintf (stderr, "external time is ahead of disksim time: %f > %f\n", curtime, simtime);
      exit(1);
   }

   simtime = curtime;
   disksim_printstats ();

   // fprintf (stderr, "disksim_shutdown done\n");
}


/* Prints the current disksim statistics.  This call does not reset the    */
/* statistics, so the simulation can continue to run and calculate running */
/* totals.  "syssimtime" should be the current simulated time of the       */
/* system-level simulation.                                                */

void 
disksim_interface_dump_stats (
        struct disksim_interface *iface,
        double syssimtime)
{
   double curtime = syssimtime;

#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_dump_stats\n", simtime );
    fflush( outputfile );
#endif

   disksim = iface->disksim;

   // fprintf (stderr, "disksim_dump_stats\n");

   if ((disksim->intq) && (disksim->intq->time < curtime) && ((disksim->intq->time + 0.0001) >= curtime)) {
      curtime = disksim->intq->time;
   }
   if (((curtime + 0.0001) < simtime) 
       || ((disksim->intq) 
	   && (disksim->intq->time < curtime))) 
   {
     fprintf (stderr, "external time is mismatched with disksim time: %f vs. %f (%f)\n", curtime, simtime, ((disksim->intq) ? disksim->intq->time : 0.0));
     exit (1);
   }

   simtime = curtime;
   disksim_cleanstats();
   disksim_printstats();

   // fprintf (stderr, "disksim_dump_stats done\n");
}


static int event_count = 0;

/* This is the callback for handling internal disksim events while running */
/* as a slave of a system-level simulation.  "syssimtime" should be the    */
/* current simulated time of the system-level simulation.                  */

void 
disksim_interface_internal_event (
        struct disksim_interface *iface,
        double syssimtime,
        void *junk)
{
   double curtime = syssimtime;
   disksim = iface->disksim;

#ifdef DEBUG_INTERFACE
   fprintf( outputfile, "****** %f: disksim_interface_internal_event: Entered  iface %p, disksim %p, syssimtime %f, junk %p\n", simtime, iface, disksim, syssimtime, junk );
   fflush(outputfile);
#endif

   /* if next event time is less than now, error.  Also, if no event is  */
   /* ready to be handled, then this is a spurious callback -- it should */
   /* not be possible with the descheduling below (allow it if it is not */
   /* possible to deschedule.                                            */

   if (disksim->intq != NULL 
       && (disksim->intq->time + 0.0001) < curtime) 
   {
     fprintf (stderr, "external time is ahead of disksim time: %f > %f\n", curtime, disksim->intq->time);
     exit (1);
   }

#ifdef DEBUG_INTERFACE
   fprintf( outputfile, "****** %lf: disksim_interface_internal_event: intq->time=%lf curtime=%lf\n", simtime, disksim->intq->time, curtime);
   fflush(outputfile);
#endif

   /* while next event time is same as now, handle next event */
   if(disksim->intq != NULL){
     ASSERT (disksim->intq->time >= simtime);
   }

   while ((disksim->intq != NULL) 
	  && (disksim->intq->time <= (curtime + 0.0001))) 
   {
     // fprintf (stderr, "handling internal event: type %d\n", disksim->intq->type);
     disksim_simulate_event(event_count++);
   }

   if (disksim->intq != NULL) {
      /* Note: this could be a dangerous operation when employing checkpoint */
      /* and, specifically, restore -- functions move around when programs   */
      /* are changed and recompiled...                                       */

      iface->sched_fn(disksim_interface_internal_event, 
		      disksim->intq->time,
		      iface->ctx);
   }

   // fprintf (stderr, "disksim_internal_event done\n");
}


/* This function should be called by the system-level simulation when    */
/* it wants to issue a request into disksim.  "syssimtime" should be the */
/* system-level simulation time at which the request should "arrive".    */
/* "requestdesc" is the system-level simulator's description of the      */
/* request (device number, block number, length, etc.).                  */

void 
disksim_interface_request_arrive (
        struct disksim_interface *iface,
        double syssimtime,
        struct disksim_request *requestdesc)
{
   ioreq_event *new_event;

#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_interface_request_arrive\n", simtime );
    fflush( outputfile );
#endif

   double curtime = syssimtime;
   disksim = iface->disksim;

   new_event = (ioreq_event *) getfromextraq();

   assert (new_event != NULL);
   new_event->type = IO_REQUEST_ARRIVE;
   new_event->time = curtime;
   new_event->busno = 0;
   new_event->devno = requestdesc->devno;
   new_event->blkno = requestdesc->blkno;
   new_event->flags = requestdesc->flags;
   new_event->bcount = requestdesc->bytecount;
   new_event->batchno = requestdesc->batchno;
   new_event->batch_complete = requestdesc->batch_complete;

   new_event->flags |= TIME_CRITICAL;

   new_event->cause = 0;
   new_event->opid = 0;
   new_event->buf = requestdesc;

   io_map_trace_request (new_event);

#ifdef DEBUG_INTERFACE
   fprintf( outputfile, "****** %f: disksim_interface_request_arrive  time %lf, type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, new_event->time, new_event->type, new_event->devno, new_event->blkno, new_event->bcount, new_event->flags  );
   fflush(outputfile);
#endif

   /* issue it into simulator */
   if (disksim->intq) {
     iface->desched_fn(0.0, iface->ctx);
   }
   addtointq ((event *)new_event);

   /* while next event time is same as now, handle next event */
   while ((disksim->intq != NULL) 
	  && (disksim->intq->time <= (curtime + 0.0001))) 
   {
     disksim_simulate_event (event_count++);
   }

   if (disksim->intq) {
      /* Note: this could be a dangerous operation when employing checkpoint */
      /* and, specifically, restore -- functions move around when programs   */
      /* are changed and recompiled...                                       */

      iface->sched_fn(disksim_interface_internal_event, 
		      disksim->intq->time,
		      iface->ctx);
   }
}


void disksim_free_disksim(struct disksim_interface *iface) {
#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_free_disksim\n", simtime );
    fflush( outputfile );
#endif

  disksim_cleanup();
  free(iface->disksim);
  free(iface);
  iface = NULL;
}

double disksim_time_to_msec(double x) { return x; }
double disksim_time_from_msec(double x) { return x; }


struct dm_disk_if *
disksim_getdiskmodel(struct disksim_interface *i, int disknum)
{
#ifdef DEBUG_INTERFACE
    fprintf( outputfile, "%f: disksim_getdiskmodel\n", simtime );
    fflush( outputfile );
#endif

    return i->disksim->diskinfo->disks[disknum]->model;
}

// End of file


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

#ifndef _DISKSIM_INTERFACE_H
#define _DISKSIM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

// public interface to libdisksim

#include "disksim_reqflags.h"

/* System level request */
struct disksim_request {
  int flags;
  short devno;
  unsigned long blkno;
  int bytecount;
  double start;
  int batchno;
  int batch_complete;
  void *reqctx;         /* context passed with the request */
  short completed;
};

struct disksim_interface;

// callback when requests complete
typedef void(*disksim_interface_complete_t)(double time,
					    struct disksim_request *,
					    void *ctx);

// scheduled callback function (below)
typedef void(*disksim_interface_callback_t)(struct disksim_interface *,
					    double t,
					    void *ctx);

// callback (disksim -> client) to register a callback (client ->
// disksim); when simulated time reaches t, client code should invoke
// fn.
typedef void(*disksim_interface_sched_t)(disksim_interface_callback_t fn,
					 double,
					 void *ctx);

// cancel the pending callback set for t
typedef void(*disksim_interface_desched_t)(double t, void *ctx);

// Allocate and initialize a disksim_interface handle.
// pfile is the name of disksim's parameter file.
// ofile is the name of disksim's output file.
// ctx is an arbitrary pointer which will be passed back to the callback
// functions when they are invoked.
struct disksim_interface * 
disksim_interface_initialize (const char *pfile, 
			      const char *ofile,
			      disksim_interface_complete_t,
			      disksim_interface_sched_t,
			      disksim_interface_desched_t,
			      void *ctx,
			      int argc,
			      char **argv);


void 
disksim_interface_shutdown (struct disksim_interface *,
			    double syssimtime);
void 
disksim_interface_dump_stats (struct disksim_interface *,
			      double syssimtime);

// pump disksim's event loop
// 3rd argument to make it match a type; ignored.
void 
disksim_interface_internal_event (struct disksim_interface *,
				  double t,
				  void *junk);

// inject a request into disksim
void
disksim_interface_request_arrive (struct disksim_interface *,
				  double syssimtime, 
				  struct disksim_request *requestdesc);

void 
disksim_free_disksim(struct disksim_interface *d);

// convert milliseconds to/from whatever the native disksim
// representation is
double disksim_time_to_msec(double);
double disksim_time_from_msec(double);

// this is ugly but not as ugly as some of the alternatives ...
struct dm_disk_if *
disksim_getdiskmodel(struct disksim_interface *, int);

#ifdef __cplusplus
}
#endif

#endif // _DISKSIM_INTERFACE_H

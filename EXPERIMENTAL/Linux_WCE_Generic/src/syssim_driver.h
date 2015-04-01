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


#include "disksim_interface.h"

/*
 * Definitions for simple system simulator that uses DiskSim as a slave.
 *
 * Contributed by Eran Gabber of Lucent Technologies - Bell Laboratories
 *
 */

typedef	double SysTime;		/* system time in seconds.usec */


/* routines for translating between the system-level simulation's simulated */
/* time format (whatever it is) and disksim's simulated time format (a      */
/* double, representing the number of milliseconds from the simulation's    */
/* initialization).                                                         */

/* In this example, system time is in seconds since initialization */
// #define SYSSIMTIME_TO_MS(syssimtime)    (syssimtime*1e3)
// #define MS_TO_SYSSIMTIME(curtime)       (curtime/1e3)




/* exported by syssim_driver.c */
void syssim_schedule_callback(disksim_interface_callback_t, SysTime t, void *);
void syssim_report_completion(SysTime t, struct disksim_request *r, void *);
void syssim_deschedule_callback(double, void *);






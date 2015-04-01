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


#ifndef _MEMS_PIECEWISE_SEEK_H_
#define _MEMS_PIECEWISE_SEEK_H_

#define _VERBOSE_ 0

#define NUM_CHUNKS 8  // number of chunks used to calculate the seek time
#define SWITCH_POINT (2 * NUM_CHUNKS / 2)  // index of the switch point

#define MEMS_SWITCH_ITERATIONS 9  // number of times to iterate to find the optimal switchpoint
#define MEMS_SHIFT_PERCENTAGE 0.05  // fraction of the seek distance by which to adjust the switch point

#define INWARD 1
#define OUTWARD 2

#define OVERALL_POSITIVE 1
#define OVERALL_NEGATIVE 2

double
find_dist_nm(double start_offset, double end_offset);

double
find_seek_time_piecewise(double start_offset_nm, double end_offset_nm,
			 double spring_factor, double acceleration_nm_s_s,
			 double length_nm, double velocity_nm_s);

double find_dist_nm(double start_offset, double end_offset);
#endif

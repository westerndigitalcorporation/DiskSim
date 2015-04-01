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

#include <math.h>

#include "mems_global.h"
#include "mems_piecewise_seek.h"

/*  this allows the output mechanism to be defined at compile time so
 *  that you can compile the seek code on its own for testing.  otherwise,
 *  all logging is done to the disksim output file
 */

#ifdef __SEPERATE__
#define __OUTPUTFILE__  stderr
#else
#define __OUTPUTFILE__  outputfile
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *  These are helper functions for the piecewise-linear
 *  seek time model.
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_dist_nm()
 *
 *  returns the distance in nanometers from start_offset
 *  to end_offset.  remember that these are _offsets_ from
 *  the center of the sled.
 *
 *  inputs:
 *    mems_sled_t *sled - a pointer to the sled structure
 *    coord_t *start - the starting coordinate
 *    coord_t *end - the destination coordinate
 *
 *  outputs:
 *    double - the distance in nm from start_offset to
 *             end_offset
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
find_dist_nm(double start_offset, double end_offset) {

  double dist_nm;

  if (((start_offset > 0.0) && (end_offset > 0.0)) ||
      ((start_offset < 0.0) && (end_offset < 0.0))) {
    /*  we're on the same side of the sled  */
    dist_nm = fabs(end_offset - start_offset);
  } else {
    dist_nm = fabs(start_offset) + fabs(end_offset);
  }

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "find_dist_nm::  start_offset = %f, end_offset = %f, dist_nm = %f\n",
	    start_offset, end_offset, dist_nm);
  }

  return dist_nm;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_local_actuator_direction()
 *
 *  returns the direction which the actuators are pulling
 *  the sled, INWARD or OUTWARD.  this is used to determine
 *  if the actuators are helping the seek or hurting the
 *  seek.  see below for more.
 *
 *  inputs:
 *    double start_offset_nm - the start offset of the seek
 *    double end_offset_nm - the end offset of the seek
 *    int overall_actuator_direction - the direction the
 *                         actuators are acting.
 *
 *  outputs:
 *    int - INWARD if the actuators are pulling the sled
 *          toward the center, OUTWARD if the actuators are
 *          pulling toward the edge.
 *
 *  modifies:
 *    nothing
 *
 *  what's really going on here:
 *
 *    okay, what's the difference between INWARD, OUTWARD, OVERALL_POSITIVE,
 *  OVERALL_NEGATIVE, etc, and what is this function for anyway?  second question
 *  first:  given start and end offsets, this fuction tells you if the actuators
 *  are pulling the sled toward the rest position (INWARD) or away from the rest
 *  position (OUTWARD).  for example, if the overall_actuator_direction is
 *  OVERALL_POSITIVE, and the seek is entirely on the positive half of the square,
 *  then the actuators are pulling the sled OUTWARD.  when the seek crosses the
 *  center line, then the function decides based the larger portion of the seek.
 *  i think that answers the second question, too.  it's used to determine when
 *  the springs will help the seek and when they'll hurt.
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int find_local_actuator_direction(double start_offset_nm, double end_offset_nm,
				  int overall_actuator_direction) {

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  start_offset_nm = %f, end_offset_nm = %f\n",
	    start_offset_nm, end_offset_nm);
    
    if (overall_actuator_direction == OVERALL_POSITIVE)
      fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  overall_actuator_direction = OVERALL_POSITIVE\n");
    else
      fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  overall_actuator_direction = OVERALL_NEGATIVE\n");
  }

  if ((start_offset_nm <= 0.0) && (end_offset_nm <= 0.0)) {
    /*  if the seek is on the negative side  */
    if (overall_actuator_direction == OVERALL_POSITIVE) {
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is INWARD (1)\n");
      return INWARD;
    } else {
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is OUTWARD (2)\n");
      return OUTWARD;
    }
  } else if ((start_offset_nm >= 0.0) && (end_offset_nm >= 0.0)) {
    /*  if the seek is on the positive side  */
    if (overall_actuator_direction == OVERALL_POSITIVE) {
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is OUTWARD (3)\n");
      return OUTWARD;
    } else {
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is OUTWARD (4)\n");
      return INWARD;
    }
  } else {
    /*  the seek crosses the centerline  */
    if (fabs(start_offset_nm) > fabs(end_offset_nm)) {
      /*  i move more toward the center than away  */
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is INWARD (5)\n");
      return INWARD;
    } else {
      if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_local_actuator_direction::  local_actuator_direction is OUTWARD (6)\n");
      return OUTWARD;
    }
  }
}  

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  adjust_accel()
 *
 *  returns the acceleration in the region given by the
 *  start and end points.  the acceleration at the two points
 *  is the sum of the actuator acceleration and the spring
 *  acceleration, which is given by
 *
 *      [ actuator_accel *  offset  ]
 *      [                  -------- ]  *  spring_factor
 *      [                  length/2 ] 
 *
 *  the springs' restoring force is then a function of the
 *  displacement (offset) and a fraction (spring_factor) of
 *  the actuator force.  put another way, if the spring_factor
 *  is 0.10, then at the maximum offset the force from the springs
 *  will be equal to 10% of the force from the actuators.
 *
 *  the returned adjusted acceleration is the average of the
 *  sums at the start and end points.
 *
 *  inputs:
 *    double accel - the base acceleration from the actuators
 *    double spring_factor - the spring factor
 *    double offset_1 - the offset of the starting point
 *    double offset_2 - the offset of the ending point
 *    double length - the length of the sled
 *    int overall_actuator_direction - INWARD if the actuators
 *          are pulling the sled toward the center, OUTWARD otherwise.
 *
 *  outputs:
 *    double - the adjusted acceleration for the region between
 *             the start and end offsets, given the base actuator
 *             acceleration, the spring_factor, the length of the
 *             sled, and the overall actuator direction
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double adjust_accel(double accel, double spring_factor,
		    double offset_1, double offset_2,
		    double length, int overall_actuator_direction) {

  double adjusted_accel;

  if (find_local_actuator_direction(offset_1, offset_2, overall_actuator_direction) == INWARD) {
    /*  springs help  */
    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "adjust_accel::  springs helped!\n");
  } else {
    /*  springs hurt  */
    spring_factor = -spring_factor;
    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "adjust_accel::  springs hurt!\n");
  }

  adjusted_accel =
    (
     (accel
      +
      (accel * (spring_factor * (fabs(offset_1) / (length / 2))))
      )
     +
     (accel
      +
      (accel * (spring_factor * (fabs(offset_2) / (length / 2))))
      )
     ) / 2.0;

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "adjust_accel::  offset_1 = %f, offset_2 = %f, accel = %f\n",
	    offset_1, offset_2, accel);
    fprintf(__OUTPUTFILE__, "adjust_accel::  spring_factor = %f, length = %f\n",
	    spring_factor, length);
    fprintf(__OUTPUTFILE__, "adjust_accel::  adjusted_accel = %f, diff = %f\n",
	    adjusted_accel, ((adjusted_accel / accel) * 100.0) - 100.0);
  }

  return adjusted_accel;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_switch_offset_from_dist()
 *
 *  computes the switchpoint offset between start_offset_nm
 *  and end_offset_nm.  this finds the offset which is the
 *  midpoint between the two points.
 *
 *  inputs:
 *    double start_offset_nm - the starting offset
 *    double end_offset_nm - the end offset
 *    double dist_nm - the distance in nm between the two points
 *    int direction - the direction of the seek
 *
 *  outputs:
 *    double - the offset of the midpoint in nm
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
find_switch_offset_from_dist (double start_offset_nm, double end_offset_nm,
			      double dist_nm, int direction) {

  double switch_offset_nm;

  if (direction == OVERALL_POSITIVE) {
    switch_offset_nm = start_offset_nm + (dist_nm / 2);
  } else {
    switch_offset_nm = start_offset_nm - (dist_nm / 2);
  }

  return switch_offset_nm;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_overall_seek_direction()
 *
 *  finds the direction of the seek - OVERALL_POSITIVE
 *  or OVERALL_NEGATIVE
 *
 *  inputs:
 *    double start_offset_nm - the starting offset
 *    double end_offset_nm - the end offset
 *
 *  outputs:
 *    int - OVERALL_POSITIVE if the seek is in the positive
 *          direction, OVERALL_NEGATIVE if negative.
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
find_overall_seek_direction(double start_offset_nm, double end_offset_nm) {
  
  if (end_offset_nm > start_offset_nm) {
    return OVERALL_POSITIVE;
  } else {
    return OVERALL_NEGATIVE;
  }

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_seek_time_piecewise()
 *
 *  this is the guts of the seek code.  find_seek_time_piecewise()
 *  returns the time to seek from start_offset_nm to end_offset_nm
 *  given the various parameters.  it assumes that the
 *  initial velocity is equal to the final velocity
 *  (i.e. there are no turnarounds necessary).  it uses an
 *  iterative algorithm to find the optimal switchpoint,
 *  attempting to balance the energy in the acceleration
 *  phase with the energy in the deceleration phase.
 *
 *  please see below for a complete explanation of how it works.
 *
 *  inputs:
 *    double start_offset_nm - the starting offset
 *    double end_offset_nm - the end offset
 *    double spring_factor - spring factor of the system
 *    double acceleration_nm_s_s - actuator acceleration
 *    double length_nm - the length of the sled
 *    double velocity_nm_s - the initial and final velocity
 *
 *  outputs:
 *    double - the seek time from start to end
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
find_seek_time_piecewise(double start_offset_nm, double end_offset_nm,
			 double spring_factor, double acceleration_nm_s_s,
			 double length_nm, double velocity_nm_s) {

  double dist_nm;                      /*  distance of this seek  */
  double phase1_dist_nm;               /*  distance of the first phase  */
  double phase2_dist_nm;               /*  distance of the second phase  */
  double phase1_chunk_length_nm;       /*  length of chunks in the first phase  */
  double phase2_chunk_length_nm;       /*  length of chunks in the second phase  */
  double phase1_acceleration_nm_s_s;   /*  actuator acceleration in the first phase  */
  double phase2_acceleration_nm_s_s;   /*  actuator acceleration in the second phase  */
  double x[2 * NUM_CHUNKS + 1];        /*  position values for each chunk of the seek  */
  double v[2 * NUM_CHUNKS + 1];        /*  velocity values for each chunk of the seek  */
  double a[2 * NUM_CHUNKS + 1];        /*  adjusted acceleration values for each chunk  */
  double t[2 * NUM_CHUNKS + 1];        /*  times at each x point  */

  double x_switch, v_switch, a_switch, t_switch;  /*  position, velocity, acceleration, and time values for the switch point  */
  double v_diff, x_delta;

  double min_v_diff;
  double min_v_diff_time = 0.0;

  double total_seek_time;

  int i, jj;
  int overall_seek_direction;
  int phase1_actuator_direction, phase2_actuator_direction;

  min_v_diff = 666.0;  /*  this should be bigger than 100 percent  */
  
  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "find_seek_time_piecewise:: ** entry - start_offset_nm = %f, end_offset_nm = %f\n",
	    start_offset_nm, end_offset_nm);
  }

  /*  find seek distance and overall direction  */
  dist_nm = find_dist_nm(start_offset_nm, end_offset_nm);
  overall_seek_direction = find_overall_seek_direction(start_offset_nm, end_offset_nm);

  /*  set initial x position and set switch point to be midpoint of seek  */
  x[0] = start_offset_nm;
  x[SWITCH_POINT] = find_switch_offset_from_dist(start_offset_nm, end_offset_nm,
						 dist_nm, overall_seek_direction);

  /*  this is the loop which iteratively tries to find the optimal switch point.  during each iteration
      the loop does the following:

      find the x values for each chunk.
      find the adjusted acceleration at each point.
      start at the start point and find the v and t values
         for each point in the first phase, including the switch point.
      start at the end point and find the v and t values
         for each point in the second phase, again including the switch point.
      
      if the switch point we picked initially were the optimal one, then the
      velocity at the switch point from each phase should be equal, by conservation
      of energy.  if they're not, then we need to adjust the switch point toward the
      end that had more energy, i.e. had the highest velocity at the switch point.
      the loop below iterates a fixed number of times, adjusting the switch point toward
      the start or end point by some percentage of the seek distance.

      at compile time, you could lower the number of MEMS_SWITCH_ITERATIONS to reduce the runtime, at the
      cost of accuracy.  if the phases of the calculation are unbalanced, then the sled will not be at the
      correct velocity when it arrives at the destination.  you could also reduce the number of chunks each
      phase is broken into.  this leads to discontinuities in the seek curve.  this is due to the way it
      decides when the springs switch from helping the seek to hurting the seek.  when a chunk straddles the
      middle of the square, the springs will be changing from helping to hurting.  however, we can only choose
      one or the other for each chunk, so for part of it the spring factor will be incorrectly determined.
      the smaller the chunks are, the less significant this inaccuracy becomes.
  */
  
  for (jj = 1; jj < MEMS_SWITCH_ITERATIONS; jj++) {
    
    /*  find the length of the two phases  */
    phase1_dist_nm = find_dist_nm(start_offset_nm, x[SWITCH_POINT]);
    phase2_dist_nm = find_dist_nm(x[SWITCH_POINT], end_offset_nm);

    /*  find the length of the chunks  */
    phase1_chunk_length_nm = phase1_dist_nm / NUM_CHUNKS;
    phase2_chunk_length_nm = phase2_dist_nm / NUM_CHUNKS;

    if (_VERBOSE_) {
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  phase1_dist_nm = %f, phase1_chunk_length_nm = %f\n",
	      phase1_dist_nm, phase1_chunk_length_nm);
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  phase2_dist_nm = %f, phase2_chunk_length_nm = %f\n",
	      phase2_dist_nm, phase2_chunk_length_nm);
    }

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** figuring out phase 1 x values\n");

    /*  find the endpoints of all the chunks of the first phase  */
    for (i = 1; i < SWITCH_POINT; i++) {
      if (overall_seek_direction == OVERALL_POSITIVE)
	x[i] = x[i-1] + phase1_chunk_length_nm;
      else
	x[i] = x[i-1] - phase1_chunk_length_nm;
    }

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** figuring out phase 2 x values\n");

    /*  find the endpoints of all the chunks of the second phase  */
    for (i = SWITCH_POINT + 1; i <= 2*NUM_CHUNKS; i++) {
      if (overall_seek_direction == OVERALL_POSITIVE)
	x[i] = x[i-1] + phase2_chunk_length_nm;
      else
	x[i] = x[i-1] - phase2_chunk_length_nm;
    }  

    /*  the absolute value of the actuator acceleration is equal for both phases...  */
    phase1_acceleration_nm_s_s = acceleration_nm_s_s;
    phase2_acceleration_nm_s_s = acceleration_nm_s_s;

    /*  ...but the directions will be different  */
    if (overall_seek_direction == OVERALL_POSITIVE) {
      phase1_actuator_direction = OVERALL_POSITIVE;
      phase2_actuator_direction = OVERALL_NEGATIVE;
    } else {
      phase1_actuator_direction = OVERALL_NEGATIVE;
      phase2_actuator_direction = OVERALL_POSITIVE;
    }

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** adjusting phase 1 accelerations\n");

    /*  find the acceleration in each chunk of the first phase.  this takes into account the adjustment
	for the spring forces.  */
    for (i = 1; i <= SWITCH_POINT; i++) {
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  i = %d, x[i-1] = %f, x[i] = %f\n",
		i, x[i-1], x[i]);
      }
      a[i] = adjust_accel(phase1_acceleration_nm_s_s, spring_factor,
			  x[i-1], x[i],
			  length_nm, phase1_actuator_direction);
    }

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** adjusting phase 2 accelerations\n");

    /*  find the acceleration in each chunk of the second phase.  this takes into account the adjustment
	for the spring forces.  */
    for (i = SWITCH_POINT + 1; i <= 2*NUM_CHUNKS; i++) {
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  i = %d, x[i-1] = %f, x[i] = %f\n",
		i, x[i-1], x[i]);
      }
      a[i] = adjust_accel(phase2_acceleration_nm_s_s, spring_factor,
			  x[i-1], x[i],
			  length_nm, phase2_actuator_direction);
    }

    /*  start at time 0.0 and with the correct initial velocity  */
    t[0] = 0.0;
    v[0] = velocity_nm_s;

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** finding phase 1 t's and v's\n");

    /*  find the time and velocity at each point in the first phase  */
    for (i = 1; i <= SWITCH_POINT; i++) {
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  v[i-1] = %f, a[i] = %f, t[i-1] = %f, x[i-1] = %f, x[i] = %f\n",
		v[i-1], a[i], t[i-1], x[i-1], x[i]);
      }
      t[i] = (
	      -(v[i-1] - (a[i] * t[i-1]))
	      + sqrt(
		     (v[i-1] * v[i-1]) + ((2.0 * a[i]) * fabs((x[i] - x[i-1])))
		     )
	      ) / (a[i]);
    
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  t[%d] = %f\n", i, t[i]);
      }

      v[i] = (
	      v[i-1] + (a[i] * (t[i] - t[i-1]))
	      );
    
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  v[%d] = %f\n", i, v[i]);    
      }
    }

    /*  now we've found half of the seek time  */
    total_seek_time = t[SWITCH_POINT];

    /*  save the values at the switch point since they're about to be over-written by
	the calculation for the second phase  */
    x_switch = x[SWITCH_POINT];
    v_switch = v[SWITCH_POINT];
    a_switch = a[SWITCH_POINT];
    t_switch = t[SWITCH_POINT];

    /*  start the second phase at time 0.0 and at the initial velocity  */
    t[2*NUM_CHUNKS] = 0.0;
    v[2*NUM_CHUNKS] = velocity_nm_s;

    if (_VERBOSE_) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  ** finding phase 2 t's and v's\n");

    /*  find the time and velocity at each point in the second phase  */
    for (i = 2*NUM_CHUNKS - 1; i >= SWITCH_POINT; i--) {
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  v[i+1] = %f, a[i] = %f, t[i+1] = %f, x[i+1] = %f, x[i] = %f\n",
		v[i+1], a[i], t[i+1], x[i+1], x[i]);
      }
      t[i] = (
	      -(v[i+1] - (a[i] * t[i+1]))
	      + sqrt(
		     (v[i+1] * v[i+1]) + ((2.0 * a[i]) * fabs((x[i+1] - x[i])))
		     )
	      ) / (a[i]);
    
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  t[%d] = %f\n", i, t[i]);
      }

      v[i] = (
	      v[i+1] + (a[i] * (t[i] - t[i+1]))
	      );

      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  v[%d] = %f\n", i, v[i]);
      }
    }

    /*  now we've found the total seek time  */
    total_seek_time += t[SWITCH_POINT];    

    /*  print out all of the x, t, a, and v values that we've found  */
    if (_VERBOSE_) {
      for (i = 0; i <= 2 * NUM_CHUNKS; i++) {
	if (i == SWITCH_POINT) {
	  fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  x[%d] = %f, a[%d] = %f, v[%d] = %f, t[%d] = %f  ** SWITCH_POINT **\n",
		  i, x[i], i, a[i], i, v[i], i, t[i]);
	} else {
	  fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  x[%d] = %f, a[%d] = %f, v[%d] = %f, t[%d] = %f\n",
		  i, x[i], i, a[i], i, v[i], i, t[i]);
	}
      }

      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  total_seek_time = %f\n", fabs(total_seek_time));
    
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  x_switch = %f, x[SWITCH_POINT] = %f, diff = %f\n",
	      x_switch, x[SWITCH_POINT], ((x_switch / x[SWITCH_POINT]) * 100.0) - 100.0);
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  v_switch = %f, v[SWITCH_POINT] = %f, diff = %f\n",
	      v_switch, v[SWITCH_POINT], ((v_switch / v[SWITCH_POINT]) * 100.0) - 100.0);
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  a_switch = %f, a[SWITCH_POINT] = %f, diff = %f\n",
	      a_switch, a[SWITCH_POINT], ((a_switch / a[SWITCH_POINT]) * 100.0) - 100.0);
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  t_switch = %f, t[SWITCH_POINT] = %f, diff = %f\n",
	      t_switch, t[SWITCH_POINT], ((t_switch / t[SWITCH_POINT]) * 100.0) - 100.0);
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  trying to find new switch point.  jj = %d\n", jj);
    } 

    /*  look at the difference in the velocities calculated in the two phases  */
    v_diff = ((v_switch / v[SWITCH_POINT]) * 100.0) - 100.0;

    /*  if we're not taking the springs into account, then we don't need to iterate anymore.  */
    if (spring_factor == 0.0) {
      min_v_diff_time = total_seek_time;
      break;
    } else {
      /*  otherwise, come up with the amount by which we want to shift the switchpoint - a percentage
          of the seek distance minus a diminishing function of the iteration number  */
      x_delta = (MEMS_SHIFT_PERCENTAGE - (1.0 / ((MEMS_SWITCH_ITERATIONS - jj) * 100.0)));
    }

    if (v_switch > v[SWITCH_POINT]) {
      /*  the first phase ended up with more energy  */
      if (overall_seek_direction == OVERALL_POSITIVE) {
	x_delta = -x_delta;
      }      
    } else {
      if (overall_seek_direction == OVERALL_NEGATIVE) {
	x_delta = -x_delta;
      }      
    }

    /*  adjust the switch point  */
    x[SWITCH_POINT] += x_delta * dist_nm;
  
    if (_VERBOSE_) {
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  shifting x_switch. x_delta = %f, x_delta_nm = %f, v_diff = %f\n",
	      x_delta, (x_delta * dist_nm), v_diff);
    
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  x_switch = %f, x[SWITCH_POINT] = %f, diff = %f\n",
	      x_switch, x[SWITCH_POINT], ((x_switch / x[SWITCH_POINT]) * 100.0) - 100.0);
    }

    x[0] = start_offset_nm;

    if (_VERBOSE_) {
      fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  min_v_diff = %f, v_diff = %f\n",
	      min_v_diff, v_diff);
    }

    /*  save the minimum velocity difference and the seek time which got me that v_diff  */
    if (fabs(v_diff) < min_v_diff) {
      min_v_diff = fabs(v_diff);
      min_v_diff_time = total_seek_time;
      if (_VERBOSE_) {
	fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  updating min_v_diff\n");
	if (min_v_diff == 0.0) fprintf(__OUTPUTFILE__, "find_seek_time_piecewise::  augh!!!  min_v_diff is zero!\n");
      }
    }
  }

  /*  return the seek time from the iteration which gave me the smallest v_diff  */
  return fabs(min_v_diff_time);
}

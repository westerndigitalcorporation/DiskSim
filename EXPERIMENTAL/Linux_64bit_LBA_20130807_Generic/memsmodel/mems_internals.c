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


#include "mems_global.h"
#include "mems_internals.h"

#include "mems_piecewise_seek.h"
#include "mems_hong_seek.h"

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
 *
 *  mems_get_current_position()
 *
 *  returns the current position of the sled specified
 *  by the sled pointer.
 *
 *  inputs:
 *    mems_sled *sled - pointer to the sled structure
 *
 *  outputs:
 *    coord_t * - a pointer to the coord_t in the sled structure
 *
 *  modifies:
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

coord_t *
mems_get_current_position(mems_sled_t *sled) {

  //  ASSERT(pos != NULL);

  // pos = &sled->pos;
  return (&sled->pos);

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_equal_coords()
 *
 *  returns TRUE if the two coordinates are equal
 *
 *  inputs:
 *    coord_t *one - first coordinate
 *    coord_t *two - second coordinate
 *
 *  outputs:
 *    int - TRUE if they are equal, false if they are not
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
mems_equal_coords(coord_t *one, coord_t *two) {
  
  if ((one->x_pos == two->x_pos) && (one->y_pos == two->y_pos) && (one->y_vel == two->y_vel)) {
    return TRUE;
  } else {
    return FALSE;
  }

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_equal_coord_sets()
 *
 *  returns TRUE if the two coordinate sets are equal
 *
 *  inputs:
 *    tipsector_coord_set_t *one - first coordinate set
 *    tipsector_coord_set_t *two - second coordinate set
 *
 *  outputs:
 *    int - TRUE if they are equal, false if they are not
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int 
mems_equal_coord_sets (tipsector_coord_set_t *one, tipsector_coord_set_t *two)
{
  return (mems_equal_coords(&one->servo_start, &two->servo_start) &&
	  mems_equal_coords(&one->tipsector_start, &two->tipsector_start) &&
	  mems_equal_coords(&one->tipsector_end, &two->tipsector_end));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_media_access_time()
 *
 *  returns the time it takes to access the media between
 *  two coordinates.  this is called to find out how long
 *  a read or write will take.  it assumes that the device
 *  can read and write at the same rate.
 *
 *  inputs:
 *    mems_sled *sled - a pointer to the sled structure
 *    coord_t *start - the starting coordinate
 *    coord_t *end - the destination coordinate
 *
 *  outputs:
 *    double - the amount of time in milliseconds to access
 *             data between the two points.
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
mems_media_access_time (mems_sled_t *sled, coord_t *start, coord_t *end)
{
  double time;		/* seconds */
  double velocity;	/* nanometers per second */
  double distance;	/* nanometers */

  velocity = (double)(sled->y_access_speed_bit_s * sled->bit_length_nm);
  distance = (fabs ((double)(start->y_pos - end->y_pos))) * sled->bit_length_nm;

  time = distance / velocity;

  /* convert to milliseconds */
  time *= 1000.0;

  /*
  fprintf (stderr, "access: time = %f ms, dist = %f nm, vel = %f nm/s\n", time, distance, velocity);
  */

  return (time);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_coord_t_copy()
 *
 *  copies one coord_t into another
 *
 *  inputs:
 *    coord_t *source - coordinate to be copied
 *    coord_t *dest - coordinate to be copied into
 *
 *  outputs:
 *    none
 *
 *  modifies:
 *    dest
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mems_coord_t_copy (coord_t *source, coord_t *dest) {
  
  dest->x_pos = source->x_pos;
  dest->y_pos = source->y_pos;
  dest->y_vel = source->y_vel;

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_coord_t_copy()
 *
 *  copies one tipsector_coord_set_t into another
 *
 *  inputs:
 *    tipsector_coord_set_t *source - coordinate set to be copied
 *    tipsector_coord_set_t *dest - coordinate set to be copied into
 *
 *  outputs:
 *    none
 *
 *  modifies:
 *    dest
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mems_tipsector_coord_set_t_copy (tipsector_coord_set_t *source, tipsector_coord_set_t *dest) {

  mems_coord_t_copy(&source->servo_start, &dest->servo_start);
  mems_coord_t_copy(&source->tipsector_start, &dest->tipsector_start);
  mems_coord_t_copy(&source->tipsector_end, &dest->tipsector_end);

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_commit_move()
 *
 *  actually records a new position in the sled structure.
 *  this allows the upper levels of the simulator to call
 *  the seek time calcluation functions without changing
 *  the sled state.
 *
 *  inputs:
 *    mems_sled *sled - pointer to the sled structure
 *    coord_t *pos - pointer to the updated coordinate
 *
 *  outputs:
 *    none
 *
 *  modifies:
 *    sled->pos - updates sled->pos to be the new coordinate
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mems_commit_move(mems_sled_t *sled,
		  coord_t *pos) {

  mems_coord_t_copy(pos, &sled->pos);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_settle_time()
 *
 *  finds the settle time, which is given thusly:
 *
 *  settle time = num_time_constants * (1 / (2 pi * resonant frequency))
 *
 *  in this version of the model, the settle time is _not_
 *  dependent on the seek distance or the spring_factor
 *
 *  inputs:
 *    double num_time_constants - the number of time constants to
 *           add to a seek
 *    double resonant_frequency - resonant frequency of the sled
 *
 *  outputs:
 *    double - the settle time
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
find_settle_time(double num_time_constants, double resonant_frequency) {

  return (num_time_constants * (1.0 / ((6.2831853) * (resonant_frequency))));

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  find_turnaround_time()
 *
 *  finds the turnaround time.  this is a function of the sled velocity,
 *  the actuator acceleration and the spring forces.  that makes it also
 *  a function of position (offset).
 *
 *  inputs:
 *    double offset_nm - the offset at which the turnaround occurs
 *    double velocity_nm_s - the sled velocity
 *    double base_accel_nm_s_s - the actuator acceleration
 *    double spring_factor - the spring_factor
 *    double length_nm - the length of the square
 *
 *  outputs:
 *    double - the turnaround time
 *
 *  modifies:
 *    nothing
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
find_turnaround_time(double offset_nm, double velocity_nm_s, double base_accel_nm_s_s,
		     double spring_factor, double length_nm) {
  
  double turnaround_time;
  double spring_accel;
  
  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "find_turnaround_time::  entry\n");
    
    fprintf(__OUTPUTFILE__, "find_turnaround_time::  offset_nm = %f, length_nm = %f\n",
	    offset_nm, length_nm);
  }

  /*  scale the base acceleration by the spring_factor and the offset  */
  spring_accel = (base_accel_nm_s_s * (spring_factor * (fabs(offset_nm) / (length_nm / 2.0))));
  
  if ( ((offset_nm < 0.0) && (velocity_nm_s > 0.0)) ||
       ((offset_nm > 0.0) && (velocity_nm_s < 0.0))
       )
    {
      /*  the springs will hurt my acceleration  */
      spring_accel = -spring_accel;
    }

  /*  turnaround time is the time to stop the sled plus the time to accelerate it back to
      velocity.  these are equal, so it's just twice the time to stop.  */
  turnaround_time = 2.0 * (fabs(velocity_nm_s)) / (base_accel_nm_s_s + spring_accel);

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "find_turnaround_time::  velocity_nm_s = %f, base_accel_nm_s_s = %f, spring_accel = %f\n",
	    velocity_nm_s, base_accel_nm_s_s, spring_accel);
    
    fprintf(__OUTPUTFILE__, "find_turnaround_time::  turnaround_time = %f\n", turnaround_time);
  }

  return turnaround_time;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *
 *  mems_seek_time()
 *
 *  this function finds the seek time from point to point,
 *  taking into account all turnarounds that might have to
 *  take place.  it also returns some useful statistics
 *  that may be recorded by the calling function.
 *
 *  inputs:
 *    mems_sled *sled - a pointer to the sled structure
 *    coord_t *begin - the beginning coordinate
 *    coord_t *end - the end coordinate
 *    double *return_x_seek_time - a pointer into which to return the x seek time
 *    double *return_y_seek_time - a pointer into which to return the y seek time
 *    double *return_turnaround_time - a pointer into which to return the turnaround time
 *    double *return_turnaround_number - a pointer into which to return the number of turnarounds
 *
 *  outputs:
 *    double - the total seek time
 *
 *  modifies:
 *    return_x_seek_time
 *    return_y_seek_time
 *    return_turnaround_time
 *    return_turnaround_number
 *
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
mems_seek_time (mems_sled_t *sled,
		coord_t *begin, coord_t *end,
		double *return_x_seek_time, double *return_y_seek_time,
		double *return_turnaround_time, int *return_turnaround_number)
{
  double seek_time_x=0.0;	/* second */
  double seek_time_y=0.0;	/* second */
  double dist_x_nm=0.0;		
  double dist_y_nm=0.0;		
  int dist_y_equiv_nm=0.0;	/* used for equivalent distance calc */
  double x_accel=0.0;           /* nanometer/sec/sec */
  double y_accel=0.0;		/* nanometer/sec/sec */
  double bit_width=0.0;		/* nanometer */
  double turnaround_time=0.0;	/* second */
  double settling_time_x=0.0;   /* second */
  int y_access_speed_nm_s;
  double total_time;		/* second */

  int number_of_turns;

  double spring_factor = 0.0;
  int x_length_nm = 0;
  int y_length_nm = 0;

  /* sled = &dev->sled[sledno]; */
  
  /*
  fprintf(__OUTPUTFILE__, "\nmemsdevice_seek_time::  begin->x_pos = %d, begin->y_pos = %d, begin->y_vel = %d\n",
	  begin->x_pos, begin->y_pos, begin->y_vel);
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  end->x_pos   = %d, end->y_pos = %d, end->y_vel = %d\n",
	  end->x_pos, end->y_pos, end->y_vel);
  */

  /*  first check to see if begin == end  */
  if (mems_equal_coords(begin, end)) {
    //    fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  begin == end!\n");
    // fprintf(stderr, "memsdevice_seek_time::  begin == end!\n");

    if (return_turnaround_time != NULL) {
      *return_turnaround_time = 0.0;
    }
    
    if (return_turnaround_number != NULL) {
      *return_turnaround_number = 0;
    }
    
    if (return_x_seek_time != NULL) {
      *return_x_seek_time = 0.0;
    }
    
    if (return_y_seek_time != NULL) {
      *return_y_seek_time = 0.0;
    }

    return 0.0;
  }

  /*  initialize some variables to values given in the sled structure  */
  
  x_accel = (double)sled->x_accel_nm_s2;
  y_accel = (double)sled->y_accel_nm_s2;
  bit_width = (double)sled->bit_length_nm;
  y_access_speed_nm_s = sled->y_access_speed_bit_s * sled->bit_length_nm;
  x_length_nm = sled->x_length_nm;
  y_length_nm = sled->y_length_nm;
  spring_factor = sled->spring_factor;

  /*
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  begin->x_pos = %d, bit_width = %f, x_begin_nm = %f\n",
	  begin->x_pos, bit_width, x_begin_nm);
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  end->x_pos = %d, bit_width = %f, x_end_nm = %f\n",
	  end->x_pos, bit_width, x_end_nm);
  */

  if (end->x_pos == begin->x_pos) {
    dist_x_nm = (double)0.0;
    seek_time_x = (double)0.0;
  } else {

    /*  find the x settle time  */
    settling_time_x = find_settle_time(sled->num_time_constants, sled->sled_resonant_freq_hz);

    /*
    fprintf(__OUTPUTFILE__, "settling_time_x = %f\n", settling_time_x);
    */

    /*
    fprintf(__OUTPUTFILE__, "memsdevice_seek_time:: find_seek_time_piecewise - x_begin_nm = %f, x_end_nm = %f\n",
	    x_begin_nm, x_end_nm);
    */

    /*  find the base seek time in x  */

    if (sled->dev->seek_function == MEMS_SEEK_HONG) {
      seek_time_x = find_seek_time_hong_x((begin->x_pos * bit_width),
					  (end->x_pos * bit_width),
					  spring_factor, x_accel,
					  x_length_nm)
	+
	settling_time_x;
    } else {
      if (sled->dev->precompute_seek_count > 0) {
	seek_time_x =
	  mems_find_precomputed_seek_time(sled,
					  (begin->x_pos * bit_width), (end->x_pos * bit_width),
					  _X_SEEK_)
	  +
	  settling_time_x;
      } else {
	seek_time_x =
	  find_seek_time_piecewise((begin->x_pos * bit_width), (end->x_pos * bit_width),
				   spring_factor, x_accel,
				   x_length_nm, 0.0)
	  +
	  settling_time_x;
      }
    }
  }
  /* Now calculate seek time in Y direction.
   * First, general values needed in the calculations.
   *
   * there are eight cases to account for:
   *
   * begin = current velocity; end = new velocity;
   * below = new position BELOW current position;
   * above = new position ABOVE current position
   *
   *	begin UP	end UP		below	(1) turn, seek, turn
   *					above	(2) seek
   *
   *			end DOWN	below	(3) turn, seek
   *					above	(4) seek, turn
   *
   *	begin DOWN	end UP		below	(5) seek, turn
   *					above	(6) turn, seek
   *
   *			end DOWN	below	(7) seek
   *					above	(8) turn, seek, turn
   */

  /*
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  begin->y_vel = %d, end->y_vel = %d\n",
	  begin->y_vel, end->y_vel);

  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  begin->y_pos = %d, end->y1_pos = %d\n",
	  begin->y_pos, end->y1_pos);

  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  end->y1_pos = %d, end->y2_pos = %d\n",
	  end->y1_pos, end->y2_pos);
  */

  turnaround_time = 0.0;
  number_of_turns = 0;

  if (begin->y_vel >= 0) {
    if (end->y_vel > 0) {	/* final direction == begining direction */
      if (begin->y_pos >= end->y_pos) {
	/* case (1) */

	turnaround_time = find_turnaround_time((begin->y_pos * bit_width),
					       begin->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	turnaround_time += find_turnaround_time((end->y_pos * bit_width),
						end->y_vel,
						y_accel,
						spring_factor, y_length_nm);
	number_of_turns = 2;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 1 - turnaround_time = %f\n",
		turnaround_time);
	*/
      } else {					/* no need to turn around */
	/* case (2) */

	number_of_turns = 0;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 2 - turnaround_time = %f\n",
		turnaround_time);
	*/
      }
    } else {			/* final direction != begining direction */
      if (begin->y_pos >= end->y_pos) {
	/* case (3) */

	turnaround_time = find_turnaround_time((begin->y_pos * bit_width),
					       begin->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	number_of_turns = 1;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 3 - turnaround_time = %f\n",
		turnaround_time);
	*/
      } else {
	/* case (4) */

        turnaround_time = find_turnaround_time((end->y_pos * bit_width),
					       end->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	number_of_turns = 1;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 4 - turnaround_time = %f\n",
		turnaround_time);
	*/
      }
    }
  } else {	/* begin.y_vel < 0.0 */
    if (end->y_vel > 0) {	/* final direction != begining direction */
      if (begin->y_pos >= end->y_pos) {
	/* case (5) */

	turnaround_time = find_turnaround_time((end->y_pos * bit_width),
					       end->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	number_of_turns = 1;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 5 - turnaround_time = %f\n",
		turnaround_time);
	*/
      } else {
	/* case (6) */
	turnaround_time = find_turnaround_time((begin->y_pos * bit_width),
					       begin->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	number_of_turns = 1;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 6 - turnaround_time = %f\n",
		turnaround_time);
	*/
      }
    } else {			/* final direction == begining direction */
      if (begin->y_pos >= end->y_pos) {	/* no need to turn around */
	/* case (7) */

	number_of_turns = 0;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 7 - turnaround_time = %f\n",
		turnaround_time);
	*/
	dist_y_equiv_nm = 0;
      } else {
	/* case (8) */

	turnaround_time = find_turnaround_time((begin->y_pos * bit_width),
					       begin->y_vel,
					       y_accel,
					       spring_factor, y_length_nm);
	turnaround_time += find_turnaround_time((end->y_pos * bit_width),
						end->y_vel,
						y_accel,
						spring_factor, y_length_nm);
	number_of_turns = 2;

	/*
	fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  finding turnarounds - case 8 - turnaround_time = %f\n",
		turnaround_time);
	*/
      }
    }
  }
  
  dist_y_nm = (double)abs(end->y_pos - begin->y_pos) * sled->bit_length_nm;
  
  /*
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  begin->y_pos = %d, bit_width = %f, y_begin_nm = %f\n",
	  begin->y_pos, bit_width, y_begin_nm);
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time::  end->y1_pos = %d, bit_width = %f, y_end_nm = %f\n",
	  end->y1_pos, bit_width, y_end_nm);
  
  fprintf(__OUTPUTFILE__, "memsdevice_seek_time:: find_seek_time_piecewise - y_begin_nm = %f, y_end_nm = %f\n",
	  y_begin_nm, y_end_nm);
  */

  if (sled->dev->seek_function == MEMS_SEEK_HONG) {
    seek_time_y =
      find_seek_time_hong_y((begin->y_pos * bit_width),
			    (end->y_pos * bit_width),
			    spring_factor, y_accel,
			    y_length_nm, y_access_speed_nm_s)
      +
      turnaround_time;
  } else {
    if (sled->dev->precompute_seek_count > 0) {
      seek_time_y =
	mems_find_precomputed_seek_time(sled,
					(begin->y_pos * bit_width), (end->y_pos * bit_width),
					_Y_SEEK_)
	+
	turnaround_time;
    } else {
      seek_time_y =
	find_seek_time_piecewise((begin->y_pos * bit_width), (end->y_pos * bit_width),
				 spring_factor, y_accel,
				 y_length_nm, y_access_speed_nm_s)
	+ turnaround_time;
    }
  }

  /*
  if (distance != NULL) {
    if (distance_direction == 1) {
      *distance = (int)sqrt(dist_x_nm * dist_x_nm + ((dist_y_nm + dist_y_equiv_nm) * (dist_y_nm + dist_y_equiv_nm)));
    } else {
      *distance = (int)sqrt(dist_x_nm * dist_x_nm + dist_y_nm * dist_y_nm);
    }
    // fprintf (__OUTPUTFILE__, "dist_x = %f, dist_y = %f; dist = %d\n", dist_x_nm, dist_y_nm, *distance);
  }
  */

  if (return_turnaround_time != NULL) {
    *return_turnaround_time = (turnaround_time) * 1000.0;
  }

  if (return_turnaround_number != NULL) {
    *return_turnaround_number = number_of_turns;
  }

  if (return_x_seek_time != NULL) {
    *return_x_seek_time = seek_time_x * 1000.0;
  }

  if (return_y_seek_time != NULL) {
    *return_y_seek_time = seek_time_y * 1000.0;
  }

  /*
  fprintf (stderr, "turnaround_time = %f, number_of_turns = %d\n",
	   (turnaround_time * 1000.0), number_of_turns);
  */

  if (_VERBOSE_) {
    printf("dist_x = %f; dist_y = %f; seek_time_x = %f; seek_time_y = %f\n",
	   dist_x_nm, dist_y_nm, seek_time_x, seek_time_y);
  }

  /*  total seek time is the max of the seek time in the two dimensions  */
  total_time = max(seek_time_x, seek_time_y);

  /*  convert to milliseconds  */
  return (total_time * (double)1000.0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *  This implements the seek cache.
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
mems_seek_time_seekcache (mems_sled_t *sled,
			  coord_t *begin, 
			  coord_t *end,
			  double *return_x_seek_time,
			  double *return_y_seek_time,
			  double *return_turnaround_time,
			  int *return_turnaround_number)
{
  struct mems_seekcache *s;
  int i;

  /* First check if value is cached */
  for (i = 0; i < MEMS_SEEKCACHE; i++) {
    s = &sled->seekcache[i];
    if (mems_equal_coords(begin, &s->begin) &&
	mems_equal_coords(end, &s->end)) {
      if (return_x_seek_time) *return_x_seek_time = s->x_seek_time;
      if (return_y_seek_time) *return_y_seek_time = s->y_seek_time;
      if (return_turnaround_time) *return_turnaround_time = s->turnaround_time;
      if (return_turnaround_number) *return_turnaround_number = s->turnaround_number;
      return s->time;
    }
  }

  s = &sled->seekcache[sled->seekcache_next];
  mems_coord_t_copy(begin, &s->begin);
  mems_coord_t_copy(end, &s->end);
  s->time = mems_seek_time(sled, &s->begin, &s->end, &s->x_seek_time, &s->y_seek_time, &s->turnaround_time, &s->turnaround_number);
  if (return_x_seek_time) *return_x_seek_time = s->x_seek_time;
  if (return_y_seek_time) *return_y_seek_time = s->y_seek_time;
  if (return_turnaround_time) *return_turnaround_time = s->turnaround_time;
  if (return_turnaround_number) *return_turnaround_number = s->turnaround_number;
  sled->seekcache_next++;
  if (sled->seekcache_next == MEMS_SEEKCACHE) sled->seekcache_next = 0;
  // fprintf(stderr, "mems_seek_time_seekcache::  s->time = %f\n", s->time);
  return s->time;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 *  These implement the pre-computed seek functions
 *-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
mems_print_precomputed_seek_times(mems_t *dev) {

  int i;

  for (i = 0; i <= dev->precompute_seek_count; i++) {
    printf("i = %d, distance = %d, x_seek = %f, y_seek = %f\n",
	   i, dev->precompute_seek_distances[i], dev->precompute_x_seek_times[i], dev->precompute_y_seek_times[i]);
  }

}

double
mems_find_precomputed_seek_time(mems_sled_t *sled,
				double start_offset_nm, double end_offset_nm,
				int direction) {

  double seektime = 0.0;
  int dist_nm;
  int i;    
  mems_t *dev = sled->dev;

  dist_nm = (int)find_dist_nm(start_offset_nm, end_offset_nm);

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "mems_find_precomputed_seek_time::  entry - start_offset_nm = %f, end_offset_nm = %f\n",
	    start_offset_nm, end_offset_nm);
    fprintf(__OUTPUTFILE__, "mems_find_precomputed_seek_time::  dist_nm = %d\n",
	    dist_nm);
  }

  // fprintf(stderr, "dist_nm = %d\n", dist_nm);
  
  // mems_print_precomputed_seek_times(dev);

  if (dist_nm) {
    for (i=0; i <= dev->precompute_seek_count; i++) {
      /*
      printf("i = %d, dist = %d, x_seek = %f, y_seek = %f\n",
	     i, dev->precompute_seek_distances[i], dev->precompute_x_seek_times[i], dev->precompute_y_seek_times[i]);
      */
      if (dist_nm <= dev->precompute_seek_distances[i]) {
	if (dist_nm == dev->precompute_seek_distances[i]) {
	  // fprintf(stderr, "found precomputed seek distance %d\n", dist_nm);
	  seektime = (direction == _X_SEEK_) ?
	    dev->precompute_x_seek_times[i] :
	    dev->precompute_y_seek_times[i];
	} else {
	  double ddiff =
	    (double) (dist_nm - dev->precompute_seek_distances[(i-1)])
	    /
	    (double) (dev->precompute_seek_distances[i] - dev->precompute_seek_distances[(i-1)]);
	  /*
	  fprintf(stderr, "did not find precompute seek distance %d\n", dist_nm);
	  fprintf(stderr, "ddiff = %f\n", ddiff);
	  fprintf(stderr, "i = %d\n", i);
	  */
	  if (direction == _X_SEEK_) {
	    seektime = dev->precompute_x_seek_times[(i-1)];
	    // fprintf(stderr, "seektime = %f\n", seektime);	    
	    seektime += ddiff * (dev->precompute_x_seek_times[i] - dev->precompute_x_seek_times[(i-1)]);
	    /*
	    fprintf(stderr, "times[i] = %f, times[i-1] = %f\n",
		    dev->precompute_x_seek_times[i], dev->precompute_x_seek_times[(i-1)]);
	    fprintf(stderr, "seektime = %f\n", seektime);
	    */
	  } else {
	    seektime = dev->precompute_y_seek_times[(i-1)];
	    // fprintf(stderr, "seektime = %f\n", seektime);
	    seektime += ddiff * (dev->precompute_y_seek_times[i] - dev->precompute_y_seek_times[(i-1)]);
	    /*
	    fprintf(stderr, "times[i] = %f, times[i-1] = %f\n",
		    dev->precompute_y_seek_times[i], dev->precompute_y_seek_times[(i-1)]);
	    fprintf(stderr, "seektime = %f\n", seektime);
	    */
	  }
	}
	break;
      }
    }
    if (seektime == 0.0) {
      fprintf(stderr, "Seek distance exceeds precompute seek curve: %d\n", dist_nm);
      exit(0);
    }
  }

  if (_VERBOSE_) {
    fprintf(__OUTPUTFILE__, "mems_find_precompute_seek_time::  finished - seektime = %f\n",
	    seektime);
  }

  // fprintf(stderr, "seektime / 1000.0 = %f\n", (seektime / 1000.0));

  return(seektime / 1000.0);
}

void
mems_precompute_seek_curve(mems_t *dev) {

  coord_t begin, end;

  double x_seek_time, y_seek_time;
  int sled_length_bits;
  int offset;
  int bit_step;
  int nm_step;
  int i;
  double num_time_constants = dev->sled[0].num_time_constants;
  int precompute_seek_count = dev->precompute_seek_count;

  dev->sled[0].num_time_constants = 0.0;
  dev->precompute_seek_count = 0;

  sled_length_bits = (int) (dev->sled[0].x_length_nm / dev->sled[0].bit_length_nm);
  bit_step = sled_length_bits / precompute_seek_count;
  nm_step = (sled_length_bits * dev->sled[0].bit_length_nm) / precompute_seek_count;

  dev->precompute_seek_distances = (int *)malloc((precompute_seek_count + 1) * sizeof(int));
  dev->precompute_x_seek_times = (double *)malloc((precompute_seek_count + 1) * sizeof(double));
  dev->precompute_y_seek_times = (double *)malloc((precompute_seek_count + 1) * sizeof(double));

  /*
  printf("precompute_seek_count = %d, bit_step = %d, nm_step = %d\n",
	 precompute_seek_count, bit_step, nm_step);
  */

  //  for(offset = 0; offset <= sled_length_bits; offset += bit_step) {
  for(i = 0; i < precompute_seek_count; i++) {
    
    offset = i * bit_step;

    // if (verbose) fprintf(stdout, "main:  starting a new seek (monkey)\n");

    begin.x_pos = 0;
    begin.y_pos = 0;
    begin.y_vel = 0;

    end.x_pos = offset;
    end.y_pos = 0;
    end.y_vel = 0;
    
    /*
    printf("begin = %d, end = %d\n",
	   begin.y_vel, end.y_vel);
    */

    x_seek_time = mems_seek_time(&(dev->sled[0]),
				  &begin, &end,
				  NULL, NULL,
				  NULL, NULL);

    begin.x_pos = 0;
    begin.y_pos = 0;
    begin.y_vel = dev->sled[0].y_access_speed_bit_s;
    
    end.x_pos = 0;
    end.y_pos = offset;
    end.y_vel = dev->sled[0].y_access_speed_bit_s;

    /*
    printf("begin = %d, end = %d\n",
	   begin.y_pos, end.y_pos);
    */

    y_seek_time = mems_seek_time(&(dev->sled[0]),
				  &begin, &end,
				  NULL, NULL,
				  NULL, NULL);

    /*
    fprintf(stdout, "%d, %d, %d, %f, %f\n",
	    i, (i * nm_step), (offset * dev->sled[0].bit_length_nm), x_seek_time, y_seek_time);
    */

    dev->precompute_seek_distances[i] = offset * dev->sled[0].bit_length_nm;
    dev->precompute_x_seek_times[i]   = x_seek_time;
    dev->precompute_y_seek_times[i]   = y_seek_time;

    /*
    fprintf(stdout, "%d, %d, %f, %f\n",
	    i, (offset * dev->sled[0].bit_length_nm),
	    dev->precompute_x_seek_times[i],
	    dev->precompute_y_seek_times[i]);
    */

  }

  begin.x_pos = 0;
  begin.y_pos = 0;
  begin.y_vel = 0;
  
  end.x_pos = sled_length_bits;
  end.y_pos = 0;
  end.y_vel = 0;

  x_seek_time = mems_seek_time(&(dev->sled[0]),
				&begin, &end,
				NULL, NULL,
				NULL, NULL);
  
  begin.x_pos = 0;
  begin.y_pos = 0;
  begin.y_vel = dev->sled[0].y_access_speed_bit_s;
  
  end.x_pos = 0;
  end.y_pos = sled_length_bits;
  end.y_vel = dev->sled[0].y_access_speed_bit_s;
  
  y_seek_time = mems_seek_time(&(dev->sled[0]),
				&begin, &end,
				NULL, NULL,
				NULL, NULL);
  
  dev->precompute_seek_distances[precompute_seek_count] = sled_length_bits * dev->sled[0].bit_length_nm;
  dev->precompute_x_seek_times[precompute_seek_count]   = x_seek_time;
  dev->precompute_y_seek_times[precompute_seek_count]   = y_seek_time;

  /*
  fprintf(stdout, "%d, %d, %d, %f, %f\n",
	  precompute_seek_count, (precompute_seek_count * nm_step), sled_length_bits * dev->sled[0].bit_length_nm, x_seek_time, y_seek_time);
  */

  dev->sled[0].num_time_constants = num_time_constants;
  dev->precompute_seek_count = precompute_seek_count;

}

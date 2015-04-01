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


#ifndef _MEMS_INTERNALS_H_
#define _MEMS_INTERNALS_H_

#include "mems_global.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Internal Data Types  -=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct mems_internal_statistics {
  int i;  /*  this is bogus as well...  */
} mems_internal_statistics_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Constants and macros  -=-=-=-=-=-=-=-=-=-=-=-=-*/

#define X_OFFSET(_X_)  ((double)((_X_) - (x_length_nm / 2.0)))
#define Y_OFFSET(_Y_)  ((double)((_Y_) - (y_length_nm / 2.0)))

#define _X_SEEK_ 0
#define _Y_SEEK_ 1

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Function Prototypes  -=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * mems_equal_coords()
 *
 * returns TRUE if the two coords are equal
 *
 * inputs:  coord_t *one - pointer to the first coordinate
 *          coord_t *two - pointer to the second coordinate
 *
 * outputs: TRUE if equal, FALSE if not
 *
 * modifies:  nothing
 *
 */

int mems_equal_coords(coord_t *one, coord_t *two);

/*
 * mems_equal_coord_sets()
 *
 * returns TRUE if the two coord sets are equal
 *
 * inputs:  tipsector_coord_set_t *one - pointer to first coord_set
 *          tipsector_coord_set_t *two - pointer to second coord_set
 *
 * outputs: TRUE if equal [NOTE: exactly equal! not reversed], FALSE if not
 *
 * modifies:  nothing
 */

int mems_equal_coord_sets(tipsector_coord_set_t *one, tipsector_coord_set_t *two);

/*
 * mems_coord_t_copy()
 *
 * copies a coord_t source into dest
 *
 * inputs:  coord_t *source - pointer to the source coordinate structure
 *          coord_t *dest   - pointer to the destination coordinate structure
 *
 * outputs: none
 *
 * modifies:  dest
 *
 */

void mems_coord_t_copy(coord_t *source, coord_t *dest);

/*
 * mems_tipsector_coord_set_t_copy()
 *
 * copies an entire tipsector_coord_set_t into another
 *
 * inputs:  tipsector_coord_set_t *source - pointer to the source structure
 *          tipsector_coord_set_t *dest   - pointer to the destination structure
 *
 * outputs: none
 *
 * modifies:  dest
 *
 */

void mems_tipsector_coord_set_t_copy (tipsector_coord_set_t *source, 
				      tipsector_coord_set_t *dest);

/*
 * mems_get_current_position()
 *
 * returns the coordinate of the current sled position
 *
 * inputs:  msled_t *sled - a pointer to the sled structure
 *
 * outputs: coord_t *pos - a pointer to the coord_t structure
 *
 * modifies:  none
 *
 */

coord_t *mems_get_current_position(mems_sled_t *sled);


/*
 * mems_seek_time()
 *
 * returns the seek time from start to end.  also returns some statistics
 *
 * inputs:  msled_t *sled - a pointer to the sled structure
 *          coord_t *start, *end - pointers to the start and end coordinates
 *          double *x_seek_time, *y_seek_time - pointers to statistics values to update
 *          double *turnaround_time - pointer into which to return the total turnaround time
 *          double *turnaround_number - pointer into which to return the number of turnarounds
 *
 * outputs: double - the total seek time
 * 
 * modifies: x_seek_time - if not NULL, returns the seek time in the X dimension
 *           y_seek_time - if not NULL, returns the seek time in the Y dimension
 *           turnaround_time - if not NULL, returns the turnaround time
 *           turnaround_number - if not NULL, returns the number of turnarounds
 *
 */

double mems_seek_time(mems_sled_t *sled,
		      coord_t *begin, coord_t *end,
		      double *return_x_seek_time, double *return_y_seek_time,
		      double *return_turnaround_time, int *return_turnaround_number);

/* mems_seek_time_seekcache()
 *
 * exactly the same as mems_seek_time(), except checks a local result cache
 * before doing the expensive calculations
 */

double mems_seek_time_seekcache (mems_sled_t *sled,
				 coord_t *begin, 
				 coord_t *end,
				 double *return_x_seek_time,
				 double *return_y_seek_time,
				 double *return_turnaround_time,
				 int *return_turnaround_number);


double
mems_find_precomputed_seek_time(mems_sled_t *sled,
				double start_offset_nm, double end_offset_nm,
				int direction);

/*
 * mems_media_access_time()
 *
 * returns the time from start to end at media access speed
 *
 * inputs:  msled_t *sled - a pointer to the sled structure
 *          coord_t *start - a pointer to the start coordinate
 *          coord_t *end - a pointer to the end coordinate
 *
 * outputs: double - the total media access time
 *
 * modifies: none
 */

double mems_media_access_time(mems_sled_t *sled,
			      coord_t *start, coord_t *end);

double find_turnaround_time(double offset_nm, double velocity_nm_s, double base_accel_nm_s_s,
			    double spring_factor, double length_nm);

/*
 * mems_commit_move()
 *
 * update the position in the sled structure with the position in pos
 *
 * inputs:  msled_t *sled - a pointer to the sled structure
 *          coord_t *pos - the position coordinate to update the sled with
 *
 * outputs: none
 *
 * modifies:  sled - with new position specified in pos
 *
 */

void mems_commit_move(mems_sled_t *sled,
		      coord_t *pos);

void mems_precompute_seek_curve(mems_t *dev);

double mems_find_precomputed_seek_time(mems_sled_t *sled,
				       double start_offset_nm,
				       double end_offset_nm,
				       int direction);
#endif

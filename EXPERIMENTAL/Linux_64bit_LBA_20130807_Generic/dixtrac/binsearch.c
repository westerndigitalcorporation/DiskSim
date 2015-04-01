/*
 * DIXtrac - Automated Disk Extraction Tool
 * Authors: Jiri Schindler, John Bucy, Greg Ganger
 *
 * Copyright (c) of Carnegie Mellon University, 1999-2005.
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

#include "binsearch.h"

/*---------------------------------------------------------------------------*
 * Performs one iteration of a binary search algorithm and returns 1 if it   *
 * converged. If it converge returns 0 to indicate that binary search should *
 * continue. 'condition' specifies what constitutes a succesful test,'value' *
 * is the current solution, 'low' and 'high' are the lower and upper bound   *
 * for 'value', 'maxvalhit' is used for determining adoptive behavior, and   *
 * 'last_high' is the previous 'high' value for an unsucessful condition.    *
 *---------------------------------------------------------------------------*/
int
bin_search(int condition,
	   int *value,
	   int *low,
	   int *high,
	   int *last_high,
	   int *maxvalhit)
{
  /* this test is used for detecting adaptive behavior, i.e. cases for */
  /* which binary search does not work */
  if (condition && (*maxvalhit < *value))
    *maxvalhit = *value;

  if (*low < *high) {
    if (condition) {
      /* hit */
      *low = *value;
      if (*last_high == -1)
	*value *= 1.5;
      else 
	*value = *last_high;
      *high = *value;
    }
    else {
      *last_high = *high;
      *value = *low + ((*high - *low)/2);
      *high = *value;
    }
    /* continue searching */
    return(1);
  }
  else {
    /* found a solution */
    if (!condition) 
      /* the upper bound is a miss, it must the lower bound */
      *value = *low;
    /* stop searching */
    return(0);
  }
}

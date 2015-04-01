
/* diskmodel (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
 */



#include "dm_config.h"
#include "dm.h"



// the double time is going to be msecs to minimize old disksim code
// breakage
INLINE dm_time_t dm_time_dtoi(double t)
{
  // milliseconds to picoseconds so multiply by 10^9
  return (dm_time_t)(t * (double)(DM_TIME_MSEC));
}

INLINE double    dm_time_itod(dm_time_t t)
{
  return (double)t / (double)(DM_TIME_MSEC);
}



// denominator of angles
// #define DM_ANGLE_DENOM ((unsigned)-1)

// convert angles between doubles and fixed-point representation
INLINE double      dm_angle_itod(dm_angle_t a)
{
  //  return (double)a / (double)(unsigned)-1;
  return (double)a / (double)((long long)1 << 32);
}


// hurst_r this code doesn't look right
// why is an integer subtracted from a double?
// Seems for angles greater than 1.0 we need to subtract 1.0 until less
// Like a mod 1.0
INLINE dm_angle_t  dm_angle_dtoi(double a)
{
  if(a > 1.0) 
    a -= (int)a;
  //  return (dm_angle_t)(a * (double)(unsigned)-1);
  return (dm_angle_t)(a * (double)((long long)1 << 32));
}

// End of file


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

// #include <sys/types.h>
// #include <math.h>
#include "dm_config.h"

// bucy
// really simple; does binary search of the square root space;
// square roots must be in [1,65536]
uint32_t sqrt32(uint32_t x) {
  int c;
  uint32_t old = 0, result = 32767; 

  if(x == 0) return 0;
  else if(x == 1) return 1;

  for(c = 0; c <= 16; c++) {
    uint32_t xsq = result * result;
    if((xsq <= x )
       && (((result + 1) * (result + 1)) > x))
      {
	return result;
      }

    old = result;
    if(xsq > x) {
      result -= (1 << (15 - c));
    }
    else {
      result += (result == 65535 ? 0 : (1 << (15 - c)));
    }
/*      printf("%d -> %d\n", old, result); */
  }

  return result;
}

uint64_t sqrt64(uint64_t x) {
  int c;
  uint64_t old = 0, result = 2147483647; 

  if(x == 0) return 0;
  else if(x == 1) return 1;

  for(c = 0; c <= 32; c++) {
    uint64_t xsq = result * result;
    if((xsq <= x )
       && (((result + 1) * (result + 1)) > x))
      {
	return result;
      }

    old = result;
    if(xsq > x) {
      result -= ((uint64_t)1 << (31 - c));
    }
    else {
      result += (result == 2147483647 ? 0 : ((uint64_t)1 << (31 - c)));
    }
  }

  return result;
}


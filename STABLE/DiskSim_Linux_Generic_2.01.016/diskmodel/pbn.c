
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



#include "dm.h"

// cyl is 24 bits, head is 8, sect is 32
// dm_pbn_t is formatted in hex bits as follows: CCCCCCHHSSSSSSSS
//   where:
//     C is a 4 bit cylinder address
//     H is a 4 bit head address
//     S is a 4 bit sector address

// convert the internal format to a struct dm_pbn
int dm_to_pbn(dm_pbn_t i, struct dm_pbn *result)
{
  result->sector = 0xffffffff & i;
  result->head = (i >> 32) & 0xff;
  result->cyl = (i >> 40) & 0xffffff;
  return 0;
}

// convert a struct dm_pbn to the internal format 
int dm_from_pbn(struct dm_pbn *p, dm_pbn_t *result)
{
  *result = p->sector;
  *result |= ((dm_pbn_t)(p->head & 0xff) << 32);
  *result |= ((dm_pbn_t)(p->cyl & 0xffffff) << 40);
  return 0;
}

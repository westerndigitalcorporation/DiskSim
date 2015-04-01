/* diskmodel (version 1.1)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2003-2005
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


#include "test.h"

void testsUsage(void) {
  fprintf(stderr, "usage: layout_simple_0t <model>\n");
}

int
layout_test_simple_0t(struct dm_disk_if *d)
{
  int bad = 0;
  int lbn1, lbn2, rc;
  struct dm_pbn pbn;

  for(lbn1 = 0; lbn1 < d->dm_sectors; lbn1++) {
    rc = d->layout->dm_translate_ltop_0t(d, lbn1, MAP_FULL, &pbn, 0);
    if(rc == DM_NX) {
      bad++;
      continue;
    }

    lbn2 = d->layout->dm_translate_ptol_0t(d, &pbn, 0);
    if(lbn2 < 0) {
      bad++;
      continue;
    }
    else if(lbn2 != lbn1) {
      printf("*** %d -> (%d,%d,%d) -> %d\n",
	     lbn1, pbn.cyl, pbn.head, pbn.sector, lbn2);
      bad++;
      continue;
    }
   
  }

  return bad;
}

int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
  return layout_test_simple_0t(d);
}

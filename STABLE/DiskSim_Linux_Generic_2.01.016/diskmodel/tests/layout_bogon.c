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
  fprintf(stderr, "usage: layout_bogon <model>\n");
}


// Try to reverse-map bogus PBNs -- they should all return DM_NX, not
// some (wrong) lbn.

int 
layout_bogon(struct dm_disk_if *d) {
  int lbn = 0;
  int i, rv;
  struct dm_pbn p, p2;
  int l0, ln;
  int bad = 0;

  do {
    d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &p, 0);
    d->layout->dm_get_track_boundaries(d, &p, &l0, &ln, 0);
    d->layout->dm_translate_ltop(d, ln, MAP_FULL, &p, 0);
    p2 = p;
    for(i = 0; i < 1; i++) {
      p2.sector++;
      rv = d->layout->dm_translate_ptol(d, &p2, 0);
      if(rv >= 0) {
	printf("(%d,%d,%d) -> %d  !!! max (%d,%d,%d)\n",
	       p2.cyl, p2.head, p2.sector, rv,
	       p.cyl, p.head, p.sector);
	bad++;
      }
    }

    lbn = ln + 1;
  } while(lbn < d->dm_sectors);

  return bad;
}

int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
  return layout_bogon(d);
}

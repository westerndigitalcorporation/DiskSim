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
  fprintf(stderr, "usage: layout_track <model> <raw_layout_file>\n");
}

int layout_test_trackbound(struct dm_disk_if *d) {
  int c;
  int badct = 0;
  int bad = 0;
  int lastbad = 0;
  int lastlbn; // last lbn on the previous track

  setlinebuf(stdout);

  for(c = 0; c < d->dm_sectors; ) {
    int l1 = 0, l2 = 0;
    struct dm_pbn pbn;
    int sptl, sptp;
    int remapsector = 0;

    d->layout->dm_translate_ltop(d, c, MAP_FULL, &pbn, 0);

    sptl = d->layout->dm_get_sectors_lbn(d, c);
    sptp = d->layout->dm_get_sectors_pbn(d, &pbn);

    if(sptl != sptp) {
      printf("spt (lbn) %d != %d (pbn)\n", sptl, sptp);
      bad = 1;
    }

    d->layout->dm_get_track_boundaries(d, &pbn, &l1, &l2, 0);

	//printf("%d -> (%d,%d) = %d (%d)\n", c, l1, l2, l2 - l1 + 1, sptl);

    if(l1 >= d->dm_sectors) {
      printf("l1 past end of disk! (%d >= %d)\n", l1, d->dm_sectors);
    }
    
    if(l2 >= d->dm_sectors) {
      printf("l2 past end of disk! (%d >= %d)\n", l2, d->dm_sectors);
    }
    

    // sanity checks
    if(l1 >= l2) {
      printf("test_trackbound: %d >= %d\n", l1, l2);
      bad = 1;
    }

    if(l2 - l1 + 1 > sptl) {
      printf("%d > %d (phys spt)\n", l2 - l1 + 1, sptl);
      bad = 1;
    }

    if(!((l1 <= c) && (c <= l2))) {
      printf("test_trackbound: %d (%d,%d,%d) -> (%d, %d) \n", 
	     c, pbn.cyl, pbn.head, pbn.sector,
	     l1, l2);
      bad = 1;
    }

    // "smoothness" test... the results should cover the lbn space
    if(!lastbad && (c>0) && (l1 != (lastlbn+1)) && (lastlbn != l2)) 
    {
      printf("test_trackbound (%d): (%d,%d) <> %d+1\n", c, l1, l2, lastlbn);
      bad = 1;
    }
    
    lastlbn = l2;
    if(bad) {
      printf("bad %d\n", c);
      badct++;
    }

    if(bad) {
      c++;
    }
    else {
      c = l2 + 1;
    }

    lastbad = bad;
    bad = 0;
  }
  return badct;
}


int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
  return layout_test_trackbound(d);
}

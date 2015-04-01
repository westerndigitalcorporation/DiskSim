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
  fprintf(stderr, "usage: layout_simple <model>\n");
}



int 
layout_test_simple(struct dm_disk_if *d) {
  int count = 0, runlbn = 0, lbn = 0;
  struct dm_pbn pbn, trkpbn = {0,0,0};
  int bad = 0;

  do {
    dm_angle_t skew;
    struct dm_mech_state track;
    dm_ptol_result_t rc;

    rc = d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &pbn, 0);

    if(rc == DM_NX) {
      bad++;
      continue;
    }
    
    skew = d->layout->dm_pbn_skew(d, &pbn);


    printf("%d -> (%d,%d,%d) @ %f\n", 
	   lbn, pbn.cyl, pbn.head, pbn.sector, dm_angle_itod(skew)); 

    d->layout->dm_get_track_boundaries(d, &pbn, 0, &lbn, 0);
    lbn++;


    fflush(stdout);
  } while(lbn < d->dm_sectors);


  return bad;
}

int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
  return layout_test_simple(d);
}

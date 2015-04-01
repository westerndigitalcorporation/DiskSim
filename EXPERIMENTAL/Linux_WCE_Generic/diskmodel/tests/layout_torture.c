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
layout_torture(struct dm_disk_if *d) {
  int lbn, lbn2;
  struct dm_pbn pbn, pbn2, pbn3, pbn4;
  int rv;
  int bad = 0;
  
  while(1) {
    lbn = rand();
    rv = d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &pbn, 0);
    if(rv == DM_OK) {
      rv = d->layout->dm_translate_ptol(d, &pbn, 0);
    }
    
    pbn.cyl = rand();
    pbn.head = rand();
    pbn.sector = rand();

    pbn2 = pbn;
    pbn3 = pbn;
    pbn4 = pbn;

    rv = d->layout->dm_translate_ptol(d, &pbn, 0);

    rv = d->layout->dm_get_track_boundaries(d, &pbn2, &lbn, &lbn2, 0);

    rv = d->layout->dm_pbn_skew(d, &pbn3);

    rv = d->layout->dm_get_sectors_pbn(d, &pbn4);


    fflush(stdout);
  }


  return bad;
}

int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
  return layout_torture(d);
}

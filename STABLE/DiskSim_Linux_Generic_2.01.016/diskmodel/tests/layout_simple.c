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
#include "driver.h"

void testsUsage(void) {
    fprintf(stderr, "usage: layout_simple <model>\n");
}


int
layout_test_simple(struct dm_disk_if *d) {
    int i, lbn, count = 0, runlbn = 0;
    struct dm_pbn pbn, trkpbn = {0,0,0};
    int bad = 0;

    for(i = 0; i < d->dm_sectors; i++) {
		int lbn2;
		dm_angle_t skew, zerol;
		struct dm_mech_state track;
		dm_ptol_result_t rc;

		startClock(0);
		rc = d->layout->dm_translate_ltop(d, i, MAP_FULL, &pbn, 0);
		skew = d->layout->dm_pbn_skew(d, &pbn);
		stopClock(0);
		if(rc == DM_NX) {
			printf("*** %s %d -> NX\n", __func__, i);
			bad++;
			continue;
		}

		startClock(1);
		lbn2 = d->layout->dm_translate_ptol(d, &pbn, 0);
		stopClock(1);
		if(lbn2 == DM_NX) {
			printf("*** layout_test_simple %d -> (%d,%d,%d) -> NX\n", i, pbn.cyl, pbn.head, pbn.sector);
			bad++;
			continue;
		}

		if(i != lbn2) {
			printf("*** layout_test_simple: %8d -> (%8d, %3d, %4d) -> %8d\n", i, pbn.cyl, pbn.head, pbn.sector, lbn2);
			bad++;
		}
		else {
			printf("%8d <-> (%8d, %3d, %4d)(%u %f)\n", i, pbn.cyl, pbn.head, pbn.sector, skew, dm_angle_itod(skew));
			if((lbn2 > 0)
					&& (trkpbn.head == pbn.head)
					&& (trkpbn.cyl == pbn.cyl)
					&& ((trkpbn.sector+count) == pbn.sector)) { count++;  }
			else {
				printf("l %d %d %d %d %d\n", runlbn, trkpbn.cyl, trkpbn.head, trkpbn.sector, count);
				runlbn = i;
				trkpbn = pbn;
				count = 1;
			}
		}

		fflush(stdout);
	}


	return bad;
}

int minargs = 0;

int doTests(struct dm_disk_if *d, int argc, char **argv)
{
	addBucket("ltop()");
	addBucket("ptol()");


	return layout_test_simple(d);
}

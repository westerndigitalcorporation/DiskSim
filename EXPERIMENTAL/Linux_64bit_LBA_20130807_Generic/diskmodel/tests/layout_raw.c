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


// validate the layout against a raw layout

int minargs = 1;

void testsUsage(void) {
  fprintf(stderr, "usage: layout_raw <model> <raw layout file>\n");
}


int get_next_line(FILE *fp,
		  int *lbn,
		  struct dm_pbn *p,
		  int *count)
{
  int scanct;
  char cntstr[32];
  if((scanct = fscanf(fp, "lbn %d --> cyl %d, head %d, sect %d, %s %d\n",
		      lbn, &p->cyl, &p->head, &p->sector, cntstr, count) == 6))
  {
    if(!strcmp(cntstr, "seqcnt")) {
      (*count)++;
    }

    return 1;
  }
  else {
    printf("wrong %d\n", scanct);
    return 0;
  }
}


int raw_layout_test(struct dm_disk_if *d,
		    FILE *fp)
{
  int i;
  int lbn;
  char junk[1024];
  struct dm_pbn p;
  int count;

  struct dm_pbn p2;
  int errors = 0;
  
  // fetch off the header stuff
  fgets(junk, sizeof(junk), fp);
  fgets(junk, sizeof(junk), fp);
  fgets(junk, sizeof(junk), fp);

  //  for(i = 0; i < 85000; i++) get_next_line(fp, &lbn, &p, &count);

  while(get_next_line(fp, &lbn, &p, &count))
  {
    for(i = 0; i < count; i++) {
      dm_ptol_result_t rc = 
	d->layout->dm_translate_ltop(d, lbn+i, MAP_FULL, &p2, 0);
      
      if((p.cyl != p2.cyl)
	 || (p.head != p2.head)
	 || ((p.sector + i) != p2.sector))
      {
	printf("*** layout_raw: %d -> (%d,%d,%d) <> (%d,%d,%d)\n",
	       lbn+i, 
	       p.cyl, p.head, p.sector+i,
	       p2.cyl, p2.head, p2.sector);

	fflush(stdout);
	errors++;
      }
    }
  }

  return errors;
}








int doTests(struct dm_disk_if *d, 
	    int argc,
	    char **argv)
{
  int count = 0;
  FILE *fp = fopen(argv[3], "r");
  ddbg_assert(fp != 0);

  count = raw_layout_test(d, fp);

  fclose(fp);
  return count;
}

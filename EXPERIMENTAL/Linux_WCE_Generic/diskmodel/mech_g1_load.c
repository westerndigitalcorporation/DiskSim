
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




/*
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
 *
 * Permission to reproduce, use, and prepare derivative works of
 * this software for internal use is granted provided the copyright
 * and "No Warranty" statements are included with all reproductions
 * and derivative works. This software may also be redistributed
 * without charge provided that the copyright and "No Warranty"
 * statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 */

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */


#include "modules/modules.h"

// #include <math.h>

#include <errno.h>
#include <stdio.h>

#include <libparam/libparam.h>
#include <libparam/bitvector.h>
#include "mech_g1.h"
#include "mech_g1_private.h"

//#include "modules/dm_mech_g1_param.h"


void 
dm_mech_g1_read_extracted_seek_curve(char *filename, 
				     int *cntptr,
				     int **distsptr, 
				     dm_time_t **timesptr);

int do_1st10_seeks(struct dm_mech_g1 *result, struct lp_list *l);
int do_hpl_seek(struct dm_mech_g1 *result, struct lp_list *l);



struct dm_mech_if *
dm_mech_g1_loadparams(struct lp_block *b, int *junk) {
  
  struct dm_mech_g1 *result = malloc(sizeof(*result));
  result->hdr = dm_mech_g1;
  //  #include "modules/dm_mech_g1_param.c"
  lp_loadparams(result, b, &dm_mech_g1_mod);


  result->rotatetime = dm_time_dtoi(1000.0 / ((double)result->rpm / 60.0));
  
  return (struct dm_mech_if *)result;
}


void 
dm_mech_g1_read_extracted_seek_curve (char *filename, 
				      int *cntptr,
				      int **distsptr, 
				      dm_time_t **timesptr)
{
   int rv, mat;
   int lineflag = 1;
   int count = 0, buflen = 128;
   int *dists;
   dm_time_t *times;
   FILE *seekfile;
   char linebuf[1024];

   char *pathname = lp_search_path(lp_cwd, filename);

   if(pathname) {
     seekfile = fopen(pathname, "r");
   }
   else {
     ddbg_assert2(0, "Seek file not found in path!");
   }

   ddbg_assert3(seekfile != 0, ("fopen seekfile (%s) failed: %s", 
				filename,
				strerror(errno)));


   rv = (fgets(linebuf, sizeof(linebuf), seekfile) != 0);

   mat = sscanf(linebuf, "Seek distances measured: %d\n", &count);
   if(mat == 1) {
     buflen = count;
     lineflag = 0;
   }

   dists = calloc(buflen, sizeof(*dists));
   times = calloc(buflen, sizeof(*times));


   do {
     double time, stdev;
     int dist;
   
     if(!lineflag) {
       rv = (fgets(linebuf, sizeof(linebuf), seekfile) != 0);
     }
     else {
       lineflag = 0;       
     }
     if(rv) {
       mat = sscanf(linebuf, "%d, %lf, %lf\n", &dist, &time, &stdev);
       
       if(mat == 2 || mat == 3) {
	 if(count >= buflen-1) {
	   buflen *= 2;
	   dists = realloc(dists, buflen * sizeof(*dists));
	   times = realloc(times, buflen * sizeof(*times));
	 }
       
	 dists[count] = dist;
	 times[count] = dm_time_dtoi(time);
	 count++;
       }
       else {
	 fprintf(stderr, "*** bogus line in seek curve (%s:%d): %s\n", 
		 __FILE__, __LINE__, linebuf);
       }
     }
   } while(rv);

   fclose(seekfile);
   *cntptr = count;
   *distsptr = dists;
   *timesptr = times;
}



int do_1st10_seeks(struct dm_mech_g1 *result, 
		   struct lp_list *l) 
{
  if(result->seektime != SEEK_1ST10_PLUS_HPL) {
    fprintf(stderr, "*** warning: ignoring First 10 seeks parameter for seek function other than First 10 plus hpl.\n");
    return 0;
  }

  if(l->values_len < 10) {
    fprintf(stderr, "*** error: Want 10 first seek times (got %d)\n", l->values_len);
    return -1;
  }
  else {
    int e;
    for(e = 0; e < 10; e++) {
      if(l->values[e]->t != D) {
	fprintf(stderr, "*** error: First 10 seeks must be floats.\n");
	return -1;
      } else if(l->values[e]->v.d < 0.0) {
	fprintf(stderr, "*** error: First 10 seeks must be nonnegative.\n");
	return -1;
      }
      result->first10seeks[e] = dm_time_dtoi(l->values[e]->v.d);
     
    }
  }
  return 0;
}

int do_hpl_seek(struct dm_mech_g1 *result, struct lp_list *l) {

  if((result->seektime != SEEK_HPL)
     && (result->seektime != SEEK_1ST10_PLUS_HPL)) 
    {
      fprintf(stderr, 
	      "*** warning: ignoring hpl parameters for non-hpl seek type.\n");
      return 0;
    }

  if(l->values_len < 6) {
    fprintf(stderr, "*** error: Want 6 HPL seek equation parameters (got %d)\n", l->values_len);
    return -1;
  }
  else {
    int e;

    // first one is in cyls, rest are in milliseconds
    if(l->values[0]->t != I) {
      fprintf(stderr, "*** error: HPL v1 is in cylinders.\n");
      return -1;
    } 
    else if(l->values[0]->v.i < 0) {
      fprintf(stderr, "*** error: HPL v1 must be nonnegative.\n");
      return -1;
    }
    result->hpseek_v1 = l->values[0]->v.i;

    for(e = 1; e < 6; e++) {
      if(l->values[e]->t != D) {
	fprintf(stderr, "*** error: HPL seek eqn. values v2..v6 must  be floats.\n");
	return -1;
      } 
      else if((l->values[e]->v.d < 0.0) && (e != 5)) {
	fprintf(stderr, "*** error: HPL seek eqn. values v2..v5 must be nonnegative.\n");
	return -1;
      }

      if((l->values[e]->v.d == -1.0) && (e == 5)) {
	result->hpseek[e] = -1;
      }
      else {
	result->hpseek[e] = dm_time_dtoi(l->values[e]->v.d);
      }
    }
  }

  if(result->hpseek[5] != -1) {
    result->seekone = result->hpseek[5];
  }

  return 0;
}


// who calls this?
// in pre3-28, this seems unreachable (not sure) (bucy 2/02)
void 
dm_mech_g1_seek_init(struct dm_disk_if *d) {
  double tmpfull, tmpavg, tmptime;
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
  // XXX get rid of this
  FILE *outputfile = stderr;

  ddbg_assert2(0, "This is deprecated and/or hasn't been ported.\n");



  if ((m->seektype == SEEK_3PT_CURVE) && 
      (m->seekavg > m->seekone)) 
    {
      fprintf (outputfile, "seekone %lld, seekavg %lld, seekfull %lld\n", 
	       m->seekone, m->seekavg, m->seekfull);

      tmpfull = m->seekfull;
      tmpavg = m->seekavg;
      tmptime = (double) -10.0 * m->seekone;
      tmptime += (double) 15.0 * m->seekavg;
      tmptime += (double) -5.0 * m->seekfull;
      //      tmptime = tmptime / 
      //	(3.0 * sqrt((double) (d->dm_cyls)));

      m->seekavg *= (double) -15.0;
      m->seekavg += (double) 7.0 * m->seekone;
      m->seekavg += (double) 8.0 * m->seekfull;
      m->seekavg = m->seekavg / 
	(double) (3 * d->dm_cyls);

      m->seekfull = tmptime;

      //      fprintf (outputfile, "seekone %f, seekavg %f, seekfull %f\n", 
      //	       m->seekone, m->seekavg, m->seekfull);

      //      fprintf (outputfile, "seekone %f, seekavg %f, seekfull %f\n", 
      //       diskseektime(m, 1, 0, 1), 
      //       diskseektime(m, (d->dm_cyls / 3), 0, 1), 
      //       diskseektime(m, (d->dm_cyls - 1), 0, 1));

      if ((m->seekavg < 0.0) || (m->seekfull < 0.0)) {
	m->seektype = SEEK_3PT_CURVE;
	m->seekfull = tmpfull;
	m->seekavg = tmpavg;
      }
    }
}






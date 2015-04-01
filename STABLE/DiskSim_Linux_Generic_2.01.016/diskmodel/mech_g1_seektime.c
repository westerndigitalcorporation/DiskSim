
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


#include "mech_g1.h"
#include "mech_g1_private.h"

// g1 seektime computation functions

dm_time_t 
dm_mech_g1_seek_const(struct dm_disk_if *d,
		      struct dm_mech_state *begin,
		      struct dm_mech_state *end,
		      int rw)
{
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
  ddbg_assert(m->seektype == SEEK_CONST);
  return m->seektime;
}


dm_time_t 
dm_mech_g1_seek_3pt_curve(struct dm_disk_if *d,
			  struct dm_mech_state *begin,
			  struct dm_mech_state *end,
			  int rw)

{
  dm_time_t result;
  int cyls;
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
  
  cyls = abs(end->cyl - begin->cyl);
  if (cyls == 0) {
    result = 0;
  } 
  else {
    result = m->seekone;
    result += m->seekavg * (cyls - 1);
    result += m->seekfull * sqrt32(cyls - 1);
  }

  return result;
}


dm_time_t 
dm_mech_g1_seek_3pt_line(struct dm_disk_if *d,
			 struct dm_mech_state *begin,
			 struct dm_mech_state *end,
			 int rw)
{
  int mult;
  int numcyls;
  int cyls;
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
  dm_time_t result;

  cyls = abs(end->cyl - begin->cyl);
  numcyls = cyls;

  if(cyls == 1) {
    result = m->seekone;
  } 
  else if(cyls == 0) {
    result = 0;
  } 
  else if(cyls <= (numcyls / 3)) {
    // MATH
    mult = (3*(cyls-1) / numcyls);
    result = m->seekone;
    result += mult * (m->seekavg - result);
  } 
  else {
    // MATH
    mult = ((3*cyls - numcyls) / (2*numcyls));
    result = m->seekavg;
    result += mult * (m->seekfull - result);
  }

  return result;
}

// the hpl stuff has fp math in it and I don't have time to fix
// it now
#ifndef _DISKMODEL_FREEBSD

/* seek equation described in Ruemmler and Wilkes's IEEE Computer
 * article (March 1994).  Uses six parameters: a division point
 * (between the root based equation and the linear equation), a
 * constant and mulitplier for the square root of the distance, a
 * constant and multiplier for the distance (linear part of curve) and
 * a value for single cylinder seeks.  
 */

dm_time_t 
dm_mech_g1_seek_hpl(struct dm_disk_if *d,
		    struct dm_mech_state *begin,
		    struct dm_mech_state *end,
		    int rw)
{
  dm_time_t *hpseek;
  int dist;
  dm_time_t result;
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;


  hpseek = m->hpseek;
  dist = abs(end->cyl - begin->cyl);

  if (dist == 0) {
    result = 0.0;
  }
  else if((dist == 1) && (hpseek[5] != -1)) {
    result = m->seekone;
  } 
  else if(dist < hpseek[0]) {
    // XXX: math voodoo
    result = ((sqrt64((uint64_t)dist << 32) * hpseek[2]) >> 16) + hpseek[1];
  } 
  else {
    result = (hpseek[4] * (double) dist) + hpseek[3];
  }

  return result;
}


/* An extended version of the equation described above, wherein the
 * first ten seek distances are explicitly provided, since seek time
 * curves tend to be choppy in this region.  (See UM TR CSE-TR-194-94) 
 */

dm_time_t 
dm_mech_g1_seek_1st10_plus_hpl(struct dm_disk_if *d, 
			       struct dm_mech_state *begin,
			       struct dm_mech_state *end,
			       int rw)

{
  dm_time_t *hpseek;
  dm_time_t result;
  int dist = abs(end->cyl - begin->cyl);

  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
  
  hpseek = m->hpseek;
  
  if(dist == 0) {
    result = 0;
  } 
  else if (dist <= 10) {
    result = m->first10seeks[(dist - 1)];
  } 
  else if (dist < m->hpseek_v1) {
    // XXX: math voodoo
    result = ((sqrt64((uint64_t)dist << 32) * hpseek[2]) >> 16) + hpseek[1];
  } 
  else {
    result = (hpseek[4] * dist) + hpseek[3];
  }

  if(begin->head != end->head) {
    double headsw;
    headsw = d->mech->dm_headswitch_time(d, begin->head, end->head);
    if(headsw > result) {
      result = headsw;
    }
  }


  return result;
}


#else  
dm_time_t 
dm_mech_g1_seek_hpl(struct dm_disk_if *d,
		    struct dm_mech_state *begin,
		    struct dm_mech_state *end,
		    int rw)
{
  ddbg_assert2(0, "seek_hpl currently unavailable for kernel");
  return 0;
}

dm_time_t 
dm_mech_g1_seek_1st10_plus_hpl(struct dm_disk_if *d, 
			       struct dm_mech_state *begin,
			       struct dm_mech_state *end,
			       int rw)

{
  ddbg_assert2(0, "seek_1st10_plus_hpl currently unavailable for kernel");
  return 0;
}

#endif // _DISKMODEL_FREEBSD

dm_time_t 
dm_mech_g1_seek_extracted(struct dm_disk_if *d,
			  struct dm_mech_state *begin,
			  struct dm_mech_state *end,
			  int rw)
			      
{
  dm_time_t result = 0;
  int i;
  int dist = abs(end->cyl - begin->cyl);   
  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;

  dm_time_t t1,t2;
  int d1, d2;

   if(dist) {
     // linear search of extracted seek curve
     // might want to do something faster

     // binsearch -- d_i <= d < d_{i+1}

     for(i = 0; i < m->xseekcnt; i++) {
       if(dist == m->xseekdists[i]) {
	 result = m->xseektimes[i];
	 break;
       } 
       // The computation here will also do linear extrapolation if
       // we're past the end.
       else if (dist <= m->xseekdists[i]
		|| (i == m->xseekcnt-1))
       {
	 t1 = m->xseektimes[i-1];
	 t2 = m->xseektimes[i];
	 d1 = m->xseekdists[i-1];
	 d2 = m->xseekdists[i];
	 
	 // didn't find it exactly; do some interpolation

	 /*  	   int ddiff =  */
	 /*  	     (dist - m->xseekdists[(i-1)]) /  */
	 /*  	     (m->xseekdists[i] - m->xseekdists[(i-1)]); */

	 // this sounds perverse but in e.g. the atlas10k model
	 // 10,     1.53100
	 // 12,     1.51500
	 // so its possible to get a negative answer here!

	 result = m->xseektimes[(i-1)];

	 // this is so convoluted because dm_time_t is unsigned...
	 if(t1 > t2) {
	   // result -= ddiff * (t1 - t2);
	   result -= (dist - d1) * (t1 - t2) / (d2 - d1);
	 }
	 else {
	   //	     result += ddiff * (t2 - t1);
	   result += (dist - d1) * (t2 - t1) / (d2 - d1);
	 }
	 break;
       }
     }
   }

   return result;
}


// these must line up with disk_seek_t 
dm_mech_g1_seekfn dm_mech_g1_seekfns[] = {
  dm_mech_g1_seek_const,
  dm_mech_g1_seek_3pt_line,
  dm_mech_g1_seek_3pt_curve,
  dm_mech_g1_seek_hpl,
  dm_mech_g1_seek_1st10_plus_hpl,
  dm_mech_g1_seek_extracted
};




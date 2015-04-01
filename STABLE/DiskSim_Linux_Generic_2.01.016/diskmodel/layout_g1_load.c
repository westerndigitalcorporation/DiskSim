
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


#include <stdlib.h>
#include <string.h>

#include <libparam/libparam.h>
#include <libparam/bitvector.h>

#include "dm.h"
#include "layout_g1.h"

#include "modules/modules.h"
#include "modules/dm_layout_g1_param.h"
#include "modules/dm_layout_g1_zone_param.h"


static void checknumblocks(struct dm_layout_g1 *d);
static void setup_band_blknos(struct dm_layout_g1 *);
static void initialize_bands(struct dm_disk_if *d);
int disk_load_zones(struct lp_list *l,
		    struct dm_layout_g1 *layout);

static void dm_layout_g1_initialize(struct dm_disk_if *d);

static int 
dm_layout_g1_compute_blksinband(struct dm_disk_if *d,
				struct dm_layout_g1_band *b);


struct dm_layout_if *
dm_layout_g1_loadparams(struct lp_block *b, struct dm_disk_if *d)
{
  struct dm_layout_g1 *result = malloc(sizeof(*result));
  memset(result, 0, sizeof(*result));

  //#include "modules/dm_layout_g1_param.c"
  
  lp_loadparams(result, b, &dm_layout_g1_mod);

  result->disk = d;
  result->disk->layout = (struct dm_layout_if *)result;
  
  
  
  /*    result->hdr = g1_layout_nosparing; */
  /*    result->hdr = g1_layout_sectpertrackspare; */
  /*    result->hdr = g1_layout_sectperzonespare; */
  /*    result->hdr = g1_layout_sectpercylspare; */
  /*    result->hdr = g1_layout_sectperrangespare; */
  
  switch(result->mapping) {
  case LAYOUT_NORMAL:
  case LAYOUT_CYLSWITCHONSURF1:
  case LAYOUT_CYLSWITCHONSURF2:
    if(result->sparescheme == NO_SPARING) {
      result->hdr = g1_layout_nosparing; 
    } else {
      if (result->sparescheme == TRACK_SPARING) {
	result->hdr = g1_layout_tracksparing;
      }
      else if (result->sparescheme == SECTPERTRACK_SPARING) {
	  
	result->hdr = g1_layout_sectpertrackspare; 
      }
	
      else if ((issectpercyl(result)) ||
	       (result->sparescheme == SECTATEND_SPARING))
	{
	  result->hdr = g1_layout_sectpercylspare;
	}
      else if ((result->sparescheme == SECTSPERZONE_SPARING) ||
	       (result->sparescheme == SECTSPERZONE_SPARING_SLIPTOEND))
	{
	  result->hdr = g1_layout_sectperzonespare; 
	}
      else if (result->sparescheme == SECTPERRANGE_SPARING) {
	result->hdr = g1_layout_sectperrangespare;
      }
      else {
	ddbg_assert2(0, "Unknown sparing scheme");
      }
    }
    break;
  default:
    ddbg_assert2(0, "Unknown lbn<->pbn scheme");
  }


  // careful with the order here...
  initialize_bands(result->disk);
  dm_layout_g1_initialize(d);
  checknumblocks(result);
  setup_band_blknos(result);

  return (struct dm_layout_if *)result;
}


static void checknumblocks(struct dm_layout_g1 *d) {
  int numblocks = 0;
  int i;
  // XXX kill me
  FILE *outputfile = stderr;
  
  for (i = 0; i < d->bands_len; i++) {
    numblocks += d->bands[i].blksinband;
  }
  if (numblocks != d->disk->dm_sectors) {
    //    fprintf (outputfile, "*** warning: Numblocks provided by user does not"
    //	     " match specifications - user %d, actual %d\n",
    //	     d->disk->dm_sectors, numblocks);
  }
  d->disk->dm_sectors = numblocks;
}




//
// more stuff from disksim
//


/*
 * Fills in a table in the layout struct containing the first lbn of
 * each zone, indexed by zone.  This saves lbn/pbn translation some
 * work.  Needs diskmap to be initialized.  
 */
static void setup_band_blknos(struct dm_layout_g1 *d) {
  int c;

  d->band_blknos[0] = 0;
  for(c = 1; c < d->bands_len; c++) {
    d->band_blknos[c] = d->band_blknos[c - 1] + 
      d->bands[c-1].blksinband;
  }
}


static void initialize_bands(struct dm_disk_if *d) {
  int j;
  double tmptime, rotblks;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;

  return;
  
  // Defunct.  Cook up skews if none provided in model.
  for (j = 0; j < l->bands_len; j++) {
    double period;
    // set tmptime to the greater of a headswitch for a read or a
    // write
    
    struct dm_mech_state s1 = {0,0,0}, s2 = {1,0,0};
    dm_time_t t1 = d->mech->dm_seek_time(d, &s1, &s2, 0);
    dm_time_t t2 = d->mech->dm_seek_time(d, &s1, &s2, 1);
    
    tmptime = t1 > t2 ? t1 : t2;

    period = dm_time_itod(d->mech->dm_period(d));
    rotblks = ((double)l->bands[j].blkspertrack) / period;
    //    l->bands[j].firstblkangle = l->bands[j].firstblkangle / rotblks;

    if (l->bands[j].trackskew == -1.0) {
      ddbg_assert2(0, "unimplemented");
      l->bands[j].trackskew = tmptime;
    } 
    else if (l->bands[j].trackskew == -2.0) {
      ddbg_assert2(0, "unimplemented");
      tmptime = (double) ((int) (tmptime * rotblks + 0.999999999));
      l->bands[j].trackskew = tmptime / rotblks;
    } 

    // zero distance seek without a head switch
    // write vs. read
    // XXX ... based on disksim-pre3-28, every seek function returns
    // zero seek time for a zero distance seek!
    //    tmptime = max(diskseektime(d, 1, 0, 0), 
    //		  diskseektime(d, 1, 0, 1));
    tmptime = 0;

    if (l->bands[j].cylskew == -1.0) {
      ddbg_assert2(0, "unimplemented");
      l->bands[j].cylskew = tmptime;
    } 
    else if (l->bands[j].cylskew == -2.0) {
      ddbg_assert2(0, "unimplemented");
      tmptime = (double) ((int) (tmptime * rotblks + 0.99999999));
      l->bands[j].cylskew = tmptime / rotblks;
    } 
  }
}



/* dummy -- do not call */
int dm_layout_g1_zone_loadparams(struct lp_block *b) { ddbg_assert(0); return 0; }

int getslips(struct dm_layout_g1_band *z, struct lp_list *l);
int getdefects(struct dm_layout_g1_band *z, struct lp_list *l);


int disk_load_zones(struct lp_list *lst,
		    struct dm_layout_g1 *layout)
{
  int d;
  struct dm_layout_g1_band *result = 0;
  //  struct lp_block *b = 0;

  layout->bands_len = 0;
  for(d = 0; d < lst->values_len; d++) {
    if(lst->values[d]) layout->bands_len++;
  }

  layout->bands = malloc(layout->bands_len * sizeof(struct dm_layout_g1_band));
  layout->band_blknos = malloc(layout->bands_len * sizeof(int));
  bzero(layout->bands, layout->bands_len * sizeof(struct dm_layout_g1_band));

  result = layout->bands;
  /* iterate over bands */  
  for(d = 0; d < layout->bands_len; d++) {
    long long max = (long long)1 << 32;

    layout->bands[d].skew_units = layout->skew_units;

    if(!lst->values[d]) continue;
    if(lst->values[d]->t != BLOCK) {
	fprintf(stderr, "Bad band/zone def.\n");
	return 0;
      }
    //      b = lst->values[d]->v.b;
  

      //#include "modules/dm_layout_g1_zone_param.c"
      lp_loadparams(result, lst->values[d]->v.b, &dm_layout_g1_zone_mod);
      
      // This is now done in the blkspertrack init code.
      // Skews were doing angle_dtoi(skew / blkspertrack) but 
      // this turned out not to be the same as skew * sector_width and
      // caused off-by-1-sector bugs.
      //      result->sector_width = (dm_angle_t)(max / result->blkspertrack);

      result->trkspace = max % result->blkspertrack;

      result++;
      layout->bands[d].num = d;
  }



  return 1;
}



int getslips(struct dm_layout_g1_band *z, 
	     struct lp_list *b) 
{
  int c; int *i;
  z->numslips = 0;

  if(z->slip) {
    fprintf(stderr, "*** error: tried to declare zone slips multiple times.\n");
    return -1;
  }


  for(c = 0; c < b->values_len; c++) {
    if(b->values[c]) {
      if (b->values[c]->t != I) {
	fprintf(stderr, "*** error: Zone slips must be INTs.\n");
	return -1;
      }
      else if (b->values[c]->v.i < 0) {
	fprintf(stderr, "*** error: Zone slips must be nonnegative.\n");
	return -1;
      }

      z->numslips++;
    }
  }

  i = z->slip = malloc(z->numslips * sizeof(int));
  for(c = 0; c < z->numslips; c++) {
    *i = b->values[c]->v.i; i++;
  }
  return 0;
}


int getdefects(struct dm_layout_g1_band *z, 
	       struct lp_list *b)
{
  int c; int *i, *j;

  if(z->defect || z->remap) {
    fprintf(stderr, "*** error: tried to declare zone defects multiple times.\n");
    return -1;
  }

  z->numdefects = 0;
  for(c = 0; c < b->values_len; c++) {
    if(b->values[c]) {
      if (b->values[c]->t != I) {
	fprintf(stderr, "*** error: Zone defects must be INTs.\n");
	return -1;
      }
      else if (b->values[c]->v.i < 0) {
	fprintf(stderr, "*** error: Zone defects must be nonnegative.\n");
	return -1;
      }

      z->numdefects++;
    }
  }

  if(z->numdefects % 2) {
    fprintf(stderr, "*** error: Must have an even number of defect args.\n");
    return -1;
  }
  z->numdefects >>= 1;


  i = z->defect = malloc(z->numdefects * sizeof(int));
  j = z->remap = malloc(z->numdefects * sizeof(int));

  for(c = 0; c < z->numdefects; c++) {
      *i = b->values[c*2]->v.i; i++;
      *j = b->values[c*2 + 1]->v.i; j++;
  }

  return 0;
}





static void 
dm_layout_g1_initialize(struct dm_disk_if *d)
{
  int i;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;

  ddbg_assert((l->mapping >= 0) && (l->mapping <= LAYOUT_MAX));
  
  for (i = 0; i < l->bands_len; i++) {
    struct dm_layout_g1_band *b = &l->bands[i];

    
    ddbg_assert3((l->sparescheme != TRACK_SPARING) 
	       || ((b->numslips + b->numdefects) <= b->sparecnt),
	       ("Defects and slips outnumber the available spares: %d < %d + %d\n", 
		b->sparecnt, 
		b->numdefects, 
		b->numslips));
     
    b->blksinband = dm_layout_g1_compute_blksinband(d, b);
  }
}



// used by diskmap_initialize()
static int 
dm_layout_g1_compute_blksinband(
        struct dm_disk_if *d,
        struct dm_layout_g1_band *b)
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  int blksinband;

  int cylcnt = b->endcyl - b->startcyl + 1;


  blksinband = cylcnt * b->blkspertrack * d->dm_surfaces;
  blksinband -= b->deadspace;

  switch(l->sparescheme) {
  case TRACK_SPARING:
    blksinband -= b->sparecnt * b->blkspertrack;
    break;
  case SECTPERTRACK_SPARING:
    blksinband -= b->sparecnt * cylcnt * d->dm_surfaces;
    break;
  case SECTATEND_SPARING:
    blksinband -= b->numslips;
    break;
  case SECTSPERZONE_SPARING:
    blksinband -= b->sparecnt;
    break;
  case SECTSPERZONE_SPARING_SLIPTOEND:
    blksinband -= b->sparecnt + b->numslips;
    break;

  default:
    if ((issectpercyl(l)) && (!(issliptoend(l)))) {
      blksinband -= b->sparecnt * cylcnt;
    } 
    else if ((issectpercyl(l)) && (issliptoend(l))) {
      blksinband -= (b->sparecnt * cylcnt) + b->numslips;
    } 
    else if (l->sparescheme == SECTPERRANGE_SPARING) {
      blksinband -= b->sparecnt * (cylcnt / l->rangesize);
    } 
    else {
      ddbg_assert3(0, ("Unknown sparing scheme %d", l->sparescheme));
    }
    break;
  }

  return blksinband;
}


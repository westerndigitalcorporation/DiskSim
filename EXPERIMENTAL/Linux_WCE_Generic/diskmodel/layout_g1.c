
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



#include "layout_g1.h"
#include "marshal.h"

#define max(x,y) (((x) > (y)) ? (x) : (y))

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



// Compute start + mult*q and add x for the number of times it 
// wraps around 0.  This can probably be optimized.
static inline dm_angle_t 
addmult(dm_angle_t start, int mult, dm_angle_t q, dm_angle_t x) 
{
  //  int c;
  dm_angle_t result = start;
  long long tmp = (long long)start + mult * (long long)q;
  long long max = (long long)1 << 32;
  


/*    for(c = 0; c < mult; c++) { */
/*      tmp = result + q; */
/*      if(tmp < result) { */
/*        result = tmp + x; */
/*      } */
/*      else { */
/*        result = tmp; */
/*      } */
/*    } */
  
  return result + mult * q + (tmp / max) * x;
}

// optimize these someday (binsearch, maybe)
static struct dm_layout_g1_band *
find_band_lbn(struct dm_layout_g1 *l, int lbn)
{
  struct dm_layout_g1_band *b = &l->bands[0];
  int bandstart = 0;
  int bandno = 0;
  int templbn = lbn;

  while((templbn >= b->blksinband) || (templbn < 0)) {
    bandstart += b->blksinband;
    templbn -= b->blksinband;
    bandno++;
    b = &l->bands[bandno];
    //fprintf( stderr, "\nlbn %d, templbn %d, num %d, startcyl %d, endcyl %d, blkspertrack %d, blksinband %d, deadspace %d, bands_len %d, bandno %d", lbn, templbn, b->num, b->startcyl, b->endcyl, b->blkspertrack, b->blksinband, b->deadspace,  l->bands_len, bandno );
    ddbg_assert(bandno < l->bands_len);
    ddbg_assert(templbn >= 0);
  }
  
  return b;
}

static int 
g1_st_lbn(struct dm_disk_if *d, int lbn) {
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);
  return b->blkspertrack;
}


static struct dm_layout_g1_band *
find_band_pbn(struct dm_layout_g1 *l, struct dm_pbn *p)
{
  int c;
  for(c = 0; c < l->bands_len; c++) {
    if((p->cyl >= l->bands[c].startcyl) &&
       (p->cyl <= l->bands[c].endcyl)) 
      {
	return &l->bands[c];
      }
  }

  ddbg_assert2(0, "band not found!");
  return 0;
}


static int 
g1_st_pbn(struct dm_disk_if *d, struct dm_pbn *p) {
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_pbn p2 = *p;
  struct dm_layout_g1_band *b;

  while(!(b = find_band_pbn(l, &p2)) && p2.cyl >= 0) { p2.cyl--; }
  ddbg_assert_ptr(b);


  return b->blkspertrack;
}




static int g1_surfno_on_cyl(struct dm_layout_g1 *d,
			    struct dm_layout_g1_band *b, 
			    struct dm_pbn *p)
{

  switch(d->mapping) {
  case LAYOUT_CYLSWITCHONSURF1:
    if((p->cyl - b->startcyl) % 2) {
      return (d->disk->dm_surfaces - p->head - 1);
    }  
    break;

  case LAYOUT_CYLSWITCHONSURF2:
    if(p->cyl % 2) {
      return (d->disk->dm_surfaces - p->head - 1);
    }
    break;

  default: break;
  }
  return p->head;
}


static dm_angle_t
g1_map_pbn_skew(struct dm_disk_if *d, 
		struct dm_pbn *p)
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_pbn pbn = *p;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int trackswitches;

  dm_angle_t result = b->firstblkangle;

  int slipoffs = 0;



  switch (l->mapping) {
  case LAYOUT_NORMAL:
  case LAYOUT_CYLSWITCHONSURF1:
  case LAYOUT_CYLSWITCHONSURF2:
    {
      //      int c;
      // relative to the start of the zone...
      int cylno = p->cyl - b->startcyl;
      pbn.cyl -= b->startcyl;
      pbn.head = g1_surfno_on_cyl(l, b, p);
      
      trackswitches = pbn.head + (d->dm_surfaces - 1) * pbn.cyl;

      result = addmult(result, trackswitches, b->trackskew, b->trkspace);
      result = addmult(result, pbn.cyl, b->cylskew, b->trkspace);

      /* The following assumes that slips also push skew forward in
       * some schemes 
       */
      if ((issectpercyl(l)) 
	  || (l->sparescheme == SECTPERTRACK_SPARING)) {
	
	int tracks = cylno * d->dm_surfaces;
	int cutoff;
	int i;
	
	if (l->sparescheme == SECTPERTRACK_SPARING) {
	  tracks += p->head;
	}

	// not quite accurate; the cutoff should be the lbn
	// corresponding to p
	cutoff = tracks * b->blkspertrack;
	for (i = 0; i < b->numslips; i++) {
	  if (b->slip[i] < cutoff) {
	    slipoffs++;
	  }
	}
      }
      result = addmult(result, slipoffs, b->sector_width, b->trkspace);
      
    }
    break;


  default:
    ddbg_assert(0);
  }

  result = addmult(result, pbn.sector, b->sector_width, b->trkspace);

  return result;
}
		     

static dm_angle_t 
g1_get_track_0l(struct dm_disk_if *d, 
		struct dm_mech_state *track)
{
  //  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_pbn pbn;
  //  struct dm_layout_g1_band *b;
  //  int trackswitches;
  //  dm_angle_t result;
  //  int slipoffs = 0;
  
  pbn.head = track->head;
  pbn.cyl = track->cyl;
  pbn.sector = 0;

  return d->layout->dm_pbn_skew(d, &pbn);
}




static void
g1_convert_ptoa(struct dm_disk_if *d, 
		struct dm_pbn *p,
		dm_angle_t *angle,
		dm_angle_t *width)
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_pbn pbn = *p;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);

  dm_angle_t result = b->firstblkangle;

  int slipoffs = 0;

  p = &pbn;

  switch (l->mapping) {
  case LAYOUT_NORMAL:
  case LAYOUT_CYLSWITCHONSURF1:
  case LAYOUT_CYLSWITCHONSURF2:
    p->cyl -= b->startcyl;

    /* The following assumes that slips also push skew forward in
     * some schemes 
     */
    if ((issectpercyl(l)) 
	|| (l->sparescheme == SECTPERTRACK_SPARING)) {

      int tracks = p->cyl * d->dm_surfaces;
      int cutoff;
      int i;
 
      if (l->sparescheme == SECTPERTRACK_SPARING) {
	tracks += p->head;
      }
      cutoff = tracks * b->blkspertrack;
      for (i = 0; i < b->numslips; i++) {
	if (b->slip[i] < cutoff) {
	  slipoffs++;
	}
      }
    }
    result = addmult(result, slipoffs, b->sector_width, b->trkspace);
    break;


  default:
    ddbg_assert(0);
  }

  result = addmult(result, pbn.sector, b->sector_width, b->trkspace);
  

  if(angle) *angle = result;
  ddbg_assert2(width == 0, "unimplemented");
}

static dm_ptol_result_t
g1_convert_atop(struct dm_disk_if *d,
		struct dm_mech_state *a,
		struct dm_pbn *result)
{
  //  int trackswitches;
  struct dm_pbn track;
  struct dm_layout_g1_band *b;
  dm_angle_t angle;

  int slipoffs = 0;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;


  track.cyl = a->cyl;
  track.head = a->head;

  b = find_band_pbn(l, &track);

  angle = a->theta - b->firstblkangle;

  switch (l->mapping) {
  case LAYOUT_NORMAL:
  case LAYOUT_CYLSWITCHONSURF1:
  case LAYOUT_CYLSWITCHONSURF2:

    /* The following assumes that slips also push skew forward in
     * some schemes 
     */
    if ((issectpercyl(l)) 
	|| (l->sparescheme == SECTPERTRACK_SPARING)) {

      int tracks = track.cyl * d->dm_surfaces;
      int cutoff;
      int i;
 
      if (l->sparescheme == SECTPERTRACK_SPARING) {
	tracks -= track.head;
      }
      // count the slips
      cutoff = tracks * b->blkspertrack;
      for (i = 0; i < b->numslips; i++) {
	if (b->slip[i] < cutoff) {
	  slipoffs++;
	}
      }
    }
    // subtract out the slips
    angle -= addmult(0, slipoffs, b->sector_width, b->trkspace);
    break;


  default:
    ddbg_assert(0);
  }

  result->cyl = a->cyl;
  result->head = a->head;
  // rounds angle down to lower sector boundary
  result->sector = (angle / (b->sector_width)) % b->blkspertrack;

  // if skews are not in whole sectors, we need to round up here
  if((b->trackskew % b->sector_width)
     || (b->cylskew % b->sector_width)) {
    if((angle % b->sector_width) > 0) {
      result->sector = ((result->sector+1) % b->blkspertrack);
    }
  }

  // XXX ... check that angle isn't a defect
  return DM_OK;
}



static dm_ptol_result_t
g1_ptol_0t(struct dm_disk_if *d, 
	   struct dm_pbn *p,
	   int *remapsector)
{
  dm_angle_t zerol;
  //  long long max = ((long long)1 << 32);

  struct dm_mech_state a;
  struct dm_pbn p2;
    
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);

  a.theta = p->sector * b->sector_width + b->firstblkangle;
  zerol = d->layout->dm_get_track_zerol(d, (struct dm_mech_state *)p);

  if(zerol > a.theta) {
    //    a.theta -= b->trkspace;
  }
  a.theta -= zerol;

  a.head = p->head;
  a.cyl = p->cyl;

  d->layout->dm_convert_atop(d, &a, &p2);

  return d->layout->dm_translate_ptol(d, &p2, remapsector);
}

/*
 * The next several functions compute the lbn stored in a physical
 * media sector <cyl, surf, blk>, for different sparing/mapping
 * schemes.  The returned value is a proper lbn, or DM_REMAPPED if the
 * sector is a remapped defect, or DM_SLIPPED if the sector is a
 * slipped defect or an unused spare.
 */

static dm_ptol_result_t
g1_ptol_nosparing(struct dm_disk_if *d, 
		  struct dm_pbn *p,
		  int *remapsector)

{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  struct dm_pbn pbn = *p;

  p = &pbn;


  ddbg_assert_ptr(b);
  ddbg_assert(p->cyl >= 0);
  ddbg_assert(p->cyl >= b->startcyl);
  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head >= 0);
  ddbg_assert(p->head < d->dm_surfaces);
  ddbg_assert(p->sector >= 0);
  ddbg_assert(p->sector < b->blkspertrack);


  p->head = g1_surfno_on_cyl(l, b, p);
  
  lbn += (((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) * b->blkspertrack;
  lbn += p->sector - b->deadspace;

  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}



static dm_ptol_result_t
g1_ptol_sectpertrackspare(struct dm_disk_if *d, 
			  struct dm_pbn *p,
			  int *remapsector)
{
  int i;
  int lbnspercyl;
  int firstblkoncyl;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  struct dm_pbn pbn = *p;
  p = &pbn;

  ddbg_assert_ptr(b);
  ddbg_assert(p->cyl >= 0);
  ddbg_assert(p->cyl >= b->startcyl);
  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head >= 0);
  ddbg_assert(p->head < d->dm_surfaces);
  ddbg_assert(p->sector >= 0);
  ddbg_assert(p->sector < b->blkspertrack);


  p->head = g1_surfno_on_cyl(l, b, p);
  firstblkoncyl = (p->cyl - b->startcyl) * d->dm_surfaces * b->blkspertrack;
  p->sector += firstblkoncyl + (p->head * b->blkspertrack);
  for (i=(b->numdefects-1); i>=0; i--) {
    if (p->sector == b->defect[i]) {        /* Remapped bad block */
      return DM_REMAPPED;
    }
    if(p->sector == b->remap[i]) {
      if(remapsector) *remapsector = 1;
      p->sector = b->defect[i];
      break;
    }
  }
  for (i = (b->numslips-1); i >= 0; i--) {
    if (p->sector == b->slip[i]) {          /* Slipped bad block */
      return DM_SLIPPED;
    }
    if ((p->sector > b->slip[i]) 
	&& ((b->slip[i] / (b->blkspertrack * d->dm_surfaces)) == (p->cyl - b->startcyl)) 
	&& ((b->slip[i] % (b->blkspertrack * d->dm_surfaces)) == p->head)) 
      {
	p->sector--;
      }
  }

  if((p->sector % b->blkspertrack) >= (b->blkspertrack - b->sparecnt)) {   
    /* Unused spare block */
    return DM_SLIPPED;
  }

  /* recompute in case sector was remapped */
  p->cyl = p->sector / (b->blkspertrack * d->dm_surfaces);
  p->head = p->sector % (b->blkspertrack * d->dm_surfaces);
  p->head = p->head / b->blkspertrack;
  p->sector = p->sector % b->blkspertrack;

  lbnspercyl = (b->blkspertrack - b->sparecnt) * d->dm_surfaces;
  lbn += p->cyl * lbnspercyl;
  lbn += p->head * (b->blkspertrack - b->sparecnt);
  lbn += p->sector - b->deadspace;


  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}



static dm_ptol_result_t
g1_ptol_sectpercylspare(struct dm_disk_if *d, 
			struct dm_pbn *p,
			int *remapsector)
{
  int i;
  int lbnspercyl;
  int blkspercyl;
  int firstblkoncyl;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  struct dm_pbn pbn = *p;
  p = &pbn;

  /*    ddbg_assert(b); */
  /*    ddbg_assert(p->cyl >= 0); */
  /*    ddbg_assert(p->cyl >= b->startcyl); */
  /*    ddbg_assert(p->cyl < d->numcyls); */
  /*    ddbg_assert(surfaceno >= 0); */
  /*    ddbg_assert(surfaceno < d->dm_surfaces); */
  /*    ddbg_assert(p->sector >= 0); */
  /*    ddbg_assert(p->sector < b->blkspertrack); */


  //printf ("ptol_sectpercylspare: p->cyl %d, surfaceno %d, p->sector %d, lbn %d\n", p->cyl, surfaceno, p->sector, lbn);

  p->head = g1_surfno_on_cyl(l, b, p);
  blkspercyl = b->blkspertrack * d->dm_surfaces;
  lbnspercyl = blkspercyl - b->sparecnt;
  firstblkoncyl = (p->cyl - b->startcyl) * blkspercyl;
  p->sector += firstblkoncyl + (p->head * b->blkspertrack);

  for (i = (b->numdefects - 1); i >= 0; i--) {
    if (p->sector == b->defect[i]) {        /* Remapped bad block */
      return DM_REMAPPED;
    }
    if (p->sector == b->remap[i]) {
      if(remapsector) *remapsector = 1;
      p->sector = b->defect[i];
      break;
    }
  }

  for (i = (b->numslips - 1); i >= 0; i--) {
    if (p->sector == b->slip[i]) {          /* Slipped bad block */
      return DM_SLIPPED;
    }
    if (p->sector > b->slip[i]) {
      if ((b->slip[i] / blkspercyl) == (p->cyl - b->startcyl)) {
	p->sector--;
      } else if (issliptoend(l)) {
	lbn--;
      }
    }
  }

  /* check for unused spare blocks */
  if (((!isspareatfront(l)) && 
       ((p->sector % blkspercyl) >= lbnspercyl)) 
      ||
      ((isspareatfront(l)) && 
       ((p->sector % blkspercyl) < b->sparecnt)))
    {
      return DM_SLIPPED;
    }
   
  /* recompute in case sector was remapped */
  p->cyl = p->sector / blkspercyl;
  p->head = p->sector % blkspercyl;
  p->head = p->head / b->blkspertrack;
  p->sector = p->sector % b->blkspertrack;

  lbn += p->cyl * lbnspercyl;
  lbn += p->head * b->blkspertrack;
  lbn += p->sector - b->deadspace;
  lbn -= (isspareatfront(l)) ? b->sparecnt : 0;


  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}


static dm_ptol_result_t
g1_ptol_sectperrangespare(struct dm_disk_if *d, 
			  struct dm_pbn *p,
			  int *remapsector)
{
  int i;
  int lbnsperrange;
  int blksperrange;
  int blkspercyl;
  int firstblkoncyl;
  int rangeno;
  
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];

  struct dm_pbn pbn = *p;
  p = &pbn;


  ddbg_assert_ptr(b);
  ddbg_assert(p->cyl >= 0);
  ddbg_assert(p->cyl >= b->startcyl);
  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head >= 0);
  ddbg_assert(p->head < d->dm_surfaces);
  ddbg_assert(p->sector >= 0);
  ddbg_assert(p->sector < b->blkspertrack);


  p->head = g1_surfno_on_cyl(l, b, p);
  blkspercyl = b->blkspertrack * d->dm_surfaces;
  blksperrange = blkspercyl * l->rangesize;
  lbnsperrange = blksperrange - b->sparecnt;
  rangeno = (p->cyl - b->startcyl) / l->rangesize;

  firstblkoncyl = (p->cyl - b->startcyl) * blkspercyl;
  p->sector += firstblkoncyl + (p->head * b->blkspertrack);

  for (i=(b->numdefects-1); i>=0; i--) {
    if (p->sector == b->defect[i]) {        /* Remapped bad block */
      return DM_REMAPPED;
    }
    if (p->sector == b->remap[i]) {
      if(remapsector) *remapsector = 1;
      p->sector = b->defect[i];
      break;
    }
  }

  for (i=(b->numslips-1); i>=0; i--) {
    if (p->sector == b->slip[i]) {          /* Slipped bad block */
      return DM_SLIPPED;
    }
    if (p->sector > b->slip[i]) {
      if ((b->slip[i] / blksperrange) == rangeno) {
	p->sector--;
      }
    }
  }

  p->sector = p->sector % blksperrange;

  /* check for unused spare blocks */
  if (p->sector >= lbnsperrange) {
    return DM_SLIPPED;
  }

  lbn += rangeno * lbnsperrange;
  lbn += p->sector - b->deadspace;

  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}


static dm_ptol_result_t
g1_ptol_sectperzonespare(struct dm_disk_if *d, 
			 struct dm_pbn *p,
			 int *remapsector)
{
  int i;
  int blkspercyl;
  int firstblkoncyl;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];

  struct dm_pbn pbn = *p;
  p = &pbn;


  ddbg_assert_ptr(b);
  ddbg_assert(p->cyl >= 0);
  ddbg_assert(p->cyl >= b->startcyl);
  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head >= 0);
  ddbg_assert(p->head < d->dm_surfaces);
  ddbg_assert(p->sector >= 0);
  ddbg_assert(p->sector < b->blkspertrack);


  //printf ("ptol_sectperzonespare: p->cyl %d, p->head %d, p->sector %d, lbn %d\n", p->cyl, p->head, p->sector, lbn);

  /* compute p->sector within zone 
   * i.e. counting sectors from start of zone
   */
  p->head = g1_surfno_on_cyl(l, b, p);
  blkspercyl = b->blkspertrack * d->dm_surfaces;
  firstblkoncyl = (p->cyl - b->startcyl) * blkspercyl;
  p->sector += firstblkoncyl + (p->head * b->blkspertrack);

  for (i = (b->numdefects - 1); i >= 0; i--) {
    if (p->sector == b->defect[i]) {        /* Remapped bad block */
      return DM_REMAPPED;
    }
    if (p->sector == b->remap[i]) {
      if(remapsector) *remapsector = 1;
      p->sector = b->defect[i];
      break;
    }
  }

  for (i=(b->numslips-1); i>=0; i--) {
    if (p->sector == b->slip[i]) {          /* Slipped bad block */
      return DM_SLIPPED;
    }
    if (p->sector > b->slip[i]) {
      p->sector--;
    }
  }

  /* check for unused spare blocks */
  if (p->sector >= b->blksinband) {
    return DM_SLIPPED;
  }

  lbn += p->sector - b->deadspace;

  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}


static dm_ptol_result_t
g1_ptol_trackspare(struct dm_disk_if *d, 
		   struct dm_pbn *p,
		   int *remapsector)
{
  int i;
  int trackno;
  int lasttrack;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];

  struct dm_pbn pbn = *p;
  p = &pbn;

  ddbg_assert_ptr(b);
  ddbg_assert(p->cyl >= 0);
  ddbg_assert(p->cyl >= b->startcyl);
  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head >= 0);
  ddbg_assert(p->head < d->dm_surfaces);
  ddbg_assert(p->sector >= 0);
  ddbg_assert(p->sector < b->blkspertrack);


  p->head = g1_surfno_on_cyl (l, b, p);
  trackno = (p->cyl - b->startcyl) * d->dm_surfaces + p->head;
  for (i=(b->numdefects-1); i>=0; i--) {
    if (trackno == b->defect[i]) {   /* Remapped bad track */
      return DM_REMAPPED;
    }
    if (trackno == b->remap[i]) {
      trackno = b->defect[i];
      break;
    }
  }
  for (i=(b->numslips-1); i>=0; i--) {
    if (trackno == b->slip[i]) {     /* Slipped bad track */
      return DM_SLIPPED;
    }
    if (trackno > b->slip[i]) {
      trackno--;
    }
  }
  lasttrack = (b->blksinband + b->deadspace) / b->blkspertrack;
  if (trackno > lasttrack) {                 /* Unused spare track */
    return DM_SLIPPED;
  }
  lbn += p->sector + (trackno * b->blkspertrack) - b->deadspace;

  if(lbn < 0) {
    return DM_SLIPPED;
  }
  else {
    return lbn;
  }
}



/*
 *                                                                          
 * The next several functions compute the lbn boundaries (first and
 * last) for a given physical track <cyl, surf>, for different
 * sparing/mapping schemes.  The start and end values are proper lbns,
 * or DM_REMAPPED if the entire track consists of remapped defects, or
 * DM_SLIPPED if the entire track consists of slipped defects or
 * unused spare space.  Note: the lbns returned account for slippage
 * but purposefully ignore remapping of sectors to other tracks.
 *                                                                          
 * Note: the per-scheme functions take a pointer to the relevant band
 *  and the first lbn in that band, in addition to the basic
 *  parameters.  
 *                                                                          
 * These functions are organized by sparing scheme ... though we could
 * potentially have the cross-product of sparing and mapping schemes,
 * normal, cylcswitchonsurf1 and cylswitchonsurf2 all share the per-
 * sparing-scheme implementations.  (bucy 1/02)
 * */

/* Assume that we never slip sectors over track boundaries! */



dm_ptol_result_t
g1_track_boundaries_nosparing(struct dm_disk_if *d,
			      struct dm_pbn *p,
			      int *first_lbn,
			      int *last_lbn,
			      int *remapsector)
{

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  int temp_lbn = lbn;

  //  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;

  // this implementation lived in disk_get_lbn_boundaries_for_track

  p->head = g1_surfno_on_cyl(l, b, p);
  lbn += ((((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) * 
	  b->blkspertrack) - b->deadspace;

  if(first_lbn) {
    *first_lbn = 
      ((lbn + b->blkspertrack) <= temp_lbn) 
      ? DM_SLIPPED
      : max(lbn, temp_lbn);
  }

  if(last_lbn) {
    lbn += (b->blkspertrack - 1);
    *last_lbn = (lbn <= temp_lbn) ? DM_SLIPPED : lbn;
  }

  return DM_OK;
}

dm_ptol_result_t
g1_track_boundaries_sectpertrackspare(struct dm_disk_if *d,
				      struct dm_pbn *p,
				      int *first_lbn,
				      int *last_lbn,
				      int *remapsector)

{
  /* lbn equals first block in band */
  int i;
  //  int blkno;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  int temp_lbn = lbn;

  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;


  p->head = g1_surfno_on_cyl(l, b, p);
  lbn += ((((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) 
	  * lbnspertrack) - b->deadspace;

  if (first_lbn) {
    *first_lbn = ((lbn + lbnspertrack) <= temp_lbn) 
      ? DM_SLIPPED 
      : max(temp_lbn, lbn);
  }

  if (last_lbn) {
    lbn += (lbnspertrack - 1);
    *last_lbn = (lbn <= temp_lbn) ? DM_SLIPPED : lbn;
  }

  p->sector = (((p->cyl - b->startcyl) * d->dm_surfaces) 
	       + p->head) * b->blkspertrack;

  for (i = 0; i < b->numdefects; i++) {
    if ((p->sector <= b->defect[i]) 
	&& ((p->sector + b->blkspertrack) > b->defect[i])) {
      if(remapsector) *remapsector = 1;
    }
  }
  return DM_OK;
}


dm_ptol_result_t
g1_track_boundaries_sectpercylspare(struct dm_disk_if *d,
				    struct dm_pbn *p,
				    int *first_lbn,
				    int *last_lbn,
				    int *remapsector)

{
  /* lbn equals first block in band */
  int i;
  int blkno = 0;
  int lbnadd;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  int temp_lbn = lbn;

  //  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;


  if (first_lbn) {
    lbnadd = 0;
    // use brute force -- back translate each pbn to figure out startlbn
    for (blkno = 0; blkno < b->blkspertrack; blkno++) {
      int remapsector2 = 0;
      p->sector = blkno;
      *first_lbn = g1_ptol_sectpercylspare(d, p, &remapsector2);
					    
      if ((*first_lbn) == DM_REMAPPED) {
	lbnadd++;
      }

      if (remapsector2) {
      	*first_lbn = DM_SLIPPED;
      }
      if ((*first_lbn) >= 0) {
	*first_lbn = max(((*first_lbn) - lbnadd), temp_lbn);
	break;
      }
    }
    if (p->sector == b->blkspertrack) {
      *first_lbn = DM_SLIPPED;
    }
  }
  if (last_lbn) {
    // was 1, want the last block on this track rather than
    // the first of the next.  bucy 200208
    lbnadd = 0;
    /* use brute force -- back translate each pbn to figure out endlbn */
    for (p->sector = (b->blkspertrack-1); p->sector >= 0; p->sector--) {
      int remapsector2 = 0;
      *last_lbn = g1_ptol_sectpercylspare(d, p, &remapsector2);
      if ((*last_lbn) == DM_REMAPPED) {
	lbnadd++;
      }

      if (remapsector2) {
      	*last_lbn = DM_SLIPPED;
      }

      if ((*last_lbn) >= 0) {
	*last_lbn = (*last_lbn) + lbnadd;
	break;
      }
    }
    if (p->sector == DM_SLIPPED) {
      *last_lbn = DM_SLIPPED;
    }
  }
  p->head = g1_surfno_on_cyl(l, b, p);
  p->sector = (((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) * b->blkspertrack;
  for (i = 0; i < b->numdefects; i++) {
    if ((blkno <= b->defect[i]) && 
	((blkno + b->blkspertrack) > b->defect[i])) {
      // XXX global/remapsetor
      if(remapsector) *remapsector = 1;
    }
  }

  return DM_OK;
}


dm_ptol_result_t
g1_track_boundaries_sectperrangespare(struct dm_disk_if *d,
				      struct dm_pbn *p,
				      int *first_lbn,
				      int *last_lbn,
				      int *remapsector)

{
  /* lbn equals first block in band */
  int i;
  int blkno;
  int lbnadd;


  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  int temp_lbn = lbn;

  //  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;

  if (first_lbn) {
    lbnadd = 0;
    /* use brute force -- back translate each pbn to figure out startlbn */
    for (blkno = 0; blkno < b->blkspertrack; blkno++) {
      int remapsector2 = 0;
      p->sector = blkno;
      *first_lbn = g1_ptol_sectperrangespare(d,p,&remapsector2);
      if ((*first_lbn) == DM_REMAPPED) {
	lbnadd++;
      }

      if (remapsector2) {
      	*first_lbn = DM_SLIPPED;
      }
      if ((*first_lbn) >= 0) {
	*first_lbn = max(((*first_lbn) - lbnadd), temp_lbn);
	break;
      }
    }
    if (blkno == b->blkspertrack) {
      *first_lbn = DM_SLIPPED;
    }
  }
  if (last_lbn) {
    // was 1, want the last block on this track rather than
    // the first of the next.  bucy 200208
    lbnadd = 0;
    /* use brute force -- back translate each pbn to figure out endlbn */
    for (p->sector = (b->blkspertrack-1); p->sector >= 0; p->sector--) {
      int remapsector2 = 0;
      *last_lbn = g1_ptol_sectperrangespare(d,p,&remapsector2);
					    
      if ((*last_lbn) == DM_REMAPPED) {
	lbnadd++;
      }
      if (remapsector2) {
      	*last_lbn = DM_SLIPPED;
      }
      if ((*last_lbn) >= 0) {
	*last_lbn = (*last_lbn) + lbnadd;
	break;
      }
    }
    if (blkno == DM_SLIPPED) {
      *last_lbn = DM_SLIPPED;
    }
  }
  p->head = g1_surfno_on_cyl(l, b, p);
  p->sector = (((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) * b->blkspertrack;
  for (i = 0; i < b->numdefects; i++) {
    if ((p->sector <= b->defect[i]) && ((p->sector + b->blkspertrack) > b->defect[i])) {
      if(remapsector) *remapsector = 1;
    }
  }
  return DM_OK;
}



dm_ptol_result_t
g1_track_boundaries_sectperzonespare(struct dm_disk_if *d,
				     struct dm_pbn *p,
				     int *first_lbn,
				     int *last_lbn,
				     int *remapsector)
{
  /* lbn equals first block in band */
  int i;
  //  int blkno;
  int lbnadd;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];
  int temp_lbn = lbn;

  //  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;


  if (first_lbn) {
    lbnadd = 0;
    /* use brute force -- back translate each pbn to figure out startlbn */
    for (p->sector = 0; p->sector < b->blkspertrack; p->sector++) {
      int remapsector2 = 0;
      *first_lbn = g1_ptol_sectperzonespare(d, p, &remapsector2);
      if ((*first_lbn) == DM_REMAPPED) {
	lbnadd++;
      }
      if (remapsector2) {
      	*first_lbn = DM_SLIPPED;
      }
      if ((*first_lbn) >= 0) {
	*first_lbn = max(((*first_lbn) - lbnadd), temp_lbn);
	break;
      }
    }

    if(p->sector == b->blkspertrack) {
      *first_lbn = DM_SLIPPED;
    }

  } // find first_lbn

  if (last_lbn) {
    // was 1, want the last block on this track rather than
    // the first of the next.  bucy 200208
    lbnadd = 0;
    /* use brute force -- back translate each pbn to figure out endlbn */
    for (p->sector = (b->blkspertrack - 1); p->sector >= 0; p->sector--) 
      {
	// XXX global/remapsector
	int remapsector2 = 0;
	*last_lbn = g1_ptol_sectperzonespare(d,p,&remapsector2);
	if ((*last_lbn) == DM_REMAPPED) {
	  lbnadd++;
	}

	if (remapsector2) {
	  *last_lbn = DM_SLIPPED;
	}
	if ((*last_lbn) >= 0) {
	  *last_lbn = (*last_lbn) + lbnadd;
	  break;
	}
      }

    //    if (blkno == DM_SLIPPED) {
    //      *last_lbn = DM_SLIPPED;
    //    }

  } // find last_lbn

  p->head = g1_surfno_on_cyl(l,b,p);
  p->sector = (((p->cyl - b->startcyl) * d->dm_surfaces) + p->head) * b->blkspertrack;
  for (i = 0; i < b->numdefects; i++) {
    if ((p->sector <= b->defect[i]) && 
	((p->sector + b->blkspertrack) > b->defect[i])) 
      {
	if(remapsector) *remapsector = 1;
      }
  }
  return DM_OK;
}


dm_ptol_result_t
g1_track_boundaries_trackspare(struct dm_disk_if *d,
			       struct dm_pbn *p,
			       int *first_lbn,
			       int *last_lbn,
			       int *remapsector)
{
  int i;
  int trackno;
  int lasttrack;
  //  int blkno;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, p);
  int lbn = l->band_blknos[b->num];

  //  int lbnspertrack = b->blkspertrack - b->sparecnt;

  struct dm_pbn pbn = *p;
  p = &pbn;


  p->head = g1_surfno_on_cyl(l,b,p);
  trackno = ((p->cyl - b->startcyl) * d->dm_surfaces) + p->head;

  for(i = (b->numdefects - 1); i >= 0; i--) {
    if(trackno == b->defect[i]) {   /* Remapped bad track */
      lbn = DM_REMAPPED;
    }
    if(trackno == b->remap[i]) {
      trackno = b->defect[i];
      break;
    }
  }

  for(i = (b->numslips-1); i >= 0; i--) {
    if(trackno == b->slip[i]) {     /* Slipped bad track */
      lbn = DM_SLIPPED;
    }
    if(trackno > b->slip[i]) {
      trackno--;
    }
  }

  lasttrack = (b->blksinband + b->deadspace) / b->blkspertrack;

  if(trackno > lasttrack) {                 /* Unused spare track */
    lbn = DM_SLIPPED;
  }

  p->sector = lbn + (trackno * b->blkspertrack) - b->deadspace;



  if(first_lbn) {
    if(lbn < 0) {
      *first_lbn = lbn;
    }
    else {
      *first_lbn = ((p->sector + b->blkspertrack) <= lbn) 
	? DM_SLIPPED 
	: max(p->sector, lbn);
    }
  }

  if(last_lbn) {
    p->sector += (b->blkspertrack - 1);
    if(lbn < 0) {
      *last_lbn = lbn;
    }
    else {
      *last_lbn = (p->sector <= lbn) 
	? DM_SLIPPED 
	: p->sector;
    }
  }



  return DM_OK;
}



static dm_ptol_result_t
g1_ltop_0t(struct dm_disk_if *d, 
		  int lbn, 
		  dm_layout_maptype maptype,
		  struct dm_pbn *result,
		  int *remapsector)
{
  //  long long max = (long long)1 << 32;
  dm_angle_t a;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);  

  d->layout->dm_translate_ltop(d,lbn,maptype,result,remapsector);
  a = d->layout->dm_pbn_skew(d,result);

  result->sector = (a / b->sector_width) % b->blkspertrack;
  if((a % b->sector_width) > 0) {
    result->sector = (result->sector + 1) % b->blkspertrack;
  }
  return DM_OK;
}


/*
 * The next several functions compute the physical media location to
 * which a given lbn (blkno) is mapped for different sparing/mapping
 * schemes.  The returned value is a proper lbn, or DM_REMAPPED if the
 * sector is a remapped defect, or DM_SLIPPED if the sector is a slipped
 * defect or an unused spare.
 */

/*
 * NOTE: No slipping beyond the end of a track is allowed.  The
 * following code will produce incorrect results if this rule is
 * violated.  To fix this, trackno needs to be recomputed after slips
 * and slips on immediately previous tracks need to be accounted for.
 * Low Priority.  
 */

static dm_ptol_result_t
g1_ltop_nosparing(struct dm_disk_if *d, 
		  int lbn, 
		  dm_layout_maptype maptype,
		  struct dm_pbn *result,
		  int *remapsector)
{
  int blkspertrack;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);
  struct dm_pbn pbn = *result;
  result = &pbn;

  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  blkspertrack = b->blkspertrack;
  result->cyl = lbn / (blkspertrack * d->dm_surfaces) + b->startcyl;

  result->cyl = (lbn / blkspertrack) % d->dm_surfaces;
  result->cyl = g1_surfno_on_cyl(l,b,result);
  result->sector = lbn % blkspertrack;

  return DM_OK;
}


static dm_ptol_result_t
g1_ltop_sectpertrackspare(struct dm_disk_if *d, 
			  int lbn, 
			  dm_layout_maptype maptype,
			  struct dm_pbn *result,
			  int *remapsector)
{
  int i, trackno;
  int firstblkontrack = -1;
  

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);

  int blkspertrack = b->blkspertrack;
  int lbnspertrack = blkspertrack - b->sparecnt;

  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  trackno = lbn / lbnspertrack;




  lbn %= lbnspertrack;
  if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
    firstblkontrack = blkspertrack * trackno;
    for (i=0; i<b->numslips; i++) {
      if ((b->slip[i] >= firstblkontrack) && 
	  ((b->slip[i] - firstblkontrack) <= lbn)) 
	{
	  lbn++;
	}
    }
  }
  if(maptype == MAP_FULL) {
    for(i = 0; i < b->numdefects; i++) {
      if(b->defect[i] == (firstblkontrack + lbn)) {
	if(remapsector) *remapsector = 1;
	trackno = b->remap[i] / blkspertrack;
	firstblkontrack = blkspertrack * trackno;
	lbn = b->remap[i] % blkspertrack;
      }
    }
  }

  result->cyl = trackno/d->dm_surfaces + b->startcyl;

  result->head = trackno % d->dm_surfaces;
  result->head = g1_surfno_on_cyl(l,b,result);
  
  
  if(lbn >= blkspertrack) {
    //printf("Somehow computed a blkno that crosses to another track\n");
    //printf("This could be the result of too many slips, or of a bug\n");
    //printf("in the defect-related computations...\n");
    ddbg_assert(0);
  }
  else {
    result->sector = lbn;
  }

  return DM_OK;
}


static dm_ptol_result_t
g1_ltop_sectpercylspare(struct dm_disk_if *d, 
			int lbn, 
			dm_layout_maptype maptype,
			struct dm_pbn *result,
			int *remapsector)

{
  int i;
  int blkspertrack;
  int blkspercyl;
  int lbnspercyl;
  int firstblkoncyl = 0;
  int cyl;
  int slips = 0;


  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);

  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  blkspertrack = b->blkspertrack;
  blkspercyl = blkspertrack * d->dm_surfaces;
  lbnspercyl = blkspercyl - b->sparecnt;

  cyl = lbn / lbnspercyl;
  lbn = lbn % lbnspercyl;

  if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
    firstblkoncyl = cyl * blkspertrack * d->dm_surfaces;
    for (i=0; i<b->numslips; i++) {
      if (((issliptoend(l)) && 
	   ((b->slip[i]/blkspercyl) < cyl)) || 
	  ((b->slip[i] >= firstblkoncyl) && 
	   ((b->slip[i] - firstblkoncyl) <= (lbn+slips)))) 
	{
	  slips++;
	}
    }
  }
  lbn += slips;
  if (maptype == MAP_FULL) {
    for (i=0; i<b->numdefects; i++) {
      if ((b->defect[i] == (firstblkoncyl + lbn)) && 
	  (b->remap[i] != b->defect[i])) 
	{
	  if(remapsector) *remapsector = 1;
	  lbn = b->remap[i];
	  cyl = lbn / blkspercyl;
	  lbn = lbn % blkspercyl;
	  goto g1_ltop_sectpercylspare_done;
	}
    }
  }
   
  cyl += lbn / lbnspercyl;
  lbn = lbn % lbnspercyl;

 g1_ltop_sectpercylspare_done:
  
  result->cyl = cyl + b->startcyl;
  result->head = lbn / blkspertrack;
  result->head = g1_surfno_on_cyl(l,b,result);
  result->sector = lbn % blkspertrack;

  return DM_OK;
}


/* Assume no remapping/slipping out of a range. */

static dm_ptol_result_t
g1_ltop_sectperrangespare(struct dm_disk_if *d, 
			  int lbn, 
			  dm_layout_maptype maptype,
			  struct dm_pbn *result,
			  int *remapsector)
{
  int i;
  int blksperrange;
  int blkspercyl;
  int lbnsperrange;
  int firstblkinrange = 0;
  int rangeno;
  int slips = 0;


  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);

  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  blkspercyl = b->blkspertrack * d->dm_surfaces;
  blksperrange = blkspercyl * l->rangesize;
  lbnsperrange = blksperrange - b->sparecnt;
  rangeno = lbn / lbnsperrange;

  lbn = lbn % lbnsperrange;

  if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
    firstblkinrange = rangeno * blksperrange;
    for (i=0; i<b->numslips; i++) {
      if ((b->slip[i] >= firstblkinrange) && 
	  ((b->slip[i] - firstblkinrange) <= (lbn+slips))) {
	slips++;
      }
    }
  }
  lbn += slips;
  if (maptype == MAP_FULL) {
    for (i=0; i<b->numdefects; i++) {
      if ((b->defect[i] == (firstblkinrange + lbn)) && 
	  (b->remap[i] != b->defect[i])) {
	// XXX global/remapsetor
	if(remapsector) *remapsector = 1;
	lbn = b->remap[i];
	lbn = lbn % blksperrange;
	break;
      }
    }
  }

  
  result->cyl = b->startcyl + 
    (rangeno * l->rangesize) + (lbn / blkspercyl);

  result->head = (lbn % blkspercyl) / b->blkspertrack;
  result->head = g1_surfno_on_cyl(l,b,result);
  result->sector = lbn % b->blkspertrack;

  return DM_OK;
}


static dm_ptol_result_t
g1_ltop_sectperzonespare(struct dm_disk_if *d, 
			 int lbn, 
			 dm_layout_maptype maptype,
			 struct dm_pbn *result,
			 int *remapsector)
{
  int i;
  int blkspercyl;
  int slips = 0;

  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);
  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
    for (i=0; i<b->numslips; i++) {
      if (b->slip[i] <= (lbn+slips)) {
	slips++;
      }
    }
  }
  lbn += slips;

  if (maptype == MAP_FULL) {
    for (i=0; i<b->numdefects; i++) {
      if ((b->defect[i] == lbn) && 
	  (b->remap[i] != b->defect[i])) {

	if(remapsector) *remapsector = 1;
	lbn = b->remap[i];
	break;
      }
    }
  }

  blkspercyl = b->blkspertrack * d->dm_surfaces;

  result->cyl = b->startcyl + lbn / blkspercyl;
  result->head = (lbn % blkspercyl) / b->blkspertrack;
  result->head = g1_surfno_on_cyl(l,b,result);
  result->sector = lbn % b->blkspertrack;

  return DM_OK;
}


/*
 * NOTE: The total number of allowable slips and remaps per band is
 * equal to the number of spare tracks per band.  The following code
 * will produce incorrect results if this rule is violated.  To fix
 * this, b needs to be re-calculated after the detection of a
 * slip or spare.  Also, slips on immediately previous bands need to
 * be accounted for.  Lastly, the mismatch in sectors per track
 * between zones must be handled.  
 * Extremely low priority.  
 */

static dm_ptol_result_t
g1_ltop_trackspare(struct dm_disk_if *d, 
		   int lbn, 
		   dm_layout_maptype maptype,
		   struct dm_pbn *result,
 		   int *remapsector)

{
  int i;
  int blkspertrack;
  int trackno;


  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_lbn(l, lbn);
  lbn -= l->band_blknos[b->num];
  lbn += b->deadspace;

  blkspertrack = b->blkspertrack;
  trackno = lbn/blkspertrack;
  if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
    for (i=0; i<b->numslips; i++) {
      if (b->slip[i] <= trackno) {
	trackno++;
      }
    }
  }
  if (maptype == MAP_FULL) {
    for (i=0; i<b->numdefects; i++) {
      if (b->defect[i] == trackno) {
	trackno = b->remap[i];
	break;
      }
    }
  }

  result->cyl = (trackno/d->dm_surfaces) + b->startcyl;
  result->head = trackno % d->dm_surfaces;
  result->head = g1_surfno_on_cyl(l,b,result);
  result->sector = lbn % blkspertrack;

  return DM_OK;
}


static dm_ptol_result_t
g1_seek_distance(struct dm_disk_if *d,
		 int start_lbn,
		 int end_lbn)
{
  struct dm_pbn p1, p2;
  int rv;

  rv = d->layout->dm_translate_ltop(d, start_lbn, MAP_NONE, &p1, 0);
  if(rv != DM_OK) {
    return rv;
  }

  rv = d->layout->dm_translate_ltop(d, end_lbn, MAP_NONE, &p2, 0);
  if(rv != DM_OK) {
    return rv;
  }

  
  return abs(p2.cyl - p1.cyl);
}


static dm_angle_t
g1_get_sector_width(struct dm_disk_if *d,
		    struct dm_pbn *track,
		    int num)
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *b = find_band_pbn(l, track);

  dm_angle_t result;

  if(num > b->blkspertrack) {
    return 0;
  }

  result = b->sector_width;
  result *=  num;
  return result;
}

static dm_angle_t
g1_lbn_offset(struct dm_disk_if *d, int lbn1, int lbn2)
{
  struct dm_pbn pbn1, pbn2;
  dm_angle_t a1, a2;

  //  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  //  struct dm_layout_g1_band *b1 = find_band_lbn(l, lbn1);
  //  struct dm_layout_g1_band *b2 = find_band_lbn(l, lbn2);
  
  d->layout->dm_translate_ltop(d, lbn1, MAP_FULL, &pbn1, 0);
  d->layout->dm_translate_ltop(d, lbn2, MAP_FULL, &pbn2, 0);

  a1 = d->layout->dm_pbn_skew(d, &pbn1);
  a2 = d->layout->dm_pbn_skew(d, &pbn2);

  return a2 - a1;
}



static int 
layout_g1_marshaled_len(struct dm_disk_if *d) {
  int c;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  int result = sizeof(struct dm_marshal_hdr) + sizeof(struct dm_layout_g1); 

  // band blknos
  result += l->bands_len * sizeof(int);

  // add the zones
  result += l->bands_len * sizeof(struct dm_layout_g1_band);
  for(c = 0; c < l->bands_len; c++) {
    struct dm_layout_g1_band *b = &l->bands[c];
    result += b->numslips * sizeof(int);
    // defects and remapped locations
    result += b->numdefects * 2 * sizeof(int);
  }
  
  return result;
}

static void *
layout_g1_marshal(struct dm_disk_if *d, char *buff) {
  int c;
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  char *ptr = buff;
  struct dm_marshal_hdr *hdr = (struct dm_marshal_hdr *)buff;
  hdr->type = DM_LAYOUT_G1_TYP;
  hdr->len = layout_g1_marshaled_len(d);
  
  ptr += sizeof(struct dm_marshal_hdr);
  memcpy(ptr, d->layout, sizeof(struct dm_layout_g1));

  // fix function pointers here
  marshal_fns((void **)&l->hdr, 
	       sizeof(l->hdr) / sizeof(void *),
	       ptr,
	       DM_LAYOUT_G1_TYP);

  ptr += sizeof(struct dm_layout_g1);

 
  // now do the zones
  for(c = 0; c < l->bands_len; c++) {
    struct dm_layout_g1_band *b = &l->bands[c];
    memcpy(ptr, b, sizeof(struct dm_layout_g1_band));
    ptr += sizeof(struct dm_layout_g1_band);

    // slips
    memcpy(ptr, b->slip, b->numslips * sizeof(int));
    ptr += b->numslips * sizeof(int);

    // defect locations
    memcpy(ptr, b->defect, b->numdefects * sizeof(int));
    ptr += b->numdefects * sizeof(int);

    // defect remaps
    memcpy(ptr, b->remap, b->numdefects * sizeof(int));
    ptr += b->numdefects * sizeof(int);
  }

  // do the band blknos array
  memcpy(ptr, l->band_blknos, l->bands_len * sizeof(int));
  ptr += l->bands_len * sizeof(int);

  return ptr;
}

char *
layout_g1_unmarshal(struct dm_marshal_hdr *hdr, 
		     void **result,
		     void *parent)
{
  int c;
  struct dm_layout_g1 *l = malloc(sizeof(struct dm_layout_g1));
  char *ptr = (char *)hdr;
  
  ddbg_assert(hdr->type == DM_LAYOUT_G1_TYP);
  
  ptr += sizeof(struct dm_marshal_hdr);

  memcpy(l, ptr, sizeof(struct dm_layout_g1));
  unmarshal_fns((void **)&l->hdr,
		 sizeof(l->hdr) / sizeof(void*),
		 ptr,
		 DM_LAYOUT_G1_TYP);

  ptr += sizeof(struct dm_layout_g1);
  
  l->bands = malloc(l->bands_len * sizeof(struct dm_layout_g1_band));

  // now do the zones
  for(c = 0; c < l->bands_len; c++) {
    struct dm_layout_g1_band *b = &l->bands[c];
    memcpy(b, ptr, sizeof(struct dm_layout_g1_band));
    ptr += sizeof(struct dm_layout_g1_band);

    // slips
    if(b->numslips != 0) {
      b->slip = malloc(b->numslips * sizeof(int));
      memcpy(b->slip, ptr, b->numslips * sizeof(int));
      ptr += b->numslips * sizeof(int);
    }

    // defect locations
    if(b->numdefects != 0) {
      b->defect = malloc(b->numdefects * sizeof(int));
      memcpy(b->defect, ptr, b->numdefects * sizeof(int));
      ptr += b->numdefects * sizeof(int);

      // defect remaps
      b->remap = malloc(b->numdefects * sizeof(int));
      memcpy(b->remap, ptr, b->numdefects * sizeof(int));
      ptr += b->numdefects * sizeof(int);
    }
  }

  // do the band blknos array
  l->band_blknos = malloc(l->bands_len * sizeof(int));
  memcpy(l->band_blknos, ptr, l->bands_len * sizeof(int));
  ptr += l->bands_len * sizeof(int);


  l->disk = parent;

  *result = l;
  return ptr;
}


// returns the number of zones for the layout
static int 
g1_get_numzones(struct dm_disk_if *d) 
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  return l->bands_len;
}

// Fetch the info about the nth zone and store it in the result
// parameter z.  Returns 0 on success, -1 on error (bad n)
// n should be within 0 and (dm_get_numzones() - 1)
static int 
g1_get_zone(struct dm_disk_if *d, 
	    int n, 
	    struct dm_layout_zone *result)
{
  struct dm_layout_g1 *l = (struct dm_layout_g1 *)d->layout;
  struct dm_layout_g1_band *z = (struct dm_layout_g1_band *)0;

  // check args
  if(n < 0 || n >= l->bands_len) { return -1; }

  z = &l->bands[n];

  result->spt = z->blkspertrack;
  result->lbn_low = l->band_blknos[n];
  result->lbn_high = l->band_blknos[n] + z->blksinband - 1;

  result->cyl_low = z->startcyl;
  result->cyl_high = z->endcyl;

  return 0;
}


// any function that appears in the interface must be listed here
void *layout_g1_fns[] = {
  g1_ltop_nosparing,
  g1_ltop_sectpertrackspare,  
  g1_ltop_sectpercylspare,
  g1_ltop_sectperrangespare,
  g1_ltop_sectperzonespare,
  g1_ltop_trackspare,

  g1_ltop_0t,

  g1_ptol_nosparing,
  g1_ptol_sectpertrackspare,
  g1_ptol_sectpercylspare,
  g1_ptol_sectperrangespare,
  g1_ptol_sectperzonespare,
  g1_ptol_trackspare,

  g1_ptol_0t,

  g1_st_lbn,
  g1_st_pbn,

  g1_track_boundaries_nosparing,
  g1_track_boundaries_sectpertrackspare,
  g1_track_boundaries_sectpercylspare,
  g1_track_boundaries_sectperrangespare,
  g1_track_boundaries_sectperzonespare,
  g1_track_boundaries_trackspare,

  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa,
  g1_convert_atop,
  g1_get_sector_width,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_lbn_offset,
  g1_get_numzones,
  g1_get_zone
};

struct dm_marshal_module dm_layout_g1_marshal_mod =
{ 
  layout_g1_unmarshal, 
  layout_g1_fns, 
  sizeof(layout_g1_fns) / sizeof(void *) 
};


// instances of interface struct for the various sparing schemes

struct dm_layout_if g1_layout_nosparing = {
  g1_ltop_nosparing,
  g1_ltop_0t,
  g1_ptol_nosparing,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_nosparing,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa, // ptoa
  g1_convert_atop, // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};

struct dm_layout_if g1_layout_tracksparing = {
  g1_ltop_trackspare,
  g1_ltop_0t,
  g1_ptol_trackspare,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_trackspare,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa, // ptoa
  g1_convert_atop, // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};

struct dm_layout_if g1_layout_sectpertrackspare = {
  g1_ltop_sectpertrackspare,
  g1_ltop_0t,
  g1_ptol_sectpertrackspare,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_sectpertrackspare,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa, // ptoa
  g1_convert_atop, // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};

struct dm_layout_if g1_layout_sectpercylspare = {
  g1_ltop_sectpercylspare,
  g1_ltop_0t,
  g1_ptol_sectpercylspare,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_sectpercylspare,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa, // ptoa
  g1_convert_atop, // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};

struct dm_layout_if g1_layout_sectperrangespare = {
  g1_ltop_sectperrangespare,
  g1_ltop_0t,
  g1_ptol_sectperrangespare,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_sectperrangespare,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa,  // ptoa
  g1_convert_atop,  // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};

struct dm_layout_if g1_layout_sectperzonespare = {
  g1_ltop_sectperzonespare,
  g1_ltop_0t,
  g1_ptol_sectperzonespare,
  g1_ptol_0t,
  g1_st_lbn,
  g1_st_pbn,
  g1_track_boundaries_sectperzonespare,
  g1_seek_distance,
  g1_map_pbn_skew,
  g1_get_track_0l,
  g1_convert_ptoa, // ptoa
  g1_convert_atop,  // atop
  g1_get_sector_width,
  g1_lbn_offset,
  layout_g1_marshaled_len,
  layout_g1_marshal,
  g1_get_numzones,
  g1_get_zone
};




#if 0

// the old old old disksim loader code used this
static void bandcopy(struct dm_layout_g1_band **destbands, 
		     struct dm_layout_g1_band *srcbands, 
		     int numbands)
{
   int i;
   *destbands = malloc(numbands * sizeof(struct dm_layout_g1_band));
   memcpy(*destbands,srcbands,sizeof(struct dm_layout_g1_band));

   for (i=0; i<numbands; i++) {
      (*destbands)[i].slip = malloc (srcbands[i].numslips * sizeof(int));
      (*destbands)[i].defect = malloc (srcbands[i].numdefects * sizeof(int));
      (*destbands)[i].remap = malloc (srcbands[i].numdefects * sizeof(int));

      memcpy((*destbands)[i].slip, 
	     srcbands[i].slip, 
	     srcbands[i].numslips * sizeof(int));
      memcpy((*destbands)[i].defect, 
	     srcbands[i].defect, 
	     srcbands[i].numdefects * sizeof(int));
      memcpy((*destbands)[i].remap, 
	     srcbands[i].remap, 
	     srcbands[i].numdefects * sizeof(int));
   }
}

#endif 



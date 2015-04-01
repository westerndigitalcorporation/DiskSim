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

#include "layout_g2.h"

#include <stdlib.h>


#define min(x,y) (x) < (y) ? (x) : (y)


static struct dm_layout_g2_zone *
find_zone_lbn(struct dm_disk_if *d,
	      int lbn)
{
  int c;
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  for(c = 0; c < l->zones_len; c++) {
    if((l->zones[c].lbnlow <= lbn) 
       && (lbn <= l->zones[c].lbnhigh)) 
      {
	return &l->zones[c];
      }
  }
  ddbg_assert(0);

  return 0;
}


static struct dm_layout_g2_zone *
find_zone_pbn(struct dm_disk_if *d,
	      struct dm_pbn *p)
{
  int c;
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  for(c = 0; c < l->zones_len; c++) {
    if((l->zones[c].cyllow <= p->cyl) 
       && (p->cyl <= l->zones[c].cylhigh)) 
      {
	return &l->zones[c];
      }
  }
  ddbg_assert(0);

  return 0;
}


// return 1 if they overlap, ..
static int
g2_nodecmp(const void *p1, const void *p2) 
{
  struct dm_layout_g2_node *n1 = (struct dm_layout_g2_node *)p1;
  struct dm_layout_g2_node *n2 = (struct dm_layout_g2_node *)p2;

  if(n1->lbn >= (n2->lbn + n2->len)) {
    return 1;
  }
  else if(n2->lbn >= (n1->lbn + n1->len)) {
    return -1;
  }
  else return 0;
}


static dm_ptol_result_t
ltop(struct dm_disk_if *d, 
     int lbn, 
     dm_layout_maptype mt,
     struct dm_pbn *result,
     int *remapsector)
{
  struct dm_layout_g2_node keynode, *rn;
  struct dm_layout_g2_zone *z = find_zone_lbn(d,lbn);
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;

  memset(&keynode, 0, sizeof(keynode));
  keynode.lbn = lbn;
  keynode.len = 1;

  rn = bsearch(&keynode, 
	       l->ltop_map, 
	       l->ltop_map_len, 
	       sizeof(struct dm_layout_g2_node),
	       g2_nodecmp);

  if(!rn) {
    return DM_NX;
  }
  else {
    *result = rn->loc;
    result->sector += (lbn - rn->lbn);
    return DM_OK;
  }
}


static dm_ptol_result_t
ltop_0t(struct dm_disk_if *d, 
	int lbn, 
	dm_layout_maptype mt,
	struct dm_pbn *result,
	int *remapsector)
{
  ddbg_assert(0);
  return 0;
}

  
static dm_ptol_result_t
ptol(struct dm_disk_if *d, 
     struct dm_pbn *p,
     int *remapsector)
{
  int i;
  struct dm_layout_g2_node *curr;

  struct dm_layout_g2_surf *track;
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;

  ddbg_assert(p->cyl < d->dm_cyls);
  ddbg_assert(p->head < d->dm_surfaces);

  curr = &l->ptol_map[p->cyl].surfaces[p->head].extents[0];
  for(i = 0; 
      i < l->ptol_map[p->cyl].surfaces[p->head].extents_len; 
      i++, curr++) 
  {
    if((curr->loc.cyl == p->cyl)
       && (curr->loc.head == p->head)
       && (p->sector >= curr->loc.sector)
       && (p->sector < (curr->loc.sector + curr->len)))
      
    {
      return curr->lbn + (p->sector - curr->loc.sector);
    }
  } 

  return DM_NX;
}

static dm_ptol_result_t
ptol_0t(struct dm_disk_if *d, 
	struct dm_pbn *p,
	int *remapsector)
{
  ddbg_assert(0);
  return 0;
}




static int 
trkcmp(const void *ptr1, const void *ptr2)
{
  const struct dm_pbn *p1 = ptr1;
  const struct dm_pbn *p2 = ptr2;

  if(p1->cyl < p2->cyl) return -1;
  else if(p2->cyl < p1->cyl) return 1;

  if(p1->head < p2->head) return -1;
  else if(p2->head < p1->head) return 1;

  return 0;
}


static dm_ptol_result_t
g2_defect_count(struct dm_disk_if *d,
		struct dm_pbn *track,
		int *result)
{
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  struct dm_pbn *p, *p1, *p2;

  if(!l->defects_len) {
    *result = 0;
    goto out;
  }

  p = bsearch(track,
	      l->defects, 
	      l->defects_len, 
	      sizeof(struct dm_pbn), 
	      trkcmp);

  if(!p) {
    *result = 0;
  }
  else {
    p1 = p2 = p;
    
    while(!trkcmp(p1, p)) p1--;  
    while(!trkcmp(p2, p)) p2++;  
    
    *result = p2 - p1 - 1;
  }

 out:

  return DM_OK;
}


static int
st_lbn(struct dm_disk_if *d,
       int lbn)
{
  struct dm_layout_g2_zone *z = find_zone_lbn(d, lbn);
  return z->st;
}

static int
st_pbn(struct dm_disk_if *d,
       struct dm_pbn *p)
{
  struct dm_pbn p2 = *p;
  struct dm_layout_g2_zone *z;

  // return st for the nearest (lower) zone if this cyl is unmapped
  while(!(z = find_zone_pbn(d, &p2)) && p2.cyl >= 0) { p2.cyl--; }
  ddbg_assert_ptr(NULL != z);

  return z->st;
}


static dm_ptol_result_t
track_boundaries(struct dm_disk_if *d,
		 struct dm_pbn *p,
		 int *l1,
		 int *l2,
		 int *remapsector)
{
  struct dm_pbn p1, p2;
  struct dm_layout_g2_zone *z = find_zone_pbn(d, p);

  p1 = *p;
  p2 = *p;

  p1.sector = 0;
  p2.sector = z->st;
  
  if(l1) {
    do { 
      *l1 = d->layout->dm_translate_ptol(d, &p1, remapsector);
      p1.sector++;
    } while((*l1 == DM_NX) && (p1.sector < z->st));
  }

  if(l2) {
    do {
      p2.sector--;
      *l2 = d->layout->dm_translate_ptol(d, &p2, remapsector);
    } while((*l2 == DM_NX) && p2.sector);
  }

  return DM_OK;
}

static dm_angle_t
sector_width(struct dm_disk_if *d,
	     struct dm_pbn *track,
	     int num)
{
  struct dm_layout_g2_zone *z = find_zone_pbn(d, track);
  return z->sector_width * num;
}



static dm_angle_t
pbn_skew_new(struct dm_disk_if *d,
	 struct dm_pbn *p)
{
  int i;
  int lasthead, lastcyl;

  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  struct dm_layout_g2_zone *z = find_zone_pbn(d, p);
  int fltop_ext = l->ptol_map[p->cyl].first_ltop_extent;

  dm_angle_t result = l->ptol_map[p->cyl].skew;


  lastcyl = l->ltop_map[fltop_ext].loc.cyl;
  lasthead = l->ltop_map[fltop_ext].loc.head;

  for(i = fltop_ext; i < l->ltop_map_len; i++) {
    if(l->ltop_map[i].loc.cyl != lastcyl) { 
      result += z->csskew;
      lastcyl = l->ltop_map[i].loc.cyl;
    }
    if(l->ltop_map[i].loc.head != lasthead) {
      result += z->hsskew;
      lasthead = l->ltop_map[i].loc.head;
    }

    if((l->ltop_map[i].loc.cyl == p->cyl)
       && (l->ltop_map[i].loc.head == p->head)) goto done;
  }
  ddbg_assert(0);

 done:

  result += p->sector * z->sector_width;

  return result;
}


static dm_angle_t
pbn_skew(struct dm_disk_if *d,
	 struct dm_pbn *p)
{
  int i;
  int lastcyl, lasthead;
  dm_angle_t result = 0;
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  struct dm_layout_g2_zone *z = find_zone_pbn(d, p);

  // count headswitch/cylswitch
  // find start of zone in ltop map
  // slooooow
  // precompute -- table of first index per zone
  for(i = 0; i < l->ltop_map_len; i++) {
    if(l->ltop_map[i].loc.cyl == z->cyllow) break;
  }

  if((l->ltop_map[i].loc.cyl == p->cyl)
     && (l->ltop_map[i].loc.head == p->head)) goto done;

  lastcyl = l->ltop_map[i].loc.cyl;
  lasthead = l->ltop_map[i].loc.head;
  
  // walk forward until we get to p
  // precompute: skew per cylinder
  for( ; i < l->ltop_map_len; i++) {
    if(l->ltop_map[i].loc.cyl != lastcyl) result += z->csskew;
    if(l->ltop_map[i].loc.head != lasthead) result += z->hsskew;
    if((l->ltop_map[i].loc.cyl == p->cyl)
       && (l->ltop_map[i].loc.head == p->head)) goto done;
  }
  ddbg_assert(0);
 done:
  result += p->sector * z->sector_width;

  return result;
}

static void
ptoa(struct dm_disk_if *d,
     struct dm_pbn *p,
     dm_angle_t *start,
     dm_angle_t *width)
{
  struct dm_layout_g2_zone *z = find_zone_pbn(d, p);

  if(start)
    *start = z->sector_width * p->sector;

  if(width)
    *start = z->sector_width;
}


static dm_ptol_result_t
atop(struct dm_disk_if *d,
     struct dm_mech_state *a,
     struct dm_pbn *result)
{
  struct dm_pbn pbn;
  struct dm_layout_g2_zone *z;
  result->head = a->head;
  result->cyl = a->cyl;

  pbn.head = a->head;
  pbn.cyl = a->cyl;

  z = find_zone_pbn(d, &pbn);

  result->sector = (a->theta / z->sector_width) % z->st;

  return DM_OK;
}


static dm_ptol_result_t
g2_seek_distance(struct dm_disk_if *d,
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


  // returns the number of zones for the layout
static int 
g2_get_numzones(struct dm_disk_if *d)
{
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  return l->zones_len;
}

// Fetch the info about the nth zone and store it in the result
// parameter z.  Returns 0 on success, -1 on error (bad n)
// n should be within 0 and (dm_get_numzones() - 1)
static int 
g2_get_zone(struct dm_disk_if *d, int n, struct dm_layout_zone *result)
{
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  struct dm_layout_g2_zone *z;

  if(result == 0) { return -1; }
  if(n < 0 || n >= l->zones_len) { return -1; }
  
  z = &l->zones[n];

  result->spt = z->st;

  result->lbn_low = z->lbnlow;
  result->lbn_high = z->lbnhigh;

  result->cyl_low = z->cyllow;
  result->cyl_high = z->cylhigh;

  
  return 0;
}


struct dm_layout_if layout_g2 = {
  ltop,
  ltop_0t,
  ptol,
  ptol_0t,
  st_lbn,
  st_pbn,
  track_boundaries,
  g2_seek_distance, //  seekdist,
  pbn_skew_new,
  0, // get_zerol
  ptoa, // ptoa
  atop, // atop
  sector_width,
  0, // lbn offset
  0, // marshall -- probably not implemented for g2
  0, // unmarshall
  g2_get_numzones,
  g2_get_zone,
  g2_defect_count
};

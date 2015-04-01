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

#include <libparam/libparam.h>
#include <libparam/bitvector.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "layout_g2.h"

#include "modules/dm_layout_g2_param.h"


extern struct dm_layout_if layout_g2;
typedef enum { REVOLUTIONS, SECTORS, NONE } skew_unit_t;
#include "modules/dm_layout_g2_zone_param.h"

static int defects_len = 0;
static struct dm_pbn *defects;


void
g2_load_zones(struct dm_layout_g2 *l, 
	      struct lp_list *zl);


// sort by ascending track
static int trackcmp(const void *p1,
		    const void *p2)
{
  struct dm_layout_g2_node *n1 = (struct dm_layout_g2_node *)p1;
  struct dm_layout_g2_node *n2 = (struct dm_layout_g2_node *)p2;

  if(n1->loc.cyl < n2->loc.cyl) {
    return -1;
  }
  else if(n1->loc.cyl > n2->loc.cyl) {
    return 1;
  }

  if(n1->loc.head < n2->loc.head) {
    return -1;
  }
  else if(n1->loc.head > n2->loc.head) {
    return 1;
  }

  if(n1->loc.sector < n2->loc.sector) {
    return -1;
  }
  else if(n1->loc.sector > n2->loc.sector) {
    return 1;
  }


  return 0;
}

static int layout_g2_loadmap(struct dm_layout_g2 *l) 
{
  FILE *fd;
  char junk[1024];
  char countstr[16];
  char *mapfile;

  int extentguess;
  int extents_ct = 0;

  struct dm_pbn pbn;
  int lbn,c,h,s,count;
  int lastc, lasth;
  int cyls, heads;
  int i, j;
  // index [cyl][head], number of extents for that track
  int **track_ext_counts;

  struct dm_layout_g2_node *curr;
  struct dm_layout_g2_node *track_extents;

  struct dm_pbn *currdefect;

  mapfile = lp_search_path(lp_cwd, l->mapfile);

  fd = fopen(mapfile, "r");
  ddbg_assert3(fd != 0, ("failed to open layout mappings file %s", l->mapfile));

  // ignore first 2 lines
  fgets(junk, sizeof(junk), fd);
  fgets(junk, sizeof(junk), fd);

  if(fscanf(fd, "%d cylinders, %*d rot, %d heads\n", &cyls, &heads) != 2) {
    ddbg_assert2(0, "*** error: layout_g2_loadmap: need <cyls> ... <heads>\n");
  }

  //  printf("*** layout_g2: %s %s\n", vendor, model);
  //  printf("*** layout_g2: %d %d\n", cyls, heads);

  extentguess = cyls * heads * 2; // 0t tracks
  extentguess += extentguess/4;         // if there are a lot of defects
  l->ltop_map = calloc(extentguess, sizeof(struct dm_layout_g2_node));
  curr = &l->ltop_map[0];

  l->ptol_map = calloc(cyls, sizeof(struct dm_layout_g2_cyl));

  for(i = 0; i < cyls; i++) {
    l->ptol_map[i].surfaces = calloc(heads, sizeof(struct dm_layout_g2_surf));
  }

  track_ext_counts = calloc(cyls, sizeof(int *));
  for(i = 0; i < cyls; i++) {
    track_ext_counts[i] = calloc(heads, sizeof(int));
  }

  curr = &l->ltop_map[0];
  // ltop map
  while(fscanf(fd, "lbn %d --> cyl %d, head %d, sect %d, %s %d\n",
	       &curr->lbn,
	       &curr->loc.cyl,
	       &curr->loc.head,
	       &curr->loc.sector,
	       countstr,
	       &curr->len) == 6) 
  {
    if(!strcmp(countstr, "seqcnt")) {
      curr->len++;
    }

    track_ext_counts[curr->loc.cyl][curr->loc.head]++;
    curr++;
    extents_ct++;
    l->ltop_map_len++;
  }

  // copy the ltop map and sort it by track
  track_extents = calloc(extents_ct, sizeof(struct dm_layout_g2_node));
  memcpy(track_extents, l->ltop_map, extents_ct * sizeof(struct dm_layout_g2_node));

  qsort(track_extents, extents_ct, sizeof(struct dm_layout_g2_node), 
	trackcmp);

  // ptol map
  curr = &track_extents[0];
  for(i = 0; i < cyls; i++) {
    for(j = 0; j < heads; j++) {
      int k;
      int extents_len = track_ext_counts[i][j];

      l->ptol_map[i].surfaces[j].extents = calloc(extents_len, sizeof(struct dm_layout_g2_node));
      l->ptol_map[i].surfaces[j].extents_len = extents_len;

      for(k = 0; k < extents_len; k++) {
	l->ptol_map[i].surfaces[j].extents[k] = *curr;
	curr++;
      }
    }
  }


  // free the extra extent list
  free(track_extents);

  //  ddbg_assert2(extents_ct > (cyls * heads), "EOF on layout mappings file??");

  //  l->extents_len = extents_ct;

  // populate the defect list
  defects = calloc(extents_ct >> 3, sizeof(struct dm_pbn));
  currdefect = &defects[0];
  while(fscanf(fd, "Defect at cyl %d, head %d, sect %d\n",
	       &currdefect->cyl,
	       &currdefect->head,
	       &currdefect->sector) == 3)
  {
    currdefect++;
    defects_len++;
  }




  fclose(fd);
  return 0;
}


static void 
precompute_skews(struct dm_disk_if *d)
{
  int i = 0;
  struct dm_layout_g2 *l = (struct dm_layout_g2 *)d->layout;
  struct dm_layout_g2_zone *z = &l->zones[0];
  
  for(i = 0; i < d->dm_cyls; i++) {
    l->ptol_map[i].first_ltop_extent = -1;
  }

  l->ptol_map[0].first_ltop_extent = 0;
  
  for(i = 0 ; i < l->ltop_map_len; i++) {
    int currcyl = l->ltop_map[i].loc.cyl;
  
    if(l->ptol_map[currcyl].first_ltop_extent == -1) {
      l->ptol_map[currcyl].first_ltop_extent = i;

      if(z->cylhigh < currcyl) {
	z++;
      }
      
      // start with the skew of the previous cylinder
      if(currcyl == z->cyllow) {
	l->ptol_map[currcyl].skew = z->zskew;
      }
      else {
	l->ptol_map[currcyl + 1].skew = l->ptol_map[currcyl].skew;
      }
    }

    if(l->ltop_map[i+1].loc.cyl != l->ltop_map[i].loc.cyl) {
      l->ptol_map[currcyl+1].skew += z->csskew;
    }
    else if(l->ltop_map[i+1].loc.head != l->ltop_map[i].loc.head) {
      l->ptol_map[currcyl+1].skew += z->hsskew;
    }
   
  }

}


// Defects are now loaded from the raw layout file itself.  bucy/20030714
//  int 
//  g2_load_defects(struct lp_list *l) {
//    int i;

//    defects_len = 0;
//    defects = calloc(l->values_len / 3, sizeof(struct dm_pbn));

//    for(i = 0; i < l->values_len / 3; i++) {
//      if(l->values[i*3]) {
//        defects[i].cyl = l->values[3*i]->v.i;
//        defects[i].head = l->values[3*i+1]->v.i;
//        defects[i].sector = l->values[3*i+2]->v.i;
//        defects_len++;
//      }
//    }

//    return 0;
//  }

static int 
pbncmp(const void *ptr1, const void *ptr2)
{
  const struct dm_pbn *p1 = ptr1;
  const struct dm_pbn *p2 = ptr2;

  if(p1->cyl < p2->cyl) return -1;
  else if(p2->cyl < p1->cyl) return 1;

  if(p1->head < p2->head) return -1;
  else if(p2->head < p1->head) return 1;

  if(p1->sector < p2->sector) return -1;
  else if(p2->sector < p1->sector) return 1;
  else return 0;
}


// sort by ascending PBNs
void g2_finish_defects(struct dm_layout_g2 *l) 
{
  l->defects = defects;

  qsort(l->defects, defects_len, sizeof(struct dm_pbn), pbncmp);
  
  l->defects_len = defects_len;

  defects = 0;
  defects_len = 0;
}

struct dm_layout_if *
dm_layout_g2_loadparams(struct lp_block *b, struct dm_disk_if *parent) {
  int i;
  int rv;

  struct dm_layout_g2 *result = malloc(sizeof(struct dm_layout_g2));
  memset(result, 0, sizeof(struct dm_layout_g2));

  //  #include "modules/dm_layout_g2_param.c"
  lp_loadparams(result, b, &dm_layout_g2_mod);

  rv = layout_g2_loadmap(result);
  ddbg_assert(rv == 0);
  free(result->mapfile); // strdup()ed in loader code
  result->mapfile = 0;

  // find the zone param

  result->hdr = layout_g2;

  parent->layout = (struct dm_layout_if *)result;

  // do the skew precomputation
  precompute_skews(parent);


  g2_finish_defects(result);

  return (struct dm_layout_if *)result;
}



static int
g2_load_zone(struct dm_layout_g2_zone *result, 
	     struct lp_block *b)
{

  //#include "modules/dm_layout_g2_zone_param.c"
  lp_loadparams(result, b, &dm_layout_g2_zone_mod);

  return 0; // avoid a warning
}

void
g2_load_zones(struct dm_layout_g2 *l, 
	      struct lp_list *zl)
{
  int i;
  int z = 0;

  l->zones_len = 0;

  for(i = 0; i < zl->values_len; i++) {
    if(zl->values[i] != 0) {
      l->zones_len++;
    }
  }

  l->zones = malloc(l->zones_len * sizeof(struct dm_layout_g2_zone));
  memset(l->zones, 0, l->zones_len * sizeof(struct dm_layout_g2_zone));
  
  for(i = 0; i < zl->values_len; i++) {
    if(!zl->values[i]) continue;

    g2_load_zone(&l->zones[z], zl->values[i]->v.b);
    z++;
  }

}


// dummy
int dm_layout_g2_zone_loadparams(struct lp_block *b)
{ 
  ddbg_assert(0); 

  return DM_OK;
}



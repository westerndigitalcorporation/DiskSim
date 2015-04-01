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

#include "layout_g4.h"
#include "layout_g4_private.h"

#include <libparam/libparam.h>
#include <libddbg/libddbg.h>
#include <stdlib.h>

#include "modules/dm_layout_g4_param.h"

// offset of element p in array base of size s
static int
aoffset(void *base, void *p, int s) {
  return (p - base) / s;
}

struct lp_list *
marshal_g4_remaps(struct remap *remaps, int n) {
  struct lp_list *l = lp_new_list();
  int i;

  l->linelen = 6;

  for(i = 0; i < n; i++) {
    struct lp_value *v;
    struct remap *ri = &remaps[i];

    v = lp_new_intv(ri->off);
    lp_list_add(l,v);
    v = lp_new_intv(ri->count);
    lp_list_add(l,v);
    v = lp_new_intv(ri->dest.cyl);
    lp_list_add(l,v);
    v = lp_new_intv(ri->dest.head);
    lp_list_add(l,v);
    v = lp_new_intv(ri->dest.sector);
    lp_list_add(l,v);
    v = lp_new_intv(ri->spt);
    lp_list_add(l,v);
  }


  return l;
}

struct lp_list *
marshal_g4_slip(struct slip *slips, int n) {
  struct lp_list *l = lp_new_list();
  int i = 0;

  l->linelen = 2;

  if(slips[0].off == 0 && slips[0].count == 0) {
    i = 1;
  }

  for( ; i < n; i++) {
    struct lp_value *v;
    v = lp_new_intv(slips[i].off);
    lp_list_add(l,v);
    v = lp_new_intv(slips[i].count - slips[i-1].count);
    lp_list_add(l,v);
  }

  return l;
}

struct lp_list *
marshal_g4_tp(struct track *t, int len) {
  struct lp_list *l = lp_new_list();
  int i;

  l->linelen = 3;

  for(i = 0; i < len; i++) {
    struct lp_value *v;

    v = lp_new_intv(t[i].low);
    lp_list_add(l, v);
    v = lp_new_intv(t[i].high);
    lp_list_add(l, v);
    v = lp_new_intv(t[i].spt);
    lp_list_add(l, v);
  }


  return l;
}

struct lp_list *
marshal_g4_idx_ent(struct idx_ent *e, 
		   struct idx *idx, 
		   struct track *track,
		   struct lp_list *l) 
{
  int i;
  struct lp_value *v;
  char *ctstr; // child type
  int coffset; // offset of child in its array

  printf("%s() lbn %d alen %f off %f\n", __func__, e->lbn,
	 dm_angle_itod(e->alen), dm_angle_itod(e->off));

  v = lp_new_intv(e->lbn);
  lp_list_add(l, v);
  v = lp_new_intv(e->cyl);
  lp_list_add(l, v);
  v = lp_new_doublev(dm_angle_itod(e->off));
  lp_list_add(l, v);

  v = lp_new_intv(e->len);
  lp_list_add(l, v);
  v = lp_new_intv(e->cyllen);
  lp_list_add(l, v);
  v = lp_new_doublev(dm_angle_itod(e->alen));
  lp_list_add(l, v);

  v = lp_new_intv(e->runlen);
  lp_list_add(l, v);
  v = lp_new_intv(e->cylrunlen);
  lp_list_add(l, v);

  switch(e->childtype) {
  case IDX:
    ctstr = "IDX";
    coffset = aoffset(idx, e->child.i, sizeof(*e->child.i));
    break;
  case TRACK:
    ctstr = "TRACK";
    coffset = aoffset(track, e->child.t, sizeof(*e->child.t));
    break;
  default: ddbg_assert(0); break;
  }

  v = lp_new_stringv(ctstr);
  lp_list_add(l, v);

  v = lp_new_intv(coffset);
  lp_list_add(l, v);

  if(e->childtype == TRACK) {
    v = lp_new_intv(e->head);
    lp_list_add(l, v);
  }

  return l;
}

struct lp_list *
marshal_g4_idx(struct idx *idx, int n, struct idx *idxen, struct track *track) {
  int i, j;
  struct lp_list *l = lp_new_list();

  l->linelen = 1;

  for(i = 0; i < n; i++) {
    struct idx *ii = &idx[i];
    struct lp_list *lent = lp_new_list();
    struct lp_value *vlent = lp_new_listv(lent);

    // this breaks if an index has a mixture of track and index
    // children
    if(idx[i].ents[0].childtype == TRACK) {
      lent->linelen = 11;
    }
    else {
      lent->linelen = 10;
    }

    for(j = 0; j < ii->ents_len; j++) {
      marshal_g4_idx_ent(&ii->ents[j],  idxen, track, lent);
    }

    lp_list_add(l, vlent);
  }


  return l;
}

struct lp_block *
marshal_layout_g4(struct dm_layout_g4 *lin) {

  struct lp_block *b = lp_new_block();
  struct lp_list *l;
  struct lp_value *v;
  struct lp_param *p;

  b->source_file = 0;
  b->name = 0;
  lp_lookup_type("dm_layout_g4", &b->type);

  l = marshal_g4_tp(lin->track, lin->track_len);
  v = lp_new_listv(l);
  p = lp_new_param("TP", 0, v);
  lp_add_param(&b->params, &b->params_len, p);

  l = marshal_g4_idx(lin->idx, lin->idx_len, lin->idx, lin->track);
  v = lp_new_listv(l);
  p = lp_new_param("IDX", 0, v);
  lp_add_param(&b->params, &b->params_len, p);

  l = marshal_g4_slip(lin->slips, lin->slips_len);
  v = lp_new_listv(l);
  p = lp_new_param("Slips", 0, v);
  lp_add_param(&b->params, &b->params_len, p);

  l = marshal_g4_remaps(lin->remaps, lin->remaps_len);
  v = lp_new_listv(l);
  p = lp_new_param("Remaps", 0, v);
  lp_add_param(&b->params, &b->params_len, p);

  return b;
}










#if 0
int **
do_ptol_map(struct rect *r, int surfs)
{
  int i;
  int acc = 0;
  int cyls = r->ents[0].cyl;
  struct rect_ent *ri;
  int **result;

  for(i = 0; i < r->ents_len; i++) {
    if(r->ents[i].cyl > cyls) {
      cyls = r->ents[i].cyl;
    }
  }

  cyls = cyls - r->ents[0].cyl + 1;

  result = calloc((cyls + 1) * surfs , sizeof(int));
  for(i = 0; i < cyls; i++) {
    result[i] = &result[(i+1) * surfs];
  }

  for(i = 0, ri = &r->ents[0]; i < r->ents_len; i++, ri++) {
    result[ri->cyl][ri->surf] = acc;
    acc += ri->len;
  }

  return result;
}

#endif

#define TP_FIELDS 3
int 
g4_load_tp(struct dm_layout_g4 *r,
	   struct lp_list *l)
{
  int i;
  
  ddbg_assert(r->track_len == 0);

#ifdef DEBUG_MODEL_G4
  dump_lp_list( l,  "g4_load_tp" );
#endif

  r->track_len = l->values_pop / TP_FIELDS;

  r->track = calloc(r->track_len, sizeof(*r->track));

  for(i = 0; i < r->track_len; i++) {
    r->track[i].low = l->values[TP_FIELDS*i]->v.i;
    r->track[i].high = l->values[TP_FIELDS*i+1]->v.i;
    r->track[i].spt = l->values[TP_FIELDS*i+2]->v.i;
    r->track[i].sw = dm_angle_dtoi(1.0 / r->track[i].spt);
  }

#ifdef DEBUG_MODEL_G4
  dump_g4_layout( r, "g4_load_tp" );
#endif

  return 0;
}


// this is now the minimum since they're variable lenth
#define IDX_FIELDS 10
int 
g4_load_idx(struct dm_layout_g4 *r,
	   struct lp_list *l)
{

  int i, j;
  // offset in l
  int loff = 0;
  
  r->idx_len = l->values_pop;

  r->idx = calloc(r->idx_len, sizeof(*r->idx));

  for(i = 0; i < r->idx_len; i++) {
    int reallen = 0;
    struct idx *opi = &r->idx[i];
    struct lp_list *li = l->values[i]->v.l;
    struct idx_ent *entj;

    loff = 0;
    
    // this is a little sloppy yields an upper bound, not an
    // exact number
    opi->ents_len = li->values_pop / IDX_FIELDS;
    opi->ents = calloc(opi->ents_len, sizeof(*opi->ents));

    for(j = 0, entj = opi->ents; 
	j < opi->ents_len && loff < li->values_pop; 
	j++, entj++) 
    {
      int off;

      entj->lbn = li->values[loff++]->v.i;
      entj->cyl = li->values[loff++]->v.i;
      entj->off = dm_angle_dtoi(li->values[loff++]->v.d);

      entj->len = li->values[loff++]->v.i;
      entj->cyllen = li->values[loff++]->v.i;
      entj->alen = dm_angle_dtoi(li->values[loff++]->v.d);

      entj->runlen = li->values[loff++]->v.i;
      entj->cylrunlen = li->values[loff++]->v.i;

      if(!strcmp(li->values[loff]->v.s, "TRACK")) {
	loff++;
	entj->childtype = TRACK;
	off = li->values[loff++]->v.i;
	ddbg_assert(off < r->track_len);
	entj->child.t = &r->track[off];
	entj->head = li->values[loff++]->v.i;
      }
      else if(!strcmp(li->values[loff]->v.s, "IDX")) {
	loff++;
	entj->childtype = IDX;
	// XXX weird recursion
	off = li->values[loff++]->v.i;
	ddbg_assert(off < r->idx_len);
	entj->child.i = &r->idx[off];
      }
      else {
	ddbg_assert(0);
      }

      reallen++;
    }

    opi->ents_len = reallen;
  }



  return 0;
}

#define SLIP_FIELDS 2
int 
g4_load_slips(struct dm_layout_g4 *r,
	      struct lp_list *l)
{
  int i;
  int tot = 0;
  r->slips_len = l->values_pop / SLIP_FIELDS;
  r->slips_len; // add "0" entry

  // allocate one extra; see below
  r->slips = calloc(r->slips_len+1, sizeof(*r->slips));

  for(i = 0; i < r->slips_len; i++) {
    r->slips[i].off = l->values[SLIP_FIELDS * i]->v.i;
    tot += l->values[SLIP_FIELDS * i + 1]->v.i;
    r->slips[i].count = tot;
  }

  // 0th lbn could be slipped
  if(r->slips[0].off > 0) { 
    memmove(r->slips + 1, r->slips, r->slips_len * sizeof(*r->slips));
    r->slips[0].off = 0;
    r->slips[0].count = 0;
    r->slips_len++;
  }

  return 0;
}

#define REMAP_FIELDS 6
int 
g4_load_remaps(struct dm_layout_g4 *r,
	       struct lp_list *l)
{
  int i;
  r->remaps_len = l->values_pop / REMAP_FIELDS;

  r->remaps = calloc(r->remaps_len, sizeof(*r->remaps));

  for(i = 0; i < r->remaps_len; i++) {
    r->remaps[i].off = l->values[REMAP_FIELDS * i]->v.i;
    r->remaps[i].count = l->values[REMAP_FIELDS * i + 1]->v.i;
    r->remaps[i].dest.cyl = l->values[REMAP_FIELDS * i + 2]->v.i;
    r->remaps[i].dest.head = l->values[REMAP_FIELDS * i + 3]->v.i;
    r->remaps[i].dest.sector = l->values[REMAP_FIELDS * i + 4]->v.i;
    r->remaps[i].spt = l->values[REMAP_FIELDS * i + 5]->v.i;
  }


  return 0;
}



struct dm_layout_if *
dm_layout_g4_loadparams(struct lp_block *b, struct dm_disk_if *parent)
{
  int i;
  struct dm_layout_g4 *result = calloc(1, sizeof(*result));
  
  result->hdr = layout_g4;
  result->parent = parent;

#ifdef DEBUG_MODEL_G4
  dump_lp_block( b, "dm_layout_g4_loadparams" );
#endif

  lp_loadparams(result, b, &dm_layout_g4_mod);

  // XXX ick
  result->root = &result->idx[result->idx_len - 1];

  // XXX shouldn't do this
  parent->layout = (struct dm_layout_if *)result;

  return (struct dm_layout_if *)result;
}

// End of file

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

#ifndef _DM_LAYOUT_G4_H
#define _DM_LAYOUT_G4_H


#include "dm.h"

struct pat;
struct rect;

struct slip {
  int off;
  int count;  // positive for slips, negative for spares
};

// Integrate skew data with ltop tree or have a separate 0l0t tree?
// For things that reset to zero periodically, etc, this may save
// copies of structures or more insts... "fake zones" again ... 


struct remap {
  int off;
  int count;
  struct dm_pbn dest;

  // Have to put phys spt here too.  If we had a separate 0l->0t tree...
  int spt;
  dm_angle_t sw;  // sector_width
};

struct track;
struct idx;

typedef enum { IDX, TRACK } g4_node_t;
union g4_node { struct track *t; struct idx *i; void *x; };

struct track {
  int low;
  int high;

  int spt;
  dm_angle_t sw;  // sector_width
};


struct idx_ent {

  // Offset

  int lbn;      // absolute if top-level, relative otherwise
  int cyl;      // ditto

  // skew to the start of each inst relative to the start of the
  // enclosing pat.
  dm_angle_t off;

  // Child Length

  // if this is RLE'd, the offset of the next inst relative to the
  // start of this one.  I don't think we need arunlen because
  // we never need to know the number of radians that a thing covers...
  int len;          // the number of lbns in one instance of the child
  int cyllen;       // the number of cylinders in one instance of the child
  dm_angle_t alen;  // the total rotational angle of one instance of the child


  // Run Length -- indicates RLE if runlen > len, etc

  int runlen;       // the number of lbns this entry covers
  int cylrunlen;    // the number of cylinders this entry covers


  g4_node_t childtype;  // whether the child is an index node (IDX) or track pattern (TP)
  union g4_node child;
  int head; // only for childtype == TRACK
};


struct idx {
  // invariant (?) .. all of the ents children are the same type --
  // IDX or TRACK
  struct idx_ent *ents;
  int ents_len;
};

struct dm_layout_g4 {
  struct dm_layout_if hdr;

  struct dm_disk_if *parent;

  struct track *track;
  int track_len;

  struct idx *idx;
  int idx_len;


  // In principle, we could have some screwy layout that the lbn order
  // isn't the same as the cylinder order, in which case we'd have 2
  // pat trees, one sorted by lbn and the other by cyl.
  struct idx *root; // always an OP

  struct slip *slips;
  int slips_len;

  struct remap *remaps;
  int remaps_len;


};

extern struct dm_layout_if layout_g4;

#endif    // _DM_LAYOUT_G4_H

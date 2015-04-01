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

#ifndef _DM_LAYOUT_G2_H
#define _DM_LAYOUT_G2_H

#include "dm.h"

struct dm_layout_g2 {
  struct dm_layout_if hdr;

  struct dm_layout_g2_node *ltop_map;
  struct dm_layout_g2_cyl *ptol_map;
  int ltop_map_len;

  struct dm_layout_g2_zone *zones;
  int zones_len;

  char *mapfile; // name of raw layout file
  dm_skew_unit_t skew_units;


  // list of defect locations
  // currently only used for dm_defect_count()
  struct dm_pbn *defects;
  int defects_len;
};


struct dm_layout_g2_node {
  int lbn;
  //  int cyl;
  //  int head;
  struct dm_pbn loc; // location of lbnlow
  int len;
};

struct dm_layout_g2_cyl {
  struct dm_layout_g2_surf *surfaces;
  int surfaces_len;
  dm_angle_t skew;
  int first_ltop_extent; // first entry in ltop_map for this cyl
};

struct dm_layout_g2_surf {
  int lbnlow;
  int lbnhigh;
  // pointers back to ltop map extents
  struct dm_layout_g2_node *extents;
  int extents_len;
};


struct dm_layout_g2_zone {
  int st;
  int cyllow;
  int cylhigh;

  int lbnlow;
  int lbnhigh;

  dm_angle_t csskew; // cyl switch
  dm_angle_t hsskew; // head switch
  dm_angle_t zskew;  // skew to beginning of zone ("first block offset")

  dm_angle_t sector_width;

  dm_skew_unit_t skew_units;
};



#endif  // _DM_LAYOUT_G2_H





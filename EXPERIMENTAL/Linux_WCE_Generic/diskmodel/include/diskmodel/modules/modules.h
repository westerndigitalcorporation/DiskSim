
#ifndef _DM_MODULES_H
#define _DM_MODULES_H   


#include "dm_disk_param.h"
#include "dm_layout_g1_param.h"
#include "dm_layout_g1_zone_param.h"
#include "dm_layout_g2_param.h"
#include "dm_layout_g2_zone_param.h"
#include "dm_layout_g4_param.h"
#include "dm_mech_g1_param.h"


static struct lp_mod *dm_mods[] = {
 &dm_disk_mod ,
 &dm_layout_g1_mod ,
 &dm_layout_g1_zone_mod ,
 &dm_layout_g2_mod ,
 &dm_layout_g2_zone_mod ,
 &dm_layout_g4_mod ,
 &dm_mech_g1_mod 
};

typedef enum {
  DM_MOD_DISK,
  DM_MOD_LAYOUT_G1,
  DM_MOD_LAYOUT_G1_ZONE,
  DM_MOD_LAYOUT_G2,
  DM_MOD_LAYOUT_G2_ZONE,
  DM_MOD_LAYOUT_G4,
  DM_MOD_MECH_G1
} dm_mod_t;

#define DM_MAX_MODULE 6
#endif // _DM_MODULES_H

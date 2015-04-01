
#ifndef _DM_LAYOUT_G2_ZONE_PARAM_H
#define _DM_LAYOUT_G2_ZONE_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_layout_g2_zone param loader function */
int dm_layout_g2_zone_loadparams(struct lp_block *b);

typedef enum {
   DM_LAYOUT_G2_ZONE_FIRST_CYLINDER_NUMBER,
   DM_LAYOUT_G2_ZONE_LAST_CYLINDER_NUMBER,
   DM_LAYOUT_G2_ZONE_FIRST_LBN,
   DM_LAYOUT_G2_ZONE_LAST_LBN,
   DM_LAYOUT_G2_ZONE_BLOCKS_PER_TRACK,
   DM_LAYOUT_G2_ZONE_ZONE_SKEW,
   DM_LAYOUT_G2_ZONE_SKEW_UNITS,
   DM_LAYOUT_G2_ZONE_SKEW_FOR_TRACK_SWITCH,
   DM_LAYOUT_G2_ZONE_SKEW_FOR_CYLINDER_SWITCH
} dm_layout_g2_zone_param_t;

#define DM_LAYOUT_G2_ZONE_MAX_PARAM		DM_LAYOUT_G2_ZONE_SKEW_FOR_CYLINDER_SWITCH
extern void * DM_LAYOUT_G2_ZONE_loaders[];
extern lp_paramdep_t DM_LAYOUT_G2_ZONE_deps[];


static struct lp_varspec dm_layout_g2_zone_params [] = {
   {"First cylinder number", I, 1 },
   {"Last cylinder number", I, 1 },
   {"First LBN", I, 1 },
   {"Last LBN", I, 1 },
   {"Blocks per track", I, 1 },
   {"Zone Skew", D, 0 },
   {"Skew units", S, 0 },
   {"Skew for track switch", D, 0 },
   {"Skew for cylinder switch", D, 0 },
   {0,0,0}
};
#define DM_LAYOUT_G2_ZONE_MAX 9
static struct lp_mod dm_layout_g2_zone_mod = { "dm_layout_g2_zone", dm_layout_g2_zone_params, DM_LAYOUT_G2_ZONE_MAX, (lp_modloader_t)dm_layout_g2_zone_loadparams,  0, 0, DM_LAYOUT_G2_ZONE_loaders, DM_LAYOUT_G2_ZONE_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_LAYOUT_G2_ZONE_PARAM_H

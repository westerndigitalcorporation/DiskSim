
#ifndef _DM_LAYOUT_G1_ZONE_PARAM_H
#define _DM_LAYOUT_G1_ZONE_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_layout_g1_zone param loader function */
int dm_layout_g1_zone_loadparams(struct lp_block *b);

typedef enum {
   DM_LAYOUT_G1_ZONE_FIRST_CYLINDER_NUMBER,
   DM_LAYOUT_G1_ZONE_LAST_CYLINDER_NUMBER,
   DM_LAYOUT_G1_ZONE_BLOCKS_PER_TRACK,
   DM_LAYOUT_G1_ZONE_OFFSET_OF_FIRST_BLOCK,
   DM_LAYOUT_G1_ZONE_SKEW_UNITS,
   DM_LAYOUT_G1_ZONE_EMPTY_SPACE_AT_ZONE_FRONT,
   DM_LAYOUT_G1_ZONE_SKEW_FOR_TRACK_SWITCH,
   DM_LAYOUT_G1_ZONE_SKEW_FOR_CYLINDER_SWITCH,
   DM_LAYOUT_G1_ZONE_NUMBER_OF_SPARES,
   DM_LAYOUT_G1_ZONE_SLIPS,
   DM_LAYOUT_G1_ZONE_DEFECTS
} dm_layout_g1_zone_param_t;

#define DM_LAYOUT_G1_ZONE_MAX_PARAM		DM_LAYOUT_G1_ZONE_DEFECTS
extern void * DM_LAYOUT_G1_ZONE_loaders[];
extern lp_paramdep_t DM_LAYOUT_G1_ZONE_deps[];


static struct lp_varspec dm_layout_g1_zone_params [] = {
   {"First cylinder number", I, 1 },
   {"Last cylinder number", I, 1 },
   {"Blocks per track", I, 1 },
   {"Offset of first block", D, 1 },
   {"Skew units", S, 0 },
   {"Empty space at zone front", I, 1 },
   {"Skew for track switch", D, 0 },
   {"Skew for cylinder switch", D, 0 },
   {"Number of spares", I, 1 },
   {"slips", LIST, 1 },
   {"defects", LIST, 1 },
   {0,0,0}
};
#define DM_LAYOUT_G1_ZONE_MAX 11
static struct lp_mod dm_layout_g1_zone_mod = { "dm_layout_g1_zone", dm_layout_g1_zone_params, DM_LAYOUT_G1_ZONE_MAX, (lp_modloader_t)dm_layout_g1_zone_loadparams,  0, 0, DM_LAYOUT_G1_ZONE_loaders, DM_LAYOUT_G1_ZONE_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_LAYOUT_G1_ZONE_PARAM_H

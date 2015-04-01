
#ifndef _DM_LAYOUT_G1_PARAM_H
#define _DM_LAYOUT_G1_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_layout_g1 param loader function */
struct dm_layout_if *dm_layout_g1_loadparams(struct lp_block *b, struct dm_disk_if *d);

typedef enum {
   DM_LAYOUT_G1_LBN_TO_PBN_MAPPING_SCHEME,
   DM_LAYOUT_G1_SPARING_SCHEME_USED,
   DM_LAYOUT_G1_RANGESIZE_FOR_SPARING,
   DM_LAYOUT_G1_SKEW_UNITS,
   DM_LAYOUT_G1_ZONES
} dm_layout_g1_param_t;

#define DM_LAYOUT_G1_MAX_PARAM		DM_LAYOUT_G1_ZONES
extern void * DM_LAYOUT_G1_loaders[];
extern lp_paramdep_t DM_LAYOUT_G1_deps[];


static struct lp_varspec dm_layout_g1_params [] = {
   {"LBN-to-PBN mapping scheme", I, 1 },
   {"Sparing scheme used", I, 1 },
   {"Rangesize for sparing", I, 1 },
   {"Skew units", S, 0 },
   {"Zones", LIST, 1 },
   {0,0,0}
};
#define DM_LAYOUT_G1_MAX 5
static struct lp_mod dm_layout_g1_mod = { "dm_layout_g1", dm_layout_g1_params, DM_LAYOUT_G1_MAX, (lp_modloader_t)dm_layout_g1_loadparams,  0, 0, DM_LAYOUT_G1_loaders, DM_LAYOUT_G1_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_LAYOUT_G1_PARAM_H

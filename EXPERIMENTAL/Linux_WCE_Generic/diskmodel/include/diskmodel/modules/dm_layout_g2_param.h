
#ifndef _DM_LAYOUT_G2_PARAM_H
#define _DM_LAYOUT_G2_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_layout_g2 param loader function */
struct dm_layout_if *dm_layout_g2_loadparams(struct lp_block *, struct dm_disk_if *parent);

typedef enum {
   DM_LAYOUT_G2_LAYOUT_MAP_FILE,
   DM_LAYOUT_G2_ZONES
} dm_layout_g2_param_t;

#define DM_LAYOUT_G2_MAX_PARAM		DM_LAYOUT_G2_ZONES
extern void * DM_LAYOUT_G2_loaders[];
extern lp_paramdep_t DM_LAYOUT_G2_deps[];


static struct lp_varspec dm_layout_g2_params [] = {
   {"Layout Map File", S, 1 },
   {"Zones", LIST, 1 },
   {0,0,0}
};
#define DM_LAYOUT_G2_MAX 2
static struct lp_mod dm_layout_g2_mod = { "dm_layout_g2", dm_layout_g2_params, DM_LAYOUT_G2_MAX, (lp_modloader_t)dm_layout_g2_loadparams,  0, 0, DM_LAYOUT_G2_loaders, DM_LAYOUT_G2_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_LAYOUT_G2_PARAM_H

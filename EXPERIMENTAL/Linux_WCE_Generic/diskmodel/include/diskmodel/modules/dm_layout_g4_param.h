
#ifndef _DM_LAYOUT_G4_PARAM_H
#define _DM_LAYOUT_G4_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_layout_g4 param loader function */
struct dm_layout_if *dm_layout_g4_loadparams(struct lp_block *, struct dm_disk_if *);

typedef enum {
   DM_LAYOUT_G4_TP,
   DM_LAYOUT_G4_IDX,
   DM_LAYOUT_G4_SLIPS,
   DM_LAYOUT_G4_REMAPS
} dm_layout_g4_param_t;

#define DM_LAYOUT_G4_MAX_PARAM		DM_LAYOUT_G4_REMAPS
extern void * DM_LAYOUT_G4_loaders[];
extern lp_paramdep_t DM_LAYOUT_G4_deps[];


static struct lp_varspec dm_layout_g4_params [] = {
   {"TP", LIST, 1 },
   {"IDX", LIST, 1 },
   {"Slips", LIST, 1 },
   {"Remaps", LIST, 1 },
   {0,0,0}
};
#define DM_LAYOUT_G4_MAX 4
static struct lp_mod dm_layout_g4_mod = { "dm_layout_g4", dm_layout_g4_params, DM_LAYOUT_G4_MAX, (lp_modloader_t)dm_layout_g4_loadparams,  0, 0, DM_LAYOUT_G4_loaders, DM_LAYOUT_G4_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_LAYOUT_G4_PARAM_H

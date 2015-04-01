
#ifndef _DISKSIM_SYNCSET_PARAM_H
#define _DISKSIM_SYNCSET_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_syncset param loader function */
int disksim_syncset_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_SYNCSET_TYPE,
   DISKSIM_SYNCSET_DEVICES
} disksim_syncset_param_t;

#define DISKSIM_SYNCSET_MAX_PARAM		DISKSIM_SYNCSET_DEVICES
extern void * DISKSIM_SYNCSET_loaders[];
extern lp_paramdep_t DISKSIM_SYNCSET_deps[];


static struct lp_varspec disksim_syncset_params [] = {
   {"type", S, 1 },
   {"devices", LIST, 1 },
   {0,0,0}
};
#define DISKSIM_SYNCSET_MAX 2
static struct lp_mod disksim_syncset_mod = { "disksim_syncset", disksim_syncset_params, DISKSIM_SYNCSET_MAX, (lp_modloader_t)disksim_syncset_loadparams,  0, 0, DISKSIM_SYNCSET_loaders, DISKSIM_SYNCSET_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_SYNCSET_PARAM_H

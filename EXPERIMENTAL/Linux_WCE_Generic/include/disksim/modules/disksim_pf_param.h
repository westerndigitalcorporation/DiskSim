
#ifndef _DISKSIM_PF_PARAM_H
#define _DISKSIM_PF_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_pf param loader function */
int disksim_pf_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_PF_NUMBER_OF_PROCESSORS,
   DISKSIM_PF_PROCESS_FLOW_TIME_SCALE
} disksim_pf_param_t;

#define DISKSIM_PF_MAX_PARAM		DISKSIM_PF_PROCESS_FLOW_TIME_SCALE
extern void * DISKSIM_PF_loaders[];
extern lp_paramdep_t DISKSIM_PF_deps[];


static struct lp_varspec disksim_pf_params [] = {
   {"Number of processors", I, 1 },
   {"Process-Flow Time Scale", D, 1 },
   {0,0,0}
};
#define DISKSIM_PF_MAX 2
static struct lp_mod disksim_pf_mod = { "disksim_pf", disksim_pf_params, DISKSIM_PF_MAX, (lp_modloader_t)disksim_pf_loadparams,  0, 0, DISKSIM_PF_loaders, DISKSIM_PF_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_PF_PARAM_H


#ifndef _DISKSIM_IOMAP_PARAM_H
#define _DISKSIM_IOMAP_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_iomap param loader function */
int disksim_iomap_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_IOMAP_TRACEDEV,
   DISKSIM_IOMAP_SIMDEV,
   DISKSIM_IOMAP_LOCSCALE,
   DISKSIM_IOMAP_SIZESCALE,
   DISKSIM_IOMAP_OFFSET
} disksim_iomap_param_t;

#define DISKSIM_IOMAP_MAX_PARAM		DISKSIM_IOMAP_OFFSET
extern void * DISKSIM_IOMAP_loaders[];
extern lp_paramdep_t DISKSIM_IOMAP_deps[];


static struct lp_varspec disksim_iomap_params [] = {
   {"tracedev", I, 1 },
   {"simdev", S, 1 },
   {"locScale", I, 1 },
   {"sizeScale", I, 1 },
   {"offset", I, 0 },
   {0,0,0}
};
#define DISKSIM_IOMAP_MAX 5
static struct lp_mod disksim_iomap_mod = { "disksim_iomap", disksim_iomap_params, DISKSIM_IOMAP_MAX, (lp_modloader_t)disksim_iomap_loadparams,  0, 0, DISKSIM_IOMAP_loaders, DISKSIM_IOMAP_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_IOMAP_PARAM_H

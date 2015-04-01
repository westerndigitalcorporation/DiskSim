
#ifndef _DISKSIM_IOSIM_PARAM_H
#define _DISKSIM_IOSIM_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_iosim param loader function */
int disksim_iosim_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_IOSIM_IO_TRACE_TIME_SCALE,
   DISKSIM_IOSIM_IO_MAPPINGS
} disksim_iosim_param_t;

#define DISKSIM_IOSIM_MAX_PARAM		DISKSIM_IOSIM_IO_MAPPINGS
extern void * DISKSIM_IOSIM_loaders[];
extern lp_paramdep_t DISKSIM_IOSIM_deps[];


static struct lp_varspec disksim_iosim_params [] = {
   {"I/O Trace Time Scale", D, 0 },
   {"I/O Mappings", LIST, 0 },
   {0,0,0}
};
#define DISKSIM_IOSIM_MAX 2
static struct lp_mod disksim_iosim_mod = { "disksim_iosim", disksim_iosim_params, DISKSIM_IOSIM_MAX, (lp_modloader_t)disksim_iosim_loadparams,  0, 0, DISKSIM_IOSIM_loaders, DISKSIM_IOSIM_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_IOSIM_PARAM_H


#ifndef _DISKSIM_IODRIVER_PARAM_H
#define _DISKSIM_IODRIVER_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_iodriver param loader function */
struct iodriver *disksim_iodriver_loadparams(struct lp_block *b);


/* prototype for disksim_iodriver param loader function */
struct ioq *disksim_ioqueue_loadparams(struct lp_block *b, int printqueuestats, int printcritstats, int printidlestats, int printintarrstats, int printsizestats);

typedef enum {
   DISKSIM_IODRIVER_TYPE,
   DISKSIM_IODRIVER_CONSTANT_ACCESS_TIME,
   DISKSIM_IODRIVER_USE_QUEUEING_IN_SUBSYSTEM,
   DISKSIM_IODRIVER_SCHEDULER
} disksim_iodriver_param_t;

#define DISKSIM_IODRIVER_MAX_PARAM		DISKSIM_IODRIVER_SCHEDULER
extern void * DISKSIM_IODRIVER_loaders[];
extern lp_paramdep_t DISKSIM_IODRIVER_deps[];


static struct lp_varspec disksim_iodriver_params [] = {
   {"type", I, 1 },
   {"Constant access time", D, 1 },
   {"Use queueing in subsystem", I, 1 },
   {"Scheduler", BLOCK, 1 },
   {0,0,0}
};
#define DISKSIM_IODRIVER_MAX 4
static struct lp_mod disksim_iodriver_mod = { "disksim_iodriver", disksim_iodriver_params, DISKSIM_IODRIVER_MAX, (lp_modloader_t)disksim_iodriver_loadparams,  0, 0, DISKSIM_IODRIVER_loaders, DISKSIM_IODRIVER_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_IODRIVER_PARAM_H


#ifndef _DISKSIM_CTLR_STATS_PARAM_H
#define _DISKSIM_CTLR_STATS_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_ctlr_stats param loader function */
int disksim_ctlr_stats_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_CACHE_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_SIZE_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_LOCALITY_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_BLOCKING_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_INTERFERENCE_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_QUEUE_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_CRIT_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_IDLE_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_INTARR_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_STREAK_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_STAMP_STATS,
   DISKSIM_CTLR_STATS_PRINT_CONTROLLER_PER_DEVICE_STATS
} disksim_ctlr_stats_param_t;

#define DISKSIM_CTLR_STATS_MAX_PARAM		DISKSIM_CTLR_STATS_PRINT_CONTROLLER_PER_DEVICE_STATS
extern void * DISKSIM_CTLR_STATS_loaders[];
extern lp_paramdep_t DISKSIM_CTLR_STATS_deps[];


static struct lp_varspec disksim_ctlr_stats_params [] = {
   {"Print controller cache stats", I, 1 },
   {"Print controller size stats", I, 1 },
   {"Print controller locality stats", I, 1 },
   {"Print controller blocking stats", I, 1 },
   {"Print controller interference stats", I, 1 },
   {"Print controller queue stats", I, 1 },
   {"Print controller crit stats", I, 1 },
   {"Print controller idle stats", I, 1 },
   {"Print controller intarr stats", I, 1 },
   {"Print controller streak stats", I, 1 },
   {"Print controller stamp stats", I, 1 },
   {"Print controller per-device stats", I, 1 },
   {0,0,0}
};
#define DISKSIM_CTLR_STATS_MAX 12
static struct lp_mod disksim_ctlr_stats_mod = { "disksim_ctlr_stats", disksim_ctlr_stats_params, DISKSIM_CTLR_STATS_MAX, (lp_modloader_t)disksim_ctlr_stats_loadparams,  0, 0, DISKSIM_CTLR_STATS_loaders, DISKSIM_CTLR_STATS_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_CTLR_STATS_PARAM_H

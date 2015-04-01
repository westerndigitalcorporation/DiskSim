
#ifndef _DISKSIM_IODRIVER_STATS_PARAM_H
#define _DISKSIM_IODRIVER_STATS_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_iodriver_stats param loader function */
int disksim_iodriver_stats_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_SIZE_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_LOCALITY_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_BLOCKING_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_INTERFERENCE_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_QUEUE_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_CRIT_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_IDLE_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_INTARR_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_STREAK_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_STAMP_STATS,
   DISKSIM_IODRIVER_STATS_PRINT_DRIVER_PER_DEVICE_STATS
} disksim_iodriver_stats_param_t;

#define DISKSIM_IODRIVER_STATS_MAX_PARAM		DISKSIM_IODRIVER_STATS_PRINT_DRIVER_PER_DEVICE_STATS
extern void * DISKSIM_IODRIVER_STATS_loaders[];
extern lp_paramdep_t DISKSIM_IODRIVER_STATS_deps[];


static struct lp_varspec disksim_iodriver_stats_params [] = {
   {"Print driver size stats", I, 1 },
   {"Print driver locality stats", I, 1 },
   {"Print driver blocking stats", I, 1 },
   {"Print driver interference stats", I, 1 },
   {"Print driver queue stats", I, 1 },
   {"Print driver crit stats", I, 1 },
   {"Print driver idle stats", I, 1 },
   {"Print driver intarr stats", I, 1 },
   {"Print driver streak stats", I, 1 },
   {"Print driver stamp stats", I, 1 },
   {"Print driver per-device stats", I, 1 },
   {0,0,0}
};
#define DISKSIM_IODRIVER_STATS_MAX 11
static struct lp_mod disksim_iodriver_stats_mod = { "disksim_iodriver_stats", disksim_iodriver_stats_params, DISKSIM_IODRIVER_STATS_MAX, (lp_modloader_t)disksim_iodriver_stats_loadparams,  0, 0, DISKSIM_IODRIVER_STATS_loaders, DISKSIM_IODRIVER_STATS_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_IODRIVER_STATS_PARAM_H

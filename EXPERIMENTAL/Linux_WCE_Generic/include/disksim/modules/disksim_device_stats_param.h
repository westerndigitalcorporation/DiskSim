
#ifndef _DISKSIM_DEVICE_STATS_PARAM_H
#define _DISKSIM_DEVICE_STATS_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_device_stats param loader function */
int disksim_device_stats_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_QUEUE_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_CRIT_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_IDLE_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_INTARR_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_SIZE_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_SEEK_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_LATENCY_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_XFER_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_ACCTIME_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_INTERFERE_STATS,
   DISKSIM_DEVICE_STATS_PRINT_DEVICE_BUFFER_STATS
} disksim_device_stats_param_t;

#define DISKSIM_DEVICE_STATS_MAX_PARAM		DISKSIM_DEVICE_STATS_PRINT_DEVICE_BUFFER_STATS
extern void * DISKSIM_DEVICE_STATS_loaders[];
extern lp_paramdep_t DISKSIM_DEVICE_STATS_deps[];


static struct lp_varspec disksim_device_stats_params [] = {
   {"Print device queue stats", I, 1 },
   {"Print device crit stats", I, 1 },
   {"Print device idle stats", I, 1 },
   {"Print device intarr stats", I, 1 },
   {"Print device size stats", I, 1 },
   {"Print device seek stats", I, 1 },
   {"Print device latency stats", I, 1 },
   {"Print device xfer stats", I, 1 },
   {"Print device acctime stats", I, 1 },
   {"Print device interfere stats", I, 1 },
   {"Print device buffer stats", I, 1 },
   {0,0,0}
};
#define DISKSIM_DEVICE_STATS_MAX 11
static struct lp_mod disksim_device_stats_mod = { "disksim_device_stats", disksim_device_stats_params, DISKSIM_DEVICE_STATS_MAX, (lp_modloader_t)disksim_device_stats_loadparams,  0, 0, DISKSIM_DEVICE_STATS_loaders, DISKSIM_DEVICE_STATS_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_DEVICE_STATS_PARAM_H

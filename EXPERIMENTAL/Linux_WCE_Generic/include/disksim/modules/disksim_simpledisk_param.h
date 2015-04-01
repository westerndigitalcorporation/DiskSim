
#ifndef _DISKSIM_SIMPLEDISK_PARAM_H
#define _DISKSIM_SIMPLEDISK_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_simpledisk param loader function */
struct simpledisk *disksim_simpledisk_loadparams(struct lp_block *b);


/* prototype for disksim_simpledisk param loader function */
struct ioq *disksim_ioqueue_loadparams(struct lp_block *b, int printqueuestats, int printcritstats, int printidlestats, int printintarrstats, int printsizestats);

typedef enum {
   DISKSIM_SIMPLEDISK_SCHEDULER,
   DISKSIM_SIMPLEDISK_MAX_QUEUE_LENGTH,
   DISKSIM_SIMPLEDISK_BLOCK_COUNT,
   DISKSIM_SIMPLEDISK_BUS_TRANSACTION_LATENCY,
   DISKSIM_SIMPLEDISK_BULK_SECTOR_TRANSFER_TIME,
   DISKSIM_SIMPLEDISK_NEVER_DISCONNECT,
   DISKSIM_SIMPLEDISK_PRINT_STATS,
   DISKSIM_SIMPLEDISK_COMMAND_OVERHEAD,
   DISKSIM_SIMPLEDISK_CONSTANT_ACCESS_TIME,
   DISKSIM_SIMPLEDISK_ACCESS_TIME
} disksim_simpledisk_param_t;

#define DISKSIM_SIMPLEDISK_MAX_PARAM		DISKSIM_SIMPLEDISK_ACCESS_TIME
extern void * DISKSIM_SIMPLEDISK_loaders[];
extern lp_paramdep_t DISKSIM_SIMPLEDISK_deps[];


static struct lp_varspec disksim_simpledisk_params [] = {
   {"Scheduler", BLOCK, 1 },
   {"Max queue length", I, 1 },
   {"Block count", I, 1 },
   {"Bus transaction latency", D, 0 },
   {"Bulk sector transfer time", D, 1 },
   {"Never disconnect", I, 1 },
   {"Print stats", I, 1 },
   {"Command overhead", D, 1 },
   {"Constant access time", D, 0 },
   {"Access time", D, 1 },
   {0,0,0}
};
#define DISKSIM_SIMPLEDISK_MAX 10
static struct lp_mod disksim_simpledisk_mod = { "disksim_simpledisk", disksim_simpledisk_params, DISKSIM_SIMPLEDISK_MAX, (lp_modloader_t)disksim_simpledisk_loadparams,  0, 0, DISKSIM_SIMPLEDISK_loaders, DISKSIM_SIMPLEDISK_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_SIMPLEDISK_PARAM_H

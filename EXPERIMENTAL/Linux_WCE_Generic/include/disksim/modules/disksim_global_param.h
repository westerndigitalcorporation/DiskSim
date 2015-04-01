
#ifndef _DISKSIM_GLOBAL_PARAM_H
#define _DISKSIM_GLOBAL_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_global param loader function */
int disksim_global_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_GLOBAL_INIT_SEED,
   DISKSIM_GLOBAL_INIT_SEED_WITH_TIME,
   DISKSIM_GLOBAL_REAL_SEED,
   DISKSIM_GLOBAL_REAL_SEED_WITH_TIME,
   DISKSIM_GLOBAL_STATISTIC_WARM_UP_TIME,
   DISKSIM_GLOBAL_STATISTIC_WARM_UP_IOS,
   DISKSIM_GLOBAL_STAT_DEFINITION_FILE,
   DISKSIM_GLOBAL_OUTPUT_FILE_FOR_TRACE_OF_IO_REQUESTS_SIMULATED,
   DISKSIM_GLOBAL_DETAILED_EXECUTION_TRACE
} disksim_global_param_t;

#define DISKSIM_GLOBAL_MAX_PARAM		DISKSIM_GLOBAL_DETAILED_EXECUTION_TRACE
extern void * DISKSIM_GLOBAL_loaders[];
extern lp_paramdep_t DISKSIM_GLOBAL_deps[];


static struct lp_varspec disksim_global_params [] = {
   {"Init Seed", I, 0 },
   {"Init Seed with time", I, 0 },
   {"Real Seed", I, 0 },
   {"Real Seed with time", I, 0 },
   {"Statistic warm-up time", D, 0 },
   {"Statistic warm-up IOs", I, 0 },
   {"Stat definition file", S, 1 },
   {"Output file for trace of I/O requests simulated", S, 0 },
   {"Detailed execution trace", S, 0 },
   {0,0,0}
};
#define DISKSIM_GLOBAL_MAX 9
static struct lp_mod disksim_global_mod = { "disksim_global", disksim_global_params, DISKSIM_GLOBAL_MAX, (lp_modloader_t)disksim_global_loadparams,  0, 0, DISKSIM_GLOBAL_loaders, DISKSIM_GLOBAL_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_GLOBAL_PARAM_H


#ifndef _DISKSIM_PF_STATS_PARAM_H
#define _DISKSIM_PF_STATS_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_pf_stats param loader function */
int disksim_pf_stats_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_PF_STATS_PRINT_PER_PROCESS_STATS,
   DISKSIM_PF_STATS_PRINT_PER_CPU_STATS,
   DISKSIM_PF_STATS_PRINT_ALL_INTERRUPT_STATS,
   DISKSIM_PF_STATS_PRINT_SLEEP_STATS
} disksim_pf_stats_param_t;

#define DISKSIM_PF_STATS_MAX_PARAM		DISKSIM_PF_STATS_PRINT_SLEEP_STATS
extern void * DISKSIM_PF_STATS_loaders[];
extern lp_paramdep_t DISKSIM_PF_STATS_deps[];


static struct lp_varspec disksim_pf_stats_params [] = {
   {"Print per-process stats", I, 1 },
   {"Print per-CPU stats", I, 1 },
   {"Print all interrupt stats", I, 1 },
   {"Print sleep stats", I, 1 },
   {0,0,0}
};
#define DISKSIM_PF_STATS_MAX 4
static struct lp_mod disksim_pf_stats_mod = { "disksim_pf_stats", disksim_pf_stats_params, DISKSIM_PF_STATS_MAX, (lp_modloader_t)disksim_pf_stats_loadparams,  0, 0, DISKSIM_PF_STATS_loaders, DISKSIM_PF_STATS_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_PF_STATS_PARAM_H

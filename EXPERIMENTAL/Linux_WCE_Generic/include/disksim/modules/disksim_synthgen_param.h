
#ifndef _DISKSIM_SYNTHGEN_PARAM_H
#define _DISKSIM_SYNTHGEN_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_synthgen param loader function */
int disksim_synthgen_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_SYNTHGEN_STORAGE_CAPACITY_PER_DEVICE,
   DISKSIM_SYNTHGEN_DEVICES,
   DISKSIM_SYNTHGEN_BLOCKING_FACTOR,
   DISKSIM_SYNTHGEN_PROBABILITY_OF_SEQUENTIAL_ACCESS,
   DISKSIM_SYNTHGEN_PROBABILITY_OF_LOCAL_ACCESS,
   DISKSIM_SYNTHGEN_PROBABILITY_OF_READ_ACCESS,
   DISKSIM_SYNTHGEN_PROBABILITY_OF_TIME_CRITICAL_REQUEST,
   DISKSIM_SYNTHGEN_PROBABILITY_OF_TIME_LIMITED_REQUEST,
   DISKSIM_SYNTHGEN_TIME_LIMITED_THINK_TIMES,
   DISKSIM_SYNTHGEN_GENERAL_INTER_ARRIVAL_TIMES,
   DISKSIM_SYNTHGEN_SEQUENTIAL_INTER_ARRIVAL_TIMES,
   DISKSIM_SYNTHGEN_LOCAL_INTER_ARRIVAL_TIMES,
   DISKSIM_SYNTHGEN_LOCAL_DISTANCES,
   DISKSIM_SYNTHGEN_SIZES
} disksim_synthgen_param_t;

#define DISKSIM_SYNTHGEN_MAX_PARAM		DISKSIM_SYNTHGEN_SIZES
extern void * DISKSIM_SYNTHGEN_loaders[];
extern lp_paramdep_t DISKSIM_SYNTHGEN_deps[];


static struct lp_varspec disksim_synthgen_params [] = {
   {"Storage capacity per device", I, 1 },
   {"devices", LIST, 1 },
   {"Blocking factor", I, 1 },
   {"Probability of sequential access", D, 1 },
   {"Probability of local access", D, 1 },
   {"Probability of read access", D, 1 },
   {"Probability of time-critical request", D, 1 },
   {"Probability of time-limited request", D, 1 },
   {"Time-limited think times", LIST, 1 },
   {"General inter-arrival times", LIST, 1 },
   {"Sequential inter-arrival times", LIST, 1 },
   {"Local inter-arrival times", LIST, 1 },
   {"Local distances", LIST, 1 },
   {"Sizes", LIST, 1 },
   {0,0,0}
};
#define DISKSIM_SYNTHGEN_MAX 14
static struct lp_mod disksim_synthgen_mod = { "disksim_synthgen", disksim_synthgen_params, DISKSIM_SYNTHGEN_MAX, (lp_modloader_t)disksim_synthgen_loadparams,  0, 0, DISKSIM_SYNTHGEN_loaders, DISKSIM_SYNTHGEN_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_SYNTHGEN_PARAM_H

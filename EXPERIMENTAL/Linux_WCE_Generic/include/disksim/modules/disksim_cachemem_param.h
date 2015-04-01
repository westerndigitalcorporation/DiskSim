
#ifndef _DISKSIM_CACHEMEM_PARAM_H
#define _DISKSIM_CACHEMEM_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_cachemem param loader function */
struct cache_if *disksim_cachemem_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_CACHEMEM_CACHE_SIZE,
   DISKSIM_CACHEMEM_SLRU_SEGMENTS,
   DISKSIM_CACHEMEM_LINE_SIZE,
   DISKSIM_CACHEMEM_BIT_GRANULARITY,
   DISKSIM_CACHEMEM_LOCK_GRANULARITY,
   DISKSIM_CACHEMEM_SHARED_READ_LOCKS,
   DISKSIM_CACHEMEM_MAX_REQUEST_SIZE,
   DISKSIM_CACHEMEM_REPLACEMENT_POLICY,
   DISKSIM_CACHEMEM_ALLOCATION_POLICY,
   DISKSIM_CACHEMEM_WRITE_SCHEME,
   DISKSIM_CACHEMEM_FLUSH_POLICY,
   DISKSIM_CACHEMEM_FLUSH_PERIOD,
   DISKSIM_CACHEMEM_FLUSH_IDLE_DELAY,
   DISKSIM_CACHEMEM_FLUSH_MAX_LINE_CLUSTER,
   DISKSIM_CACHEMEM_READ_PREFETCH_TYPE,
   DISKSIM_CACHEMEM_WRITE_PREFETCH_TYPE,
   DISKSIM_CACHEMEM_LINE_BY_LINE_FETCHES,
   DISKSIM_CACHEMEM_MAX_GATHER
} disksim_cachemem_param_t;

#define DISKSIM_CACHEMEM_MAX_PARAM		DISKSIM_CACHEMEM_MAX_GATHER
extern void * DISKSIM_CACHEMEM_loaders[];
extern lp_paramdep_t DISKSIM_CACHEMEM_deps[];


static struct lp_varspec disksim_cachemem_params [] = {
   {"Cache size", I, 1 },
   {"SLRU segments", LIST, 0 },
   {"Line size", I, 1 },
   {"Bit granularity", I, 1 },
   {"Lock granularity", I, 1 },
   {"Shared read locks", I, 1 },
   {"Max request size", I, 1 },
   {"Replacement policy", I, 1 },
   {"Allocation policy", I, 1 },
   {"Write scheme", I, 1 },
   {"Flush policy", I, 1 },
   {"Flush period", D, 1 },
   {"Flush idle delay", D, 1 },
   {"Flush max line cluster", I, 1 },
   {"Read prefetch type", I, 1 },
   {"Write prefetch type", I, 1 },
   {"Line-by-line fetches", I, 1 },
   {"Max gather", I, 1 },
   {0,0,0}
};
#define DISKSIM_CACHEMEM_MAX 18
static struct lp_mod disksim_cachemem_mod = { "disksim_cachemem", disksim_cachemem_params, DISKSIM_CACHEMEM_MAX, (lp_modloader_t)disksim_cachemem_loadparams,  0, 0, DISKSIM_CACHEMEM_loaders, DISKSIM_CACHEMEM_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_CACHEMEM_PARAM_H

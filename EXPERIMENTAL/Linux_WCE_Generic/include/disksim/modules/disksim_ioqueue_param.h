
#ifndef _DISKSIM_IOQUEUE_PARAM_H
#define _DISKSIM_IOQUEUE_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_ioqueue param loader function */
struct ioq *disksim_ioqueue_loadparams(struct lp_block *b, int printqueuestats, int printcritstats, int printidlestats, int printintarrstats, int printsizestats);

typedef enum {
   DISKSIM_IOQUEUE_SCHEDULING_POLICY,
   DISKSIM_IOQUEUE_CYLINDER_MAPPING_STRATEGY,
   DISKSIM_IOQUEUE_WRITE_INITIATION_DELAY,
   DISKSIM_IOQUEUE_READ_INITIATION_DELAY,
   DISKSIM_IOQUEUE_SEQUENTIAL_STREAM_SCHEME,
   DISKSIM_IOQUEUE_MAXIMUM_CONCAT_SIZE,
   DISKSIM_IOQUEUE_OVERLAPPING_REQUEST_SCHEME,
   DISKSIM_IOQUEUE_SEQUENTIAL_STREAM_DIFF_MAXIMUM,
   DISKSIM_IOQUEUE_SCHEDULING_TIMEOUT_SCHEME,
   DISKSIM_IOQUEUE_TIMEOUT_TIMEWEIGHT,
   DISKSIM_IOQUEUE_TIMEOUT_SCHEDULING,
   DISKSIM_IOQUEUE_SCHEDULING_PRIORITY_SCHEME,
   DISKSIM_IOQUEUE_PRIORITY_SCHEDULING
} disksim_ioqueue_param_t;

#define DISKSIM_IOQUEUE_MAX_PARAM		DISKSIM_IOQUEUE_PRIORITY_SCHEDULING
extern void * DISKSIM_IOQUEUE_loaders[];
extern lp_paramdep_t DISKSIM_IOQUEUE_deps[];


static struct lp_varspec disksim_ioqueue_params [] = {
   {"Scheduling policy", I, 1 },
   {"Cylinder mapping strategy", I, 1 },
   {"Write initiation delay", D, 1 },
   {"Read initiation delay", D, 1 },
   {"Sequential stream scheme", I, 1 },
   {"Maximum concat size", I, 1 },
   {"Overlapping request scheme", I, 1 },
   {"Sequential stream diff maximum", I, 1 },
   {"Scheduling timeout scheme", I, 1 },
   {"Timeout time/weight", I, 1 },
   {"Timeout scheduling", I, 1 },
   {"Scheduling priority scheme", I, 1 },
   {"Priority scheduling", I, 1 },
   {0,0,0}
};
#define DISKSIM_IOQUEUE_MAX 13
static struct lp_mod disksim_ioqueue_mod = { "disksim_ioqueue", disksim_ioqueue_params, DISKSIM_IOQUEUE_MAX, (lp_modloader_t)disksim_ioqueue_loadparams,  0, 0, DISKSIM_IOQUEUE_loaders, DISKSIM_IOQUEUE_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_IOQUEUE_PARAM_H

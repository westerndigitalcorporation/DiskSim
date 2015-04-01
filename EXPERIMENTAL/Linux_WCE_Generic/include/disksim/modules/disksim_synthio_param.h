
#ifndef _DISKSIM_SYNTHIO_PARAM_H
#define _DISKSIM_SYNTHIO_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_synthio param loader function */
int disksim_synthio_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_SYNTHIO_NUMBER_OF_IO_REQUESTS_TO_GENERATE,
   DISKSIM_SYNTHIO_MAXIMUM_TIME_OF_TRACE_GENERATED,
   DISKSIM_SYNTHIO_SYSTEM_CALLRETURN_WITH_EACH_REQUEST,
   DISKSIM_SYNTHIO_THINK_TIME_FROM_CALL_TO_REQUEST,
   DISKSIM_SYNTHIO_THINK_TIME_FROM_REQUEST_TO_RETURN,
   DISKSIM_SYNTHIO_GENERATORS
} disksim_synthio_param_t;

#define DISKSIM_SYNTHIO_MAX_PARAM		DISKSIM_SYNTHIO_GENERATORS
extern void * DISKSIM_SYNTHIO_loaders[];
extern lp_paramdep_t DISKSIM_SYNTHIO_deps[];


static struct lp_varspec disksim_synthio_params [] = {
   {"Number of I/O requests to generate", I, 1 },
   {"Maximum time of trace generated", D, 1 },
   {"System call/return with each request", I, 1 },
   {"Think time from call to request", D, 1 },
   {"Think time from request to return", D, 1 },
   {"Generators", LIST, 1 },
   {0,0,0}
};
#define DISKSIM_SYNTHIO_MAX 6
static struct lp_mod disksim_synthio_mod = { "disksim_synthio", disksim_synthio_params, DISKSIM_SYNTHIO_MAX, (lp_modloader_t)disksim_synthio_loadparams,  0, 0, DISKSIM_SYNTHIO_loaders, DISKSIM_SYNTHIO_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_SYNTHIO_PARAM_H

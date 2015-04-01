
#ifndef _DISKSIM_DEVICE_PARAM_H
#define _DISKSIM_DEVICE_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_device param loader function */
void disksim_device_loadparams(void);

#define DISKSIM_DEVICE_MAX_PARAM		
extern void * DISKSIM_DEVICE_loaders[];
extern lp_paramdep_t DISKSIM_DEVICE_deps[];


static struct lp_varspec disksim_device_params [] = {
   {0,0,0}
};
#define DISKSIM_DEVICE_MAX 0
static struct lp_mod disksim_device_mod = { "disksim_device", disksim_device_params, DISKSIM_DEVICE_MAX, (lp_modloader_t)disksim_device_loadparams,  0, 0, DISKSIM_DEVICE_loaders, DISKSIM_DEVICE_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_DEVICE_PARAM_H

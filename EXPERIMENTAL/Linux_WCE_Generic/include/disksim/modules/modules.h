
#ifndef _DISKSIM_MODULES_H
#define _DISKSIM_MODULES_H   


#include "disksim_bus_param.h"
#include "disksim_bus_stats_param.h"
#include "disksim_cachedev_param.h"
#include "disksim_cachemem_param.h"
#include "disksim_ctlr_param.h"
#include "disksim_ctlr_stats_param.h"
#include "disksim_device_param.h"
#include "disksim_device_stats_param.h"
#include "disksim_disk_param.h"
#include "disksim_global_param.h"
#include "disksim_iodriver_param.h"
#include "disksim_iodriver_stats_param.h"
#include "disksim_iomap_param.h"
#include "disksim_ioqueue_param.h"
#include "disksim_iosim_param.h"
#include "disksim_logorg_param.h"
#include "disksim_pf_param.h"
#include "disksim_pf_stats_param.h"
#include "disksim_simpledisk_param.h"
#include "disksim_stats_param.h"
#include "disksim_syncset_param.h"
#include "disksim_synthgen_param.h"
#include "disksim_synthio_param.h"


static struct lp_mod *disksim_mods[] = {
 &disksim_bus_mod ,
 &disksim_bus_stats_mod ,
 &disksim_cachedev_mod ,
 &disksim_cachemem_mod ,
 &disksim_ctlr_mod ,
 &disksim_ctlr_stats_mod ,
 &disksim_device_mod ,
 &disksim_device_stats_mod ,
 &disksim_disk_mod ,
 &disksim_global_mod ,
 &disksim_iodriver_mod ,
 &disksim_iodriver_stats_mod ,
 &disksim_iomap_mod ,
 &disksim_ioqueue_mod ,
 &disksim_iosim_mod ,
 &disksim_logorg_mod ,
 &disksim_pf_mod ,
 &disksim_pf_stats_mod ,
 &disksim_simpledisk_mod ,
 &disksim_stats_mod ,
 &disksim_syncset_mod ,
 &disksim_synthgen_mod ,
 &disksim_synthio_mod 
};

typedef enum {
  DISKSIM_MOD_BUS,
  DISKSIM_MOD_BUS_STATS,
  DISKSIM_MOD_CACHEDEV,
  DISKSIM_MOD_CACHEMEM,
  DISKSIM_MOD_CTLR,
  DISKSIM_MOD_CTLR_STATS,
  DISKSIM_MOD_DEVICE,
  DISKSIM_MOD_DEVICE_STATS,
  DISKSIM_MOD_DISK,
  DISKSIM_MOD_GLOBAL,
  DISKSIM_MOD_IODRIVER,
  DISKSIM_MOD_IODRIVER_STATS,
  DISKSIM_MOD_IOMAP,
  DISKSIM_MOD_IOQUEUE,
  DISKSIM_MOD_IOSIM,
  DISKSIM_MOD_LOGORG,
  DISKSIM_MOD_PF,
  DISKSIM_MOD_PF_STATS,
  DISKSIM_MOD_SIMPLEDISK,
  DISKSIM_MOD_STATS,
  DISKSIM_MOD_SYNCSET,
  DISKSIM_MOD_SYNTHGEN,
  DISKSIM_MOD_SYNTHIO
} disksim_mod_t;

#define DISKSIM_MAX_MODULE 22
#endif // _DISKSIM_MODULES_H

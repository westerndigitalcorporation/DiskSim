
#ifndef _DISKSIM_DISK_PARAM_H
#define _DISKSIM_DISK_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for disksim_disk param loader function */
struct disk* disksim_disk_loadparams(struct lp_block *b, int *num);


/* prototype for disksim_disk param loader function */
struct dm_disk_if* dm_disk_loadparams(struct lp_block *b, int *num);


/* prototype for disksim_disk param loader function */
struct ioq *disksim_ioqueue_loadparams(struct lp_block *b, int printqueuestats, int printcritstats, int printidlestats, int printintarrstats, int printsizestats);

typedef enum {
   DISKSIM_DISK_MODEL,
   DISKSIM_DISK_SCHEDULER,
   DISKSIM_DISK_MAX_QUEUE_LENGTH,
   DISKSIM_DISK_BULK_SECTOR_TRANSFER_TIME,
   DISKSIM_DISK_SEGMENT_SIZE_,
   DISKSIM_DISK_NUMBER_OF_BUFFER_SEGMENTS,
   DISKSIM_DISK_PRINT_STATS,
   DISKSIM_DISK_PER_REQUEST_OVERHEAD_TIME,
   DISKSIM_DISK_TIME_SCALE_FOR_OVERHEADS,
   DISKSIM_DISK_HOLD_BUS_ENTIRE_READ_XFER,
   DISKSIM_DISK_HOLD_BUS_ENTIRE_WRITE_XFER,
   DISKSIM_DISK_ALLOW_ALMOST_READ_HITS,
   DISKSIM_DISK_ALLOW_SNEAKY_FULL_READ_HITS,
   DISKSIM_DISK_ALLOW_SNEAKY_PARTIAL_READ_HITS,
   DISKSIM_DISK_ALLOW_SNEAKY_INTERMEDIATE_READ_HITS,
   DISKSIM_DISK_ALLOW_READ_HITS_ON_WRITE_DATA,
   DISKSIM_DISK_ALLOW_WRITE_PREBUFFERING,
   DISKSIM_DISK_PRESEEKING_LEVEL,
   DISKSIM_DISK_NEVER_DISCONNECT,
   DISKSIM_DISK_AVG_SECTORS_PER_CYLINDER,
   DISKSIM_DISK_MAXIMUM_NUMBER_OF_WRITE_SEGMENTS,
   DISKSIM_DISK_USE_SEPARATE_WRITE_SEGMENT,
   DISKSIM_DISK_LOW__WATER_MARK,
   DISKSIM_DISK_HIGH__WATER_MARK,
   DISKSIM_DISK_SET_WATERMARK_BY_REQSIZE,
   DISKSIM_DISK_CALC_SECTOR_BY_SECTOR,
   DISKSIM_DISK_ENABLE_CACHING_IN_BUFFER,
   DISKSIM_DISK_BUFFER_CONTINUOUS_READ,
   DISKSIM_DISK_MINIMUM_READ_AHEAD_,
   DISKSIM_DISK_MAXIMUM_READ_AHEAD_,
   DISKSIM_DISK_READ_AHEAD_OVER_REQUESTED,
   DISKSIM_DISK_READ_AHEAD_ON_IDLE_HIT,
   DISKSIM_DISK_READ_ANY_FREE_BLOCKS,
   DISKSIM_DISK_FAST_WRITE_LEVEL,
   DISKSIM_DISK_COMBINE_SEQ_WRITES,
   DISKSIM_DISK_STOP_PREFETCH_IN_SECTOR,
   DISKSIM_DISK_DISCONNECT_WRITE_IF_SEEK,
   DISKSIM_DISK_WRITE_HIT_STOP_PREFETCH,
   DISKSIM_DISK_READ_DIRECTLY_TO_BUFFER,
   DISKSIM_DISK_IMMED_TRANSFER_PARTIAL_HIT,
   DISKSIM_DISK_READ_HIT_OVER_AFTER_READ,
   DISKSIM_DISK_READ_HIT_OVER_AFTER_WRITE,
   DISKSIM_DISK_READ_MISS_OVER_AFTER_READ,
   DISKSIM_DISK_READ_MISS_OVER_AFTER_WRITE,
   DISKSIM_DISK_WRITE_HIT_OVER_AFTER_READ,
   DISKSIM_DISK_WRITE_HIT_OVER_AFTER_WRITE,
   DISKSIM_DISK_WRITE_MISS_OVER_AFTER_READ,
   DISKSIM_DISK_WRITE_MISS_OVER_AFTER_WRITE,
   DISKSIM_DISK_READ_COMPLETION_OVERHEAD,
   DISKSIM_DISK_WRITE_COMPLETION_OVERHEAD,
   DISKSIM_DISK_DATA_PREPARATION_OVERHEAD,
   DISKSIM_DISK_FIRST_RESELECT_OVERHEAD,
   DISKSIM_DISK_OTHER_RESELECT_OVERHEAD,
   DISKSIM_DISK_READ_DISCONNECT_AFTERREAD,
   DISKSIM_DISK_READ_DISCONNECT_AFTERWRITE,
   DISKSIM_DISK_WRITE_DISCONNECT_OVERHEAD,
   DISKSIM_DISK_EXTRA_WRITE_DISCONNECT,
   DISKSIM_DISK_EXTRADISC_COMMAND_OVERHEAD,
   DISKSIM_DISK_EXTRADISC_DISCONNECT_OVERHEAD,
   DISKSIM_DISK_EXTRADISC_INTER_DISCONNECT_DELAY,
   DISKSIM_DISK_EXTRADISC_2ND_DISCONNECT_OVERHEAD,
   DISKSIM_DISK_EXTRADISC_SEEK_DELTA,
   DISKSIM_DISK_MINIMUM_SEEK_DELAY,
   DISKSIM_DISK_IMMEDIATE_BUFFER_READ,
   DISKSIM_DISK_IMMEDIATE_BUFFER_WRITE
} disksim_disk_param_t;

#define DISKSIM_DISK_MAX_PARAM		DISKSIM_DISK_IMMEDIATE_BUFFER_WRITE
extern void * DISKSIM_DISK_loaders[];
extern lp_paramdep_t DISKSIM_DISK_deps[];


static struct lp_varspec disksim_disk_params [] = {
   {"Model", BLOCK, 1 },
   {"Scheduler", BLOCK, 1 },
   {"Max queue length", I, 1 },
   {"Bulk sector transfer time", D, 1 },
   {"Segment size (in blks)", I, 1 },
   {"Number of buffer segments", I, 1 },
   {"Print stats", I, 1 },
   {"Per-request overhead time", D, 1 },
   {"Time scale for overheads", D, 1 },
   {"Hold bus entire read xfer", I, 1 },
   {"Hold bus entire write xfer", I, 1 },
   {"Allow almost read hits", I, 1 },
   {"Allow sneaky full read hits", I, 1 },
   {"Allow sneaky partial read hits", I, 1 },
   {"Allow sneaky intermediate read hits", I, 1 },
   {"Allow read hits on write data", I, 1 },
   {"Allow write prebuffering", I, 1 },
   {"Preseeking level", I, 1 },
   {"Never disconnect", I, 1 },
   {"Avg sectors per cylinder", I, 1 },
   {"Maximum number of write segments", I, 1 },
   {"Use separate write segment", I, 1 },
   {"Low (write) water mark", D, 1 },
   {"High (read) water mark", D, 1 },
   {"Set watermark by reqsize", I, 1 },
   {"Calc sector by sector", I, 1 },
   {"Enable caching in buffer", I, 1 },
   {"Buffer continuous read", I, 1 },
   {"Minimum read-ahead (blks)", I, 1 },
   {"Maximum read-ahead (blks)", I, 1 },
   {"Read-ahead over requested", I, 1 },
   {"Read-ahead on idle hit", I, 1 },
   {"Read any free blocks", I, 1 },
   {"Fast write level", I, 1 },
   {"Combine seq writes", I, 1 },
   {"Stop prefetch in sector", I, 1 },
   {"Disconnect write if seek", I, 1 },
   {"Write hit stop prefetch", I, 1 },
   {"Read directly to buffer", I, 1 },
   {"Immed transfer partial hit", I, 1 },
   {"Read hit over. after read", D, 1 },
   {"Read hit over. after write", D, 1 },
   {"Read miss over. after read", D, 1 },
   {"Read miss over. after write", D, 1 },
   {"Write hit over. after read", D, 1 },
   {"Write hit over. after write", D, 1 },
   {"Write miss over. after read", D, 1 },
   {"Write miss over. after write", D, 1 },
   {"Read completion overhead", D, 1 },
   {"Write completion overhead", D, 1 },
   {"Data preparation overhead", D, 1 },
   {"First reselect overhead", D, 1 },
   {"Other reselect overhead", D, 1 },
   {"Read disconnect afterread", D, 1 },
   {"Read disconnect afterwrite", D, 1 },
   {"Write disconnect overhead", D, 1 },
   {"Extra write disconnect", I, 1 },
   {"Extradisc command overhead", D, 1 },
   {"Extradisc disconnect overhead", D, 1 },
   {"Extradisc inter-disconnect delay", D, 1 },
   {"Extradisc 2nd disconnect overhead", D, 1 },
   {"Extradisc seek delta", D, 1 },
   {"Minimum seek delay", D, 1 },
   {"Immediate buffer read", I, 1 },
   {"Immediate buffer write", I, 1 },
   {0,0,0}
};
#define DISKSIM_DISK_MAX 65
static struct lp_mod disksim_disk_mod = { "disksim_disk", disksim_disk_params, DISKSIM_DISK_MAX, (lp_modloader_t)disksim_disk_loadparams,  0, 0, DISKSIM_DISK_loaders, DISKSIM_DISK_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_DISK_PARAM_H

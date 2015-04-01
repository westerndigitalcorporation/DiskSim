#ifndef __MEMS_BUFFER_H_
#define __MEMS_BUFFER_H_

#include "mems_global.h"

void
move_segment_to_head (struct mems_segment *tmpseg,
		      mems_t *dev);

int
mems_buffer_check (int firstblock,
		   int lastblock,
		   mems_t *dev);

void
mems_buffer_insert (int firstblock,
		    int lastblock,
		    mems_t *dev);

#endif

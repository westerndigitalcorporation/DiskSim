#include "mems_global.h"
#include "mems_buffer.h"

/************************************************************************
 * Buffer functions
 *
 * FIXME: This is by no means a good/comprehensive way to account
 * for block buffering, but it works for quick comparisons with disks.
 ************************************************************************/

void
move_segment_to_head (struct mems_segment *tmpseg,
		      mems_t *dev) 
{
  if ((dev->numsegs > 1) && (tmpseg != dev->seglist))
    {
      tmpseg->prev->next = tmpseg->next;
      if (tmpseg->next)
	{
	  tmpseg->next->prev = tmpseg->prev;
	}

      tmpseg->next = dev->seglist;
      dev->seglist = tmpseg;
      tmpseg->next->prev = tmpseg;
      tmpseg->prev = NULL;
    }
}

/* Returns zero if all blocks not found in buffer; otherwise success */
int
mems_buffer_check (int firstblock,
		   int lastblock,
		   mems_t *dev)
{
  struct mems_segment *seg;
  int blk = firstblock;

  if (!dev->numsegs) return 0;

  dev->stat.num_buffer_accesses++;

  /* For each block in the range [firstblock,lastblock], check to
   * see if the block is cached.  If ALL blocks are cached, return
   * success.
   *
   * This looks convoluted but really isn't. */

  seg = dev->seglist;
  while (seg)
    {
      if ((blk >= seg->startblkno && blk < seg->endblkno))
	{
	  blk = seg->endblkno;
	  move_segment_to_head(seg, dev);	/* seg points to head of list */
	  if (blk >= lastblock)
	    {
	      dev->stat.num_buffer_hits++;
	      return 1;
	    }
	}
      seg = seg->next;	/* seg now points to second element in list */
    }

  return 0;	/* Failure */
}

void
mems_buffer_insert (int firstblock,
		    int lastblock,
		    mems_t *dev)
{
  struct mems_segment *seg;
  int blk = firstblock;

  if (!dev->numsegs) return;

  if ((lastblock - firstblock + 1) > dev->segsize)
    {
      printf("lastblock = %d, firstblock = %d, diff = %d, segsize = %d\n",
	     lastblock, firstblock, (lastblock - firstblock + 1), dev->segsize);
    }
  assert((lastblock - firstblock + 1) <= dev->segsize);
  
  /* First, check if any segments contain firstblock, or if firstblock
   * is the "next" block for a segment.  For example, if a segment
   * contains blocks 10--20, then firstblock==[10,21] matches the
   * segment. */
  for (seg = dev->seglist; seg; seg = seg->next)
    {
      if ((firstblock >= seg->startblkno) && 
	  (firstblock <= (seg->endblkno + 1))) break;
    }
  if (seg)
    {
      /* If the segment already contains firstblock--lastblock,
       * all we need to do is move the segment to the head of MRU list */
      if ((firstblock >= seg->startblkno) && (lastblock <= seg->endblkno))
	{
	  move_segment_to_head(seg, dev);
	  return;
	}
      
      /* Else, add these blocks to the segment.  If we fill up the segment
       * while doing this, we move this segment to the front, then skip
       * to below to put the rest of the blocks in the LRU segment. */
      if ((seg->endblkno - seg->startblkno + 1) < dev->segsize)
	{
	  move_segment_to_head(seg, dev);
	  for (blk = firstblock; blk <= lastblock; blk++)
	    {
	      if (seg->endblkno < blk)
		{
		  seg->endblkno = blk;
		  if ((seg->endblkno - seg->startblkno + 1) >= dev->segsize)
		    break;
		}
	    }
	  if (blk >= lastblock) return;
	}
    }

  /* If we reach here, there are still blocks to put in the buffer.
   * Find the LRU segment, and put the remaining blocks there. */
  seg = dev->seglist;
  while (seg->next) seg = seg->next;
  move_segment_to_head(seg, dev);
  seg->startblkno = blk;
  seg->endblkno = lastblock;

  return;
}

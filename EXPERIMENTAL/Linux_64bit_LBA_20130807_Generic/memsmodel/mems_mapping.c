/*
 * DiskSim Storage Subsystem Simulation Environment (Version 4.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty" statements
 * are included with all reproductions and derivative works and associated
 * documentation. This software may also be redistributed without charge
 * provided that the copyright and "No Warranty" statements are included
 * in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
 * OR DOCUMENTATION.
 *
 */


/* mems_mapping.c
 *
 * Functions to map logical block numbers to physical locations on
 * the memsdevice.
 */

#include "mems_global.h"
#include "mems_mapping.h"


mems_sled_t *
mems_lbn_to_sled (mems_t *dev, int lbn)
{
  /* This function determines to which sled a particular request maps,
   * and returns that sled number [0 through N-1].
   *
   * This function may (easily) be extended to support data
   * replication -- need to add support in mems_request_arrive
   * in order to do this.
   */

  int i;

  if (lbn < 0 || lbn >= dev->numblocks) {
    fprintf(stderr, "LBN %d outside valid range (%d)\n", lbn, dev->numblocks);
    exit(1);
  }

  /* The obvious case: only one sled */
  if (dev->num_sleds == 1) return &dev->sled[0];

  /* Simple division:
   * If there are 100 sectors across 4 sleds, they are divided as:
   *	 0-24	sled 0
   *	25-49	sled 1
   *	50-74	sled 2
   *	75-99	sled 3
   */
  for (i = 0; i < dev->num_sleds; i++) {
    int sled_lastblock = (i+1) * dev->numblocks / dev->num_sleds - 1;
    if (lbn <= sled_lastblock) break;
  }

  // fprintf(stderr, "LBN %d maps to sled %d (numblocks=%d, num_sleds=%d)\n", lbn, i, dev->numblocks, dev->num_sleds);

  return &dev->sled[i];
}


/* Terminology time.
 *
 * A "cell" is the set of tipsectors used to read a block.  Example:
 * If the sled is at <0,0> and you turn on 64 tips to read block 0,
 * the 64 tipsectors read form a "cell".
 *
 * A "row" is a group of cells that can be read simultaneously.
 * Example:  If the sled is at <0,0> and you can turn on 640 tips
 * simultaneously, then you have a "row" of 10 cells.
 *
 * A "column" is a group of rows that can be read in a single sweep
 * of the sled.  If there are 5 columns, then it takes 5 sweeps to
 * read all the data in a cylinder (e.g., all the data at a given
 * X offset).
 */

static inline int rows_per_column (mems_sled_t *sled) {
  return 
    (sled->y_length_nm / sled->bit_length_nm - sled->servo_burst_length_bits)
    / (sled->servo_burst_length_bits + sled->tip_sector_length_bits);
}

static inline int columns_per_cylinder (mems_sled_t *sled) {
  return sled->tips_usable / sled->tips_simultaneous;
}

static inline int cells_per_row (mems_sled_t *sled) {
  return sled->tips_simultaneous / sled->tip_sectors_per_lbn;
}

static inline int cells_per_column (mems_sled_t *sled) {
  return cells_per_row(sled) * rows_per_column(sled);
}

static inline int cells_per_cylinder (mems_sled_t *sled) {
  return cells_per_column(sled) * columns_per_cylinder(sled);
}

static inline void 
shift_coordset (tipsector_coord_set_t *cs, mems_sled_t *sled)
{
  int x_shift = sled->x_length_nm / sled->bit_length_nm / 2;
  int y_shift = (rows_per_column(sled) * (sled->servo_burst_length_bits + sled->tip_sector_length_bits) + sled->servo_burst_length_bits) / 2;

  /* Shift the X position right by (num_cylinders/2) */
  cs->servo_start.x_pos     -= x_shift;
  cs->tipsector_start.x_pos -= x_shift;
  cs->tipsector_end.x_pos   -= x_shift;

  /* Shift the Y position up.  This uses a slightly more complicated
   * formula (FIXME: explain it!) */
  cs->servo_start.y_pos     -= y_shift;
  cs->tipsector_start.y_pos -= y_shift;
  cs->tipsector_end.y_pos   -= y_shift;
}


void
mems_lbn_to_position (int lbn,
		      mems_sled_t *sled,
		      tipsector_coord_set_t *up,
		      tipsector_coord_set_t *dn,
		      tipset_t *tipset,
		      int *cylptr, int *surfaceptr, int *blkptr)
{
  /* This function maps a logical block number to a set of starting
   * and ending coordinates and velocities. */

  int locallbn            = lbn % sled->numblocks;
  int y_access_speed_nm_s = sled->y_access_speed_bit_s * sled->bit_length_nm;
  int cylinder;
  int column;
  int row;
  int cell;

  cylinder =  locallbn / cells_per_cylinder(sled);
  column   = (locallbn % cells_per_cylinder(sled)) / cells_per_column(sled);
  row      = (locallbn % cells_per_column(sled)) / cells_per_row(sled);
  cell     =  locallbn % cells_per_row(sled);
  
  /* X position is always the cylinder number */
  up->servo_start.x_pos = up->tipsector_start.x_pos = up->tipsector_end.x_pos = cylinder;
  dn->servo_start.x_pos = dn->tipsector_start.x_pos = dn->tipsector_end.x_pos = cylinder;
  
  /* Velocity is always positive in "up" direction, negative in "dn" */
  up->servo_start.y_vel = up->tipsector_start.y_vel = up->tipsector_end.y_vel = y_access_speed_nm_s;
  dn->servo_start.y_vel = dn->tipsector_start.y_vel = dn->tipsector_end.y_vel = -y_access_speed_nm_s;
  
  /* Row inversion:
   * Rows are inverted on odd-numbered columns because of the
   * streaming nature of the layout:
   *
   *       cylinder0
   *    col0  col1  col2
   *    ----  ----  ----
   *    row0  row2  row0
   *    row1  row1  row1
   *    row2  row0  row2
   *
   * However, if [and only if] columns_per_cylinder() is odd,
   * the even-numbered columns should be inverted instead:
   *
   *                          cylinder1
   *                       col0  col1  col2
   *                       ----  ----  ----
   *                       row0  row2  row0
   *                       row1  row1  row1
   *                       row2  row0  row2
   *
   * In this example, you can immediately turn around from
   * cyl0/col2/row2 and start streaming from cyl1/col0/row0.
   */
  
  if (sled->layout_policy == MEMS_LAYOUT_STREAMING) {
    if (columns_per_cylinder(sled) % 2) {
      if (cylinder % 2) {
	/* odd cylinder, so invert even-numbered columns */
	if (!(column % 2)) row = rows_per_column(sled) - row - 1;
      } else {
	/* even cylinder, so invert odd-numbered columns */
	if (column % 2) row = rows_per_column(sled) - row - 1;
      }
    } else {
      /* always invert odd-numbered columns */
      if (column % 2) row = rows_per_column(sled) - row - 1;
    }
  }
  
  /* Calculate the "up" Y positions */
  up->servo_start.y_pos = row * (sled->servo_burst_length_bits + sled->tip_sector_length_bits);
  up->tipsector_start.y_pos = up->servo_start.y_pos + sled->servo_burst_length_bits;
  up->tipsector_end.y_pos = up->tipsector_start.y_pos + sled->tip_sector_length_bits;
  
  /* Calculate the "dn" Y positions based on the "up" positions */
  dn->servo_start.y_pos = up->tipsector_end.y_pos + sled->servo_burst_length_bits;
  dn->tipsector_start.y_pos = up->tipsector_end.y_pos;
  dn->tipsector_end.y_pos = up->tipsector_start.y_pos;
  
  /* Tipset found by [offset in current column] + [column shift] */
  tipset->tip_start = cell * sled->tip_sectors_per_lbn + column * sled->tips_simultaneous;
  tipset->tip_end = tipset->tip_start + sled->tip_sectors_per_lbn - 1;
  tipset->num_tips = tipset->tip_end - tipset->tip_start + 1;
  
  /* Finally, "shift" the positions so the sled is centered on [0,0] */
  shift_coordset(up, sled);
  shift_coordset(dn, sled);
  
  /* Note: These must be -below- the row inversion above! */
  if (cylptr) *cylptr = cylinder;
  if (surfaceptr) *surfaceptr = column;
  if (blkptr) *blkptr = row * cells_per_row(sled) + cell;

}


int
mems_get_number_of_cylinders (mems_t *dev)
{
  return dev->sled[0].x_length_nm / dev->sled[0].bit_length_nm;
}


int
mems_get_blocks_per_cylinder (mems_sled_t *sled)
{
  return cells_per_cylinder(sled);
}


void
mems_check_numblocks (mems_t *dev) {
  int numcylinders = mems_get_number_of_cylinders(dev);
  int numblocks = cells_per_cylinder(&dev->sled[0]) * numcylinders * dev->num_sleds;
  int i;

  if (dev->numblocks == 0) {	/* Set numblocks here */
    dev->numblocks = numblocks;
    for (i = 0; i < dev->num_sleds; i++) dev->sled[i].numblocks = numblocks / dev->num_sleds;
    /*
    fprintf(stderr, "Setting dev->numblocks = %d, sled->numblocks = %d\n", dev->numblocks, dev->sled[0].numblocks);
    */
  } else {	/* Check numblocks and warn if necessary */
    if (dev->numblocks != numblocks) {
      fprintf(stderr, "Warning: dev->numblocks inconsistent (spec: %d, actual: %d)\n", dev->numblocks, numblocks);
    }
    for (i = 0; i < dev->num_sleds; i++) {
      if (dev->sled[i].numblocks != numblocks / dev->num_sleds) {
	fprintf(stderr, "Warning: sled[%d]->numblocks inconsistent (spec: %d, actual: %d)\n", i, dev->sled[i].numblocks, numblocks / dev->num_sleds);
      }
    }
  }
  return;
}


int
mems_centermost_lbn (mems_sled_t * sled) {
  /* Horiz offset = (x_length_bits / 2) times cells_per_cylinder */
  /* Vertical offset = 1/2 cells_per_cylinder */
  int horiz_offset = (sled->x_length_nm / sled->bit_length_nm / 2) * cells_per_cylinder(sled);
  int vert_offset = cells_per_cylinder(sled) / 2;

  return horiz_offset + vert_offset;
}

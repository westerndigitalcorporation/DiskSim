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


#ifndef _MEMS_MAPPING_H_
#define _MEMS_MAPPING_H_

#include "mems_global.h"
#include "mems_internals.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Mapping Data Types  -=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef int lbn_t;

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Constants and macros  -=-=-=-=-=-=-=-=-=-=-=-=-*/

/* memsdevice data layout_policy defines */
// #define MEMSDEVICE_DATALAYOUT_SEQUENTIAL		1
// #define MEMSDEVICE_DATALAYOUT_SHORTTRACK		2
// #define MEMSDEVICE_DATALAYOUT_LONGTRACK		3
// #define MEMS_DATALAYOUT_SEQUENTIALLY_OPTIMAL	        4
// #define MEMS_DATALAYOUT_TIPSWITCH_SUBTRACK		5
// #define MEMS_DATALAYOUT_COLUMNSWITCH_FULLTRACK	6
#define MEMS_LAYOUT_SIMPLE                      6
#define MEMS_LAYOUT_STREAMING			7

/*-=-=-=-=-=-=-=-=-=-=-=-=-  Function Prototypes  -=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * mems_lbn_to_position()
 *
 * maps an lbn to a set of coordinates, and a set of tips
 *
 * inputs:  lbn_t lbn - logical block number to map
 *          msled_t *sled - pointer to set to sled structure
 *          tipsector_coord_set *up - pointer to the "up" coordinate set
 *          tipsector_coord_set *down - pointer to the "down" coordinate set
 *          tipset_t *tipset - the set of tips required to read this lbn
 *
 * outputs: int *cylptr     - X-offset (cylinder) of block
 *	    int *surfaceptr - column offset of block
 *	    int *blkptr     - block offset within column
 *
 * modifies:  sled - point to the sled structure
 *            up - set to the "up" coordinate set
 *            down - set to the "down" coordinate set
 *            tipset - set to the set of tips required to read this lbn
 *          
 */

void mems_lbn_to_position(lbn_t lbn,
			  mems_sled_t *sled,
			  tipsector_coord_set_t *up,
			  tipsector_coord_set_t *down,
			  tipset_t *tipset,
			  int *cylptr, int *surfaceptr, int *blkptr);

/*
 * mems_lbn_to_sled()
 *
 * maps an lbn to a sled
 *
 * inputs:  mems_t *dev - a pointer to the memsdevice structure
 *          lbn_t lbn - the lbn to be mapped
 *
 * outputs: mems_sled_t * -  a pointer to the msled_t structure
 *				   this lbn maps to
 *
 * modifies:  none
 * 
 */

mems_sled_t *mems_lbn_to_sled(mems_t *dev, lbn_t lbn);

/*
 * mems_check_numblocks()
 *
 * checks the number of blocks specified in the device to see that it matches
 * the layout specified.  if incorrect, then it updates numblocks.
 *
 * inputs:  mems_t *dev - a pointer to the memsdevice structure
 *
 * outputs:  none
 *
 * modifies:  dev - if the number of blocks specified does not match the layout
 *
 */

void mems_check_numblocks(mems_t *dev);

/*
 * mems_centermost_lbn()
 *
 * provides the lbn where the sled is closest to the centermost position.
 *
 * inputs: 	mems_sled_t *sled - pointer to the sled
 *
 * outputs: 	int lbn
 *
 * modifies: 	none
 */

int mems_centermost_lbn (mems_sled_t * sled);

/*
 * mems_get_number_of_cylinders()
 *
 * provides the number of cylinders (e.g., distinct X bit positions).
 *
 * inputs:	mems_t *dev - pointer to device
 *
 * outputs:	int - number of cylinders
 *
 * modifies:	none
 */

int mems_get_number_of_cylinders (mems_t *dev);

/*
 * mems_get_blocks_per_cylinder()
 *
 * provides the number of blocks per cylinder on the sled
 *
 * inputs:	mems_sled_t *sled - pointer to sled
 *
 * outputs:	int - number of blocks per cylinder
 *
 * modifies:	none
 */

int mems_get_blocks_per_cylinder (mems_sled_t *sled);

#endif

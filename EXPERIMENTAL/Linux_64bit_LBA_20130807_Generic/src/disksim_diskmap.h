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



/*
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
 *
 * Permission to reproduce, use, and prepare derivative works of
 * this software for internal use is granted provided the copyright
 * and "No Warranty" statements are included with all reproductions
 * and derivative works. This software may also be redistributed
 * without charge provided that the copyright and "No Warranty"
 * statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
 * TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 */

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */

#ifndef DISKSIM_DISKMAP_H
#define DISKSIM_DISKMAP_H


/* Sizing defines for different mapping related fields */

#define MAXSLIPS	2048
#define MAXDEFECTS	2048


/* Disk sparing schemes (for replacing bad media) */

#define NO_SPARING                            0
#define TRACK_SPARING                         1
#define SECTPERCYL_SPARING                    2
#define SECTPERTRACK_SPARING                  3
#define SECTPERCYL_SPARING_SLIPTOEND          4
#define SECTPERCYL_SPARING_ATFRONT            5
#define SECTPERCYL_SPARING_ATFRONT_SLIPTOEND  6
#define SECTATEND_SPARING                     7
#define SECTPERRANGE_SPARING                  8
#define SECTSPERZONE_SPARING                  9
#define SECTSPERZONE_SPARING_SLIPTOEND       10
#define MAXSPARESCHEME                       10


/* Disk layout algorithms */

#define LAYOUT_NORMAL           0
#define LAYOUT_CYLSWITCHONSURF1 1   /* cyl. switches stay on same surface  */
                                    /* -- first cyl of each zone is normal */
#define LAYOUT_CYLSWITCHONSURF2 2   /* cyl. switches stay on same surface  */
                                    /* -- all even-#d cyls are normal      */
#define LAYOUT_SERPENTINE       3   /* planning for the future */
#define LAYOUT_RAWFILE          4   /* the layout is in block-to-block file*/
#define LAYOUT_MAX              4


/* convenient macros */

#define isspareatfront(currdisk) \
                (((currdisk)->sparescheme == SECTPERCYL_SPARING_ATFRONT) || \
                 ((currdisk)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND))

#define issliptoend(currdisk) \
                (((currdisk)->sparescheme == SECTPERCYL_SPARING_SLIPTOEND) || \
                 ((currdisk)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND) || \
                 ((currdisk)->sparescheme == SECTATEND_SPARING))

#define issectpercyl(currdisk) \
                (((currdisk)->sparescheme == SECTPERCYL_SPARING) || \
                 ((currdisk)->sparescheme == SECTPERCYL_SPARING_SLIPTOEND) || \
                 ((currdisk)->sparescheme == SECTPERCYL_SPARING_ATFRONT) || \
                 ((currdisk)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND))


/* disksim_diskmap.c functions */

struct band;
struct disk;

void diskmap_initialize (struct disk *currdisk);

struct band * disk_translate_lbn_to_pbn (struct disk *currdisk, int blkno, int maptype, int *cylptr, int *surfaceptr, int *blkptr);

int disk_translate_pbn_to_lbn (struct disk *currdisk, struct band *currband, int cylno, int surfaceno, int blkno);

double disk_map_pbn_skew (struct disk *currdisk, struct band *currband, int cylno, int surfaceno);

void disk_get_lbn_boundaries_for_track (struct disk *currdisk, 
					struct band *currband, 
					int cylno, 
					int surfaceno, 
					int *startptr, 
					int *endptr);

void disk_check_numblocks (struct disk *currdisk);

int disk_compute_blksinband (struct disk *currdisk, struct band *currband);


int disk_pbn_to_lbn_sectpertrackspare(struct disk *currdisk, 
				      struct band *currband,
				      int cylno, int surfaceno,
				      int blkno);

int disk_pbn_to_lbn_sectpercylspare(struct disk *currdisk, 
				    struct band *currband,
				    int cylno, int surfaceno,
				    int blkno);

int disk_pbn_to_lbn_sectperrangespare(struct disk *currdisk, 
				      struct band *currband,
				      int cylno, int surfaceno,
				      int blkno);

int disk_pbn_to_lbn_sectperzonespare(struct disk *currdisk, 
				     struct band *currband,
				     int cylno, int surfaceno,
				     int blkno);

int disk_pbn_to_lbn_trackspare(struct disk *currdisk, 
			       struct band *currband,
			       int cylno, int surfaceno, 
			       int blkno);


int disk_pbn_to_lbn_nospare(struct disk *currdisk, 
			       struct band *currband,
			       int cylno, int surfaceno, 
			       int blkno);

int disk_pbn_to_lbn_rawlayout(struct disk *currdisk, 
			      struct band *currband, 
			      int cylno,
			      int surfaceno, 
			      int blkno);




#endif   /* DISKSIM_DISKMAP_H */


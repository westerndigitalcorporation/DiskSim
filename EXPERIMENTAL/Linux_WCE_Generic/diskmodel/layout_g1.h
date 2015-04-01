/* diskmodel (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
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


// carry-over of old disksim layout code
#ifndef _DM_LAYOUT_G1_H
#define _DM_LAYOUT_G1_H
#include "dm.h"


// Disk sparing schemes (for replacing bad media)
typedef enum {
  NO_SPARING                           = 0,
  TRACK_SPARING                        = 1,
  SECTPERCYL_SPARING                   = 2,
  SECTPERTRACK_SPARING                 = 3,
  SECTPERCYL_SPARING_SLIPTOEND         = 4,
  SECTPERCYL_SPARING_ATFRONT           = 5,
  SECTPERCYL_SPARING_ATFRONT_SLIPTOEND = 6,
  SECTATEND_SPARING                    = 7,
  SECTPERRANGE_SPARING                 = 8,
  SECTSPERZONE_SPARING                 = 9,
  SECTSPERZONE_SPARING_SLIPTOEND       = 10
} dm_layout_g1_spare_t;
#define MAXSPARESCHEME SECTSPERZONE_SPARING_SLIPTOEND

// layout types
typedef enum {
  LAYOUT_NORMAL           = 0,
  LAYOUT_CYLSWITCHONSURF1 = 1,  /* cyl. switches stay on same surface  
                                 * -- first cyl of each zone is normal 
				 */

  LAYOUT_CYLSWITCHONSURF2 = 2   /* cyl. switches stay on same surface  
				 * -- all even-#d cyls are normal      
				 */
} dm_layout_g1_map_t;

#define LAYOUT_MAX LAYOUT_CYLSWITCHONSURF2



// stupid macros
#define isspareatfront(l) \
                (((l)->sparescheme == SECTPERCYL_SPARING_ATFRONT) || \
                 ((l)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND))

#define issliptoend(l) \
                (((l)->sparescheme == SECTPERCYL_SPARING_SLIPTOEND) || \
                 ((l)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND) || \
                 ((l)->sparescheme == SECTATEND_SPARING))

#define issectpercyl(l) \
                (((l)->sparescheme == SECTPERCYL_SPARING) || \
                 ((l)->sparescheme == SECTPERCYL_SPARING_SLIPTOEND) || \
                 ((l)->sparescheme == SECTPERCYL_SPARING_ATFRONT) || \
                 ((l)->sparescheme == SECTPERCYL_SPARING_ATFRONT_SLIPTOEND))



struct dm_layout_g1_band;


struct dm_layout_g1 {
  struct dm_layout_if hdr;
  struct dm_disk_if *disk; // back pointer

  dm_layout_g1_spare_t  sparescheme;
  int                   rangesize;
  dm_layout_g1_map_t    mapping;


  struct dm_layout_g1_band *bands;
  int bands_len;
  int         *band_blknos;    // first lbn per band indexed by band
  dm_skew_unit_t skew_units;
};



struct dm_layout_g1_band {
  int    startcyl;
  int    endcyl;
  int    blkspertrack;
  int    deadspace;

  dm_angle_t firstblkangle;
  dm_angle_t cylskew;
  dm_angle_t trackskew;

  int    blksinband;
  int    sparecnt;
  int    numslips;
  int    numdefects;
  int   *slip;
  int   *defect;
  int   *remap;
  int   num; /* number of this band */
  /*     int  *remap; */

  dm_angle_t sector_width;

  // dead space at the end of each track
  // blkspertrack * sector_width + trkspace should equal the number of 
  // ticks in a circle
  dm_angle_t trkspace;  


  dm_skew_unit_t skew_units;
}; 


// the interfaces
extern struct dm_layout_if g1_layout_nosparing;
extern struct dm_layout_if g1_layout_tracksparing;
extern struct dm_layout_if g1_layout_sectpertrackspare;
extern struct dm_layout_if g1_layout_sectpercylspare;
extern struct dm_layout_if g1_layout_sectperrangespare;
extern struct dm_layout_if g1_layout_sectperzonespare;

#endif   /*  _DM_LAYOUT_G1_H  */

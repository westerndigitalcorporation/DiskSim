/* diskmodel (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2003.
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

// g1 layout seek function prototypes.

#ifndef _DM_MECH_G1_PRIVATE_H
#define _DM_MECH_G1_PRIVATE_H


dm_time_t 
dm_mech_g1_seek_const(struct dm_disk_if *d,
		      struct dm_mech_state *begin,
		      struct dm_mech_state *end,
		      int rw);

dm_time_t 
dm_mech_g1_seek_3pt_curve(struct dm_disk_if *d,
			  struct dm_mech_state *begin,
			  struct dm_mech_state *end,
			  int rw);

dm_time_t 
dm_mech_g1_seek_3pt_line(struct dm_disk_if *d,
			 struct dm_mech_state *begin,
			 struct dm_mech_state *end,
			 int rw);

dm_time_t 
dm_mech_g1_seek_hpl(struct dm_disk_if *d,
		    struct dm_mech_state *begin,
		    struct dm_mech_state *end,
		    int rw);

dm_time_t 
dm_mech_g1_seek_1st10_plus_hpl(struct dm_disk_if *d, 
			       struct dm_mech_state *begin,
			       struct dm_mech_state *end,
			       int rw);

dm_time_t 
dm_mech_g1_seek_extracted(struct dm_disk_if *d,
			  struct dm_mech_state *begin,
			  struct dm_mech_state *end,
			  int rw);


extern dm_mech_g1_seekfn dm_mech_g1_seekfns[];
// grrr ... keep this up to date with the table in mech_g1_seektime.c
#define DM_MECH_G1_SEEKFNS_LEN 6



#endif //  _DM_MECH_G1_PRIVATE_H

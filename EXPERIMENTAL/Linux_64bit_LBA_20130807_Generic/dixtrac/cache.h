/*
 * DIXtrac - Automated Disk Extraction Tool
 * Authors: Jiri Schindler, John Bucy, Greg Ganger
 *
 * Copyright (c) of Carnegie Mellon University, 1999-2005.
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

#ifndef __CACHE_H
#define __CACHE_H

#define MAX_SEGMENTS              1024

void cache_init(void);

int cache_discard_req_after_read(void);
int cache_prefetch_after_one_block_read(void);
int cache_allow_prefetch_after_read(void);
int cache_prefetch_track_boundary(void);
int cache_prefetch_cylinder_boundary(void);
int cache_read_free_blocks(void);

int cache_prefetch_size(void);
int cache_min_prefetch_size(void);
int cache_max_prefetch_size(void);

int cache_min_read_ahead(void);
int cache_max_read_ahead(void);

int cache_minimum_read_ahead(void);
int cache_read_on_arrival(void);
int cache_write_on_arrival(void);

int cache_writes_cached(void);
int cache_writes_appended(void);

int cache_contig_writes_appended(void);

int cache_segment_size(void);
int cache_share_rw_segment(void);
int cache_count_read_segments(void);

int disksim_count_all_segments(void);
int disksim_continuous_read(void);
int disksim_caching_in_buffer(void);
int disksim_fast_write(void);

int cd_segment_size(void);
int cd_discard_req_after_read(void);
int cd_share_rw_segment(void);
int cd_read_free_blocks(void);
int cd_read_on_arrival(void);
int cd_write_on_arrival(void);
int cd_contig_writes_appended(void);
int cd_count_write_segments(void);
int cd_number_of_segments(void);

/* internal functions */

#endif

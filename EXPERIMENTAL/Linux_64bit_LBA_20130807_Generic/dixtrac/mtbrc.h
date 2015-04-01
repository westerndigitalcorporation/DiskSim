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

#ifndef __MTBRC_H
#define __MTBRC_H

/* keep it as a multiple of CYL_DISTANCES  */
#define TRIALS                  12
#define REPETITIONS             9
#define COARSE_REPEATS          (REPETITIONS / 3)

#define REGULAR_MTBRC           1
#define COMPL_OVERHEAD_MTBRC    2
#define SEEKDIST_MTBRC          3

/* threshold for rejecting a request which suffered extra rotation, in revs */
#define MISS_THRESHOLD           0.75

#define HIT_PERCENTILE           75
#define COARSE_HIT_PERCENTILE    25

#define COARSE_OFFSET_INCR       3

/* max sector offset from the begining of the cylinder for X */
#define MAX_SECT_OFFSET          1

/* constants for various req types to MTBRC */
/* Y request uses the same track */
#define MTBRC_SAME_TRACK         0
/* Y request uses the next track in the same cylinder, head switch incurred */
#define MTBRC_NEXT_TRACK         1

#define MTBRC_READ_HIT           2
#define MTBRC_READ_MISS          3

#define MTBRC_WRITE_HIT          4
/* #define MTBRC_CACHE_SAME_TRACK    3 */
#define MTBRC_NEXT_CYL           5

#define MTBRC_CYL_DIST           6

#define CYL_DISTANCES            4

// #define CYL_DIST_1               1
// #define CYL_DIST_2               2
// #define CYL_DIST_3               5
// #define CYL_DIST_4               9

#define CYL_DIST_1               2
#define CYL_DIST_2               2
#define CYL_DIST_3               2
#define CYL_DIST_4               2

/* constants for the size of the matrix solved in find_completion_times */
#define MATRIX_ROWS               9
#define MATRIX_COLS               9

unsigned int mtbrc(int rw1,int rw2,int req_type,float *hostdelay,
    float *ireqdist,float *ireqdist_time, float *med_xfer);

int
seek_mtbrc(int rw1,int rw2,int startcyl,int cyldist,
	   float *hostdelay,float *ireqdist,float *ireqdist_time,
	   float *med_xfer);

unsigned int compl_overhead(int rw1,int rw2,int req_type,float *hostdelay, 
    float *med_xfer);

void mtbrc_lbn_pair(int startcyl,int sect_offset,int req_type,int cyldist,
    int *lbn1,int *lbn2);

unsigned int mtbrc_run_commands(int req_type,int rw1,int rw2,int lbn1,int lbn2,
    int* hdel,long* cmd2_time);

long find_one_mtbrc(int startcyl,int req_type,int rw1,int rw2,int trials,
    int cyldist,long* host_delay,int* ireq_dist);

long 
find_one_mtbrc_dumb(int startcyl,
		    int req_type,
		    int rw1,
		    int rw2,
		    int trials,
		    int cyldist,
		    long* host_delay,
		    int* ireq_dist);

long 
find_one_mtbrc_bins(int startcyl,
		    int req_type,
		    int rw1,
		    int rw2,
		    int trials,
		    int cyldist,
		    long* host_delay,
		    int* ireq_dist);

void find_completion_times(float *rc,float *wc,float *rmaw,float *wmaw,
    float *wmar,float *rmar,float *rhar,float *rhaw,float *whaw,float *whar);

#endif

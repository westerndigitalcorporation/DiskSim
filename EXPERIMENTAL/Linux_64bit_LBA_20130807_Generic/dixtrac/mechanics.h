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

#ifndef __MECHANICS_H
#define __MECHANICS_H

int find_seek_time(int distance,
		   float *max, 
		   float *mean, 
		   float *variance,
		   int delay_us);

int 
find_seek_time_mtbrc(int distance, 
		     float *max, 
		     float *mean,
		     float *variance,
		     int delay_us);

// ten_seeks optional (pass 0 if you don't care)
void 
get_seek_times(float *single_cyl,
	       float *full_seek,
	       float *ten_seeks,
	       void(*seekout)(void *ctx, int dist, float time, float stddev),
	       void *seekout_ctx,
	       int delay_us);

int dx_seek_curve_len(void);

void get_rotation_speed(int *rot,float *error); 
void get_head_switch(double *);
void get_write_settle(double head_switch, double *result);


#endif

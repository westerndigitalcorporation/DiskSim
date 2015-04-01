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

#ifndef __DIXTRAC_H
#define __DIXTRAC_H

#include "config.h"
#include "build_scsi.h"

/* definitions of the various file extensions - put after model name */
#define LAYOUT_EXT         "bl"
#define SEEK_EXT           "seek"
#define SEEK_ALL_EXT       "all.seek"
#define DATA_EXT           "diskspecs"
#define PARAM_EXT          "parv"
#define LAYOUT_FULL_EXT    "layout.full"
#define LAYOUT_CLEAN_EXT   "layout.clean"
#define LAYOUT_MAP_EXT     "layout.mappings"
#define TRACES_EXT         "trace"
#define SYNTHTRACE_EXT     "synthtrace"
#define LLTRACE_EXT        "lltrace"
#define LBN_FILE_EXT       "lbn.requests"

/* if defined, the output file contains parameters relevalnt to DiskSim  */
/* mostly for generating a file that can be directly used in DiskSim     */
#define DISKSIM_PARAMS

/* enables address translation counter */
#define ADDR_TRANS_COUNTER

#define ONE_REVOLUTION     (long) (1000*get_one_rev_time())

#define N_SAMPLES     20
#define N_SETS         2

int  disk_init(char *device, int use_scsi_device);
void disk_shutdown(int use_scsi_device);

long time_second_command(scsi_command_t *c1,scsi_command_t *c2);
long time_two_commands(scsi_command_t *c1,scsi_command_t *c2);

void busy_wait_loop(long howlong);

void find_mean_max(int sets,int samples,long *tset, float *max, float *mean, float *variance);
int  bin_search(int condition,int *value,int *low,int *high,int *last_high,
		int *maxvalhit);

float get_host_delay(void); 
float trans_addr_delay(void); 
void warmup_disk(void); 
void dxobtain_layout(int extractlayout,int use_layout_file,int use_raw_layout,
		     int gen_complete_mappings,int use_mapping_file, 
		     char *model_name,int start_lbn, char *mapping_file);
void do_tests(char *model_name); 
#ifdef MEASURE_TIME
float elapsed_time(void);
#endif

/* definitions & fuctions for tweaking bits on the CACHE MODE page */

#define CACHE_WCE                 1
#define CACHE_RCD                 2
#define CACHE_MF                  3
#define CACHE_IC                  4
#define CACHE_CAP                 5
#define CACHE_DRA                 6
#define CACHE_DISC                7
#define CACHE_FSW                 8
#define CACHE_ABPF                9

#define MIN_READ_AHEAD            0
#define MAX_READ_AHEAD            1

void disable_caches(void); 
void enable_caches(void);
void enable_prefetch(void);
int cache_bit_savable(uint bit);
int set_cache_bit(uint bit,u_int8_t value);
u_int8_t get_cache_bit(uint bit);
char set_cache(char bits);
void get_cache_bytes(uint bnum,char bits[],char mask[],uint count);
void set_cache_bytes(uint bnum,char bits[],char mask[],uint count);
int cache_bytes_savable(uint bnum,char bits[],char mask[],uint count);
int get_cache_numsegs(void);
void set_cache_numsegs(int numsegs);
void get_cache_prefetch(unsigned int *min,unsigned int *max);
void set_cache_prefetch(unsigned int min,unsigned int max);
void find_track_boundries(void);

void set_mode_page_bytes(u_int8_t pagenum, uint bnum, char bits[], char mask[],
			 uint count);
void set_cache_buffer_full_ratio(int param);

#ifdef DEBUG
void do_debug_mode_tests(char *model_name,int param);
#endif
void do_in_out_seeks(char *model_name);

#endif



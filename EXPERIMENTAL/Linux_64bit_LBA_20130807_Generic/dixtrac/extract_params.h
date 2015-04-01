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

#ifndef __EXTRACT_PARAMS_H
#define __EXTRACT_PARAMS_H

#include "dixtrac.h"
#include "extract_layout.h"
#include "mtbrc.h"

#define ANY_DEFECT                0
#define NO_CYL_DEFECT             1
#define NO_TRACK_DEFECT           2

#define ANY_BAND                 9998
#define MAX_BAND                 9999

#define TRACK_SKEW     0
#define CYLINDER_SKEW  1
#define BAND_SKEW      2

void get_disk_label(char *label);
void get_detailed_disk_label(char *product,
			     char *vendor,
			     char *revision,
			     char *serialnum);

void get_geometry(int *cyls, int *rot, int *heads, int *rot_off);
void get_capacity(int *maxlbn,int *bsize);

void write_entry(FILE *fp,char *label,char *value);
void write_parfile_entry(FILE *fp,char *label,char *value);

void get_physical_address(int lbn,int *cyl,int *head,int *sector);
void get_lbn_address(int cyl, int head, int sector, int *lbn);

void generate_random_cylinders(int cyls[],int howmany,int defects);

int random_cylinder(int cushion,int defects,int band);

int diskinfo_get_first_valid_cyl(void);
int diskinfo_get_last_valid_cyl(void);

void diskinfo_physical_address(int lbn,int *cyl,int *head,int *sector);
void diskinfo_lbn_address(int cyl, int head, int sector, int *lbn);
void diskinfo_track_boundaries(int cyl, int head, int *lbn1, int *lbn2);
int  diskinfo_blks_in_zone(int lbn);
#ifdef DISKINFO_ONLY
band_t* diskinfo_get_band(int cyl);
#endif
int diskinfo_cyl_firstlbn(int cyl, int rand_max_offset);

int set_notch_page(int notch);
float get_bulk_transfer_time(void);
void decompose_overheads(float *a,float *b,int c1,int lbn1,int c2,int lbn2);

void get_skew_information(void(*skewout)(void *, int, float, float), 
			  void *skewout_ctx);

void get_water_marks(float *lw, float *hw, int *set_by_reqsize);

int diskinfo_random_band_cylinder(int bandnum, int cushion);

int diskinfo_sect_per_track(int cyl);
float get_one_rev_time(void);

/* mathematical functions */
extern void gaussj(float a[][MATRIX_COLS-1], int n, float b[][2], int m);
extern void fit(float x[], float y[], int ndata, float sig[], int mwt, float *a,
	float *b, float *siga, float *sigb, float *chi2, float *q);

#endif /* __EXTRACT_PARAMS_H */


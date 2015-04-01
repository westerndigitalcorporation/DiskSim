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


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mems_internals.h"
#include "disksim_malloc.h"

void initialize_device(mems_t *dev,
		       double spring_factor,
		       double time_constants,
		       int hong) {

  mems_sled_t *sled;

  sled = &(dev->sled[0]);
  sled->dev = dev;

  sled->pos.x_pos = 0;
  sled->pos.y_pos = 0;
  sled->pos.y_vel = 0;
  
  sled->x_accel_nm_s2 = 746.2 * 1000000000.0; 
  sled->y_accel_nm_s2 = 746.2 * 1000000000.0; 
  sled->bit_length_nm = 50.0;
  sled->y_access_speed_bit_s = 200000;
  sled->x_length_nm = 2000.0 * 50.0;
  sled->y_length_nm = 2000.0 * 50.0;
  sled->spring_factor = spring_factor;

  sled->num_time_constants = time_constants;
  sled->sled_resonant_freq_hz = 739;

  sled->servo_burst_length_bits = 10;
  sled->tip_sector_length_bits = 80;
  sled->tip_sectors_per_lbn = 64;

  dev->precompute_seek_count = 0;
  dev->seek_function = hong;
  // mems_precompute_seek_curve(dev);
  
}

int main(int argc, char *argv[]) {

  mems_t dev;
  coord_t begin, end;

  double spring_factor = 0.0;
  double accel;
  double length_nm;
  double velocity_nm_s;

  double seek_time;
  double seek_time_zero = 0.0;

  int bit_step = 10;
  int i;
  int verbose = 0;
  int three_dee = 0;
  int mathematica = 0;
  int transpose = 0;
  int hong = 0;

  double time_constants = 0.0;

  int x_pos, y_pos;
  begin.x_pos = 1000;
  begin.y_pos = 1000;

  //start_nm = -1;
  //end_nm = -1;

  spring_factor = 0.0;

  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0)
      verbose = 1;
    if (strcmp(argv[i], "-spring") == 0)
      spring_factor = atof(argv[i+1]);
    if (strcmp(argv[i], "-step") == 0)
      bit_step = atoi(argv[i+1]);
    if (strcmp(argv[i], "-start") == 0)
      //start_nm = atof(argv[i+1]);
    if (strcmp(argv[i], "-end") == 0)
      //end_nm = atof(argv[i+1]);
    if (strcmp(argv[i], "-3d") == 0)
      three_dee = 1;
    if (strcmp(argv[i], "-num") == 0)
      time_constants = atof(argv[i+1]);
    if (strcmp(argv[i], "-x") == 0)
      begin.x_pos = atoi(argv[i+1]);
    if (strcmp(argv[i], "-y") == 0)
      begin.y_pos = atoi(argv[i+1]);
    if (strcmp(argv[i], "-math") == 0)
      mathematica = atoi(argv[i+1]);
    if (strcmp(argv[i], "-transpose") == 0)
      transpose = 1;
    if (strcmp(argv[i], "-hong") == 0)
      hong = 1;
  }
  
  accel = 746.2 * 1000000000.0;
  length_nm = 2000.0 * 50.0;
  //velocity_nm_s = 200000.0 * 50.0;
  velocity_nm_s = 0.0;

  dev.sled = malloc(sizeof(mems_sled_t));

  initialize_device(&dev,
		    spring_factor,
		    time_constants,
		    hong);

  begin.y_vel = 0;

  end.x_pos = 0;
  end.y_pos = 0;
  end.y_vel = 0;
  
  //fprintf(stdout, "spring_factor = %f\n", spring_factor);

  for(x_pos = -1000; x_pos <= 1000; x_pos += bit_step) {
    for(y_pos = -1000; y_pos <= 1000; y_pos += bit_step) {

  /*
  for(x_pos = 0; x_pos <= 1000; x_pos += bit_step) {
    for(y_pos = 0; y_pos <= 1000; y_pos += bit_step) {
  */  
      //x_pos = 1000;
      //y_pos = 500;
      
      if (!transpose) {
	end.x_pos = x_pos;
	end.y_pos = y_pos;
      } else {
	end.x_pos = y_pos;
	end.y_pos = x_pos;
      }
      
      if (verbose) fprintf(stdout, "main:  starting a new seek (monkey)\n");
      
      seek_time = mems_seek_time(&(dev.sled[0]),
				  &begin, &end,
				  NULL, NULL,
				  NULL, NULL);

      spring_factor = dev.sled[0].spring_factor;
      dev.sled[0].spring_factor = 0.0;
      
      seek_time_zero = mems_seek_time(&(dev.sled[0]),
				       &begin, &end,
				       NULL, NULL,
				       NULL, NULL);
      
      dev.sled[0].spring_factor = spring_factor;

      if (verbose) fprintf(stdout, "goose\n");
      if (mathematica == 1) {
	fprintf(stdout, "%f ", seek_time);
      } else if (mathematica == 2) {
	fprintf(stdout, "%f ", (seek_time - seek_time_zero));
      } else {
	if (!transpose) {
	  fprintf(stdout, "%d %d %f %f %f\n",
		  x_pos, y_pos, seek_time, seek_time_zero, (seek_time - seek_time_zero));
	} else {
	  fprintf(stdout, "%d %d %f %f %f\n",
		  y_pos, x_pos, seek_time, seek_time_zero, (seek_time - seek_time_zero));
	}	  

	/*
	fprintf(stdout, "%f %f %f %f\n",
		x_velocity_diff, x_time_diff, y_velocity_diff, y_time_diff);
	*/

      }
    }
    if (mathematica != 0) fprintf(stdout, "\n");
  }

  free(dev.sled);
  
  return 0;
}

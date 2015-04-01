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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// fetch basic stats from a disk

#include "dixtrac.h"
#include "scsi_all.h"
#include "extract_params.h"

// XXX yuck
extern char *scsi_buf;

void do_stats(void) 
{
  int lbns, bsize;
  int cyls, heads, rot, rot_off;

  char product[SID_PRODUCT_SIZE+1];
  char vendor[SID_VENDOR_SIZE+1];
  char serialnum[SVPD_SERIAL_NUM_SIZE+1];
  char revision[SID_REVISION_SIZE+1];

  char serno2[SVPD_SERIAL_NUM_SIZE+1];

  memset(product, 0, sizeof(product));
  memset(vendor, 0, sizeof(vendor));
  memset(revision, 0, sizeof(revision));
  memset(serialnum, 0, sizeof(serialnum));


  get_detailed_disk_label(product, vendor, revision, serialnum);

  printf("VENDOR    :  %s\n", vendor);
  printf("PRODUCT   :  %s\n", product);
  printf("REVISION  :  %s\n", revision);  
  printf("SERIALNUM :  %s\n", serialnum);

  {
      int i;
      struct scsi_vpd_unit_serial_number *svpd_serial;
      exec_scsi_command(scsi_buf, SCSI3_inq_command(SVPD_UNIT_SERIAL_NUMBER));
      svpd_serial = (struct scsi_vpd_unit_serial_number *)scsi_buf;
      assert(svpd_serial->length <= SVPD_SERIAL_NUM_SIZE);
      
      memcpy(serno2, svpd_serial->serial_num, svpd_serial->length);
      
      for (i = 0; i < SVPD_SERIAL_NUM_SIZE; i++) {
	  if(serno2[i] < ' ') {
	      serno2[i] = 0;
	  }
      }
      printf("VENDORSN  :  %s\n", serno2);
  }
  
  get_capacity(&lbns, &bsize);
  printf("LBNS      :  %d\n", lbns);
  printf("BLOCKSIZE :  %d\n", bsize);

  get_geometry(&cyls, &rot, &heads, &rot_off);
  printf("CYLS      :  %d\n", cyls);
  printf("HEADS     :  %d\n", heads);
  printf("ROT       :  %d\n", rot);
  printf("ROT OFF   :  %d\n", rot_off);



}

void 
usage(void)
{
  fprintf(stderr, "dx_stat -d <disk dev>\n");
}


int 
main(int argc, char **argv)
{
  static struct option opts[] = { 
    {0,0,0,0} 
  };
  int opts_idx;
  int c;

  char dev[1024];
  dev[0] = 0;
  
  while((c = getopt_long(argc, argv, "d:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }

  if(!*dev) { 
    usage();
    exit(1);
  }

  disk_init(dev, 1);


  setlinebuf(stdout);

  do_stats();


  exit(0);
  return 0;
}


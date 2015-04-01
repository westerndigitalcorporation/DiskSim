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

#include <libparam/libparam.h>
#include <diskmodel/dm.h>
#include <diskmodel/modules/modules.h>
#include <libddbg/libddbg.h>

#include "mechanics.h"
#include "extract_layout.h"
#include "dixtrac.h"

struct mech_params {
  double fullseek;
  double singleseek;
  double writesettle;
  double headswitch;
  int rpms;
  double rpmerr;
};


void
output_model(struct lp_block *d, 
	     struct mech_params *params,
	     FILE *fp,
	     char *seekfile_name)
{
  struct lp_block *mechblk;
  struct lp_param *mechparam;

  struct lp_param *p;
  

  mechblk = lp_new_block();
  mechblk->type = DM_MOD_MECH_G1;
  
  p = lp_new_param("Access time type", "", lp_new_stringv("trackSwitchPlusRotation"));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);

  p = lp_new_param("Seek type", "", lp_new_stringv("extracted"));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);

  p = lp_new_param("Full seek curve", "", lp_new_stringv(seekfile_name));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);

//   p = lp_new_param("Single cylinder seek time", "", 
// 		   lp_new_doublev(params->singleseek));
//   lp_add_param(&mechblk->params, &mechblk->params_len, p);
  
//   p = lp_new_param("Full strobe seek time", "", 
// 		   lp_new_doublev(params->fullseek));
//   lp_add_param(&mechblk->params, &mechblk->params_len, p);

  p = lp_new_param("Add. write settling delay", "", 
		   lp_new_doublev(params->writesettle));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);
  
  p = lp_new_param("Head switch time", "", 
		   lp_new_doublev(params->headswitch));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);  

  p = lp_new_param("Rotation speed (in rpms)", "", 
		   lp_new_intv(params->rpms));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);  

  p = lp_new_param("Percent error in rpms", "", 
		   lp_new_doublev(params->rpmerr));
  lp_add_param(&mechblk->params, &mechblk->params_len, p);  

  mechparam = lp_new_param("Mechanical Model", "", lp_new_blockv(mechblk));
  lp_add_param(&d->params, &d->params_len, mechparam);


  unparse_block(d, fp);
}


void 
seekout(void *ctx, int dist, float time)
{
  fprintf((FILE *)ctx, "%d, %f\n", dist, time);
}

// fp for the model, seekfd for the seeks
struct mech_params *
do_mech(FILE *fp)
{
  struct mech_params *params;
  //  float singlecyl, fullstrobe;
  int rot;
  //  int seeks_len;
  float error;
  double head_switch, write_settle;

  params = calloc(1, sizeof(struct mech_params));

  disable_caches();

  get_rotation_speed(&rot,&error);
  fprintf(stderr, "Rotation speed (in rpms) %d\n", rot);

  params->rpms = rot;
  params->rpmerr = error;

//   seeks_len = dx_seek_curve_len();
//   fprintf(seekfd, "Seek distances measured: %d\n", seeks_len);
//   get_seek_times(&singlecyl, &fullstrobe, 0, seekout, seekfd);
 
//   params->fullseek = fullstrobe;
//   params->singleseek = singlecyl;


  do {
    get_head_switch(&head_switch);
  } while(head_switch < 0.0);

  fprintf(stderr, "*** Head Switch  : %f \n", head_switch);
  params->headswitch = head_switch < 0.0 ? 0.0 : head_switch;

  get_write_settle(head_switch, &write_settle);
  fprintf(stderr, "*** Write Settle  : %f \n", write_settle);
  params->writesettle = write_settle < 0.0 ? 0.0 : write_settle;

  return params;
}


void 
usage(void)
{
  fprintf(stderr, "dx_mech -d <disk dev> -o <output file> -l <layout model> -s <seekfile>\n");
}

extern struct dm_disk_if *adisk;

int 
main(int argc, char **argv)
{
  static struct option opts[] = { {0,0,0,0} };
  int opts_idx;
  int c;
  FILE *outfile, *l;

  char l_name[1024];
  char outfile_name[1024];
  char seekfile_name[1024];
  char dev[1024];

  struct lp_tlt **tlts;
  int tlts_len;
  struct mech_params *params;

  outfile_name[0] = 0;
  seekfile_name[0] = 0;
  dev[0] = 0;
  l_name[0] = 0;
 
  
  while((c = getopt_long(argc, argv, "d:o:l:s:", opts, &opts_idx)) != -1) {
    switch(c) {
    case -1:
      break;
    case 'd':
      strncpy(dev, optarg, sizeof(dev));
      break;
    case 'o':
      strncpy(outfile_name, optarg, sizeof(outfile_name));
      break;
    case 's':
      strncpy(seekfile_name, optarg, sizeof(seekfile_name));
      break;
    case 'l':
      // layout model
      strncpy(l_name, optarg, sizeof(l_name));
      break;
    default:
      usage();
      exit(1);
      break;
    }
  }

  ddbg_assert_setfile(stderr);

  srand(time(0) % 71999);

  if(!*outfile_name || !*seekfile_name || !*dev || !*l_name) { 
    usage();
    exit(1);
  }

  // set up SCSI stuff
  disk_init(dev, 1);

  // set up output file
  if(!strcmp(outfile_name, "-")) {
    outfile = stdout;
  }
  else {
    outfile = fopen(outfile_name, "w");
    if(!outfile) {
      perror("fopen outfile");
      exit(1);
    }
  }


  setlinebuf(outfile);


  // load model
  l = fopen(l_name, "r");
  if(!l) {
    perror("fopen layout\n");
    exit(1);
  }

  for(c = 0; c <= DM_MAX_MODULE; c++) {
    lp_register_module(dm_mods[c]);
  }

  lp_init_typetbl();
  lp_loadfile(l, &tlts, &tlts_len, l_name, 0, 0);
  adisk = (struct dm_disk_if *)lp_instantiate("foo", 0);


  params = do_mech(outfile);

  output_model(tlts[0]->it.block, params, outfile, seekfile_name);

  exit(0);
  return 0;
}


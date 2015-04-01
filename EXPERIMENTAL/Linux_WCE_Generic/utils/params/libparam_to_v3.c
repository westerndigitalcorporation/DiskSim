
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


#include <libparam/libparam.h>

#include <modules/disksim_disk_param.h>
// #include <disksim/src/modules/modules.h>

//#include "modules/dm_disk_param.h"
//#include "modules/dm_layout_g1_param.h"
//#include "modules/dm_mech_g1_param.h"

#include <diskmodel/modules/dm_disk_param.h>
#include <diskmodel/modules/dm_layout_g1_param.h>
#include <diskmodel/modules/dm_mech_g1_param.h>

// #include <diskmodel/modules/modules.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libddbg/libddbg.h>



#define DISKSIM_drand48 drand48

int nums[4];

enum what {
  DISK,
  DMDISK,
  DMLAYOUT,
  DMMECH
};

char *oldspec;
char *newspec;
char *modelfile;


void fix_zonelist(struct lp_list *l) {
  int c;
  int zonetype;
  lp_lookup_type("dm_layout_g1_zone", &zonetype);
  
  for(c = 0; c < l->values_len; c++) {
    if(!l->values[c]) { continue; }

    l->values[c]->v.b->type = zonetype;
  }
}



int *noop(struct lp_block *junk1, int junk2) { return 0; }
int *loader(struct lp_block *b, int foo) {
  int c;
  char modelfileparam[80];
  char modelname[80];
  FILE *df, *mf;

  struct lp_block disk, dmdisk, layout, mech;
  struct lp_param layout_p, mech_p, modelfile_p;
  struct lp_value layout_v, mech_v, modelfile_v;

  bzero(&mech, sizeof(struct lp_block));
  bzero(&disk, sizeof(struct lp_block));
  bzero(&dmdisk, sizeof(struct lp_block));
  bzero(&layout, sizeof(struct lp_block));

  bzero(&layout_p, sizeof(struct lp_param));
  bzero(&mech_p, sizeof(struct lp_param));
  bzero(&modelfile_p, sizeof(struct lp_param));

  sprintf(modelfileparam, "source %s", modelfile);

  modelfile_p.name = strdup("Model");
  modelfile_p.v = &modelfile_v;
  modelfile_p.v->v.s = modelfileparam;
  modelfile_p.v->t = S;


  lp_add_param(&disk.params, &disk.params_len, &modelfile_p);

  for(c = 0; c < b->params_len; c++) {
    if(!b->params[c]) {
      continue;
    }

    // top-level DM params
    if(lp_param_name(nums[DMDISK], b->params[c]->name) != -1) {
      lp_add_param(&dmdisk.params, &dmdisk.params_len, b->params[c]);
    }
    // DM layout params
    else if(lp_param_name(nums[DMLAYOUT], b->params[c]->name) != -1) {
      if(!strcmp(b->params[c]->name, "Zones")) {
	fix_zonelist(LVAL(b->params[c]));
      }
      lp_add_param(&layout.params, &layout.params_len, b->params[c]);
    }
    // DM mech params
    else if(lp_param_name(nums[DMMECH], b->params[c]->name) != -1) {
      lp_add_param(&mech.params, &mech.params_len, b->params[c]);
    }
    // must be a disksim_disk param
    else if(lp_param_name(nums[DISK], b->params[c]->name) != -1) {
      if(!strcmp(b->params[c]->name, "Scheduler")) {
	lp_lookup_type("disksim_ioqueue", &b->params[c]->v->v.b->type);
      }
      lp_add_param(&disk.params, &disk.params_len, b->params[c]);
    }
  }

  lp_lookup_type("disksim_disk", &disk.type);


  layout_p.name = strdup("Layout Model");
  layout_p.v = &layout_v;
  layout_p.v->v.b = &layout;
  layout_p.v->t = BLOCK;


  mech_p.name = strdup("Mechanical Model");
  mech_p.v = &mech_v;
  mech_p.v->v.b = &mech;
  mech_p.v->t = BLOCK; 


  dmdisk.name = strdup(b->name);
  disk.name = strdup(b->name);
/*    disk.type = nums[DISK]; */
  sprintf(modelname, "%s_model", b->name);
  dmdisk.name = modelname;
  dmdisk.type = nums[DMDISK];
  mech.type = nums[DMMECH]; 
  layout.type = nums[DMLAYOUT];

  lp_add_param(&dmdisk.params, &dmdisk.params_len, &layout_p);
  lp_add_param(&dmdisk.params, &dmdisk.params_len, &mech_p);



  ddbg_assert(df = fopen(newspec, "w"));
  ddbg_assert(mf = fopen(modelfile, "w"));


  unparse_block(&disk, df);
  printf("\n\n\n");
  unparse_block(&dmdisk, mf);

  return 0;
}


int main(int argc, char **argv) {

  FILE *inputfile;
  // this is the pre3-28 disk
  struct lp_mod disk = { "disk", 
			 disksim_disk_params, 
			 DISKSIM_DISK_MAX, 
			 loader, 
			 0, 
			 0};

  // this is the v3 disksim disk
  struct lp_mod dsdisk = { "disksim_disk", 
			   disksim_disk_params, 
			   DISKSIM_DISK_MAX, 
			   loader, 
			   0, 
			   0 };

  // disksim pre3 ioqueue
  struct lp_mod ioqueue = { "ioqueue", 0, 0, noop, 0, 0, 0, 0 };
  // disksim v3 ioqueue
  struct lp_mod dsioqueue = { "disksim_ioqueue", 0, 0, noop, 0, 0, 0, 0 };
  // disksim pre3 zone
  struct lp_mod zone = { "zone", 0, 0, noop, 0, 0, 0, 0 };
  // diskmodel zone
  struct lp_mod dmzone = { "dm_layout_g1_zone", 0, 0, noop, 0, 0, 0, 0 };
  struct lp_mod dm_disk = dm_disk_mod;
  struct lp_mod dm_layout_g1 = dm_layout_g1_mod;
  struct lp_mod dm_mech_g1 = dm_mech_g1_mod;

  if(argc != 5) {
    fprintf(stderr, "usage: convert_diskspec <oldspec> <diskname> <newspec> <model>\n");
    exit(1);
  }

  oldspec = argv[1];
  newspec = argv[3];
  modelfile = argv[4];

  ddbg_assert_setfile(stderr);

  inputfile = fopen(oldspec, "r");

  nums[DISK] = lp_register_module(&disk);
  lp_register_module(&dsdisk);
  lp_register_module(&ioqueue);
  lp_register_module(&dsioqueue);
  lp_register_module(&zone);
  lp_register_module(&dmzone);

  nums[DMDISK] = lp_register_module(&dm_disk);
  nums[DMLAYOUT] = lp_register_module(&dm_layout_g1);
  nums[DMMECH] = lp_register_module(&dm_mech_g1);

  lp_init_typetbl();
  lp_loadfile(inputfile, 
	      0,  // tlts
	      0,  // tlts_len
	      oldspec, // source file
	      0, // overrides
	      0); // overrides_len

  lp_instantiate(argv[2], argv[2]);

  return 0;
}


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




#include "config.h"
#include "disksim_global.h"
#include "disksim_iodriver.h"
#include "disksim_disk.h"

#include "modules/modules.h"

#include <libparam/libparam.h>

#include <diskmodel/modules/modules.h>
//#include <memsmodel/modules/modules.h>
/*SSD:*/
//#include <ssdmodel/modules/modules.h>

#include <stdio.h>



static void disksim_topoloader(struct lp_topospec *ts, int len) {
  int rv = load_iodriver_topo(ts, len);
  ddbg_assert2(rv != 0, "Topospec load failed!");
}


int disksim_loadparams(char *inputfile, int synthgen) {
  int rv;
  int c;
  struct lp_tlt **tlts = NULL;
  int tlts_len = 0;

  // register modules with libparam
  for(c = 0; c <= DISKSIM_MAX_MODULE; c++) {
    lp_register_module(disksim_mods[c]);
  }

  // diskmodel modules
  for(c = 0; c <= DM_MAX_MODULE; c++) {
    lp_register_module(dm_mods[c]);
  }

  // memsmodel modules
//  for(c = 0; c <= MEMSMODEL_MAX_MODULE; c++) {
//    lp_register_module(memsmodel_mods[c]);
//  }

  // ssdmodel modules
//  for(c = 0; c <= SSDMODEL_MAX_MODULE; c++) {
//    lp_register_module(ssdmodel_mods[c]);
//  }

  lp_register_topoloader(disksim_topoloader);

  //  lp_init_typetbl();


  disksim->parfile = fopen(inputfile,"r");
  ddbg_assert2(disksim->parfile != NULL, 
	     ("Parameter file \"%s\" cannot be opened for read access\n", 
	      inputfile));

  lp_init_typetbl();

  rv = lp_loadfile(disksim->parfile, 
		   &tlts, 
		   &tlts_len, 
		   inputfile,
		   disksim->overrides,
		   disksim->overrides_len);

  lp_unparse_tlts(tlts, tlts_len, outputfile, inputfile);

  lp_instantiate("Global", "Global");
  lp_instantiate("Stats", "Stats");

  // instantiate any logorgs, syncsets we find
  for(c = 0; c < lp_typetbl_len; c++) {
    if(lp_typetbl[c] != 0 && (lp_typetbl[c]->spec != 0)) {
      if(!strcmp(lp_lookup_base_type(lp_typetbl[c]->sub, 0), "disksim_logorg")) {
	iodriver_load_logorg(lp_typetbl[c]->spec);
      }

      else if(!strcmp(lp_lookup_base_type(lp_typetbl[c]->sub, 0), "disksim_syncset")) {
	disk_load_syncsets(lp_typetbl[c]->spec);
      }
    }
  }

  // do this *after* logorgs get instantiated!
  if(synthgen) {
    lp_instantiate("Proc", "Proc");
    lp_instantiate("Synthio", "Synthio");
  }


  fclose(disksim->parfile);
  return rv;
}

// End of file


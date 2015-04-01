/* diskmodel (version 1.1)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2003-2005
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
 */

#include <diskmodel/dm.h>
#include <diskmodel/modules/modules.h>

#include <libparam/libparam.h>
#include <libddbg/libddbg.h>

#include <stdio.h>



extern doTests(struct dm_disk_if *, int, char **);
extern void testsUsage(void);
extern int minargs;


int main(int argc, char **argv) 
{
  int c;
  FILE *modelfile;
  struct dm_disk_if *disk;
  char *modelname;
  
  ddbg_assert_setfile(stderr);

  if(argc < minargs) {
    usage();
    exit(1);
  }

  modelfile = fopen(argv[1], "r");
  if(!modelfile) {
    fprintf(stderr, "*** error: failed to open \"%s\"\n", argv[1]);
  }

  for(c = 0; c <= DM_MAX_MODULE; c++) {
    struct lp_mod *mod;

    if(c == DM_MOD_DISK) {
      mod = dm_mods[c];
    }
    else {
      mod = dm_mods[c];
    }

    lp_register_module(mod);
  }

  lp_init_typetbl();
  lp_loadfile(modelfile, 0, 0, argv[1], 0, 0);
  fclose(modelfile);
  
  modelname = argc >= 3 ? argv[2] : 0;
  
  disk = lp_instantiate("foo", modelname);
  printf("*** got a dm_disk with %d sectors!\n", disk->dm_sectors);

  
  doTests(disk, argc, argv);



  exit(0);
  // NOTREACHED
  return 0;
}

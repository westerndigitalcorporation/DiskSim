
#include <libparam/libparam.h>
#include <stdio.h>
#include <string.h>

// disksim's 
#include <modules/modules.h>
#include <diskmodel/modules/modules.h>

// merge_blocks <f1> <path> <f2> <outfile>

// load f1

// Replace value of <path> in the first block in <f1> with the first
// block in f2.

// output the result to <outfile>

// modifies path
struct lp_block *
do_subst(struct lp_block *b, 
	 struct lp_block *newthing, 
	 char *destpath,
	 char *srcpath)
{
  int i;

  if(srcpath) {
    // recurse to find src block
    char *rest = strchr(srcpath, ':');

    if(rest) {
      // split the string at the delimiting character
      *rest = 0;
      rest++;
    }

    for(i = 0; i < newthing->params_len; i++) {
      if(newthing->params[i]) {
	if(!strcmp(srcpath, newthing->params[i]->name)) {
	  return do_subst(b, newthing->params[i]->v->v.b, destpath, 0);
	}
      }
    }
    fprintf(stderr, "*** error: couldn't find %s in %s\n", 
	    srcpath,
	    newthing->name);
    exit(1);
  }


  if(!destpath) {
    newthing->name = 0;
    return newthing;
  }
  else {
    char *rest = strchr(destpath, ':');

    if(rest) {
      // split the string at the delimiting character
      *rest = 0;
      rest++;
    }

    for(i = 0; i < b->params_len; i++) {
      if(b->params[i]) {
	if(!strcmp(destpath, b->params[i]->name)) {
	  b->params[i]->v->t = BLOCK;
	  b->params[i]->v->v.b = newthing;
	  return b;
	}
      }
    }
    fprintf(stderr, "*** error: couldn't find %s in %s\n", 
	    destpath,
	    b->name);
    exit(1);
  }
}


int main(int argc, char **argv) 
{
  int i;
  char *destpath, *srcpath;
  FILE *infile = 0, *newblockfile = 0, *outfile = 0;

  struct lp_tlt **tlts1, **tlts2;
  int tlts1_len, tlts2_len;
  struct lp_block *result;

  if(argc != 6) {
    fprintf(stderr, "usage: merge_blocks <f1> <path> <f2> <path> <outfile>\n");
    exit(1);
  }

  for(i = 0; i <= DM_MAX_MODULE; i++) 
    lp_register_module(dm_mods[i]);

  for(i = 0; i <= DISKSIM_MAX_MODULE; i++) 
    lp_register_module(disksim_mods[i]);

  lp_init_typetbl();

  destpath = argv[2];
  srcpath = argv[4];

  infile = fopen(argv[1], "r");
  newblockfile = fopen(argv[3], "r");
  outfile = fopen(argv[5], "w");

  lp_loadfile(infile, &tlts1, &tlts1_len, argv[1], 0, 0);
  lp_loadfile(newblockfile, &tlts2, &tlts2_len, argv[3], 0, 0);


  result = do_subst(tlts1[0]->it.block, tlts2[0]->it.block, 
		    destpath, srcpath);

  unparse_block(result, outfile);


  return 0;
}

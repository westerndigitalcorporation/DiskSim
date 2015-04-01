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

#include <libddbg/libddbg.h>
#include <libparam/libparam.h>
#include <diskmodel/dm.h>
#include <disksim/modules/modules.h>
#include <diskmodel/modules/modules.h>

#include "cache.h"
#include "dixtrac.h"
#include "mtbrc.h"
#include "extract_params.h"
#include "extract_layout.h"


// XXX
extern struct dm_disk_if *adisk;
extern diskinfo_t *thedisk;

// void write_entry(FILE *fp, char *name, char *value);


#define PRINT_WARNING

void tests(FILE *fp, char *modelfile, char *full_name) {
  float bulk_transfer_time;
  float low_wm,high_wm;
  int set_by_reqsize;
  float rc = 0.0, 
    wc = 0.0,
    rmaw = 0.0,
    wmaw = 0.0,
    wmar = 0.0,
    rmar = 0.0,
    rhar = 0.0,
    rhaw = 0.0,
    whaw = 0.0,
    whar = 0.0;
  int numsegs,i, segsize;

  struct lp_param *p;
  struct lp_value *value;

  struct lp_block *diskblock = lp_new_block();
  diskblock->type = lp_mod_name("disksim_disk");
  diskblock->name = full_name;

#ifdef MEASURE_TIME
  elapsed_time();
#endif

  // beginning of overheads

  // printf("%s() (%s:%d): ", __func__, __FILE__, __LINE__);
  printf("Measuring controller overheads...");
  fflush(stdout);
  
  value = lp_new_stringv("foo");
  p = lp_new_param("Model", "", value);
  p->v->source_file = modelfile;
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* if set to zero, other overheads are use as a more precise value      */
  /* set it to zero, and determine more precise overheads                 */
  value = lp_new_doublev(0.0);
  p = lp_new_param("Per-request overhead time", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* use all parameters at face value */
  value = lp_new_doublev(1.0);
  p = lp_new_param("Time scale for overheads", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  bulk_transfer_time = get_bulk_transfer_time();
  // usecs ??
  value = lp_new_doublev(bulk_transfer_time);
  p = lp_new_param("Bulk sector transfer time", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Hold bus entire read xfer", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Hold bus entire write xfer", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Allow almost read hits", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Allow sneaky full read hits", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Allow sneaky partial read hits", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Allow sneaky intermediate read hits", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(cache_share_rw_segment());
  p = lp_new_param("Allow read hits on write data", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Allow write prebuffering", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Preseeking level", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   /* !!! */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Never disconnect", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   value = lp_new_intv(1);
   PRINT_WARNING;
   p = lp_new_param("Print stats", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);
   
   value = lp_new_intv(thedisk->sectpercyl);
   p = lp_new_param("Avg sectors per cylinder", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);
   
   value = lp_new_intv(8);
   PRINT_WARNING;
   p = lp_new_param("Max queue length", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);
   
   // end of overheads
   printf("done.\n");

   // ioqueue
   // printf("%s() (%s:%d): ",__func__, __FILE__, __LINE__);
   printf("Measuring scheduling parameters...");
   fflush(stdout);

   {
       struct lp_block *ioqblk = lp_new_block();
       struct lp_param *ioqpar = 
	   lp_new_param("Scheduler", "", lp_new_blockv(ioqblk));
       ioqblk->type = lp_mod_name("disksim_ioqueue");

       lp_add_param(&diskblock->params, &diskblock->params_len, ioqpar);

       value = lp_new_intv(8);
       PRINT_WARNING;
       p = lp_new_param("Scheduling policy", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);
     
       value = lp_new_intv(4);
       PRINT_WARNING;
       p = lp_new_param("Cylinder mapping strategy", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_doublev(0.0);
       PRINT_WARNING;
       p = lp_new_param("Write initiation delay", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_doublev(0.0);
       PRINT_WARNING;
       p = lp_new_param("Read initiation delay", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Sequential stream scheme", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Maximum concat size", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Overlapping request scheme", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Sequential stream diff maximum", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Scheduling timeout scheme", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Timeout time/weight", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Timeout scheduling", "", value);;
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Scheduling priority scheme", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);

       value = lp_new_intv(0);
       PRINT_WARNING;
       p = lp_new_param("Priority scheduling", "", value);
       lp_add_param(&ioqblk->params, &ioqblk->params_len, p);
       
   }
   // end of ioqueue
   printf("done.\n");

   // cache
   // printf("%s() (%s:%d) :",__func__, __FILE__, __LINE__); 
   printf("Measuring cache parameters...\n");
   fflush(stdout);

   enable_caches();

   numsegs = disksim_count_all_segments();
   i = cd_count_write_segments();
   
   i = (i < 1 ? 1 : i);
   /* make sure there are as many segments in total as there are write segments */
   /* important for disks that use adaptive multi-segmented caches */
   numsegs = (numsegs < i ? i : numsegs); 
   
   value = lp_new_intv(numsegs);
   p = lp_new_param("Number of buffer segments", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

   value = lp_new_intv(i);
   p = lp_new_param("Maximum number of write segments", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);
   
  segsize = cd_segment_size();
  value = lp_new_intv(segsize);
  p = lp_new_param("Segment size (in blks)", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(!cd_share_rw_segment());
  p = lp_new_param("Use separate write segment", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  get_water_marks(&low_wm,&high_wm,&set_by_reqsize);

  value = lp_new_doublev(low_wm);
  p = lp_new_param("Low (write) water mark", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(high_wm);
  p = lp_new_param("High (read) water mark", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(set_by_reqsize);
  p = lp_new_param("Set watermark by reqsize", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);


  value = lp_new_intv(1);
  PRINT_WARNING;
  p = lp_new_param("Calc sector by sector", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);



#ifdef DISKSIM_PARAMS  
  // value = lp_new_intv(1);
  // p = lp_new_param("Calc sector by sector", "", value);
  // lp_add_param(&diskblock->params, &diskblock->params_len, p);
#endif

  value = lp_new_intv(disksim_caching_in_buffer());
  p = lp_new_param("Enable caching in buffer", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(disksim_continuous_read());
  p = lp_new_param("Buffer continuous read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* value = lp_new_intv(cache_min_read_ahead()); */
   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Minimum read-ahead (blks)", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);

  i = cache_max_read_ahead();
  value = lp_new_intv((i > segsize ? segsize : i));
  p = lp_new_param("Maximum read-ahead (blks)", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(cd_discard_req_after_read());
  p = lp_new_param("Read-ahead over requested", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

   value = lp_new_intv(0);
   PRINT_WARNING;
   p = lp_new_param("Read-ahead on idle hit", "", value);
   lp_add_param(&diskblock->params, &diskblock->params_len, p);
 
  value = lp_new_intv(cd_read_free_blocks());
  p = lp_new_param("Read any free blocks", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(disksim_fast_write());
  p = lp_new_param("Fast write level", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(cd_read_on_arrival());
  p = lp_new_param("Immediate buffer read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(cd_write_on_arrival());
  p = lp_new_param("Immediate buffer write", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(cd_contig_writes_appended());
  p = lp_new_param("Combine seq writes", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

#ifdef MEASURE_TIME
  printf("Generation of cache parameters: %.3f seconds\n",elapsed_time());
#endif

  disable_caches();

  /* detailed cache specs. - 5 entries */
  value = lp_new_intv(0);
  PRINT_WARNING;
  p = lp_new_param("Stop prefetch in sector", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(0);
  PRINT_WARNING;
  p = lp_new_param("Disconnect write if seek", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(1);
  PRINT_WARNING;
  p = lp_new_param("Write hit stop prefetch", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(1);
  PRINT_WARNING;
  p = lp_new_param("Read directly to buffer", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(1);
  PRINT_WARNING;
  p = lp_new_param("Immed transfer partial hit", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* end of detailed cache specs. */

#ifdef MEASURE_TIME
  elapsed_time();
#endif

  // printf("%s() (%s:%d): ",__func__, __FILE__, __LINE__);
  printf("Measuring cache overheads...");
  fflush(stdout);

  find_completion_times(&rc,&wc,&rmaw,&wmaw,&wmar,&rmar,&rhar,&rhaw,
			&whaw,&whar);

  // cache overheads
  value = lp_new_doublev(rhar);
  p = lp_new_param("Read hit over. after read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(rhaw);
  p = lp_new_param("Read hit over. after write", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(rmar);
  p = lp_new_param("Read miss over. after read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(rmaw);
  p = lp_new_param("Read miss over. after write", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(whar);
  p = lp_new_param("Write hit over. after read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(whaw);
  p = lp_new_param("Write hit over. after write", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(wmar);
  p = lp_new_param("Write miss over. after read", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(wmaw);
  p = lp_new_param("Write miss over. after write", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  // end cache params
  printf("done.\n");

#ifdef MEASURE_TIME
  printf("Generation -after overheads: %.3f seconds\n",elapsed_time());
#endif

  // more overheads

  // printf("%s() (%s:%d): ", __func__, __FILE__, __LINE__);
  // printf("Measuring completion overheads...\n");

  // #ifdef DISKSIM_PARAMS  
  /* the next 15 parameters used in "high precision disk simulation" */
  value = lp_new_doublev(rc);
  p = lp_new_param("Read completion overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(wc);
  p = lp_new_param("Write completion overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("Data preparation overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("First reselect overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("Other reselect overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("Read disconnect afterread", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("Read disconnect afterwrite", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  PRINT_WARNING;
  p = lp_new_param("Write disconnect overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_intv(0);
  p = lp_new_param("Extra write disconnect", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* if above parameter is one, the next five values are meaningful */
  value = lp_new_doublev(0.0);
  p = lp_new_param("Extradisc command overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  p = lp_new_param("Extradisc disconnect overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  p = lp_new_param("Extradisc inter-disconnect delay", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  p = lp_new_param("Extradisc 2nd disconnect overhead", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  value = lp_new_doublev(0.0);
  p = lp_new_param("Extradisc seek delta", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);
  /* end of params pertaining to 'Extra write disconnect' == TRUE */

  value = lp_new_doublev(0.0);
  p = lp_new_param("Minimum seek delay", "", value);
  lp_add_param(&diskblock->params, &diskblock->params_len, p);

  /* end of params for "high precision disk simulation"           */
  // #endif

  unparse_block(diskblock, fp);
}


void usage(void) {
    fprintf(stderr, "usage: ./dx_rest -d dev -o outfile -m model --full-name <name>\n");
}

int 
main(int argc, char **argv)
{
    int c;

    static struct option opts[] = { 
	{"full-name", 1, 0, 0},
	{0,0,0,0} };
    int opts_idx;

    char dev[1024];
    char m_name[1024];
    char outfile_name[1024];
    char *full_name;
    FILE *outfile, *model;

    struct lp_tlt **tlts;
    int tlts_len;

    dev[0] = 0;
    m_name[0] = 0;
    outfile_name[0] = 0;

    setlinebuf(stdout);

    while((c = getopt_long(argc, argv, "d:o:m:", opts, &opts_idx)) != -1) {
	switch(c) {
	case -1:
	    break;
	case 0:
	    if(opts_idx == 0) {
		full_name = strdup(optarg);
	    }
	    else {
		usage();
		exit(1);
	    }
	    break;
	case 'd':
	    strncpy(dev, optarg, sizeof(dev));
	    break;
	case 'o':
	    strncpy(outfile_name, optarg, sizeof(outfile_name));
	    break;
	case 'm':
	    // model file
	    strncpy(m_name, optarg, sizeof(m_name));
	    break;
	default:
	    usage();
	    exit(1);
	    break;
	}
    }

    srand(time(0) % 71999);

    if(!*dev || !*outfile_name || !*m_name || !full_name) {
	usage();
	exit(1);
    }

    ddbg_assert_setfile(stdout);

    disk_init(dev, 1);

    if(!strcmp(outfile_name, "-")) {
	outfile = stdout;
    }
    else {
	outfile = fopen(outfile_name, "w");
	if(!outfile) {
	    perror("fopen() outfile");
	    exit(1);
	}
    }
    setlinebuf(outfile);


    for(c = 0; c <= DM_MAX_MODULE; c++) {
	lp_register_module(dm_mods[c]);
    }

    for(c = 0; c <= DISKSIM_MAX_MODULE; c++) {
	lp_register_module(disksim_mods[c]);
    }

    lp_init_typetbl();

    model = fopen(m_name, "r");
    if(!model) {
	perror("fopen model\n");
	exit(1);
    }

    lp_loadfile(model, &tlts, &tlts_len, m_name, 0, 0);

    adisk = (struct dm_disk_if *)lp_instantiate("foo", 0);
    thedisk = calloc(1, sizeof(*thedisk));

    tests(outfile, m_name, full_name);

    return 0;
}




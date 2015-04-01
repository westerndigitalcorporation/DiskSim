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

#include "dixtrac.h"
#include "read_diskspec.h"
#include "extract_layout.h"

#define MAXDISKS  10
#define TRUE      1
#define FALSE     0

static disk* disks = NULL;
static int numdisks = 1;

static FILE *outputfile;

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
diskinfo_t * 
read_layout_file(char *modelname)
{
   FILE *fp;
   char filename[280] = "";

   /* redirect output from disk_read_specs */
   outputfile = fopen("/dev/null", "w");
   assert(outputfile != NULL);

   sprintf (filename, "%s.%s", modelname,LAYOUT_FULL_EXT);
   fp = fopen (filename, "r");
   if (!fp)
     return(NULL);
   dxtrc_disk_read_specs(fp);
   fclose(fp);
   fclose(outputfile);
   return((diskinfo_t *) &disks[0]);
}


void dxtrc_getparam_int(FILE *parfile,
                  char *parname,
                  int *parptr,
                  int parchecks,
                  int parminval,
                  int parmaxval)
{
   char line[201];

   sprintf(line, "%s: %s", parname, "%d");
   if (fscanf(parfile, line, parptr) != 1) {
      fprintf(stderr, "Error reading '%s'\n", parname);
      exit(0);
   }
   fgets(line, 200, parfile); /* this allows comments, kinda ugly hack */
   if (((parchecks & 1) && (*parptr < parminval)) || ((parchecks & 2) && (*parptr > parmaxval))) {
      fprintf(stderr, "Invalid value for '%s': %d\n", parname, *parptr);
      exit(0);
   }
   fprintf (outputfile, "%s: %d\n", parname, *parptr);
}


void dxtrc_getparam_double(FILE *parfile,
                     char *parname,
                     double *parptr,
                     int parchecks,
                     double parminval,
                     double parmaxval)
{
   char line[201];

   sprintf(line, "%s: %s", parname, "%lf");
   if (fscanf(parfile, line, parptr) != 1) {
      fprintf(stderr, "Error reading '%s'\n", parname);
      assert(0);
   }
   fgets(line, 200, parfile);
   if (((parchecks & 1) && (*parptr < parminval)) || ((parchecks & 2) && (*parptr > parmaxval)) || ((parchecks & 4) && (*parptr <= parminval))) {
      fprintf(stderr, "Invalid value for '%s': %f\n", parname, *parptr);
      assert(0);
   }
   fprintf (outputfile, "%s: %f\n", parname, *parptr);
}


void dxtrc_getparam_bool(FILE *parfile,
                   char *parname,
                   int *parptr)
{
   char line[201];

   sprintf(line, "%s %s", parname, "%d");
   if (fscanf(parfile, line, parptr) != 1) {
      fprintf(stderr, "Error reading '%s'\n", parname);
      exit(0);
   }
   fgets(line, 200, parfile);
   if ((*parptr != TRUE) && (*parptr != FALSE)) {
      fprintf(stderr, "Invalid value for '%s': %d\n", parname, *parptr);
      exit(0);
   }
   fprintf (outputfile, "%s %d\n", parname, *parptr);
}

void dxtrc_bandcopy(band *destbands, band *srcbands, int numbands)
{
   int i, j;

   for (i=0; i<numbands; i++) {
      destbands[i].startcyl = srcbands[i].startcyl;
      destbands[i].endcyl = srcbands[i].endcyl;
      destbands[i].firstblkno = srcbands[i].firstblkno;
      destbands[i].blkspertrack = srcbands[i].blkspertrack;
      destbands[i].deadspace = srcbands[i].deadspace;
      destbands[i].trackskew = srcbands[i].trackskew;
      destbands[i].cylskew = srcbands[i].cylskew;
      destbands[i].blksinband = srcbands[i].blksinband;
      destbands[i].sparecnt = srcbands[i].sparecnt;
      destbands[i].numslips = srcbands[i].numslips;
      destbands[i].numdefects = srcbands[i].numdefects;
      for (j=0; j<srcbands[i].numslips; j++) {
         destbands[i].slip[j] = srcbands[i].slip[j];
      }
      for (j=0; j<srcbands[i].numdefects; j++) {
         destbands[i].defect[j] = srcbands[i].defect[j];
         destbands[i].remap[j] = srcbands[i].remap[j];
      }
   }
}

#define getparam_int     dxtrc_getparam_int
#define getparam_bool    dxtrc_getparam_bool
#define getparam_double  dxtrc_getparam_double

void dxtrc_band_read_specs(disk *currdisk, int numbands, FILE *parfile,
			   int numsurfaces, int sparescheme)
{
   band *bands = currdisk->bands;
   int i;
   int bandno;
   int bandno_expected;
   int startcyl;
   int endcyl;
   int blkspertrack;
   int deadspace;
   int sparecnt;
   int numslips;
   int numdefects;
   int defect;
   int remap;

   for (bandno_expected=1; bandno_expected<=numbands; bandno_expected++) {
      if (fscanf(parfile, "Band #%d\n", &bandno) != 1) {
	 fprintf(stderr, "Error reading band number\n");
	 exit(0);
      }
      if (bandno != bandno_expected) {
	 fprintf(stderr, "Invalid value for band number - %d\n", bandno);
	 exit(0);
      }
      fprintf (outputfile, "Band #%d\n", bandno);
      bandno--;

      getparam_int(parfile, "First cylinder number", &startcyl, 1, 0, 0);
      bands[bandno].startcyl = startcyl;

      getparam_int(parfile, "Last cylinder number", &endcyl, 1, 0, 0);
      bands[bandno].endcyl = endcyl;

      getparam_int(parfile, "Blocks per track", &blkspertrack, 1, 1, 0);
      bands[bandno].blkspertrack = blkspertrack;

      getparam_double(parfile, "Offset of first block", &bands[bandno].firstblkno, 1, (double) 0.0, (double) 0.0);

      getparam_int(parfile, "Empty space at zone front", &deadspace, 1, 0, 0);
      bands[bandno].deadspace = deadspace;

      getparam_double(parfile, "Skew for track switch", &bands[bandno].trackskew, 1, (double) -2.0, (double) 0.0);
      getparam_double(parfile, "Skew for cylinder switch", &bands[bandno].cylskew, 1, (double) -2.0, (double) 0.0);

      getparam_int(parfile, "Number of spares", &sparecnt, 1, 0, 0);
      bands[bandno].sparecnt = sparecnt;

      getparam_int(parfile, "Number of slips", &numslips, 3, 0, MAXSLIPS);
      bands[bandno].numslips = numslips;
      for (i=0; i<numslips; i++) {
         getparam_int(parfile, "Slip", &bands[bandno].slip[i], 1, 0, 0);
      }

      getparam_int(parfile, "Number of defects", &numdefects, 3, 0, MAXDEFECTS);
      bands[bandno].numdefects = numdefects;
      for (i=0; i<numdefects; i++) {
         if (fscanf(parfile, "Defect: %d %d\n", &defect, &remap) != 2) {
	    fprintf(stderr, "Error reading defect #%d\n", i);
	    exit(0);
         }
         if ((defect < 0) || (remap < 0)) {
	    fprintf(stderr, "Invalid value(s) for defect - %d %d\n", defect, remap);
	    exit(0);
         }
         bands[bandno].defect[i] = defect;
         bands[bandno].remap[i] = remap;
         fprintf (outputfile, "Defect: %d %d\n", defect, remap);
      }

      /* fprintf (outputfile, "blksinband %d, sparecnt %d, blkspertrack %d, numtracks %d\n", bands[bandno].blksinband, sparecnt, blkspertrack, ((endcyl-startcyl+1)*numsurfaces)); */
   }
}



void dxtrc_disk_read_specs(FILE *parfile)
{
  /* char line[201]; */
   FILE *specfile = parfile;
   /* char brandname[81]; */
   int i;
   int copies;
   /*
   int specno;
   int specno_expected = 1;
   */
   int diskno = 0;
   /*
   double acctime;
   double seektime;
   char seekinfo[81];
   int extractseekcnt = 0;
   int *extractseekdists = NULL;
   double *extractseektimes = NULL;
   double seekavg;
   */
   int numbands;
   int mapping;
   /*
   int enablecache;
   int contread;
   double hp1, hp2, hp3, hp4, hp5, hp6;
   double first10seeks[10];
   */
   disk *currdisk;
   /*
   int useded;

   getparam_int(parfile, "\nNumber of disks", &numdisks, 3, 0, MAXDISKS);
   */
   numdisks = 1; /* schindjr */

   if (!numdisks) {
      return;
   }

   disks = (disk*) malloc(numdisks * (sizeof(disk)));
   assert (disks != NULL);

   while (diskno < numdisks) {
      currdisk = &disks[diskno];
      /*
      if (fscanf(parfile, "\nDisk Spec #%d\n", &specno) != 1) {
	 fprintf(stderr, "Error reading disk spec number\n");
	 exit(0);
      }
      if (specno != specno_expected) {
	 fprintf(stderr, "Unexpected value for disk spec number: specno %d, expected %d\n", specno, specno_expected);
	 exit(0);
      }
      fprintf (outputfile, "\nDisk Spec #%d\n", specno);
      specno_expected++;
      
      getparam_int(parfile, "# disks with Spec", &copies, 1, 0, 0);
      if (copies == 0) {
	 copies = numdisks - diskno;
      }
      */
      copies = 1;  /* schindjr */
      /*
      if ((diskno+copies) > numdisks) {
	 fprintf(stderr, "Too many disk specifications provided\n");
	 exit(0);
      }

      if (fgets(line, 200, parfile) == NULL) {
	 fprintf(stderr, "End of file before disk specifications\n");
	 exit(0);
      }
      specfile = parfile;
      if (sscanf(line, "Disk brand name: %s\n", brandname) == 1) {
	 specfile = disk_locate_spec_in_specfile(parfile, brandname, line);
         if (fgets(line, 200, specfile) == NULL) {
            fprintf(stderr, "End of file before disk specifications\n");
            exit(0);
         }
      }

      if (sscanf(line, "Access time (in msecs): %lf\n", &acctime) != 1) {
         fprintf(stderr, "Error reading access time\n");
         exit(0);
      }
      if ((acctime < 0.0) && (!diskspecialaccesstime(acctime))) {
	 fprintf(stderr, "Invalid value for access time - %f\n", acctime);
	 exit(0);
      }
      fprintf (outputfile, "Access time (in msecs): %f\n", acctime);

      getparam_double(specfile, "Seek time (in msecs)", &seektime, 0, (double) 0.0, (double) 0.0);
      if ((seektime < 0.0) && (!diskspecialseektime(seektime))) {
         fprintf(stderr, "Invalid value for seek time - %f\n", seektime);
         exit(0);
      }
      
      getparam_double(specfile, "Single cylinder seek time", &currdisk->seekone, 1, (double) 0.0, (double) 0.0);

      if (fscanf(specfile, "Average seek time: %s", seekinfo) != 1) {
	 fprintf(stderr, "Error reading average seek time\n");
	 exit(0);
      }
      fgets(line, 200, specfile);
      if (seektime == EXTRACTION_SEEK) {
	 seekavg = 0.0;
         disk_read_extracted_seek_curve(seekinfo, &extractseekcnt, &extractseekdists, &extractseektimes);
      } else {
	 if (sscanf(seekinfo, "%lf", &seekavg) != 1) {
	    fprintf(stderr, "Invalid value for average seek time: %s\n", seekinfo);
	    exit(0);
	 }
      }
      if (seekavg < 0.0) {
         fprintf(stderr, "Invalid value for average seek time - %f\n", seekavg);
         exit(0);
      }
      fprintf (outputfile, "Average seek time: %s\n", seekinfo);

     
      getparam_double(specfile, "Full strobe seek time", &currdisk->seekfull, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Add. write settling delay", &currdisk->seekwritedelta, 0, (double) 0.0, (double) 0.0);

      if (fscanf(specfile, "HPL seek equation values: %lf %lf %lf %lf %lf %lf", &hp1, &hp2, &hp3, &hp4, &hp5, &hp6) != 6) {
	 fprintf(stderr, "Error reading HPL seek equation values\n");
	 exit(0);
      }
      fgets(line, 200, specfile);
      if ((hp1 < 0) | (hp2 < 0) | (hp3 < 0) | (hp4 < 0) | (hp5 < 0) | (hp6 < -1)) {
         fprintf(stderr, "Invalid value for an HPL seek equation values - %f %f %f %f %f %f\n", hp1, hp2, hp3, hp4, hp5, hp6);
         exit(0);
      }
      fprintf (outputfile, "HPL seek equation values: %f %f %f %f %f %f\n", hp1, hp2, hp3, hp4, hp5, hp6);
      if (hp6 != -1) {
	 currdisk->seekone = hp6;
      }

      if ((j = fscanf(specfile, "First 10 seek times: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", &first10seeks[0], &first10seeks[1], &first10seeks[2], &first10seeks[3], &first10seeks[4], &first10seeks[5], &first10seeks[6], &first10seeks[7], &first10seeks[8], &first10seeks[9])) == 0) {
	 fprintf(stderr, "Error reading first 10 seek times\n");
	 exit(0);
      }
      fgets(line, 200, specfile);
      if (seektime == FIRST10_PLUS_HPL_SEEK_EQUATION) {
         if ((j < 10) |(first10seeks[0] < 0) | (first10seeks[1] < 0) | (first10seeks[2] < 0) | (first10seeks[3] < 0) | (first10seeks[4] < 0) | (first10seeks[5] < 0) | (first10seeks[6] < 0) | (first10seeks[7] < 0) | (first10seeks[8] < 0) | (first10seeks[9] < 0)) {
            fprintf(stderr, "Invalid value for a First 10 seek times: %d - %f %f %f %f %f %f %f %f %f %f\n", j, first10seeks[0], first10seeks[1], first10seeks[2], first10seeks[3], first10seeks[4], first10seeks[5], first10seeks[6], first10seeks[7], first10seeks[8], first10seeks[9]);
            exit(0);
         }
         fprintf (outputfile, "First 10 seek times:    %f %f %f %f %f %f %f %f %f %f\n", first10seeks[0], first10seeks[1], first10seeks[2], first10seeks[3], first10seeks[4], first10seeks[5], first10seeks[6], first10seeks[7], first10seeks[8], first10seeks[9]);
      } else {
         fprintf (outputfile, "First 10 seek times:    %f\n", first10seeks[0]);
      }

      getparam_double(specfile, "Head switch time", &currdisk->headswitch, 1, (double) 0.0, (double) 0.0);

      getparam_double(specfile, "Rotation speed (in rpms)", &currdisk->rpm, 4, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Percent error in rpms", &currdisk->rpmerr, 3, (double) 0.0, (double) 100.0);
      */

      getparam_int(specfile, "Number of data surfaces", &currdisk->numsurfaces, 1, 1, 0);
      getparam_int(specfile, "Number of cylinders", &currdisk->numcyls, 1, 1, 0);
      getparam_int(specfile, "Blocks per disk", &currdisk->numblocks, 1, 1, 0);

      /*
      getparam_double(specfile, "Per-request overhead time", &currdisk->overhead, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Time scale for overheads", &currdisk->timescale, 1, (double) 0.0, (double) 0.0);

      getparam_double(specfile, "Bulk sector transfer time", &currdisk->blktranstime, 1, (double) 0.0, (double) 0.0);
      getparam_bool(specfile, "Hold bus entire read xfer:", &currdisk->hold_bus_for_whole_read_xfer);
      getparam_bool(specfile, "Hold bus entire write xfer:", &currdisk->hold_bus_for_whole_write_xfer);

      getparam_bool(specfile, "Allow almost read hits:", &currdisk->almostreadhits);
      getparam_bool(specfile, "Allow sneaky full read hits:", &currdisk->sneakyfullreadhits);
      getparam_bool(specfile, "Allow sneaky partial read hits:", &currdisk->sneakypartialreadhits);
      getparam_bool(specfile, "Allow sneaky intermediate read hits:", &currdisk->sneakyintermediatereadhits);
      getparam_bool(specfile, "Allow read hits on write data:", &currdisk->readhitsonwritedata);
      getparam_bool(specfile, "Allow write prebuffering:", &currdisk->writeprebuffering);
      getparam_int(specfile, "Preseeking level", &currdisk->preseeking, 3, 0, 2);
      getparam_bool(specfile, "Never disconnect:", &currdisk->neverdisconnect);
      getparam_bool(specfile, "Print stats for disk:", &currdisk->printstats);
      */

      getparam_int(specfile, "Avg sectors per cylinder", &currdisk->sectpercyl, 3, 1, currdisk->numblocks);

      /*
      getparam_int(specfile, "Max queue length at disk", &currdisk->maxqlen, 1, 0, 0);

      currdisk->queue = ioqueue_readparams(specfile, disk_printqueuestats, disk_printcritstats, disk_printidlestats, disk_printintarrstats, disk_printsizestats);

      getparam_int(specfile, "Number of buffer segments", &currdisk->numsegs, 1, 1, 0);
      getparam_int(specfile, "Maximum number of write segments", &currdisk->numwritesegs, 3, 1, currdisk->numsegs);
      getparam_int(specfile, "Segment size (in blks)", &currdisk->segsize, 3, 1, currdisk->numblocks);
      getparam_bool(specfile, "Use separate write segment:", &useded);
      currdisk->dedicatedwriteseg = (segment *)useded;

      getparam_double(specfile, "Low (write) water mark", &currdisk->writewater, 3, (double) 0.0, (double) 1.0);
      getparam_double(specfile, "High (read) water mark", &currdisk->readwater, 3, (double) 0.0, (double) 1.0);
      getparam_bool(specfile, "Set watermark by reqsize:", &currdisk->reqwater);

      getparam_bool(specfile, "Calc sector by sector:", &currdisk->sectbysect);

      getparam_int(specfile, "Enable caching in buffer", &enablecache, 3, 0, 2);
      getparam_int(specfile, "Buffer continuous read", &contread, 3, 0, 4);

      getparam_int(specfile, "Minimum read-ahead (blks)", &currdisk->minreadahead, 3, 0, currdisk->segsize);

      getparam_int(specfile, "Maximum read-ahead (blks)", &currdisk->maxreadahead, 3, 0, currdisk->segsize);

      getparam_int(specfile, "Read-ahead over requested", &currdisk->keeprequestdata, 3, -1, 1);

      getparam_bool(specfile, "Read-ahead on idle hit:", &currdisk->readaheadifidle);
      getparam_bool(specfile, "Read any free blocks:", &currdisk->readanyfreeblocks);

      getparam_int(specfile, "Fast write level", &currdisk->fastwrites, 3, 0, 2);

      getparam_int(specfile, "Immediate buffer read", &currdisk->immedread, 3, 0, 2);
      getparam_int(specfile, "Immediate buffer write", &currdisk->immedwrite, 3, 0, 2);
      getparam_bool(specfile, "Combine seq writes:", &currdisk->writecomb);
      getparam_bool(specfile, "Stop prefetch in sector:", &currdisk->stopinsector);
      getparam_bool(specfile, "Disconnect write if seek:", &currdisk->disconnectinseek);
      getparam_bool(specfile, "Write hit stop prefetch:", &currdisk->write_hit_stop_readahead);
      getparam_bool(specfile, "Read directly to buffer:", &currdisk->read_direct_to_buffer);
      getparam_bool(specfile, "Immed transfer partial hit:", &currdisk->immedtrans_any_readhit);

      getparam_double(specfile, "Read hit over. after read", &currdisk->overhead_command_readhit_afterread, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read hit over. after write", &currdisk->overhead_command_readhit_afterwrite, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read miss over. after read", &currdisk->overhead_command_readmiss_afterread, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read miss over. after write", &currdisk->overhead_command_readmiss_afterwrite, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Write over. after read", &currdisk->overhead_command_write_afterread, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Write over. after write", &currdisk->overhead_command_write_afterwrite, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read completion overhead", &currdisk->overhead_complete_read, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Write completion overhead", &currdisk->overhead_complete_write, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Data preparation overhead", &currdisk->overhead_data_prep, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "First reselect overhead", &currdisk->overhead_reselect_first, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Other reselect overhead", &currdisk->overhead_reselect_other, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read disconnect afterread", &currdisk->overhead_disconnect_read_afterread, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Read disconnect afterwrite", &currdisk->overhead_disconnect_read_afterwrite, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Write disconnect overhead", &currdisk->overhead_disconnect_write, 1, (double) 0.0, (double) 0.0);

      getparam_bool(specfile, "Extra write disconnect:", &currdisk->extra_write_disconnect);
      getparam_double(specfile, "Extradisc command overhead", &currdisk->extradisc_command, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Extradisc disconnect overhead", &currdisk->extradisc_disconnect1, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Extradisc inter-disconnect delay", &currdisk->extradisc_inter_disconnect, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Extradisc 2nd disconnect overhead", &currdisk->extradisc_disconnect2, 1, (double) 0.0, (double) 0.0);
      getparam_double(specfile, "Extradisc seek delta", &currdisk->extradisc_seekdelta, 1, (double) 0.0, (double) 0.0);

      getparam_double(specfile, "Minimum seek delay", &currdisk->minimum_seek_delay, 1, (double) 0.0, (double) 0.0);

      */
      getparam_int(specfile, "LBN-to-PBN mapping scheme", &mapping, 3, 0, LAYOUT_MAX);
      getparam_int(specfile, "Sparing scheme used", &currdisk->sparescheme, 3, NO_SPARING, MAXSPARESCHEME);
      getparam_int(specfile, "Rangesize for sparing", &currdisk->rangesize, 1, 1, 0);
      getparam_int(specfile, "Number of bands", &numbands, 1, 1, 0);

      for (i=diskno; i<(diskno+copies); i++) {
	/*
         disks[i].acctime = acctime;
         disks[i].seektime = seektime;
         disks[i].seekone = currdisk->seekone;
         disks[i].seekavg = seekavg;
         disks[i].seekfull = currdisk->seekfull;
	 disks[i].seekwritedelta = currdisk->seekwritedelta;
         disks[i].hpseek[0] = hp1;
         disks[i].hpseek[1] = hp2;
         disks[i].hpseek[2] = hp3;
         disks[i].hpseek[3] = hp4;
         disks[i].hpseek[4] = hp5;
         disks[i].hpseek[5] = hp6;
	 for (j=0; j<10; j++) {
	    disks[i].first10seeks[j] = first10seeks[j];
	 }
	 disks[i].extractseekcnt = extractseekcnt;
	 disks[i].extractseekdists = extractseekdists;
	 disks[i].extractseektimes = extractseektimes;
         disks[i].headswitch = currdisk->headswitch;
         disks[i].rpm = currdisk->rpm;
         disks[i].rpmerr = currdisk->rpmerr;
	 */
         disks[i].numsurfaces = currdisk->numsurfaces;
         disks[i].numcyls = currdisk->numcyls;
         disks[i].numblocks = currdisk->numblocks;
	 /* disks[i].printstats = currdisk->printstats; */
         disks[i].sectpercyl = currdisk->sectpercyl;
	 disks[i].devno = i;
         /* 
	 disks[i].maxqlen = currdisk->maxqlen; 
         disks[i].write_hit_stop_readahead = currdisk->write_hit_stop_readahead;
         disks[i].read_direct_to_buffer = currdisk->read_direct_to_buffer;
         disks[i].immedtrans_any_readhit = currdisk->immedtrans_any_readhit;
         disks[i].extra_write_disconnect = currdisk->extra_write_disconnect;
         disks[i].overhead_command_readhit_afterread = currdisk->overhead_command_readhit_afterread;
         disks[i].overhead_command_readhit_afterwrite = currdisk->overhead_command_readhit_afterwrite;
         disks[i].overhead_command_readmiss_afterread = currdisk->overhead_command_readmiss_afterread;
         disks[i].overhead_command_readmiss_afterwrite = currdisk->overhead_command_readmiss_afterwrite;
         disks[i].overhead_command_write_afterread = currdisk->overhead_command_write_afterread;
         disks[i].overhead_command_write_afterwrite = currdisk->overhead_command_write_afterwrite;
         disks[i].overhead_complete_read = currdisk->overhead_complete_read;
         disks[i].overhead_complete_write = currdisk->overhead_complete_write;
         disks[i].overhead_data_prep = currdisk->overhead_data_prep;
         disks[i].overhead_reselect_first = currdisk->overhead_reselect_first;
         disks[i].overhead_reselect_other = currdisk->overhead_reselect_other;
         disks[i].overhead_disconnect_read_afterread = currdisk->overhead_disconnect_read_afterread;
         disks[i].overhead_disconnect_read_afterwrite = currdisk->overhead_disconnect_read_afterwrite;
         disks[i].overhead_disconnect_write = currdisk->overhead_disconnect_write;
         disks[i].extradisc_command = currdisk->extradisc_command;
         disks[i].extradisc_disconnect1 = currdisk->extradisc_disconnect1;
         disks[i].extradisc_inter_disconnect = currdisk->extradisc_inter_disconnect;
         disks[i].extradisc_disconnect2 = currdisk->extradisc_disconnect2;
         disks[i].extradisc_seekdelta = currdisk->extradisc_seekdelta;
         disks[i].minimum_seek_delay = currdisk->minimum_seek_delay;
         disks[i].readanyfreeblocks = currdisk->readanyfreeblocks;
         disks[i].overhead = currdisk->overhead;
         disks[i].timescale = currdisk->timescale;
         disks[i].blktranstime = currdisk->blktranstime;
         disks[i].hold_bus_for_whole_read_xfer = currdisk->hold_bus_for_whole_read_xfer;
         disks[i].hold_bus_for_whole_write_xfer = currdisk->hold_bus_for_whole_write_xfer;
         disks[i].almostreadhits = currdisk->almostreadhits;
         disks[i].sneakyfullreadhits = currdisk->sneakyfullreadhits;
         disks[i].sneakypartialreadhits = currdisk->sneakypartialreadhits;
         disks[i].sneakyintermediatereadhits = currdisk->sneakyintermediatereadhits;
         disks[i].readhitsonwritedata = currdisk->readhitsonwritedata;
         disks[i].writeprebuffering = currdisk->writeprebuffering;
         disks[i].preseeking = currdisk->preseeking;
         disks[i].neverdisconnect = currdisk->neverdisconnect;
	 disks[i].numsegs = currdisk->numsegs;
	 disks[i].numwritesegs = currdisk->numwritesegs;
	 disks[i].segsize = currdisk->segsize;
	 disks[i].dedicatedwriteseg = currdisk->dedicatedwriteseg;
	 disks[i].writewater = currdisk->writewater;
	 disks[i].readwater = currdisk->readwater;
	 disks[i].reqwater = currdisk->reqwater;
	 disks[i].sectbysect = currdisk->sectbysect;
	 disks[i].enablecache = enablecache;
	 disks[i].contread = contread;
	 disks[i].minreadahead = currdisk->minreadahead;
	 disks[i].maxreadahead = currdisk->maxreadahead;
	 disks[i].keeprequestdata = currdisk->keeprequestdata;
	 disks[i].readaheadifidle = currdisk->readaheadifidle;
	 disks[i].fastwrites = currdisk->fastwrites;
	 disks[i].immedread = currdisk->immedread;
	 disks[i].immedwrite = currdisk->immedwrite;
	 disks[i].writecomb = currdisk->writecomb;
	 disks[i].stopinsector = currdisk->stopinsector;
	 disks[i].disconnectinseek = currdisk->disconnectinseek;
	 disks[i].seglist = NULL;
         disks[i].buswait = NULL;
	 disks[i].currenthda = NULL;
	 disks[i].effectivehda = NULL;
	 disks[i].currentbus = NULL;
	 disks[i].effectivebus = NULL;
	 disks[i].pendxfer = NULL;
	 disks[i].outwait = NULL;
	 */
	 disks[i].mapping = mapping;
         disks[i].sparescheme = currdisk->sparescheme;
	 disks[i].rangesize = currdisk->rangesize;
         disks[i].numbands = numbands;
	 /*
	 if (i != diskno) {
	    disks[i].queue = ioqueue_copy(currdisk->queue);
	 }
	 */
	 /* band info is different in "our" disk structure - schindjr */
         /* disks[i].bands = (band *) malloc(numbands*(sizeof(band))); 
	 assert(disks[i].bands != NULL);
	 */
	 if (i == diskno) {
            dxtrc_band_read_specs(currdisk, numbands, specfile, currdisk->numsurfaces, currdisk->sparescheme);
	    
         } else {
	   printf ("Bandcopy occured!\n");
	    dxtrc_bandcopy(disks[i].bands, currdisk->bands, numbands);
	 }
      }
      diskno += copies;
   }
   //   diskmap_initialize (&disks[0]);
}

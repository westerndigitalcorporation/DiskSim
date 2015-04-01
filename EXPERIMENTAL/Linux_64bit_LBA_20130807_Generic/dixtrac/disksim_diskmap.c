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

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */

/* this define delimits the code  that has been added to this file */
/* for the purporses of this package. Do not delete. */

#define PARAM_EXTRACT

#ifdef PARAM_EXTRACT

#include "dixtrac.h"
#include "disksim_diskmap.h"
#include "extract_layout.h"
typedef band_t band;
typedef diskinfo_t disk;
#define TRUE 1
#define FALSE 0
int remapsector = FALSE;
#define max(a,b) (((a)>(b))?(a):(b))
int bandstart;

#else

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_disk.h"
#include "disksim_disk.h"


int disk_get_numcyls (int diskno)
{
   disk *currdisk = getdisk (diskno);
   return(currdisk->numcyls);
}


void disk_get_mapping (int maptype, int diskno, int blkno, int *cylptr, int *surfaceptr, int *blkptr)
{
   disk *currdisk = getdisk (diskno);
   /* The last zero is temporary, schedulers must be able to specify */
   (void) disk_translate_lbn_to_pbn(currdisk, blkno, maptype, cylptr, surfaceptr, blkptr);
}


/* Documentation claims this is not used for any calculations... */
int disk_get_number_of_blocks (int diskno)
{
   disk *currdisk = getdisk (diskno);
   return(currdisk->numblocks);
}


int disk_get_avg_sectpercyl (int diskno)
{
   disk *currdisk = getdisk (diskno);
   return(currdisk->sectpercyl);
}


/* Checks physical blocks only, remember num logical blocks does not
   necessarily equal num physical blocks */
void disk_check_numblocks(disk *currdisk)
{
   int numblocks = 0;
   int i;

   for (i=0; i < currdisk->numbands; i++) {
      numblocks += currdisk->bands[i].blksinband;
   }
   if (numblocks != currdisk->numblocks) {
      fprintf (outputfile, "Numblocks provided by user does not match specifications - user %d, actual %d\n", currdisk->numblocks, numblocks);
   }
   currdisk->numblocks = numblocks;
}

#endif /* ! PARAM_EXTRACT */


int disk_compute_blksinband (disk *currdisk, band *currband)
{
   int blksinband;
   int cylcnt = currband->endcyl - currband->startcyl + 1;
   
   if (currdisk->mapping == LAYOUT_RAWFILE) {
     return(currband->blksinband);
   }

   blksinband = cylcnt * currband->blkspertrack * currdisk->numsurfaces;
   blksinband -= currband->deadspace;

   /* GROK -- everything like this should be in disksim_diskmap.c !! */
   if (currdisk->sparescheme == TRACK_SPARING) {
      blksinband -= currband->sparecnt * currband->blkspertrack;
   } else if (currdisk->sparescheme == SECTPERTRACK_SPARING) {
      blksinband -= currband->sparecnt * cylcnt * currdisk->numsurfaces;
   } else if (currdisk->sparescheme == SECTATEND_SPARING) {
      blksinband -= currband->numslips;
   } else if (currdisk->sparescheme == SECTSPERZONE_SPARING) {
      blksinband -= currband->sparecnt;
   } else if (currdisk->sparescheme == SECTSPERZONE_SPARING_SLIPTOEND) {
      blksinband -= currband->sparecnt + currband->numslips;
   } else if ((issectpercyl(currdisk)) && (!(issliptoend(currdisk)))) {
      blksinband -= currband->sparecnt * cylcnt;
   } else if ((issectpercyl(currdisk)) && (issliptoend(currdisk))) {
      blksinband -= (currband->sparecnt * cylcnt) + currband->numslips;
   } else if (currdisk->sparescheme == SECTPERRANGE_SPARING) {
      blksinband -= currband->sparecnt * (cylcnt / currdisk->rangesize);
   } else {
      fprintf (stderr, "Unknown sparing scheme at disk_compute_blksinband: %d\n", currdisk->sparescheme);
      exit (-1);
   }

   return (blksinband);
}


void diskmap_initialize (disk *currdisk)
{
   int i;

   if ((currdisk->mapping < 0) || (currdisk->mapping > LAYOUT_MAX)) {
      fprintf(stderr, "Invalid value for mapping in disk_param_override: %d\n", currdisk->mapping);
      exit(0);
   }

   for (i=0; i<currdisk->numbands; i++) {
      band *currband = &currdisk->bands[i];

      if ((currdisk->sparescheme == TRACK_SPARING) &&
          ((currband->numslips + currband->numdefects) > currband->sparecnt)) {
         fprintf(stderr, "Defects and slips outnumber the available spares: %d < %d + %d\n", currband->sparecnt, currband->numdefects, currband->numslips);
         exit(0);
      }

      currband->blksinband = disk_compute_blksinband (currdisk, currband);
   }
}


static int diskmap_surfno_on_cylinder (disk *currdisk, band *currband, int cylno, int surfaceno)
{
   if (currdisk->mapping == LAYOUT_CYLSWITCHONSURF1) {
      if ((cylno - currband->startcyl) % 2) {
         //printf ("reversed surfaceno %d (to %d) on cylinder %d\n", surfaceno, (currdisk->numsurfaces-surfaceno-1), cylno);
         return (currdisk->numsurfaces - surfaceno - 1);
      }
   } else if (currdisk->mapping == LAYOUT_CYLSWITCHONSURF2) {
      if (cylno % 2) {
         //printf ("reversed surfaceno %d (to %d) on cylinder %d\n", surfaceno, (currdisk->numsurfaces-surfaceno-1), cylno);
         return (currdisk->numsurfaces - surfaceno - 1);
      }
   }

   //printf ("did not reverse surfaceno %d on cylno %d\n", surfaceno, cylno);
   return (surfaceno);
}


double disk_map_pbn_skew(disk *currdisk, band *currband, int cylno, int surfaceno)
{
   double skew = (double) currband->firstblkno;
   int trackswitches;
   int slipoffs = 0;

   switch (currdisk->mapping) {
   case LAYOUT_NORMAL:
   case LAYOUT_CYLSWITCHONSURF1:
   case LAYOUT_CYLSWITCHONSURF2:
      surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
      cylno -= currband->startcyl;
      trackswitches = surfaceno + (currdisk->numsurfaces - 1) * cylno;
      skew += currband->trackskew * (double) trackswitches;
      skew += currband->cylskew * (double) cylno;
      skew /= currdisk->rotatetime;
      /* The following assumes that slips also push skew forward in some schemes */
      if ((issectpercyl(currdisk)) || (currdisk->sparescheme == SECTPERTRACK_SPARING)) {
         int tracks = cylno * currdisk->numsurfaces;
         int cutoff;
         int i;
 
         if (currdisk->sparescheme == SECTPERTRACK_SPARING) {
            tracks += surfaceno;
         }
         cutoff = tracks * currband->blkspertrack;
         for (i=0; i<currband->numslips; i++) {
            if (currband->slip[i] < cutoff) {
               slipoffs++;
            }
         }
      }
      skew += (double) slipoffs / (double) currband->blkspertrack;
      break;
   case LAYOUT_RAWFILE:
      skew = 0;
      break;
   case LAYOUT_SERPENTINE:
   default:
      fprintf(stderr, "Unknown mapping (%d) at disk_map_pbn_skew()\n",
	      currdisk->mapping);
      exit (0);
   }
   return(skew);
}


/****************************************************************************/
/*                                                                          */
/* The next several functions compute the lbn stored in a physical media    */
/*   sector <cyl, surf, blk>, for different sparing/mapping schemes.  The    */
/*   returned value is a proper lbn, or -2 if the sector is a remapped      */
/*   defect, or -1 if the sector is a slipped defect or an unused spare.    */
/*                                                                          */
/* Note: the per-scheme functions take a pointer to the relevant band       */
/*   and the first lbn in that band, in addition to the basic parameters.   */
/*                                                                          */
/****************************************************************************/

int disk_pbn_to_lbn_sectpertrackspare(disk *currdisk, band *currband,
				      int cylno, int surfaceno,
				      int blkno, int lbn)
{
   int i;
   int lbnspercyl;
   int firstblkoncyl;

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   firstblkoncyl = (cylno - currband->startcyl) * currdisk->numsurfaces * currband->blkspertrack;
   blkno += firstblkoncyl + (surfaceno * currband->blkspertrack);
   for (i=(currband->numdefects-1); i>=0; i--) {
      if (blkno == currband->defect[i]) {        /* Remapped bad block */
         return(-2);
      }
      if (blkno == currband->remap[i]) {
         remapsector = TRUE;
         blkno = currband->defect[i];
         break;
      }
   }
   for (i=(currband->numslips-1); i>=0; i--) {
      if (blkno == currband->slip[i]) {          /* Slipped bad block */
         return(-1);
      }
      if ((blkno > currband->slip[i]) && ((currband->slip[i] / (currband->blkspertrack * currdisk->numsurfaces)) == (cylno - currband->startcyl)) && ((currband->slip[i] % (currband->blkspertrack * currdisk->numsurfaces)) == surfaceno)) {
         blkno--;
      }
   }

   if ((blkno % currband->blkspertrack) >= (currband->blkspertrack - currband->sparecnt)) {   /* Unused spare block */
      return(-1);
   }

   /* recompute in case sector was remapped */
   cylno = blkno / (currband->blkspertrack * currdisk->numsurfaces);
   surfaceno = blkno % (currband->blkspertrack * currdisk->numsurfaces);
   surfaceno = surfaceno / currband->blkspertrack;
   blkno = blkno % currband->blkspertrack;

   lbnspercyl = (currband->blkspertrack - currband->sparecnt) * currdisk->numsurfaces;
   lbn += cylno * lbnspercyl;
   lbn += surfaceno * (currband->blkspertrack - currband->sparecnt);
   lbn += blkno - currband->deadspace;
   lbn = (lbn < 0) ? -1 : lbn;

   return(lbn);
}


int disk_pbn_to_lbn_sectpercylspare(disk *currdisk, band *currband,
				    int cylno, int surfaceno,
				    int blkno, int lbn)
{
   int i;
   int lbnspercyl;
   int blkspercyl;
   int firstblkoncyl;

//printf ("pbn_to_lbn_sectpercylspare: cylno %d, surfaceno %d, blkno %d, lbn %d\n", cylno, surfaceno, blkno, lbn);

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkspercyl = currband->blkspertrack * currdisk->numsurfaces;
   lbnspercyl = blkspercyl - currband->sparecnt;
   firstblkoncyl = (cylno - currband->startcyl) * blkspercyl;
   blkno += firstblkoncyl + (surfaceno * currband->blkspertrack);

   for (i=(currband->numdefects-1); i>=0; i--) {
      if (blkno == currband->defect[i]) {        /* Remapped bad block */
         return(-2);
      }
      if (blkno == currband->remap[i]) {
         remapsector = TRUE;
         blkno = currband->defect[i];
         break;
      }
   }

   for (i=(currband->numslips-1); i>=0; i--) {
      if (blkno == currband->slip[i]) {          /* Slipped bad block */
         return(-1);
      }
      if (blkno > currband->slip[i]) {
         if ((currband->slip[i] / blkspercyl) == (cylno - currband->startcyl)) {
            blkno--;
         } else if (issliptoend(currdisk)) {
            lbn--;
         }
      }
   }

   /* check for unused spare blocks */
   if (((!isspareatfront(currdisk)) && ((blkno % blkspercyl) >= lbnspercyl)) ||
       ((isspareatfront(currdisk)) && ((blkno % blkspercyl) < currband->sparecnt))) {
      return(-1);
   }

   /* recompute in case sector was remapped */
   cylno = blkno / blkspercyl;
   surfaceno = blkno % blkspercyl;
   surfaceno = surfaceno / currband->blkspertrack;
   blkno = blkno % currband->blkspertrack;

   lbn += cylno * lbnspercyl;
   lbn += surfaceno * currband->blkspertrack;
   lbn += blkno - currband->deadspace;
   lbn -= (isspareatfront(currdisk)) ? currband->sparecnt : 0;
   lbn = (lbn < 0) ? -1 : lbn;

//printf ("pbn_to_lbn_sectpercylspare: result %d\n", lbn);

   return(lbn);
}


int disk_pbn_to_lbn_sectperrangespare(disk *currdisk, band *currband,
				    int cylno, int surfaceno,
				    int blkno, int lbn)
{
   int i;
   int lbnsperrange;
   int blksperrange;
   int blkspercyl;
   int firstblkoncyl;
   int rangeno;

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkspercyl = currband->blkspertrack * currdisk->numsurfaces;
   blksperrange = blkspercyl * currdisk->rangesize;
   lbnsperrange = blksperrange - currband->sparecnt;
   rangeno = (cylno - currband->startcyl) / currdisk->rangesize;

   firstblkoncyl = (cylno - currband->startcyl) * blkspercyl;
   blkno += firstblkoncyl + (surfaceno * currband->blkspertrack);

   for (i=(currband->numdefects-1); i>=0; i--) {
      if (blkno == currband->defect[i]) {        /* Remapped bad block */
         return(-2);
      }
      if (blkno == currband->remap[i]) {
         remapsector = TRUE;
         blkno = currband->defect[i];
         break;
      }
   }

   for (i=(currband->numslips-1); i>=0; i--) {
      if (blkno == currband->slip[i]) {          /* Slipped bad block */
         return(-1);
      }
      if (blkno > currband->slip[i]) {
         if ((currband->slip[i] / blksperrange) == rangeno) {
            blkno--;
         }
      }
   }

   blkno = blkno % blksperrange;

   /* check for unused spare blocks */
   if (blkno >= lbnsperrange) {
      return(-1);
   }

   lbn += rangeno * lbnsperrange;
   lbn += blkno - currband->deadspace;
   lbn = (lbn < 0) ? -1 : lbn;

   return(lbn);
}


int disk_pbn_to_lbn_sectperzonespare(disk *currdisk, band *currband,
				      int cylno, int surfaceno,
				      int blkno, int lbn)
{
   int i;
   int blkspercyl;
   int firstblkoncyl;

//printf ("pbn_to_lbn_sectperzonespare: cylno %d, surfaceno %d, blkno %d, lbn %d\n", cylno, surfaceno, blkno, lbn);

   /* compute blkno within zone */
   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkspercyl = currband->blkspertrack * currdisk->numsurfaces;
   firstblkoncyl = (cylno - currband->startcyl) * blkspercyl;
   blkno += firstblkoncyl + (surfaceno * currband->blkspertrack);

   for (i=(currband->numdefects-1); i>=0; i--) {
      if (blkno == currband->defect[i]) {        /* Remapped bad block */
         return(-2);
      }
      if (blkno == currband->remap[i]) {
         remapsector = TRUE;
         blkno = currband->defect[i];
         break;
      }
   }

   for (i=(currband->numslips-1); i>=0; i--) {
      if (blkno == currband->slip[i]) {          /* Slipped bad block */
         return(-1);
      }
      if (blkno > currband->slip[i]) {
         blkno--;
      }
   }

   /* check for unused spare blocks */
   if (blkno >= currband->blksinband) {
      return(-1);
   }

   lbn += blkno - currband->deadspace;
   lbn = (lbn < 0) ? -1 : lbn;

//printf ("pbn_to_lbn_sectperzonespare: result %d\n", lbn);

   return(lbn);
}


int disk_pbn_to_lbn_trackspare(disk *currdisk, band *currband,
			       int cylno, int surfaceno, int blkno, int lbn)
{
   int i;
   int trackno;
   int lasttrack;

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   trackno = (cylno - currband->startcyl) * currdisk->numsurfaces + surfaceno;
   for (i=(currband->numdefects-1); i>=0; i--) {
      if (trackno == currband->defect[i]) {   /* Remapped bad track */
	 return(-2);
      }
      if (trackno == currband->remap[i]) {
	 trackno = currband->defect[i];
	 break;
      }
   }
   for (i=(currband->numslips-1); i>=0; i--) {
      if (trackno == currband->slip[i]) {     /* Slipped bad track */
	 return(-1);
      }
      if (trackno > currband->slip[i]) {
	 trackno--;
      }
   }
   lasttrack = (currband->blksinband + currband->deadspace) / currband->blkspertrack;
   if (trackno > lasttrack) {                 /* Unused spare track */
      return(-1);
   }
   lbn += blkno + (trackno * currband->blkspertrack) - currband->deadspace;
   lbn = (lbn < 0) ? -1 : lbn;
   return(lbn);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
#ifdef RAWLAYOUT_SUPPORT
int 
disk_pbn_to_lbn_rawlayout(disk *currdisk, band *currband, int cylno, 
			  int surfaceno, int blkno)
{
  if ((currdisk->rlcyls[cylno].surface[surfaceno].firstlbn + blkno) >
      currdisk->rlcyls[cylno].surface[surfaceno].lastlbn) {
    return(-1);
  }
  else {
#ifdef DETAILED_RAW_LAYOUT
    int i,nomapped,lbn;
    lbn = currdisk->rlcyls[cylno].surface[surfaceno].firstlbn;
    nomapped = currdisk->rlcyls[cylno].surface[surfaceno].sect[0];

    for(i = 0; i < currdisk->rlcyls[cylno].surface[surfaceno].valid; i++) {
      if (blkno <= (currdisk->rlcyls[cylno].surface[surfaceno].seqcnt[i] +
		    currdisk->rlcyls[cylno].surface[surfaceno].sect[i] + 1)) {
	break;
      }
      else {
	nomapped += currdisk->rlcyls[cylno].surface[surfaceno].sect[i] +
	  currdisk->rlcyls[cylno].surface[surfaceno].seqcnt[i] + 1;
      }
    }
    return(lbn + nomapped + blkno - 
	   currdisk->rlcyls[cylno].surface[surfaceno].sect[i]);
#else
    return(currdisk->rlcyls[cylno].surface[surfaceno].firstlbn + blkno);
#endif
  }
}
#endif

/* -2 means defect, -1 means dead space (reserved, spare or slip) */
/* does not currently recognize different mapping */
int disk_translate_pbn_to_lbn(disk *currdisk, band *currband,
			      int cylno, int surfaceno, int blkno)
{
   int lbn = 0;
   int bandno = 0;

#ifdef RAWLAYOUT_SUPPORT
   if (currdisk->mapping == LAYOUT_RAWFILE) {
     if ((cylno < 0) || (cylno >= currdisk->numcyls) || (surfaceno < 0)
	 || (surfaceno >= currdisk->numsurfaces) || (blkno < 0)
	 || (blkno >= currdisk->numblocks)) {
       fprintf(stderr, "Illegal PBN values at disk_translate_pbn_to_lbn: %d %d %d\n", cylno, surfaceno, blkno);
       exit(0);
     }
     return(disk_pbn_to_lbn_rawlayout(currdisk,currband,cylno,surfaceno,
				      blkno));
   }
#endif

   if ((!currband) || (cylno < 0) || (cylno < currband->startcyl)
       || (cylno >= currdisk->numcyls) || (surfaceno < 0)
       || (surfaceno >= currdisk->numsurfaces) || (blkno < 0)
       || (blkno >= currband->blkspertrack)) {
      fprintf(stderr, "Illegal PBN values at disk_translate_pbn_to_lbn: %d %d %d\n", cylno, surfaceno, blkno);
      exit(0);
   }
   switch (currdisk->mapping) {
   case LAYOUT_NORMAL:
   case LAYOUT_CYLSWITCHONSURF1:
   case LAYOUT_CYLSWITCHONSURF2:

      /* XXX - building a little table of these would save recalculating each
         time - the next 4 lines account for 4% of the total execution time */
      while (currband != &currdisk->bands[bandno]) {
         lbn += currdisk->bands[bandno].blksinband;
         bandno++;
         if (bandno >= currdisk->numbands) {
            fprintf(stderr, "Currband not found in band list for currdisk\n");
            exit(0);
         }
      }

      if (currdisk->sparescheme == NO_SPARING) {
         surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
         lbn += (((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack;
         lbn += blkno - currband->deadspace;
         lbn = (lbn < 0) ? -1 : lbn;
      } else if (currdisk->sparescheme == TRACK_SPARING) {
         lbn = disk_pbn_to_lbn_trackspare(currdisk, currband, cylno, surfaceno, blkno, lbn);
      } else if (currdisk->sparescheme == SECTPERTRACK_SPARING) {
         lbn = disk_pbn_to_lbn_sectpertrackspare(currdisk, currband, cylno, surfaceno, blkno, lbn);
      } else if ((issectpercyl(currdisk)) || (currdisk->sparescheme == SECTATEND_SPARING)) {
         lbn = disk_pbn_to_lbn_sectpercylspare(currdisk, currband, cylno, surfaceno, blkno, lbn);
      } else if ((currdisk->sparescheme == SECTSPERZONE_SPARING) || (currdisk->sparescheme == SECTSPERZONE_SPARING_SLIPTOEND)) {
         lbn = disk_pbn_to_lbn_sectperzonespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
      } else if (currdisk->sparescheme == SECTPERRANGE_SPARING) {
         lbn = disk_pbn_to_lbn_sectperrangespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
      } else {
         fprintf(stderr, "Unknown sparing scheme at disk_translate_pbn_to_lbn: %d\n", currdisk->sparescheme);
         exit(0);
      }
      break;
#ifdef RAWLAYOUT_SUPPORT
   case LAYOUT_RAWFILE:
      fprintf(stderr,"disk_translate_pbn_to_lbn: LAYOUT_RAWFILE is not handled here!\n");
      exit(0);
#endif
   case LAYOUT_SERPENTINE:
   default:
      fprintf(stderr, "Unknown mapping at disk_translate_pbn_to_lbn: %d\n", currdisk->mapping);
      exit(0);
   }

   return(lbn);
}


/****************************************************************************/
/*                                                                          */
/* The next several functions compute the lbn boundaries (first and last)   */
/*   for a given physical track <cyl, surf>, for different sparing/mapping   */
/*   schemes.  The start and end values are proper lbns, or -2 if the       */
/*   entire track consists of remapped defects, or -1 if the entire track   */
/*   consists of slipped defects or unused spare space.  Note: the lbns     */
/*   returned account for slippage but purposefully ignore remapping of     */
/*   sectors to other tracks.                                               */
/*                                                                          */
/* Note: the per-scheme functions take a pointer to the relevant band       */
/*   and the first lbn in that band, in addition to the basic parameters.   */
/*                                                                          */
/****************************************************************************/

/* Assume that we never slip sectors over track boundaries! */

void disk_get_lbn_boundaries_sectpertrackspare(disk *currdisk, band *currband,
        				       int cylno, int surfaceno,
        				       int *startptr, int *endptr,
        				       int lbn)
{
   /* lbn equals first block in band */
   int i;
   int blkno;
   int temp_lbn = lbn;
   int lbnspertrack = currband->blkspertrack - currband->sparecnt;

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   lbn += ((((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * lbnspertrack) - currband->deadspace;
   if (startptr) {
      *startptr = ((lbn + lbnspertrack) <= temp_lbn) ? -1 : max(temp_lbn, lbn);
   }
   if (endptr) {
      lbn += lbnspertrack;
      *endptr = (lbn <= temp_lbn) ? -1 : lbn;
   }

   blkno = (((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack;
   for (i=0; i<currband->numdefects; i++) {
      if ((blkno <= currband->defect[i]) && ((blkno + currband->blkspertrack) > currband->defect[i])) {
         remapsector = TRUE;
      }
   }
}


void disk_get_lbn_boundaries_sectpercylspare(disk *currdisk, band *currband,
        				     int cylno, int surfaceno,
        				     int *startptr, int *endptr,
        				     int lbn)
{
   /* lbn equals first block in band */
   int i;
   int blkno = 0;
   int lbnadd;
   int temp_lbn = lbn;

   if (startptr) {
      lbnadd = 0;
      /* use brute force -- back translate each pbn to figure out startlbn */
      for (blkno=0; blkno<currband->blkspertrack; blkno++) {
         remapsector = FALSE;
         *startptr = disk_pbn_to_lbn_sectpercylspare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*startptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *startptr = -1;
         }
         if ((*startptr) >= 0) {
            *startptr = max(((*startptr) - lbnadd),temp_lbn);
            break;
         }
      }
      if (blkno == currband->blkspertrack) {
         *startptr = -1;
      }
   }
   if (endptr) {
      lbnadd = 1;
      /* use brute force -- back translate each pbn to figure out endlbn */
      for (blkno=(currband->blkspertrack-1); blkno>=0; blkno--) {
         remapsector = FALSE;
         *endptr = disk_pbn_to_lbn_sectpercylspare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*endptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *endptr = -1;
         }
         if ((*endptr) >= 0) {
            *endptr = (*endptr) + lbnadd;
            break;
         }
      }
      if (blkno == -1) {
         *endptr = -1;
      }
   }
   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkno = (((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack;
   for (i=0; i<currband->numdefects; i++) {
      if ((blkno <= currband->defect[i]) && ((blkno + currband->blkspertrack) > currband->defect[i])) {
         remapsector = TRUE;
      }
   }
}


void disk_get_lbn_boundaries_sectperrangespare(disk *currdisk, band *currband,
        				     int cylno, int surfaceno,
        				     int *startptr, int *endptr,
        				     int lbn)
{
   /* lbn equals first block in band */
   int i;
   int blkno;
   int lbnadd;
   int temp_lbn = lbn;

   if (startptr) {
      lbnadd = 0;
      /* use brute force -- back translate each pbn to figure out startlbn */
      for (blkno=0; blkno<currband->blkspertrack; blkno++) {
         remapsector = FALSE;
         *startptr = disk_pbn_to_lbn_sectperrangespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*startptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *startptr = -1;
         }
         if ((*startptr) >= 0) {
            *startptr = max(((*startptr) - lbnadd),temp_lbn);
            break;
         }
      }
      if (blkno == currband->blkspertrack) {
         *startptr = -1;
      }
   }
   if (endptr) {
      lbnadd = 1;
      /* use brute force -- back translate each pbn to figure out endlbn */
      for (blkno=(currband->blkspertrack-1); blkno>=0; blkno--) {
         remapsector = FALSE;
         *endptr = disk_pbn_to_lbn_sectperrangespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*endptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *endptr = -1;
         }
         if ((*endptr) >= 0) {
            *endptr = (*endptr) + lbnadd;
            break;
         }
      }
      if (blkno == -1) {
         *endptr = -1;
      }
   }
   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkno = (((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack;
   for (i=0; i<currband->numdefects; i++) {
      if ((blkno <= currband->defect[i]) && ((blkno + currband->blkspertrack) > currband->defect[i])) {
         remapsector = TRUE;
      }
   }
}


void disk_get_lbn_boundaries_sectperzonespare(disk *currdisk, band *currband,
					     int cylno, int surfaceno,
					     int *startptr, int *endptr,
					     int lbn)
{
   /* lbn equals first block in band */
   int i;
   int blkno;
   int lbnadd;
   int temp_lbn = lbn;

   if (startptr) {
      lbnadd = 0;
      /* use brute force -- back translate each pbn to figure out startlbn */
      for (blkno=0; blkno<currband->blkspertrack; blkno++) {
         remapsector = FALSE;
         *startptr = disk_pbn_to_lbn_sectperzonespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*startptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *startptr = -1;
         }
         if ((*startptr) >= 0) {
            *startptr = max(((*startptr) - lbnadd),temp_lbn);
            break;
         }
      }
      if (blkno == currband->blkspertrack) {
         *startptr = -1;
      }
   }
   if (endptr) {
      lbnadd = 1;
      /* use brute force -- back translate each pbn to figure out endlbn */
      for (blkno=(currband->blkspertrack-1); blkno>=0; blkno--) {
         remapsector = FALSE;
         *endptr = disk_pbn_to_lbn_sectperzonespare(currdisk, currband, cylno, surfaceno, blkno, lbn);
         if ((*endptr) == -2) {
            lbnadd++;
         }
         if (remapsector) {
            *endptr = -1;
         }
         if ((*endptr) >= 0) {
            *endptr = (*endptr) + lbnadd;
            break;
         }
      }
      if (blkno == -1) {
         *endptr = -1;
      }
   }
   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   blkno = (((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack;
   for (i=0; i<currband->numdefects; i++) {
      if ((blkno <= currband->defect[i]) && ((blkno + currband->blkspertrack) > currband->defect[i])) {
         remapsector = TRUE;
      }
   }
}


void disk_get_lbn_boundaries_trackspare(disk *currdisk, band *currband,
					int cylno, int surfaceno,
					int *startptr, int *endptr, int lbn)
{
   int i;
   int trackno;
   int lasttrack;
   int blkno;

   surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
   trackno = ((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno;
   for (i=(currband->numdefects-1); i>=0; i--) {
      if (trackno == currband->defect[i]) {   /* Remapped bad track */
	 lbn = -2;
      }
      if (trackno == currband->remap[i]) {
	 trackno = currband->defect[i];
	 break;
      }
   }
   for (i=(currband->numslips-1); i>=0; i--) {
      if (trackno == currband->slip[i]) {     /* Slipped bad track */
	 lbn = -1;
      }
      if (trackno > currband->slip[i]) {
	 trackno--;
      }
   }
   lasttrack = (currband->blksinband + currband->deadspace) / currband->blkspertrack;
   if (trackno > lasttrack) {                 /* Unused spare track */
      lbn = -1;
   }
   blkno = lbn + (trackno * currband->blkspertrack) - currband->deadspace;
   if (startptr) {
      if (lbn < 0) {
	 *startptr = lbn;
      } else {
         *startptr = ((blkno + currband->blkspertrack) <= lbn) ? -1 : max(blkno, lbn);
      }
   }
   if (endptr) {
      blkno += currband->blkspertrack;
      if (lbn < 0) {
	 *endptr = lbn;
      } else {
         *endptr = (blkno <= lbn) ? -1 : blkno;
      }
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
#ifdef RAWLAYOUT_SUPPORT
void 
disk_get_lbn_boundaries_rawlayout(disk *currdisk, band *currband,
				  int cylno, int surfaceno,
				  int *startptr, int *endptr)
{
  *startptr = currdisk->rlcyls[cylno].surface[surfaceno].firstlbn;
  *endptr = currdisk->rlcyls[cylno].surface[surfaceno].lastlbn + 1;
  if (!(currdisk->rlcyls[cylno].surface[surfaceno].valid)) {
    /*  if (*startptr == *endptr) */
    /* if both are the same, the track is not mapped */
    *startptr = -1;
    *endptr = -1;
  }
}
#endif

void disk_get_lbn_boundaries_for_track(disk *currdisk, band *currband,
				       int cylno, int surfaceno,
				       int *startptr, int *endptr)
{
   int temp_lbn, lbn = 0;
   int bandno = 0;

   if ((!startptr) && (!endptr)) {
      return;
   }

#ifdef RAWLAYOUT_SUPPORT
   if (currdisk->mapping == LAYOUT_RAWFILE) {
     if ((cylno < 0) || (cylno >= currdisk->numcyls) ||
	 (surfaceno < 0) || (surfaceno >= currdisk->numsurfaces)) {
       fprintf(stderr, "Illegal PBN values at disk_get_lbn_boundaries_for_track: %d %d\n", cylno, surfaceno);
       exit(0);
     }
     disk_get_lbn_boundaries_rawlayout(currdisk, currband, cylno, surfaceno, 
				       startptr, endptr);
     return;
   }
#endif

   if ((currband == NULL) || (cylno < currband->startcyl) || (cylno >= currdisk->numcyls) || (surfaceno < 0) || (surfaceno >= currdisk->numsurfaces)) {
      fprintf(stderr, "Illegal PBN values at disk_get_lbn_boundaries_for_track: %d %d\n", cylno, surfaceno);
      fprintf(stderr, "Info: startcyl %d, numcyls %d, numsurfaces %d\n", ((currband)?currband->startcyl:-1), currdisk->numcyls, currdisk->numsurfaces);
      exit(0);
   }

   switch (currdisk->mapping) {
   case LAYOUT_NORMAL:
   case LAYOUT_CYLSWITCHONSURF1:
   case LAYOUT_CYLSWITCHONSURF2:
      while (currband != &currdisk->bands[bandno]) {
         lbn += currdisk->bands[bandno].blksinband;
         bandno++;
         if (bandno >= currdisk->numbands) {
	    fprintf(stderr, "Currband not found in band list for currdisk\n");
	    exit(0);
         }
      }

      temp_lbn = lbn;

      if (currdisk->sparescheme == NO_SPARING) {
         surfaceno = diskmap_surfno_on_cylinder (currdisk, currband, cylno, surfaceno);
         lbn += ((((cylno - currband->startcyl) * currdisk->numsurfaces) + surfaceno) * currband->blkspertrack) - currband->deadspace;
         if (startptr) {
            *startptr = ((lbn + currband->blkspertrack) <= temp_lbn) ? -1 : max(lbn, temp_lbn);
         }
         if (endptr) {
            lbn += currband->blkspertrack;
            *endptr = (lbn <= temp_lbn) ? -1 : lbn;
         }
      } else if (currdisk->sparescheme == TRACK_SPARING) {
         disk_get_lbn_boundaries_trackspare(currdisk, currband, cylno, surfaceno, startptr, endptr, lbn);
      } else if (currdisk->sparescheme == SECTPERTRACK_SPARING) {
         disk_get_lbn_boundaries_sectpertrackspare(currdisk, currband, cylno, surfaceno, startptr, endptr, lbn);
      } else if ((issectpercyl(currdisk)) || (currdisk->sparescheme == SECTATEND_SPARING)) {
         disk_get_lbn_boundaries_sectpercylspare(currdisk, currband, cylno, surfaceno, startptr, endptr, lbn);
      } else if ((currdisk->sparescheme == SECTSPERZONE_SPARING) || (currdisk->sparescheme == SECTSPERZONE_SPARING_SLIPTOEND)) {
         disk_get_lbn_boundaries_sectperzonespare(currdisk, currband, cylno, surfaceno, startptr, endptr, lbn);
      } else if (currdisk->sparescheme == SECTPERRANGE_SPARING) {
         disk_get_lbn_boundaries_sectperrangespare(currdisk, currband, cylno, surfaceno, startptr, endptr, lbn);
      } else {
         fprintf(stderr, "Unknown sparing scheme at disk_get_lbn_boundaries_for_track: %d\n", currdisk->sparescheme);
         exit(0);
      }
      break;
#ifdef RAWLAYOUT_SUPPORT
   case LAYOUT_RAWFILE:
      fprintf(stderr,"disk_get_lbn_boundaries_for_track: LAYOUT_RAWFILE is not handled here!\n");
      exit(0);
#endif
   case LAYOUT_SERPENTINE:
   default:
      fprintf(stderr, "Unknown mapping at disk_get_lbn_boundaries_for_track: %d\n", currdisk->sparescheme);
      exit(0);
   }
}


/****************************************************************************/
/*                                                                          */
/* The next several functions compute the physical media location to which  */
/*   a given lbn (blkno) is mapped for different sparing/mapping schemes.    */
/*   The returned value is a proper lbn, or -2 if the sector is a remapped  */
/*   defect, or -1 if the sector is a slipped defect or an unused spare.    */
/*                                                                          */
/* Note: the per-scheme functions take a pointer to the relevant band       */
/*   and the adjusted version of the lbn (i.e., relative to the band's      */
/*   first lbn), in addition to the basic parameters.                       */
/*                                                                          */
/****************************************************************************/


/* NOTE:  No slipping beyond the end of a track is allowed.
          The following code will produce incorrect results if this rule
          is violated.
          To fix this, trackno needs to be recomputed after slips and slips
          on immediately previous tracks need to be accounted for.
          Low Priority.
*/

void disk_lbn_to_pbn_sectpertrackspare(disk *currdisk, band *currband,
				       int blkno, int maptype, int *cylptr,
				       int *surfaceptr, int *blkptr)
{
   int i;
   int firstblkontrack = -1;

   int blkspertrack = currband->blkspertrack;
   int lbnspertrack = blkspertrack - currband->sparecnt;
   int trackno = blkno / lbnspertrack;

   blkno = blkno % lbnspertrack;
   if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
      firstblkontrack = blkspertrack * trackno;
      for (i=0; i<currband->numslips; i++) {
         if ((currband->slip[i] >= firstblkontrack) && ((currband->slip[i] - firstblkontrack) <= blkno)) {
            blkno++;
         }
      }
   }
   if (maptype == MAP_FULL) {
      for (i=0; i<currband->numdefects; i++) {
         if (currband->defect[i] == (firstblkontrack + blkno)) {
            remapsector = TRUE;
            trackno = currband->remap[i] / blkspertrack;
            firstblkontrack = blkspertrack * trackno;
            blkno = currband->remap[i] % blkspertrack;
         }
      }
   }

   if (cylptr) {
      *cylptr = trackno/currdisk->numsurfaces + currband->startcyl;
   }
   if (surfaceptr) {
      *surfaceptr = trackno % currdisk->numsurfaces;
      *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
   }
   if (blkptr) {
      if (blkno >= blkspertrack) {
         printf ("Somehow computed a blkno that crosses to another track\n");
         printf ("This could be the result of too many slips, or of a bug\n");
         printf ("in the defect-related computations...\n");
         exit (-1);
      }
      *blkptr = blkno;
   }
}


void disk_lbn_to_pbn_sectpercylspare(disk *currdisk, band *currband,
				     int blkno, int maptype, int *cylptr,
				     int *surfaceptr, int *blkptr)
{
   int i;
   int blkspertrack;
   int blkspercyl;
   int lbnspercyl;
   int firstblkoncyl;
   int cyl;
   int slips = 0;

   blkspertrack = currband->blkspertrack;
   blkspercyl = blkspertrack * currdisk->numsurfaces;
   lbnspercyl = blkspercyl - currband->sparecnt;

   cyl = blkno / lbnspercyl;
   blkno = blkno % lbnspercyl;

   if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
      firstblkoncyl = cyl * blkspertrack * currdisk->numsurfaces;
      for (i=0; i<currband->numslips; i++) {
         if (((issliptoend(currdisk)) && ((currband->slip[i]/blkspercyl) < cyl)) || ((currband->slip[i] >= firstblkoncyl) && ((currband->slip[i] - firstblkoncyl) <= (blkno+slips)))) {
            slips++;
         }
      }
   }
   blkno += slips;
   if (maptype == MAP_FULL) {
      for (i=0; i<currband->numdefects; i++) {
         if ((currband->defect[i] == (firstblkoncyl + blkno)) && (currband->remap[i] != currband->defect[i])) {
            remapsector = TRUE;
            blkno = currband->remap[i];
            cyl = blkno / blkspercyl;
            blkno = blkno % blkspercyl;
            goto disk_lbn_to_pbn_sectpercylspare_done;
         }
      }
   }

   cyl += blkno / lbnspercyl;
   blkno = blkno % lbnspercyl;

disk_lbn_to_pbn_sectpercylspare_done:

   if (cylptr) {
      *cylptr = cyl + currband->startcyl;
   }
   if (surfaceptr) {
      *surfaceptr = blkno / blkspertrack;
      *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
   }
   if (blkptr) {
      *blkptr = blkno % blkspertrack;
   }
}


/* Assume no remapping/slipping out of a range. */

void disk_lbn_to_pbn_sectperrangespare(disk *currdisk, band *currband,
				     int blkno, int maptype, int *cylptr,
				     int *surfaceptr, int *blkptr)
{
   int i;
   int blksperrange;
   int blkspercyl;
   int lbnsperrange;
   int firstblkinrange;
   int rangeno;
   int slips = 0;

   blkspercyl = currband->blkspertrack * currdisk->numsurfaces;
   blksperrange = blkspercyl * currdisk->rangesize;
   lbnsperrange = blksperrange - currband->sparecnt;
   rangeno = blkno / lbnsperrange;

   blkno = blkno % lbnsperrange;

   if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
      firstblkinrange = rangeno * blksperrange;
      for (i=0; i<currband->numslips; i++) {
         if ((currband->slip[i] >= firstblkinrange) && ((currband->slip[i] - firstblkinrange) <= (blkno+slips))) {
            slips++;
         }
      }
   }
   blkno += slips;
   if (maptype == MAP_FULL) {
      for (i=0; i<currband->numdefects; i++) {
         if ((currband->defect[i] == (firstblkinrange + blkno)) && (currband->remap[i] != currband->defect[i])) {
            remapsector = TRUE;
            blkno = currband->remap[i];
            blkno = blkno % blksperrange;
            break;
         }
      }
   }

   if (cylptr) {
      *cylptr = currband->startcyl + (rangeno * currdisk->rangesize) + (blkno / blkspercyl);
   }
   if (surfaceptr) {
      *surfaceptr = (blkno % blkspercyl) / currband->blkspertrack;
      *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
   }
   if (blkptr) {
      *blkptr = blkno % currband->blkspertrack;
   }
}


void disk_lbn_to_pbn_sectperzonespare(disk *currdisk, band *currband,
				     int blkno, int maptype, int *cylptr,
				     int *surfaceptr, int *blkptr)
{
   int i;
   int blkspercyl;
   int slips = 0;

   if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
      for (i=0; i<currband->numslips; i++) {
         if (currband->slip[i] <= (blkno+slips)) {
            slips++;
         }
      }
   }
   blkno += slips;
   if (maptype == MAP_FULL) {
      for (i=0; i<currband->numdefects; i++) {
         if ((currband->defect[i] == blkno) && (currband->remap[i] != currband->defect[i])) {
            remapsector = TRUE;
            blkno = currband->remap[i];
            break;
         }
      }
   }

   blkspercyl = currband->blkspertrack * currdisk->numsurfaces;

   if (cylptr) {
      *cylptr = currband->startcyl + blkno / blkspercyl;
   }
   if (surfaceptr) {
      *surfaceptr = (blkno % blkspercyl) / currband->blkspertrack;
      *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
   }
   if (blkptr) {
      *blkptr = blkno % currband->blkspertrack;
   }
}


/* NOTE:  The total number of allowable slips and remaps per band
	  is equal to the number of spare tracks per band.  The 
	  following code will produce incorrect results if this rule 
	  is violated.  To fix this, currband needs to be re-calculated 
	  after the detection of a slip or spare.  Also, slips on 
	  immediately previous bands need to be accounted for.  Lastly,
	  the mismatch in sectors per track between zones must be 
	  handled.  Extremely low priority.
*/

void disk_lbn_to_pbn_trackspare(disk *currdisk, band *currband,
				int blkno, int maptype, int *cylptr,
				int *surfaceptr, int *blkptr)
{
   int i;
   int blkspertrack;
   int trackno;

   blkspertrack = currband->blkspertrack;
   trackno = blkno/blkspertrack;
   if ((maptype == MAP_ADDSLIPS) || (maptype == MAP_FULL)) {
      for (i=0; i<currband->numslips; i++) {
	 if (currband->slip[i] <= trackno) {
	    trackno++;
	 }
      }
   }
   if (maptype == MAP_FULL) {
      for (i=0; i<currband->numdefects; i++) {
	 if (currband->defect[i] == trackno) {
	    trackno = currband->remap[i];
	    break;
	 }
      }
   }
   if (cylptr) {
      *cylptr = (trackno/currdisk->numsurfaces) + currband->startcyl;
   }
   if (surfaceptr) {
      *surfaceptr = trackno % currdisk->numsurfaces;
      *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
   }
   if (blkptr) {
      *blkptr = blkno % blkspertrack;
   }
}

#ifdef RAWLAYOUT_SUPPORT

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
band *
disk_lbn_to_pbn_rawlayout(disk *currdisk,band *currband,int blkno, 
			  int maptype,int *cylptr,int *surfaceptr,int *blkptr){
  int i,r,cyl;
  int cond;
  int high,low;
  int maxhit,lasthigh = -1;

  /*  blkno += currband->firstblkno; */

  maxhit = low = 0;
  r = high = currdisk->numlbnranges-1;
  do {
    if (r > currdisk->numlbnranges-1) {
      cond = 0;
    }
    else {
      if (blkno <= currdisk->lbnrange[r].lastlbn) {
	cond = (blkno >= currdisk->lbnrange[r].firstlbn);
      }
      else {
	cond = 1;
      }
    }
    /* printf("%d [%d,%d]\t\t%d\n",lasthigh,low,high,cond); */
  } while(bin_search(cond,&r,&low,&high,&lasthigh,&maxhit));

  *cylptr = cyl = currdisk->lbnrange[r].cyl;

  for(i=0;i<currdisk->numsurfaces;i++) {
    disk_get_lbn_boundaries_rawlayout(currdisk,currband,cyl,i, &low,&high);
    if ((low <= blkno) && (high > blkno)) {
      if (blkptr) {
#ifdef DETAILED_RAW_LAYOUT
	int k,offset,skipped;
	int surfaceno = i;
        skipped = 0; 
	offset = currdisk->rlcyls[cyl].surface[surfaceno].sect[0];
	
	for(k = 0; k < currdisk->rlcyls[cyl].surface[surfaceno].valid; k++) {
	  if (blkno <= (currdisk->rlcyls[cyl].surface[surfaceno].seqcnt[k] +
			currdisk->rlcyls[cyl].surface[surfaceno].firstlbn +
			offset)) {
	    break;
	  }
	  else {
	    offset += currdisk->rlcyls[cyl].surface[surfaceno].sect[k] +
	      currdisk->rlcyls[cyl].surface[surfaceno].seqcnt[k] + 1;
	    skipped += currdisk->rlcyls[cyl].surface[surfaceno].sect[k+1] -
	      offset;
	      
	  }
	}
	*blkptr = offset + skipped;
#else
	*blkptr = blkno - low;
#endif
      }
      if (surfaceptr) {
	*surfaceptr = i;
      }
      break;
    }
  }
  
  for(i=0;i<currdisk->numbands;i++) {
    if ((currdisk->bands[i].startcyl <= cyl) && 
	(currdisk->bands[i].endcyl >= cyl)) {
      currband = &currdisk->bands[i];
      break;
    }
  }
  if (i == currdisk->numbands) {
    printf("%d\n",cyl);
    fprintf(stderr,"raw_layout_search: Did not find corresponding zone!\n");
    assert(1 == 0);
    exit(0);
  }
  return(currband);
}
#endif

band * disk_translate_lbn_to_pbn(disk *currdisk, int blkno, int maptype,
				 int *cylptr, int *surfaceptr, int *blkptr)
{
   int bandno = 0;
   int blkspertrack;
   band *currband = &currdisk->bands[0];

   if ((maptype > MAP_FULL) || (maptype < MAP_IGNORESPARING)) {
      fprintf(stderr, "Unimplemented mapping type at disk_translate_lbn_to_pbn: %d\n", maptype);
      exit(0);
   }

#ifdef RAWLAYOUT_SUPPORT
   if (currdisk->mapping == LAYOUT_RAWFILE) {
     if (blkno >= currdisk->numblocks) {
       fprintf(stderr, "blkno outside addressable space of disk: %d\n", blkno);
       assert(1==0);
       exit(0);
     }
     currband = disk_lbn_to_pbn_rawlayout(currdisk,currband, blkno, maptype,
					  cylptr, surfaceptr, blkptr);
     return(currband);
   }
#endif

   bandstart = 0;
   while ((blkno >= currband->blksinband) || (blkno < 0)) {
      bandstart += currband->blksinband;
      blkno -= currband->blksinband;
      bandno++;
      currband = &currdisk->bands[bandno];
      if ((bandno >= currdisk->numbands) || (blkno < 0)) {
         fprintf(stderr, "blkno outside addressable space of disk: %d, %d\n", blkno, bandno);
         exit(0);
      }
   }
   blkno += currband->deadspace;

   if (maptype == MAP_NONE)
      return (currband);

   switch(currdisk->mapping) {
   case LAYOUT_NORMAL:
   case LAYOUT_CYLSWITCHONSURF1:
   case LAYOUT_CYLSWITCHONSURF2:
      if ((maptype == MAP_IGNORESPARING) || (currdisk->sparescheme == NO_SPARING)) {
         blkspertrack = currband->blkspertrack;
         if (cylptr) {
	    *cylptr = blkno/(blkspertrack * currdisk->numsurfaces) + currband->startcyl;
         }
         if (surfaceptr) {
	    *surfaceptr = blkno/blkspertrack % currdisk->numsurfaces;
            *surfaceptr = diskmap_surfno_on_cylinder (currdisk, currband, *cylptr, *surfaceptr);
         }
         if (blkptr) {
	    *blkptr = blkno % blkspertrack;
         }
      } else {
         if (currdisk->sparescheme == TRACK_SPARING) {
	    disk_lbn_to_pbn_trackspare(currdisk, currband, blkno, maptype, cylptr, surfaceptr, blkptr);
         } else if (currdisk->sparescheme == SECTPERTRACK_SPARING) {
	    disk_lbn_to_pbn_sectpertrackspare(currdisk, currband, blkno, maptype, cylptr, surfaceptr, blkptr);
         } else if ((issectpercyl(currdisk)) || (currdisk->sparescheme == SECTATEND_SPARING)) {
	    disk_lbn_to_pbn_sectpercylspare(currdisk, currband, blkno, maptype, cylptr, surfaceptr, blkptr);
         } else if ((currdisk->sparescheme == SECTSPERZONE_SPARING) || (currdisk->sparescheme == SECTSPERZONE_SPARING_SLIPTOEND)) {
	    disk_lbn_to_pbn_sectperzonespare(currdisk, currband, blkno, maptype, cylptr, surfaceptr, blkptr);
         } else if (currdisk->sparescheme == SECTPERRANGE_SPARING) {
	    disk_lbn_to_pbn_sectperrangespare(currdisk, currband, blkno, maptype, cylptr, surfaceptr, blkptr);
         } else {
	    fprintf(stderr, "Unknown sparing scheme at disk_translate_lbn_to_pbn: %d\n", currdisk->sparescheme);
	    exit(0);
         }
      }
      break;
#ifdef RAWLAYOUT_SUPPORT
   case LAYOUT_RAWFILE:
     fprintf(stderr,"disk_translate_lbn_to_pbn: LAYOUT_RAWFILE is not handled here!\n");
     exit(0);
#endif
   case LAYOUT_SERPENTINE:
      /* look through this file for comments on which functions should
	 be changed for this. skew, pbn-lbn and lbn-pbn *should* be the only
	 things that need changing */
   default:
      fprintf(stderr, "Unknown mapping at disk_translate_lbn_to_pbn: %d\n", currdisk->mapping);
      exit(0);
   }
   return(currband);
}

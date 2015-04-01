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


/*
 * DIXtrac - Automated Disk Extraction Tool
 * Authors: Jiri Schindler, Greg Ganger
 *
 * Copyright (c) of Carnegie Mellon University, 1999, 2000.
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

/* uncomment the next line if compiling as part of DiskSim code */
#define DISKSIM 

#ifdef DISKSIM

#include <stdio.h>
#include <stdlib.h>

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_disk.h"
#define internal_error      printf
#define error_handler       printf

#else

#include "dixtrac.h"
#include "handle_error.h"
#include "extract_layout.h"
typedef band_t band;
typedef diskinfo_t disk;

#endif

#ifdef RAWLAYOUT_SUPPORT

/*  #define PRINT_LAYOUT_INFO  1 */

#define MAXSECTORS     1000
/* the threshold is multiplied by the capacity of the disk (in GB) */
//#define COUNT_THRSHLD  40
#define COUNT_THRSHLD  19
/* the difference in the number cylinders to distinguish different zones */
#define PERCENT        0.04
/* discard the cylinder with the same SPT if more than CYL_DISTANCE away */
/* allow for spare cylinders and "long enough defect */
#define MIN_CYL_DISTANCE 20

#define NOVAL          -9999

typedef struct sfreq {
  int count,cyls;
  int mincyl,maxcyl;
} sfreq_t;


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
determine_zones(disk *currdisk, sfreq_t *sectfreq, int capacity)
{
  typedef struct sp {
    int spt;
    int count;
    int avgcyl;
    struct sp *next;
  } spt_t;

  spt_t *sectspertrk, *currspt, *temp;
  spt_t *prev = 0;
  int i;

  int sectpercyl_sum = 0;

  if (!(sectspertrk = (spt_t *) malloc(sizeof(spt_t))))
    fprintf(stderr,"Could not allocate memory!\n");

  sectspertrk->spt = 0;
  sectspertrk->count = 0;
  sectspertrk->avgcyl = 0;
  sectspertrk->next = NULL;

  for(i=1;i<MAXSECTORS;i++) {
    if (sectfreq[i].count > 0) {
      int avg = sectfreq[i].cyls / sectfreq[i].count;
      currspt = sectspertrk;
      do {
	if (abs(avg - currspt->avgcyl) > (currspt->avgcyl * PERCENT)) {
	  if (sectfreq[i].count >= (COUNT_THRSHLD * capacity)) {
	    if (!(temp = (spt_t *) malloc(sizeof(spt_t)))) {
	      fprintf(stderr,"Could not allocate memory!\n");
	    }
	    temp->spt = i;
	    temp->count = sectfreq[i].count;
	    temp->avgcyl = sectfreq[i].cyls / sectfreq[i].count;
	    temp->next = currspt;
	    if (currspt == sectspertrk) {
	      sectspertrk = temp;
	    }
	    else {
	      prev->next = temp;
	    }
	  }
	  break;
	} 
	else { 	/* within an allowed limit*/
	  if (sectfreq[i].count > currspt->count) {
	    currspt->spt = i;
	    currspt->count = sectfreq[i].count;
	    currspt->avgcyl = sectfreq[i].cyls / sectfreq[i].count;
	    break;
	  }
	}
	prev = currspt;
      } while ((currspt = currspt->next));
    }
#ifdef PRINT_LAYOUT_INFO
    if (i == 1) {
      printf("\tSPT\tCount\tAvg. Cyl\n");
      printf("=============================\n");
    }
    if (sectfreq[i].count > 0) {
      printf("\t%d\t%d\t%d\n",i,sectfreq[i].count, 
	     sectfreq[i].cyls / sectfreq[i].count);
    }
#endif
  }
  
  currspt = sectspertrk;

#ifdef PRINT_LAYOUT_INFO
  printf("\nSPT\tCount\tAvg. Cyl\tMin. Cyl\tMax. Cyl\n");
  printf("========================================================\n");
#endif

  i = 0;
  do {
    if (currspt->count > 0) {
      currdisk->bands[i].startcyl = sectfreq[currspt->spt].mincyl;
      currdisk->bands[i].endcyl = sectfreq[currspt->spt].maxcyl;
      currdisk->bands[i].blkspertrack = currspt->spt;
      currdisk->bands[i].blksinband = 
	currdisk->rlcyls[currdisk->bands[i].endcyl].lastlbn - 
	currdisk->rlcyls[currdisk->bands[i].startcyl].firstlbn + 1;

      currdisk->bands[i].firstblkno = 0.0;
      currdisk->bands[i].deadspace = 0;
      currdisk->bands[i].sparecnt = 0;
      currdisk->bands[i].numslips = 0;

      sectpercyl_sum += currspt->spt * currdisk->numsurfaces;

#ifdef PRINT_LAYOUT_INFO
      printf("%3d\t%6d\t%6d",currspt->spt,currspt->count,currspt->avgcyl);
      printf("\t\t%6d\t\t%6d\n",sectfreq[currspt->spt].mincyl,
             sectfreq[currspt->spt].maxcyl);
#endif
      i++;
    }
    temp = currspt;
    free(temp);
    currspt = currspt->next;
  } while (currspt);
  currdisk->numbands = i;

  currdisk->sectpercyl = sectpercyl_sum / currdisk->numbands;

#ifdef PRINT_LAYOUT_INFO
  printf("\n\n");
  printf("SPT\tStart LBN\tEnd LBN  \tMin. Cyl\tMax. Cyl\n");
  printf("================================================================\n");
  {
    int total = 0;
    for(i=0;i< currdisk->numbands;i++) {
      printf("%3d\t%8d\t%8d",currdisk->bands[i].blkspertrack,
	     currdisk->rlcyls[currdisk->bands[i].startcyl].firstlbn,
	     currdisk->rlcyls[currdisk->bands[i].startcyl].firstlbn +
	     currdisk->bands[i].blksinband - 1);
      printf("\t%6d\t\t%6d\n",
	     /*  currdisk->bands[i].blksinband, */
	     currdisk->bands[i].startcyl,
	     currdisk->bands[i].endcyl);
      total += currdisk->bands[i].blksinband;
    }
    printf("\nTotal = %d\n",total);
  }
#endif
}

/*---------------------------------------------------------------------------*
 * Zero on success, nonzero on failure                                       * 
 *---------------------------------------------------------------------------*/
int
read_raw_layout(disk *currdisk,char *rlfilename) {
#define LINELEN      100
  char line[LINELEN];
  int rot, numsurfaces;
  int lbn, cyl, head, sect, seqcnt;
  int linenum,retval,currcyl,currhead,valid_idx,i,spt;
  int range = 0;
  FILE *fp;
  int maxlbn,blocksize,numcyls;

  int *sf_cyls;
  sfreq_t *sectfreq;
#ifndef DISKSIM
  int cnt;
#endif

  if (!(fp = fopen (rlfilename, "r"))) {
    error_handler("Error opening raw layout file %s!\n",rlfilename);
    return -1;
  }
  
  if (!(sectfreq = (sfreq_t *) malloc(MAXSECTORS*sizeof(sfreq_t)))) 
    internal_error("Could not allocate sectfreq structure!\n");
  memset(sectfreq,0,sizeof(sectfreq));
  for(i=1;i<MAXSECTORS;i++)
    sectfreq[i].mincyl = NOVAL;

  if (!(sf_cyls = (int *) malloc(MAXSECTORS*sizeof(int)))) 
    internal_error("Could not allocate sf_cyls structure!\n");
  for(i=0;i<MAXSECTORS;i++)
    sf_cyls[i] = NOVAL;

  fgets(line,LINELEN,fp);
  retval = fscanf(fp,"maxlbn %d, blocksize %d\n",&maxlbn,&blocksize);
  retval = fscanf (fp, "%d cylinders, %d rot, %d heads\n", &numcyls,&rot, 
		   &numsurfaces);

  if (retval != 3) {
    error_handler("Cannot read .layout.mappings file!\n");
    return -1;
  }

  linenum = 4;

  /*  currdisk->sectsize = blocksize; */
  currdisk->numsurfaces = numsurfaces;
  currdisk->numblocks = maxlbn+1;
  currdisk->numcyls = numcyls;
  currdisk->mapping = LAYOUT_RAWFILE;
  currdisk->sparescheme = NO_SPARING;

  if (!(currdisk->lbnrange = (lbn_rawlayout_t *) malloc(currdisk->numcyls * 
        currdisk->numsurfaces * sizeof(lbn_rawlayout_t))))
    internal_error("Could not allocate lbn_rawlayout_t structure!\n");

  if (!(currdisk->rlcyls = (cyl_rawlayout_t *)
	malloc(currdisk->numcyls * sizeof(cyl_rawlayout_t))))
    internal_error("Could not allocate cyl_rawlayout_t structure!\n");

  memset(currdisk->rlcyls,0,currdisk->numcyls * sizeof(cyl_rawlayout_t));
  for(i=0;i<currdisk->numcyls;i++)
    currdisk->rlcyls[i].firstlbn = NOVAL;

  currcyl = NOVAL;
  currhead = NOVAL;

  /*  printf("Reading layout for %s %s\n", manufacturer, product); */

  fscanf (fp, "lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n", 
	  &lbn, &cyl, &head, &sect, &seqcnt);
  do { 
    /* while (5 == fscanf (fp, "lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n", 
       &lbn, &cyl, &head, &sect, &seqcnt)) { */
    if (currdisk->numcyls <= cyl) {
      error_handler("Cyl %d on line %d is bigger than the max value of %d!\n",
		    cyl,linenum,currdisk->numcyls-1);
      return -1;
    }
    if (currdisk->numsurfaces <= head) {
      error_handler("Head %d on line %d is bigger than the max value of %d!\n",
		    head,linenum,currdisk->numsurfaces-1);
      return -1;
    }

    if ((currhead != head) || (currcyl != cyl)){
      if (currhead != NOVAL) {
	/* record the last lbn for the the surface */
	currdisk->rlcyls[currcyl].surface[currhead].lastlbn = lbn - 1;

	/* increment the count of tracks with # of sectors */
	spt = currdisk->rlcyls[currcyl].surface[currhead].lastlbn - 
	  currdisk->rlcyls[currcyl].surface[currhead].firstlbn + 1;

	if (spt >= MAXSECTORS) {
	  internal_error("Sector count exceeds %d (MAXSECTORS).\n",MAXSECTORS);
	}

	sectfreq[spt].count++;
	sectfreq[spt].cyls += /*  currcyl */ cyl;

	if (sf_cyls[spt] != NOVAL) {
	  if (abs(/*  currcyl */ cyl - sf_cyls[spt]) > MIN_CYL_DISTANCE) {
	    sectfreq[spt].mincyl = /*  currcyl */ cyl; 

	    sectfreq[spt].count = 1;
	    sectfreq[spt].cyls = /*  currcyl */ cyl;
	  }
	}
	sf_cyls[spt] = /*  currcyl */ cyl;

	/* record min and max cylinder for the given number of blocks */
	if ((sectfreq[spt].mincyl>/*  currcyl */ cyl) || 
	    (sectfreq[spt].mincyl==NOVAL)){
	  sectfreq[spt].mincyl = /*  currcyl */ cyl;
	}
	if (sectfreq[spt].maxcyl < /*  currcyl */ cyl)
	  sectfreq[spt].maxcyl = /*  currcyl */ cyl;
      }
      currhead = head;
    }
    if (currcyl != cyl) {
      if (currcyl != NOVAL) {
	currdisk->rlcyls[currcyl].lastlbn = lbn-1;
	currdisk->lbnrange[range-1].lastlbn = lbn-1;
      }
      currcyl = cyl;
      currdisk->rlcyls[currcyl].validcyl = 1;

      currdisk->lbnrange[range].firstlbn = lbn;
      currdisk->lbnrange[range].cyl = cyl;
      range++;

      if ((currdisk->rlcyls[currcyl].firstlbn > lbn) || 
	  (currdisk->rlcyls[currcyl].firstlbn == NOVAL)){
	currdisk->rlcyls[currcyl].firstlbn = lbn;
      }
    }
    /* !!! coalesce sequences if sect == 0 */
    valid_idx = currdisk->rlcyls[currcyl].surface[head].valid++;

    if (!valid_idx) {
      currdisk->rlcyls[currcyl].surface[head].firstlbn = lbn;
    }
#ifdef DETAILED_RAW_LAYOUT
    /* put in the sector and seqcnt values to the appropriate values */
    currdisk->rlcyls[currcyl].surface[head].sect[valid_idx] = sect;
    currdisk->rlcyls[currcyl].surface[head].seqcnt[valid_idx] = seqcnt;
#endif
    linenum++;
  } while (5 == fscanf (fp, "lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n",
			&lbn, &cyl, &head, &sect, &seqcnt));

  currdisk->rlcyls[currcyl].lastlbn = lbn+seqcnt;;
  currdisk->rlcyls[currcyl].surface[head].lastlbn = lbn+seqcnt;

  /* put in the values for range */
  currdisk->lbnrange[range-1].lastlbn = lbn+seqcnt;
  currdisk->numlbnranges=range;
#ifndef DISKSIM
  currdisk->defect_lists = (defect_t *) malloc (0x80000);
  memset((void *)currdisk->defect_lists,0,0x80000);
  cnt = 1;
  do {
    retval = fscanf (fp, "Defect at cyl %d, head %d, sect %d\n",
		     &currdisk->defect_lists[cnt].cyl,
		     &currdisk->defect_lists[cnt].head,
		     &currdisk->defect_lists[cnt].sect);
    if (retval == 3) {
      /*  printf ("Defect at cyl %d, head %d, sect %d\n", currdisk->defect_lists[cnt].cyl, currdisk->defect_lists[cnt].head, currdisk->defect_lists[cnt].sect); */
      cnt++;
    }
  /* prune duplicated list entries if they are in mappings file */
    if ((cnt > 2) && (currdisk->defect_lists[cnt-1].cyl == currdisk->defect_lists[cnt-2].cyl) &&
	(currdisk->defect_lists[cnt-1].head == currdisk->defect_lists[cnt-2].head) &&
	(currdisk->defect_lists[cnt-1].sect == currdisk->defect_lists[cnt-2].sect)) {
      cnt--;
    }
  } while (retval == 3);
  currdisk->defect_lists[0].cyl = cnt-1;
#endif  

  determine_zones(currdisk,sectfreq,(int) (((double) currdisk->numblocks) * 
		  ((double) blocksize) /1024 /1024 /1024));
  free(sectfreq);
  free(sf_cyls);
  fclose(fp);


  return 0;
}
#endif

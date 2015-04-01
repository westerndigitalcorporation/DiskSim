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


/* The following code is designed to extract the logical to physical  */
/* layout information of a modern SCSI disk.  It works for a number   */
/* of drives, but makes some assumptions about their behavior and has */
/* a number of known limitations.                                     */

/* Assumptions:                                                       */
/*              -- ignoring defects, the drive sequentially maps lbns */
/*                 onto the tracks down a cylinder and then onto the  */
/*                 next cylinder, starting from track #0 again.       */
/*              -- get_physical_address() can query the drive to      */
/*                 translate a lbn into the correct physical cylinder */
/*                 head and sector, where the latter is correct       */
/*                 relative to other sectors on the track (e.g.,      */
/*                 either the first expected logical sect or physical */
/*                 offset zero for the track).                        */

/* Limitations:                                                       */
/*              -- does not work for sectspercyl sparing disks that   */
/*                 keep their spares at the front of the cylinder     */
/*                 (e.g., Seagate Hawk)                               */
/*              -- does not work for "serpentine" disks (no known     */
/*                 products yet)                                      */
/*              -- does not extract skews (cylinder, track or zone),  */
/*                 since this is part of empirical extraction         */
/*              -- assumes that all sectspertrack sparing disks use   */
/*                 only a single sector of spare space per track.     */

/* To do:                                                             */
/*              -- remove up-front SLIPTOEND identification from      */
/*                 SECTSPERZONE sparing schemes.  Instead, try to     */
/*                 discover this after rest of extraction by looking  */
/*                 for patterns in the spaces after spares and defects*/


#define EXTRERR   -9876
// #define EXTRACT_ERROR(A...)      internal_error(A)
#define EXTRACT_ERROR(A...)      abort_extraction(A); return(EXTRERR)

#ifndef LAYOUT_ONLY
#include "build_scsi.h"
#include "print_bscsi.h"
#else
#define SECT_SIZE 512
#endif

#include "dixtrac.h"
#include "handle_error.h"
#include "extract_layout.h"
#include "extract_params.h"
#include "scsi_conv.h"


// XXX not here
#include <libddbg/libddbg.h>
#include <diskmodel/dm.h>

// XXX
extern struct dm_disk_if *adisk;

/* the number of iterations, rundom lbn translations, to determine max head */
#define MAXHEAD_TRANSLATIONS       200

//#define TEST_EVERY_LBN_TRANSLATION

#ifndef TEST_EVERY_LBN_TRANSLATION
#define USE_TRANSLATION_CACHE
#endif


#undef min
#define min(a,b) (((a)<(b))?(a):(b))
#undef max
#define max(a,b) (((a)>(b))?(a):(b))

diskinfo_t *diskinfo_list = NULL;

extern char *scsi_buf;

#ifndef LAYOUT_ONLY
extern diskinfo_t *thedisk;
#endif

void
abort_extraction (const char *fmt,
		  ...)
{
  char answer;
  va_list args;

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
  printf ("Quick layout extraction unsuccessful.\n");
  printf ("Do full layout extraction instead? (takes several hours) [Y/N]: ");
  while ((answer = getchar ()))
  {
    /* make sure to eliminate extra chars */
    if (answer == '\n' || answer == '\r')
      continue;
    /* the user doen't want the raw layout */
    if (answer == 'n')
    {
      exit (1);
    }
    else if (answer == 'y')
    {
      break;
    }
    else
    {
      printf ("Unrecognized answer. Please use either 'y' or 'n'.");
    }
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Get diskinfo structure associated with disk #devno, or allocate a new     * 
 *   one if none yet exists.                                                 *
 *                                                                           * 
 *---------------------------------------------------------------------------*/

diskinfo_t *
get_diskinfo (uint devno)
{
  diskinfo_t *diskinfo = diskinfo_list;

  while ((diskinfo != NULL) && (diskinfo->devno != devno))
  {
    diskinfo = diskinfo->next;
  }

  if (diskinfo == NULL)
  {
    diskinfo = (diskinfo_t *) malloc (sizeof (diskinfo_t));
    bzero (diskinfo, sizeof (diskinfo_t));
    diskinfo->next = diskinfo_list;
    diskinfo_list = diskinfo;
    diskinfo->devno = devno;
  }

  return (diskinfo);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Get the main information provided by the read capacity call: the number   *
 *   of lbns and size of each.  This is done by either querying the drive or *
 *   using the pre-queried information held in the diskinfo structure.       * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static void
get_read_capacity_info (diskinfo_t * diskinfo,
			int *maxlbnPtr,
			int *blocksizePtr)
{
  if ((diskinfo != NULL) && (diskinfo->query_results != NULL))
  {
    *maxlbnPtr = diskinfo->query_results->maxlbn;
    *blocksizePtr = diskinfo->query_results->blocksize;

  }
  else
  {
#ifdef LAYOUT_ONLY
    assert (0);
#else
    get_capacity (maxlbnPtr, blocksizePtr);
#endif
  }
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Get the main information provided by the read capacity call: the number   *
 *   of lbns and size of each.  This is done by either querying the drive or *
 *   using the pre-queried information held in the diskinfo structure.       * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static void
get_read_geometry_info (diskinfo_t * diskinfo,
			int *numcylsPtr,
			int *rotPtr,
			int *numsurfacesPtr,
			int *dummyPtr)
{
  if ((diskinfo != NULL) && (diskinfo->query_results != NULL))
  {
    *numcylsPtr = diskinfo->query_results->numcyls;
    *rotPtr = diskinfo->query_results->rot;
    *numsurfacesPtr = diskinfo->query_results->numsurfaces;

  }
  else
  {
#ifdef LAYOUT_ONLY
    assert (0);
#else
    get_geometry (numcylsPtr, rotPtr, numsurfacesPtr, dummyPtr);
#endif
  }
}


static void 
get_phys_address(int lbn, int *cyl, int *head, int *sector)
{
  struct dm_pbn pbn;
  adisk->layout->dm_translate_ltop(adisk, lbn, MAP_FULL, &pbn, 0);

  if(cyl) *cyl = pbn.cyl;
  if(head) *head = pbn.head;
  if(sector) *sector = pbn.sector;
}


#define get_capacity(a,b) \
		get_read_capacity_info (diskinfo_list, (a), (b))

#define get_geometry(a,b,c,d) \
		get_read_geometry_info (diskinfo_list, (a), (b), (c), (d))


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Read the defect list from the drive and store it for later use.           * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

defect_t *
read_defect_list (diskinfo_t * diskinfo,
		  defect_t **defects,
		  int *defects_len,
		  int *fmt)
{
#ifndef LAYOUT_ONLY

  char *defectlists = scsi_buf;
  struct scsi_defect_data_header *header =
    (struct scsi_defect_data_header *) defectlists;

  struct scsi_physical_sector_format *list =
    (struct scsi_physical_sector_format *)
    &defectlists[sizeof (struct scsi_defect_data_header)];

  uint listlen;
  uint format;
  int i, j;

  defect_t *defect_lists;
#endif

  if ((diskinfo != NULL) && (diskinfo->defect_lists == NULL)
      && (diskinfo->query_results != NULL))
  {
    diskinfo->defect_lists = diskinfo->query_results->defect_lists;
    return (diskinfo->defect_lists);
  }

#ifdef LAYOUT_ONLY
  assert (0);
  return -1;
#else

  assert (defectlists != NULL);
  
  exec_scsi_command (defectlists, SCSI_read_defect_data (0xFFFFF));
  // print_scsi_defect_data (defectlists);
  // print_hexpage(defectlists, 20);
  
#ifdef USE_READ_DEFECT_DATA_10
  listlen = _2btol((u_int8_t *) &header->length[0]);
  format = header->format & SRDDH10_DLIST_FORMAT_MASK;
  listlen  = listlen >> 3;
#else
  listlen = _4btol((u_int8_t *) &header->length[0]);
  format = header->format & SRDDH12_DLIST_FORMAT_MASK;
#endif

  /* OK to use SRDDH10 since both 10 and 12 byte versions are the same */
  if ((format != SRDDH10_BYTES_FROM_INDEX_FORMAT) && 
      (format != SRDDH10_PHYSICAL_SECTOR_FORMAT)) 
  {
      internal_error ("Cannot handle defect list format (%d) \n", format);
  }

  defect_lists = calloc(listlen, sizeof(defect_t));

  if(fmt) *fmt = format;
  if(defects_len) *defects_len = listlen;

  j = 0;
  for (i = 0; i < listlen; i++)
  {
    defect_lists[j].cyl = _3btol ((u_int8_t *) &list[i].cylno[0]);
    defect_lists[j].head = (int) list[i].headno;
    defect_lists[j].sect = _4btol ((u_int8_t *) &list[i].sectno[0]);

    /* This extra step is needed because some disks duplicate defect list */
    /* entries (for no reason that I understand...).                      */

    if (!j
	|| (defect_lists[j].cyl != defect_lists[j - 1].cyl) 
	|| (defect_lists[j].head != defect_lists[j - 1].head) 
	|| (defect_lists[j].sect != defect_lists[j - 1].sect))
    {
      j++;
    }
    else
    {
      if(defects_len) (*defects_len)--;
    }
  }


  if (diskinfo != NULL)
  {
    diskinfo->defect_lists = defect_lists;
  }

  if(defects) *defects = defect_lists;

  return 0;
#endif
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Read the defect list from the drive and store it for later use.           * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

#ifndef LAYOUT_ONLY
void
writeout_defect_list (defect_t * defect_lists,
		      FILE * fp)
{
  int i;

  for (i = 1; i <= defect_lists[0].cyl; i++)
  {
    fprintf (fp, "Defect at cyl %d, head %d, sect %d\n",
	     defect_lists[i].cyl, defect_lists[i].head, defect_lists[i].sect);
    // printf ("Defect at cyl %d, head %d, sect %d\n", defect_lists[i].cyl, defect_lists[i].head, defect_lists[i].sect);
  }
}
#endif


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Return the "defectno"th defect from the specified list.                   * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static void
get_defect_from_list (defect_t * defect_lists,
		      int defectno,
		      int *cylP,
		      int *headP,
		      int *sectP)
{
  if (defectno < defect_lists[0].cyl)
  {
    *cylP = defect_lists[defectno + 1].cyl;
    *headP = defect_lists[defectno + 1].head;
    *sectP = defect_lists[defectno + 1].sect;
  }
  else
  {
    *cylP = -1;
    *headP = -1;
    *sectP = -1;
  }
}



/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Search the defect list for a defect located on the specified track.  If   * 
 *   one is found, return the corresponding sector number.  Otherwise,       * 
 *   return -2  (Note: -1 is not used as a return value since it is a valid  * 
 *   value for track sparing drives.)                                        * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

#if 0				/* to avoid warning -- not currently used... */
static int
get_defect_on_track (defect_t * defect_lists,
		     uint cyl,
		     uint head)
{
  int i;

  for (i = 1; i <= defect_lists[0].cyl; i++)
  {
    if ((defect_lists[i].cyl == cyl) && (defect_lists[i].head == head))
    {
      return (defect_lists[i].sect);
    }
  }

  return (-2);
}
#endif


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Count the number of defects on the specified track.                       * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

int
count_defects_on_track (struct dm_disk_if *d,
			uint cyl,
			uint head)
{
  int defects = 0;
  struct dm_pbn track;

  track.cyl = cyl;
  track.head = head;
  track.sector = 0;

  // XXX check return code
  d->layout->dm_defect_count (d, &track, &defects);

  return defects;
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Count the number of defects on the specified cylinder.                    * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

int
count_defects_on_cylinder (struct dm_disk_if *d,
			   uint cyl)
{
  int defects = 0;
  int i;
  struct dm_pbn track;

  track.cyl = cyl;
  track.sector = 0;

  // XXX don't assume all surfaces mapped
  for (i = 0; i < d->dm_surfaces; i++)
  {
    int res;
    track.head = i;
    d->layout->dm_defect_count (d, &track, &res);
    defects += res;
  }

  return defects;
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Count the number of defects in the specified range of cylinders.          * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

int
count_defects_in_cylrange (struct dm_disk_if *d,
			   uint cyl1,
			   uint cyl2)
{
  int defects = 0;
  int i;

  for (i = cyl1; i <= cyl2; i++)
  {
    defects += count_defects_on_cylinder (d, i);
  }

  return defects;
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Find a range of <count> consecutive, defectfree cylinders, with the       * 
 * search beginning at the cylinder numbered <firsttry>.                     * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

int
find_defectfree_cylinder_range (struct dm_disk_if *d,
				int firsttry,
				int count)
{
  int startcyl = firsttry;
  int i;


  for (i = firsttry; i < d->dm_cyls; i++)
  {
    if (count_defects_on_cylinder (d, i) != 0)
    {
      startcyl = i + 1;
    }
    else if (i == (startcyl + count))
    {
      return (startcyl);
    }
  }

  EXTRACT_ERROR ("Could not find %d defectfree cylinders in a row.\n", count);
  return (-1);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Boolean function that determines whether the specified track contains     * 
 * spare sectors (independent of whether or not they are in use).            * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static int
istrackwithspare (diskinfo_t * diskinfo,
		  int cyl,
		  int head)
{
  int i;

  if (diskinfo->sparescheme == SECTPERTRACK_SPARING)
  {
    return (1);
  }

  if ((isspareatfront (diskinfo)) && (head == 0))
  {
    return (1);
  }

  if ((!isspareatfront (diskinfo)) && (head == (diskinfo->numsurfaces - 1)))
  {
    if (issectpercyl (diskinfo))
    {
      return (1);
    }

    for (i = 0; i < diskinfo->numbands; i++)
    {
      if ((cyl >= diskinfo->bands[i].startcyl)
	  && (cyl <= diskinfo->bands[i].endcyl))
      {
	break;
      }
    }

    /* sectperrange disks have spare sectors at the end of the last surface */
    /* on each <rangesize> cylinders, starting over in each zone.           */
    if ((diskinfo->sparescheme == SECTPERRANGE_SPARING)
	&& (((cyl - diskinfo->bands[i].startcyl) % (diskinfo)->rangesize) ==
	    (diskinfo->rangesize - 1)))
    {
      return (1);
    }
  }

  return (0);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Determine which band contains the given cylinder, or return -1 if none.   * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static int
determine_bandno_of_cyl (diskinfo_t * diskinfo,
			 int cyl)
{
  int i;
  for (i = 0; i < diskinfo->numbands; i++)
  {
    if ((cyl >= diskinfo->bands[i].startcyl)
	&& (cyl <= diskinfo->bands[i].endcyl))
    {
      return (i);
    }
  }
  return (-1);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Determine whether surfaces on the given cylinder are numbered downward,   * 
 *   as is normally done.  Return 1 if yes, and 0 if no.                     * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

static int
are_ascending_surfaces_numbered (diskinfo_t * diskinfo,
				 int cyl)
{
  if (diskinfo->mapping == LAYOUT_CYLSWITCHONSURF1)
  {
    int bandno = determine_bandno_of_cyl (diskinfo, cyl);
    assert (bandno != -1);
    if ((cyl - diskinfo->bands[bandno].startcyl) % 2)
    {
      return (0);
    }
  }
  else if (diskinfo->mapping == LAYOUT_CYLSWITCHONSURF2)
  {
    if (cyl % 2)
    {
      return (0);
    }
  }
  return (1);
}


/*---------------------------------------------------------------------------*
 *                                                                           *
 * Find the next sequential track (cyl, head), given a current track (cyl,   *
 * head) and an lbn on that track.  All three fields are updated to          *
 * correspond to said next track.                                            *
 *   Also, a defect from another track that is remapped to this one may or   *
 *   may not be counted here, depending on dumb luck... sigh.                *
 *                                                                           *
 *---------------------------------------------------------------------------*/

static void
find_next_track (int *lbnPtr,
		 int *cylPtr,
		 int *headPtr)
{
  int cyl2 = *cylPtr;
  int head2 = *headPtr;
  int sect;

  // printf ("find_next_track: lbn %d, cyl %d, head %d\n", *lbnPtr, *cylPtr, *headPtr);

  while ((*cylPtr == cyl2) && (*headPtr == head2))
  {
    *lbnPtr += 50;
    get_phys_address (*lbnPtr, cylPtr, headPtr, &sect);
  }
}






/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Write raw LBN-to-PBN mappings (all of them!) and some other info to an    * 
 * output file that can later be used to perform layout extraction without   * 
 * the actual disk.  This has been very useful for debugging.                * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

void
writeout_all_lbntopbn_mappings (uint devno,
				uint maxlbn,
				char *modelname,
				int start_lbn)
{
#ifndef LAYOUT_ONLY
  FILE *fp;
  int lbn, cyl, head, sect;
  int cyl2, head2, sect2;
  int seqcnt;
  int blocksize;
  int numcyls, rot, numsurfaces, dummy;
  char filename[80];
  char label[80];
  defect_t *defect_lists;

  sprintf (filename, "%s.%s", modelname, LAYOUT_MAP_EXT);
  fp = fopen (filename, "w");
  if (fp == NULL)
  {
    error_handler
      ("writeout_all_lbntopbn_mappings: unable to open file named %s for writing\n",
       LAYOUT_MAP_EXT);
  }
  get_disk_label (label);
  /*  fprintf(fp,"Disk brand name: %s\n",label); */

  /* printf ("Info extracted from a %s %s\n", manufacturer, product); */
  /* fprintf (fp, "Info extracted from a %s %s\n", manufacturer, product); */

  printf ("Info extracted from a %s\n", label);
  fprintf (fp, "Info extracted from a %s\n", label);

  get_capacity (&lbn, &blocksize);
  //printf ("maxlbn %d, blocksize %d\n", lbn, blocksize);
  fprintf (fp, "maxlbn %d, blocksize %d\n", lbn, blocksize);

  get_geometry (&numcyls, &rot, &numsurfaces, &dummy);
  //printf ("%d cylinders, %d rot, %d heads\n", numcyls, rot, numsurfaces);
  fprintf (fp, "%d cylinders, %d rot, %d heads\n", numcyls, rot, numsurfaces);

  for (lbn = start_lbn; lbn < maxlbn; lbn++)
  {

    get_phys_address (lbn, &cyl, &head, &sect);
    seqcnt = 0;
    get_phys_address ((lbn + 1), &cyl2, &head2, &sect2);
    while ((cyl == cyl2) && (head == head2) && (sect2 == (sect + seqcnt + 1)))
    {
      seqcnt++;
      get_phys_address ((lbn + 1 + seqcnt), &cyl2, &head2, &sect2);
    }

    //printf ("lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n", lbn, cyl, head, sect, seqcnt);
    fprintf (fp, "lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n", lbn, cyl,
	     head, sect, seqcnt);
    lbn += seqcnt;
  }

  if (!read_defect_list(NULL, &defect_lists, 0, 0))
  {
    writeout_defect_list (defect_lists, fp);
  }

  fclose (fp);
#endif
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Read previously extracted raw LBN-to-PBN mappings (all of them!) from     * 
 * the file named mapping_file.  Construct a "query_results" structure       * 
 * with the corresponding mappings, to be used in place of disk queries      * 
 * for the real LBN-to-PBN mappings.  This is mostly useful for debugging.   * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

void
readin_all_lbntopbn_mappings (uint devno,
			      char *mapping_file)
{
  FILE *fp;
  int seqcnt;
  diskinfo_t *diskinfo = get_diskinfo (devno);
  struct query_results *query_results;
  int rangecnt;
  int cnt;
  int ret;

  // fprintf (stderr, "readin_all_lbntopbn_mappings: devno %d, mapping_file=%s\n", devno, mapping_file);

  query_results =
    (struct query_results *) malloc (sizeof (struct query_results));
  bzero (query_results, sizeof (struct query_results));
  diskinfo->query_results = query_results;

  fp = fopen (mapping_file, "r");
  if (fp == NULL)
  {
    error_handler
      ("readin_all_lbntopbn_mappings: unable to open file named %s for reading\n",
       mapping_file);
  }

  fscanf (fp, "Info extracted from a %[^\n]\n", query_results->drivename);
  // printf ("Info extracted from a %s\n", query_results->drivename);

  fscanf (fp, "maxlbn %d, blocksize %d\n", &query_results->maxlbn,
	  &query_results->blocksize);
  // printf ("maxlbn %d, blocksize %d\n", query_results->maxlbn, query_results->blocksize);

  fscanf (fp, "%d cylinders, %d rot, %d heads\n", &query_results->numcyls,
	  &query_results->rot, &query_results->numsurfaces);
  // printf ("%d cylinders, %d rot, %d heads\n", query_results->numcyls, query_results->rot, query_results->numsurfaces);

  rangecnt = 0;
  do
  {
    ret = fscanf (fp, "lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n",
		  &query_results->ranges[rangecnt].lbn,
		  &query_results->ranges[rangecnt].cyl,
		  &query_results->ranges[rangecnt].head,
		  &query_results->ranges[rangecnt].sect, &seqcnt);
    if (ret == 5)
    {
      //printf ("lbn %d --> cyl %d, head %d, sect %d, seqcnt %d\n", query_results->ranges[rangecnt].lbn, query_results->ranges[rangecnt].cyl, query_results->ranges[rangecnt].head, query_results->ranges[rangecnt].sect, seqcnt);

      query_results->ranges[rangecnt].lastlbn = seqcnt +
	query_results->ranges[rangecnt].lbn;
      rangecnt++;
      if (rangecnt >= MAX_RANGES)
      {
	error_handler
	  ("query results file has too many lbn_to_pbn ranges (%d+)\n",
	   rangecnt);
      }
    }
  }
  while (ret == 5);
  query_results->rangecnt = rangecnt;
  // printf ("rangecnt %d\n", rangecnt);

  query_results->defect_lists = (defect_t *) malloc (0x80000);
  bzero (query_results->defect_lists, 0x80000);
  cnt = 1;
  do
  {
    ret = fscanf (fp, "Defect at cyl %d, head %d, sect %d\n",
		  &query_results->defect_lists[cnt].cyl,
		  &query_results->defect_lists[cnt].head,
		  &query_results->defect_lists[cnt].sect);
    if (ret == 3)
    {
      //printf ("Defect at cyl %d, head %d, sect %d\n", query_results->defect_lists[cnt].cyl, query_results->defect_lists[cnt].head, query_results->defect_lists[cnt].sect);
      cnt++;
    }
    /* prune duplicated list entries if they are in mappings file */
    if ((cnt > 2)
	&& (query_results->defect_lists[cnt - 1].cyl ==
	    query_results->defect_lists[cnt - 2].cyl)
	&& (query_results->defect_lists[cnt - 1].head ==
	    query_results->defect_lists[cnt - 2].head)
	&& (query_results->defect_lists[cnt - 1].sect ==
	    query_results->defect_lists[cnt - 2].sect))
    {
      cnt--;
    }
  }
  while (ret == 3);
  query_results->defect_lists[0].cyl = cnt - 1;

  fclose (fp);
}


// g1 layout foo
/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Write the extracted layout information to an output file fp in a format   * 
 * that is equivalent to that used in disksim parameter files.               *
 * If sansdefects is not 0, also the defects are written out to the file.    *
 *                                                                           * 
 *---------------------------------------------------------------------------*/

void
write_layout_file (diskinfo_t * diskinfo,
		   FILE * fp,
		   int sansdefects)
{
  int i;
  char value[255];

  sprintf (value, "%d", diskinfo->mapping);
  write_entry (fp, "LBN-to-PBN mapping scheme", value);

  sprintf (value, "%d", diskinfo->sparescheme);
  write_entry (fp, "Sparing scheme used", value);

  sprintf (value, "%d",
	   ((diskinfo->rangesize == 0) ? 1 : diskinfo->rangesize));
  write_entry (fp, "Rangesize for sparing", value);

  sprintf (value, "%d", diskinfo->numbands);
  write_entry (fp, "Number of bands", value);

  for (i = 0; i < diskinfo->numbands; i++)
  {
    fprintf (fp, "Band #%d\n", (i + 1));

    sprintf (value, "%d", diskinfo->bands[i].startcyl);
    write_entry (fp, "First cylinder number", value);

    sprintf (value, "%d", diskinfo->bands[i].endcyl);
    write_entry (fp, "Last cylinder number", value);

    sprintf (value, "%d", diskinfo->bands[i].blkspertrack);
    write_entry (fp, "Blocks per track", value);

    sprintf (value, "%f", diskinfo->bands[i].firstblkno);
    write_entry (fp, "Offset of first block", value);

    sprintf (value, "%d", diskinfo->bands[i].deadspace);
    write_entry (fp, "Empty space at zone front", value);

    sprintf (value, "%f", diskinfo->bands[i].trackskew);
    write_entry (fp, "Skew for track switch", value);

    sprintf (value, "%f", diskinfo->bands[i].cylskew);
    write_entry (fp, "Skew for cylinder switch", value);

    sprintf (value, "%d", diskinfo->bands[i].sparecnt);
    write_entry (fp, "Number of spares", value);

    if (sansdefects)
    {
      sprintf (value, "%d", 0);
      write_entry (fp, "Number of slips", value);

      sprintf (value, "%d", 0);
      write_entry (fp, "Number of defects", value);
    }
    else
    {
      int j;
      sprintf (value, "%d", diskinfo->bands[i].numslips);
      write_entry (fp, "Number of slips", value);

      for (j = 0; j < diskinfo->bands[i].numslips; j++)
      {
	fprintf (fp, "Slip:  %d\n", diskinfo->bands[i].slip[j]);
      }
      sprintf (value, "%d", diskinfo->bands[i].numdefects);
      write_entry (fp, "Number of defects", value);

      for (j = 0; j < diskinfo->bands[i].numdefects; j++)
      {
	fprintf (fp, "Defect:  %d  %d\n", diskinfo->bands[i].defect[j],
		 diskinfo->bands[i].remap[j]);
      }
    }
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 * Write the extracted layout information to an output file in a format      * 
 * that is equivalent to that used in disksim parameter files.  The          * 
 * output file's name will be "modelname.layout.style", where modelname      * 
 * is the corresponding input parameter and style is either 'full' or        * 
 * 'clean' depending on the sansdefects input parameter.  (1 --> clean)      *
 * The file has four additional lines of parameters so that it can be used   *
 * standalone for reading in the layout info.                                * 
 *                                                                           * 
 *---------------------------------------------------------------------------*/

void
writeout_layout (diskinfo_t * diskinfo,
		 int sansdefects,
		 char *modelname)
{
  FILE *fp;
  char filename[80];
  char value[255];

  if (sansdefects)
  {
    sprintf (filename, "%s.%s", modelname, LAYOUT_CLEAN_EXT);
  }
  else
  {
    sprintf (filename, "%s.%s", modelname, LAYOUT_FULL_EXT);
  }
  fp = fopen (filename, "w");
  if (fp == NULL)
  {
    error_handler
      ("writeout_layout: unable to open file named %s for writing\n",
       filename);
  }

  sprintf (value, "%d", diskinfo->numsurfaces);
  write_entry (fp, "Number of data surfaces", value);

  sprintf (value, "%d", diskinfo->numcyls);
  write_entry (fp, "Number of cylinders", value);

  sprintf (value, "%d", diskinfo->numblocks);
  write_entry (fp, "Blocks per disk", value);

  sprintf (value, "%d", diskinfo->sectpercyl);
  write_entry (fp, "Avg sectors per cylinder", value);


  write_layout_file (diskinfo, fp, sansdefects);
  fclose (fp);
}



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
#include "handle_error.h"
#include "build_scsi.h"
#include "print_bscsi.h"
#include "extract_layout.h"
#include "extract_params.h"

extern diskinfo_t *thedisk;

#ifdef ADDR_TRANS_COUNTER
extern int transaddr_cntr;
#endif

extern char *scsi_buf;

#ifndef DISKINFO_ONLY

#include <libparam/libparam.h>
#include <libddbg/libddbg.h>
#undef MAP_NONE           
#undef MAP_IGNORESPARING  
#undef MAP_ZONEONLY       
#undef MAP_ADDSLIPS       
#undef MAP_FULL           
#undef MAP_FROMTRACE      
#undef MAP_AVGCYLMAP      
#undef MAXMAPTYPE         
#include <diskmodel/dm.h>
#include <diskmodel/dm_types.h>


extern struct dm_disk_if *adisk;
#endif

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
diskinfo_physical_address(int lbn,int *cyl,int *head,int *sector) 
{
#ifndef DISKINFO_ONLY
  struct dm_pbn pbn;
  adisk->layout->dm_translate_ltop(adisk, lbn, MAP_FULL, &pbn, 0);
  *cyl = pbn.cyl;
  *head = pbn.head;
  *sector = pbn.sector;
#else
  disk_translate_lbn_to_pbn(thedisk,lbn,MAP_FULL,cyl,head,sector);
#endif
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
diskinfo_lbn_address(int cyl, int head, int sector, int *lbn) 
{
#ifndef DISKINFO_ONLY
  int r = 0;
  struct dm_pbn pbn;
  pbn.cyl = cyl;
  pbn.head = head;
  pbn.sector = sector;
  *lbn = adisk->layout->dm_translate_ptol(adisk, &pbn, &r);

  if ((*lbn < 0) || (r != 0)) {
    *lbn = -1;
    //    internal_error("diskinfo_lbn_address: bogus location (%d,%d,%d)\n",
    //		   cyl,head,sector);
  }
#else
  band_t *currband = diskinfo_get_band(cyl);

  if (currband)
    *lbn = disk_translate_pbn_to_lbn(thedisk,currband,cyl,head,sector);
  else {
    internal_error("diskinfo_lbn_address: cyl %d out of bounds!\n",cyl);
  }
#endif
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
diskinfo_track_boundaries(int cyl, int head, int *lbn1, int *lbn2) 
{
#ifndef DISKINFO_ONLY
  int r = 0;
  struct dm_pbn pbn;
  pbn.cyl = cyl;
  pbn.head = head;
  pbn.sector = 0;
  adisk->layout->dm_get_track_boundaries(adisk,&pbn,lbn1,lbn2,&r);

  if ((*lbn1 < 0) || (r != 0)) {
    internal_error("diskinfo_lbn_address: %d cyl out of bounds!\n",cyl);
  }
  /* printf("Diskmodel returned %d sectors\n",*lbn2 - *lbn1); */

#else
  disk_get_lbn_boundaries_for_track(thedisk,diskinfo_get_band(cyl),
				    cyl,head,lbn1,lbn2);
  /*
    XXX
    Disksim code returns LBN one past the last on the track so adjust it 
  */
  --(*lbn2); 
  /* printf("Disksim (old) returned %d sectors\n",*lbn2 - *lbn1); */
#endif
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
diskinfo_blks_in_zone(int lbn)
{
#ifndef DISKINFO_ONLY
  struct dm_layout_zone z;
  int i;
  for (i = 0 ; i < adisk->layout->dm_get_numzones(adisk); i++) {
    if (adisk->layout->dm_get_zone(adisk,i,&z))
      internal_error("diskinfo_random_band_cyl: error getting zoneinfo!\n");
    
    if ((lbn >= z.lbn_low) || (lbn <= z.lbn_high))
      return(z.lbn_high - z.lbn_low);
  }
  internal_error("diskinfo_blks_in_zone error!\n");
  return(0);
#else
  int cyl, head, sector;
  band_t *currband;
  disk_translate_lbn_to_pbn(thedisk,lbn,MAP_FULL,&cyl,&head,&sector);
  currband = diskinfo_get_band(cyl);
  return(disk_compute_blksinband(thedisk,currband));
#endif
}


/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
diskinfo_get_first_valid_cyl(void)
{
  int cyl,head,sector;

  diskinfo_physical_address(0,&cyl,&head,&sector);
  /* printf("First Cyl - %d\n",cyl); */
  return(cyl);
}
    
/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
diskinfo_get_last_valid_cyl(void)
{
  int cyl;
  int head,sector;

  /*  this is a hack to deal with disks where the last LBN is not on the last
      cylinder.  once the zoning code is correct for the limping head disks
      then you should be able to go through the zones and find the last
      valid cylinder more correctly.  -schlos  */

  cyl = adisk->dm_cyls - 1; 
  return cyl;

  // UNREACHED
  

#ifndef DISKINFO_ONLY
  diskinfo_physical_address(adisk->dm_sectors-1,&cyl,&head,&sector);
#else
  diskinfo_physical_address(thedisk->numblocks-1,&cyl,&head,&sector);
#endif
  /* printf("Last Cyl - %d\n",cyl); */
  return(cyl);
}
    

/*---------------------------------------------------------------------------*
 * Generates a random cylinder number between lowest cylinder and highest    *
 * cylinder minus "cushion" cylinders. The returned cylinder contains        *
 * sectors mapped to valid LBNs.                                             * 
 *---------------------------------------------------------------------------*/
static int
diskinfo_get_rand_cyl(int cushion)
{
#define MAXTRIES  50

  int lbn = -1;
  int cyl;
  int counter = 0;
  int head,sector;

  do {
    if (counter < (MAXTRIES-1)) {
      lbn = rand() % (adisk->dm_sectors);
    }

    else {  /* gave up, just use the first lbn */
      lbn = 0;
    }

    diskinfo_physical_address(lbn,&cyl,&head,&sector);

    //    printf("%s() disk LBN: %d -> [%d,%d,%d]\n", __func__, lbn,cyl,head,sector); 

    if ((cyl < adisk->dm_cyls - cushion)) {
      break;
    }

    counter++;
    if (counter> MAXTRIES) {
      internal_error("diskinfo_get_rand_cyl: tried %d times, giving up.\n",
		     MAXTRIES);
    }
  } while(1);

  return cyl;
}
#undef MAXTRIES


/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
static int
cyl_defect_condition(int cyl, int defects)
{
  int lbn,head,dummy[2],condition;

  switch (defects) {
  case NO_TRACK_DEFECT:
    lbn = diskinfo_cyl_firstlbn(cyl,0);
    diskinfo_physical_address(lbn,&dummy[0],&head,&dummy[1]);
    condition = count_defects_on_track(adisk, (uint) cyl, head);
    break;

  case NO_CYL_DEFECT:
    condition = count_defects_on_cylinder(adisk, (uint) cyl);
    break;

  default:
    condition = 0;
  }

  return condition;
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static int 
random_cyl_w_max_spt(int *sectors,int *bandnum)
{
  static int max_index = -1;
  int i,cyl,temp_sect,low,high;
  int cyl_max_sectors = 0;
  int max_sectors = 0;

  /* choose band with most sectors/track */
  if (max_index == -1) {
    low = 0;
#ifndef DISKINFO_ONLY
    high =  adisk->layout->dm_get_numzones(adisk);
#else
    high = thedisk->numbands;
#endif
  }
  else {
    low = max_index;
    high = max_index + 1;
  }

  for(i=low ; i < high ; i++) {
    cyl = diskinfo_random_band_cylinder(i,0);
    temp_sect = diskinfo_sect_per_track(cyl);

    /* choose a cylinder with max # of tracks */
    if (max_sectors < temp_sect) {
      max_sectors = temp_sect;
      cyl_max_sectors = cyl;
      max_index = i;
    }
  }
  *bandnum = max_index;
  *sectors = max_sectors;
  return(cyl_max_sectors);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
random_cylinder(int cushion, int defects, int band)
{
  int cyl,sectors,bandnum;

  do {
    switch (band) {
    case ANY_BAND:
      cyl = diskinfo_get_rand_cyl(cushion);
      break;
    case MAX_BAND:
      cyl = random_cyl_w_max_spt(&sectors,&bandnum);
      break;
    default:
      cyl = diskinfo_random_band_cylinder(band,cushion);
      /* internal_error("random_cylinder: %d is bad parameter!\n",band); */
    }
  } while(cyl_defect_condition(cyl,defects));
  return(cyl);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
void
generate_random_cylinders(int cyls[],int howmany,int defects) 
{
  /* int i,j,lbn,head,dummy[2],condition; */
  int i,j,condition;

  for(i = 0 ; i< howmany ; i++) {
    do {
      condition = 0;
      cyls[i] = diskinfo_get_rand_cyl(howmany);

      /* check for duplicate lbns */
      for(j = 0 ; j < i ; j++) 
	if (! condition) {
	  condition = (cyls[j] == cyls[i]);
	}
      condition = condition &&
	cyl_defect_condition(cyls[i],defects);
      /*
      switch (defects) {
      case NO_TRACK_DEFECT:
	lbn = diskinfo_cyl_firstlbn(cyls[i],0);
	diskinfo_physical_address(lbn,&dummy[0],&head,&dummy[1]);
	condition = condition &&
	  count_defects_on_track(defect_lists,(uint) cyls[i],head);
	break;
      case NO_CYL_DEFECT:
	condition = condition &&
	  count_defects_on_cylinder(defect_lists,(uint) cyls[i]);
	break;
      }
      */
    } while (condition);
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
diskinfo_cyl_firstlbn(int cyl, int rand_max_offset) 
{

  int currtrack = -1;
  int lbn = -1;
  int i = 0;
  int lbn2;

  int minlbn = adisk->dm_sectors;
  struct dm_pbn pbn;
  int r = 0; 

  for (i = 0; i < adisk->dm_surfaces; i++) {
    pbn.cyl = cyl;
    pbn.head = i;
    pbn.sector = 0;
    adisk->layout->dm_get_track_boundaries(adisk,&pbn,&lbn,&lbn2,&r);
    if ((lbn >= 0) && (lbn < minlbn)) {
      minlbn = lbn;
      currtrack = i;
    }
  }

  lbn = minlbn;

  if ((rand_max_offset > 0) && (lbn != -1))
    lbn += (rand() % rand_max_offset);

  ddbg_assert(lbn >= 0);

  return(lbn);
}
 
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int 
diskinfo_sect_per_track(int cyl)
{
#ifndef DISKINFO_ONLY
  int rv;
  struct dm_pbn pbn;

  // ick ... this track may not be mapped and newer (g4) layouts
  // only "know" the answer for mapped tracks, etc.

  pbn.cyl = cyl;
  pbn.head = 0;
  pbn.sector = 0;

  rv = adisk->layout->dm_get_sectors_pbn(adisk,&pbn);

  return rv;

#else
  int dummy[2],head,lbn1,lbn2;
  lbn1 = diskinfo_cyl_firstlbn(cyl,0);
  if (lbn1 == -1)
    /* the cylinder does not have any valid lbn mapping */
    return(-1);
  diskinfo_physical_address(lbn1,&dummy[0],&head,&dummy[1]);
  disk_get_lbn_boundaries_for_track(thedisk,diskinfo_get_band(cyl),
				    cyl,head,&lbn1,&lbn2);
  return(lbn2 - lbn1);
#endif
}


/*---------------------------------------------------------------------------*
 * Generates a random cylinder number between lowest cylinder and highest    *
 * cylinder minus "cushion" cylinders.                                       * 
 *---------------------------------------------------------------------------*/
int
diskinfo_random_band_cylinder(int bandnum, int cushion)
{
#define MAX_RETRIES 20

  struct dm_pbn p;
  struct dm_layout_zone z;
  int lbn, rv;

  if (bandnum >= adisk->layout->dm_get_numzones(adisk)) {
    internal_error("diskinfo_random_band_cyl: %d bandnum invalid!\n",
		   bandnum);
  }

  if (adisk->layout->dm_get_zone(adisk,bandnum,&z)) {
    internal_error("diskinfo_random_band_cyl: error getting zoneinfo!\n");
  }

  // pick a random lbn in the zone in question, translate it and use
  // that cyl

  lbn = (rand() % (z.lbn_high - z.lbn_low + 1)) + z.lbn_low;
  rv = adisk->layout->dm_translate_ltop(adisk, lbn, MAP_FULL, &p, 0);
  ddbg_assert(rv == DM_OK);


  return lbn == -1 ? -1 : p.cyl;
} 
#undef MAX_RETRIES

/*---------------------------------------------------------------------------*
 * TEMPORARY function for the transition between diskinfo and dm             * 
 *---------------------------------------------------------------------------*/
#ifndef DISKINFO_ONLY
#else
band_t* 
diskinfo_get_band(int cyl) 
{
  int i = 0;
  do {
    if ((thedisk->bands[i].startcyl <= cyl) &&
	(thedisk->bands[i].endcyl >= cyl)) 
      break;
    i++;
  } while (i<thedisk->numbands);

  if (i<thedisk->numbands) {
    return(&thedisk->bands[i]);
  }
  else {
    return(NULL);
  }
}
#endif

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
get_disk_label(char *label)
{
  char product[SID_PRODUCT_SIZE+1];
  char vendor[SID_VENDOR_SIZE+1];
  char serialnum[SVPD_SERIAL_NUM_SIZE+1];
  char revision[SID_REVISION_SIZE+1];

  memset(product, 0, sizeof(product));
  memset(vendor, 0, sizeof(vendor));
  memset(revision, 0, sizeof(revision));
  memset(serialnum, 0, sizeof(serialnum));

  get_detailed_disk_label(product, vendor, revision, serialnum);
  sprintf (label,"%s model %s revision %s sn %s",
	   vendor, product, revision, serialnum);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/


// Product, vendor and serno should be 1 byte larger than the 
// values in the inquiry data (0-term) and already zeroed.
void 
get_detailed_disk_label(char *product,
			char *vendor,
			char *rev,
			char *serno)
{
  int i;


  struct scsi_inquiry_data *sid;
  struct scsi_vpd_unit_serial_number *svpd_serial;

  exec_scsi_command(scsi_buf, SCSI_inq_command());
  sid = (struct scsi_inquiry_data *)scsi_buf;

  if(vendor) {
    memcpy(vendor, sid->vendor, sizeof(sid->vendor));
    for (i = 0; i < SID_VENDOR_SIZE; i++) {
      if (vendor[i] < ' ') {
	vendor[i] = 0;
      }
    }
  }

  if(product) {
    memcpy(product, sid->product, sizeof(sid->product));
    for (i = 0; i < SID_PRODUCT_SIZE; i++) {
      if (product[i] < ' ') {
	product[i] = 0;
      }
    }
  }

  // revision
  if(rev) {
    memcpy(rev, sid->revision, sizeof(sid->revision));
    for (i = 0; i < SID_PRODUCT_SIZE; i++) {
      if (rev[i] < ' ') {
	rev[i] = 0;
      }
    }
  }

  // exec_scsi_command(scsi_buf, SCSI3_inq_command(SVPD_UNIT_SERIAL_NUMBER));
  // svpd_serial = (struct scsi_vpd_unit_serial_number *)scsi_buf;
  svpd_serial = (struct scsi_vpd_unit_serial_number *) (scsi_buf+32);

  assert(svpd_serial->length <= SVPD_SERIAL_NUM_SIZE);

  memcpy(serno, svpd_serial->serial_num, svpd_serial->length);

  for (i = 0; i < SVPD_SERIAL_NUM_SIZE; i++) {
    if(serno[i] < ' ') {
      serno[i] = 0;
    }
  }

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
get_geometry(int *cyls, int *rot, int *heads, int *rot_off)
{
  u_int8_t p_offset,block_offset = 4; 
  exec_scsi_command(scsi_buf, SCSI_mode_sense(GEOM_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  *cyls =_3btol ((u_int8_t *) &scsi_buf[p_offset+2]);
  *rot = _2btol ((u_int8_t *) &scsi_buf[p_offset+20]);
  *heads = (u_int8_t) scsi_buf[p_offset+5];
  *rot_off = (u_int8_t) scsi_buf[p_offset +18];
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
get_capacity(int *maxlbn,int *bsize) 
{
  exec_scsi_command(scsi_buf,SCSI_read_capacity(0,0));
  if (maxlbn)
      *maxlbn = _4btol ((u_int8_t *) &scsi_buf[0]);
  if (bsize)
      *bsize = _4btol ((u_int8_t *) &scsi_buf[4]);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
write_formatted_line(FILE *fp,char *label,char *value,char delimeter) 
{
#define COLUMN    35
#define MAXLINE  255
  char padding[MAXLINE];
  /* char line[MAXLINE]; */
  int i=0;
  if (strlen(label) < COLUMN)
    for(;i<(COLUMN-strlen(label));i++)
      padding[i] = ' ';

  padding[i] = '\0';
  /* sprintf(line,"%s:%s%s\n",label,padding,value); */
  /* write(fd,line,strlen(line)); */
  fprintf(fp,"%s%c%s%s\n",label,delimeter,padding,value);
}
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
write_entry(FILE *fp,char *label,char *value) 
{
  write_formatted_line(fp,label,value,':');
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
write_parfile_entry(FILE *fp,char *label,char *value) 
{
  write_formatted_line(fp,label,value,' ');
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_physical_address(int lbn,int *cyl,int *head,int *sector) 
{
  int len;
/*   if ((lbn >= 0) && (lbn <= maxlbn)) { */

  len = create_lba_to_phys_page(scsi_buf,lbn);
  exec_scsi_command(scsi_buf,SCSI_send_diag_command(len));
  exec_scsi_command(scsi_buf,SCSI_recv_diag_command(len));

  *cyl = _3btol((u_int8_t *) &scsi_buf[6]); 
  *head = (unsigned int) scsi_buf[9];
  *sector = _4btol((u_int8_t *) &scsi_buf[10]);

#ifdef ADDR_TRANS_COUNTER
  transaddr_cntr++;
#endif

/*   }  */
/*   else { */
/*     *cyl = -1; */
/*     *head = -1; */
/*     *sector = -1; */
/*   } */
  
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_lbn_address(int cyl, int head, int sector, int *lbn) 
{
  int len;
  len = create_phys_to_lba_page(scsi_buf,cyl,head,sector);
  exec_scsi_command(scsi_buf,SCSI_send_diag_command(len));
  exec_scsi_command(scsi_buf,SCSI_recv_diag_command(len));
  *lbn = _4btol((u_int8_t *) &scsi_buf[6]);
}

/*---------------------------------------------------------------------------*
 * Sets the active notch on the disk to "notch" and returns 0. If drive is   *
 * not notched, returns -1.                                                  * 
 *---------------------------------------------------------------------------*/
int
set_notch_page(int notch)
{
#ifndef SILENT
  static int print = 1;
#endif
  int datalen,p_offset,block_offset = 4;
  u_int8_t page_code;

  /* check if NOTCH_PAGE defined at all */
  /* First send a DISCON_RECON_PAGE, then send MODE SENSE for NOTCH_PAGE  */
  /* if we got the old data (works for the HPC 2247), no NOTCH PAGE       */
  exec_scsi_command(scsi_buf,SCSI_mode_sense(DISCON_RECON_PAGE));
  p_offset = block_offset + (u_int8_t) scsi_buf[3];
  page_code=(u_int8_t) (scsi_buf[p_offset] & 0x3F);

  exec_scsi_command(scsi_buf,SCSI_mode_sense(NOTCH_PAGE));

  if (page_code == ((u_int8_t) (scsi_buf[p_offset] & 0x3F))) {
    /* the drive does not implement NOTCH PAGE */
#ifndef SILENT
    if (print)
      printf("set_notch_page: Disk does not implement NOTCH PAGE!\n");
    print = 0;
#endif
  }
  datalen = (u_int8_t) scsi_buf[0]+1;

  if (scsi_buf[p_offset+2] & 0x80) {
    /* set the mode data length to zero */
    //    scsi_buf[0] = 0;
    
    /* set active notch to 1 */
    _lto2b (notch, (u_int8_t *)&scsi_buf[p_offset+6]);
    
    /* | 0x40 to set LPN bit, &~0x40 to clear */
    scsi_buf[p_offset+2] = scsi_buf[p_offset+2] &~0x40;
    
    exec_scsi_command(scsi_buf,SCSI_mode_select(datalen));
    return(0);
  }
  else {
#ifndef SILENT    
    printf("set_notch_page: Disk is not notched!\n");
#endif
    return(-1);
  }
}


void dump_notches(void) {
  int notches = 0, i;
  int p_offset,block_offset = 4;
  int cmds;
  // get the number of notches

  exec_scsi_command(scsi_buf,SCSI_mode_sense(NOTCH_PAGE));
  p_offset = block_offset + (u_int8_t) scsi_buf[3];  

  print_scsi_mode_sense_data (scsi_buf);
  notches = _2btol((u_int8_t *) &scsi_buf[p_offset + 4]);

  printf("dump_notches: got %d notches\n", notches);

  printf("pages: ");
  for(cmds = 16; cmds <= 23; cmds++) {
    uint8_t cmdi = scsi_buf[p_offset + cmds];
    printf("%.2hhx ", cmdi);
  }
  printf("\n");

  // fetch each notch page
  for(i = 1; i <= notches; i++) {
    uint8_t b2;
    int lpn;
    int spt, ts, cs,  active;
    set_notch_page(i);
    
    exec_scsi_command(scsi_buf,SCSI_mode_sense(NOTCH_PAGE));
    p_offset = block_offset + (u_int8_t) scsi_buf[3];  
    b2 = scsi_buf[p_offset+2];
    lpn = b2 & 0x40;
    active = _2btol((u_int8_t *) &scsi_buf[p_offset+6]);

    printf("notch %d active %d %.2hhx ", i, active, b2);

    if(lpn) {
      int start, end;
      start = _4btol((u_int8_t *) &scsi_buf[p_offset+8]);
      end = _4btol((u_int8_t *) &scsi_buf[p_offset+12]);
      printf("start %d end %d\n", start, end);
    }
    else {
      int c0, ci, h0, hi;
      c0 = _3btol((u_int8_t *) &scsi_buf[p_offset+8]);
      h0 = scsi_buf[p_offset+11];
      ci = _3btol((u_int8_t *) &scsi_buf[p_offset+12]);
      hi = scsi_buf[p_offset+15];
      printf("start (%d,%d) end (%d,%d)\n", c0, h0, ci, hi);
    }
    

    exec_scsi_command(scsi_buf, SCSI_mode_sense(FORMAT_PAGE));
    p_offset = (u_int8_t) scsi_buf[3] + block_offset;

    spt = _2btol((u_int8_t *) &scsi_buf[p_offset + 10]);
    ts =  _2btol((u_int8_t *) &scsi_buf[p_offset + 16]);
    cs =  _2btol((u_int8_t *) &scsi_buf[p_offset + 18]);
    printf("notch %d spt %d ts %d cs %d\n", i, spt, ts, cs);
    

  }

}

/*---------------------------------------------------------------------------*
 * Returns the bulk transfer time in miliseconds. It is the time it takes to *
 * transfer 512 bytes of data from the disk's cache to the host. This time   *
 * includes the SCSI bus transfer, adapter access time etc.                  *
 *---------------------------------------------------------------------------*/
float
get_bulk_transfer_time(void) 
{
  int lbn;
  float a,b;
  char orig_bits;
  
  orig_bits = set_cache(READ_CACHE_ENABLE);
  lbn = diskinfo_cyl_firstlbn(random_cylinder(1,NO_TRACK_DEFECT,ANY_BAND),0);
  decompose_overheads(&a,&b,B_READ,lbn,B_READ,lbn);
  
  set_cache(orig_bits);

  // printf("get_bulk_transfer: %.5f\n",a);
  /* return the time in miliseconds */
  return ( a/1000 );

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
decompose_overheads(float *a,   // result
		    float *b,   // result
		    int c1,     // rw
		    int lbn1,
		    int c2,     // rw
		    int lbn2)
{
#define MAX_SIZE   512
// #define MAX_POWER   8
#define MAX_POWER MAX_SIZE
#define OV_SETS     1
#define OV_SAMPLES  1

  extern long ustop,ustart; 
  int i, j, numblocks;
  float x[MAX_POWER+2], t[MAX_POWER+2], dummy[MAX_POWER+2];
  long set_a[OV_SETS][OV_SAMPLES], set_b[OV_SETS][OV_SAMPLES];

  for(i = 0; i < OV_SETS; i++) {
    //    for(j = 0; j <= MAX_POWER; j += 1) {
    for(j = 1; j <= MAX_SIZE; j++) {

      /* read into cache the track, to make sure the entry isn't
       * erased, once it is read from the cache  */
      //      numblocks = pow(2.0,(double) MAX_POWER);
      numblocks = j;
      exec_scsi_command(scsi_buf,
			SCSI_rw_command(c1,lbn1,numblocks));

      /* printf("Time to read LBN %d into the cache: %ld usec\n",
       *        lbn,(ustop-ustart));  */

      /* now read it from the cache */
      //      numblocks = pow(2.0,(double) j);
      exec_scsi_command(scsi_buf,
			SCSI_rw_command(c2,lbn2,numblocks));

      //      printf("\t Time to read from cache %3d blocks: %6ld usec\n",
      //            numblocks,(ustop-ustart));

      /* printf("\t\t%d\t%ld usec\n",numblocks,(ustop-ustart)); */
      t[j+1] = (float) (ustop-ustart);
      x[j+1] = (float) numblocks;
    }

    fit(x,t,(j-0),dummy,0,b,a,&dummy[0],&dummy[1],&dummy[2],&dummy[3]);
    set_b[0][i] = (long) *b;
    set_a[0][i] = (long) *a;
    // printf("Best fit a = %f\tb = %f\n",*a,*b);
  }

  find_mean_max(OV_SETS,OV_SAMPLES,&set_a[0][0],&dummy[0],a,0);
  find_mean_max(OV_SETS,OV_SAMPLES,&set_b[0][0],&dummy[0],b,0);

  /* printf("Mean fit a = %f\tb = %f\n",*a,*b); */

#undef OV_SETS
#undef OV_SAMPLES
#undef MAX_POWER
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static float
skew_in_band(int bandnum,int whatskew)
{
#define SKEW_SAMPLES     25
#define SKEW_SETS        2

  int i,j;
  int cyl,lbn1,lbn2,head1,sectors;
  long tset[SKEW_SETS][SKEW_SAMPLES];
  long ireq_dist[SKEW_SETS][SKEW_SAMPLES];
  float max_irdist,mean_irdist;
  extern long ustop,ustart;

  for(j = 0; j < (SKEW_SETS); j++) {
    for(i = 0; i < (SKEW_SAMPLES); i++) {
      if (whatskew == CYLINDER_SKEW) {
	int dummy;

	do {
	  cyl = random_cylinder(3,NO_CYL_DEFECT,bandnum);

	  // cyl = random_cylinder(3,ANY_DEFECT,bandnum);

	  lbn2 = diskinfo_cyl_firstlbn(cyl+1,0);
	} while((cyl == -1 || lbn2 == -1 || lbn2 == 0));

	lbn1 = lbn2 - 1;
	diskinfo_physical_address(lbn1,&cyl,&head1,&sectors);
	diskinfo_track_boundaries(cyl,head1, &lbn1, &dummy);
      }
      else { /* TRACK_SKEW */
	cyl = random_cylinder(30, NO_CYL_DEFECT, bandnum);
	lbn1 = diskinfo_cyl_firstlbn(cyl,0);
	diskinfo_physical_address(lbn1,&cyl,&head1,&sectors);
	diskinfo_track_boundaries(cyl,head1, &lbn1, &lbn2);
	lbn2++;
      }

#ifdef DEBUG_SKEW
      get_physical_address(lbn1,&cyl,&head1,&sectors);
      printf("          LBN1: %d [%d,%d,%d]\n",lbn1,cyl,head1,sectors);
      get_physical_address(lbn2,&cyl,&head2,&sectors);
      printf("          LBN2: %d [%d,%d,%d]\n",lbn2,cyl,head2,sectors);
#endif

      exec_scsi_command(scsi_buf,SCSI_rw_command(B_WRITE,lbn1,1));
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_WRITE,lbn2,1));
      tset[j][i] = ustop - ustart;

      if (tset[j][i] > ONE_REVOLUTION) 
	tset[j][i] -= ONE_REVOLUTION;

      sectors = diskinfo_sect_per_track(cyl);
      ireq_dist[j][i] = tset[j][i] * sectors / ONE_REVOLUTION;

#ifdef DEBUG_SKEW
      printf("   Time: %ld\t dist: %ld (sectors:%d)\n",tset[j][i],
	     ireq_dist[j][i],sectors); 
#endif

    }
  }

  find_mean_max(SKEW_SETS,SKEW_SAMPLES,&ireq_dist[0][0],&max_irdist,&mean_irdist,0);

  return(mean_irdist);

#undef SKEW_SAMPLES
#undef SKEW_SETS
}

/*---------------------------------------------------------------------------*
 * Retrieves information about track and cylinder skew from the disk and     *
 * stores the information into the diskinfo structure.                       * 
 *---------------------------------------------------------------------------*/
void
get_skew_information(void(*skewout)(void *, int, float, float), 
		     void *skewout_ctx)
{

#ifndef SILENT
  // #define  PRINT_SKEW_INFO
#endif
  // #define SKEW_FROM_NOTCH_PAGE 

#define ENTIRE_DISK_NOTCH    0

#define SMOOTH_SKEW          1
#define SKEW_MAXVAL          0

#ifdef SKEW_FROM_NOTCH_PAGE
  int p_offset,block_offset = 4;
  int active_notch;
  int notched_disk = 0;
  int sect_per_track,tracks_per_zone,bytes_per_sect;
  int interleave;
#endif

  int i,max_notches;
  float trackskew,cylskew;

#ifdef SMOOTH_SKEW
  int sectors;
  int last_sectors = SKEW_MAXVAL; 
  int last_trackskew = SKEW_MAXVAL;
  int last_cylskew = SKEW_MAXVAL;
#endif

#undef SKEW_MAXVAL

#ifdef SKEW_FROM_NOTCH_PAGE
  notched_disk = (set_notch_page(ENTIRE_DISK_NOTCH) != -1);

  if (notched_disk) {
    exec_scsi_command(scsi_buf,SCSI_mode_sense(NOTCH_PAGE));
    p_offset = block_offset + (u_int8_t) scsi_buf[3];
    max_notches = _2btol (&scsi_buf[p_offset+4]);
    active_notch = _2btol (&scsi_buf[p_offset+6]);
  }
  else
#endif


    max_notches = adisk->layout->dm_get_numzones(adisk);

  /* print_scsi_mode_sense_data (scsi_buf); */

#ifdef PRINT_SKEW_INFO
    printf("Number of bands: %d\n",max_notches);
#endif

   for(i = 0; i < max_notches; i++) {
#ifdef SKEW_FROM_NOTCH_PAGE
    if (notched_disk)
      set_notch_page(i+1);
    
    exec_scsi_command(scsi_buf,SCSI_mode_sense(FORMAT_PAGE));
    p_offset = block_offset + (u_int8_t) scsi_buf[3];
    
    print_scsi_mode_sense_data (scsi_buf);

    tracks_per_zone = _2btol (&scsi_buf[p_offset+2]);
    sect_per_track = _2btol (&scsi_buf[p_offset+10]);
    bytes_per_sect = _2btol (&scsi_buf[p_offset+12]);
    interleave = _2btol (&scsi_buf[p_offset+14]);
    trackskew = _2btol (&scsi_buf[p_offset+16]);
    cylskew = _2btol (&scsi_buf[p_offset+18]);
#else

#ifdef PRINT_SKEW_INFO
    printf("Band #%d\n",i+1);
#endif

    trackskew = skew_in_band(i, TRACK_SKEW);

#ifdef SMOOTH_SKEW
    sectors = diskinfo_sect_per_track(diskinfo_random_band_cylinder(i,0));
    if ((i > 0)  && (last_trackskew < trackskew) && (sectors <= last_sectors))
      trackskew = last_trackskew;
    /* printf(" (%d)",sectors); */
#endif

#ifdef PRINT_SKEW_INFO
    printf("\tTrack Skew: %f\n",trackskew);
#endif

    cylskew = skew_in_band(i, CYLINDER_SKEW);

#ifdef SMOOTH_SKEW
    if ((i > 0) && (last_cylskew < cylskew) && (sectors <= last_sectors))
      cylskew = last_cylskew;
    last_sectors = sectors;
    last_trackskew = trackskew;
    last_cylskew = cylskew;
#endif

#ifdef PRINT_SKEW_INFO
    printf("\tCylinder Skew: %f\n",cylskew);
#endif

#endif  // # else (SKEW_FROM_NOTCH_PAGE)


    // do something with the values
    skewout(skewout_ctx, i, cylskew, trackskew);
  }

#ifdef SKEW_FROM_NOTCH_PAGE
  if (notched_disk)
    set_notch_page(ENTIRE_DISK_NOTCH);
#endif
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_water_marks(float *lw, float *hw, int *set_by_reqsize) 
{
  int p_offset,block_offset = 4;

  p_offset = block_offset + (u_int8_t) scsi_buf[3];

  exec_scsi_command(scsi_buf,SCSI_mode_sense(DISCON_RECON_PAGE));
  *hw = (float) (((u_int8_t) scsi_buf[p_offset+2]) /256);
  *lw = (float) (((u_int8_t) scsi_buf[p_offset+3]) /256);

  /* !!! recognize this */
  *set_by_reqsize = 1;
}

/*---------------------------------------------------------------------------*
 * Returns the time of one revolution in miliseconds                         * 
 *---------------------------------------------------------------------------*/
float 
get_one_rev_time(void)
{
  int nominal_rot,dummy[3];
  static float one_rev = 0.0;

  if (!one_rev) {
    get_geometry(&dummy[0],&nominal_rot,&dummy[1],&dummy[2]);
    one_rev = 60000.0 / (float) (nominal_rot); 

    /*  thedisk->rotatetime = one_rev; */

  }
  return(one_rev);
}

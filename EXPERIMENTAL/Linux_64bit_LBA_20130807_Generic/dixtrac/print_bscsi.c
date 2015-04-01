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
#include "build_scsi.h"
#include "print_bscsi.h"

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_recv_diag (char *buf)
{
  int i;
  u_int8_t page_code = buf[0];
  uint plength = _2btol((u_int8_t *) &buf[2]);
  switch(page_code) {
  case 0:
    printf ("   Supported pages: %d ",plength);
    for(i=0;i<plength;i++) {
      printf(" 0x%02X ",buf[4+i]);
    }
    printf("\n\n");
    break;
  case TRANS_ADDR_PAGE:
    /* printf ("Translate Address (page legth: 0x%02X)\n",(plength+4)); */
    printf("\tCylinder: %5d\tHead: %2d\tSector:%3d",
	   _3btol((u_int8_t *) &buf[6]),
	   (unsigned int) buf[9],
	   _4btol((u_int8_t *) &buf[10]));
    printf("\n");
    break;
  }
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_read_capacity (char *buf)
{
  printf ("Read Capacity Command:\n");

  printf("\tLast LBN: %d\n", _4btol((u_int8_t *) &buf[0]));
  printf("\tBlock size: %d\n", _4btol((u_int8_t *) &buf[4]));
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_inquiry_data (char *buf,char *devname)
{
  int i;
  char product[17],manufacturer[9],revision[6],sernum[26];
   
  // printf ("Disk %d (%s%c): ",diskno,DEVNAME,'a'+diskno);
  printf ("Disk (%s): ",devname);
  memset(manufacturer,0,9);
  for (i = 0;i <8 ; i++) {
    if ((u_int8_t) buf[8+i] != 32) 
      manufacturer[i]=(u_int8_t) buf[8+i];
    else 
      manufacturer[i]='\0';
  }
  printf ("%s ",manufacturer);
  memset(product,0,17);
  for (i = 0;i <16 ; i++) {
    if ((u_int8_t) buf[16+i] != 32)
      product[i]=(u_int8_t) buf[16+i];
    else 
      product[i]='\0';
  }
  printf ("%s",product);
  for (i = 0 ; i < 4 ; i++) 
    revision[i]=(u_int8_t) buf[32+i];
  revision[4]='\0';

  memset(sernum,0,26);
  for (i = 0;i <11 ; i++) {
    sernum[i]=(u_int8_t) buf[36+i];
  }

  printf (", rev. %s, ",revision);
  switch ((u_int8_t) buf[2] & 0x07) {
  case 0:
    printf("SCSI-1");     break;
  case 1:
    printf("SCSI-1 w. CCS");     break;
  case 2:
    printf("SCSI-2");     break;
  case 3:
    printf("SCSI-3");     
  }
  printf (", SN:%s",sernum);
  printf ("\n");
  
  /*
    printf ("peripheral qualifier: %02X\n",(u_int8_t) buf[0] & 0xE0);
    printf ("device class: %02X\n",(u_int8_t) buf[0] & 0x10);
    printf ("aditional length: %02X\n",(u_int8_t) buf[4]);
    */
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_request_sense (char *buf) {

  int error_code,sense_key;
  char reason[100];

  error_code = 0x7F & (u_int8_t) buf[0] ;
  if (error_code == 0x71) {
    /* previous command, the last one didn't fail */
    strcpy(reason,"some command in the past");
   } 
   else { 
     strcpy(reason,"the last command");
   }
  /* error_code == 0x70 refers to the current command */
  sense_key = 0x0F & (u_int8_t) buf[2] ;

  printf("Request Sense for %s :",reason);
  switch (sense_key) {
  case 0:
    printf("no sense");     break;
  case 1:
    printf("recovered error");     break;
  case 2:
    printf("not ready");     break;
  case 3:
    printf("medium error");     break;
  case 4:
    printf("hardware error");     break;
  case 5:
    printf("illegal request");     break;
  case 6:
    printf("unit attention");     break;
  case 7:
    printf("data protect");     break;
  case 8:
    printf("blank check");     break;
  case 0xA:
    printf("copy aborted");     break;
  case 0xB:
    printf("command aborted");     break;
  case 0xC:
    printf("equal");     break;
  case 0xD:
    printf("volume overflow");     break;
  case 0xE:
    printf("miscompare");     break;
  default:
    printf("sense key %02X",sense_key);     
  }
  printf ("\n");
  
  /*
    printf ("peripheral qualifier: %02X\n",(u_int8_t) buf[0] & 0xE0);
    printf ("device class: %02X\n",(u_int8_t) buf[0] & 0x10);
    printf ("aditional length: %02X\n",(u_int8_t) buf[4]);
    */
}


/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_mode_sense_data (char *buf)
{
  u_int8_t block_offset = 4;
  u_int8_t p_offset = (u_int8_t) buf[3] + block_offset;
  u_int8_t page_code;

  u_int32_t cyls = 0;
  u_int32_t rot = 0;

  /* printf("Mode data length: 0x%02X\n",(u_int8_t) buf[0]); */
/*   printf("Block descriptor length (page offset): 0x%02X\n",p_offset); */

  page_code=(u_int8_t) (buf[p_offset] & 0x3F);
  /* printf("Page length: 0x%02X\n",(u_int8_t)buf[p_offset+1]); */
  printf("Mode parameter page: 0x%02X",page_code);
  switch (page_code) {
  case DISCON_RECON_PAGE: 
    printf(" (Disconnect-Reconnect page)\n");
    printf("\tBuffer Full Ratio: %d - %d%%\n",
	   ((u_int8_t) buf[p_offset+2]),
	   ((u_int8_t) buf[p_offset+2])*100 /256);
    printf("\tBuffer Empty Ratio: %d - %d%%\n",
	   ((u_int8_t) buf[p_offset+3]),
	   ((u_int8_t) buf[p_offset+3])*100 /256);
    printf("\tBus Inactivity Limit: %d*100 usec\n",
	   _2btol((u_int8_t *) &buf[p_offset+4]));
    printf("\tDisconnect Time Limit: %d*100 usec\n",
	   _2btol((u_int8_t *) &buf[p_offset+6]));
    printf("\tConnect Time Limit: %d*100 usec\n",
	   _2btol((u_int8_t *) &buf[p_offset+8]));
    printf("\tMaximum Burst Size: %d*512 bytes (%s)\n",
	   _2btol((u_int8_t *) &buf[p_offset+10]),
	   ((_2btol((u_int8_t *) &buf[p_offset+10]) == 0) ? "No limit" : ""));
    printf("\tEnable MODIFY DATA POINTER: %d\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x80))>>7));
    printf("\tFair priority arbitration for read: %d\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x40))>>6));
    printf("\tFair priority arbitration for write: %d\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x20))>>5));
    printf("\tFair priority arbitration for status control: %d\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x10))>>4));
    printf("\tDisconnect Immediate: %d\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x08))>>3));
    printf("\tData Transfer Disconnect Control: 0x%02X\n",
	   (((u_int8_t) (buf[p_offset+12] & 0x07))));
    break;

  case CACHE_PAGE: 
    printf(" (Cache page)\n");
    printf("\tWrite Cache Enable (WCE):   %d\n",
	   ((u_int8_t) (buf[p_offset+2] & 0x04))>>2);
    printf("\tMultiplication Factor (MF): %d\n",
	   ((u_int8_t) (buf[p_offset+2] & 0x02))>>1);
    printf("\tRead Cache Disable (RCD):   %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x01)));
    printf("\n");
    printf("\tInitiator Control (IC): %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x80))>>7);
    printf("\tAbort Prefetch (ABPF):  %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x40))>>6);
    printf("\tCaching Analysis (CAP): %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x20))>>5);
    printf("\tDiscontinuity (DISC):   %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x10))>>4);
    printf("\tSize Enable (SIZE):     %d\n",
    	   ((u_int8_t) (buf[p_offset+2] & 0x08))>>3);

    printf("\tDemand Read Retention Priority:   %d\n",
    	   ((u_int8_t) (buf[p_offset+3] & 0xF0))>>4);
    printf("\tDemand Write Retention Priority:  %d\n",
    	   ((u_int8_t) (buf[p_offset+3] & 0x0F)));
    printf("\tDisable Prefetch Transfer Length: %d\n",
    	   ((u_int16_t) (buf[p_offset+4])));
    printf("\tMinimum Prefetch: %d\n",
    	   ((u_int16_t) (buf[p_offset+6])));
    printf("\tMaximum Prefetch: %d\n",
    	   ((u_int16_t) (buf[p_offset+8])));
    printf("\tPrefetch Ceiling: %d\n",
    	   ((u_int16_t) (buf[p_offset+10])));
    printf("\tNumber of Cache Segments: %d\n",
    	   ((u_int8_t) (buf[p_offset+13])));
    printf("\tCache Segment Size:       %d\n",
    	   ((u_int16_t) (buf[p_offset+14])));
    printf("\tNon-Cache Segment Size:   %d\n",
    	   ((u_int16_t) (buf[p_offset+17])));
    printf("\n");
    printf("\tForce Sequential Write (FSW):     %d\n",
    	   ((u_int8_t) (buf[p_offset+12] & 0x80))>>7);
    printf("\tCache Segment Size (LBCSS):       %d (%s)\n", 
	   ((u_int8_t) (buf[p_offset+12] & 0x40))>>6,
    	   ((((u_int8_t) (buf[p_offset+12] & 0x40))>>6) == 1 ? "in sectors" :
	    "in bytes"));
    printf("\tDisable Read-Ahead (DRA):         %d\n",
    	   ((u_int8_t) (buf[p_offset+12] & 0x20))>>5);
    break;
  case NOTCH_PAGE: 
    printf(" (Notch page)\n");
    printf("\tLPN bit: %d\n",((((buf[p_offset+2]) & 0x40) != 0) ? 1 : 0));
    printf("\tMax # of notches: %d\n",_2btol((u_int8_t *) &buf[p_offset+4]));
    printf("\tActive notch: %d\n",_2btol((u_int8_t *) &buf[p_offset+6]));

    if (buf[p_offset+2] &0x40) { /* LPN bit set */
      printf("\tStarting boundary - \tLBN: %d\n", _4btol((u_int8_t *) &buf[p_offset+8]));
      printf("\tEnding boundary -\tLBN: %d\n", _4btol((u_int8_t *) &buf[p_offset+12]));
    }
    else {
      printf("\tStarting boundary -\tCylinder: %d\t Head: %d\n",
	     _3btol((u_int8_t *) &buf[p_offset+8]), (u_int8_t) buf[p_offset+11]);
      printf("\tEnding boundary -\tCylinder: %d\t Head: %d\n",
	     _3btol((u_int8_t *) &buf[p_offset+12]), (u_int8_t) buf[p_offset+15]);
    }
    break;

  case FORMAT_PAGE: 
    printf(" (Format page)\n");
    printf("\tTracks per zone: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+2]));
    printf("\tReplacement sectors per zone: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+4]));
    printf("\tReplacement tracks per zone: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+6]));
    printf("\tSectors per track: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+10]));
    printf("\tBytes per sector: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+12]));
    printf("\tInterleave: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+14]));
    printf("\tTrack skew: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+16]));
    printf("\tCylinder skew: %d\n",
	   _2btol((u_int8_t *) &buf[p_offset+18]));
    break;
  case GEOM_PAGE: 
    printf(" (Geometry page)\n");
    cyls =_3btol((u_int8_t *) &buf[p_offset+2]);
    rot = _2btol((u_int8_t *) &buf[p_offset+20]);
    printf("\t# of cylinders: %d\n",cyls);
    printf("\t# of heads: %d\n",(u_int8_t) buf[p_offset+5]);
    printf("\tRotation: %d rpm \n",rot);
    printf("\tRotational offset: %d\n",(u_int8_t) buf[p_offset +18]);
    break;
  default:
    printf("\n");
  }
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_hexpage(char *p,int len) 
{
  int i;
  printf("Byte: ");
  for (i=0;i<len;i++) 
    printf("%2d ",i);
  printf("\n");
  printf("------");
  for (i=0;i<len;i++) 
    printf("---");
  printf("\n");
  printf("Data: ");
  for (i=0;i<len;i++) 
    printf("%02X ",(u_int8_t) p[i]);
  printf("\n\n");
}


/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void 
print_scsi_defect_data (char *buf)
{
   struct scsi_defect_data_header *header = 
       (struct scsi_defect_data_header *) buf;
   struct scsi_defect_desc_phys_sector *list = 
       (struct scsi_defect_desc_phys_sector *) &buf[sizeof(struct scsi_defect_data_header)];
   uint listlen;
   uint format;
   int i;

#ifdef USE_READ_DEFECT_DATA_10
   listlen = _2btol((u_int8_t *) &header->length[0]);
   listlen  = listlen >> 3;
#else
   listlen = _4btol((u_int8_t *) &header->length[0]);
#endif

   if ((header->format & 0x10) && (header->format & 0x08)) {
      printf ("Defect List (%d entries from both Primary and Grown lists):\n", listlen);
   } else if (header->format & 0x10) {
      printf ("Defect List (%d entries from Primary list only):\n", listlen);
   } else if (header->format & 0x08) {
      printf ("Defect List (%d entries from Grown list only):\n", listlen);
   } else {
      printf ("Defect List -- neither Primary nor Grown list is present\n");
      return;
   }


  
   /* OK to use SRDDH10 since both 10 and 12 byte versions are the same */
   format = header->format & SRDDH10_DLIST_FORMAT_MASK;
   if ((format != SRDDH10_BYTES_FROM_INDEX_FORMAT) && 
       (format != SRDDH10_PHYSICAL_SECTOR_FORMAT)) 
   {
       printf ("Unknown defect list format (%d) -- can't print it\n", format);
       return;
   }

   for (i=0; i<listlen; i++) {
      if (format == 0x5) {
         printf("\tCyl: %4d \tHead: %2d \tSector: %4d\n", _3btol((u_int8_t *) &list[i].cylinder[0]), (uint) list[i].head, _4btol((u_int8_t *) &list[i].sector[0]));
      } else {
         printf("\tCyl: %4d \tHead: %2d \tByteOff: %4d\n", _3btol((u_int8_t *) &list[i].cylinder[0]), (uint) list[i].head, _4btol((u_int8_t *) &list[i].sector[0]));
      }
   }
}


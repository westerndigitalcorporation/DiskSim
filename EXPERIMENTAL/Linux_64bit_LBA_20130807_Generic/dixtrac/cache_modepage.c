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
#include "print_bscsi.h"
#include "handle_error.h"

extern char *scsi_buf;

/*---------------------------------------------------------------------------*
 * Turns off the read and write disk caches.                                 * 
 *---------------------------------------------------------------------------*/
void 
disable_caches(void) 
{
  set_cache(READ_CACHE_DISABLE | WRITE_CACHE_DISABLE);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
enable_caches(void) 
{
  set_cache(READ_CACHE_ENABLE | WRITE_CACHE_ENABLE);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
char
set_cache(char bits)
{
  char b[2];
  char m[2];
  b[0] = bits;
  m[0] = 0x05;
  set_cache_bytes(2,b,m,1);
  /* printf("Original bits: 0x%02X\n",b[0]); */
  return(b[0]);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
enable_prefetch(void)
{
  set_cache_bit(CACHE_DISC,1);
  set_cache_bit(CACHE_DRA,0);
}

/*---------------------------------------------------------------------------*
 * Tests if the specified bytes are savable in the Cache Mode Page. Returns  *
 * 1 if the bytes are savable, otherwise returns 0. The function tests this  *
 * by setting 'count' bytes, starting at byte 'bnum' to the bits specified   *
 * in the 'bits' array. A 'mask' is applied to the bytes (0xFF mask applies  *
 * the bits at their value). If the Cache Mode Page contains the same        *
 * pattern as in bits then bits are negated first and them applied.          *
 *---------------------------------------------------------------------------*/
int 
cache_bytes_savable(uint bnum,char bits[],char mask[],uint count)
{
  char b[MAX_MODE_PAGE_SIZE];
  char orig_bits[MAX_MODE_PAGE_SIZE];
  u_int8_t p_offset,block_offset = 4; 
  int i;
  char *page;

  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | CACHE_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  page = &scsi_buf[p_offset];

  /* save original bits */
  memcpy(orig_bits,(void *) page+bnum,count);
  /* set the bits to the desired value */
  memcpy((void *) b,bits,count);

  for( i=0; i<count; i++)
    if ((mask[i] & orig_bits[i]) == (mask[i] & b[i])) {
      /* the original and the to-be-stored bits are the same, invert them */
      b[i] = ~b[i];
    }

  set_cache_bytes(bnum,b,mask,count);

  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | CACHE_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  page = &scsi_buf[p_offset];

  /* compare newly retrieved bytes with the original ones */
  if (strncmp(page+bnum,orig_bits,count)) {
    /* bits were changed successfully */
    /* restore the original value */
    set_cache_bytes(bnum,orig_bits,mask,count);
    return(1);
  }
  else {
    /* printf("Failed, cannot set those bits ...\n"); */
    return(0);
  }
}

/*---------------------------------------------------------------------------*
 * Given 'bit' on the Cache Mode Page, it returns the correct mask and the   *
 * position (offset) on the page in 'bnum'. It also returns the value of the *
 * bit set to one.                                                           * 
 *---------------------------------------------------------------------------*/
void
select_cache_bit(uint bit, int *bnum, char bits[], char mask[])
{
  switch(bit) {
  case CACHE_WCE:
    bits[0] = 0x04;
    mask[0] = 0x04;
    *bnum = 2;
    break;
  case CACHE_RCD:
    bits[0] = 0x01;
    mask[0] = 0x01;
    *bnum = 2;
    break;
  case CACHE_DRA:
    bits[0] = 0x20;
    mask[0] = 0x20;
    *bnum = 12;
    break;
  case CACHE_DISC:
    bits[0] = 0x10;
    mask[0] = 0x10;
    *bnum = 2;
    break;
  case CACHE_MF:
    bits[0] = 0x02;
    mask[0] = 0x02;
    *bnum = 2;
    break;
  case CACHE_CAP:
    bits[0] = 0x20;
    mask[0] = 0x20;
    *bnum = 2;
    break;
  case CACHE_ABPF:
    bits[0] = 0x40;
    mask[0] = 0x40;
    *bnum = 2;
    break;
  case CACHE_IC:
    bits[0] = 0x80;
    mask[0] = 0x80;
    *bnum = 2;
    break;
  case CACHE_FSW:
    bits[0] = 0x80;
    mask[0] = 0x80;
    *bnum = 12;
    break;
  default:
    internal_error("select_cache_bit: Bit %d not recognized!\n",bit);
  }
}
/*---------------------------------------------------------------------------*
 * Determines if the 'bit' is savable in the Cache Mode Page and returns 1   *
 * if it is. Otwerwise returns 0.                                            * 
 *---------------------------------------------------------------------------*/
int
cache_bit_savable(uint bit)
{
  char bits[2];
  char mask[2];
  int bnum;

  select_cache_bit(bit,&bnum,bits,mask);
  return(cache_bytes_savable(bnum,bits,mask,1));
}

/*---------------------------------------------------------------------------*
 * Sets the 'bit' on Cache Mode Page to the specific value and returns 1.    *
 * Meaningful values are either 1 or 0. If the desired cannot be set,        *
 * returns 0.                                                                * 
 *---------------------------------------------------------------------------*/
int
set_cache_bit(uint bit,u_int8_t value)
{
  char bits[2];
  char mask[2];
  int bnum;
  if (!cache_bit_savable(bit))
    return(0);
  select_cache_bit(bit,&bnum,bits,mask);
  /* if value is set to zero, clear the bit */
  if (!value) 
    bits[0] = 0;
  set_cache_bytes(bnum,bits,mask,1);
  return(1);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
u_int8_t
get_cache_bit(uint bit)
{
  char bits[2];
  char mask[2];
  int bnum;
  u_int8_t p_offset,block_offset = 4; 
  char *page;

  select_cache_bit(bit,&bnum,bits,mask);
  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | CACHE_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  page = &scsi_buf[p_offset];

  return(((u_int8_t) scsi_buf[p_offset+bnum] & mask[0]) != 0);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_cache_bytes(uint bnum,char bits[],char mask[],uint count)
{
  u_int8_t p_offset,block_offset = 4; 
  char *page;
  int i;
  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | CACHE_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  page = &scsi_buf[p_offset];
  for( i=0 ; i < count ; i++) {
    bits[i] = page[bnum+i] & mask[i];
  }
}

/*---------------------------------------------------------------------------*
 * Sets the byte bnum of the Cache Mode Page to the 'bits'. Returns the      *
 * previous value stored in the byte bnum. Only the bits which are set to 1  *
 * in the corresponding mask are actually modified.                          *
 * b[0] = 0xFF; m[0] = 0x0 will modify nothing. m[0] = 0xFF will change all  *
 * bits to the pattern set in b[0].                                          *
 *---------------------------------------------------------------------------*/
void
set_cache_bytes(uint bnum,char bits[],char mask[],uint count)
{
  /* #define DEBUG_CACHES */

  u_int8_t p_offset,block_offset = 4; 
  uint datalen = 0x18;
  char *page;
  char temp_bytes[MAX_MODE_PAGE_SIZE];
  int i;

  memset(scsi_buf,0,datalen);
  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | CACHE_PAGE));

  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  datalen = (u_int8_t) scsi_buf[0]+1;
  page = &scsi_buf[0];

#ifdef DEBUG_CACHES
  print_scsi_mode_sense_data(scsi_buf);  

  print_hexpage((char *)scsi_buf+p_offset,scsi_buf[p_offset+1]);
  /*
  printf("CACHE datalen: 0x%02X\n", datalen);
  for(i=0;i< datalen ;i++)
    printf("%02X ",(u_int8_t) scsi_buf[i]);
    */
  printf("\n\n");
#endif

  /* retrieve original bits */
  for( i=0; i<count; i++) 
    temp_bytes[i] = scsi_buf[p_offset+bnum+i] & mask[i];
  /* orig_bits = scsi_buf[p_offset+2] & 0x05; */

  /* clear the bits */
  for( i=0; i<count; i++) 
    scsi_buf[p_offset+bnum+i] = (~mask[i]) & scsi_buf[p_offset+bnum+i];
  /* scsi_buf[p_offset+2] = ((u_int8_t) (scsi_buf[p_offset+2] & 0xF8)); */

  /*set the bits according to the parameter bits */
  for( i=0; i<count; i++) 
    scsi_buf[p_offset+bnum+i] = (bits[i]&mask[i]) | scsi_buf[p_offset+bnum+i];
  /* scsi_buf[p_offset+2] = ((u_int8_t) (scsi_buf[p_offset+2] | bits)); */
  
  /* clear the PS bit */
  scsi_buf[p_offset] = ((u_int8_t) (scsi_buf[p_offset] & 0x3F));

  /* set the mode data length to zero */
  scsi_buf[0] = 0x0;

#ifdef DEBUG_CACHES
  printf("Page to be written:\n");
  print_hexpage((char *)scsi_buf+p_offset,scsi_buf[p_offset+1]);
  /*
  for(i=0;i< datalen ;i++)
    printf("%02X ",(u_int8_t) scsi_buf[i]);
  printf("\n");
  */
#endif
  exec_scsi_command(scsi_buf, SCSI_mode_select(datalen)); 

  /* copy the original bits to the array given as an argument */
  for( i=0; i<count; i++) 
    bits[i] = temp_bytes[i];
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int 
get_cache_numsegs(void)
{
  char bits[4];
  char mask[4];

  memset((void *) bits,0,sizeof(bits));
  /* set mask */
  memset((void *) mask,0xFF,sizeof(mask));

  get_cache_bytes(13,bits,mask,1);
  return((int) bits[0]);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
set_cache_numsegs(int numsegs)
{
  char bits[4];
  char mask[4];

  memset((void *) bits,0,sizeof(bits));
  /* set mask */
  memset((void *) mask,0xFF,sizeof(mask));

  bits[0] = (u_int8_t) numsegs;
  set_cache_bytes(13,bits,mask,1);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
get_cache_prefetch(unsigned int *min,unsigned int *max)
{
  char bits[4];
  char mask[4];

  memset((void *) bits,0,sizeof(bits));
  /* set mask */
  memset((void *) mask,0xFF,sizeof(mask));

  get_cache_bytes(6,bits,mask,4);

  *min = _2btol((u_int8_t *) bits);
  *max = _2btol((u_int8_t *) bits+2);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
set_cache_prefetch(unsigned int min,unsigned int max)
{
  char bits[4];
  char mask[4];

  memset((void *) bits,0,sizeof(bits));
  /* set mask */
  memset((void *) mask,0xFF,sizeof(mask));

  _lto2b(min,(u_int8_t *) bits);
  _lto2b(max,(u_int8_t *) bits+2);

  set_cache_bytes(6,bits,mask,4);
}


/*---------------------------------------------------------------------------*
 * Sets the byte bnum of the Cache Mode Page to the 'bits'. Returns the      *
 * previous value stored in the byte bnum. Only the bits which are set to 1  *
 * in the corresponding mask are actually modified.                          *
 * b[0] = 0xFF; m[0] = 0x0 will modify nothing. m[0] = 0xFF will change all  *
 * bits to the pattern set in b[0].                                          *
 *---------------------------------------------------------------------------*/
void
set_mode_page_bytes(u_int8_t pagenum, uint bnum, char bits[], char mask[],
		    uint count) {
  /* #define DEBUG_CACHES */

  u_int8_t p_offset,block_offset = 4; 
  uint datalen = 0x18;
  char *page;
  char temp_bytes[MAX_MODE_PAGE_SIZE];
  int i;

  memset(scsi_buf,0,datalen);
  exec_scsi_command(scsi_buf,
		    SCSI_mode_sense(SMS_PAGE_CTRL_CURRENT | pagenum));

  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  datalen = (u_int8_t) scsi_buf[0]+1;
  page = &scsi_buf[0];

#ifdef DEBUG_CACHES
  print_scsi_mode_sense_data(scsi_buf);  

  print_hexpage((char *)scsi_buf+p_offset,scsi_buf[p_offset+1]);
  /*
  printf("CACHE datalen: 0x%02X\n", datalen);
  for(i=0;i< datalen ;i++)
    printf("%02X ",(u_int8_t) scsi_buf[i]);
    */
  printf("\n\n");
#endif

  /* retrieve original bits */
  for( i=0; i<count; i++) 
    temp_bytes[i] = scsi_buf[p_offset+bnum+i] & mask[i];
  /* orig_bits = scsi_buf[p_offset+2] & 0x05; */

  /* clear the bits */
  for( i=0; i<count; i++) 
    scsi_buf[p_offset+bnum+i] = (~mask[i]) & scsi_buf[p_offset+bnum+i];
  /* scsi_buf[p_offset+2] = ((u_int8_t) (scsi_buf[p_offset+2] & 0xF8)); */

  /*set the bits according to the parameter bits */
  for( i=0; i<count; i++) 
    scsi_buf[p_offset+bnum+i] = (bits[i]&mask[i]) | scsi_buf[p_offset+bnum+i];
  /* scsi_buf[p_offset+2] = ((u_int8_t) (scsi_buf[p_offset+2] | bits)); */
  
  /* clear the PS bit */
  scsi_buf[p_offset] = ((u_int8_t) (scsi_buf[p_offset] & 0x3F));

  /* set the mode data length to zero */
  scsi_buf[0] = 0x0;

#ifdef DEBUG_CACHES
  printf("Page to be written:\n");
  print_hexpage((char *)scsi_buf+p_offset,scsi_buf[p_offset+1]);
  /*
  for(i=0;i< datalen ;i++)
    printf("%02X ",(u_int8_t) scsi_buf[i]);
  printf("\n");
  */
#endif
  exec_scsi_command(scsi_buf, SCSI_mode_select(datalen)); 

  /* copy the original bits to the array given as an argument */
  for( i=0; i<count; i++) 
    bits[i] = temp_bytes[i];
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
set_cache_buffer_full_ratio(int param) {
  char b[2];
  char m[2];

  exec_scsi_command(scsi_buf,SCSI_mode_sense(DISCON_RECON_PAGE));
  // exec_scsi_command(scsi_buf,SCSI_mode_sense(CACHE_PAGE));
  
  // print_scsi_mode_sense_data (scsi_buf);

  b[0] = param;
  m[0] = 0xFF;

  //  set_mode_page_bytes(CACHE_PAGE, 2, b, m, 1);
  set_mode_page_bytes(DISCON_RECON_PAGE, 2, b, m, 1);
  print_scsi_mode_sense_data (scsi_buf);

}

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

#ifndef __BUILD_SCSI
#define __BUILD_SCSI

#include "config.h"

#ifdef DIXTRAC_LINUX_SG
#define DEVNAME         "/dev/sg"
#endif
#ifdef DIXTRAC_FREEBSD_CAM
#define DEVNAME         "/dev/pass"
#endif

#define B_WRITE		0x00000000  /* Write buffer (partner to B_READ) */
#define	B_READ		0x00000001  /* Read buffer. */

#define SCSI_DATA_OUT   0x00000001
#define SCSI_DATA_IN    0x00000000

/* used in SCSI rw command */
extern int blksize;
#define SECT_SIZE	(blksize ? blksize : 512)

/* SCSI defintion constants */
#define SCSI_1_DEF          0x01
#define SCSI_1_CCS_DEF      0x02
#define SCSI_2_DEF          0x03
#define SCSI_3_DEF          0x04

/* SCSI Command Opcodes */
#define SEEK_EXTENDED       0x2B
#define REZERO_UNIT         0x01
#define REQUEST_SENSE       0x03
#define CHANGE_DEFINITION   0x40

/* max lenght of a mode page */
#define MAX_MODE_PAGE_SIZE  0xFF

#define INQLENGTH	0xFF
#define READCAP_LENGTH  0x08
#define MSENSE_LENGTH   0xFF /* !! */

/* send/receive diagnostic page codes */
#define TRANS_ADDR_PAGE 0x40
#define DEV_STATUS_PAGE 0x41

#define TRANS_ADDR_PAGE_LEN 0xA

/* mode sense/select page codes */
#define DISCON_RECON_PAGE  0x02
#define GEOM_PAGE          0x04
#define CACHE_PAGE         0x08
#define NOTCH_PAGE         0x0C
#define FORMAT_PAGE        0x03

/* bits for enabling/disabling caches */
#define READ_CACHE_DISABLE  0x01
#define READ_CACHE_ENABLE   0x00
#define WRITE_CACHE_DISABLE 0x00
#define WRITE_CACHE_ENABLE  0x04

typedef struct scsi_command_s {
  uint length;
  uint datalen;
  struct scsi_generic command;
} scsi_command_t;

struct scsi_seek_extended {
	u_int8_t opcode;
	u_int8_t byte2;
	u_int8_t lbn[4];
	u_int8_t reserved[4];
};

/* specify what command (10 or 12 byte) to use for getting defects */
/* if undefined, 12 byte version is used */
#define USE_READ_DEFECT_DATA_10

#ifdef USE_READ_DEFECT_DATA_10
#define scsi_defect_data_header scsi_read_defect_data_hdr_10
#else
#define scsi_defect_data_header scsi_read_defect_data_hdr_12
#endif

/* Redundant, defined already in scsi_da, but left here for legacy reasons */
struct scsi_physical_sector_format {
	u_int8_t cylno[3];
	u_int8_t headno;
	u_int8_t sectno[4];
};

struct scsi_bytes_offset_from_index_format {
	u_int8_t cylno[3];
	u_int8_t headno;
	u_int8_t bytesoff[4];
};

int scsi_init(char *dev);
int close_scsi_disk(void);
char *scsi_alloc_buffer(void);
void scsi_dealloc_buffer(void);
int send_scsi_command(char *buffer, scsi_command_t *s, int cmd_id);
int recv_scsi_command(char *buffer);
int exec_scsi_command(char *addr, scsi_command_t * s);
uint scsi_command_type (int opcode);
scsi_command_t *SCSI_rw_command (int rw, uint blkno, uint bcount);
scsi_command_t *SCSI_read_capacity(uint lbn,u_int8_t pmi);
scsi_command_t *SCSI_mode_sense (u_int8_t page_code);
scsi_command_t *SCSI_mode_select (uint datalen);
scsi_command_t *SCSI_inq_command (void);
scsi_command_t * SCSI_request_sense(void);
scsi_command_t *SCSI_send_diag_command (uint datalength);
scsi_command_t *SCSI_recv_diag_command (uint datalength);
scsi_command_t *SCSI_seek_command (uint lbn);
scsi_command_t *SCSI_rezero_command (void);
scsi_command_t *SCSI_read_defect_data (uint datalen);
int create_diag_page(u_int8_t page_code,char *p,int addr);
int create_lba_to_phys_page(char *p,int addr);
int create_phys_to_lba_page(char *p,int cyl,int head, int sector);

scsi_command_t *SCSI_chg_definition(u_int8_t new_definition);
scsi_command_t *SCSI3_inq_command (u_int8_t page_code);

#endif

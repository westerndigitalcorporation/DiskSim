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

#if DIXTRAC_LINUX_SG

#define SG_HDR_OFFSET   sizeof(struct sg_header)

#ifdef STATE_THREADS
/* timeout in microseconds */
#define  ST_TIMEOUT              1000000
st_netfd_t scsidev_fd;                    /* SCSI device file descriptor */
#else
int scsidev_fd;                    /* SCSI device file descriptor */
#endif
#endif

#if DIXTRAC_FREEBSD_CAM

#define SG_HDR_OFFSET        64

struct cam_device cam_device;

#endif

#include "handle_error.h"
#include "build_scsi.h"
#include "print_bscsi.h"


#define SCSI_MAXCMDLEN     16

#ifndef SG_BIG_BUFF
#define SG_BIG_BUFF 4096
#endif

/* buffer size for SCSI data transfer */
#define BIG_BUF_SIZE  (10000*SECT_SIZE)

static int scsi_version;

int blksize = 0;

static char *master_buffer;
static scsi_command_t sc;

long ustart,ustop;
long ustart_sec,ustop_sec;

#undef min
#define min(a,b)	(((a)<(b))?(a):(b))

#define LINUX_NONBLOCK_SG_VERSION 0x20208

#if LINUX_VERSION_CODE > LINUX_NONBLOCK_SG_VERSION
#define SG_NONBLOCKING
#endif

#ifdef SG_IO
#define LINUX_SG_IO
#define SG_IO_TIMEOUT    1000
static unsigned char * sense_buffer;
/* commented the following line on 3/21/2002 since we can do non-blocking */
/*  #undef SG_NONBLOCKING */
#endif

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
char *
scsi_alloc_buffer(void) 
{
  if (!master_buffer) {
    if (!(master_buffer = (char *) malloc(BIG_BUF_SIZE))) {
      error_handler("Couldn't allocate master_buffer!\n");
    }
    memset (master_buffer,0,BIG_BUF_SIZE);
  }
#ifdef LINUX_SG_IO
    sense_buffer = (unsigned char *) malloc(20);
    assert (sense_buffer != NULL);
    memset (sense_buffer,0,4);
#endif

  return(master_buffer+SG_HDR_OFFSET+SCSI_MAXCMDLEN);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
void
scsi_dealloc_buffer(void) 
{
  free(master_buffer);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
scsi_init(char *devname) 
{
  char *inq_buf;

#ifdef DIXTRAC_LINUX_SG

#ifdef SG_NONBLOCKING
  int ictl_val;
#endif

#ifdef SG_NONBLOCKING
  int fd = open(devname, O_RDWR | O_NONBLOCK);
#else
  int fd = open(devname, O_RDWR);
#endif

  if (fd < 0) {
    error_handler("Device %s not defined or no R/W permmissions.\n",devname);
  }
#ifdef SG_NONBLOCKING
  ictl_val = 1;
  if ( 0 > ioctl(fd,SG_SET_COMMAND_Q,&ictl_val)) {
    if (errno == ENOTTY)
      ictl_val = 0;
    else
      error_handler("Problems with ioctl on device %s \n",devname);
  }
/*  #ifndef SILENT */
#if 0
  printf("Command queueing%s allowed.\n",(ictl_val == 1 ? "" : " not"));
#endif
  ictl_val = 0;
  if ( 0 > ioctl(fd,SG_SET_FORCE_PACK_ID,&ictl_val)) {
    error_handler("Could not set FORCE_PACK_ID  on device %s \n",devname);
  }
  ictl_val = BIG_BUF_SIZE;
  if ( 0 > ioctl(fd,SG_SET_RESERVED_SIZE,&ictl_val)) {
    error_handler("Problems with ioctl on device %s!\n",devname);
  }
  ioctl(fd,SG_GET_RESERVED_SIZE,&ictl_val);
/*  #ifndef SILENT */
#if 0
  fprintf(stderr, "*** The size of the RESERVED kernel buffer: %d\n",ictl_val);
  /*  ioctl(fd,SG_GET_SG_TABLESIZE,&ictl_val); */
  /*  printf("TABLESIZE: %d\n",ictl_val); */
#endif

#endif

#ifdef STATE_THREADS
  if (st_init()) {
    error_handler("Problems with st_init!\n");
  }
  
  if (!(scsidev_fd = st_netfd_open(fd))) {
    error_handler("Opening sthread fd failed\n", devname);
  }
#else
  scsidev_fd = fd;
#endif

/* DIXTRAC_LINUX_SG */
#endif

#ifdef DIXTRAC_FREEBSD_CAM
  if (cam_open_pass(devname, O_RDWR, &cam_device) == NULL) {
    error_handler("Opening pass device (%s) failed\n", devname);
  }
#endif

  inq_buf = scsi_alloc_buffer();
  /* get_scsi_version */
  send_scsi_command(inq_buf, SCSI_inq_command(),0);
  recv_scsi_command(inq_buf);
  scsi_version = ((u_int8_t) inq_buf[2]);

  /* get the device block size (not all disks use 512-byte sector) */
  exec_scsi_command(inq_buf,SCSI_read_capacity(0,0));
  /* this is a global that is accessed through te SECT_SIZE macro */
  blksize = _4btol((u_int8_t *) &inq_buf[4]);
  return(0);

}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
close_scsi_disk(void)
{
#ifdef DIXTRAC_LINUX_SG
#ifdef STATE_THREADS
  return(st_netfd_close(scsidev_fd));
#else
  return(close(scsidev_fd));
#endif
#endif

#ifdef DIXTRAC_FREEBSD_CAM
  return 0;
#endif
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
send_scsi_command(char *buffer, scsi_command_t *s, int cmd_id) 
{
#ifdef DIXTRAC_LINUX_SG

  int cmd_len,status = 0;

#ifdef LINUX_SG_IO
  sg_io_hdr_t sgio_hdr;
#else
  uint in_size,out_size;
  struct sg_header *sg_hd;
  char *buf_pointer;
#endif

#ifndef SG_TIMER  
  struct timeval start;
  struct timezone tz;
#endif


#ifdef WRITE_CHECK
  assert((s->command.opcode!=WRITE_6) && (s->command.opcode!=WRITE_10));
#endif

  cmd_len = s->length;

  /* safety checks */
  if (!cmd_len) return -1;            /* need a cmd_len != 0 */
  if (!buffer) return -1;             /* check argument to be != NULL */

/*   printf("SCSI command 0x%2X: length %d, datalen %d\n",s->command.opcode, */
/* 	 s->length,s->datalen); */

#ifdef LINUX_SG_IO
  memset((void *) &sgio_hdr, 0, sizeof(sg_io_hdr_t));
  sgio_hdr.interface_id = 'S';
  sgio_hdr.cmd_len = cmd_len;
  sgio_hdr.iovec_count = 0;
  sgio_hdr.mx_sb_len = sizeof(sense_buffer);
  if (scsi_command_type (s->command.opcode) == SCSI_DATA_IN) 
    sgio_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
  else
    sgio_hdr.dxfer_direction = SG_DXFER_TO_DEV;
  sgio_hdr.dxfer_len = s->datalen;
  sgio_hdr.dxferp = (void *) buffer;
  sgio_hdr.cmdp = (unsigned char *) &(s->command);
  sgio_hdr.sbp = (unsigned char *) sense_buffer;
  sgio_hdr.timeout = SG_IO_TIMEOUT;
  sgio_hdr.pack_id = cmd_id;
  sgio_hdr.flags = SG_FLAG_DIRECT_IO; 
#else
  /* adjust pointer to buffer passed to write so that we can prepend the 
     data in buffer with the SCSI command                                */
  buf_pointer = buffer - cmd_len;
  /* copy the command */
  memcpy(buf_pointer,(char *) &s->command,s->length );
  
  if (scsi_command_type (s->command.opcode) == SCSI_DATA_IN) {
    in_size = 0;
    out_size = s->datalen;
  }
  else {
    in_size = s->datalen;
    out_size = 0;
    /* copy the additional data to be sent */
    /* memcpy(in_buffer + SCSI_OFFSET + s->length,buffer,in_size); */
  }
  /* make sure we are sending at most # of bytes handled by sg driver */
  if (SG_HDR_OFFSET + cmd_len + in_size > SG_BIG_BUFF) return -1;
 
  if (SG_HDR_OFFSET + out_size > SG_BIG_BUFF) {
    out_size = SG_BIG_BUFF-SG_HDR_OFFSET;
    /*
    printf("exec_scsi_command: Warning - can retrieve only %d KB of data\n",
	   out_size / 1024);
    */
  }
  /* make buf_pointer point to the begining of the buffer given to write */
  buf_pointer -= SG_HDR_OFFSET;

  /* generic SCSI device header construction */
  sg_hd = (struct sg_header *) buf_pointer;
  sg_hd->reply_len   = SG_HDR_OFFSET + out_size;
  sg_hd->twelve_byte = (cmd_len == 12);
  sg_hd->pack_id = cmd_id; 
  sg_hd->result = 0;
#endif

#ifndef SG_TIMER  
  gettimeofday(&start,&tz);
#endif

#ifdef LINUX_SG_IO
  /* send command */
#ifdef STATE_THREADS
  status = st_write(scsidev_fd, &sgio_hdr, sizeof(sg_io_hdr_t),ST_TIMEOUT);
#else
  status = write(scsidev_fd, &sgio_hdr, sizeof(sg_io_hdr_t));
#endif
  if ( status < 0 || (status < sizeof(sg_io_hdr_t))) {
    /* some error happened */
    fprintf( stderr, "write(sg_io) result = 0x%x cmd = 0x%x\n",
	     status, s->command.opcode );
  }
#else
  /* send command */
  status = write(scsidev_fd, buf_pointer, SG_HDR_OFFSET + cmd_len + in_size);
  if ( status < 0 || status != SG_HDR_OFFSET + cmd_len + in_size ||
       sg_hd->result ) {
    /* some error happened */
    fprintf( stderr, "write(generic) result = 0x%x cmd = 0x%x\n",
	     sg_hd->result, s->command.opcode );
  }
#endif

#ifdef SG_TIMER

  //  printf("%s() (%d,%d)\n", __func__, sgio_hdr.duration.tv_sec,
  //	 sgio_hdr.duration.tv_usec);

#ifdef LINUX_SG_IO
  ustart = sgio_hdr.duration.tv_usec;
  ustart_sec = sgio_hdr.duration.tv_sec;
#else
  ustart = sg_hd->time.tv_usec;
  ustart_sec = sg_hd->time.tv_sec;
#endif
#else 
  ustart = start.tv_usec;
  ustart_sec = start.tv_sec;
#endif  

  return status;

/* DIXTRAC_LINUX_SG */
#endif

#ifdef DIXTRAC_FREEBSD_CAM
  union ccb ccb;
  int flags;
  int rc;
  char cmd_spec[30];

  struct timeval start;
  struct timeval stop;
  struct timezone tz;
  
  flags = (scsi_command_type (s->command.opcode) == 
	   SCSI_DATA_IN) ? CAM_DIR_IN : CAM_DIR_OUT;

  ccb.ccb_h.path_id = cam_device.path_id;
  ccb.ccb_h.target_id = cam_device.target_id;
  ccb.ccb_h.target_lun = cam_device.target_lun;

  /* The following command must be included to make CAM work */
  sprintf(cmd_spec,"");
  for(rc=0;rc<s->length;rc++) {
    sprintf(cmd_spec,"%s v",cmd_spec);
  }

  csio_build(&ccb.csio,
	     /* data_ptr */ buffer,
	     /* dxfer_len */ s->datalen,
	     /* flags */ flags | CAM_DEV_QFRZDIS,
	     /* retry_count */ 0,
	     /* timeout */ 5000, 
	     /* cmd_spec...*/ cmd_spec, 
	     ((char *) &s->command)[0],((char *) &s->command)[1],
	     ((char *) &s->command)[2],((char *) &s->command)[3],
	     ((char *) &s->command)[4],((char *) &s->command)[5],
	     ((char *) &s->command)[6],((char *) &s->command)[7],
	     ((char *) &s->command)[8],((char *) &s->command)[9],
	     ((char *) &s->command)[10],((char *) &s->command)[11],
	     ((char *) &s->command)[12],((char *) &s->command)[13],
	     ((char *) &s->command)[14],((char *) &s->command)[15]
);

  gettimeofday(&start,&tz);
  ustart = start.tv_usec;
  ustart_sec = start.tv_sec;

  rc = cam_send_ccb(&cam_device, &ccb);

  gettimeofday(&stop,&tz);
  ustop_sec = stop.tv_sec;
  ustop = stop.tv_usec;

  if (rc < 0 || 
      (ccb.ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
    fprintf(stderr, "Error: cam_send_ccb (rc=%d)\n", rc);

    if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_SCSI_STATUS_ERROR) {
      fprintf(stderr, "Printing sense buffer\n");
      scsi_sense_print(&cam_device, &ccb.csio, stderr);
    }
    return -1;
  }

  return 0;

/* DIXTRAC_FREEBSD_CAM */
#endif
}

/*---------------------------------------------------------------------------
 * Returns -1 on error, otherwise returns the ID of the command.
 *--------------------------------------------------------------------------*/
int 
recv_scsi_command(char *buffer)
{
#ifdef DIXTRAC_LINUX_SG
  int status = 0;

#ifdef LINUX_SG_IO
  sg_io_hdr_t sgio_hdr;
  int i;
#else
  char *buf_pointer;
  struct sg_header *sg_hd;
#endif

#ifndef SG_TIMER  
  struct timeval stop;
  struct timezone tz;
#endif
  
#ifdef SG_NONBLOCKING
#ifdef STATE_THREADS
#else
/* timeout in miliseconds */
#define TIMEOUT         5000  
  struct pollfd ufd;
  int rc;

  ufd.fd = scsidev_fd;
  ufd.events = POLLPRI | POLLIN;
  if ( 1 > (rc = poll(&ufd,1,TIMEOUT))) {
    if (rc == 0) {
      fprintf(stderr,"Disk timed out\n");
      return(-1);
    }
    else {
      error_handler("Error with poll syscall %s\n","");
    }
  }
#endif
#endif
#ifdef LINUX_SG_IO
  sgio_hdr.flags = SG_FLAG_DIRECT_IO; 
#ifdef STATE_THREADS
  status = st_read(scsidev_fd, &sgio_hdr, sizeof(sg_io_hdr_t),ST_TIMEOUT);
#else
  status = read(scsidev_fd, &sgio_hdr, sizeof(sg_io_hdr_t));
#endif
#else
  buf_pointer = buffer - SG_HDR_OFFSET; 
  sg_hd = (struct sg_header *) buf_pointer;
  /*  sg_hd->pack_id = cmd_id; */

  /* retrieve result */
  status = read(scsidev_fd, buf_pointer, SG_BIG_BUFF);
#endif

#ifndef SG_TIMER
  gettimeofday(&stop,&tz);
  ustop_sec = stop.tv_sec;
  /* ustop = (ustop_sec - ustart_sec)*1000000 + stop.tv_usec; */
  ustop = stop.tv_usec;
#else
#ifdef LINUX_SG_IO
  //  printf("%s() (%d,%d)\n", __func__, sgio_hdr.duration.tv_sec,
  // sgio_hdr.duration.tv_usec);


  ustop = sgio_hdr.duration.tv_usec;
  ustop_sec = sgio_hdr.duration.tv_sec;
#else
  ustop = sg_hd->time.tv_usec;
  ustop_sec = sg_hd->time.tv_sec;
#endif
#endif  

#ifdef LINUX_SG_IO  
  /*  printf("Time returned from sgio driver: %d ms\n",sgio_hdr.duration); */
  if ( status < 0 || (status < sizeof(sg_io_hdr_t))) {
    /* some error happened */
    fprintf( stderr, "read(sg_io) status = 0x%x\n", status);
    fprintf( stderr, "Sense buffer: ");
    for(i=0 ; i<sgio_hdr.sb_len_wr ; i++) 
      fprintf( stderr,"%x ",sense_buffer[i]);
    fprintf( stderr,"\n");
  }
  return(sgio_hdr.pack_id);
#else
  if ( status < 0 || sg_hd->result ) {
    /* some error happened */
    fprintf( stderr, "read(generic) status = 0x%x, result = 0x%x\n",
	     status, sg_hd->result);
    fprintf( stderr, "read(generic) sense "
	     "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
	     sg_hd->sense_buffer[0],         sg_hd->sense_buffer[1],
	     sg_hd->sense_buffer[2],         sg_hd->sense_buffer[3],
	     sg_hd->sense_buffer[4],         sg_hd->sense_buffer[5],
	     sg_hd->sense_buffer[6],         sg_hd->sense_buffer[7],
	     sg_hd->sense_buffer[8],         sg_hd->sense_buffer[9],
	     sg_hd->sense_buffer[10],        sg_hd->sense_buffer[11],
	     sg_hd->sense_buffer[12],        sg_hd->sense_buffer[13],
	     sg_hd->sense_buffer[14],        sg_hd->sense_buffer[15]);
    if (status < 0)
      error_handler("recv_scsi_command: Read error (%s)\n",strerror(errno));
  }
  else 
    /* buffer should already have all the necessarry data */
    /* just adjust the number of bytes returned */
    status -= SG_HDR_OFFSET;
  return (sg_hd->pack_id);
#endif

/* DIXTRAC_LINUX_SG */
#endif

#ifdef DIXTRAC_FREEBSD_CAM
  return(0);
#endif

}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
exec_scsi_command(char *buffer, scsi_command_t *s) {
  int status;

#ifdef SG_NONBLOCKING
#ifdef STATE_THREADS
#else
  if (0 > ioctl(scsidev_fd,SG_GET_NUM_WAITING,&status))
    error_handler("exec_scsi_command: SG_GET_NUM_WAITING ioctl (%s)\n",
		  strerror(errno));
  if (status > 0)
    error_handler("exec_scsi_command: outstanding requests %s\n","");
#endif
#endif

  send_scsi_command(buffer,s,0);
  status = recv_scsi_command(buffer);

  return(status);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
uint 
scsi_command_type (int opcode)
{
   switch (opcode) {
        case RECEIVE_DIAGNOSTIC:
	case READ_6:
	case READ_10:
	case MODE_SENSE:
	case READ_CAPACITY:
	case READ_DEFECT_DATA_10:
	case READ_DEFECT_DATA_12:
	case INQUIRY:
        case REQUEST_SENSE:
        case REZERO_UNIT:
        case SEEK_EXTENDED:
	  return SCSI_DATA_IN;
	case CHANGE_DEFINITION:
        case SEND_DIAGNOSTIC:
	case WRITE_6:
	case WRITE_10:
        case MODE_SELECT:
	case REASSIGN_BLOCKS:
	  return SCSI_DATA_OUT;
	default:
	  internal_error("unknown SCSI command opcode: %d\n", opcode);
	  return(0);
   }
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_rw_command (int rw, uint blkno, uint bcount)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  memset(c,0,sizeof(scsi_command_t));

  /*  if (((blkno & 0x1fffff) == blkno) && ((nblks & 0xff) == nblks)) { */
     /* transfer can be described in small RW SCSI command, so do it */
    /*
      struct scsi_rw *cmd_small = (struct scsi_rw *) &(c->command); 

      cmd_small->opcode = (rw == B_READ) ? READ_COMMAND : WRITE_COMMAND;
      _lto3b (blkno, cmd_small->addr);
      cmd_small->length = nblks & 0xff;
      c->length = sizeof (struct scsi_rw);
   } 
   else */
  {
    struct scsi_rw_10 *cmd_big = (struct scsi_rw_10 *) &(c->command); 
     cmd_big->opcode = (rw == B_READ) ? READ_10 : WRITE_10;
     _lto4b (blkno, cmd_big->addr);
     _lto2b (bcount, cmd_big->length);
     c->length = sizeof (struct scsi_rw_10);
  }
   c->datalen = bcount*SECT_SIZE;
   return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_read_capacity(uint lbn,u_int8_t pmi)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_read_capacity *cmd = (struct scsi_read_capacity *) 
                                   &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof (struct scsi_read_capacity);
  c->datalen = READCAP_LENGTH;
  cmd->opcode = READ_CAPACITY;
  cmd->byte2 = 0x0; /* LUN=0 */
  _lto4b (lbn,cmd->addr);
  /*  __lto3b(0,cmd->unused); */
  cmd->unused[2]=pmi;
  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_mode_select (uint datalen)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_mode_select_6 *cmd_small = (struct scsi_mode_select_6 *) 
                                      &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

  c->length = sizeof (struct scsi_mode_select_6);
  c->datalen = datalen;

  cmd_small->opcode = MODE_SELECT;
  if (scsi_version >= 2)
    cmd_small->byte2 = SMS_PF;
  cmd_small->length=datalen & 0xFF;

  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_mode_sense (u_int8_t page)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_mode_sense_6 *cmd_small = (struct scsi_mode_sense_6 *) 
                                      &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof (struct scsi_mode_sense_6);
  cmd_small->opcode = MODE_SENSE;
  if (scsi_version >= 2)
    cmd_small->byte2 = SMS_DBD;
  else 
    cmd_small->byte2 = 0;
  cmd_small->page=page;
  c->datalen =  cmd_small->length = 0xFF;
  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_inq_command (void)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_inquiry *cmd_small = (struct scsi_inquiry *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof (struct scsi_inquiry);
  c->datalen = INQLENGTH;
  cmd_small->opcode = INQUIRY;
  cmd_small->byte2 = 0x0; /* LUN=0 */
  cmd_small->length=INQLENGTH;
  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_request_sense(void)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_inquiry *cmd_small = (struct scsi_inquiry *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof (struct scsi_inquiry);
  c->datalen = INQLENGTH;
  cmd_small->opcode = REQUEST_SENSE;
  cmd_small->byte2 = 0x0; /* LUN=0 */
  cmd_small->length=INQLENGTH;
  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_send_diag_command (uint datalength)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_send_diag *cmd = (struct scsi_send_diag *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof(struct scsi_send_diag);
  c->datalen = datalength;

  cmd->opcode = SEND_DIAGNOSTIC;
  cmd->byte2 = SSD_PF;  /* page format conforms to SCSI-2 */
  if (scsi_version >= 2) {
    cmd->byte2 = SSD_PF;  /* page format conforms to SCSI-2 */
  }
  else {
    cmd->byte2 = 0;  /* page format conforms to SCSI v.1 */
  }
  _lto2b(datalength,cmd->paramlen);

/*    printf("Send Diag command: ");  */
/*    print_hexpage((char *) cmd,6);  */

  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_recv_diag_command (uint datalength)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_generic *cmd = (struct scsi_generic *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

  c->length = sizeof(struct scsi_send_diag);
  c->datalen = datalength;

  cmd->opcode = RECEIVE_DIAGNOSTIC;
  cmd->bytes[0] = 0x0; 

  /* allocation length */
  _lto2b(datalength,&cmd->bytes[2]);

  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
create_lba_to_phys_page(char *p,int addr) 
{
  memset(p,0,TRANS_ADDR_PAGE_LEN+4);
  p[0]=TRANS_ADDR_PAGE;
  p[1]=0;
  p[2]=0;
  p[3]=TRANS_ADDR_PAGE_LEN;
  
  p[4] = 0x0; /* supplied format */
  p[5] = 0x05; /* translate format */
  
  _lto4b(addr,(u_int8_t *) &p[6]);
  
  return(TRANS_ADDR_PAGE_LEN+4);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
create_phys_to_lba_page(char *p,int cyl,int head,int sector) 
{
  memset(p,0,TRANS_ADDR_PAGE_LEN+4);
  p[0]=TRANS_ADDR_PAGE;
  p[1]=0;
  p[2]=0;
  p[3]=TRANS_ADDR_PAGE_LEN;
  
  p[4] = 0x05; /* supplied format */
  p[5] = 0x0; /* translate format */
  
  _lto3b(cyl,(u_int8_t *) &p[6]);
  p[9] = (u_int8_t) head;
  _lto4b(sector,(u_int8_t *) &p[10]);
  return(TRANS_ADDR_PAGE_LEN+4);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
int 
create_diag_page(u_int8_t page_code,char *p,int addr) 
{
  switch(page_code) {
  case TRANS_ADDR_PAGE:
    printf("No longer suppored, use create_lba_to_phys_page.\n");
    return(-1);
  case DEV_STATUS_PAGE:
    return(-1);
  }
  return(-1);
}


/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_seek_command (uint lbn)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_seek_extended *cmd = 
            (struct scsi_seek_extended *) &(c->command); 
  memset (c, 0,sizeof(scsi_command_t));

  c->length = 10;
  c->datalen = 0;

  cmd->opcode = SEEK_EXTENDED;

  /* lbn */
  _lto4b(lbn,cmd->lbn);
  
  return ((scsi_command_t *) c);
}


/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_rezero_command (void)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_generic *cmd = (struct scsi_generic *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

  c->length = 6;
  c->datalen = 0;

  cmd->opcode = REZERO_UNIT;

  return ((scsi_command_t *) c);
}


/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_read_defect_data (uint datalen)
{
  scsi_command_t *c = (scsi_command_t *) &sc;

#ifdef USE_READ_DEFECT_DATA_10
  struct scsi_read_defect_data_10 *cmd = (struct scsi_read_defect_data_10 *) 
    &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

#ifdef DIXTRAC_FREEBSD_CAM
  // CAM restricts I/O to 64K of I/O, rounded up to include the full set of pages
  // so limit max I/O size to 60 Kbytes
  datalen = min(datalen, (60*1024));
#else
  datalen = min(datalen, 0xFFFF);
#endif //DIXTRAC_FREEBSD_CAM

  c->length = 10;

  cmd->opcode = READ_DEFECT_DATA_10;
  cmd->format = SRDDH10_PLIST | SRDDH10_GLIST | SRDDH10_PHYSICAL_SECTOR_FORMAT;
  _lto2b(datalen,cmd->alloc_length);	/* alloclen */
#else
  struct scsi_read_defect_data_12 *cmd = (struct scsi_read_defect_data_12 *) 
    &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

#ifdef DIXTRAC_FREEBSD_CAM
  // CAM restricts I/O to 64K of I/O, rounded up to include the full set of pages
  // so limit max I/O size to 60 Kbytes
  datalen = min(datalen, (60*1024));
#else
  datalen = min(datalen, 0x1FFFF);
#endif //DIXTRAC_FREEBSD_CAM

  c->length = 12;

  cmd->opcode = READ_DEFECT_DATA_12;
  cmd->format = SRDDH12_PLIST | SRDDH12_GLIST | SRDDH12_PHYSICAL_SECTOR_FORMAT;
  _lto4b(datalen, cmd->alloc_length);	/* alloclen */
#endif
  c->datalen = datalen;

  cmd->byte2 = 0;			/* LUN + reserved */
  cmd->reserved[0] = 0; cmd->reserved[1] = 0;
  cmd->reserved[1] = 0; cmd->reserved[3] = 0;

  cmd->control = 0;			/* unknown control bits */

  return ((scsi_command_t *) c);
}

/***********************************************************************/
/***********************************************************************/

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI3_inq_command (u_int8_t page_code)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_inquiry *cmd_small = (struct scsi_inquiry *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));
  c->length = sizeof (struct scsi_inquiry);
  c->datalen = INQLENGTH;
  cmd_small->opcode = INQUIRY;
  cmd_small->byte2 = 0x01; /* LUN=0 */
  cmd_small->page_code = page_code;
  cmd_small->length=INQLENGTH;
  return ((scsi_command_t *) c);
}

/*---------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/
scsi_command_t * 
SCSI_chg_definition(u_int8_t new_definition)
{
  scsi_command_t *c = (scsi_command_t *) &sc;
  struct scsi_generic *cmd = (struct scsi_generic *) &(c->command); 
  memset(c,0,sizeof(scsi_command_t));

  c->length = 10;
  c->datalen = 0;

  cmd->opcode = CHANGE_DEFINITION;
  cmd->bytes[0] = 0x0;  /* LUN = 0 */
  cmd->bytes[1] = 0x0;  /* do not save permanently */
  cmd->bytes[2] = new_definition;  

  return ((scsi_command_t *) c);
}


/* config.h.  Generated from config.h.in by configure.  */
/******************************************************************************
*
* file: config.h
*
* This file is to be the first file included by all header and .c files.  
* It defines all system specific headers and macros
*
*******************************************************************************
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 ******************************************************************************
 * Autoconf Configure options - do not edit
 ******************************************************************************
 */
/* Defines whether inline exists - set by autoheader */
/* #undef inline  */

/* Do std c headers exist */
#define STDC_HEADERS 1

/* Does stdarg.h exist */
#define HAVE_STDARG_H 1

/*Does malloc.h exist */
#define HAVE_MALLOC_H 1

/* Does sys/time.h exist */
#define HAVE_SYS_TIME_H 1

/* Does errno.h exist */
#define HAVE_ERRNO_H 1

/* Does poll.h exist */
#define HAVE_SYS_POLL_H 1

/* Does unistd.h exist */
#define HAVE_UNISTD_H 1

/* Does sys/types.h exist */
#define HAVE_SYS_TYPES_H 1

/* Does fcntl.h exist */
#define HAVE_FCNTL_H 1

/* Does ioctl.h exist */
#define HAVE_SYS_IOCTL_H 1
 
/* Does math.h exist */
#define HAVE_MATH_H 1

/* Does getopt.h exist */
#define HAVE_GETOPT_H 1

/* Does linux/version.h exist */
#define HAVE_LINUX_VERSION_H 1

/* Does cam/cam.h exist */
/* #undef HAVE_CAMLIB_H */

/* Does pthread.h exist */
/* #undef HAVE_PTHREAD_H */

/* Does sched.h exist */
/* #undef HAVE_SCHED_H */

/*
 ******************************************************************************
 * Things to do based on defines set by autoconf 
 ******************************************************************************
 */

#if STDC_HEADERS
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_ERRNO_H 
#include <errno.h>
#endif

#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif 

#if HAVE_SYS_IOCTL_H 
#include <sys/ioctl.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif 

#if HAVE_MATH_H
#include <math.h>
#endif 

#if HAVE_GETOPT_H
#include <getopt.h>
#endif 

#if HAVE_LINUX_VERSION_H
#include <linux/version.h>
#define DIXTRAC_LINUX_SG 1
#include <linux/../scsi/sg.h>

/* headers specific to Linux */
#include "scsi_all.h"
#include "scsi_da.h"
#include "scsi_conv.h"

#define  STATE_THREADS 1
#include "st.h"

#endif

#if HAVE_CAMLIB_H
#include <cam/cam.h>
#include <cam/cam_ccb.h>
#include <cam/scsi/scsi_pass.h>
#include <camlib.h>
#include <cam/scsi/scsi_all.h>
#include <cam/scsi/scsi_da.h>
#define DIXTRAC_FREEBSD_CAM 1
#endif

#endif /* __CONFIG_H__ */

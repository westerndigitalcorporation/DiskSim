
/* libddbg (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
 */



#include <stdarg.h>
#include "libddbg.h"

#ifndef _LIBDDBG_FREEBSD

#include <signal.h>
#include <stdio.h>

static void fail(void) {
  raise(SIGABRT);
}


static FILE *ddbg_file = 0;
static ddbg_assert_handler ddbg_handler = (ddbg_assert_handler)fail;

void ddbg_assert_setfile(FILE *f) {
  ddbg_file = f;
}

void ddbg_assert_printmsg(const char *fmt, ...) {
  va_list ap;
  if(ddbg_file) {
    va_start(ap, fmt);
    vfprintf(ddbg_file, fmt, ap);
  }
}


#else 

#include <syslog.h>

void ddbg_kern_log(char *file, 
		   int line, 
		   const char *cond, 
		   const char *func,
		   const char *fmt, ...)
{
  char buff[1024];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buff, 1024, fmt, ap);
  va_end(ap);

  log(LOG_ALERT, buff);
}

void ddbg_assert_printmsg(const char *fmt, ...)
{
  char buff[1024];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buff, 1024, fmt, ap);
  va_end(ap);

  log(LOG_ALERT, buff);
}


static ddbg_assert_handler ddbg_handler = (ddbg_assert_handler)ddbg_kern_log;

#endif  //  _LIBDDBG_FREEBSD


void ddbg_assert_sethandler(ddbg_assert_handler h) {
  ddbg_handler = h;
}


void ddbg_assert_msg(char *file, 
		     int line, 
		     const char *cond,
		     const char *func, 
		     const char *fmt, ...)
{
  va_list ap;
  
#ifndef _LIBDDBG_FREEBSD  
  if(ddbg_file) {
    fprintf(ddbg_file, "*** assertion failed: in %s() (%s:%d): %s: ", func, file, line, cond);
    //    va_start(ap, fmt);
    //    vfprintf(ddbg_file, fmt, ap);
    //    fprintf(ddbg_file, "\n");

    ddbg_assert_printmsg(fmt, ap);
  }

#endif // _LIBDDBG_FREEBSD  

}


void ddbg_assert_fail(char *file, 
		      int line, 
		      const char *cond, 
		      const char *func, 
		      const char *fmt, ...) 
{
  va_list ap;

#ifndef _LIBDDBG_FREEBSD
  if(ddbg_file) {
    fprintf(ddbg_file, "\n");
    fflush(ddbg_file);
  }
#endif // _LIBDDBG_FREEBSD

  if(ddbg_handler) {
    va_start(ap, fmt);
    ddbg_handler(file,line,cond,func,fmt,ap);
  }
}

// End of file





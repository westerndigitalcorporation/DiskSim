
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



#ifndef _LIBTRACE_LIBTRACE_H
#define _LIBTRACE_LIBTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* tracing functions */
void ddbg_setup(FILE *);

// register a new class.  Returns an int class id to be used with
// subsequent calls
int ddbg_register(char *classname);

// start logging (class,instance) traces.  -1 is the wildcard which logs
// all instances from that class.
void ddbg_enable(int dclass, int instance);

// stop logging (class,instance) traces.  -1 is the wildcard which
// squelches all instances from that class.
void ddbg_disable(int dclass, int instance);

void ddbg_trace(int dclass, int instance, char *fmt, ...);

// set the file for traces to be sent to.  if 0, traces
// will be discarded
void ddbg_setfile(FILE *);

#define DDBG_MAX_INSTANCE 256


/* assert functions */

/*
 * If neither a file nor a handler is set (the default), ddbg_assert
 * will abort the program.  If a file has been set
 * (e.g. setfile(stderr)), a message will be written to that file; if
 * the file has been set to 0, no output will be generated.  If a
 * handler has been set, it will be called instead of aborting the
 * program.  
 */

void ddbg_assert_setfile(FILE *);

typedef void(*ddbg_assert_handler)(char *file, 
				   int line, 
				   const char *cond, 
				   const char *func,
				   const char *fmt, ...);

void ddbg_assert_sethandler(ddbg_assert_handler);
void ddbg_assert_msg(char *file, 
		     int line, 
		     const char *cond,
		     const char *func, 
		     const char *fmt, ...);
  
void ddbg_assert_fail(char *file, 
		      int line, 
		      const char *cond,
		      const char *func, 
		      const char *fmt, ...);



void ddbg_assert_printmsg(const char *fmt, ...);


// if we're using remotely recent gcc, we can use __PRETTY_FUNCTION__
// if we're using C99, we can use __func__
// otherwise, just pass 0
#ifdef __GNUC__

#if __GNUC__ > 2 || (__GNUC__ == 2 \
                        && __GNUC_MINOR__ >= (defined __cplusplus ? 6 : 4))
#define __func__ __PRETTY_FUNCTION__
#else 
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
// don't do anything -- C99 has __func__
#endif
#endif
#else 
#define __func__ ((const char *)0)

#endif


/* like assert() modulo above */
#ifndef _DDBG_DISABLE_ASSERTIONS
#define ddbg_assert(cond) do { int _ltassert_cond = (int)(cond); \
if(!_ltassert_cond) { \
ddbg_assert_msg(__FILE__, __LINE__, #cond, __func__, ""); \
ddbg_assert_fail(__FILE__, __LINE__, #cond, __func__, ""); \
} } while(0)
#else
#define ddbg_assert(cond) 
#endif

/* print a message */
#ifndef _DDBG_DISABLE_ASSERTIONS
#define ddbg_assert2(cond,fmt) do { int _ltassert_cond = (int)(cond); \
if(!_ltassert_cond) { \
ddbg_assert_msg(__FILE__, __LINE__, #cond, __func__, fmt); \
ddbg_assert_fail(__FILE__, __LINE__, #cond, __func__, fmt); \
} } while(0)
#else
#define ddbg_assert2(x,y)
#endif

/* like assert() modulo above */
#ifndef _DDBG_DISABLE_ASSERTIONS
//#define ddbg_assert_ptr(cond) do { void* _ltassert_cond = (void*)(cond);
#define ddbg_assert_ptr(cond) do { \
if(!(cond)) { \
ddbg_assert_msg(__FILE__, __LINE__, #cond, __func__, ""); \
ddbg_assert_fail(__FILE__, __LINE__, #cond, __func__, ""); \
} } while(0)
#else
#define ddbg_assert_ptr(cond) 
#endif

/* invoke by e.g. 
 *   ddbg_assert3(cond, ("foo %d", 3));
 */

#ifndef _DDBG_DISABLE_ASSERTIONS
#define ddbg_assert3(cond,msg) do { int _ltassert_cond = (int)(cond);\
if(!_ltassert_cond) { \
 ddbg_assert_msg(__FILE__, __LINE__, #cond, __func__, ""); \
 ddbg_assert_printmsg msg; \
 ddbg_assert_fail(__FILE__, __LINE__, #cond, __func__, ""); \
} } while(0)
#else
#define ddbg_assert3(x,y)
#endif



// your compiler must support C99 variadic macros to use this
#if __STDC_VERSION__ >= 199901L

/* print a message like printf */
#define ddbg_assert4(cond,fmt,...) ((cond) ? 0 : ddbg_assert_msg(__FILE__, __LINE__, #cond, __func__, fmt, __VA_ARGS__))

#endif


#ifdef __cplusplus
}
#endif


#endif /* _LIBTRACE_LIBTRACE_H */






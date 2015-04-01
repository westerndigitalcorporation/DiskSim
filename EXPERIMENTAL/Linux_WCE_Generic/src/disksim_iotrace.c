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
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
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

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */

#include "config.h"

#include "disksim_global.h"
#include "disksim_hptrace.h"
#include "disksim_iotrace.h"
#include "disksim_simresult.h"

static void iotrace_initialize_iotrace_info ()
{
   disksim->iotrace_info = (iotrace_info_t *)DISKSIM_malloc (sizeof(iotrace_info_t));
   bzero ((char *)disksim->iotrace_info, sizeof(iotrace_info_t));

   tracebasetime = 0.0;
   firstio = TRUE;
   lasttime = 0.0;
   basebigtime = -1;
   basesmalltime = -1;
   basesimtime = 0.0;
   validate_lastserv = 0.0;
   validate_nextinter = 0.0;
   accumulated_event_time = 0.0;
   lastaccesstime = 0.0;
}


void iotrace_set_format (char *formatname)
{
   if (disksim->iotrace_info == NULL) {
      iotrace_initialize_iotrace_info();
   }

   disksim->traceendian = _LITTLE_ENDIAN;
   disksim->traceheader = TRUE;
   if (strcmp(formatname, "0") == 0) {
      disksim->traceformat = DEFAULT;
   } else if (strcmp(formatname, "ascii") == 0) {
	/* default ascii trace format */
      disksim->traceformat = ASCII;
   } else if (strcmp(formatname, "raw") == 0) {
	/* format of traces collected at Michigan */
      disksim->traceformat = RAW;
   } else if (strcmp(formatname, "validate") == 0) {
	/* format of disk validation traces */
      disksim->traceformat = VALIDATE;
   } else if (strcmp(formatname, "hpl") == 0) {
	/* format of traces provided by HPLabs for research purposes */
      disksim->traceformat = HPL;
      disksim->traceendian = _BIG_ENDIAN;
   } else if (strcmp(formatname, "hpl2") == 0) {
	/* format of traces provided by HPLabs for research purposes,     */
        /* after they have been modified/combined by the `hplcomb' program */
      disksim->traceformat = HPL;
      disksim->traceendian = _BIG_ENDIAN;
      disksim->traceheader = FALSE;
   } else if (strcmp(formatname, "dec") == 0) {
	/* format of some traces provided by dec for research purposes */
      disksim->traceformat = DEC;
   } else if (strcmp(formatname, "emcsymm") == 0) {
	/* format of Symmetrix traces provided by EMC for research purposes */
      disksim->traceformat = EMCSYMM;
   } else if (strcmp(formatname, "emcbackend") == 0) {
        /* format of Symmetrix backend traces that have been converted to map
	   logical devices to physical devices  */
      disksim->traceformat = EMCBACKEND;
   } else if (strcmp(formatname, "batch") == 0) {
        /* ascii traces with added batch information */
      disksim->traceformat = BATCH;
   } else {
      fprintf(stderr, "Unknown trace format - %s\n", formatname);
      exit(1);
   }
}


static int iotrace_read_space (FILE *tracefile, char *spaceptr, int spacesize)
{
   if (fread(spaceptr, spacesize, 1, tracefile) != 1) {
      return(-1);
   }
   return(0);
}


static int iotrace_read_char (FILE *tracefile, char *charptr)
{
   StaticAssert (sizeof(char) == 1);
   if (fread(charptr, sizeof(char), 1, tracefile) != 1) {
      return(-1);
   }
   return(0);
}


static int iotrace_read_short (FILE *tracefile, short *shortptr)
{
   StaticAssert (sizeof(short) == 2);
   if (fread(shortptr, sizeof(short), 1, tracefile) != 1) {
      return(-1);
   }
   if (disksim->endian != disksim->traceendian) {
      *shortptr = ((*shortptr) << 8) + (((*shortptr) >> 8) & 0xFF);
   }
   return(0);
}


// obsolete routine??
/*

static int iotrace_read_int32 (FILE *tracefile, int32_t *intP)
{
   int i;
   intchar swapval;
   intchar intcharval;

   StaticAssert (sizeof(int) == 4);
   if (fread(&intcharval.value, sizeof(int), 1, tracefile) != 1) {
      return(-1);
   }
   if (disksim->endian != disksim->traceendian) {
      for (i=0; i<sizeof(int); i++) {
         swapval.byte[i] = intcharval.byte[(sizeof(int) - i - 1)];
      }

      fprintf (outputfile, "intptr.value %x, swapval.value %x\n", intcharval.value, swapval.value);

      intcharval.value = swapval.value;
   }
   *intP = intcharval.value;
   return(0);
}
*/


#define iotrace_read_float(a, b) iotrace_read_int32(a, b)


ioreq_event * iotrace_validate_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   char line[201];
   char rw;
   double servtime;

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) new_event);
      return(NULL);
   }
   new_event->time = simtime + (validate_nextinter / (double) 1000);
   if (sscanf(line, "%c %s %d %d %lf %lf\n", 
	      &rw, 
	      validate_buffaction, 
	      &new_event->blkno, 
	      &new_event->bcount, 
	      &servtime, 
	      &validate_nextinter) != 6) 
   {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      ddbg_assert(0);
   }
   validate_lastserv = servtime / (double) 1000;
   if (rw == 'R') {
      new_event->flags = READ;
   } else if (rw == 'W') {
      new_event->flags = WRITE;
   } else {
      fprintf(stderr, "Invalid R/W value: %c\n", rw);
      exit(1);
   }
   new_event->devno = 0;
   new_event->buf = 0;
   new_event->opid = 0;
   new_event->cause = 0;
   new_event->busno = 0;
   new_event->tempint2 = 0;
   new_event->tempint1 = 0;
   validate_lastblkno = new_event->blkno;
   validate_lastbcount = new_event->bcount;
   validate_lastread = new_event->flags & READ;
   return(new_event);
}


static ioreq_event * iotrace_dec_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   assert ("removed for distribution" == 0);
   iotrace_read_space (tracefile, NULL, 0);
   return (NULL);
}


static void iotrace_hpl_srt_convert_flags (ioreq_event *curr)
{
   int flags;

   flags = curr->flags;
   curr->flags = 0;
   if (flags & HPL_READ) {
      curr->flags |= READ;
      hpreads++;
   } else {
      hpwrites++;
   }
   if (!(flags & HPL_ASYNC)) {
      curr->flags |= TIME_CRITICAL;
      if (curr->flags & READ) {
         syncreads++;
      } else {
         syncwrites++;
      }
   }
   if (flags & HPL_ASYNC) {
      if (curr->flags & READ) {
         curr->flags |= TIME_LIMITED;
         asyncreads++;
      } else {
         asyncwrites++;
      }
   }
}


// obsolete routine??
/*
static ioreq_event * iotrace_hpl_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   int32_t size;
   int32_t id;
   int32_t sec;
   int32_t usec;
   int32_t val;
   int32_t junkint;
   unsigned int failure = 0;

   while (TRUE) {
      failure |= iotrace_read_int32(tracefile, &size);
      failure |= iotrace_read_int32(tracefile, &id);
      failure |= iotrace_read_int32(tracefile, &sec);
      failure |= iotrace_read_int32(tracefile, &usec);
      if (failure) {
         addtoextraq((event *) new_event);
         return(NULL);
      }
      if (((id >> 16) < 1) || ((id >> 16) > 4)) {
         fprintf(stderr, "Error in trace format - id %x\n", id);
         exit(1);
      }
      if (((id & 0xFFFF) != HPL_SHORTIO) && ((id & 0xFFFF) != HPL_SUSPECTIO)) {
         fprintf(stderr, "Unexpected record type - %x\n", id);
         exit(1);
      }
      new_event->time = (double) sec * (double) MILLI;
      new_event->time += (double) usec / (double) MILLI;

      if ((disksim->traceheader == FALSE) && (new_event->time == 0.0)) {
         tracebasetime = simtime;
      }

      failure |= iotrace_read_int32(tracefile, &val);     traced request start time
      new_event->tempint1 = val;
      failure |= iotrace_read_int32(tracefile, &val);     traced request stop time
      new_event->tempint2 = val;
      new_event->tempint2 -= new_event->tempint1;
      failure |= iotrace_read_int32(tracefile, &val);
      new_event->bcount = val;
      if (new_event->bcount & 0x000001FF) {
         fprintf(stderr, "HPL request for non-512B multiple size: %d\n", new_event->bcount);
         exit(1);
      }
      new_event->bcount = new_event->bcount >> 9;
      failure |= iotrace_read_int32(tracefile, &val);
      new_event->blkno = val;
      failure |= iotrace_read_int32(tracefile, &val);
      new_event->devno = (val >> 8) & 0xFF;
      failure |= iotrace_read_int32(tracefile, &val);        drivertype
      failure |= iotrace_read_int32(tracefile, &val);        cylno
       for convenience and historical reasons, this cast is being allowed
       (the value is certain to be less than 32 sig bits, and will not be
       used as a pointer).
      new_event->buf = (void *) val;
      failure |= iotrace_read_int32(tracefile, &val);
      new_event->flags = val;
      iotrace_hpl_srt_convert_flags(new_event);
      failure |= iotrace_read_int32(tracefile, &junkint);            info
      size -= 13 * sizeof(int32_t);
      if ((id >> 16) == 4) {
         failure |= iotrace_read_int32(tracefile, &val);   queuelen
         new_event->slotno = val;
         size -= sizeof(int32_t);
      }
      if ((id & 0xFFFF) == HPL_SUSPECTIO) {
         failure |= iotrace_read_int32(tracefile, &junkint);     susflags
         size -= sizeof(int32_t);
      }
      if (failure) {
         addtoextraq((event *) new_event);
         return(NULL);
      }
      if (size) {
         fprintf(stderr, "Unmatched size for record - %d\n", size);
         exit(1);
      }
      new_event->cause = 0;
      new_event->opid = 0;
      new_event->busno = 0;
      if ((id & 0xFFFF) == HPL_SHORTIO) {
         return(new_event);
      }
   }
}
*/


static int iotrace_month_convert (char *monthstr, int year)
{
   if (strcmp(monthstr, "Jan") == 0) {
      return(0);
   } else if (strcmp(monthstr, "Feb") == 0) {
      return(31);
   } else if (strcmp(monthstr, "Mar") == 0) {
      return((year % 4) ? 59 : 60);
   } else if (strcmp(monthstr, "Apr") == 0) {
      return((year % 4) ? 90 : 91);
   } else if (strcmp(monthstr, "May") == 0) {
      return((year % 4) ? 120 : 121);
   } else if (strcmp(monthstr, "Jun") == 0) {
      return((year % 4) ? 151 : 152);
   } else if (strcmp(monthstr, "Jul") == 0) {
      return((year % 4) ? 181 : 182);
   } else if (strcmp(monthstr, "Aug") == 0) {
      return((year % 4) ? 212 : 213);
   } else if (strcmp(monthstr, "Sep") == 0) {
      return((year % 4) ? 243 : 244);
   } else if (strcmp(monthstr, "Oct") == 0) {
      return((year % 4) ? 273 : 274);
   } else if (strcmp(monthstr, "Nov") == 0) {
      return((year % 4) ? 304 : 305);
   } else if (strcmp(monthstr, "Dec") == 0) {
      return((year % 4) ? 334 : 335);
   }
   assert(0);
   return(-1);
}


static double iotrace_raw_get_hirestime (int bigtime, int smalltime)
{
   unsigned int loresticks;
   int small, turnovers;
   int smallticks;

   if (basebigtime == -1) {
      basebigtime = bigtime;
      basesmalltime = smalltime;
      basesimtime = 0.0;
   } else {
      small = (basesmalltime - smalltime) & 0xFFFF;
      loresticks = (bigtime - basebigtime) * 11932 - small;
      turnovers = (int) (((double) loresticks / (double) 65536) + (double) 0.5);
      smallticks = turnovers * 65536 + small;
      basebigtime = bigtime;
      basesmalltime = smalltime;
      basesimtime += (double) smallticks * (double) 0.000838574;
   }
   return(basesimtime);
}


/* kept mainly as an example */
/*

static ioreq_event * iotrace_raw_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   int bigtime;
   short small;
   int smalltime;
   int failure = 0;
   char order, crit;
   double schedtime, donetime;
   int32_t val;

   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   failure |= iotrace_read_short(tracefile, &small);
   smalltime = ((int) small) & 0xFFFF;
   new_event->time = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_short(tracefile, &small);
   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   smalltime = ((int) small) & 0xFFFF;
   schedtime = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   failure |= iotrace_read_short(tracefile, &small);
   smalltime = ((int) small) & 0xFFFF;
   donetime = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_char(tracefile, &order);
   failure |= iotrace_read_char(tracefile, &crit);
   if (crit) {
      new_event->flags |= TIME_CRITICAL;
   }
   failure |= iotrace_read_int32(tracefile, &val);
   new_event->bcount = val >> 9;
   failure |= iotrace_read_int32(tracefile, &val);
   new_event->blkno = val;
   failure |= iotrace_read_int32(tracefile, &val);
   new_event->devno = val;
   failure |= iotrace_read_int32(tracefile, &val);
   new_event->flags = val & READ;
   new_event->cause = 0;
   new_event->buf = 0;
   new_event->opid = 0;
   new_event->busno = 0;
   new_event->tempint1 = (int)((schedtime - new_event->time) * (double) 1000);
   new_event->tempint2 = (int)((donetime - schedtime) * (double) 1000);
   if (failure) {
      addtoextraq((event *) new_event);
      new_event = NULL;
   }
   return(new_event);
}
*/


static ioreq_event * iotrace_emcsymm_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   char line[201];
   char operation[15];
   unsigned int director;

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) new_event);
      return(NULL);
   }
   if (sscanf(line, "%lf %s %x %x %d %d\n", &new_event->time, operation, &director, &new_event->devno, &new_event->blkno, &new_event->bcount) != 6) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
   if (!strcasecmp(operation,"Read")) {
      new_event->flags = READ;
   } else if (!strcasecmp(operation,"Write")) {
      new_event->flags = WRITE;
   } else {
      fprintf(stderr, "Unknown operation: %s in iotrace event\n",operation);
      fprintf(stderr, "line: %s", line);
      exit(1);
   }
   new_event->buf = 0;
   new_event->opid = 0;
   new_event->busno = 0;
   new_event->cause = 0;
   return(new_event);
}

static ioreq_event * iotrace_emcbackend_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   char line[201];
   char operation[15];
   unsigned int director;
   char bus[2];
   unsigned int disk, hyper;

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) new_event);
      return(NULL);
   }

   if (sscanf(line, "%lf %s %x %x %d %d %s %d %d\n", 
              &new_event->time, operation, &director, &hyper, &new_event->blkno, &new_event->bcount, bus, &disk, &new_event->devno) != 9) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      exit(0);
   }
   if (!strcasecmp(operation,"Read")) {
      new_event->flags = READ;
   } else if (!strcasecmp(operation,"Write")) {
      new_event->flags = WRITE;
   } else {
      fprintf(stderr, "Unknown operation: %s in iotrace event\n",operation);
      fprintf(stderr, "line: %s", line);
      exit(0);
   }

   new_event->time *= 1000.0;  /* emc trace times are in seconds!!  */

   new_event->buf = 0;
   new_event->opid = 0;
   new_event->busno = 0;
   new_event->cause = 0;
   return(new_event);
}


static ioreq_event * iotrace_ascii_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   char line[201];

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) new_event);
      return(NULL);
   }

#ifdef DEBUG_IOTRACE
   fprintf( outputfile, "*** %f: ASCII Line: %s", simtime, line );
   fflush(outputfile);
#endif

   if (sscanf(line, "%lf %d %d %d %x\n", &new_event->time, &new_event->devno, &new_event->blkno, &new_event->bcount, &new_event->flags) != 5) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
   if (new_event->flags & ASYNCHRONOUS) {
      new_event->flags |= (new_event->flags & READ) ? TIME_LIMITED : 0;
   } else if (new_event->flags & SYNCHRONOUS) {
      new_event->flags |= TIME_CRITICAL;
   }
   new_event->buf = 0;
   new_event->opid = 0;
   new_event->busno = 0;
   new_event->cause = 0;
   return(new_event);
}

static ioreq_event * iotrace_batch_get_ioreq_event (FILE *tracefile, ioreq_event *new_event)
{
   char line[201];

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) new_event);
      return(NULL);
   }
   if (sscanf(line, "%lf %d %d %d %x %d\n", &new_event->time, &new_event->devno, &new_event->blkno, &new_event->bcount, &new_event->flags, &new_event->batchno) != 6) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
   if (new_event->flags & ASYNCHRONOUS) {
      new_event->flags |= (new_event->flags & READ) ? TIME_LIMITED : 0;
   } else if (new_event->flags & SYNCHRONOUS) {
      new_event->flags |= TIME_CRITICAL;
   }
   if (new_event->flags & BATCH_COMPLETE) {
     new_event->batch_complete = 1;
   } else {
     new_event->batch_complete = 0;
   }

   new_event->buf = 0;
   new_event->opid = 0;
   new_event->busno = 0;
   new_event->cause = 0;
   return(new_event);
}


ioreq_event * iotrace_get_ioreq_event (FILE *tracefile, int traceformat, ioreq_event *temp)
{
   switch (traceformat) {
      
   case ASCII:
      temp = iotrace_ascii_get_ioreq_event(tracefile, temp);
      break;
      
//   case RAW:
//      temp = iotrace_raw_get_ioreq_event(tracefile, temp);
//      break;
      
//   case HPL:
//      temp = iotrace_hpl_get_ioreq_event(tracefile, temp);
//      break;

   case DEC:
      temp = iotrace_dec_get_ioreq_event(tracefile, temp);
      break;

   case VALIDATE:
      temp = iotrace_validate_get_ioreq_event(tracefile, temp);
      break;

   case EMCSYMM:
      temp = iotrace_emcsymm_get_ioreq_event(tracefile, temp);
      break;

   case EMCBACKEND:
      temp = iotrace_emcbackend_get_ioreq_event(tracefile, temp);
      break;

   case BATCH:
      temp = iotrace_batch_get_ioreq_event(tracefile, temp);
      break;

   default:
      fprintf(stderr, "Unknown traceformat in iotrace_get_ioreq_event - %d\n", traceformat);
      exit(1);
   }

   return ((ioreq_event *)temp);
}


static void iotrace_hpl_srt_tracefile_start (char *tracedate)
{
   char crap[40];
   char monthstr[40];
   int day;
   int hour;
   int minute;
   int second;
   int year;

   if (sscanf(tracedate, "%s\t= \"%s %s %d %d:%d:%d %d\";\n", crap, crap, monthstr, &day, &hour, &minute, &second, &year) != 8) {
      fprintf(stderr, "Format problem with 'tracedate' line in HPL trace - %s\n", tracedate);
      exit(1);
   }
   if (baseyear == 0) {
      baseyear = year;
   }
   day = day + iotrace_month_convert(monthstr, year);
   if (year != baseyear) {
      day += (baseyear % 4) ? 365 : 366;
   }
   if (baseday == 0) {
      baseday = day;
   }
   second = second + (60 * minute) + (3600 * hour) + (86400 * (day - baseday));
   if (basesecond == 0) {
      basesecond = second;
   }
   second -= basesecond;
   tracebasetime += (double) 1000 * (double) second;
}


static void iotrace_hpl_initialize_file (FILE *tracefile, int print_tracefile_header)
{
   char letter = '0';
   char line[201];
   char linetype[40];

   if (disksim->traceheader == FALSE) {
      return;
   }
   while (1) {
      if (fgets(line, 200, tracefile) == NULL) {
         fprintf(stderr, "No 'tracedate' line in HPL trace\n");
         exit(1);
      }
      sscanf(line, "%s", linetype);
      if (strcmp(linetype, "tracedate") == 0) {
         break;
      }
   }
   iotrace_hpl_srt_tracefile_start(line);
   while (letter != 0x0C) {
      if (fscanf(tracefile, "%c", &letter) != 1) {
         fprintf(stderr, "End of header information never found - end of file\n");
         exit(1);
      }
      if ((print_tracefile_header) && (letter != 0x0C)) {
         printf("%c", letter);
      }
   }
}


void iotrace_initialize_file (FILE *tracefile, int traceformat, int print_tracefile_header)
{
   if (traceformat == HPL) {
      iotrace_hpl_initialize_file(tracefile, print_tracefile_header);
   }
}


void iotrace_printstats (FILE *outfile)
{
   if (disksim->iotrace_info == NULL) {
      return;
   }
   if (hpreads | hpwrites) {
      fprintf (outfile, "\n");
      fprintf(outfile, "Total reads:    \t%d\t%5.2f\n", hpreads, ((double) hpreads / (double) (hpreads + hpwrites)));
      fprintf(outfile, "Total writes:   \t%d\t%5.2f\n", hpwrites, ((double) hpwrites / (double) (hpreads + hpwrites)));
      fprintf(outfile, "Sync Reads:  \t%d\t%5.2f\t%5.2f\n", syncreads, ((double) syncreads / (double) (hpreads + hpwrites)), ((double) syncreads / (double) hpreads));
      fprintf(outfile, "Sync Writes: \t%d\t%5.2f\t%5.2f\n", syncwrites, ((double) syncwrites / (double) (hpreads + hpwrites)), ((double) syncwrites / (double) hpwrites));
      fprintf(outfile, "Async Reads: \t%d\t%5.2f\t%5.2f\n", asyncreads, ((double) asyncreads / (double) (hpreads + hpwrites)), ((double) asyncreads / (double) hpreads));
      fprintf(outfile, "Async Writes:\t%d\t%5.2f\t%5.2f\n", asyncwrites, ((double) asyncwrites / (double) (hpreads + hpwrites)), ((double) asyncwrites / (double) hpwrites));
   }
}

// End of file


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



/* hplcomb is a simple program for combining multiple separate, but */
/* related, HPL trace files into a single combined trace file (with  */
/* the request arrival times updated appropriately and the HPL trace */
/* file headers stripped).  The one input parameter is the name of  */
/* a file that lists the trace files to be combined (one per line,  */
/* in ascending order of how they should appear in the resulting    */
/* combined trace.                                                  */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include<string.h>

/* must enable on Suns and Alphas */
#if 1
#define u_int32_t unsigned int
#define int32_t int
#endif

typedef union {
   int32_t	value;
   char		byte[4];
} intchar;

#define TRUE	1
#define FALSE	0

#define _BIG_ENDIAN	1
#define _LITTLE_ENDIAN	2

/* Trace Formats */

#define NCR_ASCII	1
#define HPL_SRT		2
#define DEC_RAMAKR	3
#define DEFAULT		NCR_ASCII

int endian = _LITTLE_ENDIAN;
int traceformat = NCR_ASCII;
int traceendian = _LITTLE_ENDIAN;
int tracebatch = FALSE;
int tracecompress = FALSE;
int print_tracefile_header = FALSE;

FILE *iotracefile = NULL;
FILE *iotracebatch = NULL;

char iotracefilename[200];

int baseyear = 0;
int baseday = 0;
int basesecond = 0;
int currentsecond = 0;


int io_month_convert( char *monthstr, int year )
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
   assert (0);
   return (-1);
}


void io_hpl_srt_tracefile_start( char *tracedate )
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
   day = day + io_month_convert(monthstr, year);
   if (year != baseyear) {
      day += (baseyear % 4) ? 365 : 366;
   }
   if (baseday == 0) {
      baseday = day;
   }
   second += (60 * minute) + (3600 * hour) + (86400 * (day - baseday));
   if (basesecond == 0) {
      basesecond = second;
   }
   currentsecond = second - basesecond;
}


void hpl_srt_initialize_file( FILE *iotracefile )
{
   char letter = '0';
   char line[201];
   char linetype[40];

   while (1) {
      if (fgets(line, 200, iotracefile) == NULL) {
         fprintf(stderr, "No 'tracedate' line in HPL trace\n");
         exit(1);
      }
      sscanf(line, "%s", linetype);
      if (strcmp(linetype, "tracedate") == 0) {
         break;
      }
   }
   io_hpl_srt_tracefile_start(line);
   while (letter != 0x0C) {
      if (fscanf(iotracefile, "%c", &letter) != 1) {
	 fprintf(stderr, "End of header information never found - end of file\n");
	 exit(1);
      }
   }
}


int trace_batch_initialize( FILE *batchfile, FILE **fileptr, char *tracefilename )
{
   char commandname[200];

   if ((*fileptr) && (tracecompress)) {
      sprintf(commandname, "compress %s &", tracefilename);
      system(commandname);
   }
   fclose(*fileptr);
   if (fscanf(batchfile, "%s\n", tracefilename) != 1) {
      return(0);
   }
   if (tracecompress) {
      sprintf(commandname, "uncompress %s", tracefilename);
      system(commandname);
   }
   if ((*fileptr = fopen(tracefilename,"r")) == NULL) {
      fprintf(stderr, "Batched tracefile %s cannot be opened for read access\n", tracefilename);
      exit(1);
   }
   hpl_srt_initialize_file(*fileptr);
   return(1);
}


int main( int argc, char **argv )
{
   int recordcnt = 0;
   intchar doubleword;
   intchar converter;

   assert (sizeof(intchar) == 4);
   if (argc != 2) {
      fprintf(stderr,"Usage: %s batchfile\n", argv[0]);
      exit(1);
   }
   if ((iotracebatch = fopen(argv[1],"r")) == NULL) {
      fprintf(stderr,"%s cannot be opened for read access\n", argv[1]);
      exit(1);
   }
   traceformat = HPL_SRT;
   traceendian = _BIG_ENDIAN;
   tracebatch = TRUE;
   tracecompress = TRUE;
   while (trace_batch_initialize(iotracebatch, &iotracefile, iotracefilename)) {
      while (fread(&doubleword.value, sizeof(int32_t), 1, iotracefile) == 1) {
	 if ((recordcnt % 14) == 2) {
	    converter.byte[0] = doubleword.byte[3];
	    converter.byte[1] = doubleword.byte[2];
	    converter.byte[2] = doubleword.byte[1];
	    converter.byte[3] = doubleword.byte[0];
	    doubleword.value = currentsecond + converter.value;
	    converter.byte[0] = doubleword.byte[3];
	    converter.byte[1] = doubleword.byte[2];
	    converter.byte[2] = doubleword.byte[1];
	    converter.byte[3] = doubleword.byte[0];
	    doubleword.value = converter.value;
	 }
	 fwrite(&doubleword.value, sizeof(int), 1, stdout);
         recordcnt++;
      }
      if (recordcnt % 14) {
         fprintf(stderr, "Partial record at end of tracefile - %d\n", (recordcnt % 14));
	 exit(1);
      }
   }

   fclose(iotracefile);
   fclose(iotracebatch);
   exit(0);
}


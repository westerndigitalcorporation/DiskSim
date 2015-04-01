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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define RMS_INCR 0.0001

#define MAX_POINTS	10000

double line1_x[MAX_POINTS];
double line1_y[MAX_POINTS];
double line2_x[MAX_POINTS];
double line2_y[MAX_POINTS];

void get_line( char *filename, double *x, double *y, char *distname)
{
   FILE *infile;
   char line[201];
   double junk;
   int i = 0;

   if (strcmp(filename, "stdin") == 0) {
      infile = stdin;
   } else if ((infile = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "Can't open infile %s\n", filename);
      exit(1);
   }
   while ((fgets(line, 200, infile)) && (strcmp(line, distname))) {
   }
   while (fgets(line, 200, infile)) {
      if (sscanf(line, "%lf %lf %lf %lf", &x[i], &junk, &junk, &y[i]) != 4) {
	 fprintf(stderr, "Invalid line in distribution: %s\n", line);
	 exit(1);
      }
      if (y[i] >= 1.0) {
	 return;
      }
      if (i >= MAX_POINTS) {
         fprintf (stderr, "Too many lines in distribution: %d\n", i);
         exit(1);
      }
      i++;
   }
}


double compute_rms( double *x1, double *y1, double *x2, double *y2 )
{
   int count = 0;
   double runval = 0.0;
   double runsquares = 0.0;
   int i1 = 0;
   int i2 = 0;
   double pnt = RMS_INCR;
   double yfrac;
   double x1pnt, x2pnt;

   for (; pnt <= 1.0; pnt += RMS_INCR) {
      while (y1[i1] < pnt) {
	 i1++;
      }
      while (y2[i2] < pnt) {
	 i2++;
      }
      yfrac = (pnt - y1[(i1-1)]) / (y1[i1] - y1[(i1-1)]);
      x1pnt = x1[(i1-1)] + (yfrac * (x1[i1] - x1[(i1-1)]));
      yfrac = (pnt - y2[(i2-1)]) / (y2[i2] - y2[(i2-1)]);
      x2pnt = x2[(i2-1)] + (yfrac * (x2[i2] - x2[(i2-1)]));
      runval += fabs(x1pnt - x2pnt);
      runsquares += (x1pnt - x2pnt) * (x1pnt - x2pnt);
      count++;
   }
   return(sqrt(runsquares / (double) count));
}


int main( int argc, char **argv )
{
   int diskno;
   char distname[201];
   double rms;
   int rootnames;
   int doforservs;

   if (argc != 6) {
      fprintf(stderr, "Usage: %s filename1 fileroot2 diskno rootnames doforservs\n", argv[0]);
      exit(1);
   }
   if (sscanf(argv[3], "%d", &diskno) != 1) {
      fprintf(stderr, "Invalid disk number: %s\n", argv[3]);
      exit(1);
   }
   if (sscanf(argv[5], "%d", &doforservs) != 1) {
      fprintf(stderr, "Invalid value for doforservs: %s\n", argv[3]);
      exit(1);
   }
   if (doforservs) {
      if (diskno == -1) {
         sprintf(distname, "IOdriver Physical access time distribution\n");
      } else {
         sprintf(distname, "IOdriver #0 device #%d Physical access time distribution\n", diskno);
      }
   } else {
      if (diskno == -1) {
         sprintf(distname, "IOdriver Response time distribution\n");
      } else {
         sprintf(distname, "IOdriver #0 device #%d Response time distribution\n", diskno);
      }
   }
   get_line(argv[1], line1_x, line1_y, distname);
   if (sscanf(argv[4], "%d", &rootnames) != 1) {
      fprintf(stderr, "Invalid number of rootnames: %s\n", argv[4]);
      exit(1);
   }
   if (rootnames == -1) {
      sprintf(distname, "VALIDATE Trace access time distribution\n");
      rootnames = 0;
   }
   if (rootnames == 0) {
      get_line(argv[2], line2_x, line2_y, distname);
      rms = compute_rms(line1_x, line1_y, line2_x, line2_y);
      printf("rms = %f\n", rms);
   } else {
      double runrms = 0.0;
      double runsquares = 0.0;
      char filename[51];
      int i;

      for (i=1; i<=rootnames; i++) {
	 sprintf(filename, "%s.%d", argv[2], i);
         get_line(filename, line2_x, line2_y, distname);
         rms = compute_rms(line1_x, line1_y, line2_x, line2_y);
	 runrms += rms;
	 runsquares += rms * rms;
      }
      runrms /= (double) rootnames;
      runsquares = (runsquares / (double) rootnames) - (runrms * runrms);
      runsquares = (runsquares > 0.0) ? sqrt(runsquares) : 0.0;
      printf("rms = %f (%f)\n", runrms, runsquares);
   }
   exit(0);
}


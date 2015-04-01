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

/* Portions of the code in this file taken from the DiskSim source code. */

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

#include "dixtrac.h"
#include "handle_error.h"
#include "paramfile_gen.h"
#include "extract_params.h"

static char value[255];

extern diskinfo_t *thedisk;

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
write_param_file(char *model_name) 
{
  
  FILE *fp = stdout;
  char output_fname[255];
  char brandname[255];

  int numblocks = thedisk->numblocks;

  get_disk_label(brandname);

  sprintf(output_fname,"%s.%s",model_name,PARAM_EXT); 
  fp = fopen(output_fname,"w");
  assert(fp != NULL);

  {
    int tmp = 0x11223344;
    char *tmp2 = (char *) &tmp;
    strcpy(value,
	   (tmp2[0]==0x44) ? "Little" : ((tmp2[0] == 0x11) ? "Big" : "-1"));
  }
  write_entry(fp,"\nByte Order (Endian)",value);
   
  sprintf(value,"%s",( 0 ? "PID" : "42" ));
  write_entry(fp,"Init Seed",value);

  sprintf(value,"%s",( 0 ? "PID" :"42" ));
  write_entry(fp,"Real Seed",value);

  sprintf(value,"%.1f %s",0.0,( 1 ? "seconds" : "I/Os" ));
  write_entry(fp,"Statistic warm-up period",value);

  sprintf(value,"%.1f %s",0.0,( 1 ? "seconds" : "I/Os" ));
  write_entry(fp,"Checkpoint to null every",value);

  sprintf(value,"%s","statdefs.validgrow");
  write_entry(fp,"Stat (dist) definition file",value);

  sprintf(value,"%d",0);
  write_entry(fp,"Output file for trace of I/O requests simulated",value);


   fprintf(fp, "\nI/O Subsystem Input Parameters\n");
   fprintf(fp, "------------------------------\n");


   fprintf(fp, "\nPRINTED I/O SUBSYSTEM STATISTICS\n\n");

   pars_iodriver_read_toprints(fp);
   pars_bus_read_toprints(fp);
   pars_controller_read_toprints(fp);
   pars_disk_read_toprints(fp);

   fprintf(fp, "\nGENERAL I/O SUBSYSTEM PARAMETERS\n\n");

   pars_io_read_generalparms(fp);


   fprintf(fp, "\nCOMPONENT SPECIFICATIONS\n");

   pars_iodriver_read_specs(fp);
   pars_bus_read_specs(fp);
   pars_controller_read_specs(fp);
   pars_disk_read_specs(fp,brandname,model_name);

   fprintf(fp, "\nPHYSICAL ORGANIZATION\n");

   pars_iodriver_read_physorg(fp);
   pars_controller_read_physorg(fp);
   pars_bus_read_physorg(fp);


   fprintf(fp, "\nSYNCHRONIZATION\n");

   pars_disk_read_syncsets(fp);

   fprintf(fp, "\nLOGICAL ORGANIZATION\n");

   pars_iodriver_read_logorg(fp,numblocks);
   pars_controller_read_logorg(fp);


   pars_pf_readparams(fp,numblocks);

   fclose(fp);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
pars_iodriver_read_toprints(FILE *fp) 
{
   /* iodriver_read_toprints(parfile); */
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver size stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp,"Print driver locality stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp,"Print driver blocking stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp,"Print driver interference stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver queue stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver crit stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver idle stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver intarr stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver streak stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver stamp stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp,"Print driver per-device stats?",value);
   /* end of    iodriver_read_toprints(parfile); */
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_bus_read_toprints(FILE *fp)
{
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print bus idle stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print bus arbwait stats?",value);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_bus_read_specs(FILE *fp)
{
   int numbuses = 2;
   int i;

   fprintf(fp,"\n");
   sprintf(value,"%d",numbuses);
   write_entry(fp,"Number of buses",value);
   for(i=1;i<=numbuses;i++) {
      fprintf(fp, "\nBus Spec #%d\n",i);

      sprintf(value,"%d",1);
      write_entry(fp,"# buses with Spec",value);
      sprintf(value,"%d",1);
      write_entry(fp, "Bus Type",value);
      sprintf(value,"%d",1);
      write_entry(fp, "Arbitration type",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Arbitration time",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Read block transfer time",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Write block transfer time",value);
      sprintf(value,"%d",( i==numbuses ? 1 : 0));
      write_entry(fp, "Print stats for bus",value);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_bus_read_physorg(FILE *fp)
{
   int i,j;
   int numbuses = 2;
   for(i=1;i<=numbuses;i++) {
      fprintf(fp, "\nBus #%d\n",i);
      fprintf(fp, "# of utilized slots: %d\n",i);
      for(j=1;j<=i;j++) {
	sprintf(value,(i==j ? "1" : "#-1"));
	fprintf(fp, "Slots: %s %s\n", 
		(j==1 ? "Controllers" : "Devices"),value);
      }
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_controller_read_toprints(FILE *fp)
{
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller cache stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller size stats?",value); 
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller locality stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller blocking stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller interference stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller queue stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller crit stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller idle stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller intarr stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller streak stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller stamp stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print controller per-device stats?",value);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_controller_read_specs(FILE *fp)
{
   int i;
   int numcontrollers = 1;

   fprintf(fp,"\n");
   sprintf(value,"%d",numcontrollers);
   write_entry(fp,"Number of controllers", value);

   for(i=1;i<=numcontrollers;i++) {
      fprintf(fp, "\nController Spec #%d\n",i);

      sprintf(value,"%d",1);
      write_entry(fp, "# controllers with Spec",value);
      sprintf(value,"%d",1);
      write_entry(fp, "Controller Type",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Scale for delays",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Bulk sector transfer time",value);
      sprintf(value,"%d",0);
      write_entry(fp, "Maximum queue length",value);
      sprintf(value,"%d",1);
      write_entry(fp, "Print stats for controller",value);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_controller_read_physorg(FILE *fp)
{
   int i;
   int numcontrollers = 1;

   /* fprintf(fp,"\n"); */
   /* sprintf(value,"%d",numcontrollers); */
   /* write_entry(fp,"Number of controllers", value); */

   for(i=1;i<=numcontrollers;i++) {
      fprintf(fp, "\nController #%d\n",i);
      fprintf(fp, "# of connected buses: %d\n",1);
      fprintf(fp, "Connected buses: %s\n","#+1");
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_controller_read_logorg(FILE *fp)
{
  sprintf(value,"%d",0);
  write_entry(fp, "\n# of controller-level organizations",value);
  fprintf(fp,"\n");
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_disk_read_toprints(FILE *fp)
{
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device queue stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device crit stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device idle stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device intarr stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device size stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print device seek stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print device latency stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print device xfer stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print device acctime stats?",value);
   sprintf(value,"%d",0);
   write_parfile_entry(fp, "Print device interfere stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print device buffer stats?",value);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_disk_read_specs(FILE *fp,char *brandname, char *model_name)
{
   int i;
   int numdisks = 1;

   fprintf(fp,"\n");
   sprintf(value,"%d",numdisks);
   write_entry(fp,"Number of storage devices", value);

   for(i=1;i<=numdisks;i++) {
      fprintf(fp, "\nDevice Spec #%d\n",i);

      sprintf(value,"%d",1);
      write_entry(fp, "# devices with Spec",value);

      sprintf(value,"%s","disk");
      write_entry(fp, "Device type for Spec",value);

      sprintf(value,"%s",brandname);
      write_entry(fp, "Disk brand name",value);
      sprintf(value,"%s.%s",model_name,DATA_EXT);
      write_entry(fp, "Disk specification file",value);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_disk_read_syncsets(FILE *fp)
{
   int i;
   int numsyncsets = 0;

   fprintf(fp, "\nNumber of synchronized sets: %d\n", numsyncsets);

   for (i=1; i<=numsyncsets; i++) {
      fprintf(fp,"\n");
      fprintf(fp, "Number of disks in set #%d: %d\n",0,0);
      fprintf(fp, "Synchronized disks: %d-%d\n",0,0);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_io_read_generalparms(FILE *fp)
{
   int i,tracemappings=0;

   sprintf(value,"%.1f",1.0);
   write_entry(fp, "I/O Trace Time Scale",value);
   sprintf(value,"%d",tracemappings);
   write_entry(fp, "Number of I/O Mappings",value);

   for (i=0; i<tracemappings; i++) {
     fprintf(fp,"Mapping: %x %x %d %d %d\n",0,0,0,0,0);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_iodriver_read_specs(FILE *fp)
{
   int i;
   int numiodrivers = 1;

   sprintf(value,"%d",numiodrivers);
   write_entry(fp,"\nNumber of device drivers", value);

   for(i=1;i<=numiodrivers;i++) {
      fprintf(fp, "\nDevice Driver Spec #%d\n", i);
      sprintf(value,"%d",1);
      write_entry(fp,"# drivers with Spec",value);
      sprintf(value,"%d",1);
      write_entry(fp,"Device driver type",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp,"Constant access time",value);

      pars_ioqueue_readparams(fp);

      sprintf(value,"%d",1);
      write_entry(fp,"Use queueing in subsystem",value);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_ioqueue_readparams(FILE *fp)
{
   sprintf(value,"%d",1);
   write_entry(fp,"Scheduling policy",value);
   sprintf(value,"%d",1);
   write_entry(fp, "Cylinder mapping strategy",value);
   sprintf(value,"%.1f",0.0);
   write_entry(fp, "Write initiation delay",value);
   sprintf(value,"%.1f",0.0);
   write_entry(fp,"Read initiation delay", value);
   sprintf(value,"%d",0);
   write_entry(fp,"Sequential stream scheme",value);
   sprintf(value,"%d",128);
   write_entry(fp,"Maximum concat size",value);
   sprintf(value,"%d",0);
   write_entry(fp,"Overlapping request scheme",value);
   sprintf(value,"%d",0);
   write_entry(fp, "Sequential stream diff maximum",value);
   sprintf(value,"%d",0);
   write_entry(fp, "Scheduling timeout scheme",value);
   sprintf(value,"%d",6);
   write_entry(fp, "Timeout time/weight",value);
   sprintf(value,"%d",4);
   write_entry(fp, "Timeout scheduling",value);
   sprintf(value,"%d",0);
   write_entry(fp, "Scheduling priority scheme",value);
   sprintf(value,"%d",4);
   write_entry(fp, "Priority scheduling",value);
}



/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_iodriver_read_physorg(FILE *fp)
{
   int numbuses = 1;
   int i;

   for(i=1;i<=numbuses;i++) {
      fprintf(fp, "\nDriver #%d\n",i);
      fprintf(fp, "# of connected buses: %d\n",1);
      fprintf(fp, "Connected buses: %d\n",1);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_iodriver_read_logorg(FILE *fp,int numblocks)
{
  int numsysorgs=1;

  fprintf(fp,"\n");
  sprintf(value,"%d",numsysorgs);
  write_entry(fp, "# of system-level organizations",value);
  pars_logorg_readparams(fp,numblocks);

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_logorg_readparams(FILE *fp,int numblocks)
{
   int orgno = 1;
   char byparts[10] = "Parts";
   char type[20] = "Asis";
   char redun[30] = "Noredun";
   char comp[10] = "Whole";
   
   fprintf(fp,"\n");
   fprintf(fp,"Organization #%d: %s %s %s %s\n",orgno,byparts,type,redun,comp);

   if (strcmp(comp, "Whole") == 0) {
     sprintf(value,"%d",1);
     write_entry(fp, "Number of devices",value);
     sprintf(value,"%d-%d",1,1);
     write_entry(fp, "Devices",value);
   }
   sprintf(value,"%d",1);
   write_entry(fp, "High-level device number",value);
   
   if (strcmp(type, "Asis") == 0) {
   } else if (strcmp(type, "Striped") == 0) {
   } else if (strcmp(type, "Random") == 0) {
   } else if (strcmp(type, "Ideal") == 0) {
   }

   sprintf(value,"%d",numblocks);
   write_entry(fp,  "Stripe unit (in sectors)",value);

   if (strcmp(redun, "Noredun") == 0) {
   } else if (strcmp(redun, "Shadowed") == 0) {
   } else if (strcmp(redun, "Parity_disk") == 0) {
   } else if (strcmp(redun, "Parity_rotated") == 0) {
   }

   sprintf(value,"%d",0);
   write_entry(fp, "Synch writes for safety",value);
   sprintf(value,"%d",2);
   write_entry(fp, "Number of copies",value);
   sprintf(value,"%d",6);
   write_entry(fp, "Copy choice on read",value);
   sprintf(value,"%.1f",0.5);
   write_entry(fp, "RMW vs. reconstruct",value);
   sprintf(value,"%d",64);
   write_entry(fp, "Parity stripe unit",value);
   sprintf(value,"%d",1);
   write_entry(fp, "Parity rotation type",value);
   sprintf(value,"%.6f",0.0);
   write_entry(fp, "Time stamp interval",value);
   sprintf(value,"%.6f",60000.0);
   write_entry(fp, "Time stamp start time",value);
   sprintf(value,"%.6f",10000000000.0);
   write_entry(fp, "Time stamp stop time",value);
   sprintf(value,"%s","stamps");
   write_entry(fp,"Time stamp file name",value);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_pf_readparams(FILE *fp,int numblocks)
{

   fprintf(fp, "\nProcess-Flow Input Parameters\n");
   fprintf(fp, "-----------------------------\n");

   fprintf (fp, "\nPRINTED PROCESS-FLOW STATISTICS\n\n");

   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print per-process stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print per-CPU stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print all interrupt stats?",value);
   sprintf(value,"%d",1);
   write_parfile_entry(fp, "Print sleep stats?",value);

   fprintf (fp, "\nGENERAL PROCESS-FLOW PARAMETERS\n\n");

   sprintf(value,"%d",1);
   write_entry(fp, "Number of processors",value);
   sprintf(value,"%.1f",1.0);
   write_entry(fp, "Process-Flow Time Scale",value);

   fprintf (fp, "\nSYNTHETIC I/O TRACE PARAMETERS\n\n");

   pars_synthio_readparams(fp,numblocks);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_readparams(FILE *fp,int numblocks)
{
   int i;
   int numgens = 1;

   sprintf(value,"%d",5);
   write_entry(fp, "Number of generators",value);
   sprintf(value,"%d",10000);
   write_entry(fp, "Number of I/O requests to generate",value);
   sprintf(value,"%.1f",1000.0);
   write_entry(fp, "Maximum time of trace generated (in seconds)",value);
   sprintf(value,"%d",0);
   write_entry(fp, "System call/return with each request",value);
   sprintf(value,"%.1f",0.0);   
   write_entry(fp, "Think time from call to request",value);
   sprintf(value,"%.1f",0.0);   
   write_entry(fp, "Think time from request to return",value);
   
   for(i=1;i<=numgens;i++) {

      fprintf(fp, "\nGenerator description #%d\n",i);

      sprintf(value,"%d",5);
      write_entry(fp, "Generators with description",value);

      sprintf(value,"%d",numblocks);
      write_entry(fp, "Storage capacity per device (in blocks)",value);
      sprintf(value,"%d",1);
      write_entry(fp, "Number of storage devices",value);

      sprintf(value,"%d",0);
      write_entry(fp, "First storage device",value);

      sprintf(value,"%d",8);
      write_entry(fp, "Blocking factor",value);

      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Probability of sequential access",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Probability of local access",value);
      sprintf(value,"%.2f",0.66);
      write_entry(fp, "Probability of read access",value);
      sprintf(value,"%.1f",1.0);
      write_entry(fp, "Probability of time-critical request",value);
      sprintf(value,"%.1f",0.0);
      write_entry(fp, "Probability of time-limited request",value);

      fprintf(fp, "Time-limited think times (in milliseconds)\n");
      pars_synthio_read_distr(fp,"normal",30.0,100.0,0.0);

      fprintf(fp, "General inter-arrival times (in milliseconds)\n");
      pars_synthio_read_distr(fp,"exponential",0.0,0.0,0.0);

      fprintf(fp, "Sequential inter-arrival times (in milliseconds)\n");
      pars_synthio_read_distr(fp,"normal",0.0,0.0,0.0);

      fprintf(fp, "Local inter-arrival times (in milliseconds)\n");
      pars_synthio_read_distr(fp,"exponential",0.0,0.0,0.0);

      fprintf(fp, "Local distances (in blocks)\n");
      pars_synthio_read_distr(fp,"normal",0.0,40000.0,0.0);

      fprintf(fp, "Sizes (in blocks)\n");
      pars_synthio_read_distr(fp,"exponential",0.0,8.0,0.0);
   }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_read_uniform(FILE *fp, float min, float max)
{
   /* newdistr->type = SYNTHIO_UNIFORM; */
   sprintf(value,"%.1f",min);
   write_entry(fp, "Minimum value:",value);
   sprintf(value,"%.1f",max);
   write_entry(fp, "Maximum value:",value);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
pars_synthio_read_normal(FILE *fp, float mean, float variance)
{
   /* newdistr->type = SYNTHIO_NORMAL; */
   sprintf(value,"%.1f",mean);
   write_entry(fp, "Mean",value);
   sprintf(value,"%.1f",variance);
   write_entry(fp, "Variance",value);

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_read_exponential(FILE *fp,float base, float mean)
{
   /* newdistr->type = SYNTHIO_EXPONENTIAL; */
   sprintf(value,"%.1f",base);
   write_entry(fp, "Base value",value);
   sprintf(value,"%.1f",mean);
   write_entry(fp, "Mean",value);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_read_poisson(FILE *fp,float base, float mean)
{
   /* newdistr->type = SYNTHIO_POISSON; */
   sprintf(value,"%.1f",base);
   write_entry(fp, "Base value",value);
   sprintf(value,"%.1f",mean);
   write_entry(fp, "Mean",value);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_read_twovalue(FILE *fp,float defval, float secval, float probab)
{
   /* newdistr->type = SYNTHIO_TWOVALUE; */
   sprintf(value,"%.1f",defval);
   write_entry(fp, "Default value",value);
   sprintf(value,"%.1f",secval);
   write_entry(fp, "Second value",value);
   sprintf(value,"%.1f",probab);
   write_entry(fp, "Probability of second value",value);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
pars_synthio_read_distr(FILE *fp,char *type,float arg1,float arg2,float arg3)
{
   fprintf (fp, "Type of distribution: %s\n", type);
   switch (type[0]) {
      case 'u':
	pars_synthio_read_uniform(fp,arg1,arg2);
	break;
      case 'n':
	pars_synthio_read_normal(fp,arg1,arg2);
	break;
      case 'e':
	pars_synthio_read_exponential(fp,arg1,arg2);
	break;
      case 'p':
	pars_synthio_read_poisson(fp,arg1,arg2);
	break;
      case 't':
	pars_synthio_read_twovalue(fp,arg1,arg2,arg3);
	break;
      default:
	internal_error("Invalid value for 'type of distribution' - %s\n", type);
   }

}

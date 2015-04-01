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
#include "trace_gen.h"
#include "build_scsi.h"
#include "print_bscsi.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "handle_error.h"

#include "dixtrac.h"

#define NUM_REQUESTS                 50

#define MAX_NUMBLOCKS                8
/* in microseconds */
#define MAX_DELAY                 50000
#define MIN_DELAY                   400

typedef struct {
  float arrival;
  int lbn;
  int blocks;
  int type;
} synthtrace_t;

extern char *scsi_buf;

extern diskinfo_t *thedisk;

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static synthtrace_t *
trace_requests(char *model_name,int trace_source,int reqnum, int *howmany)
{
  FILE *lbnfile = NULL;
  char fname[280] = "";
  int i,maxlbn,dummy;
  synthtrace_t *request, *allreqs = NULL;

  maxlbn = thedisk->numblocks-1;

   switch (trace_source) {
   case LBN_FILE_TRACE:
     sprintf (fname, "%s.%s", model_name,LBN_FILE_EXT);
     lbnfile = fopen (fname, "r");
     assert(lbnfile != NULL);
#ifndef SILENT
     printf("Using %s file!\n",fname);
#endif
     fscanf(lbnfile,"Number of Requests: %d",howmany);
     allreqs = request = (synthtrace_t *)malloc(*howmany*sizeof(synthtrace_t));
     assert(request != NULL);
     for(i=0;i<*howmany;i++) {
       if (fscanf(lbnfile,"%d,%d\n",&request->lbn,&request->blocks) != 2)
	 error_handler("Error reading line %d of %s!\n",i+1,fname);
       if ((request->lbn  > (thedisk->numblocks - 1)) || (request->lbn  < 0))
	 error_handler("Invalid LBN on line %d of %s !\n",i+1,fname);

       request->type = B_READ;
       request++;
     }
     break;
   case SYNTH_DISKSIM_TRACE:
     sprintf (fname, "%s.%s", model_name,SYNTHTRACE_EXT);
     if (!(lbnfile = fopen (fname, "r"))) {
       error_handler("Could not open %s file!\n",fname);
     }
#ifndef SILENT
     printf("Using %s file!\n",fname);
#endif
     *howmany = 0;
     while (fgets(fname,200,lbnfile)) {
       (*howmany)++;
     }
     rewind(lbnfile);
     allreqs = request = (synthtrace_t *)malloc(*howmany*sizeof(synthtrace_t));
     assert(request != NULL);
     for(i=0 ; i < *howmany ; i++) {
       if (fscanf(lbnfile,"%f %d %d %d %d\n",&request->arrival,&dummy,
		  &request->lbn,&request->blocks, &request->type) != 5)
	 internal_error("Error reading line %d of %s!\n",i+1,fname);
       if ((request->lbn  > (thedisk->numblocks - 1)) || (request->lbn  < 0))
	 internal_error("Invalid LBN on line %d of %s !\n",i+1,fname);
       /* request->type = request->type & 0x1; */
       request->type = ((request->type & 0x1) ? B_READ : B_WRITE);
       request++;
     }
     break;
   case GENERATED_TRACE:
     if (reqnum == 0)
       *howmany = NUM_REQUESTS;
     else
       *howmany = reqnum;
     allreqs = request = (synthtrace_t *)malloc(*howmany*sizeof(synthtrace_t));
     assert(request != NULL);
     for(i=0;i<*howmany;i++) {
       request->blocks = 1 + (rand() % MAX_NUMBLOCKS);
        request->lbn = rand() % (maxlbn - request->blocks);
       if ((request->blocks + request->lbn) % 2) 
	 request->type = B_READ;
       else
	 request->type = B_WRITE;
	request++;
     }
     break;
   default:
     internal_error("generate_validation_trace: trace_source %d unknown!\n");
   }
   if (trace_source != GENERATED_TRACE)
     fclose(lbnfile);

   return(allreqs);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
generate_validation_trace(char *model_name,int trace_source,int reqnum)
{
   FILE *fp;
   char fname[280] = "";
   int i,howmany;
   long delay = 0;
   long real_delay = 0;
   long compl_time,elapsed = 0;
   long start_sec[2],start[2],end[2];
   long beg_sec,beg_start;
   extern long ustop,ustart,ustart_sec;
   synthtrace_t *request, *allreqs;

   /* end[0] = 0; start[0] = 0; start_sec[0] = 0; */

   request = allreqs = trace_requests(model_name,trace_source,reqnum,&howmany);

   sprintf (fname, "%s.%s", model_name,TRACES_EXT);
   fp = fopen (fname, "w");
   assert(fp != NULL);

   exec_scsi_command(scsi_buf,SCSI_inq_command());
   /* get the time of the just-completed command */
   end[1] = ustop;  start[1] = ustart;  start_sec[1] = ustart_sec;

   beg_sec = ustart_sec;
   beg_start = ustart;

   if (trace_source == SYNTH_DISKSIM_TRACE) 
     delay = (long) (request->arrival*1000) - (end[1]-start[1]);
   else
     delay = 14567;

   delay = 0;
   busy_wait_loop(delay);

   for(i=0;i<howmany;i++) {
     /* save the times from the previous command */
     end[0] = end[1];  start[0] = start[1];  start_sec[0] = start_sec[1];

     /* issue command and time it */
     exec_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
						request->blocks));

     /* get the time of the just-completed command */
     end[1] = ustop;  start[1] = ustart;   start_sec[1] = ustart_sec;

     compl_time = end[1] - start[1];
     real_delay = ((start[1] + (1000000 * start_sec[1])) -
		   (end[0] + (1000000 * start_sec[0])));

     if (trace_source == SYNTH_DISKSIM_TRACE) {
       if (i+1 < howmany) {
	 elapsed = (start[1] - beg_start + (1000000*(start_sec[1]-beg_sec)));
	 delay = (long) ((request+1)->arrival * 1000) - elapsed - compl_time;
       }
       else
	 delay = 1000;
     }
     else {
       delay = (long) (MIN_DELAY + (rand() % MAX_DELAY));
     }
     /* printf("Elapsed time is %.3f seconds\n",(float) elapsed/1000000); */

     /* (float) elapsed/1000, */
     fprintf(fp,"%s Hit %10d %3d %12.5f %12.5f\n",
	     ((request->type == B_READ)?"R":"W"),request->lbn,request->blocks,
	     (float) compl_time,(float) real_delay);
     
     /* printf("Delay:%ld\treal:%ld\tcompl:%ld\tarrival:%ld\n",delay, */
     /* 	      real_delay,compl_time,(long) request->arrival); */

     busy_wait_loop(delay);

     request++;
   }
   /*  printf("Elapsed time: %f\n", */
   /*  	  (float) (ustop+(ustop_sec-beg_sec)*1000000)-beg_start); */
   fclose(fp);
   free(allreqs);
}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
generate_traxtent_trace(char *model_name)
{
   FILE *fp;
   char fname[280] = "";
   int i,howmany;
   long compl_time;
   long start_sec[20],start[20],end_sec[20],end[20];
   long beg_sec,beg_start;
   extern long ustop,ustop_sec,ustart,ustart_sec;
   int retid,tag = 0;
   synthtrace_t *request, *allreqs;
   int tag1;

   /* end[0] = 0; start[0] = 0; start_sec[0] = 0; */
   int reqnum;
   request = allreqs = trace_requests(model_name,LBN_FILE_TRACE,
				      reqnum,&howmany);
   if (howmany < 2) {
     error_handler("The file %s.%s has too few requests!\n",
		   model_name,LBN_FILE_TRACE);
   }
   sprintf (fname, "%s.%s", model_name,TRACES_EXT);
   fp = fopen (fname, "w");
   assert(fp != NULL);

   tag1 = request->lbn;
   send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
					      request->blocks),tag1);
   start[tag] = ustart;  start_sec[tag] = ustart_sec;
   /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]);  */
   
   beg_sec = ustart_sec;
   beg_start = ustart;
   
   request++;
   tag = !tag;
   tag1 = request->lbn;
   send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
					      request->blocks),tag1);
   start[tag] = ustart;  start_sec[tag] = ustart_sec;
   /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]); */

   for(i=0;i < howmany ; i++) { 
     request--; 
     tag = !tag;
     retid = recv_scsi_command(scsi_buf);
     if (retid != tag) {
        /*  printf("WARNING: Returned tag = %d (should be %d)\n",retid,tag); */
     }
     /*  printf("LBN: %d\n",retid); */
     end[tag] = ustop;  end_sec[tag] = ustop_sec;
     /*  printf("E%d   End time: %10.1f\n",tag,(float) end[tag]); */

     if (( i>0 ) ) {
       compl_time = (end[tag]+(end_sec[tag]-end_sec[!tag])*1000000)-end[!tag];
     }
     else {
       compl_time = (end[tag]+(end_sec[tag]-start_sec[tag])*1000000)-start[tag];
     }
     fprintf(fp,"%s Hit %10d %3d %12.5f %12.5f\n",
	     ((request->type == B_READ)?"R":"W"),request->lbn,request->blocks,
	     (float) compl_time,(float) 0);
     
     request+=2;
     if (i >= (howmany-2 )) {
       continue;
     }
     tag1 = request->lbn;
     send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
						request->blocks),
		       tag1);
     start[tag] = ustart;  start_sec[tag] = ustart_sec;
     /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]); */
   }
     printf("Elapsed time: %f\n", 
     	  (float) (ustop+(ustop_sec-beg_sec)*1000000)-beg_start); 
   fclose(fp);
   free(allreqs);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
generate_lbn_file(char *model_name)
{
   FILE *lbnfile;
   char fname[280] = "";
  
   int maxlbn,lbn,i,howmany,numblocks = 0;

  maxlbn = thedisk->numblocks-1;

   howmany = NUM_REQUESTS;
   sprintf (fname, "%s.%s", model_name,LBN_FILE_EXT);
   lbnfile = fopen (fname, "w");

   assert(lbnfile != NULL);
   fprintf(lbnfile,"Number of Requests: %d\n",howmany);

   for(i=0;i<howmany;i++) {
     numblocks = 1 + (rand() % MAX_NUMBLOCKS);
     lbn = rand() % (maxlbn - numblocks);
     fprintf(lbnfile,"%d,%d\n",lbn,numblocks);
   }
   fclose(lbnfile);
}

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
#include "handle_error.h"
#include "extract_layout.h"
#include "extract_params.h"
#include "scsi_conv.h"

/* the number of iterations, rundom lbn translations, to determine max head */
extern char *scsi_buf;

extern long ustart;
extern long ustop;
extern long ustart_sec;
extern long ustop_sec;

#define BUS_BANDWIDTH 41943040
#define MAX_OUTSTANDING 1
#define NO_TRIALS 30
#define NO_TEST_TRIALS 3
#define ITERA 2000
#define SECTOR_INC 1
#define SECTOR_START 0
#define SINGLE_SECTOR_US 6
#define TRACK_SWITCH_US 100
#define MAX_CACHE 200
#define MAX_FLUSHES 1
#define ONE_ROTATION 6000
#define COMMAND_ID 127
#define HEAD_SWITCH 400

void clear_cache(long sourcelbn,long size){
  int status = -1;
  //fprintf(stderr,"source %ld size %ld\n",sourcelbn,size);
  status = exec_scsi_command(scsi_buf,
			     SCSI_rw_command(B_READ,
					     sourcelbn,
					     size));
  if(status == -1){
    fprintf(stderr,"id = %d\n",status);    
  }
  /*  send_scsi_command(scsi_buf,SCSI_rw_command(B_READ,sourcelbn,
      size),9999);
      status = recv_scsi_command(scsi_buf);
      while(status == -1){
      status = recv_scsi_command(scsi_buf);
      }*/
  //  fprintf(stderr,"id = %d\n",status);
}

#define HALF_MAX 1
#define MAX_TEST 2 * HALF_MAX
#define HALF_MAX_TEST 20
#define MAX_TEST_TEST 2 * HALF_MAX_TEST
#define MAX_DIST 1000
#define MAX_SECTORS 1025
#define MAX_DELAY 50000
#define MAX_FAIL_RETRY 10
#define MAX_FAIL 5
#define SECT_SHIFT 0
#define BASE_SECT 334
#define MIN_MINUS -1000
#define MIN_SAMPLES NO_TRIALS - .03 * NO_TRIALS

//#define MULTIPLE 1

#ifdef BLAH
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

   send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
					      request->blocks),tag);
   start[tag] = ustart;  start_sec[tag] = ustart_sec;
   /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]);  */
   
   beg_sec = ustart_sec;
   beg_start = ustart;
   
   request++;
   tag = !tag;
   send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
					      request->blocks),tag);
   start[tag] = ustart;  start_sec[tag] = ustart_sec;
   /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]); */

   for(i=0;i < howmany ; i++) { 
     request--; 
     tag = !tag;
     retid = recv_scsi_command(scsi_buf);
     if (retid != tag) {
        printf("WARNING: Returned tag = %d (should be %d)\n",retid,i);
     }
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
     send_scsi_command(scsi_buf,SCSI_rw_command(request->type,request->lbn,
						request->blocks),
		       tag);
     start[tag] = ustart;  start_sec[tag] = ustart_sec;
     /*  printf("S%d Start time: %10.1f\n",tag,(float) start[tag]); */
   }
     printf("Elapsed time: %f\n", 
     	  (float) (ustop+(ustop_sec-beg_sec)*1000000)-beg_start); 
   fclose(fp);
   free(allreqs);
}
#endif

int comp_long(const void *first,const void *second){
  
  if(*(long *)first < *(long *)second){
    return(-1);
  }else if (*(long *)first > *(long *)second){
    return(1);
  }else{
    return(0);
  }
}

void
rotate_wait_loop(unsigned long howlong)
{
  //  long t1,t2;
  struct timeval tv;
  struct timezone tz;
  int before = 0;

  /*  gettimeofday(&tv,&tz);
      t1 = tv.tv_sec;
      t2 = tv.tv_usec;*/
  //  fprintf(stderr,"how long %ld\n",howlong);
  gettimeofday(&tv,&tz);    
  do {
    //    fprintf(stderr,"time %ld\n",tv.tv_usec % ONE_ROTATION);
    if(((unsigned long)tv.tv_usec + 1000000 * (unsigned long)tv.tv_sec) % ONE_ROTATION <= howlong){
      before = 1;
    }
    if(((unsigned long)tv.tv_usec + 1000000 * (unsigned long)tv.tv_sec) % ONE_ROTATION >= howlong && 
       before == 1){
      //      fprintf(stderr," time %lu current %lu\n",((unsigned long)tv.tv_usec + 1000000 * (unsigned long)tv.tv_sec) % ONE_ROTATION,(unsigned long)tv.tv_usec + 1000000 * (unsigned long)tv.tv_sec);
      return;
    }
    gettimeofday(&tv,&tz);    
  } while(1);
}

#ifdef BLAH
  busy_wait_loop(100);
  send_scsi_command(scsi_buf,
		    SCSI_rw_command(B_READ,
				    lbn[count],
				    sectors[count]*bsize),
		    count);
  mystart[count] = ustart;  
  mystart_sec[count] = ustart_sec;
  //  tag = !tag;
  count++;
  busy_wait_loop(100);
  send_scsi_command(scsi_buf,
		    SCSI_rw_command(B_READ,
				    lbn[count],
				    sectors[count]*bsize),
		    count);
  //  tag = !tag;
  count++;
#endif

  /*  status = recv_scsi_command(scsi_buf);
      
      while(status != -1){
      status = recv_scsi_command(scsi_buf);
      }*/

  /*  return_id = recv_scsi_command(scsi_buf);
      if (return_id != tag) {
      printf("WARNING: Returned tag = %d (should be %d)\n",return_id,tag);
      }
      mystop[tag] = mystart[!tag] = ustop;  
      mystop_sec[tag] = mystart_sec[!tag] = ustop_sec;
      
      mydelay[tag] = ((mystop[tag] + (1000000 * mystop_sec[tag])) -
      (mystart[tag] + (1000000 * mystart_sec[tag])));
      return_id = recv_scsi_command(scsi_buf);
      if (return_id != !tag) {
      printf("WARNING: Returned tag = %d (should be %d)\n",return_id,!tag);
      }
      
      mystop[!tag] = ustop;
      mystop_sec[!tag] = ustop_sec;
      
      mydelay[!tag] = ((mystop[!tag] + (1000000 * mystop_sec[!tag])) -
      (mystart[!tag] + (1000000 * mystart_sec[!tag])));
  */

#ifdef BLAH
    return_id = recv_scsi_command(scsi_buf);
    /*    if (return_id != count) {
	  fprintf(stderr,"WARNING: Returned tag = %d (should be %d)\n",
	  return_id,count);
	  }else{
	  fprintf(stderr,"Returned tag = %d\n",
	  return_id);
	  }*/
    //    fprintf(stderr,"LBN %ld sample_count %d count %d sectors %d\n",lbn[return_id],sample_count[return_id],count,sectors[return_id]);
    
    if(return_id != (MAX_TEST_TEST-1)){
      mystop[return_id] = mystart[return_id+1] = ustop;  
      mystop_sec[return_id] = mystart_sec[return_id+1] = ustop_sec;
    }else{
      mystop[return_id] = mystart[0] = ustop;  
      mystop_sec[return_id] = mystart_sec[0] = ustop_sec;
    }
    mydelay[return_id] = ((mystop[return_id] + 
			   (1000000 * mystop_sec[return_id])) -
			  (mystart[return_id] + 
			   (1000000 * mystart_sec[return_id])));
    //    mydelay[count] -= ((sectors[return_id])*1000000)/81920;

    send_scsi_command(scsi_buf,
		      SCSI_rw_command(B_READ,
				      lbn[count],
				      sectors[count]*bsize),
		      count);
    //    tag = !tag;
#endif

long test_track_boundry(long start_lbn,
			int track_size,
			long maxlbn,
			long bsize){
  int sectors[MAX_TEST_TEST];
  int sample_count[MAX_TEST_TEST];
  long lbn[MAX_TEST_TEST];
  //  long orig_lbn[MAX_TEST_TEST];
  long time[MAX_TEST_TEST];
  long last_time[MAX_TEST_TEST];
  //  int sector_inc[MAX_TEST_TEST];
  //  int last_inc[MAX_TEST_TEST];
  /*long mystart[MAX_TEST_TEST];
    long mystart_sec[MAX_TEST_TEST];
    long mystop[MAX_TEST_TEST];
    long mystop_sec[MAX_TEST_TEST];
    long mydelay[MAX_TEST_TEST];*/
  unsigned long mystart;
  unsigned long mystart_sec;
  unsigned long mystop;
  unsigned long mystop_sec;
  unsigned long mydelay;
  int no_samples[MAX_TEST_TEST];
  int status=0;
  //  int return_id=0;
  int i;
  //,k;
  int count=0;
  int done=0;
  long tempLBN;
  //  long median;
  //  long rand_wait;
  int trial_failed=0;
  int failed_test=0;
  int count_done[MAX_TEST_TEST];
  int failed[MAX_TEST_TEST];
  //  int tag=0;
  //  int firstpass=0;
  //  int secondpass=0;
  unsigned long rotate_value[MAX_TEST_TEST];
  unsigned long rotate_inc = ONE_ROTATION/NO_TEST_TRIALS;
  //  struct timeval tv;
  //  struct timezone tz;
  long last_fail = 0;

  tempLBN = start_lbn + track_size * (MAX_TEST_TEST-1);
  //  fprintf(stderr,"lbn %ld tempLBN %ld\n",start_lbn,tempLBN);
  //  tempLBN = start_lbn + track_size;

  //  tempLBN = start_lbn + 2*track_size;

  for(i=0;i<HALF_MAX_TEST;i++){
    time[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    //    tempLBN += 2 * track_size;
    tempLBN -= track_size;
    count_done[i] = 0;
    sectors[i] = track_size;
    sample_count[i] = -1;
    no_samples[i] = 0;
    rotate_value[i] = 0;
    failed[i] = 0;
    done++;
  }

  //  tempLBN = start_lbn;

  for(i=HALF_MAX_TEST;i<MAX_TEST_TEST;i++){
    time[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    //    tempLBN += 2 * track_size;
    tempLBN -= track_size;
    sectors[i] = track_size;
    count_done[i] = 0;
    sample_count[i] = -1;
    no_samples[i] = 0;
    rotate_value[i] = 0;
    failed[i] = 0;
    done++;
  }

  for(i=0;i<MAX_TEST_TEST;i++){
    //    fprintf(stderr,"i %d LBN %ld\n",i,lbn[i]);
  }
  //  exit(0);
  while(done != 0){
    //    fprintf(stderr,"done %d count %d sample_count %d\n",done,count,sample_count[count]);
    /*    status = send_scsi_command(scsi_buf,
	  SCSI_rw_command(B_READ,
	  lbn[count],
	  sectors[count]*bsize),
	  COMMAND_ID);*/
    //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[count],sectors[count],count);
    if(sectors[count] < 0){
      fprintf(stderr,"ERROR: Sectors less than zero %d\n",sectors[count]);
      abort();
    }
    //    rand_wait = rand() % ONE_ROTATION;
    //    fprintf(stderr,"Waiting %ld microseconds\n",rand_wait);
    //    if(count != 0){
    
    while(rotate_value[count] == 0){
      //      gettimeofday(&tv,&tz);    
      //      rotate_value[count] = (tv.tv_usec + 1000000 * tv.tv_sec) % ONE_ROTATION;
      //if(count == 0){
      rotate_value[count] = rand() % ONE_ROTATION;
      //}else{
      //	rotate_value[count] = rotate_value[count-1];
      //}
      //      fprintf(stderr,"rotate value %ld count %d\n",rotate_value[count],count);
    }
    //    rand_wait = rand() % ONE_ROTATION;
    //busy_wait_loop(rand_wait);
    if(sample_count[count] != NO_TEST_TRIALS){
      /*	fprintf(stderr,"waiting %lu, base %lu, inc %lu\n",(rotate_value[count]
		+ 
		sample_count[count]
		* rotate_inc) % 
		ONE_ROTATION,rotate_value[count],sample_count[count] * 
		rotate_inc);*/
      if(sample_count[count] > 0){
	rotate_wait_loop((rotate_value[count] + 
			  sample_count[count] * rotate_inc) % ONE_ROTATION);
      }else{
	rotate_wait_loop(rotate_value[count]);
      }
      
      //    rand_wait = rand() % ONE_ROTATION;
      //    busy_wait_loop(rand_wait);
      
      
      //      fprintf(stderr,"LBN %ld sample_count %d count %d sectors %d\n",lbn[count],sample_count[count],count,sectors[count]);
      status = exec_scsi_command(scsi_buf,
				 SCSI_rw_command(B_READ,
						 lbn[count],
						 sectors[count]*bsize));
      
      if(status == -1){
	fprintf(stderr,"id = %d\n",status);    
      }
      
      mystart = ustart;
      mystart_sec = ustart_sec;
      
      
      /*return_id = recv_scsi_command(scsi_buf);
	while(return_id == -1){
	return_id = recv_scsi_command(scsi_buf);
	} */     
      
      mystop = ustop;
      mystop_sec = ustop_sec;
      
      //    fprintf(stderr,"rotate value %ld actual %ld\n",(rotate_value[count] + sample_count[count] * rotate_inc) % ONE_ROTATION,(mystart + mystart_sec * 1000000) % ONE_ROTATION);
      
      mydelay = ((mystop + (1000000 * mystop_sec)) -
		 (mystart + (1000000 * mystart_sec)));
      /*fprintf(stderr,"1my delay %ld size %d i %d\n",mydelay,
	sectors[count],count);*/
      //      fprintf(stderr,"current time after %lu elapsed %lu\n",(unsigned long)(mystop + 1000000 * mystop_sec),mydelay);
      if(mydelay < MAX_DELAY && sample_count[count] >= 0 && 
	 count_done[count] == 0){
	time[count] += mydelay;	
	no_samples[count]++;
	//	fprintf(stderr,"selected %lu for LBN %ld\n",mydelay,lbn[count]);
	//	fprintf(stderr,"my delay %ld size %d count %d trial %d time %ld samples %d\n",mydelay,sectors[count],count,sample_count[count],time[count],no_samples[count]);
      }else if(count_done[count] == 0 && sample_count[count] >= 0){
	//      fprintf(stderr,"2my delay %ld size %d count %d trial %d\n",mydelay,sectors[count],count,sample_count[count]);
	trial_failed = 1;
      }
    }
    if(count_done[count] == 0){
      if(sample_count[count] == NO_TEST_TRIALS){
	
	/*	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld\n",lbn[count], last_time[count]/(NO_TEST_TRIALS-1),time[count]/(NO_TEST_TRIALS-1),
		(time[count] - last_time[count])/(NO_TEST_TRIALS-1));*/
	/*      fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld\n",lbn[count], last_time[count]/(NO_TEST_TRIALS),time[count]/(NO_TEST_TRIALS),
		(time[count] - last_time[count])/(NO_TEST_TRIALS));*/
	/*	fprintf(stderr,"lbn %ld time %ld size %d\n",
		lbn[count],
		(time[count] - last_time[count])/(NO_TEST_TRIALS-1),
		sectors[count]);*/
	/*fprintf(stderr,"2LBN %ld orig %ld extra %ld diff %ld\n",lbn[count], last_time[count],time[count]/no_samples[count],
	  (time[count]/no_samples[count] - 
	  last_time[count]));*/
	
	
	//      if(firstpass == 1){
	//fprintf(stderr,"count %d track_size %d\n",sectors[count],track_size);
	if(sectors[count] == (track_size + 1)){
	  /*	  fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld samples %d\n",lbn[count], last_time[count],time[count]/no_samples[count],
		  (time[count]/no_samples[count] - 
		  last_time[count]),no_samples[count]);*/
	  
	  sectors[count]--;
	  if(last_time[count] != 0 &&
	     ((time[count]/no_samples[count] -
	       last_time[count])) > (HEAD_SWITCH)){
	    /*	     last_time[count])/(NO_TEST_TRIALS)) > (ONE_ROTATION/5 +
		     TRACK_SWITCH_US)){*/
	    fprintf(stderr,"Test LBN %ld track size %d\n",lbn[count],sectors[count]);
	    //return(1);
	  }else{
	    if(last_time[count] != 0){
	      
	      rotate_value[count] = rand() % ONE_ROTATION;
	      if(failed[count] < MAX_FAIL){	      
		failed_test = 1;
		failed[count]++;
		//		fprintf(stderr,"FAILED LBN %ld trying again %d\n",lbn[count],failed[count]);		
	      }else{
		if(lbn[count] < last_fail || last_fail == 0){
		  //		  fprintf(stderr,"FAILED LBN %ld track size %d\n",lbn[count],sectors[count]);
		  last_fail = lbn[count];
		}else{
		  //		  fprintf(stderr,"NOT FAILED LBN %ld track size %d\n",lbn[count],sectors[count]);
		}
	      }
	      //	    return(lbn[count]);
	    }
	  }
	  if(failed_test == 0){
	    count_done[count] = 1;
	    done--;
	  }
	}else{
	  sectors[count]++;
	}
	last_time[count] = time[count]/no_samples[count];
	if(failed_test == 1){
	  last_time[count] = 0;
	}
	failed_test = 0;
	time[count] = 0;
	sample_count[count] = -2;
	no_samples[count] = 0;
	/*      if(secondpass == 1 && count == (MAX_TEST_TEST-1)){
		done = 1;
		}    */
	/*      if(firstpass == 1 && count == (MAX_TEST_TEST-1)){
		done = 1;
		}*/
	/*      if(count == (MAX_TEST_TEST-1)){
		firstpass = 1;
		rand_wait = rand() % ONE_ROTATION;
		busy_wait_loop(rand_wait);
		fprintf(stderr,"first pass\n");    
		}*/
      }
      
      if(trial_failed == 0){
	sample_count[count]++;
      }
      trial_failed = 0;
    }
    count++;
    
    if(count == MAX_TEST_TEST){
      count = 0;
    }
  }
  return(last_fail);
}

int find_track_boundry(long start_lbn,
		       int start_sectors,
		       //		       int track_size,
		       int inc_sectors,
		       long maxlbn,
		       long bsize){
  int sectors[MAX_TEST];
  int sample_count[MAX_TEST];
  long lbn[MAX_TEST];
  long orig_lbn[MAX_TEST];
  long time[MAX_TEST];
  long last_time[MAX_TEST];
  int sector_inc[MAX_TEST];
  int last_inc[MAX_TEST];
  int count_done[MAX_TEST];
  int forward[MAX_TEST];
  int retry_count[MAX_TEST];
  int no_samples[MAX_TEST];
  int last_count[MAX_TEST];
  long mystart[MAX_TEST];
  long mystart_sec[MAX_TEST];
  long mystop[MAX_TEST];
  long mystop_sec[MAX_TEST];
  long mydelay[MAX_TEST];
  long seek_time[MAX_TEST];
  long prior_diff[MAX_TEST];
  int prior_count[MAX_TEST];
  unsigned long rotate_value[MAX_TEST];
  unsigned long rotate_inc = ONE_ROTATION/NO_TRIALS;
  int status=0;
  int return_id=0;
  int i;//,k;
  int count=0;
  int done=0;
  long tempLBN=start_lbn + (MAX_TEST-1) * start_sectors;
  //  long median;
  //  int tag=0;
  //  long rand_wait=0;
  int trial_failed=0;

  for(i=0;i<MAX_TEST;i++){
    time[i] = 0;
    retry_count[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    orig_lbn[i] = tempLBN;
    tempLBN -= start_sectors;
    sectors[i] = start_sectors;
    sector_inc[i] = inc_sectors;
    rotate_value[i] = 0;
    last_inc[i] = 0;
    sample_count[i] = 0;
    count_done[i] = 0;
    no_samples[i] = 0;
    seek_time[i] = 0;
    last_count[i] = 0;
    prior_count[i] = 1;
    prior_diff[i] = 0;
    if(i == 0){
#ifdef MULTIPLE
      count_done[i] = 0;
#else
      count_done[i] = 1;
#endif
      sectors[i] = BASE_SECT;
      sector_inc[i] = 0;
      done--;
    }
    forward[i] = 0;
    done++;    
  }

  for(i=0;i<MAX_TEST;i++){
    //    fprintf(stderr,"i %d LBN %ld\n",i,lbn[i]);
  }

  /*  return_id = recv_scsi_command(scsi_buf);
      
      while(return_id != -1){
      return_id = recv_scsi_command(scsi_buf);
      }*/

#ifdef MULTIPLE
  count = 1;
#else
  count = 0;
#endif
  while(done != 0){
    
    while(rotate_value[count] == 0){
      rotate_value[count] = rand() % ONE_ROTATION;
    }

    if(sample_count[count] != NO_TEST_TRIALS){
      if(sample_count[count] > 0){
	rotate_wait_loop((rotate_value[count] + 
			  sample_count[count] * rotate_inc) % ONE_ROTATION);
      }else{
	rotate_wait_loop(rotate_value[count]);
      }
      
      //    rand_wait = rand() % ONE_ROTATION;
      //    fprintf(stderr,"Waiting %ld microseconds\n",rand_wait);
      //    if(count != 0){
      //    busy_wait_loop(rand_wait);
      //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[count],sectors[count]+sector_inc[count],count);
#ifdef MULTIPLE
      send_scsi_command(scsi_buf,
			SCSI_rw_command(B_READ,
					(lbn[count] + sectors[count] + 
					 sector_inc[count]),
					((sectors[count] + 
					  sector_inc[count])*bsize)),
			0);
      mystart[0] = ustart;  
      mystart_sec[0] = ustart_sec;
      send_scsi_command(scsi_buf,
			SCSI_rw_command(B_READ,
					lbn[count],
					((sectors[count] + 
					  sector_inc[count])*bsize)),
			1);
      return_id = recv_scsi_command(scsi_buf);
      if (return_id != 0) {
	printf("WARNING: Returned tag = %d (should be %d)\n",return_id,0);
      }
      mystop[0] = mystart[1] = ustop;  
      mystop_sec[0] = mystart_sec[1] = ustop_sec;
      
      mydelay[0] = ((mystop[0] + (1000000 * mystop_sec[0])) -
		    (mystart[0] + (1000000 * mystart_sec[0])));
      return_id = recv_scsi_command(scsi_buf);
      if (return_id != 1) {
	printf("WARNING: Returned tag = %d (should be %d)\n",return_id,1);
      }
      
      mystop[1] = ustop;  
      mystop_sec[1] = ustop_sec;
      
      mydelay[1] = ((mystop[1] + (1000000 * mystop_sec[1])) -
		    (mystart[1] + (1000000 * mystart_sec[1])));
#else    
      if(count != 0){
	status = exec_scsi_command(scsi_buf,
				   SCSI_rw_command(B_READ,
						   lbn[count],
						   ((sectors[count] + 
						     sector_inc[count])*bsize)));
	
	
	mystart[count] = ustart;
	mystart_sec[count] = ustart_sec;
	mystop[count] = ustop;
	mystop_sec[count] = ustop_sec;
	mydelay[count] = ((mystop[count] + (1000000 * mystop_sec[count])) -
			  (mystart[count] + (1000000 * mystart_sec[count])));
	//      fprintf(stderr,"1my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
	//      mydelay[count] -= ((sectors[count] + sector_inc[count])*1000000)/81920;
	/*      mydelay[count] -= ((sector_inc[count] + sectors[count] - 
		last_count[count])*1000000)/81920;
		mydelay[count] -= ((sector_inc[count] + sectors[count] - 
		last_count[count])*1000000)/81920;*/
	//      mydelay[count] -= ((sectors[count])*750000)/81920;
      }
      //      fprintf(stderr,"2my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
#endif
      if(mydelay[count] < 0){
	mydelay[count] = 0;
      }
      //    }
      if(count_done[count] == 0){
	if(mydelay[count] < MAX_DELAY){
	  time[count] += mydelay[count];	
	  //	fprintf(stderr,"my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
	  no_samples[count]++;
	}else if(count_done[count] == 0 && sample_count[count] >= 0){
	  trial_failed = 1;
	}
#ifdef MULTIPLE
	if(mydelay[count-1] < MAX_DELAY){
	  time[count-1] += mydelay[count-1];	
	  no_samples[count-1]++;
	}
#endif
      }
    }
    
    if(count_done[count] == 0){      
      //      fprintf(stderr,"my delay %ld size %d count %d samples %d\n",mydelay[count],sectors[count]+sector_inc[count],count,sample_count[count]);
      if(sample_count[count] == NO_TRIALS){
	if(sector_inc[count] == 1 && sectors[count] == 0){
	  seek_time[count] = time[count]/no_samples[count] - ONE_ROTATION/2;
#ifdef MULTIPLE
	  seek_time[count-1] = (time[count-1]/no_samples[count-1] 
				- ONE_ROTATION/2);
#endif
	}
	/*	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld base %d last inc %d sector inc %d last %d count %d\n",lbn[count], last_time[count],
		time[count]/(no_samples[count]) - seek_time[count],
		(time[count]/no_samples[count] - 
		last_time[count] - seek_time[count]),
		sectors[count],last_inc[count],
		sector_inc[count],
		last_count[count],count);*/
#ifdef MULTIPLE
	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld base %d last inc %d sector inc %d\n",lbn[count] + sectors[count] + sector_inc[count], 
		last_time[count-1],
		  time[count-1]/(no_samples[count-1]) - seek_time[count-1],
		time[count-1]/no_samples[count-1] - 
		last_time[count-1] - seek_time[count-1],
		sectors[count],last_inc[count],
		sector_inc[count]);
#endif
	if(last_time[count] != 0 && (time[count]/no_samples[count] - 
				     seek_time[count] -
				     last_time[count]) > 
	   (HEAD_SWITCH)){
	  if(sector_inc[count] == 1){
	    /*if(sectors[count] != 1){
	      sectors[count]--;
	      }*/
	    fprintf(stderr,"TEST2 LBN %ld track size %d id %d\n",
		    lbn[count],sectors[count],return_id);
	    count_done[count] = 1;
	    done--;
	    return(sectors[count]);
	    if(done == 0){
		
	    }
	  }else{
	    sectors[count] += last_inc[count];
	    if(last_inc[count] != 0){
	      sector_inc[count] /= 4;
	    }else{
	      sector_inc[count] /= 2;	      
	    }
	      if(sector_inc[count] == 0){
		sector_inc[count] = 1;
	      }
	      last_inc[count] = 0;
	  }
	}else{
	  //	    if(((time[count] - last_time[count])/NO_TRIALS) > MIN_MINUS){ 
	  if((sectors[count]+sector_inc[count]) >= MAX_SECTORS){
	    sectors[count] = start_sectors;
	    sector_inc[count] = inc_sectors;
	    last_inc[count] = sector_inc[count];
	    lbn[count] = orig_lbn[count];
	    retry_count[count]++;
	    if(retry_count[count] > MAX_FAIL_RETRY){
	      fprintf(stderr,"LBN %ld not found\n",lbn[count]);
	      return(-1);
	    }
	  }else{
	    last_inc[count] = sector_inc[count];
	    last_count[count] = sectors[count] + sector_inc[count];
	    prior_diff[count] = (time[count]/no_samples[count] - 
				 seek_time[count] -
				 last_time[count]);
	    prior_count[count] = sector_inc[count] + last_inc[count];
	    sector_inc[count] *= 2;
	  }
	    if(time[count]/no_samples[count]-seek_time[count] > 
	       last_time[count]){
	      last_time[count] = (time[count]/
				  no_samples[count])-seek_time[count];
	    }
	}
	  //	    }
	time[count] = 0;
	sample_count[count] = 0;
	no_samples[count] = 0;
      }
    }
    if(count == 0){
      if(sample_count[count] == NO_TRIALS){
	sectors[count] = BASE_SECT;	
	sample_count[count] = 0;
	lbn[count] = orig_lbn[count];
      }else{
	//	sectors[count] += SECT_SHIFT;
	lbn[count] += SECT_SHIFT;
      }
    }
    
    if(trial_failed == 0){
      sample_count[count]++;
    }
    trial_failed = 0;
#ifdef MULTIPLE
    count = 1;
#else
    count++;
#endif
    if(count == MAX_TEST){
      count = 0;
    }
  }
  return(0);
}

int test_track_boundry3(long start_lbn,
		       int start_sectors,
		       //		       int track_size,
		       int inc_sectors,
		       long maxlbn,
		       long bsize){
  int sectors[MAX_TEST_TEST];
  int sample_count[MAX_TEST_TEST];
  long lbn[MAX_TEST_TEST];
  long orig_lbn[MAX_TEST_TEST];
  long time[MAX_TEST_TEST];
  long last_time[MAX_TEST_TEST];
  int sector_inc[MAX_TEST_TEST];
  int last_inc[MAX_TEST_TEST];
  int count_done[MAX_TEST_TEST];
  int forward[MAX_TEST_TEST];
  int retry_count[MAX_TEST_TEST];
  int no_samples[MAX_TEST_TEST];
  int last_count[MAX_TEST_TEST];
  long mystart[MAX_TEST_TEST];
  long mystart_sec[MAX_TEST_TEST];
  long mystop[MAX_TEST_TEST];
  long mystop_sec[MAX_TEST_TEST];
  long mydelay[MAX_TEST_TEST];
  long seek_time[MAX_TEST_TEST];
  long prior_diff[MAX_TEST_TEST];
  int prior_count[MAX_TEST_TEST];
  int status=0;
  int return_id=0;
  int i;//,k;
  int count=0;
  int done=0;
  long tempLBN=start_lbn + (MAX_TEST_TEST-1) * MAX_DIST;
  //  long median;
  //  int tag=0;
  long rand_wait=0;

  for(i=0;i<MAX_TEST_TEST;i++){
    time[i] = 0;
    retry_count[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    orig_lbn[i] = tempLBN;
    tempLBN -= MAX_DIST;
    sectors[i] = start_sectors;
    sector_inc[i] = inc_sectors;
    last_inc[i] = 0;
    sample_count[i] = 0;
    count_done[i] = 0;
    no_samples[i] = 0;
    seek_time[i] = 0;
    last_count[i] = 0;
    prior_count[i] = 1;
    prior_diff[i] = 0;
    if(i == 0){
#ifdef MULTIPLE
      count_done[i] = 0;
#else
      count_done[i] = 1;
#endif
      sectors[i] = BASE_SECT;
      sector_inc[i] = 0;
      done--;
    }
    forward[i] = 0;
    done++;    
  }

  for(i=0;i<MAX_TEST_TEST;i++){
    fprintf(stderr,"i %d LBN %ld\n",i,lbn[i]);
  }

  return_id = recv_scsi_command(scsi_buf);
  
  while(return_id != -1){
    return_id = recv_scsi_command(scsi_buf);
  }

#ifdef MULTIPLE
  count = 1;
#else
  count = 0;
#endif
  while(done != 0){
    
    rand_wait = rand() % ONE_ROTATION;
    //    fprintf(stderr,"Waiting %ld microseconds\n",rand_wait);
    //    if(count != 0){
    busy_wait_loop(rand_wait);
    //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[count],sectors[count]+sector_inc[count],count);
#ifdef MULTIPLE
    send_scsi_command(scsi_buf,
		      SCSI_rw_command(B_READ,
				      (lbn[count] + sectors[count] + 
					sector_inc[count]),
				      ((sectors[count] + 
					sector_inc[count])*bsize)),
		      0);
    mystart[0] = ustart;  
    mystart_sec[0] = ustart_sec;
    send_scsi_command(scsi_buf,
		      SCSI_rw_command(B_READ,
				      lbn[count],
				      ((sectors[count] + 
					sector_inc[count])*bsize)),
		      1);
    return_id = recv_scsi_command(scsi_buf);
    if (return_id != 0) {
      printf("WARNING: Returned tag = %d (should be %d)\n",return_id,0);
    }
    mystop[0] = mystart[1] = ustop;  
    mystop_sec[0] = mystart_sec[1] = ustop_sec;
    
    mydelay[0] = ((mystop[0] + (1000000 * mystop_sec[0])) -
		  (mystart[0] + (1000000 * mystart_sec[0])));
    return_id = recv_scsi_command(scsi_buf);
    if (return_id != 1) {
      printf("WARNING: Returned tag = %d (should be %d)\n",return_id,1);
    }
    
    mystop[1] = ustop;  
    mystop_sec[1] = ustop_sec;
    
    mydelay[1] = ((mystop[1] + (1000000 * mystop_sec[1])) -
		  (mystart[1] + (1000000 * mystart_sec[1])));
#else    
    if(count != 0){
      status = exec_scsi_command(scsi_buf,
				 SCSI_rw_command(B_READ,
						 lbn[count],
						 ((sectors[count] + 
						   sector_inc[count])*bsize)));
      
      
      mystart[count] = ustart;
      mystart_sec[count] = ustart_sec;
      mystop[count] = ustop;
      mystop_sec[count] = ustop_sec;
      mydelay[count] = ((mystop[count] + (1000000 * mystop_sec[count])) -
			(mystart[count] + (1000000 * mystart_sec[count])));
    //      fprintf(stderr,"1my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
      mydelay[count] -= ((sectors[count] + sector_inc[count])*1000000)/81920;
      /*      mydelay[count] -= ((sector_inc[count] + sectors[count] - 
	      last_count[count])*1000000)/81920;
	      mydelay[count] -= ((sector_inc[count] + sectors[count] - 
	      last_count[count])*1000000)/81920;*/
      //      mydelay[count] -= ((sectors[count])*750000)/81920;
    }
    //      fprintf(stderr,"2my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
#endif
    if(mydelay[count] < 0){
      mydelay[count] = 0;
    }
    //    }
    if(count_done[count] == 0){
      if(mydelay[count] < MAX_DELAY){
	time[count] += mydelay[count];	
	//	fprintf(stderr,"my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
	no_samples[count]++;
      }
#ifdef MULTIPLE
      if(mydelay[count-1] < MAX_DELAY){
	time[count-1] += mydelay[count-1];	
	no_samples[count-1]++;
      }
#endif
      
      //      fprintf(stderr,"my delay %ld size %d\n",mydelay,sectors[count]+sector_inc[count]);
      if(sample_count[count] == NO_TEST_TRIALS){
	if(no_samples[count] >= MIN_SAMPLES){
	  if(sector_inc[count] == 1 && sectors[count] == 0){
	    seek_time[count] = time[count]/no_samples[count] - ONE_ROTATION/2;
#ifdef MULTIPLE
	    seek_time[count-1] = (time[count-1]/no_samples[count-1] 
				  - ONE_ROTATION/2);
#endif
	  }
	  fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld base %d last inc %d sector inc %d last %d\n",lbn[count], last_time[count],
		  time[count]/(no_samples[count]) - seek_time[count],
		  (time[count]/no_samples[count] - 
		   last_time[count] - seek_time[count]),
		  sectors[count],last_inc[count],
		  sector_inc[count],
		  last_count[count]);
#ifdef MULTIPLE
	  fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld base %d last inc %d sector inc %d\n",lbn[count] + sectors[count] + sector_inc[count], 
		  last_time[count-1],
		  time[count-1]/(no_samples[count-1]) - seek_time[count-1],
		  time[count-1]/no_samples[count-1] - 
		  last_time[count-1] - seek_time[count-1],
		  sectors[count],last_inc[count],
		  sector_inc[count]);
#endif
	  if(last_time[count] != 0 && (time[count]/no_samples[count] - 
				       seek_time[count] -
					last_time[count]) > 
	     (HEAD_SWITCH)){
	    if(sector_inc[count] == 1){
	      /*if(sectors[count] != 1){
		sectors[count]--;
		}*/
	      fprintf(stderr,"TEST2 LBN %ld track size %d id %d\n",
		      lbn[count],sectors[count],return_id);
	      count_done[count] = 1;
	      done--;
	      return(sectors[count]);
	      if(done == 0){
		
	      }
	    }else{
	      sectors[count] += last_inc[count];
	      if(last_inc[count] != 0){
		sector_inc[count] /= 4;
	      }else{
		sector_inc[count] /= 2;	      
	      }
	      if(sector_inc[count] == 0){
		sector_inc[count] = 1;
	      }
	      last_inc[count] = 0;
	    }
	  }else{
	    //	    if(((time[count] - last_time[count])/NO_TEST_TRIALS) > MIN_MINUS){ 
	    if((sectors[count]+sector_inc[count]) >= MAX_SECTORS){
	      sectors[count] = start_sectors;
	      sector_inc[count] = inc_sectors;
	      last_inc[count] = sector_inc[count];
	      lbn[count] = orig_lbn[count];
	      retry_count[count]++;
	      if(retry_count[count] > MAX_FAIL_RETRY){
		fprintf(stderr,"LBN %ld not found\n",lbn[count]);
		return(-1);
	      }
	    }else{
	      last_inc[count] = sector_inc[count];
	      last_count[count] = sectors[count] + sector_inc[count];
	      prior_diff[count] = (time[count]/no_samples[count] - 
				   seek_time[count] -
				   last_time[count]);
	      prior_count[count] = sector_inc[count] + last_inc[count];
	      sector_inc[count] *= 2;
	    }
	    if(time[count]/no_samples[count]-seek_time[count] > 
	       last_time[count]){
	      last_time[count] = (time[count]/
				  no_samples[count])-seek_time[count];
	    }
	  }
	  //	    }
	}else{
	  fprintf(stderr,"2LBN %ld orig %ld extra %ld diff %ld base %d last inc %d sector inc %d\n",lbn[count], last_time[count]/(no_samples[count]),time[count]/(no_samples[count]),
		  (time[count] - last_time[count])/(no_samples[count]),
		  sectors[count],last_inc[count],
		  sector_inc[count]);
	}
	time[count] = 0;
	sample_count[count] = 0;
	no_samples[count] = 0;
      }
    }
    if(count == 0){
      if(sample_count[count] == NO_TEST_TRIALS){
	sectors[count] = BASE_SECT;	
	sample_count[count] = 0;
	lbn[count] = orig_lbn[count];
      }else{
	//	sectors[count] += SECT_SHIFT;
	lbn[count] += SECT_SHIFT;
      }
    }
    
    sample_count[count]++;    
#ifdef MULTIPLE
    count = 1;
#else
    count++;
#endif
    if(count == MAX_TEST_TEST){
      count = 0;
    }
  }
  return(0);
}

int find_track_boundry2(long start_lbn,
		       int start_sectors,
		       //		       int track_size,
		       int inc_sectors,
		       long maxlbn,
		       long bsize){
  int sectors[MAX_TEST];
  int sample_count[MAX_TEST];
  long lbn[MAX_TEST];
  long orig_lbn[MAX_TEST];
  long time[MAX_TEST];
  //  long sample_time[MAX_TEST][NO_TRIALS];
  int sample_time_count[MAX_TEST];
  long last_time[MAX_TEST];
  int sector_inc[MAX_TEST];
  int last_inc[MAX_TEST];
  int count_done[MAX_TEST];
  int forward[MAX_TEST];
  int retry_count[MAX_TEST];
  long mystart[MAX_OUTSTANDING];
  long mystart_sec[MAX_OUTSTANDING];
  long mystop[MAX_OUTSTANDING];
  long mystop_sec[MAX_OUTSTANDING];
  long mydelay=0;
  int status=0;
  int return_id=0;
  int i;//,k;
  int count=0;
  int done=0;
  long tempLBN=start_lbn;
  //  long median;
  int tag=0;

  for(i=0;i<HALF_MAX;i++){
    /*    for(k=0;k<NO_TRIALS;k++){
	  sample_time[i][k] = 0;
	  }*/
    time[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    orig_lbn[i] = tempLBN;
    tempLBN += 2 * MAX_DIST;
    sectors[i] = start_sectors;
    sector_inc[i] = inc_sectors;
    last_inc[i] = 0;
    sample_count[i] = 0;
    count_done[i] = 0;
    sample_time_count[i] = 0;
    forward[i] = 0;
    done++;
  }

  tempLBN -= MAX_DIST;

  for(i=HALF_MAX;i<MAX_TEST;i++){
    /*for(k=0;k<NO_TRIALS;k++){
      sample_time[i][k] = 0;
      }*/
    time[i] = 0;
    last_time[i] = 0;
    lbn[i] = tempLBN;
    orig_lbn[i] = tempLBN;
    tempLBN -= 2 * MAX_DIST;
    sectors[i] = start_sectors;
    sector_inc[i] = inc_sectors;
    last_inc[i] = 0;
    sample_count[i] = 0;
    count_done[i] = 0;
    sample_time_count[i] = 0;
    if(i == HALF_MAX){
      count_done[i] = 1;
      sectors[i] = SECT_SHIFT;
      done--;
    } 
    forward[i] = 0;
    done++;    
  }


  for(i=0;i<MAX_TEST;i++){
    if(i==HALF_MAX){
      //      fprintf(stderr,"HALF %d\n",i);
    }
    fprintf(stderr,"i %d LBN %ld\n",i,lbn[i]);
  }
  /*    exit(0);*/

  for(i=0;i<MAX_OUTSTANDING;i++){
    if(count == MAX_TEST){
      fprintf(stderr,"ERROR:  Number outstanding %d greater than sample %d\n",
	      i,count);
      abort();
    }
 
    //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[i],sectors[i],i);
    status = -1;
    while(status < 0){
    status = send_scsi_command(scsi_buf,
			       SCSI_rw_command(B_READ,
					       lbn[i],
					       sectors[i]*bsize),
			       i);
    //    fprintf(stderr,"send command finish status %d\n",status);
    }
    mystart[i] = ustart;
    mystart_sec[i] = ustart_sec;
    count++;
    tag++;
  }
  
  if(count == MAX_TEST){
    count = 0;
  }

  if(tag == MAX_OUTSTANDING){
    tag = 0;
  }

  while(done != 0){
    //    fprintf(stderr,"LBN %ld sample_count %d count %d\n",lbn[count],sample_count[count],count);
    
    return_id = recv_scsi_command(scsi_buf);

    while(return_id == -1){
      //      fprintf(stderr,"return id %d\n",return_id);
      return_id = recv_scsi_command(scsi_buf);
    }      

    if (return_id != tag){
      fprintf(stderr,"WARNING: Returned tag = %d (should be %d)\n",
	      return_id,tag);
      //      abort();
    }else{
      /*      fprintf(stderr,"Returned tag = %d\n",
	      return_id);*/    
    }
    mystop[tag] = ustop;
    mystop_sec[tag] = ustop_sec;
    if(tag != (MAX_OUTSTANDING-1)){
      mystart[tag+1] = ustop;
      mystop_sec[tag+1] = ustop_sec;
    }else{
      mystart[0] = ustop;
      mystop_sec[0] = ustop_sec;
    }

    mydelay = ((mystop[return_id] + (1000000 * mystop_sec[return_id])) -
	       (mystart[return_id] + (1000000 * mystart_sec[return_id])));
    
    //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[count],sectors[count],count);
    //status = -1;
    //    while(status < 0){
    busy_wait_loop(5);
    status = send_scsi_command(scsi_buf,
			       SCSI_rw_command(B_READ,
					       lbn[count],
					       sectors[count]*bsize),
			       tag);
    //      fprintf(stderr,"send command finish done %d status %d\n",count_done[count],status);
    //    }
    mystart[tag] = ustart;
    mystart_sec[tag] = ustart_sec;
    tag++;
    if(tag == MAX_OUTSTANDING){
      tag = 0;
    }
    
    
    if(count_done[count] == 0){
      //      fprintf(stderr,"not done %d\n",count_done[return_id]);
      
      if(sample_count[count] != 0){
	//sample_time[count][sample_count[count]] = mydelay;
	if(mydelay <= MAX_DELAY){
	  time[count] += mydelay;
	  sample_time_count[count]++;
	}
      }
      
      if(sample_count[count] == NO_TRIALS){
	
	/*	time[count] = 999999;
		for(i=1;i<NO_TRIALS;i++){
		if(time[count] > sample_time[count][i]){
		time[count] = sample_time[count][i];
		}
		}*/
	//	fprintf(stderr,"minimum time for %d is %ld\n",count,time[count]);
	/*qsort(&sample_time[count][1],NO_TRIALS-1,sizeof(long),
	  &comp_long);

	  time[count] = sample_time[count][NO_TRIALS/2];*/
	//	fprintf(stderr,"median time for %d is %ld\n",count,time[count]);

	if((time[count]/(NO_TRIALS-1)) <= MAX_DELAY){
	  if(last_time[count] != 0){
	    /*	  fprintf(stderr,"lbn %ld time %ld size %d\n",
		  lbn[count],
		  (time[count] - last_time[count])/(NO_TRIALS-1),
		  sectors[count]);*/
	  }
	  
	  /*	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld size %d\n",lbn[count], last_time[count]/(NO_TRIALS-1),time[count]/(NO_TRIALS-1),
		(time[count] - last_time[count])/(NO_TRIALS-1),sectors[count]);*/
	  fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld size %d last inc %d\n",lbn[count], last_time[count]/(NO_TRIALS-1),time[count]/(NO_TRIALS-1),
		  (time[count] - last_time[count])/(NO_TRIALS-1),
		  sectors[count],last_inc[count]);
	  
	  if(last_time[count] != 0 && 	   
	     ((time[count] -
	       last_time[count])/
	      (NO_TRIALS-1)) > (1 * SINGLE_SECTOR_US +
					     //	     last_time[count])/(NO_TRIALS-1)) > (1 * SINGLE_SECTOR_US +
					     TRACK_SWITCH_US)){
	    if(last_inc[count] == 1){
	      if(sectors[count] != 1){
		sectors[count]--;
	      }
	      fprintf(stderr,"TEST2 LBN %ld track size %d id %d\n",
		      lbn[count],sectors[count],return_id);
	      /*	    if(forward[count] == 0){
			    forward[count] = 1;
			    }else{*/
	      count_done[count] = 1;
	      done--;
	      if(done == 0){
		//	      return(sectors[HALF_MAX]);
	      }
	      //}
	      //return(1);
	    }else{
	      if(last_inc[count] < sectors[count]){
		sectors[count] -= last_inc[count];
	      }
	      if(sectors[count] < 0){
		fprintf(stderr,"1sector_inc %d sectors %d lb %ld\n",
			last_inc[count],
			sectors[count],
			lbn[count]);
		abort();
	      }
	      if(forward[count] == 1){
		lbn[count] += last_inc[count];
	      }
	      //fprintf(stderr,"sector here %d last %d count %d\n",sector_inc[count],last_inc[count],count);
	      sector_inc[count] = last_inc[count]/2;
	      last_inc[count] = sector_inc[count];
	      //	  sectors -= sector_inc;	    
	    }
	  }else{
	    //	fprintf(stderr,"sector here %d\n",sector_inc);
	    if(last_time[count] != 0){
	      sectors[count] += sector_inc[count];
	      if(sectors[count] < 0){
		fprintf(stderr,"2sector_inc %d\n",sector_inc[count]);
		abort();
	      }
	      //	    fprintf(stderr,"sector here %d %d\n",sector_inc[count],count);
	      last_inc[count] = sector_inc[count];
	      if(forward[count] == 1){
		if(lbn[count] - sector_inc[count] < 0){
		  sectors[count] -= (sector_inc[count] - lbn[count]);
		  lbn[count] = 0;
		  count_done[count] = 1;
		  done--;
		} 
		lbn[count] -= sector_inc[count];
	      }
	      sector_inc[count] *= 2;
	      if(sectors[count] >= MAX_SECTORS){		
		sectors[count] = start_sectors;
		if(sectors[count] < 0){
		  fprintf(stderr,"3sector_inc %d\n",last_inc[count]);
		  abort();
		}
		sector_inc[count] = inc_sectors;
		last_inc[count] = sector_inc[count];
		lbn[count] = orig_lbn[count];
		retry_count[count]++;
		if(retry_count[count] > MAX_FAIL_RETRY){
		  fprintf(stderr,"LBN %ld not found\n",lbn[count]);
		  return(-1);
		}
	      }
	    }
	  }
	  
	  last_time[count] = time[count];
	}
	time[count] = 0;
	sample_time_count[count] = 0;
	sample_count[count] = 0;
	//      fprintf(stderr,"2sectors %d sector_inc %d\n",sectors,sector_inc);
      }
    }else{
      /*      fprintf(stderr,"2LBN %ld orig %ld extra %ld diff %ld size %d last inc %d\n",lbn[count], last_time[count],time[count],
	      (time[count] - last_time[count]),sectors[count],last_inc[count]);*/
    }
    sample_count[count]++;    
    count++;
    
    if(count == (HALF_MAX+1)){
      if(sample_count[count-1] == NO_TRIALS){
	sectors[count-1] = SECT_SHIFT;	
	sample_count[count-1] = 0;
      }else{
	sectors[count-1] += SECT_SHIFT;
      }
    }
    if(count == MAX_TEST){
      count = 0;
    }
  }
  return(0);
}

long test_track_boundry2(long start_lbn,
			 int track_size,
			 long maxlbn,
			 long bsize){
  int sectors[MAX_TEST];
  int sample_count[MAX_TEST];
  long lbn[MAX_TEST];
  long time[MAX_TEST];
  long last_time[MAX_TEST];
  long mystart=0;
  long mystart_sec=0;
  long mystop=0;
  long mystop_sec=0;
  long mydelay=0;
  int status=0;
  //  int return_id=0;
  int i;
  int count=0;
  int count_trials=0;
  int firstpass=0;
  int secondpass=0;
  int done=0;
  //  long tempLBN=start_lbn;

  /*  status = send_scsi_command(scsi_buf,
      SCSI_rw_command(B_READ,
      start_lbn,
      sectors*bsize),
      COMMAND_ID);
      
      return_id = recv_scsi_command(scsi_buf);
      while(return_id == -1){
      return_id = recv_scsi_command(scsi_buf);
      }      
      
      if((start_lbn + 1000 + MAX_CACHE*MAX_FLUSHES) > maxlbn){
      start_clear = start_lbn - 1000 - (MAX_CACHE*MAX_FLUSHES);
      }else{
      start_clear = start_lbn + 1000;
      }
      for(i=0;i<MAX_FLUSHES;i++){
      clear_cache(start_clear,MAX_CACHE*bsize);
      start_clear += MAX_CACHE;
      }*/

  /*  for(i=0;i<HALF_MAX;i++){
      time[i] = 0;
      last_time[i] = 0;
      lbn[i] = tempLBN;
      tempLBN += 2 * track_size;
      sectors[i] = track_size;
      sample_count[i] = 0;
      }
      
      tempLBN -= track_size;
      
      for(i=HALF_MAX;i<MAX_TEST;i++){
      time[i] = 0;
      last_time[i] = 0;
      lbn[i] = tempLBN;
      tempLBN -= 2 * track_size;
      sectors[i] = track_size;
      sample_count[i] = 0;
      }*/

  /*  for(i=0;i<MAX_TEST;i++){
      fprintf(stderr,"i %d LBN %ld sectors %d\n",i,lbn[i],sectors[i]);
    }*/
  /*exit(0);*/

  for(i=0;i<MAX_TEST;i++){
    time[i] = 0;
    last_time[i] = 0;
    lbn[i] = start_lbn + i * track_size;
    sectors[i] = track_size;
    sample_count[i] = 0;
  }
  
  while(done != 1){
    //   fprintf(stderr,"LBN %ld sample_count %d count %d sectors %d\n",lbn[count],sample_count[count],count,sectors[count]);
    
    /*    status = send_scsi_command(scsi_buf,
	  SCSI_rw_command(B_READ,
	  lbn[count],
	  sectors[count]*bsize),
	  COMMAND_ID);*/
    //    fprintf(stderr,"send command start lbn %ld count %ld id %d\n",lbn[count],sectors[count],count);
    if(sectors[count] < 0){
      fprintf(stderr,"ERROR: Sectors less than zero %d\n",sectors[count]);
      abort();
    }
    status = exec_scsi_command(scsi_buf,
			       SCSI_rw_command(B_READ,
					       lbn[count],
					       sectors[count]*bsize));
    
    if(status == -1){
      fprintf(stderr,"id = %d\n",status);    
    }
    mystart = ustart;
    mystart_sec = ustart_sec;
    
    /*return_id = recv_scsi_command(scsi_buf);
      while(return_id == -1){
      return_id = recv_scsi_command(scsi_buf);
      } */     

    mystop = ustop;
    mystop_sec = ustop_sec;
    
    mydelay = ((mystop + (1000000 * mystop_sec)) -
	       (mystart + (1000000 * mystart_sec)));

    if(sample_count[count] != 0){
      time[count] += mydelay;
    }

    if(sample_count[count] == NO_TEST_TRIALS){

      if(last_time[count] != 0){
	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld\n",lbn[count], last_time[count]/(NO_TEST_TRIALS-1),time[count]/(NO_TEST_TRIALS-1),
		(time[count] - last_time[count])/(NO_TEST_TRIALS-1));
	/*	fprintf(stderr,"lbn %ld time %ld size %d\n",
		lbn[count],
		(time[count] - last_time[count])/(NO_TEST_TRIALS-1),
		sectors[count]);*/
      }

      if(secondpass == 1){
	/*	fprintf(stderr,"LBN %ld orig %ld extra %ld diff %ld\n",lbn[count], last_time[count]/(NO_TEST_TRIALS-1),time[count]/(NO_TEST_TRIALS-1),
		(time[count] - last_time[count])/(NO_TEST_TRIALS-1));*/
	
	if(last_time[count] != 0 && 	   
	   ((time[count] -
	     last_time[count])/(NO_TEST_TRIALS-1)) > (1 * SINGLE_SECTOR_US +
						      TRACK_SWITCH_US)){
	  sectors[count]--;
	  fprintf(stderr,"Test LBN %ld track size %d\n",lbn[count],sectors[count]);
	  //return(1);
	}else{
	  if(last_time[count] != 0){
	    return(lbn[count]);
	  }
	}
      }

      if(firstpass == 1){
	sectors[count]++;
      }

      last_time[count] = time[count];
      time[count] = 0;
      sample_count[count] = -1;

      if(secondpass == 1 && count == (MAX_TEST-1)){
	done = 1;
      }    
      if(firstpass == 1 && count == (MAX_TEST-1)){
	secondpass = 1;
      }    
      if(count == (MAX_TEST-1)){
	firstpass = 1;
      }
    }
    sample_count[count]++;    
    count++;
    
    if(count == MAX_TEST){
      count_trials++;
      count = 0;
      if(count_trials == NO_TEST_TRIALS){
	count_trials = 0;
      }
    }
    /*    if((start_lbn + 1000 + MAX_CACHE*MAX_FLUSHES) > maxlbn){
	  start_clear = start_lbn - 1000 - (MAX_CACHE*MAX_FLUSHES);
	  }else{
	  start_clear = start_lbn + 1000;
	  }
	  for(i=0;i<MAX_FLUSHES;i++){
	  clear_cache(start_clear,MAX_CACHE*bsize);
	  start_clear += MAX_CACHE;
	  }*/
  }
  return(0);
}

int find_single_track_boundry(long start_lbn,
			      int start_sectors,
			      int inc_sectors,
			      long maxlbn,
			      long bsize){
  int sectors=start_sectors;
  int sector_inc=inc_sectors;
  int last_inc=inc_sectors;
  int sample_count=0;
  long time=0;
  long last_time=0;
  long mystart=0;
  long mystart_sec=0;
  long mystop=0;
  long mystop_sec=0;
  long mydelay=0;
  int done=0;
  int status=0;
  //  int return_id=0;
  long start_clear;
  int i;
  int retry_count=0;

  while(done == 0){
    //    fprintf(stderr,"sectors %d sector_inc %d\n",sectors,sector_inc);
    
    /*    status = send_scsi_command(scsi_buf,
	  SCSI_rw_command(B_READ,
	  start_lbn,
	  sectors*bsize),
	  COMMAND_ID);
	  
	  return_id = recv_scsi_command(scsi_buf);
	  while(return_id == -1){
	  return_id = recv_scsi_command(scsi_buf);
	  }      
    */

    //    fprintf(stderr,"send command start lbn %ld count %ld\n",start_lbn,sectors);
    if(sectors < 0){
      fprintf(stderr,"ERROR: Sectors less than zero %d\n",sectors);
      abort();
    }
    status = exec_scsi_command(scsi_buf,
		      SCSI_rw_command(B_READ,
				      start_lbn,
				      sectors*bsize));
    
    if(status == -1){
      fprintf(stderr,"id = %d\n",status);    
    }
    mystart = ustart;
    mystart_sec = ustart_sec;
    mystop = ustop;
    mystop_sec = ustop_sec;
    
    mydelay = ((mystop + (1000000 * mystop_sec)) -
	       (mystart + (1000000 * mystart_sec)));

    if(sample_count != 0){
      time += mydelay;
    }

    if(sample_count == NO_TRIALS){

      /*if(last_time != 0){
	fprintf(stderr,"lbn %ld time %ld size %d inc %d last inc %d time %ld\n",
	start_lbn,
	(time - last_time)/(NO_TRIALS-1),
	sectors,sector_inc,last_inc,time);
	}*/
      
      if(last_time != 0 && 
	 ((time - 
	   last_time)/(NO_TRIALS-1)) > (last_inc * SINGLE_SECTOR_US +
			     TRACK_SWITCH_US)){
	if(last_inc == 1){
	  sectors--;
	  fprintf(stderr,"Found LBN %ld track size %d..........................\n",start_lbn,
		  sectors);
	  return(sectors);
	}else{
	  //  fprintf(stderr,"sector_inc %d\n",sector_inc);
	  if(last_inc < sectors){
	    sectors -= last_inc;
	  }
	  //	  fprintf(stderr,"2sector here %d\n",sector_inc);
	  sector_inc = last_inc/2;
	  last_inc = sector_inc;
	  // fprintf(stderr,"sectors here %d\n",sectors);	  
	}
      }else{
	//fprintf(stderr,"sector here %d\n",sector_inc);
	if(last_time != 0){
	  sectors += sector_inc;
	  last_inc = sector_inc;
	  sector_inc *= 2;
	  if(sectors >= MAX_SECTORS){
	    sectors = start_sectors;
	    sector_inc = inc_sectors;
	    last_inc = sector_inc;
	    retry_count++;
	    if(retry_count > MAX_FAIL_RETRY){
	      fprintf(stderr,"LBN %ld not found\n",start_lbn);
	      return(-1);
	    }
	  }
	}
      }

      //      fprintf(stderr,"sector here %d last %d sectors %d\n",sector_inc,last_inc,sectors);
      last_time = time;
      time = 0;
      sample_count = 0;
      //      fprintf(stderr,"2sectors %d sector_inc %d\n",sectors,sector_inc);
    }

    sample_count++;

    if((start_lbn + 1000 + MAX_CACHE*MAX_FLUSHES) > maxlbn){
      start_clear = start_lbn - 1000 - (MAX_CACHE*MAX_FLUSHES);
    }else{
      start_clear = start_lbn + 1000;
    }
    for(i=0;i<MAX_FLUSHES;i++){
      clear_cache(start_clear,MAX_CACHE*bsize);
      start_clear += MAX_CACHE;
    }
  }
  //  fprintf(stderr,"No cache\n");
  return(0);
}

void find_track_boundries(){
  long maxlbn;
  long bsize;
  //  long lbn[MAX_OUTSTANDING];
  long lbn;
  int sectors[MAX_OUTSTANDING];
  int status;
  int ids[MAX_OUTSTANDING];
  int found_id;
  //  int return_id;
  //  long mystart[MAX_OUTSTANDING],mystop[MAX_OUTSTANDING];
  //  long mystart_sec[MAX_OUTSTANDING],mystop_sec[MAX_OUTSTANDING];
  //  long mydelay;
  long time[MAX_OUTSTANDING];
  long last_time[MAX_OUTSTANDING];
  int found[MAX_OUTSTANDING];
  int sector_inc = SECTOR_INC;
  int received;
  int send_count;
  int sent;
  int i;
  int sample_count[MAX_OUTSTANDING];
  //  int done = 0;
  int track_size;
  int valid;
  int while_counter=0;
  long detect_time=0;
  long valid_time=0;
  struct timeval tvs;
  struct timeval tve;
  struct timezone tz;

  send_scsi_command(scsi_buf,SCSI_read_capacity(0,0),9999);
  status = recv_scsi_command(scsi_buf);
  while(status == -1){
    status = recv_scsi_command(scsi_buf);
  }
  maxlbn = _4btol ((u_int8_t *) &scsi_buf[0]);
  bsize = _4btol ((u_int8_t *) &scsi_buf[4]);

  if(maxlbn < MAX_CACHE){
    abort();
  }
  //  clear_cache(maxlbn, bsize);

  //  fprintf(stderr,"Cache cleared max lbn %ld max size %ld\n",maxlbn,bsize);

  received = 0;
  send_count = 0;
  sent = 0;

  for(i=0;i<MAX_OUTSTANDING;i++){
    time[i] = 0;
    last_time[i] = 0;
    //    lbn[i] = i * 1000;
    sectors[i] = SECTOR_START;
    ids[i] = 0;
    found[i] = 0;
    sample_count[i] = 0;
    //    fprintf(stderr,"LBN %ld %d\n",lbn[i],i);
  }
  //  lbn[MAX_OUTSTANDING-1] = maxlbn;
  fprintf(stderr,"max_lbn %ld max b_size %ld\n",maxlbn,bsize);

  found_id = 0;
  
  lbn = 0;
  valid = 0;
  track_size = 0;

  while(lbn < maxlbn-10){
    
    track_size = -1;
    while_counter = 0;
    gettimeofday(&tvs,&tz);
    while(track_size == -1){
      //      fprintf(stderr,"1LBN %ld\n",lbn);
      track_size = find_track_boundry(lbn,
					     sectors[0],
					     sector_inc,
					     maxlbn,
					     bsize);
      //      return;
      if(track_size == -1){
	fprintf(stderr,"Bad LBN at %ld\n",lbn);
	lbn++;
      }
      while_counter++;
      if(while_counter > 100){
	//	fprintf(stderr,"while loop find %d\n",while_counter);
      }
      //      lbn = lbn + track_size;
      //      track_size = -1;
    }
    gettimeofday(&tve,&tz);

    detect_time += ((unsigned long)tve.tv_usec + 
		    1000000 * (unsigned long)tve.tv_sec -
		    (unsigned long)tvs.tv_usec - 
		    1000000 * (unsigned long)tvs.tv_sec);

    lbn = lbn + track_size;
    //    fprintf(stderr,"2LBN %ld\n",lbn);
    
    while_counter = 0;
    //    fprintf(stderr,"blah2 v %d t %d\n",valid,track_size);
    valid = 0;
    gettimeofday(&tvs,&tz);
    while(valid == 0 && track_size != 0){
      //      fprintf(stderr,"blah\n");
      /*      valid = test_track_boundry(lbn,
	      track_size,
	      maxlbn,
	      bsize);*/
      /*      track_size = test_track_boundry(lbn,
	      track_size-1,
	      sector_inc,
	      maxlbn,
	      bsize);*/
      //      for(i=0;i<600;i++){
      //      fprintf(stderr,"3LBN %ld\n",lbn);
      while(lbn + track_size * MAX_TEST_TEST >= maxlbn){
	lbn -= track_size;
      }
      valid = test_track_boundry(lbn,
				 track_size,
				 maxlbn,
				 bsize);
      //lbn = lbn + 334 * MAX_TEST_TEST;      
      //	fprintf(stderr,"split\n");
      //      }
      //      return;
      if(valid != 0){
	lbn = valid;
      }else{
	lbn = lbn + MAX_TEST_TEST * track_size;
      }
      if(lbn >= maxlbn || (lbn + 3*track_size) >= maxlbn){
	valid = maxlbn;
      }
      //      fprintf(stderr,"4LBN %ld\n",lbn);
      while_counter++;
      if(while_counter > 1000){
	//	fprintf(stderr,"while loop verify %d\n",while_counter);
      }
    }
    gettimeofday(&tve,&tz);

    valid_time += ((unsigned long)tve.tv_usec + 
		   1000000 * (unsigned long)tve.tv_sec -
		   (unsigned long)tvs.tv_usec - 
		   1000000 * (unsigned long)tvs.tv_sec);
  }
#ifdef BLAH
  while(done == 0){
    if(sent != MAX_OUTSTANDING){

      /*      for(i=0;i<MAX_OUTSTANDING;i++){
	      if(ids[i] == 0){
	      found_id = i;
	      i = MAX_OUTSTANDING;
	      }
	      }
      */
      if(found_id == MAX_OUTSTANDING){
	found_id = 0;
      }
      ids[found_id]=0;
      
      if((lbn[found_id] + sectors[found_id]) > maxlbn){
	lbn[found_id] = maxlbn - sectors[found_id];
      }

      status = send_scsi_command(scsi_buf,
				 SCSI_rw_command(B_READ,
						 lbn[found_id],
						 sectors[found_id]*bsize),
				 found_id);
      mystart[found_id] = ustart;
      mystart_sec[found_id] = ustart_sec;
      if(found_id == 0 || found_id == (MAX_OUTSTANDING-1)){
	/*fprintf(stderr,"sent id %d to %ld for %d\n",found_id,
	  lbn[found_id],sectors[found_id]);*/
      }
      if(sample_count[found_id] == 11){
	if(found_id == 0 && last_time[found_id] != 0){
	  fprintf(stderr,"lbn %ld time %ld size %d id %d\n",lbn[found_id],
		  (time[found_id] - last_time[found_id])/10,
		  sectors[found_id],found_id);
	}

	if(last_time[found_id] != 0 && found_id == 0 && 
	   ((time[found_id] - 
	     last_time[found_id])/10) > (sector_inc * SINGLE_SECTOR_US +
					 TRACK_SWITCH_US)){
	  /*	  if(found[found_id] == 0){
		  found[found_id] = 1;
	    }*/
	  found[found_id]++;
	  if(sector_inc == 1){
	    sectors[found_id] -= sector_inc;
	    fprintf(stderr,"LBN %ld track size %d\n",lbn[found_id],
		    sectors[found_id]);
	    done = 1;
	  }else{
	    fprintf(stderr,"sector_inc %d\n",sector_inc);
	    sectors[found_id] -= 2*sector_inc;
	    sector_inc /= 2;
	    sectors[found_id] -= sector_inc;	    
	  }
	}

	if(found_id == 0){
	  sectors[found_id] += sector_inc;
	  /*fprintf(stderr,"Id %d count %d sectors %d\n",
	    found_id,sector_inc,sectors[found_id]);*/
	}

	last_time[found_id] = time[found_id];
	time[found_id] = 0;
	sample_count[found_id] = 0;
	//sectors[found_id] += SECTOR_INC;
      }
      sent++;
      send_count++;
      sample_count[found_id]++;
    }
    
    return_id = recv_scsi_command(scsi_buf);
    if(return_id != -1){      
      mystop[return_id] = ustop;
      mystop_sec[return_id] = ustop_sec;
      
      mydelay = ((mystop[return_id] + (1000000 * mystop_sec[return_id])) -
		 (mystart[return_id] + (1000000 * mystart_sec[return_id])));
      if(return_id == 0 || return_id == (MAX_OUTSTANDING-1)){
      	/*fprintf(stderr,"recieved id %d to %ld for %d time %ld\n",return_id,
	  lbn[return_id],sectors[return_id],mydelay);*/
      }
      //      fprintf(stderr,"received id %d time %ld\n",return_id,mydelay);

      if(sample_count[return_id] != 0){
	time[return_id] += mydelay;
      }
      ids[return_id] = 0;
      sent--;
      received++;
    }
    found_id++;
  }
#endif

  for(i=0;i<MAX_OUTSTANDING;i++){
    //    fprintf(stderr,"lbn %d size %d found %d\n",i*1000,sectors[i],found[i]);
  }
  fprintf(stderr,"max_lbn %ld max b_size %ld\n",maxlbn,bsize);
  fprintf(stderr,"detect time %ld valid time %ld\n",detect_time,valid_time);
}

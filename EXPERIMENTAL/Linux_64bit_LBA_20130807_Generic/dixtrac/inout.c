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
#include "extract_params.h"
#include "handle_error.h"

extern long ustart,ustop;

extern diskinfo_t *thedisk;

extern char *scsi_buf;

#define SEEK_SAMPLES     16
#define SEEK_SETS        3
  /* time is in microseconds */
#define MAX_SEEK_TIME    (1000*1000*1000)
#define USE_SEEK

#undef min
#define min(a,b) (((a)<(b))?(a):(b))
#define fetch_lbn(s)           diskinfo_cyl_firstlbn(s,0)
#define add_cyls(s,d)  \
 (((s+d)<thedisk->numcyls) ? diskinfo_cyl_firstlbn((s+d),0) : -1)

#define TEN_SEEKS 10
#define MINCYL      diskinfo_get_first_valid_cyl()
#define MAXCYL      diskinfo_get_last_valid_cyl()
#define MAXDIST     (MAXCYL-MINCYL)

void find_avrg_max(int sets,int samples,long *tset,float *max,float *mean,
     float *avrg,float *maxval, int distance, int ff, char *gname,float *time);
int find_precise_seek_time(int distance,float *max,float *mean,float *avrg,
     float *maxval, char *fname, char *gname, int fe, int ff, int fh,float *time);
void get_inout_times(float *single_cyl,float *full_seek,float *ten_seeks,
     char *fname);
void get_precise_seeks(float *single_cyl,float *full_seek,float *ten_seeks,
     char *fname);
void out_all_data(long *tset, char *fname, int distance, int fe,float *time);




void 
do_in_out_seeks(char *model_name) 
{
  float single_cyl_seek = 0;
  float full_strobe_seek = 0;
  float ten_seeks[TEN_SEEKS];

  // char bits[4];
  //  char mask[4];

  /* set bits */
  // memset((void *) bits,0x40,sizeof(bits));
  /* set mask */
  // memset((void *) mask,0x40,sizeof(mask));

  //set_mode_page_bytes(QUANTUM_PAGE,9,bits,mask,1);

  get_inout_times(&single_cyl_seek,&full_strobe_seek,ten_seeks,model_name);

}

void 
find_avrg_max(int sets,int samples,long *tset,float *max,float *mean,float *avrg,float *maxval, int distance, int ff, char *gname,float *time)
{

/* discard the values more than PERCENTAGE away from the median value */
#define PERCENTAGE    0.20
  int i,j,k,median;
  float temp;
  long *oset;
  float *total,*howmany;
  char line[255];
  int outlay,count;

  count=0;
  oset = (long *) malloc(sizeof(long)*2*sets*samples);
  assert(oset!=NULL);
  howmany = (float *) malloc(sizeof(float)*2*sets);
  assert(howmany!=NULL);
  total = (float *) malloc(sizeof(float)*2*sets);
  assert(total!=NULL);

  /* sort all the elements, for finding median */
  memcpy((void *) oset,(void *) tset, (sizeof(long)*2*sets*samples));
  for (i=0;i<2*sets*samples; i++)
    for(j=0;j<2*sets*samples; j++)
	if(oset[j]>oset[j+1]) {
	  temp = oset[j+1];
	  oset[j+1] = oset[j];
	  oset[j] = temp;
	}
  
  /* find median */
  median = oset[((2*sets*samples)/2)];

  for(j=0;j<sets*2;j++) {
    howmany[j] = 0;
    total[j] = 0;

    /* calculate sum and count valid elements */
    for(i=0;i<samples;i++) {
      if (tset[j*samples+i] !=0) {
	total[j] += tset[j*samples+i];
	howmany[j]++;
      }
    }

    /* discard the values more than PERCENTAGE away */
    for(i=0;i<samples;i++) {
      if ((tset[j*samples+i]!=0) &&
	  ((tset[j*samples+i] > median*(1+PERCENTAGE)))) {
	outlay=1;
	count++;
	sprintf(line,"%6.0ld", tset[j*samples+i]);
	if(write(ff,line,strlen(line)) < 0)
	  error_handler("Error writing file %s!", gname);
	howmany[j]--;
	total[j] -= tset[j*samples+i];
	tset[j*samples+i] = 0;
      }
    }
    /* copy to oset the trimmed dataset */
    memcpy((void *) oset,(void *) tset, (sizeof(float)*2*sets*samples));

    /* find the max value */
    for(i=0;i<samples;i++) 
      oset[j*samples+i] = tset[j*samples+i];
    for(k=0;k<samples-1;k++) {
      if(oset[j*samples+k]>oset[j*samples+k+1]) {
	temp = oset[j*samples+k+1];
	oset[j*samples+k+1] = oset[j*samples+k];
	oset[j*samples+k] = temp;
      }
    }

    maxval[j] = oset[j*samples+samples-1]; 
    if (howmany[j])
      avrg[j] = total[j] / howmany[j]; 
    else 
      avrg[j] = 0;
  }
    if(outlay==1){
      sprintf(line,"\ndistance: %d median: %5.0d discarded values: %d elapsed time: %.3f\n",
	      distance,median,count,*time);
      if (write(ff,line,strlen(line)) < 0) 
	error_handler("Error writing file %s!",gname); 
    }
    outlay=0;
  

  *max = 0;
  *mean = 0;
  for(j=0;j<sets*2;j++) {
    *max += maxval[j];
    *mean += avrg[j];
  }
  *max = *max / (sets*2);
  *mean = *mean / (sets*2);


  free(oset);
  free(howmany);
  free(total);
}

void
out_all_data(long *tset, char *fname, int distance, int fe,float *time)
{
  int i,j;
  char line[255];
  float temp;
  sprintf(line, "distance: %d total time: %3.3f\n", distance,*time);
  if (write(fe,line,strlen(line)) < 0)
    error_handler("Error writing file %s!",fname);
  for(i=0;i<SEEK_SETS;i++){
    sprintf(line,"Section %d inwards\n",i);
    if (write(fe,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);
    for(j=0;j<SEEK_SAMPLES;j++){
      temp=tset[2*i*SEEK_SAMPLES+j];
      temp=temp/1000;
      sprintf(line,"%7.3f",temp); 
      if (write(fe,line,strlen(line)) < 0)
	error_handler("Error writing file %s!",fname);
    }
    sprintf(line,"\n"); 
    if (write(fe,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);
    sprintf(line,"Section %d outwards\n",i);
    if (write(fe,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);
    for(j=0;j<SEEK_SAMPLES;j++){
      temp=tset[(2*i+1)*SEEK_SAMPLES+j];
      temp=temp/1000;
      sprintf(line,"%7.3f", temp);
      if (write(fe,line,strlen(line)) < 0) 
	error_handler("Error writing file %s!",fname); 
    }
    sprintf(line,"\n"); 
    if (write(fe,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);

  }
}

int 
find_precise_seek_time(int distance,float *max,float *mean,float *avrg,float *maxval, char *fname, char *gname, int fe, int ff, int fh,float *time)
{
  extern long ustart,ustop;
  long tset[SEEK_SETS*2][SEEK_SAMPLES];
  int startlbn,i,j,lbn,startcyl,section,range;
  long min_time_in = MAX_SEEK_TIME;
  long min_time_out = MAX_SEEK_TIME;
  int maxcyl  = MAXCYL;
  int maxdist = MAXDIST;
  int mincyl = MINCYL;
  float temp;
  /* clear the array with seek times */
  memset((void *) &tset,0,sizeof(tset));

  if (distance>maxdist)
    return(-1);
  section = (maxcyl/SEEK_SETS);


  for(j=0;j<SEEK_SETS;j++) {
    exec_scsi_command(scsi_buf,SCSI_rezero_command());
    startcyl = -1;
    do {
      range = section - distance/SEEK_SETS;
      startcyl = mincyl + (rand() % (range+1)) + range*j;

      //this should not happen       
      while ((startcyl + distance) >maxcyl) {
       startcyl--;
       }
      
      startlbn = fetch_lbn(startcyl);
      /* choose the lbn of the seek target */
      lbn = add_cyls(startcyl,distance);
    } while ((startcyl <0) || (lbn<0));


    for(i=0;i<SEEK_SAMPLES;i++) {
      /* seek to the desired cylinder */
#ifdef USE_SEEK
      exec_scsi_command(scsi_buf,SCSI_seek_command(startlbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,startlbn,1));
#endif

      /* seek inwards */
      /* printf("Seek in to cylinder %d [%d]\n",startcyl+distance,lbn); */
#ifdef USE_SEEK
      exec_scsi_command(scsi_buf,SCSI_seek_command(lbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn,1));
#endif
      tset[2*j][i] = (ustop-ustart) % 1000000;
      min_time_in = min(min_time_in,tset[2*j][i]);
      
      /* seek back to original cylinder - opposite arm movement */
#ifdef USE_SEEK
      exec_scsi_command(scsi_buf,SCSI_seek_command(startlbn));  
#else
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,startlbn,1));
#endif
      tset[2*j+1][i] = (ustop-ustart) % 1000000;
      min_time_out = min(min_time_out,tset[2*j+1][i]);

     
    }
  }
  temp = elapsed_time();

  out_all_data(&tset[0][0], fname, distance, fe,time);

  find_avrg_max(SEEK_SETS,SEEK_SAMPLES,&tset[0][0],max,mean,avrg,maxval,
		distance, ff, gname,time);

  out_all_data(&tset[0][0], fname, distance, fh,time);
  //above functions will get the time for the start of their seeks
  *time += temp;
  //update the running time

  return(0);
}

void 
get_inout_times(float *single_cyl,float *full_seek,float *ten_seeks,char *fname)
{
  int skips[6][2] = {{9,1},{19,2},{49,5},{99,10},{499,25},{MAXDIST,100}};
  float mean,max,temp;
  float avrg[2*SEEK_SETS];
  float maxval[2*SEEK_SETS];
  int i = 1;
  int idx = 0;
  int fd,j,n,fe,ff,fh;
  int counter = 0;
  char line[255], sname[255], gname[255], hname[255];
  float time=0;

  memset((void *) &avrg,0,sizeof(avrg));
  memset((void *) &maxval,0,sizeof(maxval));

  sprintf(hname, "%s.%s",fname, "discard");  
  sprintf(gname, "%s.%s",fname, "outlay");
  sprintf(sname, "%s.%s",fname, "all");  
  sprintf(fname, "%s.%s",fname, "prec.seek");  

  while(i<=MAXDIST) {
    if (i > skips[idx][0]) 
      idx++;
    i += skips[idx][1];
    counter++;
  }
  counter++;

  if ((fd = open(fname,O_WRONLY | O_CREAT | O_TRUNC,0644)) == -1) { 
      error_handler("Couldn't open file %s for seek curves!",fname); 
 } 
  if ((fe = open(sname,O_WRONLY | O_CREAT | O_TRUNC,0644)) == -1) {
    error_handler("Couldn't open file %s for seek curves!",sname);
 }
  if ((ff = open(gname,O_WRONLY | O_CREAT | O_TRUNC,0644)) == -1) {
    error_handler("Couldn't open file %s for seek curves!",gname);
  }
  if ((fh = open(hname,O_WRONLY | O_CREAT | O_TRUNC,0644)) == -1) {
    error_handler("Couldn't open file %s for seek curves!",hname);
  }

  i = 1;
  idx = 0;
  temp = elapsed_time();
  while(i < MAXDIST) {
    find_precise_seek_time(i,&max, &mean,&avrg[0],&maxval[0],sname,
			   gname,fe,ff,fh,&time);
    mean = mean / 1000;

    for (n=0; n<SEEK_SETS*2; n++){
      avrg[n] = avrg[n] / 1000;
      maxval[n] = maxval[n] / 1000;
    }
    sprintf(line,"%d, %8.5f\n",i,mean);
    if (write(fd,line,strlen(line)) < 0) 
      error_handler("Error writing file %s!",fname);
    for (j=0; j<SEEK_SETS; j++){
      sprintf(line, "%8.4f%8.4f",avrg[2*j],avrg[2*j+1]);
      if (write(fd,line,strlen(line)) < 0)
	error_handler("Error writing file %s!",fname);
    }
    for (j=0; j<SEEK_SETS; j++){
      sprintf(line, "%8.4f%8.4f",maxval[2*j],maxval[2*j+1]);
      if (write(fd,line,strlen(line)) < 0)
	error_handler("Error writing file %s!",fname);
    }
    sprintf(line,"\n");
    if (write(fd,line,strlen(line)) < 0) 
      error_handler("Error writing file %s!",fname);
    if (i > skips[idx][0]) 
      idx++;
    if (i < 11) {
      *ten_seeks = mean;
      ten_seeks++;
    }
    if (i==1) 
      *single_cyl = mean;
    i += skips[idx][1];
  }

  /* determine a full strobe seek cylinder range */
  i = MAXDIST;
  find_precise_seek_time(i,&max, &mean,&avrg[0],&maxval[0],sname,
			 gname,fe,ff,fh,&time);
  mean=mean/1000;
  for (n=0; n<SEEK_SETS*2; n++){
    avrg[n] = avrg[n] / 1000;
    maxval[n] = maxval[n] / 1000;
  }
  *full_seek = mean / 1000;
  sprintf(line,"\n%d, %8.5f\n",i,mean);
  if (write(fd,line,strlen(line)) < 0)
    error_handler("Error writing file %s!",fname);
  for (j=0; j<SEEK_SETS; j++){
    sprintf(line, "%8.4f%8.4f",avrg[2*j],avrg[2*j+1]);
    if (write(fd,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);
  }
  for (j=0; j<SEEK_SETS; j++){
    sprintf(line, "%8.4f%8.4f",maxval[2*j],maxval[2*j+1]);
    if (write(fd,line,strlen(line)) < 0)
      error_handler("Error writing file %s!",fname);
  }
  close(fd);
  close(fe);
}



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
#include "print_bscsi.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "scsi_conv.h"
#include "read_diskspec.h"
#include "mtbrc.h"
#include "mechanics.h"
#include "trace_gen.h"
#include "paramfile_gen.h"
#include "cache.h"
#include "handle_error.h"
#include "dxopts.h"

extern diskinfo_t *thedisk;
extern char       *scsi_buf;

typedef struct drt {
  double arrtime;
  long   runtime;
  double endtime;
  int    lbn,size,type;
  int    reqid;
  int    flags;
  struct drt *next;
  struct drt *prev;
} diskreq_t;


#define EXTRAS             6

/* this is the fraction of blocks to reconstruct before quitting */
#define STOP_POINT         1.0

#define BINS       11
/*  int percentages[] = {50,75,80,85,90,95,99,100}; */
int percentages[] = {5,10,25,50,75,80,85,90,95,99,100};

typedef struct sts {
  int    totalreqs;
  int    idlereqs;
  int    failures;

  int    outstanding;

  double sumdist;
  double sumdist_square;
  int    maxdist;
  
  double fg_sumseek;
  double fg_sumseek_square;
  double fg_rtime;
  double fg_rtime_square;
  double idle_rtime;
  double idle_rtime_square;

  int    idleseq;
  int    max_idleseq;
  int    idleseq_sum;

  struct {
    double avg_time;
    double var_time;
    double avg_seek;
    double avg_outstanding;
  } fg_reqs[BINS];

  struct {
    double avg_time;
    double var_time;
    double avg_distance;
    double var_distance;
    double avg_idleseq;
    int    max_idleseq;
    int    failed;
  } idle_reqs[BINS];

  int     perc[BINS];
  int     idx;
} stats_t;

stats_t  stats;

int totalblocks = 0;

double lambda = 2.0;
int size   = 8;
int type   = B_READ;

double currtime,begintime;

#define SCRUB_NEXT_TRACK           -1
#define SCRUB_NEAREST_TRACK        -2

int scrub_blkno;
int scrub_bcount;

#define PERMANENT_CUSTOMER          0x01
#define VACATIONING_SERVER          0x10

#define PERMANENT_CUSTOMER_STR      "Permanent Customer"
#define VACATIONING_SERVER_STR      "Vacationing Server"

int modeltype = VACATIONING_SERVER;
int algorithm = SCRUB_NEAREST_TRACK;

int bgtasks   = 1;
#define FGTASKS              250

/* in seconds */
#define ARRIVAL_WINDOW           0.1

#define DEBUG_PRINT(A...)        printf(A)

#define FRGRND           0x01
#define MEDIA_VERIFY     0x08
#define REQ_AT_DISK      0x10
#define REQ_FINISHED     0x80


#if HAVE_GETOPT_H
static struct option const longopts[] =
{
  DX_COMMON_OPTS_LIST

  {"cache-parameters",  required_argument, NULL, 'c'},
  {"lambda",            required_argument, NULL, 'b'},
  {"size",              required_argument, NULL, 's'},
  {"writes",            no_argument, NULL, 'w'},
  {"permanent",         no_argument, NULL, 'n'},
  {"ordered",           no_argument, NULL, 'o'},

  {0,0,0,0}
};
#endif

static char const * const option_help[] = {
"-c  --cache-parameters  Set the disk's read and write cache to the argument.",
"-b  --lambda            the arrival rate of requests (arrivals per second).",
"-s  --size              the size of foreground requests (in blocks).",
"-w  --writes            use WRITEs instead of READs for foreground requests.",
"-n  --permanent         permanent customer model instead of vacationing.",
"-o  --ordered           use ordered reconstruction instead of greedy.",

 DX_COMMON_OPTS_HELP

0
};

#define OPTIONS_STRING   "ahtu:g::rm:lfc:b:s:won"

/* function declarations */
int
get_scrub_blocks(diskinfo_t *currdisk,diskreq_t **queue,int *start,int *bcount);



/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
get_initial_time(void) 
{
#define MSEC      0.000001
  struct timeval tv;
  struct timezone tz;
  
  gettimeofday(&tv,&tz);  
  begintime = tv.tv_sec + MSEC * tv.tv_usec;
  currtime = 0;
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static double
get_current_time(void) 
{
  struct timeval tv;
  struct timezone tz;
  
  gettimeofday(&tv,&tz);  
  currtime = tv.tv_sec + MSEC * tv.tv_usec - begintime;
  return(currtime);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static double
get_poisson_arrival(double lambda) 
{
  double x,u;
  u = (double) rand() / (double) RAND_MAX;
  x = (-1) / lambda * log(1 - u);
  return(x);
  /*  return(lambda * exp((-lambda) * x)); */
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
inline static int
get_random_lbn(void) 
{
  return(rand() % (thedisk->numblocks-1));
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static diskreq_t*
get_new_request(double currtime, double lambda,int size, int type)
{
  diskreq_t *r;
  double poisson;
  if (!(r = (diskreq_t *) malloc(sizeof(diskreq_t)))) {
    error_handler("Cound not allocate memory!\n");
  }
  poisson = get_poisson_arrival(lambda);
  r->arrtime = poisson + currtime;

  while ((r->lbn = get_random_lbn()) >= (thedisk->numblocks - size - 2));
  
  r->size = size;
  r->type = type;
  r->flags = FRGRND;

  return(r);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
enqueue_request(diskreq_t **queue, diskreq_t *request)
{
  diskreq_t *curr = *queue;
  diskreq_t *last = NULL;
  while ((curr) && (request->arrtime > curr->arrtime)) {
    last = curr;
    curr = curr->next;
  }
  request->prev = last;
  if (last)
    last->next = request;
  else
    *queue = request;
  request->next = curr;
  if (curr)
    curr->prev = request;

  request->reqid = 1; /* get_new_id(); */
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static diskreq_t*
curr_request(diskreq_t **queue)
{
  diskreq_t *curr = *queue;
  diskreq_t *last = NULL;
  while ((curr) && (currtime > curr->arrtime)) {
    last = curr;
    curr = curr->next;
  }
  return(last);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static diskreq_t*
dequeue_request(diskreq_t **queue)
{
  diskreq_t *curr = *queue;
  if (curr) {
    *queue = curr->next;
    if (curr->next) {
      (curr->next)->prev = NULL;
    }
    return(curr);
  }
  else
    DEBUG_PRINT("Warning, queue is empty, nothing dequeued!\n");
  return(NULL);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static diskreq_t*
last_request(diskreq_t **queue,int *count)
{
  int num = 0;
  diskreq_t *curr = *queue;
  if (curr) {
    while (curr->next) {
      curr = curr->next;
      num++;
    }
    if (count)
      *count = num;
    return(curr);
  }
  else {
    return(NULL);
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static int 
count_outstanding(diskreq_t **queue) 
{
  int counter = 0;
  diskreq_t *curr = *queue;
  while (curr) {
    if ((curr->arrtime < currtime) && (!(curr->flags & REQ_FINISHED)))
      counter++;
    curr = curr->next;
  }

  return(counter);
   
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
print_queue(diskreq_t **queue) 
{
  diskreq_t *curr = *queue;
  while (curr) {
    DEBUG_PRINT("%s [%9.3f]: lbn %d\t size %d\tflags: 0x%02x\n",
		((curr->flags & FRGRND) ? "FG  " : "IDLE"),
		curr->arrtime,curr->lbn,curr->size,curr->flags);
    curr = curr->next;
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
generate_requests(diskreq_t **queue)
{
  double temptime;
  diskreq_t *newreq;
  diskreq_t *tempreq;
  int count;
  
  if ((tempreq = last_request(queue,&count))) 
    temptime = tempreq->arrtime;
  else
    temptime = currtime + ARRIVAL_WINDOW;

  /* if (count > EXTRAS) 
     DEBUG_PRINT("  Enough in advance, not generating new requests...\n"); */
  while (((currtime + ARRIVAL_WINDOW) > temptime) && (count < EXTRAS)) {
    /*  DEBUG_PRINT("\tGenerated a request in advance\n"); */
    /* generate random workload */
    newreq = get_new_request(temptime,lambda,size,type);
    /* put on the queue */
    enqueue_request(queue,newreq);
    temptime = newreq->arrtime;
    /*  DEBUG_PRINT("\t  temptime: %f   currtime: %f\n",temptime,currtime); */
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
issue_request(diskreq_t **queue)
{
  /* static int counter = 0; */
  diskreq_t *r = *queue;
  double tarr,tcurr;
  
  /* get the time from of the request */ 
  tarr = r->arrtime;

  /* busy loop until it's the time */
  do {
    tcurr = get_current_time();
  } while (tarr > tcurr);

  send_scsi_command(scsi_buf,SCSI_rw_command(r->type,r->lbn,r->size),r->reqid);
  /*
  if (r->flags & FRGRND)
    DEBUG_PRINT("Sending %s: lbn %9d\t size %4d\t%9.6f s\n",
		((r->flags & FRGRND) ? "Foreground" : "Idle      "),
		r->lbn,r->size,r->arrtime);
  */
  r->flags |= REQ_AT_DISK;
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static diskreq_t*
find_idle_request(diskreq_t **queue)
{
  int blkno,bcount;
  if (get_scrub_blocks(thedisk,queue,&blkno,&bcount)) {
    diskreq_t *r;
    if (!(r = (diskreq_t *) malloc(sizeof(diskreq_t)))) {
      error_handler("Cound not allocate memory!\n");
    }
    r->lbn = blkno;
    r->size = bcount;
    r->type = B_READ;
    r->flags = MEDIA_VERIFY;
    return(r);
  }
  else {
    /* we have scrubbed the whole disk, stop simulation */
    fprintf(stdout,"Run finished in %f seconds.\n",currtime);
    return(NULL);
  }

}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
receive_request(diskreq_t **queue)
{
  int id;
  diskreq_t *curr = *queue;
  id = recv_scsi_command(scsi_buf);

  currtime = get_current_time();
  curr->endtime = currtime;
  curr->flags &= ~REQ_AT_DISK;
  curr->flags |= REQ_FINISHED;

  /*
  if (curr->flags & FRGRND)
    DEBUG_PRINT("The response time: %f\n",(curr->endtime-curr->arrtime)* 1000);
  */
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
init_stats(stats_t *stats)
{
  stats->totalreqs = 0;
  stats->idlereqs = 0;
  stats->failures = 0;
  stats->sumdist = 0;
  stats->sumdist_square = 0;

  stats->failures = 0;
  stats->outstanding = 0;

  stats->maxdist = (int) (log((double) thedisk->numcyls) / log(2.0));

  stats->fg_sumseek = 0;
  stats->fg_sumseek_square = 0;
  stats->fg_rtime = 0;
  stats->fg_rtime_square = 0;
  stats->idle_rtime = 0;
  stats->idle_rtime_square = 0;

  stats->idleseq = 0;
  stats->max_idleseq = 0;
  stats->idleseq_sum = 0;

  stats->idx = 0;
  memcpy(stats->perc,percentages,sizeof(int)*BINS);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
update_stats(stats_t *stats,diskreq_t *prev,diskreq_t *curr, int outstanding)
{
  double resptime;
  int    seekdist,val;

#define VARIANCE(A,B,N)  (((A * (double) N)-pow(B,2)) / (N*(N-1)))


  resptime = (curr->endtime - curr->arrtime) * 1000;
  stats->totalreqs++;
  stats->outstanding++;
  {
    int cyl1,cyl2,surface,sect,lbn1,lbn2;
    lbn1 = curr->lbn;
    if (prev)
      lbn2 = prev->lbn + prev->size - 1;
    else
      lbn2 = lbn1;

    disk_translate_lbn_to_pbn(thedisk,lbn1,MAP_FULL,&cyl1, &surface, &sect);
    disk_translate_lbn_to_pbn(thedisk,lbn2,MAP_FULL,&cyl2, &surface, &sect);
    seekdist = abs(cyl1 - cyl2);
  }

  if (curr->flags & MEDIA_VERIFY) {
    stats->idlereqs++;
    stats->idle_rtime += resptime;
    stats->idle_rtime_square += resptime*resptime;

    if (seekdist> stats->maxdist)
      stats->failures++;

    if (stats->idlereqs > 1) {
      stats->sumdist += (double) seekdist;
      stats->sumdist_square += (double) seekdist * (double) seekdist;
    }
	
    stats->idleseq++;
  } 
  else { 
    stats->fg_rtime += resptime;
    stats->fg_rtime_square += resptime*resptime;

    stats->fg_sumseek += (double) seekdist;
    stats->fg_sumseek_square += (double) seekdist * (double) seekdist;

    stats->idleseq_sum +=stats->idleseq;
    if (stats->max_idleseq < stats->idleseq)
      stats->max_idleseq = stats->idleseq;

    if (stats->idle_reqs[stats->idx].max_idleseq < stats->idleseq)
      stats->idle_reqs[stats->idx].max_idleseq = stats->idleseq;

    stats->idleseq = 0;
  }

  val = (int) ((double) totalblocks / (double) thedisk->numblocks * 100.0);

  /*  val = (int) ((double) (stats->perc[stats->idx]) *  */
  /*	       (double)  thedisk->numcyls / 100.0);      */

  if (stats->perc[stats->idx] <= val) {
    int idx = stats->idx;
    int numreqs;

    /*  DEBUG_PRINT("Val: %d perc: %d\t%d\n",val,stats->perc[stats->idx], */
    /*   	totalblocks); */
    DEBUG_PRINT("%d%% of reconstruction done.\n",val);

    numreqs = stats->idlereqs;
    stats->idle_reqs[idx].avg_time = stats->idle_rtime / numreqs;
    stats->idle_reqs[idx].var_time = VARIANCE(stats->idle_rtime_square,
					      stats->idle_rtime,numreqs);
    
    stats->idle_reqs[idx].avg_distance = stats->sumdist / numreqs;
    stats->idle_reqs[idx].var_distance = VARIANCE(stats->sumdist_square,
						  stats->sumdist,numreqs);
    
    stats->idle_reqs[idx].failed = stats->failures;

    numreqs = stats->totalreqs - stats->idlereqs;
    /* this should be here, since we want the number of foreground reqs */
    stats->idle_reqs[idx].avg_idleseq = (double) stats->idleseq_sum / 
      (double) numreqs;
 
    stats->fg_reqs[idx].avg_time = stats->fg_rtime / numreqs;
    stats->fg_reqs[idx].var_time = VARIANCE(stats->fg_rtime_square,
					    stats->fg_rtime,numreqs);

    stats->fg_reqs[idx].avg_seek = stats->fg_sumseek / numreqs;

    stats->fg_reqs[idx].avg_outstanding = (double)stats->outstanding / 
      (double) numreqs;

    stats->idx++;
  }
}
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
print_stats(stats_t *stats)
{
  int i,numreqs;

#define PRINT_STATS(A...)          printf(A)
  PRINT_STATS("Total number of requests:           \t%d\n",stats->totalreqs);
  PRINT_STATS("Total number of foreground requests:\t%d\n",
	      stats->totalreqs - stats->idlereqs);
  PRINT_STATS("Total number of idle requests:       \t%d\n",
	      stats->idlereqs);

  numreqs = stats->idlereqs;
  if (bgtasks) {

    PRINT_STATS("\nIDLE REQUESTS STATS\n");
    PRINT_STATS("Seeks past log(N) value:             \t%d \n",
		stats->failures);
    PRINT_STATS("log(N) seek distance:                \t%d cyls\n",
		stats->maxdist);
    PRINT_STATS("Avg. response time:                  \t%6f\n",
		(double) stats->idle_rtime / stats->idlereqs);
    PRINT_STATS("Response time variance:              \t%6f\n",
		VARIANCE(stats->idle_rtime_square,stats->idle_rtime,numreqs));

    PRINT_STATS("Avg. seek distance:                  \t%.1f cyls\n",
		(double) stats->sumdist / stats->idlereqs);
    /* PRINT_STATS("Seek variance:                       \t%4.3f cyls\n",
       VARIANCE(stats->sumdist_square,stats->sumdist,numreqs)); */
    PRINT_STATS("Avg. idle requests in one sequence:   \t%4.1f\n",
		(double) stats->idleseq_sum / (double) stats->idlereqs);
    PRINT_STATS("Max idle requests in one sequence:   \t%d \n",
		stats->max_idleseq);
    
    PRINT_STATS("\n");
    PRINT_STATS("                seek distance   \tresponse time(ms)\tin sequence\n");
    PRINT_STATS("      failed    AVG          VAR\tAVG           VAR\tAVG     MAX\n");
    PRINT_STATS("----------------------------------------------------------------------------\n");
    
    for (i= 0; i < stats->idx; i++ ) {
      /* printf("%3d%%  %6d  %6.1f      %6.3f\t%6.3f     %6.3f\t%3.1f    %3d\n",*/
      printf("%3d%%  %6d    %6.1f             \t%6.3f     %6.3f\t%3.1f    %3d\n",
	     stats->perc[i],
	     stats->idle_reqs[i].failed,
	     stats->idle_reqs[i].avg_distance,
	     /*  stats->idle_reqs[i].var_distance, */
	     stats->idle_reqs[i].avg_time,
	     stats->idle_reqs[i].var_time,
	     stats->idle_reqs[i].avg_idleseq,
	     stats->idle_reqs[i].max_idleseq );
    }
  }
  PRINT_STATS("\nFOREGROUND REQUESTS STATS\n");
  numreqs = stats->totalreqs - stats->idlereqs;

  PRINT_STATS("Avg. response time:                  \t%6f\n",
	      (double) stats->fg_rtime / numreqs);
  PRINT_STATS("Response time variance:              \t%6f\n",
	      VARIANCE(stats->fg_rtime_square,stats->fg_rtime,numreqs));

  PRINT_STATS("Avg. seek distance:                  \t%.1f cyls\n",
	      (double) stats->fg_sumseek / numreqs);
  /* PRINT_STATS("Seek variance:                       \t%.3f cyls\n",
     VARIANCE(stats->fg_sumseek_square,stats->fg_sumseek,numreqs)); */
  PRINT_STATS("Avg. number of outstanding requests:\t%.1f\n",
	      (double) stats->outstanding / (double) numreqs);

  PRINT_STATS("\n");
  PRINT_STATS("      seek distance\tresponse time(ms)\t\toutstanding\n");
  PRINT_STATS("           AVG     \tAVG           VAR\t\t   AVG   \n");
  PRINT_STATS("----------------------------------------------------------------------------\n");

  for (i= 0; i < stats->idx; i++ ) {
    printf("%3d%%       %6.1f\t  %6.3f   %6.3f\t\t%6.1f\n",
	   stats->perc[i],
	   stats->fg_reqs[i].avg_seek,
	   stats->fg_reqs[i].avg_time,
	   stats->fg_reqs[i].var_time,
	   stats->fg_reqs[i].avg_outstanding);
  }
}

static void
print_preamble(void)
{

  PRINT_STATS("\n");
  PRINT_STATS("Arrival rate (lambda): %d per second\n",(int) lambda);
  PRINT_STATS("I/O size:              %d blocks \n",size);
  PRINT_STATS("I/O type:              %s\n",(type == B_READ ? "READ":"WRITE"));
  PRINT_STATS("Algorithm:             %s\n",(algorithm == SCRUB_NEXT_TRACK ?
					     "Ordered" :
					     "Greedy"));
  PRINT_STATS("Model:                 %s\n",(modeltype == PERMANENT_CUSTOMER ?
					     PERMANENT_CUSTOMER_STR : 
					     VACATIONING_SERVER_STR));
  PRINT_STATS("\n");
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
run_experiment(double lambda, int size, int type) 
{
  diskreq_t *req,*prevreq,*idlereq;
  diskreq_t *queue;
  int outstanding,stop = 0;

  int fgtask_counter = 0;

  req = prevreq = idlereq = NULL;
  queue = NULL;
  
  print_preamble();
  get_initial_time();
  init_stats(&stats);

  /* prime the disk */
  req = get_new_request(currtime,lambda,size,type);
  if (modeltype == PERMANENT_CUSTOMER)
    req->flags |= MEDIA_VERIFY;
  enqueue_request(&queue,req);

  do {
    issue_request(&queue);
    generate_requests(&queue);
    receive_request(&queue);
    
    outstanding = count_outstanding(&queue);

    if (bgtasks) {
      if (((modeltype == VACATIONING_SERVER) && (outstanding == 0)) || 
	  ((modeltype == PERMANENT_CUSTOMER) && 
	   (queue->flags & MEDIA_VERIFY))) {
	/* figure out an idle request */
	if ((idlereq = find_idle_request(&queue))) {
	  /*  DEBUG_PRINT("Issued background request: %f\n",currtime); */
	  idlereq->arrtime = currtime;
	  enqueue_request(&queue,idlereq);
	  /*  print_queue(&queue); */
	}
	else 
	  stop = 1;
      }
      /*  DEBUG_PRINT("request: %f\n",currtime); */
      /*  print_queue(&queue); */
    }
    else {
      fgtask_counter++;
      if (fgtask_counter >= FGTASKS)
	stop = 1;
    }
    /* remove the finished request from the queue */
    req = dequeue_request(&queue);

    update_stats(&stats,prevreq,req,outstanding);
    free(prevreq);
    prevreq = req;

  } while(!stop);
  free(req);

  print_stats(&stats);
}
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
usage(FILE *fp,char *program_name) 
{
   fprintf(fp,"Usage: %s [-hdtalgfrvbs] [-c BITS] -m <model_name> <disk>\n",
	  program_name);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
print_help(FILE *fp,char *program_name) 
{
  char const * const *p;
  usage(fp,program_name);
  for (p = option_help;  *p;  p++)
    fprintf(fp,"  %s\n", *p);
  fprintf(fp,"<disk> is the corresponding SCSI raw device.\n");

}

/****************************************************************************
 *
 *
 ****************************************************************************/
int 
main (int argc, char *argv[]) 
{
   char c;
   int diskno;
   char *program_name;

   int cache_bits = READ_CACHE_DISABLE | WRITE_CACHE_DISABLE;
   int turn_cache = 0;
   /* 
    * start of common options 
    */
   DX_COMMON_VARIABLES;
   /* 
    * end of common options
    */

   program_name = argv[0];
   while (strpbrk(program_name++,"/")) ;
   program_name--;

   while ((c = GETOPT(argc, argv,OPTIONS_STRING)) != EOF) {
     switch (c) {
     case 'c':
       turn_cache = 1;
       {
	 char *endptr;
	 cache_bits =  (int) strtol(optarg, &endptr, 10);
	 if (!strncmp(endptr,"ON",2)) {
	   cache_bits = 4;
	 }
	 else if (!strncmp(endptr,"OFF",3)) {
	   cache_bits = 1;
	 }
	 else if ((cache_bits < 0) || (cache_bits > 8)) {
	   error_handler("The option -c takes values ON, OFF or a number between 0 and 8!\n");
	 }
       }
       break;
     case 'b':
       lambda = (double) atoi(optarg);
       break;
     case 's':
       size = atoi(optarg);
       break;
     case 'w':
       type = B_WRITE;
       break;
     case 'n':
       modeltype = PERMANENT_CUSTOMER;
       break;
     case 'o':
         algorithm = SCRUB_NEXT_TRACK;
       break;
     /* 
      * start of common options 
      */
      DX_COMMON_OPTIONS;
     /* 
      * end of common options
      */
     }
   }
   /* 
    * start of common options check
    */
   DX_COMMON_OPTS_CHK;
   /* 
    * end of common options check
    */
   
   diskno = disk_init(argv[optind],use_scsi_device);

   /* set the seed for the random number generator */
   srand((unsigned int) getpid());

   if (turn_cache) {
     set_cache(cache_bits);
     /* disable_caches(); */
     set_cache_bit(CACHE_DRA,0);
   }
   
#ifdef MEASURE_TIME
   elapsed_time();
#endif
   /* 
    * start of common options handling
    */
   DX_COMMON_OPTS_HANDLE;
   /* 
    * end of common options handling
    */

#ifdef MEASURE_TIME
   elapsed_time();
#endif
   scrub_bcount = algorithm;
   run_experiment(lambda,size,type);
#ifdef MEASURE_TIME
   elapsed_time();
#endif
   disk_shutdown(use_scsi_device);
   return(0); 
}

/**********************************************************************
 **********************************************************************
 **********************************************************************
 **********************************************************************/
static int
find_nearest_track(disk *currdisk,int lastblkno,int *start,int *bcount) {
  static int initialize = 1; 
  static u_int32_t *scrubbed_map;
  band *currband;
  int cylno,surfaceno,sect,end;
  int i,lbn,found,dist;
  u_int32_t bitmap;
  
  if (initialize) {
    if (!(scrubbed_map = 
	  (u_int32_t *)malloc(currdisk->numcyls*sizeof(u_int32_t)))) {
      fprintf(stderr,"Couldn't allocate scrubbed_map!\n");
      exit(0);
    }
    memset(scrubbed_map,0,currdisk->numcyls*sizeof(u_int32_t));
    lbn = 0;
    while (lbn < currdisk->numblocks) {
      /*  DEBUG_PRINT("LBN: %d  [cyl %d, surface %d]\n",lbn,cylno,surfaceno); */
      currband = disk_translate_lbn_to_pbn(currdisk, lbn, MAP_FULL, 
					   &cylno, &surfaceno, &sect);
      disk_get_lbn_boundaries_for_track(currdisk, currband, cylno, surfaceno,
					start, &end);
      /* the bitmap with surface numbers has bit 1 being eqiv. to surface 0 */
      scrubbed_map[cylno] |= (u_int32_t) (0x01 << surfaceno);
      lbn = end;
    }
    initialize = 0;
  }
  /* make sure lastblkno is valid lbn */
  /* this can happe if lastlbn was on the last track */
  if (lastblkno >= currdisk->numblocks)
    lastblkno = currdisk->numblocks - 1;
  disk_translate_lbn_to_pbn(currdisk, lastblkno, MAP_FULL, 
			    &cylno, &surfaceno, &sect);
  found = 0;
  dist = 0;
  do {
    if (((cylno+dist) < currdisk->numcyls) && (scrubbed_map[cylno+dist] > 0)) {
      cylno += dist;
      found = 1;
      break;
    }
    else if (((cylno-dist) >= 0) && (scrubbed_map[cylno-dist] > 0)) {
      cylno -= dist;
      found = 1;
      break;
    }
    dist++;
  } while (((cylno+dist) < currdisk->numcyls) || ((cylno-dist) >= 0));
  
  if (found) {
    surfaceno = 0;
    bitmap = scrubbed_map[cylno];
    while ( !(bitmap & 0x01) ) {  
      bitmap = bitmap >> 1;
      surfaceno++;  
    }
    /* get the currband without actually getting lbn translations */
    i = 0;
    do {
      if ((currdisk->bands[i].startcyl <= cylno) && 
	  (currdisk->bands[i].endcyl >= cylno)) 
	break;
      i++;
    } while (i < currdisk->numbands);
    currband = &currdisk->bands[i];
    disk_get_lbn_boundaries_for_track(currdisk, currband, cylno, surfaceno,
				      start, &end);
    *bcount = (end - *start);
    scrubbed_map[cylno] &= ~((u_int32_t) (0x01 << surfaceno));
  }
  return(found);
}

int
get_scrub_blocks(diskinfo_t *currdisk,diskreq_t **queue,int *start,int *bcount)
{
  int rc = 1; 
  if (scrub_bcount > 0) {
    *start = scrub_blkno;
    if (currdisk->numblocks < (scrub_blkno+scrub_bcount)) {
      /* the block count for scrubbing is larger than the maxblock number */
      *bcount = currdisk->numblocks  - scrub_blkno;
    }
    else {
      *bcount = scrub_bcount;
    }
    scrub_blkno += *bcount;
  }
  else if (scrub_bcount == SCRUB_NEXT_TRACK) {
    int cylno,surfaceno,sect,end;
    band *currband = disk_translate_lbn_to_pbn(currdisk, scrub_blkno,
					  MAP_FULL, &cylno, &surfaceno, &sect);
    disk_get_lbn_boundaries_for_track(currdisk, currband, cylno, surfaceno,
				      start, &end);
    *bcount = (end - *start);
    scrub_blkno += *bcount;
  }
  else if (scrub_bcount == SCRUB_NEAREST_TRACK) {
    diskreq_t *r = *queue;
    if (modeltype == PERMANENT_CUSTOMER) {
      if (!(r = curr_request(queue)))
	r = *queue;
    }
    rc = find_nearest_track(currdisk,(r->lbn + r->size - 1),start,bcount);
    /* keep the current blkno in the variable, not strictly necessary */
    scrub_blkno = *start;
  }
  else{
    fprintf (stderr, "get_scrub_block: bad block count: %d!\n",
	     scrub_bcount);
    exit (0);
  }
  if ((totalblocks += *bcount) >= 
      (int) ((double) currdisk->numblocks) * (double) STOP_POINT)
    rc = 0;
  return(rc);
}

/**********************************************************************
 **********************************************************************
 **********************************************************************
 **********************************************************************/

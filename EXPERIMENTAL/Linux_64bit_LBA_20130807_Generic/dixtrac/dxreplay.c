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


#include <libddbg/libddbg.h>

#include "dixtrac.h"
#include "scsi_conv.h"
#include "trace_gen.h"
#include "handle_error.h"
#include "dxopts.h"

/* #define PRINT_TRACE */
/* #define CATCH_SIGNAL */

#ifdef CATCH_SIGNAL
#include <signal.h>
static void sig_handler(int signo);
#endif

#define NOT_COMPLETED           -1

static struct timeval tracetime;
static struct timeval begintime;

static int ignore_sleep_time = 0;
static int trace_source = GENERATED_TRACE;
static double lambda;

#define MAX_OUTSTANDING    15
static int maxqdepth = MAX_OUTSTANDING;

#define MAXEVENTS           1000

#define MAXFLIST            2*(MAX_OUTSTANDING+MAXEVENTS)

void * issue_request(void);
void * collect_request(void);
void * read_trace(char *model_name);

#define error_handler printf



typedef struct request_s {
  unsigned int lbn;
  unsigned int blocks;
  unsigned int flags;
  int tag;
  /* struct timeval timestamp; */
  struct timeval start;
  long compl_time;
  long sleep_time;
  struct request_s *next;
} request_t;

void  run_single_outstanding(request_t *request); 

/* the output trace file*/
FILE *outfp = NULL;

st_thread_t lbnreader;
st_thread_t issuer;
st_thread_t collector;


static request_t *disk_queue_head = NULL;
static request_t *disk_queue_tail = NULL;

static request_t *event_queue_head = NULL;
static request_t *event_queue_tail = NULL;

extern char *scsi_buf;

static st_cond_t count_ios;
static st_cond_t io_issued;
static st_cond_t event_signal;

// static pthread_mutex_t lastissued_mutex = PTHREAD_MUTEX_INITIALIZER;

// static st_mutex_t count_mutex;
static st_mutex_t lastreq_mutex;

static st_mutex_t queue_mutex;
static st_mutex_t free_list_mutex;

static request_t *free_list_head = NULL;
static request_t *free_list_tail = NULL;

/* shared variables/counters */
static int count = 0;
static int lastreq = 0;
static int lastissued = 0;
static int events = 0;

static int maxlbn = 1000000;

int use_scsi_device = 1;

static int genreqs = 0;
static int use_finite       = 0;


#if HAVE_GETOPT_H
static struct option const longopts[] =
{
  DX_COMMON_OPTS_LIST
  {"use-lbn-file",        no_argument, NULL, 'b'},
  {"use-synthtrace",    no_argument, NULL, 's'},
  {"ignore-sleeptime",    no_argument, NULL, 'i'},
  {"llt-trace",           no_argument, NULL, 't'},
  {"queue-depth",         required_argument, NULL, 'q'},
  {"lambda",              required_argument, NULL, 'c'},
  {"numreqs",             required_argument, NULL, 'n'},
  {"disk-end",              required_argument, NULL, 'z'},
  {0,0,0,0}
};
#endif

static char const * const option_help[] = {
"-b  --use-lbn-file      the trace is sequence of LBNs and blocks.",
"-s  --use-synthtrace    Use DiskSim trace for generating validation trace.",
"-c  --lambda            the arrival rate of requests (arrivals per second).",
"-n  --numreqs           number of random requests to generate.",
"-z  --disk-end          the max LBN used in generating synth trace.",
"-t  --llt-trace         the trace is in LLT format.",
"-i  --ignore-sleeptime  ignore sleep time (between requests) in llt trace.",
"-q  --queue-depth       Sets the queue depth (max # of oustanding requests).",
"-m  --model-name        String used as a base for created files.",          
"-h  --help              Output this help.",                                 

0
};

#define OPTIONS_STRING   "hrm:lq:c:btz:in:s"

static char *program_name;

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
usage(FILE *fp,char *program_name) 
{
   fprintf(fp,"Usage: %s [ -s | -b | -t [-i] | -c<lambda> [-n <reqs>] [-z<maxLBN>]] [-q<num>] -m <model_name> <disk>\n",
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

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
int
main(int argc, char * argv[]) {

  int use_synthtrace   = 0;
  int use_lbn_requests = 0;
  int use_llt          = 0;
  int use_lambda       = 0;
  int use_maxlbn       = 0;
  char c;
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
     case 's':
       use_synthtrace = 1;
       trace_source = SYNTH_DISKSIM_TRACE;
       break;
     case 'b':
       use_lbn_requests = 1;
       trace_source = LBN_FILE_TRACE;
       break;
     case 'n':
       use_finite = atoi(optarg);
       break;
     case 'c':
       lambda = (double) atoi(optarg);
       use_lambda = 1;
       trace_source = GENERATED_TRACE;
       maxqdepth =  1;
       break;
     case 'i':
       ignore_sleep_time = 1;
       break;
     case 'q':
       {
	 char *endptr;
	 maxqdepth = (int) strtol(optarg, &endptr, 10);
       }
       break;
     case 't':
       use_llt = 1;
       trace_source = LLT_TRACE;
       break;
     case 'z':
       use_maxlbn = 1;
       maxlbn = atoi(optarg);
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

   extractlayout = 0;
   gen_complete_mappings = 0;
   run_tests = 0;
   use_layout_file = 1;

   /* 
    * start of common options check
    */
   DX_COMMON_OPTS_CHK;
   /* 
    * end of common options check
    */

   if (!use_lambda && use_maxlbn) {
     error_handler("The option -z can be used only with option -c!\n");
   }
   if (use_lbn_requests && use_lambda) {
     // if (use_lbn_requests && use_synthtrace) {
     error_handler("The options -b and -s are mutually exclusive!\n");
   }
   if (ignore_sleep_time && !use_llt) {
     error_handler("The option -i can be used only with option -t!\n");
   }
   if (!use_synthtrace && !use_lbn_requests && !use_lambda && !use_llt) {
     use_lbn_requests = 1;
     trace_source = LBN_FILE_TRACE;
   }

   if (maxqdepth > MAX_OUTSTANDING) {
     maxqdepth = MAX_OUTSTANDING;
     printf("Limiting max queue depth to %d\n",maxqdepth);
   }

   if (use_llt && (use_lbn_requests || use_lambda)) {
     error_handler("Cannot use -t option with other options!\n");
   }

   if (!(free_list_head = (request_t *)malloc((MAXFLIST)*sizeof(request_t)))){
     fprintf(stderr,"Couldn't allocate memory for request!\n");
     exit(-1);
   }
   memset(free_list_head,0,(MAXFLIST)*sizeof(request_t));
   /* link up the free list */
   {
     int i;
     request_t *temp = free_list_head;
     request_t *temp2;
     for(i=0;i<(MAXFLIST-1);i++) {
       temp2 = temp;
       temp++;
       temp2->next = temp;
     }
     free_list_tail = temp;
     temp->next = NULL;
   }
   srand((unsigned int) getpid());

   disk_init(argv[optind],use_scsi_device);

   io_issued = st_cond_new();
   count_ios = st_cond_new();
   event_signal = st_cond_new();
   
   // count_mutex = st_mutex_new();
   queue_mutex = st_mutex_new();
   free_list_mutex = st_mutex_new();
   lastreq_mutex = st_mutex_new();
   
   lbnreader = st_thread_create((void *)read_trace,(char *)model_name,1,0);
   

#ifdef CATCH_SIGNAL
  if (signal(SIGINT,sig_handler) == SIG_ERR) 
    fprintf(stderr,"Problems catching signal!\n");
  if (signal(SIGTERM,sig_handler) == SIG_ERR) 
    fprintf(stderr,"Problems catching signal!\n");
#endif

#ifdef MEASURE_TIME
  elapsed_time();
#endif
  
  issuer = st_thread_create((void *)issue_request,NULL,1,0);
  collector = st_thread_create((void *)collect_request,NULL,1,0);
  
  
  st_thread_join(issuer,NULL);
  st_thread_join(collector,NULL);
  st_thread_join(lbnreader,NULL);

#ifdef MEASURE_TIME
  printf("Trace total replay time: %.3f seconds\n",elapsed_time());
#endif

  if (trace_source == SYNTH_DISKSIM_TRACE) 
    fclose(outfp);

  disk_shutdown(use_scsi_device);
  exit (0);

}


/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
add_free_list(request_t * request)
{
  memset(request,0,sizeof(request_t));
  st_mutex_lock(free_list_mutex);
  if (free_list_head == NULL) {
    free_list_head = request;
    free_list_tail = request;
  }
  else {
    free_list_tail->next = request;
    free_list_tail = request;
  }
  st_mutex_unlock(free_list_mutex);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static request_t *
remove_free_list(void)
{
  request_t *request;
  st_mutex_lock(free_list_mutex);
  if (free_list_head == NULL) {
    fprintf(stdout,"No items on the free list!\n");
    exit(-1);
  }
  else {
    request = free_list_head;
    if (free_list_head == free_list_tail)
      free_list_head = free_list_tail = NULL;
    else 
      free_list_head = request->next;
  }
  st_mutex_unlock(free_list_mutex);
  request-> next = NULL;
  // printf("   called remove_free_list: %d\n",--flitems);
  return(request);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
#ifdef CATCH_SIGNAL
static void
sig_handler(int signo) {
  //  if ((signo == SIGINT) || ()){
    fprintf(stderr,"Caught signal, exiting!\n");
    if (trace_source == GENERATED_TRACE) {
      fprintf(stderr,"***********\nRequests: %d\n***********\n",genreqs);
      exit(0);
    }
    // st_mutex_lock(&lastreq_mutex);
    lastreq = 1;
    // st_mutex_unlock(&lastreq_mutex);
    //  }
}
#endif
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
write_trace_entry(request_t *request) {

  static struct timeval t;
  static float   real_delay;
  static request_t lastreq;
  static int first = 1;

  // printf("Trace time:   %5ld.%06ld\n",tracetime.tv_sec,tracetime.tv_usec);
  // printf("REQUEST time: %5ld.%06ld\n",request->start.tv_sec,request->start.tv_usec);

  if (request) {
    t.tv_sec = request->start.tv_sec - tracetime.tv_sec;
    t.tv_usec = request->start.tv_usec - tracetime.tv_usec;
    
    if  (t.tv_usec < 0) {
      t.tv_usec += 1000000;
      t.tv_sec--;
    }
#ifdef PRINT_TRACE
    // if (trace_source == GENERATED_TRACE)
    if (1) {
      fprintf(stdout,"%5ld.%06ld %10d %3d %s %6ld\n",
	      (long) t.tv_sec,(long) t.tv_usec,
	      request->lbn,request->blocks,
	      ((request->flags == B_READ)?"0":"1"),
	      request->compl_time);
    }
#endif
  }
  if (trace_source == SYNTH_DISKSIM_TRACE) { /* write validate trace entry */
    if (request)
      real_delay =  
	(request->start.tv_sec * 1000000 + request->start.tv_usec) -
	(lastreq.start.tv_sec * 1000000 + lastreq.start.tv_usec +
	 lastreq.compl_time);
    else 
      real_delay = 0;
     if (!first) {
       // fprintf(stdout,"%s Hit %10d %3d %12.5f %12.5f\n",
       fprintf(outfp,"%s Hit %10d %3d %12.5f %12.5f\n",
	       ((lastreq.flags == B_READ)?"R":"W"),
	       lastreq.lbn,
	       lastreq.blocks,
	       (float) lastreq.compl_time,
	       (float) real_delay);
     }
     if (request)
       lastreq = *request;
     first = 0;
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
record_compl_time(int tag,long ustop_sec, long ustop) 
{
  request_t *q;
  // printf("Searching tag %d: ",tag);
  st_mutex_lock(queue_mutex);
  q = disk_queue_head;
  /* locate the element in the queue */
  while(q) {
    if (q->tag == tag) {
      /* found the entry, write completion time */
      q->compl_time = (ustop +(ustop_sec - q->start.tv_sec)*1000000) 
	- q->start.tv_usec;
      break;
    }
    else {
      q = q->next; 
    }
  }
  /* see if we can retire rquests from the queue */
  if (q == NULL) 
    printf("The tag %d wasn't found!\n",tag);

  while ((q) && (q == disk_queue_head) && (q->compl_time != NOT_COMPLETED)) {
    // (q->compl_time != NOT_COMPLETED))
    request_t *temp;
    /* q is the oldest issued outstanding request,
     * retire every request that has completed 
     */
    write_trace_entry(q);
    disk_queue_head = q->next;
    temp = q;
    /* discard the element from the queue */
    if (q == disk_queue_tail) {
      disk_queue_tail = NULL; 
    }
    q = q->next;
    /* put it back onto the free list */
    add_free_list(temp);
  }

  // printf("Disk queue head: %p\n",disk_queue_head);
  // printf("Disk queue tail: %p\n",disk_queue_tail);
  st_mutex_unlock(queue_mutex);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void 
add_queue_request(request_t * request)
{
  st_mutex_lock(queue_mutex);
  if (disk_queue_head == NULL) {
    disk_queue_head = request;
    disk_queue_tail = request;
  }
  else {
    disk_queue_tail->next = request;
    disk_queue_tail = request;
  }
  // printf("after add: Disk queue head: %p\n",disk_queue_head);
  // printf("after add: Disk queue tail: %p\n",disk_queue_tail);
  st_mutex_unlock(queue_mutex);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static request_t *
get_event_queue_request(void)
{
  request_t *q;

  if (events == 0) {
    // printf("get_event_queue: Waiting for new request...\n");
    st_cond_wait(event_signal);
    // printf("get_event_queue wait done\n");
  }
  q  = event_queue_head;
  assert(q);
  event_queue_head = q->next;
  if (!event_queue_head)
    event_queue_tail = NULL;

  events--;
  if (events == 0) {
    st_cond_signal(event_signal);
  }
  // printf("Returning request: %p\n",q);
  // printf("event_Queue_head: %p\n",event_queue_head);
  // printf("event_Queue_tail: %p\n",event_queue_tail); 

  /* dissasociate it from the queue */
  q->next = NULL;

  return(q);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static request_t * 
new_event_request(void)
{
  request_t *request;

  request = remove_free_list();

  if (event_queue_head == NULL) {
    event_queue_head = request;
    event_queue_tail = request;
  }
  else {
    event_queue_tail->next = request;
    event_queue_tail = request;
  }
  return(request);
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
void *
read_trace(char *model_name)
{
  int i,howmany;
  FILE *infp = NULL;
  char fname[280] = "";
  char traceline[280];
  int cont_loop = 1;
  request_t *request;

  static float last_arrival = 0;

  i=0;
  switch(trace_source) {
  case LBN_FILE_TRACE:
    sprintf (fname, "%s.%s", model_name,LBN_FILE_EXT);
    if (!(infp = fopen (fname, "r")))
      error_handler("Error opening %s!\n",fname);
#ifndef SILENT
    printf("Using %s file!\n",fname);
#endif
    fscanf(infp,"Number of Requests: %d",&howmany);
    break;
  case LLT_TRACE:
    sprintf (fname, "%s.%s", model_name, LLTRACE_EXT);
    if (!(infp = fopen (fname, "r"))) {
      error_handler("Could not open %s file!\n",fname);
    }
#ifndef SILENT
    printf("Using %s as LLT trace file!\n",fname);
#endif
    
    howmany = 0;
    while (fgets(traceline,200,infp)) {
      howmany++;
    }
    rewind(infp);
    break;
  case SYNTH_DISKSIM_TRACE:
    sprintf (fname, "%s.%s", model_name,SYNTHTRACE_EXT);
    if (!(infp = fopen (fname, "r"))) {
      error_handler("Could not open %s file!\n",fname);
    }
#ifndef SILENT
    printf("Using %s synthtrace file!\n",fname);
#endif
    sprintf (fname, "%s.%s", model_name,TRACES_EXT);
    if (!(outfp = fopen (fname, "w"))) {
      error_handler("Could not open %s output file!\n",fname);
    }
    howmany = 0;
    while (fgets(traceline,200,infp)) {
      howmany++;
    }
    rewind(infp);
    break;
  case GENERATED_TRACE:
    maxqdepth = 1;

    break;
  default:
    internal_error("Unknown trace type: %d\n",trace_source);
  }

  { /* mark the begining time of the trace */
    struct timezone tz;
    gettimeofday(&tracetime,&tz);
  }

  while(cont_loop) {
    /* this is here to catch a signal SIGINT */
    st_mutex_lock(lastreq_mutex);
    if (lastreq)
      cont_loop = 0;
    st_mutex_unlock(lastreq_mutex);
    
    if (events == MAXEVENTS) {
      // printf("Read Waiting for event signal in read_trace..\n");
      st_cond_wait(event_signal);
      // printf("\nRead wait done\n");
    }
    events++;
    request = new_event_request();
    switch(trace_source) {
    case LBN_FILE_TRACE:
      // printf("Reading an entry\n");
      if (fscanf(infp,"%d,%d\n",&request->lbn,&request->blocks) != 2)
	error_handler("Error reading line %d of %s!\n",i+1,fname);
      //  if ((request->lbn  > (thedisk->numblocks - 1)) || (request->lbn < 0))
      //  error_handler("Invalid LBN on line %d of %s !\n",i+1,fname);

      request->flags = B_READ;
      request->sleep_time = 0;
      if (++i>=howmany) {
	cont_loop =0;
      }
      break;
    case LLT_TRACE:
      {
	int rc;
	static long last_sleep = 0;
	float dummy2,arrival; /* arrival is seconds */
	int thinktime,dummy,dummy3;
	if ((rc = fscanf(infp,"%f %d %d %d %d %f %u %d\n",&arrival,&dummy,
		    &request->lbn,&request->blocks, &request->flags,
		   &dummy2,&dummy3,&thinktime)) != 8){
	  error_handler("LL_TRACE: Error reading line %d of %s!\n",i+1,fname);
	}

	// request->timestamp.tv_usec = (long) (arrival * 1000000);  
	// request->timestamp.tv_sec = ((long) arrival);

	/* request->flags = request->flags & 0x1; */
	request->flags = ((request->flags & 0x1) ? B_WRITE : B_READ);
	if (ignore_sleep_time) {
	  request->sleep_time = 0;	  
	}
	else {
	  request->sleep_time = last_sleep;
	}
	last_sleep = (long) thinktime;
      }
      if (++i>=howmany) {
	cont_loop = 0;
      }
      break;
    case GENERATED_TRACE:
      request->lbn = rand() % maxlbn; 
      // request->blocks = rand() % 10; 
      request->blocks = 16; 
      request->flags   = B_READ;    
      request->sleep_time = (long) 1000000 * get_poisson_arrival(lambda);
      ++i;
      genreqs++;
      if (genreqs==use_finite) {
	exit(0);
      }
      break;
    case SYNTH_DISKSIM_TRACE:
      {
	float arrival; /* arrival is in miliseconds */
	int dummy;
	
	if (fscanf(infp,"%f %d %d %d %d\n",&arrival,&dummy,
		   &request->lbn,&request->blocks, &request->flags) != 5)
	  internal_error("Error reading line %d of %s!\n",i+1,fname);
	// if ((request->lbn > (thedisk->numblocks - 1)) || (request->lbn < 0))
	// internal_error("Invalid LBN on line %d of %s !\n",i+1,fname);
	
	/* request->flags = request->flags & 0x1; */
	request->flags = ((request->flags & 0x1) ? B_READ : B_WRITE);

	if (i == 0) {
	  /* mark the begining time of the trace */
	  struct timezone tz;
	  gettimeofday(&tracetime,&tz);

	  begintime.tv_sec = ((long) arrival) / 1000;
	  begintime.tv_usec = (long) (arrival * 1000);
	}
	
	request->sleep_time = (long) (1000 * (arrival - last_arrival));
	// printf("Trace sleep time %ld\n",request->sleep_time);
	last_arrival = arrival;
	if (++i>=howmany) {
	  cont_loop = 0;
	}
      }
      break;
    }
    request->tag = i;
    request->compl_time = NOT_COMPLETED;
    
    st_cond_signal(event_signal);

  } /* end of while(cond) loop */
  fclose(infp);
  st_mutex_lock(lastreq_mutex);
    // printf("Setting lastread to in read_trace\n");
  lastreq = 1;
  st_mutex_unlock(lastreq_mutex);
  st_thread_exit(NULL);
  return(NULL);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void *
issue_request(void) 
{
  request_t *request;
  extern long ustart_sec,ustart;
  int first = 1;

  while(1) {
    request = get_event_queue_request();

    if ((!first) && (request->sleep_time > 0)) {
      // printf("The sleeptime is %ld\n",request->sleep_time);
      st_usleep(request->sleep_time);
    }
    // st_mutex_lock(count_mutex);
    /* more requests are available */
    if (count == maxqdepth) {
      // printf("issue_request: waiting for count_ios\n");
      st_cond_wait(count_ios);
      // printf("issue_request: Done waiting.\n");
    }
    count++;
    add_queue_request(request);

    // printf("Issued request #%d to LBN %d\n",request->tag,request->lbn);
    send_scsi_command(scsi_buf,SCSI_rw_command(request->flags,request->lbn,
					       request->blocks),
		      request->tag);
    // assert(request->tag != 0);
    if (request->tag == 0) {
      printf("Got a ZERO request!\n");
      printf("Disk queue head: %p\n",disk_queue_head);
      printf("Disk queue tail: %p\n",disk_queue_tail);
      printf("Count = %d\n",count);
    }
    /* record start time */
    request->start.tv_usec = ustart;
    request->start.tv_sec = ustart_sec;

    // st_mutex_unlock(count_mutex);

    st_mutex_lock(lastreq_mutex);
    if ((lastreq) && (events == 0)) {
      // st_mutex_lock(lastissued_mutex);
      lastissued = 1;
      // st_mutex_unlock(lastissued_mutex);

      st_mutex_unlock(lastreq_mutex);
      st_thread_exit(NULL);
    }
    st_mutex_unlock(lastreq_mutex);
    first = 0;
    st_cond_signal(io_issued);
  }
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void *
collect_request(void) 
{
  int retid;
  extern long ustop,ustop_sec;
  while(1) {
    // st_mutex_lock(count_mutex);
    /* record reqid and issue time */
    // if (count > 0) {
      // printf("Waiting for a request to be issued!\n");
      // st_cond_wait(io_issued);
      // printf("Waiting for a request to finish!\n");
      retid = recv_scsi_command(scsi_buf);
      // printf("Received request #%d\n",retid);
      
      record_compl_time(retid,ustop_sec,ustop);
      count--;
      if (count < maxqdepth) {
	// st_mutex_lock(&lastissued_mutex);
	if ((count == 0) && (lastissued)) {
	  /* flush out the last request */
	  if (trace_source == SYNTH_DISKSIM_TRACE)
	    write_trace_entry(NULL);

	  // st_mutex_unlock(count_mutex);
	  // st_mutex_unlock(&lastissued_mutex);
	  st_thread_exit(NULL);
	}
	else {
	  // printf("Signaling count_ios: %d maxqdepth %d\n",count,maxqdepth);
	  st_cond_signal(count_ios);
	}
	// st_mutex_unlock(&lastissued_mutex);
      }
    }
    // st_mutex_unlock(count_mutex);
    // printf("yield");
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
void
run_single_outstanding(request_t *request) 
{
  extern long ustart_sec,ustart;
  extern long ustop,ustop_sec;
  int retid;

  if (request->sleep_time > 0) {
    // printf("The sleeptime is %ld\n",request->sleep_time);
    busy_wait_loop(request->sleep_time);
  }
  // printf("Issued request #%d to LBN %d\n",request->tag,request->lbn);
  send_scsi_command(scsi_buf,SCSI_rw_command(request->flags,request->lbn,
					     request->blocks),
		    request->tag);
  request->start.tv_usec = ustart;
  request->start.tv_sec = ustart_sec;

  retid = recv_scsi_command(scsi_buf);
  // printf("Received request #%d\n",retid);

  request->compl_time =	(ustop + (ustop_sec - request->start.tv_sec)*1000000) 
	- request->start.tv_usec;
  write_trace_entry(request);
}

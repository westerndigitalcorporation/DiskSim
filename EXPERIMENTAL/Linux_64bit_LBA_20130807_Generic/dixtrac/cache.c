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
#include "extract_params.h"
#include "mtbrc.h"
#include "cache.h"
#include "print_bscsi.h"
#include "build_scsi.h"
#include "binsearch.h"

extern diskinfo_t *thedisk;

// XXX
#include <diskmodel/dm.h>
extern struct dm_disk_if *adisk;


extern char *scsi_buf;

static int cache_initialized = 0;

/*---------------------------------------------------------------------------*
 * Returns 1 if the cmdtime indicates that the last request was a cache hit. *
 * Otherwise returns 0.                                                      *
 *                                                                           *
 * Implementation: Assumes that a cache hit takes less than a 1/4 of a rev.  *
 *---------------------------------------------------------------------------*/
static int 
is_cache_hit(long cmdtime)
{
  /* figure the time of one revolution - in microseconds */
  /* one_rev = 1000 * get_one_rev_time(); */

  if (cmdtime < (long) ( ONE_REVOLUTION / 4 )) {
    /* hit */
    return 1;
  }
  else {
    return 0;
  }
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the cmdtime indicates that the last request was a write      *
 * cache hit. Otherwise returns 0.                                           *
 *                                                                           *
 * Implementation: Assumes that a cache hit takes less than a 1/4 of a rev.  *
 *---------------------------------------------------------------------------*/
static int 
is_write_cache_hit(long cmdtime)
{
  /* figure the time of one revolution - in microseconds */
  /* one_rev = 1000 * get_one_rev_time(); */

  if (cmdtime < (ONE_REVOLUTION / 2)) {
    /* hit */
    return(1);
  }
  else {
    return(0);
  }
}

/*---------------------------------------------------------------------------*
 * Executes the commands as stated by the parameters and returns 1 if the    *
 * the second command hit the cache. Otherwise returns 0. 'wait' denotes     *
 * how many revolutions to wait before issuing the second command.           *
 *---------------------------------------------------------------------------*/
static int 
test_cache_generic(char cachebits, int rw1, int lbn1, int sectors1,
		   int rw2, int lbn2, int sectors2,int wait)
{
  long cmd2time;
  char origbits;
  extern long ustop,ustart;

  origbits = set_cache(cachebits);
  exec_scsi_command(scsi_buf,SCSI_rw_command(rw1,lbn1,sectors1));

  /* don't call busy wait if wait is 0 - saves some microseconds */
  if (wait)
    busy_wait_loop(wait * ONE_REVOLUTION);
  exec_scsi_command(scsi_buf,SCSI_rw_command(rw2,lbn2,sectors2));
  cmd2time = ustop - ustart;
  /* printf("test_cache_generic: Time = %ld usec\n",cmd2time); */

  /* restore the cache enable bits to the original value */
  set_cache(origbits);
  return(is_cache_hit(cmd2time));
}

/*---------------------------------------------------------------------------*
 * Based on the parameters 'condition', 'value', and 'maxvalhit', which are  *
 * returned from bin_search routine (after a completed search) it determines *
 * if the value under test is set up by an adaptive algorithm.               *
 *---------------------------------------------------------------------------*/
static int
binary_search_adaptive(int condition,int value,int maxvalhit)
{
  if (!condition || (maxvalhit > value))
    return(1);
  else
    return(0);
}

/*---------------------------------------------------------------------------*
 * Issues 'howmany' random READ requests to the disk. The requests range     *
 * from 1 to 10 sectors in size.                                             *
 *---------------------------------------------------------------------------*/
static void
pollute_cache(int howmany) 
{
#define MAX_SECTORS             10
  int i,lbn,rw,sectors;
  int *cyls = calloc(howmany, sizeof(int));

  // printf("%s(howmany=%d)\n", __func__, howmany);

/* #define PROGRESS_MSG */

  generate_random_cylinders(cyls,howmany,0);

#ifdef PROGRESS_MSG
  printf("Polluting cache (%d cylinders) ",howmany);
#endif

  for(i = 0 ; i< howmany ; i++) {
    lbn = diskinfo_cyl_firstlbn(cyls[i],0);
    sectors = 1 + rand() % MAX_SECTORS;
    rw = B_READ;
    exec_scsi_command(scsi_buf,SCSI_rw_command(rw,lbn,sectors));

#ifdef PROGRESS_MSG
    printf(".");
#endif

  }

#ifdef PROGRESS_MSG
  printf("\n");
#endif

#undef MAX_SECTORS

  // printf("%s() done\n", __func__);

  free(cyls);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if disk discards a cached block after sending it to the host.   *
 * Otherwise returns 0 - i.e. disk block stays in cache and can be read many *
 * times without going to the medium.                                        * 
 *---------------------------------------------------------------------------*/
static int
test_discard_req_after_read(void)
{
  int lbn;
  lbn = diskinfo_cyl_firstlbn(random_cylinder(1,ANY_DEFECT,ANY_BAND),0);
  return(! test_cache_generic(READ_CACHE_ENABLE,
			      B_READ,lbn,1,B_READ,lbn,1,3));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk prefetches blocks after reading one block.          *
 * Otherwise, returns 0. 'offset' denotes the space (in blocks) between the  *
 * read requests used to test the prefetch.                                  *
 *---------------------------------------------------------------------------*/
static int
test_prefetch_after_read(void)
{
  int lbn,cyl;
  int offset = 1;

  cyl = random_cylinder(0,NO_CYL_DEFECT,MAX_BAND);
  lbn = diskinfo_cyl_firstlbn(cyl,0);
  return(test_cache_generic(READ_CACHE_ENABLE,
			    B_READ,lbn,1,B_READ,lbn+offset,1,3));
}
/*---------------------------------------------------------------------------*
 * Returns 1 if a READ request to the same block that was previously written *
 * hits the cache. Otherwise returns 0.                                      *
 *---------------------------------------------------------------------------*/
static int
test_share_rw_segment(void)
{
  int lbn,cyl;

  cyl = random_cylinder(0,NO_TRACK_DEFECT,ANY_BAND);
  lbn = diskinfo_cyl_firstlbn(cyl,0);
  return(test_cache_generic(READ_CACHE_ENABLE | WRITE_CACHE_ENABLE,
			    B_WRITE,lbn,1,B_READ,lbn,1,3));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if disk caches writes, i.e. returns immediately to the host     *
 * before writing out the request to the disk. Otherwise returns 0.          *
 *---------------------------------------------------------------------------*/
static int 
test_writes_cached(void)
{
  int lbn;
  char origbits;
  long cmd2time;
  extern long ustop,ustart;

  origbits = set_cache(WRITE_CACHE_ENABLE);
  lbn = diskinfo_cyl_firstlbn(random_cylinder(1,ANY_DEFECT,ANY_BAND),0);

  exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn,1));
  exec_scsi_command(scsi_buf,SCSI_rw_command(B_WRITE,lbn,1));
  cmd2time = ustop - ustart;

  /* restore the cache enable bits to the original value */
  set_cache(origbits);
  return(is_write_cache_hit(cmd2time));
  /*
  return(test_cache_generic(WRITE_CACHE_ENABLE,
				B_READ,lbn,1,B_WRITE,lbn,1,0));
   */
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk collapses two WRITE sequential requests in the      *
 * cache into one. Otherwise returns 0.                                      *
 *---------------------------------------------------------------------------*/
static int 
segment_writes_append(int gap)
{
  int cyl,offset;
  int lbn;

  cyl = random_cylinder(0,NO_TRACK_DEFECT,ANY_BAND);
  lbn = diskinfo_cyl_firstlbn(cyl,0);
  offset = 4;
  return(test_cache_generic(WRITE_CACHE_ENABLE,
			    B_WRITE,lbn,offset,B_WRITE,lbn+offset+gap,2,0));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk collapses two WRITE sequential requests in the      *
 * cache into one. Otherwise returns 0.                                      *
 *---------------------------------------------------------------------------*/
static int 
test_contig_writes_appended(void)
{
  return(segment_writes_append(0));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk collapses two WRITE requests to the same segment    *
 * in the dache into one. Otherwise returns 0.                               *
 *---------------------------------------------------------------------------*/
static int 
test_discon_writes_appended(void)
{
  return(segment_writes_append(4));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk prefetches past track boundary. Otherwise returns 0.*
 *---------------------------------------------------------------------------*/
static int
test_track_discon_prefetch(void)
{
  int cyl,lbn1,lbn2,head1,head2,sectors;

  cyl = random_cylinder(3,NO_CYL_DEFECT,ANY_BAND);
  lbn1 = diskinfo_cyl_firstlbn(cyl,0);
  diskinfo_physical_address(lbn1,&cyl,&head1,&sectors);
  diskinfo_track_boundaries(cyl,head1, &lbn1, &lbn2);
  lbn1 = lbn2 - 3;
  /* get an LBN on the next track */
  lbn2 = lbn2 + 1;
  diskinfo_physical_address(lbn2,&cyl,&head2,&sectors);
  assert(abs(head2 - head1) == 1);
  sectors = 1;
  return(test_cache_generic(READ_CACHE_ENABLE,B_READ,lbn1,sectors,
				 B_READ,lbn2,sectors,4));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk prefetches past cylinder boundary. Otherwise        *
 * returns 0.                                                                *
 *---------------------------------------------------------------------------*/
static int
test_cylinder_discon_prefetch(void)
{
  int cyl,lbn1,lbn2,sectors;

  cyl = find_defectfree_cylinder_range(adisk,
                           random_cylinder(10,ANY_DEFECT,ANY_BAND),2);

  lbn2 = diskinfo_cyl_firstlbn(cyl+1,0);
  /* get an LBN on the previous cylinder */
  lbn1 = lbn2 - 3;
  sectors = 1;
  return(test_cache_generic(READ_CACHE_ENABLE,B_READ,lbn1,sectors,
				 B_READ,lbn2,sectors,4));
}

/*---------------------------------------------------------------------------*
 * Determines, if the disk prefetches sectors logically prior to the         *
 * requested LBN.                                                            *
 *---------------------------------------------------------------------------*/
static int
test_read_free_blocks(void)
{
  int cyl1,cyl2,lbn1,lbn2,head1,head2,dummy;
  extern long ustop,ustart;
  long cmd2time;
  char origbits;

  origbits = set_cache(READ_CACHE_ENABLE);

  cyl1 = find_defectfree_cylinder_range(adisk,
                             random_cylinder(10,ANY_DEFECT,ANY_BAND),2);
  cyl2 = cyl1 + 1;
  lbn2 = diskinfo_cyl_firstlbn(cyl2,0);
  /* get an LBN on the previous cylinder */
  lbn1 = lbn2 - 3;

  diskinfo_physical_address(lbn2,&cyl2,&head2,&dummy);
  // not true for some new disks!
  //  assert(cyl2 == (cyl1 + 1));

  diskinfo_physical_address(lbn1,&cyl1,&head1,&dummy);
  //  assert(cyl2 == (cyl1 + 1));

  exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn1,1));
  exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn2,1));
  exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn2-1,1));
  cmd2time = ustop - ustart;

  /* restore the cache enable bits to the original value */
  set_cache(origbits);

  return(is_cache_hit(cmd2time));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk uses a track prefetch algorithm, otherwise returns  *
 * 0. The prefetch algorithm recognizes if two previous requests were to     *
 * track n and n-1, respectively, if so, it prefetches track n+1 to speed up *
 * sequential reads.                                                         *
 *---------------------------------------------------------------------------*/
static int
test_prefetch_track_algorithm(void)
{
  int cyl,lbn[3],head[3],sector,sectors,i;
  long cmdtime = 0;
  extern long ustop,ustart;

  cyl = random_cylinder(3,NO_CYL_DEFECT,ANY_BAND);
  lbn[0] = diskinfo_cyl_firstlbn(cyl,0);
  diskinfo_physical_address(lbn[0],&cyl,&head[0],&sector);
  diskinfo_track_boundaries(cyl,head[0], &lbn[0], &lbn[1]);
  /* 
  disk_get_lbn_boundaries_for_track(thedisk,diskinfo_get_band(cyl),cyl,head[0],
				    &lbn[0],&lbn[1]);
  */
  lbn[1]++;
  sectors = diskinfo_sect_per_track(cyl); /* lbn[1] - lbn[0]; */
  diskinfo_physical_address(lbn[1],&cyl,&head[1],&sector);
  diskinfo_track_boundaries(cyl,head[1], &lbn[1], &lbn[2]);
  /*
  disk_get_lbn_boundaries_for_track(thedisk,diskinfo_get_band(cyl),cyl,head[1],
				    &lbn[1],&lbn[2]);
  */
  lbn[2]++;
  diskinfo_physical_address(lbn[2],&cyl,&head[2],&sector);
  /*  assert(abs(head[0] - head[1]) == 1); */
  /*  assert(abs(head[1] - head[2]) == 1); */

  for( i = 0; i< 2; i++ ) {
    if (i == 0) {
      /* read track n-1 */
      exec_scsi_command(scsi_buf,
			SCSI_rw_command(B_READ,lbn[0],sectors));
    }
    else {
      /* read tracks n and see if n+1 is prefetched */
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn[1],1));
      busy_wait_loop(3 * ONE_REVOLUTION);
      exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn[2],1));
      cmdtime = ustop - ustart;
    }
  }
  return(is_cache_hit(cmdtime));
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the drive implements 'rw' request on arrival (known also as  *
 * zero latency, otherwise returns 0. A framework function for testing read- *
 * on-arrival and write-on-arrival.                                          *
 *---------------------------------------------------------------------------*/
static int
test_on_arrival(int rw)
{
  int lbn1,lbn2,cyl,sectors;
  long time,seektime,condition;
  int i;
  long tset[1][N_SAMPLES];
  float max,mean;

  char origbits;
  extern long ustop,ustart;

  condition = 1.1 * ONE_REVOLUTION;

  origbits = set_cache(READ_CACHE_ENABLE | WRITE_CACHE_ENABLE);

  for(i=0;i<N_SAMPLES;i++) {
    cyl = random_cylinder(1,NO_CYL_DEFECT,ANY_BAND);
    lbn1 = diskinfo_cyl_firstlbn(cyl,0);
  
    cyl = random_cylinder(1,NO_CYL_DEFECT,ANY_BAND);
    lbn2 = diskinfo_cyl_firstlbn(cyl,0);
    sectors = diskinfo_sect_per_track(cyl);
    
    exec_scsi_command(scsi_buf,SCSI_rw_command(rw,lbn1,1));
    exec_scsi_command(scsi_buf,SCSI_rw_command(rw,lbn2,sectors));

    time = ustop - ustart;
#if 0
    {
      int head1;
      diskinfo_physical_address(lbn1,&cyl,&head1,&sectors);
      printf("address: LBN %d = [%d,%d,%d]\n",lbn1,cyl,head1,sectors);
      diskinfo_physical_address(lbn2,&cyl,&head1,&sectors);
      printf("address: LBN %d = [%d,%d,%d]\n",lbn2,cyl,head1,sectors);
    }
#endif
    exec_scsi_command(scsi_buf,SCSI_seek_command(lbn1)); 
    seektime = ustop - ustart;

    tset[0][i] = time -= seektime;
    /*  printf("Time: %ld\t\t%.4f\n",time,(float) time/ONE_REVOLUTION); */
  }
  /* restore the cache enable bits to the original value */
  set_cache(origbits);

  find_mean_max(1,N_SAMPLES,&tset[0][0],&max,&mean,0);
  /*  printf("Reading one track (revolutions): %f\n",mean/ONE_REVOLUTION); */

  if (mean > condition)
    return(0);
  else 
    return(1);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the drive implements write-on-arrival (also known as zero    *
 * latency write). Otherwise returns 0.                                      *
 *---------------------------------------------------------------------------*/
static int
test_write_on_arrival(void)
{
  return(test_on_arrival(B_WRITE));
}


/*---------------------------------------------------------------------------*
 * Returns 1 if the drive implements read-on-arrival (also known as zero     *
 * latency read). Otherwise returns 0.                                       *
 *---------------------------------------------------------------------------*/
static int
test_read_on_arrival(void)
{
  return(test_on_arrival(B_READ));
}

/*---------------------------------------------------------------------------*
 * Returns the number of segments that are in the disk cache. 'numwrites'    *
 * denotes the number of writes that are performed during one test vector.   *
 * To dermine the number of read segments, set 'numwrites' = 0.              *
 *---------------------------------------------------------------------------*/
static int
test_number_of_segments(int numwrites)
{
  int high,low,maxhit,numsegs,modepageval;
  int lasthigh = -1;
  int lbn = 0;
  int i,rw,sectors;
  int written,discard_read,prefetch;
  struct  {
    int lbn[MAX_SEGMENTS],cyl[MAX_SEGMENTS];
  } blocks;
  extern long ustop,ustart; // ustart_sec,ustop_sec;

  discard_read = test_discard_req_after_read();
  prefetch = test_prefetch_after_read();

  /* Seed the initial number of segments with the value from the cache
   * mode page.
   */
  // numsegs = MAX_SEGMENTS / 4; // - Used to do this
  exec_scsi_command(scsi_buf,SCSI_mode_sense(CACHE_PAGE));

#define NUMSEGS_OFFSET ((u_int8_t) scsi_buf[3] + 4 + 13)
  modepageval = (int) (scsi_buf[NUMSEGS_OFFSET]);
  numsegs = modepageval * 2;
#undef NUMSEGS_OFFSET

  generate_random_cylinders(blocks.cyl, MAX_SEGMENTS, NO_TRACK_DEFECT);
  for(i = 0 ; i < MAX_SEGMENTS; i++) {
    blocks.lbn[i] = diskinfo_cyl_firstlbn(blocks.cyl[i],0);
  }

  maxhit = low = 0;
  high = numsegs;
  do {
    assert(numsegs < MAX_SEGMENTS);
    written = (numsegs < numwrites ? numsegs-2 : numwrites);

    /* 1. fill in the cache */
    for(i = 0 ; i < numsegs ; i++) {
      rw = B_READ;

      if (i > 0 && written > 0) {
	rw = B_WRITE;
	written--;
      }

      sectors = 1;

      /* printf("Do %s\n", (rw == B_READ ? "READ" : "WRITE")); */

      exec_scsi_command(scsi_buf,
			SCSI_rw_command(rw,blocks.lbn[i],sectors));
    }

    /* figure out the right lbn to read the data from the cache */
    if (discard_read) {
      if (prefetch) {
	lbn = blocks.lbn[0]+1;
      }
      else {
	internal_error("test_number_of_segments: Unknown cache!\n");
      }
    }
    else {
      lbn = blocks.lbn[0];
    }

    rw = B_READ;
    sectors = 1;

    /* now read the data from the first segment */
    exec_scsi_command(scsi_buf, SCSI_rw_command(rw,lbn,sectors));
#if 0
    printf("%s() ustop (%ld,%ld) ustart (%ld,%ld) time %ld\n", __func__,
	   ustop_sec, ustop, ustart_sec, ustart, ustop-ustart);
    
    printf("Time to do the next %s op: %ld [%d]\n",
          (rw == B_READ ? "READ" : "WRITE"), (ustop - ustart), lbn);

    printf("[%d,%d] last high: %d\n", low, high, lasthigh);
#endif

  } while(bin_search(is_cache_hit(ustop-ustart), 
		     &numsegs, 
		     &low, 
		     &high,
		     &lasthigh, 
		     &maxhit));

  printf("%s(): determined %d cache segments, ", __func__, numsegs);
  printf("mode page claims %d\n", modepageval);

  if (numsegs > modepageval) {
      printf("Mismatch in the number of cache segments.\n");
      printf("%s(): Probably adaptive segment caching algorithm.\n",__func__);
      numsegs = modepageval;
      printf("Setting value to %d.\n", numsegs);
  }
  return numsegs;
}

/*---------------------------------------------------------------------------*
 * Sets the # of segments to MAX value and sets cache bits appropriately and *
 * returns the number of segments, given the 'numwrites' WRITE requests.     *
 * A "set to max" wrapper to test_number_of_segments.                        *
 *---------------------------------------------------------------------------*/
static int
count_read_segments(int numwrites) 
{
  int retval,discard_read;
  int orignumsegs,numsegs = MAX_SEGMENTS;
  u_int8_t ic_origval,dra_origval = 0;
  int ic_savable,dra_savable = 0;
  char origbits;
  
  printf("%s()\n", __func__);

  discard_read = test_discard_req_after_read();

  if (discard_read) {
    dra_origval = get_cache_bit(CACHE_DRA);
    dra_savable = set_cache_bit(CACHE_DRA,0);
  }
  ic_origval = get_cache_bit(CACHE_IC);
  ic_savable = set_cache_bit(CACHE_IC,1);
  /* save original value */
  orignumsegs = get_cache_numsegs();
  origbits = set_cache(READ_CACHE_ENABLE | WRITE_CACHE_ENABLE);
  
  set_cache_numsegs(numsegs);
  retval = test_number_of_segments(numwrites);
  
  /* restore the cache enable bits to the original value */
  set_cache(origbits);
  /* restore original value for the number of segments in Cache Mode Page */
  set_cache_numsegs(orignumsegs);
  /*restore bit back */
  if (ic_savable)
    set_cache_bit(CACHE_IC,ic_origval);
  if (discard_read)
    if (dra_savable)
      set_cache_bit(CACHE_DRA,dra_origval);
  
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
static int
test_count_write_segments(void)
{
  int retval = 0;
  int i,rw,sectors,writes;
  struct  {
    int lbn[MAX_SEGMENTS],cyl[MAX_SEGMENTS];
  } blocks;
  extern long ustop,ustart;
  char origbits;

  // printf("%s()\n", __func__);

  origbits = set_cache(WRITE_CACHE_ENABLE);

  if (test_writes_cached()) {
    if (test_share_rw_segment()) {
      writes = 1;
      do {
	generate_random_cylinders(blocks.cyl,writes,NO_TRACK_DEFECT);
	for(i = 0 ; i< writes ; i++)
	  blocks.lbn[i] = diskinfo_cyl_firstlbn(blocks.cyl[i],0);
	
	for( i=0 ; i< writes; i++) {
	  rw = B_WRITE;
	  sectors = 1;
	  /* printf("Do %s\n", (rw == B_READ ? "READ" : "WRITE")); */
	  exec_scsi_command(scsi_buf,
			    SCSI_rw_command(rw,blocks.lbn[i],1));
	}
	if (is_write_cache_hit(ustop-ustart))
	  writes++;
	else 
	  break;
      } while(1);
      retval = writes - 1;
    }
    else 
      retval = 1;
  }
  /* restore the cache enable bits to the original value */
  set_cache(origbits);

  return(retval);
}  

/*---------------------------------------------------------------------------*
 * Fills one cache segment with data by issuing a READ request.              *
 * If 'discard_read' is 1 issues a read request for one block with LBN 'lbn' *
 * and hopes for prefetching ('offset' is in this case ignored). If          *
 * discard_read is 0, issues READ of 'offset' blocks at 'lbn' command .      *
 *---------------------------------------------------------------------------*/
static void
fill_one_segment(int lbn,int discard_read,int offset)
{
  int rv;
  int nsects;
  if (discard_read) {
    nsects = 1;
  }
  else {
    nsects = offset;
  }

  /* fill in the cache */
  assert(lbn + nsects < adisk->dm_sectors);
  rv = exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ, lbn, nsects));
  /* printf("\tFilling cache with %d sectors\n",nsects); */

  if(rv) {
    printf("%s() rv = %d, lbn %d\n", __func__, rv, lbn);
  }
}

/*---------------------------------------------------------------------------*
 * Probes the content of one cache segment and returns the completion times  *
 * of the two READ requests. If 'discard_read' is 1, it only waits long      *
 * enough for prefetching, otherwise it issues a one block read to probe the *
 * begining of the segment ('lbn'). Then, it issues a READ request to LBN    *
 * 'lbn'+'offset'-1 to probe the end of the segment.                         *
 *---------------------------------------------------------------------------*/
static void
test_one_segment(int lbn,int offset,int discard_read,long *cmd1time,
		 long *cmd2time)
{
  int lbn1,lbn2,nsects;
  extern long ustop,ustart;

  lbn1 = lbn;
  /* check first block in the cache */
  if (discard_read) {
    /* wait long enough to allow prefetching */
    busy_wait_loop(5 * ONE_REVOLUTION);
    *cmd1time = 1;
  }
  else {
    nsects = 1;
    exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn1,nsects));
    *cmd1time = ustop - ustart;
    /* printf("\tLBN1(check %d sectors)\t%5ld usec\n",nsects,*cmd1time); */
  }
  lbn2 = lbn+offset-1;
  nsects = 1;
  /* check last block in the cache */
  exec_scsi_command(scsi_buf,SCSI_rw_command(B_READ,lbn2,nsects));
  *cmd2time = ustop - ustart;
  /* printf("\tLBN2 (check: %d sectors)\t%5ld usec\n",nsects,*cmd2time); */
}


/*---------------------------------------------------------------------------*
 * Returns the size of a segment in blocks assuming there are 'numsegs'      *
 * distinct read segments.                                                   *
 *---------------------------------------------------------------------------*/
static int 
test_segment_size(int numsegs)
{
  // #define DEBUG_SEG_SIZE 
  int i,condition,hits,low,high,maxhit,lasthigh = -1;
  int cyl,sectors,offset,discard_read;
  int lbn[MAX_SEGMENTS];
  long cmd1time,cmd2time;

  // printf("%s()\n", __func__);

  if (numsegs < 0) {
    internal_error("test_segment_size: %d is not valid numsegs arguement!\n",
		   numsegs);
  }

  if (numsegs == 0) {
    return(0);
  }

#ifdef DEBUG_SEG_SIZE
  printf("Using %d segments\n",numsegs);
#endif

  // printf("(%s() (%s:%d)) :"  __func__,__FILE__, __LINE__);
#ifndef SILENT
  printf("Determining segment size...");
  fflush(stdout);
#endif

  discard_read = test_discard_req_after_read() 
    || test_prefetch_after_read();

  sectors = diskinfo_sect_per_track(random_cylinder(0,NO_CYL_DEFECT,MAX_BAND));
  offset = sectors;
  maxhit = low = 2;
  high = offset;

  do {
    assert(offset < 10*sectors);
    // Choose an arbitrary value - if the targetted offset is larger than 
    // that assume we have an adaptive behavior
    if ((offset > 4 * sectors) || (offset > 10000)) {
	goto adaptive;
    }
    
    pollute_cache(numsegs*1.5); 

#ifdef DEBUG_SEG_SIZE
    printf("[%d,%d] %d\n",low,high,lasthigh);
#endif

    for( i = 0; i < numsegs; i++) {
      cyl = random_cylinder(0,NO_CYL_DEFECT,MAX_BAND);
      lbn[i] = diskinfo_cyl_firstlbn(cyl,0);
    }
    
    /* disk keeps read data in cache, fill in 'numsegs' segments with
     * READ requests for 'offset' blocks probe cache only after all
     * segments have been filled with READs */ 
    // printf("%s() fill segments...\n", __func__);
    if (!discard_read) {
      for(i = 0; i < numsegs; i++) {
	usleep(1000);
	fill_one_segment(lbn[i], discard_read, offset);
      }
    }
    // printf("%s() fill segments done\n", __func__);
#ifndef SILENT
    printf(".");
    fflush(stdout);
#endif

    condition = 0;
    hits = 0;
    for( i = 0; i < numsegs; i++) { 

      /* disk discards read data in cache after reading, issue a READ
       * request for 1 block, and wait for prefetching probe
       * immediately after filling a segment with READ and prefetch */ 
      if (discard_read) {
	fill_one_segment(lbn[i],discard_read,offset);
      }

      /* probe the content of the segment */
      test_one_segment(lbn[i],offset,discard_read,&cmd1time,&cmd2time);
      hits += (is_cache_hit(cmd1time) && is_cache_hit(cmd2time));
      
      /* overwrite condition, only if it is already 0; condition will
       * be 1 if any segment had a cache hit  */
      if (!condition) {
	condition = (is_cache_hit(cmd1time) && is_cache_hit(cmd2time));
      }

#ifdef DEBUG_SEG_SIZE
      printf("   Conditon(%8d):   %d\t\t\t\tOffset: %d\n",lbn[i],condition,offset);
#endif

    }

    /* This sets condition to 1 even though not all cache segments had
     * a cache hit. Used for Seagate CHEETAH and BARRACUDA  */
    //    if ((hits/numsegs) >= 0.5)
    //      condition = 1;


#ifdef DEBUG_SEG_SIZE
    printf("   --------------------------------\n");
    printf("   Final cond: %d\n\n",condition);
#endif

#ifndef SILENT
    printf(".");
#endif

  } while (bin_search(condition,&offset,&low,&high,&lasthigh,&maxhit));

#ifndef SILENT
  printf("\n");
#endif

  if (binary_search_adaptive(condition,offset,maxhit)) {
  adaptive:
      printf("%s(): Probably adaptive segment caching algorithm (%d).\n",
	     __func__, offset);
#define ALIGNMENT 50  /* align on 50 sector size */
      offset = ALIGNMENT * (((sectors) / ALIGNMENT)  +
			    ((sectors) % ALIGNMENT > 0 ? 1 : 0));
#undef ALIGNMENT
      printf("Setting segment size to an estimated value of %d.\n",offset);
  }

  if (discard_read)
      offset--;
  
  return(offset);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
static int
test_prefetch_size(void)
{
  int cond,cyl,lbn,sectors;
  int high,low,numsects;
  int maxhit,lasthigh = -1;

  sectors = diskinfo_sect_per_track(random_cylinder(0,NO_CYL_DEFECT,MAX_BAND));

  maxhit = low = 0;
  numsects = high = sectors;
  do {
    pollute_cache(10); 
    cyl = random_cylinder(0,NO_CYL_DEFECT,MAX_BAND);
    lbn = diskinfo_cyl_firstlbn(cyl,0);
    cond = test_cache_generic(READ_CACHE_ENABLE | WRITE_CACHE_DISABLE,
			      B_READ,lbn,1,B_READ,lbn+numsects,1,
			      ((numsects / sectors) + 3));
    /* printf("%d [%d,%d]\t\t%d\n",lasthigh,low,high,cond); */
  } while(bin_search(cond,&numsects,&low,&high,&lasthigh,&maxhit));
  if (binary_search_adaptive(cond,numsects,maxhit))
    printf("test_prefetch_size: Probably adaptive algorithm\n");
  return(numsects);
}

/*---------------------------------------------------------------------------*
 * Returns the number of blocks that are prefetched when the cache is set to *
 * to have 'numsegs' segments. The 'readahead' is either MIN_READ_AHEAD or   *
 * MAX_READ_AHEAD to determine minimal and maximal read ahead respectively.  *
 * The function is a  "set to max" wrapper to test_prefetch_size.            *
 *---------------------------------------------------------------------------*/
static int
count_prefetch_blocks(int numsegs,int readahead)
{
  int retval = -1;
  int dra_savable,disc_savable;
  u_int8_t dra_origval,disc_origval;
  int numsegs_orig;
  unsigned int min_orig,max_orig;
  unsigned int min = 0; 
  unsigned int max = 0;

  dra_origval = get_cache_bit(CACHE_DRA);
  dra_savable = set_cache_bit(CACHE_DRA,0);
  disc_origval = get_cache_bit(CACHE_DISC);
  disc_savable = set_cache_bit(CACHE_DISC,1);
  /* save original value */
  numsegs_orig = get_cache_numsegs();
  
  set_cache_numsegs(numsegs);
  
  get_cache_prefetch(&min_orig,&max_orig);
  switch (readahead) {
  case MIN_READ_AHEAD:
    min = 0;
    max = 0;
    break;
  case MAX_READ_AHEAD:
    min = 0;
    max = 0xFFFF;
    break;
  default:
    internal_error("count_prefetch_blocks: %d is unknown type of request!\n");
  }
  set_cache_prefetch(min,max);

  retval = test_prefetch_size();

  /* restore original prefetch values */
  set_cache_prefetch(min_orig,max_orig);
  /*restore bit back */
  if (dra_savable)
    set_cache_bit(CACHE_DRA,dra_origval);
  if (disc_savable)
    set_cache_bit(CACHE_DISC,disc_origval);
  set_cache_numsegs(numsegs_orig);
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
void
cache_init(void)
{

  cache_initialized = 1;
}

/*---------------------------------------------------------------------------*
 * Returns 1 if disk discards a cached block in favor of a prefetched one.   *
 * Otherwise returns 0 - i.e. disk block stays in cache and can be read many *
 * times without going to the medium.                                        * 
 *---------------------------------------------------------------------------*/
int 
cache_discard_req_after_read(void)
{
  static int retval = -1;
  int savable;
  u_int8_t origval;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_DRA);
    savable = set_cache_bit(CACHE_DRA,1);
    retval = test_discard_req_after_read();
    
    /*restore bit back */
    if (savable)
      set_cache_bit(CACHE_DRA,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk prefetches blocks after reading one block.          *
 * Otherwise, returns 0. This tests drive's default behavior, i.e. the DRA   *
 * has been set to 1 (disable read ahead).                                   *
 *---------------------------------------------------------------------------*/
int 
cache_prefetch_after_one_block_read(void)
{
  static int retval = -1;
  int savable;
  u_int8_t origval;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_DRA);
    savable = set_cache_bit(CACHE_DRA,1);

    retval = test_prefetch_after_read();

    /*restore bit back */
    if (savable)
    set_cache_bit(CACHE_DRA,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the disk prefetches blocks after reading one block with DRA  *
 * set to 0 - i.e. allowing disk to do prefetching, if it implements it.     *
 *---------------------------------------------------------------------------*/
int 
cache_allow_prefetch_after_read(void)
{
  static int retval = -1;
  int savable;
  u_int8_t origval;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_DRA);
    savable = set_cache_bit(CACHE_DRA,0);

    retval = test_prefetch_after_read();

    /*restore bit back */
    if (savable)
    set_cache_bit(CACHE_DRA,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if a READ request to the same block that was previously written *
 * hits the cache. Otherwise returns 0.                                      *
 *---------------------------------------------------------------------------*/
int
cache_share_rw_segment(void)
{
  static int retval = -1;
  
  if (retval == -1) {
    retval = test_share_rw_segment();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if disk caches writes, i.e. returns immediately to the host     *
 * before writing out the request to the disk. Otherwise returns 0.          *
 *---------------------------------------------------------------------------*/
int 
cache_writes_cached(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_writes_cached();
  }
  return(retval);
}


/*---------------------------------------------------------------------------*
 * Returns 1 if the disk collapses two WRITE sequential requests in the      *
 * in the dache into one. Otherwise returns 0.                               *
 *---------------------------------------------------------------------------*/
int 
cache_contig_writes_appended(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_contig_writes_appended();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the number of read segments.                                      *
 *---------------------------------------------------------------------------*/
int
cache_count_read_segments(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = count_read_segments(0);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cache_segment_size(void)
{
  static int retval = -1;
  int numsegs,numsegs_orig;
  char origbits;

  if (retval == -1) {
    numsegs_orig = get_cache_numsegs();
    numsegs = cache_count_read_segments();
    set_cache_numsegs(numsegs);
    origbits = set_cache(READ_CACHE_ENABLE);

    retval = test_segment_size(numsegs);

    /* restore the cache enable bits to the original value */
    set_cache(origbits);
    /* restore original value */
    set_cache_numsegs(numsegs_orig);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if the drive implements a history-based prefetch algorithm.     *
 *---------------------------------------------------------------------------*/
int
cache_prefetch_track_algorithm(void)
{
  static int retval = -1;
  int savable;
  char origbits;
  u_int8_t origval;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_DRA);
    savable = set_cache_bit(CACHE_DRA,0);
    
    origbits = set_cache(READ_CACHE_ENABLE | WRITE_CACHE_DISABLE); 
    retval = test_prefetch_track_algorithm();

    /* restore the cache enable bits to the original value */
    set_cache(origbits);

    /*restore bit back */
    if (savable)
    set_cache_bit(CACHE_DRA,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the absolute possible maximum number of blocks the drive          *
 * prefetches after doing one block read. The # of segments is set to 1.     *
 *---------------------------------------------------------------------------*/
int
cache_max_prefetch_size(void)
{
  static int retval = -1;
  if (retval == -1) {
    retval = count_prefetch_blocks(1,MAX_READ_AHEAD);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the absolute possible maximum number of blocks the drive          *
 * prefetches after doing one block read. The # of segments is set to 1.     *
 *---------------------------------------------------------------------------*/
int
cache_min_prefetch_size(void)
{
  static int retval = -1;
  if (retval == -1) {
    retval = count_prefetch_blocks(1,MIN_READ_AHEAD);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the number of blocks the drive prefetches after doing one block   *
 * read. If honors the drve's maximal number of segmens and assumes maximal  *
 * prefetch witinn that paramenter                                           *
 *---------------------------------------------------------------------------*/
int
cache_prefetch_size(void)
{
  static int retval = -1;
  if (retval == -1) {
    retval = count_prefetch_blocks(cache_count_read_segments(),MAX_READ_AHEAD);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_prefetch_track_boundary(void)
{
  static int retval = -1;
  int dra_savable,disc_savable;
  u_int8_t dra_origval,disc_origval;

  if (retval == -1) {
    dra_origval = get_cache_bit(CACHE_DRA);
    dra_savable = set_cache_bit(CACHE_DRA,0);
    /* printf("DRA savable: %d\n",dra_savable); */

    disc_origval = get_cache_bit(CACHE_DISC);
    disc_savable = set_cache_bit(CACHE_DISC,1);
    /* printf("DISC savable: %d\n",disc_savable); */

    retval = test_track_discon_prefetch();

    /*restore bit back */
    if (dra_savable)
      set_cache_bit(CACHE_DRA,dra_origval);
    if (disc_savable)
      set_cache_bit(CACHE_DISC,disc_origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_prefetch_cylinder_boundary(void)
{
  static int retval = -1;
  int dra_savable,disc_savable;
  u_int8_t dra_origval,disc_origval;

  if (retval == -1) {
    dra_origval = get_cache_bit(CACHE_DRA);
    dra_savable = set_cache_bit(CACHE_DRA,0);
    /* printf("DRA savable: %d\n",dra_savable); */

    disc_origval = get_cache_bit(CACHE_DISC);
    disc_savable = set_cache_bit(CACHE_DISC,1);
    /* printf("DISC savable: %d\n",disc_savable); */

    retval = test_cylinder_discon_prefetch();

    /*restore bit back */
    if (dra_savable)
      set_cache_bit(CACHE_DRA,dra_origval);
    if (disc_savable)
      set_cache_bit(CACHE_DISC,disc_origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Determines, if the disk prefetches sectors logically prior to the         *
 * requested LBN.                                                             *
 *---------------------------------------------------------------------------*/
int
cache_read_free_blocks(void)
{
  static int retval = -1;
  int dra_savable,disc_savable;
  u_int8_t dra_origval,disc_origval;

  if (retval == -1) {
    dra_origval = get_cache_bit(CACHE_DRA);
    dra_savable = set_cache_bit(CACHE_DRA,1);
    /* printf("DRA savable: %d\n",dra_savable); */

    disc_origval = get_cache_bit(CACHE_DISC);
    disc_savable = set_cache_bit(CACHE_DISC,0);
    /* printf("DISC savable: %d\n",disc_savable); */

    retval = test_read_free_blocks();

    /*restore bit back */
    if (dra_savable)
      set_cache_bit(CACHE_DRA,dra_origval);
    if (disc_savable)
      set_cache_bit(CACHE_DISC,disc_origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_read_on_arrival(void)
{
  static int retval = -1;
  char origbits;
  u_int8_t origval;
  int savable;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_DRA);
    savable = set_cache_bit(CACHE_DRA,1);
    /* printf("DRA savable: %d\n",savable); */

    origbits = set_cache(READ_CACHE_DISABLE | WRITE_CACHE_DISABLE); 

    retval = test_read_on_arrival();

    /* restore the cache enable bits to the original value */
    set_cache(origbits);
    /*restore DRA bit back */
    if (savable)
      set_cache_bit(CACHE_DRA,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_write_on_arrival(void)
{
  static int retval = -1;
  char origbits;
  u_int8_t origval;
  int savable;

  if (retval == -1) {
    origval = get_cache_bit(CACHE_FSW);
    savable = set_cache_bit(CACHE_FSW,0);

    origbits = set_cache(READ_CACHE_ENABLE | WRITE_CACHE_DISABLE); 

    retval = test_write_on_arrival();

    /* restore the cache enable bits to the original value */
    set_cache(origbits);
    
    /*restore bit back */
    if (savable)
      set_cache_bit(CACHE_FSW,origval);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_max_read_ahead(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = cache_max_prefetch_size();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cache_min_read_ahead(void)
{
  static int retval = -1;
  if (retval == -1) {
    if( (retval = cache_prefetch_after_one_block_read()) ) {
      retval = cache_min_prefetch_size();
    }
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the parameter for the "Buffer continuous read" entry in DiskSim.  *
 *---------------------------------------------------------------------------*/
int
disksim_continuous_read(void)
{
  static int retval = -1;
  int csize,psize,sectors;

  if (retval == -1) {
    sectors = diskinfo_sect_per_track(random_cylinder(0,ANY_DEFECT,MAX_BAND));
    psize = test_prefetch_size();
    csize = cd_segment_size();
    if ((psize > 0.95*csize ) && (psize < 1.05*csize ))
      retval = 3;
    else if (test_prefetch_track_algorithm()) 
      retval = 4;
    else if (psize == sectors * thedisk->numsurfaces)
      retval = 2;
    else if (psize == sectors)
      retval = 1;
    else if (!test_prefetch_after_read())
      retval = 0;
    else
      retval = -1;
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the parameter for the "Fast write level" entry in DiskSim.        *
 *---------------------------------------------------------------------------*/
int
disksim_fast_write(void)
{
  static int retval = -1;

  if (retval == -1) {
    if (!test_writes_cached())
      retval = 0;
    else if (test_discon_writes_appended())
      retval = 2;
    else 
      retval = 1;
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns 1 if disk implements caching, otherwise returns 0.                *
 *---------------------------------------------------------------------------*/
int
disksim_caching_in_buffer(void)
{
  static int retval = -1;

  if (retval == -1) {
    if ((! test_discard_req_after_read()) || test_prefetch_after_read())
    retval = 1;
  else
    retval = 0;
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 * Returns the number of all (read and write) segments.                      *
 *---------------------------------------------------------------------------*/
int
disksim_count_all_segments(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = cd_number_of_segments();
    if (!test_share_rw_segment()) 
      retval += test_count_write_segments();
  }
  return(retval);
}


/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cd_number_of_segments(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_number_of_segments(0);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cd_segment_size(void)
{
  static int retval = -1;
  int numsegs;
  if (retval == -1) {
    numsegs = cd_number_of_segments();
    retval = test_segment_size(numsegs);
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cd_discard_req_after_read(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_discard_req_after_read();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cd_share_rw_segment(void)
{
  static int retval = -1;
  
  if (retval == -1) {
    retval = test_share_rw_segment();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cd_count_write_segments(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_count_write_segments();
  }
  return(retval);
}
/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cd_read_free_blocks(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_read_free_blocks();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cd_read_on_arrival(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_read_on_arrival();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int
cd_write_on_arrival(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_write_on_arrival();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
int 
cd_contig_writes_appended(void)
{
  static int retval = -1;

  if (retval == -1) {
    retval = test_contig_writes_appended();
  }
  return(retval);
}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
static void
cache_mode_page_modified(void) {
  static u_int8_t prev_value[30] = "new";
  u_int8_t block_offset = 4;
  u_int8_t p_offset;
  u_int8_t *page;

  exec_scsi_command(scsi_buf,SCSI_mode_sense(CACHE_PAGE));
  p_offset = (u_int8_t) scsi_buf[3] + block_offset;
  page = ((u_int8_t *) scsi_buf) + p_offset;
  
  if (memcmp(prev_value,"new",3))
    if (memcmp(prev_value,page,(u_int8_t) scsi_buf[p_offset+1])) {
      print_hexpage((char *) prev_value,(u_int8_t) scsi_buf[p_offset+1]);
      print_hexpage((char *) page,(u_int8_t) scsi_buf[p_offset+1]);
      printf("Page has been modified!\n");
    }
  memcpy(prev_value,page,(u_int8_t) scsi_buf[p_offset+1]);
}
 
/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/
void 
test_test_caches(void)
{
  int numsegs;

  printf ("Discard cached value after reading: \t\t%d\n",
	  test_discard_req_after_read());

  cache_mode_page_modified();

  printf ("Writes are cached: \t\t\t\t%d\n",
	  test_writes_cached());

  cache_mode_page_modified();

  printf ("Writes are appended: \t\t\t\t%d\n",
	  test_contig_writes_appended());
  
  cache_mode_page_modified();

  printf ("Number of (read) segments: \t\t\t%d\n",
	  (numsegs = test_number_of_segments(0)));

  cache_mode_page_modified();

  printf ("Size of a segment (in blocks): \t\t\t%d\n",
	  test_segment_size(numsegs));
  printf ("Size of a segment (in blocks): \t\t\t%d\n",
	  test_segment_size(numsegs));

  cache_mode_page_modified();

  printf ("Always prefetch after reading one block: \t%d\n",
	  test_prefetch_after_read());

  cache_mode_page_modified();

  printf ("DEC's prefetch algorithm: \t\t\t%d\n",
	  test_prefetch_track_algorithm());

  cache_mode_page_modified();

  printf ("Prefetch past track boundary:\t\t\t%d\n",
	  test_track_discon_prefetch());

  cache_mode_page_modified();

  printf ("Prefetch past cylinder boundary:\t\t%d\n",
	  test_cylinder_discon_prefetch());

  printf ("Write on arrival:\t\t\t\t%d\n",
	  test_write_on_arrival());

  cache_mode_page_modified();
  
  printf ("Read on arrival:\t\t\t\t%d\n",
	  test_read_on_arrival());

  cache_mode_page_modified();
    
  printf ("READ and WRITE share segment: \t\t\t%d\n",
	  test_share_rw_segment());

  cache_mode_page_modified();

  printf ("Prefetch size:\t\t\t\t\t%d sectors\n",
	  test_prefetch_size());

  cache_mode_page_modified();
}


/*---------------------------------------------------------------------------*
 * code for testing caches                                                   * 
 *---------------------------------------------------------------------------*/
void 
debug_caches(void)
{
  setbuf(stdout,NULL);

  busy_wait_loop(0.5 * ONE_REVOLUTION);
  warmup_disk();
  enable_caches();


  test_test_caches();
  
  printf ("\n--------------------------------------------------\n");
  printf ("---------------- Cache_testing -------------------\n");
  printf ("--------------------------------------------------\n\n");

  cache_mode_page_modified();

  printf ("Write segments: \t\t\t\t%d\n",
	    test_count_write_segments());

  cache_mode_page_modified();

  printf ("Fast write level: \t\t\t\t%d\n",
	  disksim_fast_write());

  cache_mode_page_modified();

  printf ("All segments: \t\t\t\t\t%d\n",
	  disksim_count_all_segments());
  
  cache_mode_page_modified();

  exit(0);


  cache_mode_page_modified();
  
  printf ("Discard cached value after reading: \t\t%d\n",
	  cache_discard_req_after_read());

  cache_mode_page_modified();

  printf ("Writes are cached: \t\t\t\t%d\n",
	  cache_writes_cached());

  cache_mode_page_modified();

  printf ("Read any free blocks: \t\t\t\t%d\n",
	  cache_read_free_blocks());

  cache_mode_page_modified();

  printf ("Writes are appended: \t\t\t\t%d\n",
	  cache_contig_writes_appended());

  cache_mode_page_modified();

  printf ("Fast write level: \t\t\t\t%d\n",
	  disksim_fast_write());
  
  cache_mode_page_modified();

//  exit(0);

  printf ("Number of (read) segments: \t\t\t%d\n",
	  cache_count_read_segments());

  cache_mode_page_modified();

  printf ("Number of all segments: \t\t\t%d\n",
	  disksim_count_all_segments());

  cache_mode_page_modified();

  printf ("Size of a segment (in blocks): \t\t\t%d\n",
	  cache_segment_size());

  cache_mode_page_modified();

  printf ("Always prefetch after reading one block: \t%d\n",
	  cache_prefetch_after_one_block_read());

  cache_mode_page_modified();

  printf ("DEC's prefetch algorithm: \t\t\t%d\n",
	  cache_prefetch_track_algorithm());

  cache_mode_page_modified();

  printf ("Prefetch past track boundary:\t\t\t%d\n",
	  cache_prefetch_track_boundary());
  cache_mode_page_modified();

  printf ("Prefetch past cylinder boundary:\t\t%d\n",
	  cache_prefetch_cylinder_boundary());

  printf ("Allow prefetch after reading: \t\t\t%d\n",
	  cache_allow_prefetch_after_read());
	  cache_mode_page_modified();

  printf ("Write on arrival:\t\t\t\t%d\n",
	  cache_write_on_arrival());

  cache_mode_page_modified();
  
  printf ("Read on arrival:\t\t\t\t%d\n",
	  cache_read_on_arrival());

  cache_mode_page_modified();
    
  printf ("READ and WRITE share segment: \t\t\t%d\n",
	  cache_share_rw_segment());

  cache_mode_page_modified();

  printf ("Prefetch size:\t\t\t\t\t%d sectors\n",
	  cache_prefetch_size());

  cache_mode_page_modified();

  printf ("Minimum read-ahead: \t\t\t\t%d\n",
	  cache_min_read_ahead());

  cache_mode_page_modified();

  printf ("Maximum read-ahead: \t\t\t\t%d\n",
	  cache_max_read_ahead());

  cache_mode_page_modified();

  printf ("DiskSim continuous read: \t\t\t%d\n",
	  disksim_continuous_read());

  cache_mode_page_modified();

  printf ("DiskSim caching in buffer: \t\t\t%d\n",
	  disksim_caching_in_buffer());

  cache_mode_page_modified();


  printf("Discard read: %d\n",test_discard_req_after_read());

}

/*---------------------------------------------------------------------------*
 *                                                                           *
 *---------------------------------------------------------------------------*/

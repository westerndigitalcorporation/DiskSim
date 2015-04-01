/*
 * DiskSim Storage Subsystem Simulation Environment (Version 3.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001, 2002, 2003.
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
 * A sample skeleton for a system simulator that calls DiskSim as
 * a slave.
 *
 * Contributed by Eran Gabber of Lucent Technologies - Bell Laboratories
 *
 * Usage:
 *	syssim <parameters file> <output file> <max. block number>
 * Example:
 *	syssim parv.seagate out 2676846
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "disksim_interface.h"
#include "syssim_interface.h"
#include "disksim_rand48.h"
#include "disksim_global.h"


static double now = 0;		/* current time */
double next_event = -1;	/* next event */

const unsigned long BLOCK_SIZE=4096;
const unsigned long SECTOR_SIZE=512;
#define BLOCK2SECTOR (BLOCK_SIZE/SECTOR_SIZE)

/*----- This is the private interface used by disksim_interface ----*/

/*
 * Schedule next callback at time t.
 * Note that there is only *one* outstanding callback at any given time.
 * The callback is for the earliest event.
 */
void
syssim_schedule_callback(void (*f)(void *,double), double t)
{
  next_event = t;
}


/*
 * de-scehdule a callback.
 */
void
syssim_deschedule_callback(void (*f)())
{
  next_event = -1;
}


/**
  * Reports completion back to the system simulator. 
  *
  * Internally, we set the completed flag for the request
  * and update the current simulated time to t.  We may
  * also want to execute a callback function. 
  */
void
syssim_report_completion(double t, struct disksim_request *r)
{
  r->completed = 1;
  now = t;
  //add_statistics(&st, t - r->start);

  // We may want to callback to the system simulator
  // r->notify_completion(t, r);
}



/*------ THIS IS THE PUBLIC INTERFACE USED BY SYSTEM SIMULATORS --- */
/**
  * @brief Get the simulated time.
  */
double syssim_gettime() {
	return now; 
}

/**
  * @brief Read a block of data from the disk.
  *
  * @param disksim The disksim structure.
  * @param blkno   The physical block number on the disk.
  * @param count   The number of bytes to read starting at the blkno.
  * @param arrive  The time to schedule the request. 
  */
unsigned long syssim_readblock(
		struct disksim_interface *disksim, 
		const unsigned long blkno, 
		const unsigned long count, 
		double arrive)
{
	struct disksim_request r; 

	/* make sure the request arrives later than 'now' */
	assert(arrive >= syssim_gettime()); 

	memset(&r, 0, sizeof(struct disksim_request));

	/* start */
	r.start = arrive; 

	/* Read request */
	r.flags = DISKSIM_READ;

	/* TODO: should we pass this in? */
	r.devno = 0; 

	/* set the block number to start from */
	r.blkno = blkno;

	assert(r.blkno >= 0);

	/* set bytes to read */
	r.bytecount = count;

	//fprintf(stdout, "blkno=%llu, %1.3e\n",(unsigned long long) blkno,
	//		(double)MS_TO_SYSSIMTIME(arrive));

	/* schedule the request in disksim */
	r.completed = 0;
	disksim_interface_request_arrive(disksim, arrive, &r);

	/* Process internal events until this I/O is completed */
	while(next_event >= 0) {
		now = next_event;
		next_event = -1;
		disksim_interface_internal_event(disksim, syssim_gettime(), 0);
	}

	if (!r.completed) {
		fprintf(stderr, "syssim_readblock: internal error. Last read not completed\n");
		return (off_t)-1;
	}

	/* read is complete sys_gettime() has current time */
	return count; 
}




/**
  * Simulate a read operation scheduled at time 'arrive'.
  * The caller uses (syssim_gettime()-arrive) to 
  * figure out how long the operation took. 
  *
  * This function only reads blocks.  If the request
  * is not block-aligned, this operation will read as 
  * many blocks as needed to fulfill the request. 
  *
  * @param disksim Points to the disksim structure. 
  * @param pos     The starting offset (from device) for read.  
  * @param count   The number of bytes to read.
  * @param arrive  The arrival time for the request. 
  *
  * @returns The number of bytes read. 
  */
unsigned long syssim_read(
		struct disksim_interface *disksim, 
		const unsigned long pos, 
		const size_t count, 
		double arrive)
{
	struct disksim_request r; 
	unsigned long sector; 
	unsigned long nblks; 
	unsigned long nbytes; 
	
	/* Calculate the physical blkno/sector (1 physical blk = 1 sector).
	 * We assume the sector is 512 bytes. 
	 */
	sector = pos/SECTOR_SIZE;

	/* calculate how many blocks to read */
	nbytes = (pos % SECTOR_SIZE) + count; 
	nblks = (unsigned long)ceill((double)nbytes/(double)SECTOR_SIZE);
	r.bytecount = nblks*SECTOR_SIZE;
	
	/* read the blocks */
	syssim_readblock(disksim, sector, nblks*SECTOR_SIZE, arrive); 
	return count; 
}


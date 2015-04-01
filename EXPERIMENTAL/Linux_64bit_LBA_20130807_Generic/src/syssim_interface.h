/*-------------------------------------------------------------------------*/
/**  @file disksim_interface.h
 *   
 *   @brief Prototypes for an interface between disksim and a system
 *          simulator. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 406 $.
 *   $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $.
 *
 */

#include "disksim_interface.h"
 
#ifndef _SYSSIM_INTERFACE_H_
#define _SYSSIM_INTERFACE_H_

#include <stdio.h>
#include "disksim_global.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)


	/*
	 * Schedule next callback at time t.
	 * Note that there is only *one* outstanding callback at any given time.
	 * The callback is for the earliest event.
	 */
	extern void syssim_schedule_callback(
			void (*f)(void *,double), 
			double t); 


	/*
	 * de-scehdule a callback.
	 */
	extern void syssim_deschedule_callback(void (*f)()); 


	/**
	 * Reports completion back to the system simulator. 
	 *
	 * Internally, we set the completed flag for the request
	 * and update the current simulated time to t.  We may
	 * also want to execute a callback function. 
	 */
	extern void syssim_report_completion(double t, struct disksim_request *r); 



	/*------ THIS IS THE PUBLIC INTERFACE USED BY SYSTEM SIMULATORS --- */
	/**
	 * @brief Get the simulated time.
	 */
	extern double syssim_gettime(); 


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
	extern unsigned long syssim_readblock(
			struct disksim_interface *disksim, 
			const unsigned long blockno, 
			const unsigned long count, 
			double arrive); 

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
          extern unsigned long syssim_read(
			struct disksim_interface *disksim, 
			const unsigned long pos, 
			const size_t count, 
			double arrive); 


	/**
	 * Simulate a write operation scheduled at time 'arrive'.
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
	extern unsigned long syssim_write(
			struct disksim_interface *disksim, 
			const unsigned long pos, 
			const size_t count, 
			double arrive); 


#endif


#ifdef __cplusplus
}
#endif

#endif

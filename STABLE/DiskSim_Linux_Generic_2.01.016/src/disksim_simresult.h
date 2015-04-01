/*
 * disksim_simresults.h
 *
 *  Created on: Dec 6, 2013
 *      Author: rhurst
 */

#ifndef DISKSIM_SIMRESULTS_H_
#define DISKSIM_SIMRESULTS_H_

#include "disksim_disk.h"

#define MAX_COMMANDS_LOGGED  2
#define FIRST_COMMAND_INDEX  0
#define SECOND_COMMAND_INDEX 1

typedef struct diskResults
{
    ioreq_event ioreq;

    struct dm_mech_state mechStateBegin;    // current head, cylinder and angular position
    struct dm_pbn        pbnDest;           // destination head, cylinder and sector

    int    seekDistance;                    // actuator seek distance (if required)
    double seekTime;                        // actuator movement time (if required)
    double latency;                         // disk rotational latency time (if required)
    double xferTime;                        // time required to transfer data from the disk (if required)
    double accessTime;                      // total time of disk activity (if required)
    struct dm_mech_acctimes mech_acctimes;  // storage for mechanical access times
    int    blocksXferred;                   // number of actual blocks transferred (DEVICE_BUFFER_SECTOR_DONE events)

#if LBA_SIZE_64
    LBA_t  block_first;                     //  first block xferred
    LBA_t  block_last;                      //  last  block xferred
#else
    int    block_first;                     //  first block xferred
    int    block_last;                      //  last  block xferred
#endif
} SEEK_RESULTS, *P_SEEK_RESULTS;

typedef struct sim_result
{
    int tagID_begin;                        // tag ID when this command is received
    int tagID_end;                          // tag ID when this command has completed
    int diskCtlrQueueDepth;                 // disk controller queue depth when this command is received
    int diskCtlrQueueDepthEnd;              // disk controller queue depth when this command has completed

    disk_cache_hit_t diskCacheHitStatus;    // command cache hit status

    double diskCtlrTimeBegin;               // time command enters the disk controller queue
    double diskCtlrTimeEnd;                 // time command completes (leaves the disk controoler queue)

    // IODriver variables
    double CCT;                             // elapsed time spent in the IODrive command FIFO
    double qCCT;                            // elapsed time spent executing the command
    double idleTime;

    int seekResultIndex;
    SEEK_RESULTS seekResults[MAX_COMMANDS_LOGGED];
} SIM_RESULT, *P_SIM_RESULT;

#define DISKSIM_SIM_RESULT_LIST (sim_result_list)

#define MAX_NUM_SIM_RESULTS   200000
#define LIST_SIM_RESULTS_END -1

extern char *simResultString;
extern SIM_RESULT sim_result_list[];

int  simresult_getNumSimResults( void );
int simresult_getSimResultContainsEndTagID( int tagID_end );
void simresult_initializeSimResults( void );

void simresult_set_seekResultIndex( int tagID, int seekResultIndex );
void simresult_add_new_request( ioreq_event *new_event, int queueDepth );
void simresult_update_diskCacheHitStatus( int tagID, disk_cache_hit_t hittype );
void simresult_update_mechanicalstate( int tagID, struct dm_mech_state begin, double realAngle, struct dm_pbn destpbn);
void simresult_update_diskInfo( int tagID, int seekDistance, double seekTime, double latency, double xferTime, double accessTime );
void simresult_update_diskCtlrQueueDepth( int tagID, int queueDepth );
void simresult_update_diskCtlrQueueDepthEnd( int tagID, int queueDepth );
void simresult_update_diskCtlrTimeBegin( int tagID, double diskCtlrTimeBegin );
void simresult_update_diskCtlrTimeEnd( int tagID, double diskCtlrTimeEnd );
void simresult_update_diskBlocksXferred( ioreq_event *curr );
void simresult_update_mech_acctimes( int tagID, struct dm_mech_acctimes *mech_acctimes );

void simresult_dumpSimResult( FILE *iob, int index );
void simresult_dumpSimResults( FILE *file );
void simresult_saveSimResults(void);

#endif /* DISKSIM_SIMRESULTS_H_ */

/*
 * disksim_simresults.c
 *
 *  Created on: Dec 6, 2013
 *      Author: rhurst
 */

#include <stdio.h>
#include "disksim_simresult.h"
#include "disksim_disk.h"

char *simResultString = "diskCtlrTimeBegin,diskCtlrTimeEnd,diskCtlrQueueDepth,diskCtlrQueueDepthEnd,tagID_begin,tagID_end,diskCacheHitStatus,CCT,qCCT"
                        ",tagID[1],opid[1],devno[1],blkno[1],blkcnt[1],flags[1]"
                        ",cylBegin[1],headBegin[1],thetaBegin[1],cylDest[1],headDest[1],sectorDest[1]"
                        ",SeekDistance[1],SeekTime[1],Latency[1],XferTime[1],AccessTime[1],rpo_seektime[1],rpo_latency[1],rpo_xfertime[1],BlocksXferred[1],block_first[1],block_last[1]"
                        ",tagID[2],opid[2],devno[2],blkno[2],blkcnt[2],flags[2]"
                        ",cylBegin[2],headBegin[2],thetaBegin[2],cylDest[2],headDest[2],sectorDest[2]"
                        ",SeekDistance[2],SeekTime[2],Latency[2],XferTime[2],AccessTime[2],rpo_seektime[2],rpo_latency[2],rpo_xfertime[2],BlocksXferred[2],block_first[2],block_last[2]";

// initialize each tagID to -1 (uninitialized flag)
void simresult_initializeSimResults( void )
{
    for( int index = 0; index < MAX_NUM_SIM_RESULTS; index++ )
    {
        DISKSIM_SIM_RESULT_LIST[index].tagID_begin = LIST_SIM_RESULTS_END;
    }
}

void simresult_dumpSimResult( FILE *iob, int index )
{
    fprintf( iob, "%.6f,%.6f,%d,%d,%d,%d,%s,%.6f,%.6f",
             DISKSIM_SIM_RESULT_LIST[index].diskCtlrTimeBegin,
             DISKSIM_SIM_RESULT_LIST[index].diskCtlrTimeEnd,
             DISKSIM_SIM_RESULT_LIST[index].diskCtlrQueueDepth,
             DISKSIM_SIM_RESULT_LIST[index].diskCtlrQueueDepthEnd,
             DISKSIM_SIM_RESULT_LIST[index].tagID_begin,
             DISKSIM_SIM_RESULT_LIST[index].tagID_end,
             disk_buffer_decode_disk_cache_hit_t( DISKSIM_SIM_RESULT_LIST[index].diskCacheHitStatus ),
             DISKSIM_SIM_RESULT_LIST[index].CCT,
             DISKSIM_SIM_RESULT_LIST[index].qCCT
    );
    for( int i = 0; i < MAX_COMMANDS_LOGGED; i++ )
    {
        fprintf( iob, ",%d,%d,%d,%d,%d,%u,%d,%d,%u,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%ld,%ld,%ld,%d,%d,%d",
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.tagID,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.opid,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.devno,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.blkno,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.bcount,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].ioreq.flags,

                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mechStateBegin.cyl,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mechStateBegin.head,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mechStateBegin.theta,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].pbnDest.cyl,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].pbnDest.head,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].pbnDest.sector,

                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].seekDistance,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].seekTime,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].latency,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].xferTime,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].accessTime,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mech_acctimes.seektime,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mech_acctimes.initial_latency,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].mech_acctimes.initial_xfer,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].blocksXferred,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].block_first,
                DISKSIM_SIM_RESULT_LIST[index].seekResults[i].block_last
        );
    }
    fprintf( iob, "\n" );
    fflush(iob);
}

void simresult_dumpSimResults( FILE *iob )
{
    for( int index = 0; index < MAX_NUM_SIM_RESULTS; index++ )
    {
        if( LIST_SIM_RESULTS_END == DISKSIM_SIM_RESULT_LIST[index].tagID_begin )
        {
            break;
        }
        simresult_dumpSimResult( iob, index );
    }
}

// save Sim Results to a file
void simresult_saveSimResults(void)
{
    FILE *simFile;

    if( NULL != (simFile = fopen( "log_sim_results.csv", "w" )) )
    {
        fprintf( simFile, "%s\n", simResultString );
        simresult_dumpSimResults( simFile );
        fclose( simFile );
    }
}

int simresult_getNumSimResults( P_SIM_RESULT sim_results )
{
    int numSimResults = 0;
    P_SIM_RESULT pSimResult = sim_results;
    while( NULL != pSimResult )
    {
        numSimResults++;
        pSimResult++;
    }
    return numSimResults;
}

// Return the tagID of the sim_result that contains tagID_end eqaul to the tagID_end
int simresult_getSimResultContainsEndTagID( int tagID_end )
{
    int index = 0;
    for( ; index < MAX_NUM_SIM_RESULTS; index++ )
    {
        if( tagID_end == DISKSIM_SIM_RESULT_LIST[index].tagID_end )
        {
            break;
        }
    }
    return index;
}

void simresult_set_seekResultIndex( int tagID, int seekResultIndex )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResultIndex = seekResultIndex;
}

void simresult_add_new_request( ioreq_event *new_event, int queueDepth )
{
    int seekResultIndex = DISKSIM_SIM_RESULT_LIST[new_event->tagID - 1].seekResultIndex;

    if( 0 == seekResultIndex )
    {
        // only log these for first command
        DISKSIM_SIM_RESULT_LIST[new_event->tagID - 1].diskCtlrQueueDepth = queueDepth;
        DISKSIM_SIM_RESULT_LIST[new_event->tagID - 1].tagID_begin        = new_event->tagID;
    }
    DISKSIM_SIM_RESULT_LIST[new_event->tagID - 1].seekResults[seekResultIndex].ioreq = *new_event;
}

// increment diskBlocksXferred for the specified IO Request
void simresult_update_diskBlocksXferred( ioreq_event *curr )
{
    int seekResultIndex = DISKSIM_SIM_RESULT_LIST[curr->tagID - 1].seekResultIndex;

    int diskBlocksXferred =  DISKSIM_SIM_RESULT_LIST[curr->tagID - 1].seekResults[seekResultIndex].blocksXferred++;
//    fprintf( outputfile, "%f: diskBlocksXferred %d\n", simtime, diskBlocksXferred );
//    dumpIOReq( "simresult_update_diskBlocksXferred", curr );
//    fflush( outputfile);
//    assert(diskBlocksXferred <= 5000);

    if(0 == DISKSIM_SIM_RESULT_LIST[curr->tagID - 1].seekResults[seekResultIndex].block_last)
    {
        DISKSIM_SIM_RESULT_LIST[curr->tagID - 1].seekResults[seekResultIndex].block_first = curr->blkno;
    }
    DISKSIM_SIM_RESULT_LIST[curr->tagID - 1].seekResults[seekResultIndex].block_last = curr->blkno;
}

void simresult_update_diskCacheHitStatus( int tagID, disk_cache_hit_t hittype )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].diskCacheHitStatus = hittype;
}

void simresult_update_mechanicalstate( int tagID, struct dm_mech_state begin, double realAngle, struct dm_pbn destpbn)
{
    int seekResultIndex = DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResultIndex;

    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].mechStateBegin       = begin;
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].mechStateBegin.theta = dm_angle_dtoi( realAngle/360.0 );
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].pbnDest              = destpbn;
}

void simresult_update_diskInfo( int tagID, int seekDistance, double seekTime, double latency, double xferTime, double accessTime )
{
    int seekResultIndex = DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResultIndex;

    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].seekDistance = seekDistance;
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].seekTime     = seekTime;
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].latency      = latency;
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].xferTime     = xferTime;
    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].accessTime   = accessTime;
}

void simresult_update_diskCtlrQueueDepth( int tagID, int queueDepth )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].diskCtlrQueueDepth = queueDepth;
}

void simresult_update_diskCtlrQueueDepthEnd( int tagID, int queueDepth )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].diskCtlrQueueDepthEnd = queueDepth;
}

void simresult_update_diskCtlrTimeBegin( int tagID, double diskCtlrTimeBegin )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].diskCtlrTimeBegin = diskCtlrTimeBegin;
}


void simresult_update_diskCtlrTimeEnd( int tagID, double diskCtlrTimeEnd )
{
    DISKSIM_SIM_RESULT_LIST[tagID - 1].diskCtlrTimeEnd = diskCtlrTimeEnd;
}

void simresult_update_mech_acctimes( int tagID, struct dm_mech_acctimes *mech_acctimes )
{
    int seekResultIndex = DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResultIndex;

    DISKSIM_SIM_RESULT_LIST[tagID - 1].seekResults[seekResultIndex].mech_acctimes = *mech_acctimes;
}

// end of file

/*
 * DiskSim Storage Subsystem Simulation Environment (Version 4.0)
 * Revision Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
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
 * DiskSim Storage Subsystem Simulation Environment (Version 2.0)
 * Revision Authors: Greg Ganger
 * Contributors: Ross Cohen, John Griffin, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 1999.
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

/*
 * DiskSim Storage Subsystem Simulation Environment
 * Authors: Greg Ganger, Bruce Worthington, Yale Patt
 *
 * Copyright (C) 1993, 1995, 1997 The Regents of the University of Michigan 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 *  This software is provided AS IS, WITHOUT REPRESENTATION FROM THE
 * UNIVERSITY OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND
 * WITHOUT WARRANTY BY THE UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS
 * OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE LIABLE FOR ANY DAMAGES,
 * INCLUDING SPECIAL , INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
 * WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OF OR IN CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS
 * BEEN OR IS HEREAFTER ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 *
 * The names and trademarks of copyright holders or authors may NOT be
 * used in advertising or publicity pertaining to the software without
 * specific, written prior permission. Title to copyright in this software
 * and any associated documentation will at all times remain with copyright
 * holders.
 */

#include "disksim_global.h"
#include "disksim_ioface.h"
#include "disksim_pfface.h"
#include "disksim_iotrace.h"
#include "disksim_iosim.h"
#include "disksim_pfsim.h"
#include "disksim_iodriver.h"
#include "disksim_ioqueue.h"
#include "disksim_simresult.h"

#include "config.h"

#include "modules/disksim_global_param.h"

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#ifdef SUPPORT_CHECKPOINTS
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif


disksim_t *disksim = NULL;
SIM_RESULT sim_result_list[MAX_NUM_SIM_RESULTS];
int numEventBlocksAllocated = 0;

/* legacy hack for HPL traces... */
#define PRINT_TRACEFILE_HEADER	FALSE


/*** Functions to allocate and deallocate empty event structures ***/

#ifdef DEBUG_EXTRAQ

void dump_event( int eventNumber, event *evt )
{
    fprintf( outputfile, "\n*** %f: dump_event %d: %p\n", simtime, eventNumber, evt );
    fprintf( outputfile, "*** %f:     next: %p\n", simtime, evt->next );
    fprintf( outputfile, "*** %f:     prev: %p\n", simtime, evt->prev );
    fprintf( outputfile, "*** %f:     time: %f\n", simtime, evt->time );
    fprintf( outputfile, "*** %f:     time: %d\n", simtime, evt->type );
}


void dump_extraq( char *msg )
{
    int i;
    event * pEvent = disksim->extraq;

    fprintf( outputfile, "****** %f: dump_extraq (%s)  event queue entries: %d\n", simtime, msg, disksim->extraqlen );
    for( i = 0; i < disksim->extraqlen; i++ )
    {
        dump_event( i, pEvent );
        pEvent = pEvent->next;
    }
    fflush( outputfile );
}
#endif


/* Creates new, empty events (using malloc) and puts them on the extraq. */
/* Called by getfromextraq when the queue is found to be empty.          */

#define EXTRAEVENTS 32

static void allocateextra ()
{
    int i;
    event *temp = NULL;

#ifdef DEBUG_EXTRAQ
    fprintf( outputfile, "****** %f: allocateextra   Allocating new events: %d\n", simtime, EXTRAEVENTS );
#endif

    StaticAssert (sizeof(event) == DISKSIM_EVENT_SIZE);

    temp = (event *)calloc(EXTRAEVENTS, sizeof(event));
    numEventBlocksAllocated += EXTRAEVENTS;
    //   if ((temp = (event *)DISKSIM_malloc(ALLOCSIZE)) == NULL) {

    ddbg_assert(temp != 0);

    //   for (i=0; i<((ALLOCSIZE/DISKSIM_EVENT_SIZE)-1); i++) {

    for(i = 0; i < EXTRAEVENTS; i++) {
        temp[i].next = &temp[i+1];
    }
    temp[EXTRAEVENTS-1].next = disksim->extraq;
    disksim->extraq = temp;
    disksim->extraqlen += EXTRAEVENTS;
    //   disksim->extraqlen = ALLOCSIZE / DISKSIM_EVENT_SIZE;
#ifdef DEBUG_EXTRAQ
    dump_extraq( "allocateextra" );
#endif
}

/* A simple check to make sure that you're not adding an event
   to the extraq that is already there! */

int addtoextraq_check(event *ev)
{
    event *temp = disksim->extraq;

    while (temp != NULL) {
        if (ev == temp) {
            // I did it this way so that I could break at this line -schlos
            ddbg_assert(ev != temp);
        }
        temp = temp->next;
    }
    return 1;
}

/* Deallocates an event structure, adding it to the extraq free pool. */

INLINE void addtoextraq (event *temp)
{
    // addtoextraq_check(temp);

    if (temp == NULL) {
        return;
    }

#ifdef DEBUG_EXTRAQ
    fprintf( outputfile, "*** %f: addtoextraq time %f, type %s (%d), devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, temp->time, getEventString(temp->type), temp->type, ((ioreq_event *)temp)->devno, ((ioreq_event *)temp)->blkno, ((ioreq_event *)temp)->bcount, ((ioreq_event *)temp)->flags  );
    dump_extraq( "addtoextraq" );
#endif

    temp->next = disksim->extraq;
    temp->prev = NULL;
    disksim->extraq = temp;
    disksim->extraqlen++;
}


/* Allocates an event structure from the extraq free pool;
 * if empty, calls allocateextra to create some more.
 */

INLINE event * getfromextraq ()
{
    event *temp;

#ifdef DEBUG_EXTRAQ
    fprintf( outputfile, "*** %f: getfromextraq\n", simtime );
    dump_extraq( "getfromextraq" );
#endif

    if(disksim->extraqlen == 0) {
        allocateextra();
        temp = disksim->extraq;
        disksim->extraq = disksim->extraq->next;
    }
    else if(disksim->extraqlen == 1) {
        temp = disksim->extraq;
        disksim->extraq = NULL;
    }
    else {
        temp = disksim->extraq;
        disksim->extraq = disksim->extraq->next;
    }

    disksim->extraqlen--;
    temp->next = NULL;
    temp->prev = NULL;
    return temp;
}


/* Deallocates a list of event structures to the extraq free pool. */

void addlisttoextraq (event **headptr)
{
    event *tmp1, *tmp2;

#ifdef DEBUG_EXTRAQ
    fprintf( outputfile, "*** %f: addlisttoextraq  headptr %p\n", simtime, headptr );
    dump_extraq( "addlisttoextraq" );
#endif

    tmp1 = *headptr;
    if( NULL == tmp1 ) return;

    /*     while ((tmp = *headptr)) { */
    /*        *headptr = tmp->next; */
    /*        addtoextraq(tmp); */
    /*     } */


    do {
        tmp2 = tmp1->next;
        addtoextraq(tmp1);
        tmp1 = tmp2;
    } while(tmp1 && (tmp1 != (*headptr)));

    *headptr = NULL;
}


/* Returns a pointer to a copy of event passed as a parameter.  */

event *event_copy (event *orig)
{
    event *new_event = getfromextraq();
    memmove((char *)new_event, (char *)orig, DISKSIM_EVENT_SIZE);
    /* bcopy ((char *)orig, (char *)new, DISKSIM_EVENT_SIZE); */
    return((event *) new_event);
}


/*** Functions to manipulate intq, the queue of scheduled events ***/

/* Prints the intq to the output file, presumably for debug.  */


void disksim_dumpintq ()
{
    event *tmp;
    int i = 0;

#ifdef DEBUG_ON
    fprintf (outputfile, "disksim_dumpintq: intqlen %d\n", disksim->intqlen );
#endif

    tmp = disksim->intq;
    while (tmp != NULL) {
        i++;
        fprintf (outputfile, "time %f, type %d\n",
                tmp->time, tmp->type);
        tmp = tmp->next;
    }
}



/* Add an event to the intq.  The "time" field indicates when the event is */
/* scheduled to occur, and the intq is maintained in ascending time order. */

/* make this a binheap or something ... avoid walking the list */

INLINE void addtointq (event *newint)
{
    /* WARNING -- HORRIBLE HACK BELOW
     * In order to make the Memulator run (which sometimes has events arrive
     * "late") I've (JLG) commented out the following snippet.  A better
     * solution should be arrived at...
     */
#if 0
    if ((temp->time + DISKSIM_TIME_THRESHOLD) < simtime) {
        fprintf(stderr, "Attempting to addtointq an event whose time has passed\n");
        fprintf(stderr, "simtime %f, curr->time %f, type = %d\n", simtime, temp->time, temp->type);
        exit(1);
    }
#endif

#ifdef DEBUG_INTQ
    fprintf( outputfile, "******** %f: Entered addtointq  event %p, time %f, type %s (%d)", simtime, newint, newint->time, getEventString(newint->type), newint->type );
    switch( newint->type )
    {
    case BUS_DELAY_COMPLETE:
    	if( NULL != ((bus_event *)newint)->delayed_event )
		{
    		ioreq_event *curr = ((bus_event *)newint)->delayed_event;
            fprintf( outputfile, ", devno %d, blkno %d, bcount %d, flags %d, cause %d\n",
            		curr->devno, curr->blkno, curr->bcount, curr->flags, curr->cause );
		}
        break;
    }
    fprintf( outputfile, "\n" );
    fflush( outputfile );
#endif

    switch(disksim->trace_mode) {
    case DISKSIM_MASTER:
        //   if(write(disksim->tracepipes[1], (char *)newint, sizeof(event)) <= 0)
        //     {
        ////	 printf("addtointq() -- master write fd = %d\n", disksim->tracepipes[1]);
        ////	 perror("addtointq() -- master write");
        //     }
        break;

    case DISKSIM_SLAVE:
    {
        event tmpevt;
        double timediff;

        //       printf("addtointq() -- slave read\n");

        //       ddbg_assert(read(disksim->tracepipes[0], (char *)&tmpevt, sizeof(event)) > 0);

        timediff = fabs(tmpevt.time - newint->time);

        /*         printf("remote event: %d %f, local event: %d %f\n", */
        /*  	      tmpevt.type,  */
        /*  	      tmpevt.time, */
        /*  	      newint->type,  */
        /*  	      newint->time); */

        ddbg_assert(tmpevt.type == newint->type);
        //       ddbg_assert(timediff <= 0.001);

        newint->time = tmpevt.time;
        if(timediff > 0.000001) {
            printf("*** SLAVE: addtointq() timediff = %f\n", timediff);
        }
        fflush(stdout);
    }
    break;

    case DISKSIM_NONE:
    default:
        break;
    }


    if (disksim->intq == NULL) {
        disksim->intq = newint;
        newint->next = NULL;
        newint->prev = NULL;
    }
    else if (newint->time < disksim->intq->time) {
        newint->next = disksim->intq;
        disksim->intq->prev = newint;
        disksim->intq = newint;
        newint->prev = NULL;
    } else {
        event *run = disksim->intq;
        assert(run->next != run);
        while (run->next != NULL) {
            if (newint->time < run->next->time) {
                break;
            }
            run = run->next;
        }

        newint->next = run->next;
        run->next = newint;
        newint->prev = run;
        if (newint->next != NULL) {
            newint->next->prev = newint;
        }
    }
}


/* Retrieves the next scheduled event from the head of the intq. */

INLINE static event * getfromintq ()
{
    event *temp = NULL;

//    disksim_dumpintq();

    if (disksim->intq == NULL) {
        return(NULL);
    }
    temp = disksim->intq;
    disksim->intq = disksim->intq->next;
    if (disksim->intq != NULL) {
        disksim->intq->prev = NULL;
    }

    temp->next = NULL;
    temp->prev = NULL;
    return(temp);
}


/*
 * Removes a given event from the intq, thus descheduling it.
 * Returns TRUE if the event was found, FALSE if it was not.
 */

int removefromintq (event *curr)
{
    event *tmp;

#ifdef DEBUG_ON
    fprintf( outputfile, "******** %f: Entered removefromintq  time %f, type %s (%d)\n", simtime, curr->time, getEventString(curr->type), curr->type );
#endif

    tmp = disksim->intq;
    while (tmp != NULL) {
        if (tmp == curr) {
            break;
        }
        tmp = tmp->next;
    }
    if (tmp == NULL) {
        return(FALSE);
    }
    if (curr->next != NULL) {
        curr->next->prev = curr->prev;
    }
    if (curr->prev == NULL) {
        disksim->intq = curr->next;
    } else {
        curr->prev->next = curr->next;
    }

    curr->next = NULL;
    curr->prev = NULL;
    return(TRUE);
}


/*** Functions to initialize the system and print statistics ***/

void scanparam_int (char *parline, char *parname, int *parptr,
        int parchecks, int parminval, int parmaxval)
{
    if (sscanf(parline, "%d", parptr) != 1) {
        fprintf(stderr, "Error reading '%s'\n", parname);
        exit(1);
    }
    if (((parchecks & 1) && (*parptr < parminval)) || ((parchecks & 2) && (*parptr > parmaxval))) {
        fprintf(stderr, "Invalid value for '%s': %d\n", parname, *parptr);
        exit(1);
    }
}


void getparam_int (FILE *parfile, char *parname, int *parptr,
        int parchecks, int parminval, int parmaxval)
{
    char line[201];

    sprintf(line, "%s: %s", parname, "%d");
    if (fscanf(parfile, line, parptr) != 1) {
        fprintf(stderr, "Error reading '%s'\n", parname);
        exit(1);
    }
    fgets(line, 200, parfile); /* this allows comments, kinda ugly hack */
    if (((parchecks & 1) && (*parptr < parminval)) || ((parchecks & 2) && (*parptr > parmaxval))) {
        fprintf(stderr, "Invalid value for '%s': %d\n", parname, *parptr);
        exit(1);
    }
    fprintf (outputfile, "%s: %d\n", parname, *parptr);
}


void getparam_double (FILE *parfile, char *parname, double *parptr,
        int parchecks, double parminval, double parmaxval)
{
    char line[201];

    sprintf(line, "%s: %s", parname, "%lf");
    if (fscanf(parfile, line, parptr) != 1) {
        fprintf(stderr, "Error reading '%s'\n", parname);
        assert(0);
    }
    fgets(line, 200, parfile);
    if (((parchecks & 1) && (*parptr < parminval)) || ((parchecks & 2) && (*parptr > parmaxval)) || ((parchecks & 4) && (*parptr <= parminval))) {
        fprintf(stderr, "Invalid value for '%s': %f\n", parname, *parptr);
        assert(0);
    }
    fprintf (outputfile, "%s: %f\n", parname, *parptr);
}


void getparam_bool (FILE *parfile, char *parname, int *parptr)
{
    char line[201];

    sprintf(line, "%s %s", parname, "%d");
    if (fscanf(parfile, line, parptr) != 1) {
        fprintf(stderr, "Error reading '%s'\n", parname);
        exit(1);
    }
    fgets(line, 200, parfile);
    if ((*parptr != TRUE) && (*parptr != FALSE)) {
        fprintf(stderr, "Invalid value for '%s': %d\n", parname, *parptr);
        exit(1);
    }
    fprintf (outputfile, "%s %d\n", parname, *parptr);
}


void resetstats ()
{
    if (disksim->external_control | disksim->synthgen | disksim->iotrace) {
        io_resetstats();
    }
    if (disksim->synthgen) {
        pf_resetstats();
    }
}


static void stat_warmup_done (timer_event *timer)
{
    WARMUPTIME = simtime;
    resetstats();
    addtoextraq((event *)timer);
}


int disksim_global_loadparams(struct lp_block *b)
{

    // #include "modules/disksim_global_param.c"
    lp_loadparams(0, b, &disksim_global_mod);

    //  printf("*** warning: seed hack -- using 1207981\n");
    //  DISKSIM_srand48(1207981);
    //  disksim->seedval = 1207981;
    return 1;
}


void disksim_printstats2()
{
    fprintf (outputfile, "\nSIMULATION STATISTICS\n");
    fprintf (outputfile, "------------------------------\n\n");
    fprintf (outputfile, "Total time of run (msec): %f\n", simtime);
    fprintf (outputfile, "Total requests simulated: %d\n", disksim->totalreqs - 1);
    fprintf (outputfile, "Total events   simulated: %d\n", disksim->event_count);
    fprintf (outputfile, "Warm-up time (msec)     : %f\n", WARMUPTIME);
    fprintf(outputfile, "\nMemory Usage:\n" );
    fprintf(outputfile, "Event Blocks Allocated    : %d\n", numEventBlocksAllocated );
    fprintf(outputfile, "Event Blocks in Free Queue: %d\n", disksim->extraqlen );
    fprintf(outputfile, "Memory Leak (bytes)       : %d\n", (numEventBlocksAllocated - disksim->extraqlen) * (int)(sizeof( struct ev )) );

    if (disksim->synthgen) {
        pf_printstats();
    }
    if (disksim->external_control | disksim->synthgen | disksim->iotrace) {
        io_printstats();
    }
}


static void setcallbacks ()
{
    disksim->issuefunc_ctlrsmart = NULL;
    disksim->queuefind_ctlrsmart = NULL;
    disksim->wakeupfunc_ctlrsmart = NULL;
    disksim->donefunc_ctlrsmart_read = NULL;
    disksim->donefunc_ctlrsmart_write = NULL;
    disksim->donefunc_cachemem_empty = NULL;
    disksim->donefunc_cachedev_empty = NULL;
    disksim->idlework_cachemem = NULL;
    disksim->idlework_cachedev = NULL;
    disksim->concatok_cachemem = NULL;
    disksim->enablement_disk = NULL;
    disksim->timerfunc_disksim = NULL;
    disksim->timerfunc_ioqueue = NULL;
    disksim->timerfunc_cachemem = NULL;
    disksim->timerfunc_cachedev = NULL;

    disksim->timerfunc_disksim = stat_warmup_done;
    disksim->external_io_done_notify = NULL;

    io_setcallbacks();
    pf_setcallbacks();
}


static void initialize ()
{
    int val = (disksim->synthgen) ? 0 : 1;

    iotrace_initialize_file (disksim->iotracefile, disksim->traceformat, PRINT_TRACEFILE_HEADER);
    while (disksim->intq) {
        addtoextraq(getfromintq());
    }
    if (disksim->external_control | disksim->synthgen | disksim->iotrace) {
        io_initialize(val);
    }
    if (disksim->synthgen) {
        pf_initialize(disksim->seedval);
    } else {
        DISKSIM_srand48(disksim->seedval);
    }
    simtime = 0.0;
}


void disksim_cleanstats ()
{
    if (disksim->external_control | disksim->synthgen | disksim->iotrace) {
        io_cleanstats();
    }
    if (disksim->synthgen) {
        pf_cleanstats();
    }
}


void disksim_set_external_io_done_notify (disksim_iodone_notify_t fn)
{
    disksim->external_io_done_notify = fn;
}


event *io_done_notify (ioreq_event *curr)
{
    if (disksim->synthgen) {
        return pf_io_done_notify(curr, 0);
    }
    if (disksim->external_control) {
        disksim->external_io_done_notify (curr, disksim->notify_ctx);
    }
    return(NULL);
}


INLINE static event * 
getnextevent(void)
{
    event *temp;

    event *curr = getfromintq();
    if ( NULL != curr ) {
        simtime = curr->time;

        if (curr->type == NULL_EVENT) {
            if ((disksim->iotrace) && io_using_external_event(curr)) {
                temp = io_get_next_external_event(disksim->iotracefile);
//                if (!temp) { // hurst_r
//                    disksim->stop_sim = TRUE;
//                }
            }
            else {
                fprintf(stderr, "NULL_EVENT not linked to any external source\n");
                exit(1);
            }

            if (temp) {
                temp->type = NULL_EVENT;
                addtointq(temp);
            }
        } // curr->type == NULL_EVENT
    }
    else {
        fprintf (outputfile, "Returning NULL from getnextevent\n");
        fflush (outputfile);
    }

    return curr;
}


void disksim_checkpoint (char *checkpointfilename)
{
    FILE *checkpointfile;

    //printf ("disksim_checkpoint: simtime %f, totalreqs %d\n", simtime, disksim->totalreqs);
    if (disksim->checkpoint_disable) {
        fprintf (outputfile, "Checkpoint at simtime %f skipped because checkpointing is disabled\n", simtime);
        return;
    }

    if ((checkpointfile = fopen(checkpointfilename, "w")) == NULL) {
        fprintf (outputfile, "Checkpoint at simtime %f skipped because checkpointfile cannot be opened for write access\n", simtime);
        return;
    }

    if (disksim->iotracefile) {
        if (strcmp (disksim->iotracefilename, "stdin") == 0) {
            fprintf (outputfile, "Checkpoint at simtime %f skipped because iotrace comes from stdin\n", simtime);
            return;
        }
        fflush (disksim->iotracefile);
        fgetpos (disksim->iotracefile, &disksim->iotracefileposition);
    }

    if (OUTIOS) {
        fflush (OUTIOS);
        fgetpos (OUTIOS, &disksim->outiosfileposition);
    }

    if (outputfile) {
        fflush (outputfile);
        fgetpos (outputfile, &disksim->outputfileposition);
    }

    rewind (checkpointfile);
    fwrite (disksim, disksim->totallength, 1, checkpointfile);
    fflush (checkpointfile);
    fclose (checkpointfile);
}


void disksim_simstop ()
{
    disksim->stop_sim = TRUE;
}


void disksim_register_checkpoint (double attime)
{
    event *checkpoint = getfromextraq ();
    checkpoint->type = CHECKPOINT;
    checkpoint->time = attime;
    addtointq(checkpoint);
}


static void prime_simulation ()
{
    event *curr;

    if (disksim->warmup_event) {
        addtointq((event *)disksim->warmup_event);
        disksim->warmup_event = NULL;
    }
    if (disksim->checkpoint_interval > 0.0) {
        disksim_register_checkpoint (disksim->checkpoint_interval);
    }
    if (disksim->iotrace) {
        if ((curr = io_get_next_external_event(disksim->iotracefile)) == NULL) {
            disksim_cleanstats();
            return;
        }
        if ((disksim->traceformat != VALIDATE) && (disksim->closedios == 0)) {
            curr->type = NULL_EVENT;
        } else if (disksim->closedios) {
            int iocnt;
            io_using_external_event(curr);
            curr->time = simtime + disksim->closedthinktime;
            for (iocnt=1; iocnt < disksim->closedios; iocnt++) {
                curr->next = io_get_next_external_event(disksim->iotracefile);
                if (curr->next) {
                    io_using_external_event(curr->next);
                    curr->time = simtime + disksim->closedthinktime;
                    addtointq(curr->next);
                }
            }
        }

        // XXX yuck ... special case for 1st req
        if(disksim->traceformat == VALIDATE) {
            ioreq_event *tmp = (ioreq_event *)curr;
            disksim_exectrace("Request issue: simtime %f, devno %d, blkno %d, time %f\n",
                    simtime, tmp->devno, tmp->blkno, tmp->time);
        }

        addtointq(curr);
    }
}

char * getEventString( int eventType )
{
    switch( eventType )
    {
    case NULL_EVENT:
        return "NULL_EVENT";
    case CPU_EVENT:
        return "CPU_EVENT";
    case SYNTHIO_EVENT:
        return "SYNTHIO_EVENT";
    case IDLELOOP_EVENT:
        return "IDLELOOP_EVENT";
    case CSWITCH_EVENT:
        return "CSWITCH_EVENT";
    case INTR_EVENT:
        return "INTR_EVENT";
    case INTEND_EVENT:
        return "INTEND_EVENT";
    case TIMER_EXPIRED:
        return "TIMER_EXPIRED";
    case CHECKPOINT:
        return "CHECKPOINT";
    case STOP_SIM:
        return "STOP_SIM";
    case IO_REQUEST_ARRIVE:
        return "IO_REQUEST_ARRIVE";
    case IO_ACCESS_ARRIVE:
        return "IO_ACCESS_ARRIVE";
    case IO_INTERRUPT_ARRIVE:
        return "IO_INTERRUPT_ARRIVE";
    case IO_RESPOND_TO_DEVICE:
        return "IO_RESPOND_TO_DEVICE";
    case IO_ACCESS_COMPLETE:
        return "IO_ACCESS_COMPLETE";
    case IO_INTERRUPT_COMPLETE:
        return "IO_INTERRUPT_COMPLETE";
    case DEVICE_OVERHEAD_COMPLETE:
        return "DEVICE_OVERHEAD_COMPLETE";
    case DEVICE_ACCESS_COMPLETE:
        return "DEVICE_ACCESS_COMPLETE";
    case DEVICE_PREPARE_FOR_DATA_TRANSFER:
        return "DEVICE_PREPARE_FOR_DATA_TRANSFER";
    case DEVICE_DATA_TRANSFER_COMPLETE:
        return "DEVICE_DATA_TRANSFER_COMPLETE";
    case DEVICE_BUFFER_SEEKDONE:
        return "DEVICE_BUFFER_SEEK_DONE";
    case DEVICE_BUFFER_TRACKACC_DONE:
        return "DEVICE_BUFFER_TRACKACC_DONE";
    case DEVICE_BUFFER_SECTOR_DONE:
        return "DEVICE_BUFFER_SECTOR_DONE";
    case DEVICE_GOT_REMAPPED_SECTOR:
        return "DEVICE_GOT_REMAPPED_SECTOR";
    case DEVICE_GOTO_REMAPPED_SECTOR:
        return "DEVICE_GOTO_REMAPPED_SECTOR";
    case BUS_OWNERSHIP_GRANTED:
        return "BUS_OWNERSHIP_GRANTED";
    case BUS_DELAY_COMPLETE:
        return "BUS_DELAY_COMPLETE";
    case CONTROLLER_DATA_TRANSFER_COMPLETE:
        return "CONTROLLER_DATA_TRANSFER_COMPLETE";
    case TIMESTAMP_LOGORG:
        return "TIMESTAMP_LOGORG";
    case IO_TRACE_REQUEST_START:
        return "IO_TRACE_REQUEST_START";
    case IO_QLEN_MAXCHECK:
        return "IO_QLEN_MAXCHECK";
    default:
        return "UNKNOWN";
    }
}


void disksim_simulate_event (int num)
{
    event *curr;

#ifdef DEBUG_ON
    fprintf(outputfile, "********** %f: Entered disksim_simulate_event, num=%d\n", simtime, num );
#endif

    if ((curr = getnextevent()) == NULL) {
        if( NULL != DISKSIM_EVENT_LOG )
        {
            fprintf( DISKSIM_EVENT_LOG, "Events stopped!" );
            fflush(DISKSIM_EVENT_LOG);
        }
        disksim_simstop ();
    }
    else {
#ifdef DEBUG_ON
        printf( "%f: Entered disksim_simulate_event, eventNum %d, time %f, type=%d, %s, busno %d, slotno %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, num, curr->time, curr->type, getEventString( curr->type ), ((ioreq_event *)curr)->busno, ((ioreq_event *)curr)->slotno, ((ioreq_event *)curr)->devno, ((ioreq_event *)curr)->blkno, ((ioreq_event *)curr)->bcount, ((ioreq_event *)curr)->flags );
        fprintf(outputfile, "********** %f: Entered disksim_simulate_event, eventNum %d, time %f, type=%d, %s, busno %d, slotno %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, num, curr->time, curr->type, getEventString( curr->type ), ((ioreq_event *)curr)->busno, ((ioreq_event *)curr)->slotno, ((ioreq_event *)curr)->devno, ((ioreq_event *)curr)->blkno, ((ioreq_event *)curr)->bcount, ((ioreq_event *)curr)->flags );
        fflush(outputfile);
#endif

        switch(disksim->trace_mode) {
        case DISKSIM_NONE:
        case DISKSIM_MASTER:
            /*        fprintf(outputfile, "*** DEBUG TRACE\t%f\t%d\t%d\n", */
            /*  	      simtime, curr->type, num); */
            /*        fflush(outputfile); */
            break;

        case DISKSIM_SLAVE:
            break;
        }

        if( NULL != DISKSIM_EVENT_LOG )
        {
            if( 0 == disksim->event_count )
            {
                // output data header
                fprintf( DISKSIM_EVENT_LOG, "time,typeString,typeValue,current,next,prev,type,typeString,devno,blkno,bcount,flags,cause,causeString\n");
            }
            fprintf( DISKSIM_EVENT_LOG, "%f,%s,%d,%p,%p,%p",
                    curr->time, getEventString(curr->type), curr->type, curr, curr->next, curr->prev );
            if( BUS_DELAY_COMPLETE == curr->type)
            {
            	if( NULL != ((bus_event *)curr)->delayed_event )
            	{
            		ioreq_event * myCurr = ((bus_event *)curr)->delayed_event;
                    fprintf( DISKSIM_EVENT_LOG, ",%d,%s,%d,%d,%d,%d,%d,%s",
                    		myCurr->type,
                    		getEventString(myCurr->type),
                    		myCurr->devno,
                    		myCurr->blkno,
                    		myCurr->bcount,
                    		myCurr->flags,
                    		myCurr->cause,
                    		iosim_decodeInterruptEvent(myCurr)
                    );
            	}
            }
            else if(curr->type >= IO_REQUEST_ARRIVE && curr->type <= CONTROLLER_DATA_TRANSFER_COMPLETE )
            {
            	if( ((ioreq_event *)curr)->cause >= 0 )
            	{
                    fprintf( DISKSIM_EVENT_LOG, ",%d,%d,%d,%d,%d",
                            ((ioreq_event *)curr)->devno,
                            ((ioreq_event *)curr)->blkno,
                            ((ioreq_event *)curr)->bcount,
                            ((ioreq_event *)curr)->flags,
                            ((ioreq_event *)curr)->cause
                    );
            	}
            	else
            	{
                    fprintf( DISKSIM_EVENT_LOG, ",%d,%d,%d,%d,%d,%s",
                            ((ioreq_event *)curr)->devno,
                            ((ioreq_event *)curr)->blkno,
                            ((ioreq_event *)curr)->bcount,
                            ((ioreq_event *)curr)->flags,
                            ((ioreq_event *)curr)->cause,
                    		iosim_decodeInterruptEvent((ioreq_event *)curr)
                    );
            	}
            }
            fprintf( DISKSIM_EVENT_LOG, "\n" );
            fflush( DISKSIM_EVENT_LOG );
        }

        simtime = curr->time;

        if (curr->type == INTR_EVENT)
        {
            intr_acknowledge (curr);
        }
        else if ((curr->type >= IO_MIN_EVENT) && (curr->type <= IO_MAX_EVENT))
        {
            io_internal_event ((ioreq_event *)curr);
        }
        else if ((curr->type >= PF_MIN_EVENT) && (curr->type <= PF_MAX_EVENT))
        {
            pf_internal_event(curr);
        }
        else if (curr->type == TIMER_EXPIRED)
        {
            timer_event *timeout = (timer_event *) curr;
            (*timeout->func) (timeout);
        }
        else if ((curr->type >= MEMS_MIN_EVENT)
                && (curr->type <= MEMS_MAX_EVENT))
        {
            io_internal_event ((ioreq_event *)curr);
        }
        /* SSD: These are ssd specific events */
        else if ((curr->type >= SSD_MIN_EVENT)
                && (curr->type <= SSD_MAX_EVENT))
        {
            io_internal_event ((ioreq_event *)curr);
        }
        else if (curr->type == CHECKPOINT)
        {
            if (disksim->checkpoint_interval) {
                disksim_register_checkpoint(simtime + disksim->checkpoint_interval);
            }
            disksim_checkpoint (disksim->checkpointfilename);
        }
        else if (curr->type == STOP_SIM)
        {
            disksim_simstop ();
        }
        else if (curr->type == EXIT_DISKSIM)
        {
            exit (0);
        }
        else
        {
            fprintf(stderr, "Unrecognized event in simulate: %d\n", curr->type);
            exit(1);
        }


#ifdef FDEBUG
        fprintf (outputfile, "Event handled, going for next\n");
        fflush (outputfile);
#endif
    }
}


static void disksim_setup_outputfile (char *filename, char *mode)
{
    if (strcmp(filename, "stdout") == 0) {
        outputfile = stdout;
    }
    else {
        if ((outputfile = fopen(filename,mode)) == NULL) {
            fprintf(stderr, "Outfile %s cannot be opened for write access\n", filename);
            exit(1);
        }
    }

    if (strlen(filename) <= 255) {
        strcpy (disksim->outputfilename, filename);
    }
    else {
        fprintf (stderr, "Name of output file is too long (>255 bytes); checkpointing disabled\n");
        disksim->checkpoint_disable = 1;
    }

    setvbuf(outputfile, 0, _IONBF, 0);
}


static void disksim_setup_iotracefile (char *filename)
{
    if (strcmp(filename, "0") != 0) {
        assert (disksim->external_control == 0);
        disksim->iotrace = 1;
        if (strcmp(filename, "stdin") == 0) {
            disksim->iotracefile = stdin;
        } else {
            if ((disksim->iotracefile = fopen(filename,"rb")) == NULL) {
                fprintf(stderr, "Tracefile %s cannot be opened for read access\n", filename);
                exit(1);
            }
        }
    } else {
        disksim->iotracefile = NULL;
    }

    if (strlen(filename) <= 255) {
        strcpy (disksim->iotracefilename, filename);
    } else {
        fprintf (stderr, "Name of iotrace file is too long (>255 bytes); checkpointing disabled\n");
        disksim->checkpoint_disable = 1;
    }
}


static void setEndian(void) {
    int n = 0x11223344;
    char *c = (char *)&n;

    if(c[0] == 0x11) {
        disksim->endian = _BIG_ENDIAN;
    }
    else {
        disksim->endian = _LITTLE_ENDIAN;
    }

}

void disksim_setup_disksim (int argc, char **argv)
{

    setEndian();

    StaticAssert (sizeof(intchar) == 4);
    if (argc < 6) {
        fprintf(stderr,"Usage: %s paramfile outfile format iotrace synthgen?\n", argv[0]);
        exit(1);
    }

#ifdef FDEBUG
    fprintf (stderr, "%s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3],
            argv[4], argv[5]);
#endif

    if ((argc - 6) % 3) {
        fprintf(stderr, "Parameter file overrides must be 3-tuples\n");
        exit(1);
    }

    disksim_setup_outputfile (argv[2], "w");
    fprintf (outputfile, "\n*** DiskSim version      : %s", VERSION );
    fprintf (outputfile, "\n*** Output file name     : %s", argv[2]);

    if(strcmp (argv[3], "external") == 0) {
        disksim->external_control = 1;
    }
    else {
        iotrace_set_format(argv[3]);
    }

    fprintf (outputfile, "\n*** Parameter File       : %s", argv[1]);
    fprintf (outputfile, "\n*** Input trace format   : %s", argv[3]);

    disksim_setup_iotracefile (argv[4]);
    fprintf (outputfile, "\n*** I/O trace used       : %s", argv[4]);

    if(strcmp(argv[5], "0") != 0) {
        disksim->synthgen = 1;
        disksim->iotrace = 0;
    }

    fprintf(outputfile, "\n*** Synthgen to be used? : %s\n", disksim->synthgen ? "yes" : "no" );

    disksim->overrides = argv + 6;
    disksim->overrides_len = argc - 6;

    fprintf(outputfile, "\n*** Command Line Overrides specified: %d", disksim->overrides_len/3 );
    if( 0 != disksim->overrides_len )
    {
        int commandIndex = 0;
        for( ; commandIndex < disksim->overrides_len; commandIndex += 3)
        {
            fprintf(outputfile, "\n*** Override %d: %s   %s   %s", (commandIndex % 3) + 1, disksim->overrides[commandIndex], disksim->overrides[commandIndex+1], disksim->overrides[commandIndex+2] );
        }
    }
    fprintf( outputfile, "\n\n" );
    fflush( outputfile );

    // asserts go to stderr by default
    ddbg_assert_setfile(stderr);


    disksim->tracepipes[0] = 8;
    disksim->tracepipes[1] = 9;

    /*   disksim->tracepipes[0] = dup(8); */
    /*    disksim->tracepipes[1] = dup(9); */
    /*    close(8); */
    /*    close(9); */

    /*    if(!strcmp(argv[6], "master")) { */
    /*      printf("*** Running in MASTER mode\n"); */
    /*      disksim->trace_mode = DISKSIM_MASTER; */
    /*      close(disksim->tracepipes[0]); */
    /*    } */
    /*    else if(!strcmp(argv[6], "slave")) { */
    /*      printf("*** Running in SLAVE mode\n"); */
    /*      disksim->trace_mode = DISKSIM_SLAVE; */
    /*      close(disksim->tracepipes[1]); */
    /*    } */
    /*    else { */
    //    printf("*** not running in master-slave mode\n");
    disksim->trace_mode = DISKSIM_NONE;
    /*    } */

#ifdef DEBUG_ON
    fprintf( outputfile, "*** Processing parameter file: %s\n\n", argv[1] );
#endif

    if(disksim_loadparams(argv[1], disksim->synthgen)) {
        fprintf(stderr, "*** error: FATAL: failed to load disksim parameter file.\n");
        exit(1);
    }
    else {
        fprintf(outputfile, "*** loadparams complete\n");
        fprintf(outputfile, "*** Running Simulation...\n");
        fflush(outputfile);
        /*       config_release_typetbl(); */
    }

    if(!disksim->iosim_info) {
        /* if the user didn't provide an iosim block */
        iosim_initialize_iosim_info ();
    }

    initialize();
    fprintf(outputfile, "Initialization complete\n");
    fprintf(outputfile, "Running simulation...\n");
    fflush(outputfile);
    prime_simulation();

    // XXX abstract this
    fclose(statdeffile);
    statdeffile = NULL;
}


void disksim_restore_from_checkpoint (char *filename)
{
#ifdef SUPPORT_CHECKPOINTS
    int fd;
    void *startaddr;
    int totallength;
    int ret;

    fd = open (filename, O_RDWR);
    assert (fd >= 0);
    ret = read (fd, &startaddr, sizeof (void *));
    assert (ret == sizeof(void *));
    ret = read (fd, &totallength, sizeof (int));
    assert (ret == sizeof(int));
    disksim = (disksim_t *) mmap (startaddr, totallength, (PROT_READ|PROT_WRITE), (MAP_PRIVATE), fd, 0);
    //printf ("mmap at %p, ret %d, len %d, addr %p\n", disksim, ret, totallength, startaddr);
    //perror("");
    assert (disksim == startaddr);

    disksim_setup_outputfile (disksim->outputfilename, "r+");
    if (outputfile != NULL) {
        ret = fsetpos (outputfile, &disksim->outputfileposition);
        assert (ret >= 0);
    }
    disksim_setup_iotracefile (disksim->iotracefilename);
    if (disksim->iotracefile != NULL) {
        ret = fsetpos (disksim->iotracefile, &disksim->iotracefileposition);
        assert (ret >= 0);
    }
    if ((strcmp(disksim->outiosfilename, "0") != 0) && (strcmp(disksim->outiosfilename, "null") != 0)) {
        if ((OUTIOS = fopen(disksim->outiosfilename, "r+")) == NULL) {
            fprintf(stderr, "Outios %s cannot be opened for write access\n", disksim->outiosfilename);
            exit(1);
        }
        ret = fsetpos (OUTIOS, &disksim->outiosfileposition);
        assert (ret >= 0);
    }
#else
    assert ("Checkpoint/restore not supported on this platform" == 0);
#endif
    setcallbacks();
}


void disksim_run_simulation ()
{
    disksim->event_count = 0;

    DISKSIM_srand48(1000003);

    while (disksim->stop_sim == FALSE) {

#ifdef DEBUG_ON
        disksim_dumpintq ();
        printf("disksim_run_simulation: event %d\n", disksim->event_count );
#endif
        disksim_simulate_event(disksim->event_count);
        disksim->event_count++;

//        printf( "base     queue: listlen %d, numoutstanding %d\n", disksim->iodriver_info->overallqueue->base.listlen, disksim->iodriver_info->overallqueue->base.numoutstanding );
//        printf( "priority queue: listlen %d, numoutstanding %d\n", disksim->iodriver_info->overallqueue->priority.listlen, disksim->iodriver_info->overallqueue->priority.numoutstanding );
//        printf( "timeout  queue: listlen %d, numoutstanding %d\n", disksim->iodriver_info->overallqueue->timeout.listlen, disksim->iodriver_info->overallqueue->timeout.numoutstanding );
    }
//    printf("disksim_run_simulation(): simulated %d events\n", disksim->event_count);
}



void disksim_cleanup(void) 
{
    int i;

    if (outputfile)
    {
        fclose (outputfile);
    }


    if (disksim->iotracefile)
    {
        fclose(disksim->iotracefile);
    }


    iodriver_cleanup();

    if(OUTIOS)
    {
        fclose(OUTIOS);
        OUTIOS = NULL;
    }
  
  if( NULL != DISKSIM_DISK_QUEUE_LOG )
  {
     fclose( DISKSIM_DISK_QUEUE_LOG );
	 DISKSIM_DISK_QUEUE_LOG = NULL;
  }
  
  if( NULL != DISKSIM_DISK_LOG )
  {
    fclose( DISKSIM_DISK_LOG );
    DISKSIM_DISK_LOG = NULL;
  }

  if( NULL != DISKSIM_DEBUG_LOG )
  {
     fclose( DISKSIM_DEBUG_LOG );
     DISKSIM_DEBUG_LOG = NULL;
  }

  if( NULL != DISKSIM_EVENT_LOG )
  {
    fclose( DISKSIM_EVENT_LOG );
    DISKSIM_EVENT_LOG = NULL;
  }

  if( NULL != DISKSIM_SYNTHIO_LOG )
  {
    fclose( DISKSIM_SYNTHIO_LOG );
    DISKSIM_SYNTHIO_LOG = NULL;
  }
}

void disksim_printstats() {
    fprintf(outputfile, "Simulation complete\n");
    fflush(outputfile);

    disksim_cleanstats();
    disksim_printstats2();
}

void disksim_cleanup_and_printstats ( time_t startTime, time_t endTime )
{
    disksim_printstats();
    fprintf( outputfile, "\nStart   Time: %s", asctime( localtime( &startTime) ));
    fprintf( outputfile, "\nEnd     Time: %s", asctime( localtime( &endTime) ));
    fprintf( outputfile, "\nElapsed Time (seconds): %f", difftime( endTime, startTime ) );
    fprintf( outputfile, "\nSimulation End" );
    disksim_cleanup();
}


int 
disksim_initialize_disksim_structure (struct disksim *disksim)
{
    FILE *tmpfp;
    //   disksim->startaddr = addr;
    //   disksim->totallength = len;
    //   disksim->curroffset = sizeof(disksim_t);

    simresult_initializeSimResults();

    disksim->closedthinktime = 0.0;
    WARMUPTIME = 0.0;
    simtime = 0.0;
    disksim->lastphystime = 0.0;
    disksim->checkpoint_interval = 0.0;
    disksim->tagIDBegin = disksim->tagIDEnd = 0;

    // if file exists, delete it and open if for write
    // otherwise no debug output
    DISKSIM_DISK_QUEUE_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_diskqueue.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_DISK_QUEUE_LOG = fopen( "log_diskqueue.txt", "w" );
    }

    DISKSIM_DISK_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_diskio.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_DISK_LOG = fopen( "log_diskio.txt", "w" );
    }

    DISKSIM_DEBUG_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_debug.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_DEBUG_LOG   = fopen( "log_debug.txt", "w" );
    }

    DISKSIM_EVENT_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_event.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_EVENT_LOG = fopen( "log_event.txt", "w" );
    }

    DISKSIM_SIM_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_sim_results.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_SIM_LOG = fopen( "log_sim_results.txt", "w" );
        fprintf( DISKSIM_SIM_LOG, "%s\n", simResultString );
        fflush( DISKSIM_SIM_LOG );
    }

    DISKSIM_SYNTHIO_LOG = NULL;
    if( NULL != (tmpfp = fopen( "log_synthio.txt", "r" )) )
    {
        fclose( tmpfp );
        DISKSIM_SYNTHIO_LOG = fopen( "log_synthio.txt", "w" );
    }

    return 0;
}


void
disksim_exectrace(char *fmt, ...) {

    if( NULL != disksim->exectrace ) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(disksim->exectrace, fmt, ap);
        va_end(ap);
        fflush( disksim->exectrace );
    }
}

// End of file disksim.cpp


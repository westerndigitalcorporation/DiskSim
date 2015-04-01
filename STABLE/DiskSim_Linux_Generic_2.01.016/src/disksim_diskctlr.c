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
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_disk.h"
#include "disksim_ioqueue.h"
#include "disksim_bus.h"
#include "disksim_simresult.h"
#include "inline.h"

#include "inst.h"

#define DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS   -2
#define DISKCTLR_DISCONNECT_REQUESTED                              -1
#define DISKCTLR_TRANSFER_COMPLETED                                 0

// move these protos, etc, to "disk_private.h" or something!
static void 
disk_buffer_sector_done (disk *currdisk, ioreq_event *curr);

static void 
disk_buffer_seekdone (disk *currdisk, ioreq_event *curr);

int  
disk_enablement_function (ioreq_event *currioreq);

static void 
disk_got_remapped_sector (disk *currdisk, ioreq_event *curr);

static void 
disk_check_bus (disk *currdisk, diskreq *currdiskreq);


static void 
disk_release_hda (disk *currdisk, 
        diskreq *currdiskreq);

static void 
disk_check_hda (disk *currdisk, 
        diskreq *currdiskreq,
        int ok_to_check_bus);

static ioreq_event * 
disk_buffer_transfer_size (disk *currdisk, 
        diskreq *currdiskreq,
        ioreq_event *curr);

static void 
disk_activate_read (disk *currdisk, 
        diskreq *currdiskreq,
        int setseg,
        int ok_to_check_bus);

static void 
disk_activate_write (disk *currdisk, 
        diskreq *currdiskreq,
        int setseg,
        int ok_to_check_bus);

static double 
disk_buffer_estimate_acctime(disk *currdisk, 
        ioreq_event *curr,
        double maxtime);

segment * disk_flush_write_cache( disk *currdisk, diskreq *currDiskReq );

void dumpIOReq( const char * msg, ioreq_event * curr )
{
    fprintf (outputfile, "%f: %s  curr %p", simtime, msg, curr );
    if( NULL != curr )
    {
        fprintf (outputfile, ", type %s (%d), cause %s (%d), tagID %d, devno %d, blkno %d, bcount %d, flags %x, opid %d",
                getEventString( curr->type ), curr->type, iosim_decodeInterruptEvent(curr), curr->cause, curr->tagID, curr->devno, curr->blkno, curr->bcount, curr->flags, curr->opid );
    }
    fprintf (outputfile, "\n" );
}


int 
disk_set_depth(int diskno, int inbusno, int depth, int slotno)
{
    int cnt;
    disk *currdisk = getdisk (diskno);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_set_depth - inbusno %d, depth %d, slotno %d\n", simtime, inbusno, depth, slotno );
    fflush( outputfile );
#endif

    cnt = currdisk->numinbuses;
    currdisk->numinbuses++;
    ddbg_assert3(((cnt + 1) <= MAXINBUSES),
            ("Too many inbuses specified for disk %d - %d (see BUG #30)\n",
                    diskno, (cnt+1)));

    currdisk->inbuses[cnt] = inbusno;
    currdisk->depth[cnt] = depth;
    currdisk->slotno[cnt] = slotno;
    return(0);
}


int disk_get_depth (int diskno)
{
    disk *currdisk = getdisk (diskno);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_get_depth - diskno %d, depth %d\n", simtime, diskno, currdisk->depth[0] );
    fflush( outputfile );
#endif

    return(currdisk->depth[0]);
}


int disk_get_slotno (int diskno)
{
    disk *currdisk = getdisk (diskno);
    return(currdisk->slotno[0]);
}


int disk_get_inbus (int diskno)
{
    disk *currdisk = getdisk (diskno);
    return(currdisk->inbuses[0]);
}


int disk_get_maxoutstanding (int diskno)
{
    disk *currdisk = getdisk (diskno);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_get_maxoutstanding  maxqlen %d\n", simtime, currdisk->maxqlen );
    fflush( outputfile );
#endif

    return(currdisk->maxqlen);
}


double disk_get_blktranstime (ioreq_event *curr)
{
    disk *currdisk = getdisk (curr->devno);
    double tmptime;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_blktranstime\n", simtime );
    fflush( outputfile );
#endif

    tmptime = bus_get_transfer_time(disk_get_busno(curr), 1, (curr->flags & READ));
    if(tmptime < currdisk->blktranstime) {
        tmptime = currdisk->blktranstime;
    }
    return(tmptime);
}


INLINE int disk_get_busno (ioreq_event *curr)
{
    disk *currdisk = getdisk (curr->devno);
    intchar busno;
    int depth;

    busno.value = curr->busno;
    depth = currdisk->depth[0];
    return(busno.byte[depth]);
}


/*
 **- disk_send_event_up_path

 Acquires the bus (if not already acquired), then uses bus_delay to
 send the event up the path.

 If the bus is already owned by this device or can be acquired
 immediately (interleaved bus), the event is sent immediately.
 Otherwise, disk_bus_ownership_grant will later send the event.  
 */

static void disk_send_event_up_path (ioreq_event *curr, double delay)
{
    disk *currdisk = getdisk (curr->devno);
    int busno;
    int slotno;

    disksim_inst_enter();

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_send_event_up_path", curr );
    fflush( outputfile );
#endif

    busno = disk_get_busno(curr);
    slotno = currdisk->slotno[0];

    /* Put new request at head of buswait queue */
    curr->next = currdisk->buswait;
    currdisk->buswait = curr;

    curr->tempint1 = busno;
    curr->time = delay;
    if(currdisk->busowned == -1) {
        /* fprintf (outputfile, "Must get ownership of the bus first\n"); */
        if(curr->next) {
            fprintf(stderr,"Multiple bus requestors detected in "
                    "disk_send_event_up_path\n");
            /* Is this OK?  Strange that the new requester is put on the head?  */
        }
        if(bus_ownership_get(busno, slotno, curr) == FALSE) {
            /* Remember when we started waiting (only place this is written) */
            currdisk->stat.requestedbus = simtime;
        }
        else {
            bus_delay(busno, DEVICE, curr->devno, delay, curr); /* Never for SCSI */
        }
    }
    else if(currdisk->busowned == busno) {
        /*
      fprintf (outputfile, "Already own bus - so send it on up\n");
         */
        bus_delay(busno, DEVICE, curr->devno, delay, curr);
    } 
    else {
        ddbg_assert2(0, "Wrong bus owned for transfer desired");
    }
}


/*
 * disk_bus_ownership_grant
 *
 * Calls bus_delay to handle the event that the disk has been granted
 * the bus.  I believe this is always initiated by a call to
 * disk_send_even_up_path.
 */

void 
disk_bus_ownership_grant(int devno, ioreq_event *curr, int busno, double arbdelay)
{
    disk *currdisk = getdisk (devno);
    ioreq_event *tmp;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_bus_ownership_grant\n", simtime );
    fflush( outputfile );
#endif

    tmp = currdisk->buswait;
    while ((tmp != NULL) && (tmp != curr)) {
        tmp = tmp->next;
    }

    ddbg_assert3(tmp != 0, ("Bus ownership granted to unknown disk request - "
            "devno %d, busno %d",
            devno, busno));

    currdisk->busowned = busno;
    currdisk->stat.waitingforbus += simtime - currdisk->stat.requestedbus;
    ddbg_assert(arbdelay == (simtime - currdisk->stat.requestedbus));
    currdisk->stat.numbuswaits++;
    bus_delay(busno, DEVICE, devno, tmp->time, tmp);
}


void 
disk_bus_delay_complete(int devno, ioreq_event *curr, int sentbusno)
{
    disk *currdisk = getdisk (devno);
    intchar slotno;
    intchar busno;
    int depth;

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_bus_delay_complete", curr );
#endif

    if(curr == currdisk->buswait) {
        currdisk->buswait = curr->next;
    } else {
        ioreq_event *tmp = currdisk->buswait;
        while ((tmp->next != NULL) && (tmp->next != curr)) {
            tmp = tmp->next;
        }

        ddbg_assert3(tmp->next == curr, ("Bus delay complete for unknown disk "
                "request - devno %d, busno %d",
                devno, busno.value));

        tmp->next = curr->next;
    }
    busno.value = curr->busno;
    slotno.value = curr->slotno;
    depth = currdisk->depth[0];
    slotno.byte[depth] = slotno.byte[depth] >> 4;
    curr->time = 0.0;
    if(depth == 0) {
        intr_request ((event *)curr);
    }
    else {
        bus_deliver_event(busno.byte[depth], slotno.byte[depth], curr);
    }
}


static void 
disk_prepare_for_data_transfer(ioreq_event *curr)
{
    disk *currdisk = getdisk(curr->devno);
    diskreq *currdiskreq = (diskreq *)curr->ioreq_hold_diskreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_prepare_for_data_transfer\n", simtime );
    fflush( outputfile );
#endif

    disksim_inst_enter();

    /*
    fprintf (outputfile, "%f, Entering disk_prepare_for_data_transfer: diskno = %d\n", simtime, curr->devno);
     */

    if(currdiskreq->flags & FINAL_WRITE_RECONNECTION_1) {
        currdiskreq->flags |= FINAL_WRITE_RECONNECTION_2;
    }
    addtoextraq((event *) curr);
    disk_check_bus(currdisk,currdiskreq);
}


/* check to see if current prefetch should be aborted */

static void disk_check_prefetch_swap (disk *currdisk)
{
    ioreq_event *nextioreq;
    diskreq     *nextdiskreq;
    int		setseg = FALSE;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_prefetch_swap\n", simtime );
    fflush( outputfile );
#endif

    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf (outputfile, "%12.6f            Entering disk_check_prefetch_swap for disk %d\n", simtime, currdisk->devno);
    //    fflush(outputfile);
    //  }

    ddbg_assert(currdisk->effectivehda != 0);

    ddbg_assert(currdisk->effectivehda->seg != 0);


    {
        int ispureprefetch =
                (currdisk->effectivehda->seg->access->flags & BUFFER_BACKGROUND)
                && (currdisk->effectivehda->seg->access->flags & READ);

        ddbg_assert2(ispureprefetch, "effectivehda->seg->access is not a pure "
                "prefetch");

    }

    nextioreq = ioqueue_show_next_request(currdisk->queue);
    if(nextioreq) {
        nextdiskreq = (diskreq *)nextioreq->ioreq_hold_diskreq;
        if(!nextdiskreq->seg) {
            setseg = TRUE;
            disk_buffer_select_segment(currdisk,nextdiskreq,FALSE);
        }
        if(nextioreq->flags & READ) {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_prefetch_swap  Calling disk_activate_read\n", simtime );
    fflush( outputfile );
#endif
            disk_activate_read(currdisk,nextdiskreq,setseg,TRUE);
        }
        else {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_prefetch_swap  Calling disk_activate_write\n", simtime );
    fflush( outputfile );
#endif
            disk_activate_write(currdisk,nextdiskreq,setseg,TRUE);
        }
    }
}


/* send completion up the line */

static void disk_request_complete(disk *currdisk, diskreq *currdiskreq, ioreq_event *curr)
{
    ioreq_event *tmpioreq = currdiskreq->ioreqlist;
    diskreq     *nextdiskreq;
    double       delay;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_request_complete  Entering\n", simtime );
    fprintf (outputfile, "********** %f: disk_request_complete - curr    : type %s (%d), devno %d, blkno %d, bcount %d, flags %X\n", simtime, getEventString( curr->type ), curr->type, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fprintf (outputfile, "********** %f: disk_request_complete - tmpioreq: type %s (%d), devno %d, blkno %d, bcount %d, flags %X\n", simtime, getEventString( tmpioreq->type ), tmpioreq->type, tmpioreq->devno, tmpioreq->blkno, tmpioreq->bcount, tmpioreq->flags );
    ioqueue_print_contents (currdisk->queue);
    fflush( outputfile );
#endif

    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf (outputfile, "%12.6f  %8p "
    //	     " Entering disk_request_complete\n", simtime, currdiskreq);
    //    fflush(outputfile);
    //  }


    //  printf("disk_request_complete %f\n", simtime);

    ddbg_assert(currdisk->effectivebus == currdiskreq);


    if(curr->blkno != tmpioreq->blkno) {
        addtoextraq((event *) curr);
        curr = ioreq_copy(tmpioreq);
    }

    currdisk->lastflags = curr->flags;

    ddbg_assert(currdisk->outstate == DISK_TRANSFERING);

    currdiskreq->flags |= COMPLETION_SENT;

    if(currdisk->const_acctime) {
        // SIMPLEDISK device path
        ddbg_assert2(!(currdiskreq->flags & HDA_OWNED),
                "HDA_OWNED set for fixed-access-time disk");


        ddbg_assert2(ioqueue_get_specific_request(currdisk->queue,tmpioreq) != NULL,
                "ioreq_event not found");

        disk_interferestats(currdisk, tmpioreq);
        /* GROK: this would seem to leak (or worse) for concatenating schedulers */
        ddbg_assert2(ioqueue_physical_access_done(currdisk->queue,tmpioreq) != NULL,
                "ioreq_event not found");

        currdisk->currentbus =
                currdisk->currenthda =
                        currdisk->effectivehda = NULL;

        tmpioreq = ioqueue_show_next_request(currdisk->queue);
        if(tmpioreq) {
            nextdiskreq = (diskreq*) tmpioreq->ioreq_hold_diskreq;
            if(nextdiskreq->ioreqlist->flags & READ) {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_request_complete  Calling disk_activate_read\n", simtime );
    fflush( outputfile );
#endif
                disk_activate_read(currdisk, nextdiskreq, FALSE, TRUE);
            }
            else {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_request_complete  Calling disk_activate_write\n", simtime );
    fflush( outputfile );
#endif
                disk_activate_write(currdisk, nextdiskreq, FALSE, TRUE);
            }
        }
    }
    // DISK device path
    else if(tmpioreq->flags & READ) {
        while (tmpioreq) {
            void* evtfound = 0;
            if(!(currdiskreq->flags & HDA_OWNED)) {

                ddbg_assert2(ioqueue_get_specific_request(currdisk->queue,tmpioreq) != NULL,
                        "ioreq_event not found");

                disk_interferestats(currdisk, tmpioreq);
            }

            evtfound = ioqueue_physical_access_done(currdisk->queue,tmpioreq);

            ddbg_assert2(evtfound != NULL, "ioreq_event not found");

            tmpioreq = tmpioreq->next;
        }
        if((currdisk->keeprequestdata == -1) && !currdiskreq->seg->diskreqlist->seg_next)
        {
            ddbg_assert2((currdiskreq->outblkno <= currdiskreq->seg->endblkno),
                    "Unable to erase request data from segment");

            currdiskreq->seg->startblkno = currdiskreq->outblkno;
        }


        if((currdiskreq == currdisk->effectivehda) || (currdiskreq == currdisk->currenthda))
        {
            if(currdiskreq->seg->access->type == NULL_EVENT) {
                disk_release_hda(currdisk,currdiskreq);
            }
            else {
                if(currdisk->preseeking != NO_PRESEEK) {
                    disk_check_prefetch_swap(currdisk);
                }
            }
        }
    }
    else {  			/* WRITE */
        if((currdiskreq == currdisk->effectivehda)
                || (currdiskreq == currdisk->currenthda))
        {
            if(currdiskreq->seg->access->type == NULL_EVENT) {
                disk_release_hda(currdisk,currdiskreq);
            }
        }
    }

    curr->time = simtime;
    curr->type = IO_INTERRUPT_ARRIVE;
    curr->cause = COMPLETION;

    if(currdisk->const_acctime) {
        delay = 0.0;
    }
    else {
        if(curr->flags & READ) {
            delay = currdisk->overhead_complete_read;
        }
        else {
            delay = currdisk->overhead_complete_write;
        }
    }

    disk_send_event_up_path(curr, (delay * currdisk->timescale));
    currdisk->outstate = DISK_WAIT_FOR_CONTROLLER;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_request_complete  Exiting\n", simtime );
    ioqueue_print_contents (currdisk->queue);
    fflush( outputfile );
#endif
}


static void disk_reconnection_or_transfer_complete (ioreq_event *curr)
{
    disk        *currdisk = getdisk(curr->devno);
    diskreq     *currdiskreq;
    ioreq_event *tmpioreq;
    double delay;

    disksim_inst_enter();

    currdiskreq = currdisk->effectivebus;
    if(!currdiskreq) {
        currdiskreq = currdisk->effectivebus = currdisk->currentbus;
    }

    ddbg_assert2(currdiskreq != 0, "effectivebus and currentbus are NULL");

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_reconnection_or_transfer_complete - currdiskreq %p, devno %d, type %d, cause %s (%d), blkno %d, bcount %d\n", simtime, currdiskreq, curr->devno, curr->type, iosim_decodeInterruptEvent(curr), curr->cause, curr->blkno, curr->bcount );
    fflush(outputfile);
#endif

    tmpioreq = currdiskreq->ioreqlist;
    while (tmpioreq) {
        if(tmpioreq->blkno == curr->blkno) {
            break;
        }
        tmpioreq = tmpioreq->next;
    }

    ddbg_assert2(tmpioreq != 0, "ioreq_event not found in effectivebus");

    switch (currdisk->outstate) {
    case DISK_WAIT_FOR_CONTROLLER:
    case DISK_TRANSFERING:
        break;
    default:
        ddbg_assert3(0, ("Disk not waiting to transfer - devno %d, state %d\n",
                curr->devno, currdisk->outstate));
        break;
    }

    currdisk->effectivebus = currdiskreq;
    currdisk->outstate = DISK_TRANSFERING;
    curr->type = DEVICE_DATA_TRANSFER_COMPLETE;
    curr = disk_buffer_transfer_size(currdisk, currdiskreq, curr);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_reconnection_or_transfer_complete - disk_buffer_transfer_size set bcount to %d\n", simtime, curr->bcount);
	dumpIOReq( "disk_reconnection_or_transfer_complete", curr );
    fflush(outputfile);
#endif

    if(curr->bcount == DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS) {
        ddbg_assert2(currdisk->outwait == 0, "non-NULL outwait found");
        currdisk->outwait = curr;
    }
    else if(curr->bcount == DISKCTLR_DISCONNECT_REQUESTED) {
        curr->type = IO_INTERRUPT_ARRIVE;
        curr->cause = DISCONNECT;
        if(tmpioreq->flags & READ) {
            delay = ((currdisk->lastflags & READ)
                    ? currdisk->overhead_disconnect_read_afterread
                            : currdisk->overhead_disconnect_read_afterwrite);
        }
        else {
            delay = ((currdiskreq->flags & EXTRA_WRITE_DISCONNECT)
                    ? currdisk->extradisc_disconnect2
                            : currdisk->overhead_disconnect_write);
        }

        disk_send_event_up_path(curr, (delay * currdisk->timescale));
        currdisk->outstate = DISK_WAIT_FOR_CONTROLLER;
    }
    else if(curr->bcount == DISKCTLR_TRANSFER_COMPLETED) {
	    disk_request_complete(currdisk, currdiskreq, curr);
    } 
    else if(curr->bcount > 0) {
        currdisk->starttrans = simtime;
        currdisk->blksdone = 0;
        disk_send_event_up_path(curr, (double) 0.0);
    }
}


/* If no SEG_OWNER exists, give seg to first read diskreq with HDA_OWNED but 
   not effectivehda, or first diskreq with BUFFER_APPEND, or effectivehda 
   if no such diskreqs exist (and the seg is appropriate)
 */

static void disk_find_new_seg_owner(disk *currdisk, segment *seg)
{
    diskreq *currdiskreq = seg->diskreqlist;
    diskreq *bestdiskreq = NULL;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_find_new_seg_owner\n", simtime );
    fflush(outputfile);
#endif

    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf (outputfile, "%12.6f            Entering disk_find_new_seg_owner for disk %d\n", simtime, currdisk->devno);
    //    fflush(outputfile);
    //  }

    ddbg_assert(seg != NULL);
    ddbg_assert(seg->recyclereq == NULL);

    while (currdiskreq) {
        if(currdiskreq->flags & SEG_OWNED) {
            return;
        }
        if(!bestdiskreq && (currdiskreq != currdisk->effectivehda) &&
                (currdiskreq->flags & HDA_OWNED) && (currdiskreq->ioreqlist) &&
                (currdiskreq->ioreqlist->flags & READ)) {
            bestdiskreq = currdiskreq;
        }
        currdiskreq = currdiskreq->seg_next;
    }

    if(!bestdiskreq) {
        currdiskreq = seg->diskreqlist;
        while (currdiskreq) {
            if(currdiskreq->hittype == BUFFER_APPEND) {
                bestdiskreq = currdiskreq;
                break;
            }
            currdiskreq = currdiskreq->seg_next;
        }
    }

    if(bestdiskreq) {
        disk_buffer_attempt_seg_ownership(currdisk,bestdiskreq);
    }
    else if(currdisk->effectivehda
            && (currdisk->effectivehda->seg == seg))
    {
        disk_buffer_attempt_seg_ownership(currdisk,currdisk->effectivehda);
    }
}


/*
 * If pure prefetch, release the hda and free the diskreq.
 * If active read, release the hda if preseeking level is appropriate.
 * If fast write, release the hda, remove the event(s) from the ioqueue,
 *   and, if COMPLETION_RECEIVED, free all structures.  If
 *   LIMITED_FASTWRITE and an appended request exists, make it the
 *   next effectivehda.
 * If slow write, call the completion routine and release the hda if
 *   preseeking level is appropriate.
 */

static void disk_release_hda(disk *currdisk, diskreq *currdiskreq)
{
    ioreq_event *tmpioreq;
    ioreq_event *holdioreq;
    segment *seg = currdiskreq->seg;
    diskreq *tmpdiskreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_release_hda  currdisk %p, currdiskreq %p\n", simtime, currdisk, currdiskreq );
    fflush(outputfile);
#endif

    diskreq *nextdiskreq = NULL;
    int release_hda = FALSE;
    int free_structs = FALSE;

    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf (outputfile, "%12.6f  %8p  Entering disk_release_hda for disk %d\n", simtime, currdiskreq,currdisk->devno);
    //    fflush(outputfile);
    //  }

    ddbg_assert(currdiskreq != NULL);

    ddbg_assert3((currdiskreq == currdisk->effectivehda),
            ("currdiskreq 0x%x, effectivehda 0x%x",
                    currdiskreq, currdisk->effectivehda));

    ddbg_assert(seg != NULL);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_release_hda  seg dump before\n", simtime );
    dump_disk_buffer_seqment ( outputfile, seg, -1 );
    fprintf (outputfile, "\n" );
    fflush(outputfile);
#endif

    if(!currdiskreq->ioreqlist) {
        ddbg_assert2((currdiskreq->flags & COMPLETION_RECEIVED),
                "COMPLETION_RECEIVED not flagged for pure prefetch");

        release_hda = TRUE;
        free_structs = TRUE;
        seg->state =
                ((currdisk->enablecache || seg->diskreqlist->seg_next)
                        ? BUFFER_CLEAN
                                : BUFFER_EMPTY);
    }
    else if(currdiskreq->ioreqlist->flags & READ) {
        if((currdisk->preseeking == PRESEEK_BEFORE_COMPLETION)
                || ((currdisk->preseeking == PRESEEK_DURING_COMPLETION)
                        && (currdiskreq->flags & COMPLETION_SENT)))
        {
            int cleanOrEmpty;
            release_hda = TRUE;

            cleanOrEmpty = !(currdiskreq->flags & COMPLETION_SENT)
	          ? BUFFER_CLEAN
	                  : BUFFER_EMPTY;

            seg->state = (disk_buffer_state_t)(currdisk->enablecache
                    || seg->diskreqlist->seg_next
                    || cleanOrEmpty);
        }
    }
    else if((currdiskreq->flags & COMPLETION_RECEIVED)
            || (currdisk->preseeking == PRESEEK_BEFORE_COMPLETION)
            || ((currdisk->preseeking == PRESEEK_DURING_COMPLETION)
                    && (currdiskreq->flags & COMPLETION_SENT)))
    {
        release_hda = TRUE;
        if(currdiskreq->flags & COMPLETION_RECEIVED) {
            free_structs = TRUE;
        }

        /* check to see if seg should be left DIRTY */

        tmpdiskreq = seg->diskreqlist;
        while (tmpdiskreq) {
            if(tmpdiskreq != currdiskreq) {
                tmpioreq = tmpdiskreq->ioreqlist;
                ddbg_assert(tmpioreq != NULL);
                if(!(tmpioreq->flags & READ)) {
                    while (tmpioreq->next) {
                        tmpioreq = tmpioreq->next;
                    }
                    if(tmpdiskreq->inblkno < (tmpioreq->blkno + tmpioreq->bcount)) {
                        seg->state = BUFFER_DIRTY;
                        if((tmpdiskreq->hittype == BUFFER_APPEND) && (currdisk->fastwrites == LIMITED_FASTWRITE)) {
                            tmpioreq = currdiskreq->ioreqlist;
                            ddbg_assert(tmpioreq != NULL);
                            // find the end of the list
                            while (tmpioreq->next) {
                                tmpioreq = tmpioreq->next;
                            }
                            if(tmpdiskreq->ioreqlist->blkno == (tmpioreq->blkno + tmpioreq->bcount))
                            {
                                nextdiskreq = tmpdiskreq;
                            }
                        }
                        break;
                    }
                }
            }
            tmpdiskreq = tmpdiskreq->seg_next;
        }

        if(!tmpdiskreq) {
            if(seg->diskreqlist->seg_next) {
                seg->state = BUFFER_CLEAN;
            }
            else {
                int clean = (currdisk->enablecache && currdisk->readhitsonwritedata);
                if(clean) {
                    seg->state = BUFFER_CLEAN;
                }
                else {
                    seg->state = BUFFER_EMPTY;
                }
            }
            currdisk->numdirty--;
            //	if((disk_printhack > 1) && (simtime >= disk_printhacktime))
            //	{
            //	    fprintf (outputfile, "                        numdirty-- = %d\n",currdisk->numdirty);
            //	    fflush(outputfile);
            //	}
            ddbg_assert3(((currdisk->numdirty >= 0)
                    && (currdisk->numdirty <= currdisk->numwritesegs)),
                    ("numdirty: %d", currdisk->numdirty));
        }
    }


    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf (outputfile, "                        free_structs = %d, release_hda = %d\n", free_structs, release_hda);
    //    fflush(outputfile);
    //  }

    if(free_structs) {
        disk_buffer_remove_from_seg(currdiskreq);
        tmpioreq = currdiskreq->ioreqlist;
        while (tmpioreq) {
            holdioreq = tmpioreq->next;
            addtoextraq((event *) tmpioreq);
            tmpioreq = holdioreq;
        }
        addtoextraq((event *) currdiskreq);
        if(seg->recyclereq) {
            /* I don't think the following code ever gets used... */
            fprintf(stderr,"GOT HERE!  SURPRISE!\n");

            ddbg_assert2(seg->diskreqlist == 0,
                    "non-NULL diskreqlist found for recycled seg");

            nextdiskreq = seg->recyclereq;
            disk_buffer_set_segment(currdisk,seg->recyclereq);
            disk_buffer_attempt_seg_ownership(currdisk,seg->recyclereq);
            seg->recyclereq = NULL;
        }
        else if(seg->diskreqlist) {
            disk_find_new_seg_owner(currdisk,seg);
        }
    }

    if(release_hda) {
        if(currdisk->currenthda == currdisk->effectivehda) {
            currdisk->currenthda = NULL;
        }
        currdisk->effectivehda = NULL;
        ddbg_assert2(seg->access->type == NULL_EVENT,
                "non-NULL seg->access->type found upon releasing hda");

        disk_check_hda(currdisk, nextdiskreq, TRUE);
    }
}


/* Set up an ioreq_event for a request if it needs bus service.  If
 *  data is ready for transfer, bcount is set to the amount.
 */

static ioreq_event *
disk_request_needs_bus(disk *currdisk,
        diskreq *currdiskreq,
        int check_watermark)
{
    ioreq_event *busioreq = NULL;
    ioreq_event *tmpioreq = currdiskreq->ioreqlist;
    diskreq     *seg_owner;
    segment     *seg = currdiskreq->seg;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_request_needs_bus Enter  currdisk %p, currdiskreq %p, check_watermark %d\n", simtime, currdisk, currdiskreq, check_watermark );
    fflush(outputfile);
#endif

    if((currdisk->outstate != DISK_IDLE) && !currdisk->outwait) {
        return 0;
    }

    if((currdiskreq->flags & FINAL_WRITE_RECONNECTION_1) &&
            !(currdiskreq->flags & FINAL_WRITE_RECONNECTION_2)) {
        return 0;
    }

    if(currdisk->const_acctime) {
        if((currdiskreq == currdisk->currenthda) &&
                (currdiskreq->overhead_done <= simtime)) {

            if(currdisk->outwait) {
                busioreq = currdisk->outwait;
                currdisk->outwait = NULL;
                busioreq->time = simtime;
            }
            else {
                busioreq = ioreq_copy(tmpioreq);
                busioreq->time = simtime;
                busioreq->type = IO_INTERRUPT_ARRIVE;
                busioreq->cause = RECONNECT;
            }

            busioreq->bcount = 0;
        }
        return busioreq;
    }

    if(seg
            && (currdiskreq != seg->recyclereq)
            && tmpioreq
            && !(currdiskreq->flags & COMPLETION_SENT))
    {
        while (tmpioreq && tmpioreq->next) {
            if(currdiskreq->outblkno < (tmpioreq->blkno + tmpioreq->bcount)) {
                break;
            }
            tmpioreq = tmpioreq->next;
        }
        if(currdiskreq->ioreqlist->flags & READ) {
            ddbg_assert2(currdiskreq->outblkno <
                    (tmpioreq->blkno + tmpioreq->bcount),
                    "read completion detected in disk_request_needs_bus");

            if((seg->endblkno > currdiskreq->outblkno)
                    && (seg->startblkno <= currdiskreq->outblkno)
                    && (!check_watermark
                            || (seg->endblkno >= (tmpioreq->blkno + tmpioreq->bcount))
                            || ((seg->endblkno - currdiskreq->outblkno) >=
                                    currdiskreq->watermark)))
            {
                if(currdisk->outwait) {
                    busioreq = currdisk->outwait;
                    currdisk->outwait = NULL;
                    busioreq->time = simtime;
                }
                else {
                    busioreq = ioreq_copy(tmpioreq);
                    busioreq->time = simtime;
                    busioreq->type = IO_INTERRUPT_ARRIVE;
                    busioreq->cause = RECONNECT;
                }
                busioreq->bcount = min(seg->endblkno,(tmpioreq->blkno + tmpioreq->bcount)) - currdiskreq->outblkno;
            }
        }
        else {			/* WRITE */
            seg_owner = disk_buffer_seg_owner(seg,FALSE);
            if(!seg_owner) {
                seg_owner = currdiskreq;
            }
            if((currdiskreq->inblkno >= (tmpioreq->blkno + tmpioreq->bcount))
                    || ((currdiskreq->outblkno == currdiskreq->ioreqlist->blkno)
                            && (currdiskreq->hittype != BUFFER_APPEND))
                            || ((seg->endblkno == currdiskreq->outblkno)
                                    && (seg->endblkno < (tmpioreq->blkno + tmpioreq->bcount))
                                    && ((seg->endblkno - seg_owner->inblkno) < seg->size)
                                    && (!check_watermark
                                            || ((seg->endblkno - currdiskreq->inblkno) <=
                                                    currdiskreq->watermark))))
            {
                if(currdisk->outwait) {
                    busioreq = currdisk->outwait;
                    currdisk->outwait = NULL;
                    busioreq->time = simtime;
                }
                else {
                    busioreq = ioreq_copy(tmpioreq);
                    busioreq->time = simtime;
                    busioreq->type = IO_INTERRUPT_ARRIVE;
                    busioreq->cause = RECONNECT;
                }
                if((currdiskreq->outblkno == currdiskreq->ioreqlist->blkno)
                        && (currdiskreq->hittype != BUFFER_APPEND))
                {
                    busioreq->bcount = min(tmpioreq->bcount,seg->size);
                }
                else {
                    int i1 = tmpioreq->blkno + tmpioreq->bcount -
                            currdiskreq->outblkno;

                    int i2 = seg->size - seg->endblkno + seg_owner->inblkno;

                    busioreq->bcount = min(i1, i2);
                }
            }
        }
    }

    return busioreq;
}


/*
 * Queue priority list:
 * 10  HDA_OWNED, !currenthda & !effectivehda (completions and full read hits)
 *  9		  effectivehda
 *  8		  currenthda
 *  7  Appending limited fastwrite (LIMITED_FASTWRITE and BUFFER_APPEND)
 *  6  Full sneakyintermediateread with seg
 *  5  Partial sneakyintermediateread with seg
 *  4  Full sneakyintermediateread currently without seg
 *  3  Partial sneakyintermediateread currently without seg
 *  2  Write prebuffer to effectivehda->seg
 *  1  Write prebuffer to currenthda->seg
 *  0  Write prebuffer with other seg
 *  -1 Write prebuffer currently without seg
 *
 * Not usable:
 *    requests which don't need bus service
 *    requests using recycled segs
 *    requests with no available segs
 *
 * If a sneakyintermediateread is detected which has a seg, was marked as
 * a read hit, has not transferred any data yet, and is no longer a hit,
 * remove the diskreq from the segment.
 *
 * Possible improvements:
 *   un-set segment if read hit is no longer a hit and no data has been
 *   transferred yet.
 *
 *   reverse queue order (oldest first) so that we don't have to peruse
 *   the entire queue if a 10 is found
 */

static diskreq *
disk_select_bus_request(disk *currdisk, ioreq_event **busioreq)
{
    diskreq *currdiskreq = currdisk->pendxfer;
    diskreq *bestdiskreq = NULL;
    ioreq_event *currioreq = 0;
    ioreq_event *tmpioreq;
    int curr_value;
    int best_value = -99;
    int curr_set_segment;
    int best_set_segment = FALSE;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_select_bus_request\n", simtime );
    fflush(outputfile);
#endif

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f         Entering disk_select_bus_request for disk %d\n", simtime, currdisk->devno);
        fflush(outputfile);
    }

    while (currdiskreq) {
        curr_value = -100;
        curr_set_segment = FALSE;
        ddbg_assert2(currdiskreq->ioreqlist != 0,
                "diskreq with NULL ioreqlist found in bus queue");

        if(currdiskreq->seg
                && (currdiskreq->seg->recyclereq == currdiskreq))
        {
            // dont do anything
        }
        else if(currdiskreq->flags & HDA_OWNED) {
            currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
            if(currioreq) {
                if(currdiskreq == currdisk->effectivehda) {
                    curr_value = 9;
                }
                else if(currdiskreq == currdisk->currenthda) {
                    curr_value = 8;
                }
                else {
                    curr_value = 10;
                }
            }
        }
        else if((best_value <= 7)
                && (currdisk->fastwrites == LIMITED_FASTWRITE)
                && (currdiskreq->hittype == BUFFER_APPEND))
        {
            currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
            if(currioreq) {
                curr_value = 7;
            }
        }
        else if((best_value <= 6)
                && (currdisk->sneakyintermediatereadhits)
                && (currdiskreq->ioreqlist->flags & READ))
        {
            if(currdiskreq->seg) {
                currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
                if(currioreq) {
                    tmpioreq = currdiskreq->ioreqlist;
                    while (tmpioreq->next) {
                        tmpioreq = tmpioreq->next;
                    }

                    curr_value = ((currdiskreq->seg->endblkno >= (tmpioreq->blkno + tmpioreq->bcount)) ? 6 : 5);
                }
                else if(currdiskreq->outblkno == currdiskreq->ioreqlist->blkno) {
                    /* Not entirely sure this works or is appropriate */
                    currioreq = disk_request_needs_bus(currdisk,currdiskreq,FALSE);
                    if(currioreq) {
                        addtoextraq((event *)currioreq);
                    }
                    else {


                        if(disk_printhack && (simtime >= disk_printhacktime)) {
                            fprintf (outputfile, "%12.6f         "
                                    "sneakyintermediatereadhits removed diskreq "
                                    "from seg\n",
                                    simtime);

                            fprintf (outputfile, "                       "
                                    "seg = %d-%d\n",
                                    currdiskreq->seg->startblkno,
                                    currdiskreq->seg->endblkno);

                            fprintf (outputfile, "                       "
                                    "diskreq = %d, %d, %d (1==R)\n",
                                    currdiskreq->ioreqlist->blkno,
                                    currdiskreq->ioreqlist->bcount,
                                    (currdiskreq->ioreqlist->flags & READ));

                            fflush(outputfile);
                        }



                        disk_buffer_remove_from_seg(currdiskreq);
                    }
                }
            }
            if(!currdiskreq->seg && (best_value <= 4)) {
                disk_buffer_select_segment(currdisk, currdiskreq, FALSE);
                if(currdiskreq->seg) {
                    if(currdiskreq->hittype == BUFFER_NOMATCH) {
                        currdiskreq->seg = NULL;
                    }
                    else {
                        if(currdisk->reqwater) {
                            currdiskreq->watermark = max(1, (int) ((double) min(currdiskreq->seg->size, currdiskreq->ioreqlist->bcount) * currdisk->readhighwatermark));
                        }
                        else {
                            currdiskreq->watermark = (int) (((double) currdiskreq->seg->size * currdisk->readhighwatermark) + (double) 0.9999999999);
                        }

                        currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
                        if(!currioreq) {
                            currdiskreq->seg = NULL;
                            currdiskreq->hittype = BUFFER_NOMATCH;
                        }
                        else {
                            curr_value = ((currdiskreq->hittype == BUFFER_WHOLE) ? 4 : 3);
                            curr_set_segment = TRUE;
                        }
                    }
                }
            }
        }
        else if((best_value <= 2)
                && (currdisk->writeprebuffering)
                && !(currdiskreq->ioreqlist->flags & READ))
        {
            if(currdiskreq->seg) {
                currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
                if(currioreq) {
                    tmpioreq = currdiskreq->ioreqlist;
                    while (tmpioreq->next) {
                        tmpioreq = tmpioreq->next;
                    }
                    if(currdisk->effectivehda &&
                            (currdisk->effectivehda->seg == currdiskreq->seg)) {
                        curr_value = 2;
                    }
                    else if(currdisk->currenthda &&
                            (currdisk->currenthda->seg == currdiskreq->seg)) {
                        curr_value = 1;
                    }
                    else {
                        curr_value = 0;
                    }
                }
            }
            else if(best_value <= -1) {
                disk_buffer_select_segment(currdisk,currdiskreq,FALSE);
                if(currdiskreq->seg) {
                    if(currdisk->reqwater) {
                        currdiskreq->watermark = max(1, (int) ((double) min(currdiskreq->seg->size, currdiskreq->ioreqlist->bcount) * currdisk->writelowwatermark));
                    }
                    else {
                        currdiskreq->watermark = (int) (((double) currdiskreq->seg->size * currdisk->writelowwatermark) + (double) 0.9999999999);
                    }
                    currioreq = disk_request_needs_bus(currdisk,currdiskreq,TRUE);
                    if(currioreq) {
                        curr_value = -1;
                        curr_set_segment = TRUE;
                    }
                    else {
                        currdiskreq->seg = NULL;
                        currdiskreq->hittype = BUFFER_NOMATCH;
                    }
                }
            }
        }

        if(curr_value >= best_value) {
            if(*busioreq) {
                addtoextraq((event *) *busioreq);
                if(best_set_segment) {
                    bestdiskreq->seg = NULL;
                    bestdiskreq->hittype = BUFFER_NOMATCH;
                }
            }
            best_value = curr_value;
            best_set_segment = curr_set_segment;
            bestdiskreq = currdiskreq;
            *busioreq = currioreq;
        } else if(curr_set_segment) {
            currdiskreq->seg = NULL;
            currdiskreq->hittype = BUFFER_NOMATCH;
        }

        currdiskreq = currdiskreq->bus_next;
    }

    if(best_set_segment) {
        disk_buffer_set_segment(currdisk,bestdiskreq);
    }

    return(bestdiskreq);
} // disk_select_bus_request()


/*
 * Attempt to start bus activity.  If currdiskreq doesn't match a
 * non-NULL effectivebus, do nothing.  Otherwise, if currdiskreq
 * doesn't match a non-NULL currentbus, do nothing.  Otherwise, if
 * there is already a request outstanding (buswait), do nothing.
 * Otherwise, check to see if currdiskreq needs some bus activity.  If
 * currdiskreq is NULL, attempt to find the appropriate next request
 * from the queue.
 *
 * Queue priority list:
 *    effectivebus (if non-NULL, this is the only choice possible)
 *    currentbus   (if non-NULL, this is the only choice possible)
 *    see disk_select_bus_request
 */

static void
disk_check_bus (disk *currdisk, diskreq *currdiskreq)
{
    diskreq     *nextdiskreq = currdiskreq;
    diskreq     *tmpdiskreq;
    ioreq_event *busioreq = NULL;
    double	delay;

    disksim_inst_enter();

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f disk_check_bus  Enter  currdisk %p, currdiskreq %p, disk %d, buswait %p\n", simtime, currdisk, currdiskreq,currdisk->devno, currdisk->buswait );
    fflush(outputfile);
#endif

    if(currdisk->buswait) {
        return;
    }

    /* add checking here for acctime >= 0.0 */

    if(currdiskreq) {
        if(currdisk->effectivebus) {
            if(currdisk->effectivebus != currdiskreq) {
                nextdiskreq = NULL;
            }
        }
        else if(currdisk->currentbus) {
            if(currdisk->currentbus != currdiskreq) {
                nextdiskreq = NULL;
            }
        }
        if(nextdiskreq) {

            /* Only truly affected call is the one from disk_check_hda.  It
             * needs to be TRUE in order to match disk_completion's call.
             * Otherwise, preseeking is disadvantaged...
             */
            busioreq = disk_request_needs_bus(currdisk,nextdiskreq,TRUE);
            if(!busioreq) {
                nextdiskreq = NULL;
            }
        }
    } 
    else {
        if(currdisk->effectivebus) {
            busioreq = disk_request_needs_bus(currdisk,currdisk->effectivebus,TRUE);
            if(busioreq) {
                nextdiskreq = currdisk->effectivebus;
            }
        }
        else if(currdisk->currentbus) {
            busioreq = disk_request_needs_bus(currdisk,currdisk->currentbus,TRUE);
            if(busioreq) {
                nextdiskreq = currdisk->currentbus;
            }
        }
        else {
            nextdiskreq = disk_select_bus_request(currdisk,&busioreq);
        }
    }

    if(nextdiskreq) {
        if(disk_printhack && (simtime >= disk_printhacktime)) {
            fprintf (outputfile, "                        nextdiskreq = %8p\n", nextdiskreq);
            fflush(outputfile);
        }

        if((nextdiskreq != currdisk->currentbus)
                && (nextdiskreq != currdisk->effectivebus))
        {
            /* remove nextdiskreq from bus queue */
            if(currdisk->pendxfer == nextdiskreq) {
                currdisk->pendxfer = nextdiskreq->bus_next;
            }
            else {
                tmpdiskreq = currdisk->pendxfer;
                while (tmpdiskreq && (tmpdiskreq->bus_next != nextdiskreq)) {
                    tmpdiskreq = tmpdiskreq->bus_next;
                }

                ddbg_assert2(tmpdiskreq != 0,
                        "Next bus request not found in bus queue");

                tmpdiskreq->bus_next = nextdiskreq->bus_next;
            }
            nextdiskreq->bus_next = NULL;
        }

        currdisk->currentbus = currdisk->effectivebus = nextdiskreq;

        if(busioreq->cause == RECONNECT) {
            if(nextdiskreq->ioreqlist->flags & READ) {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_bus  Calling disk_activate_read\n", simtime );
    fflush(outputfile);
#endif
                disk_activate_read(currdisk, nextdiskreq, FALSE, FALSE);
            }
            else {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_bus  Calling disk_activate_write\n", simtime );
    fflush(outputfile);
#endif
                disk_activate_write(currdisk, nextdiskreq, FALSE, FALSE);
            }

            if(currdisk->extradisc_diskreq == nextdiskreq) {
                currdisk->extradisc_diskreq = NULL;
                delay = currdisk->overhead_reselect_first;
            }
            else {
                delay = ((nextdiskreq->flags & EXTRA_WRITE_DISCONNECT) ? currdisk->overhead_reselect_other : currdisk->overhead_reselect_first);
            }

            if(busioreq->bcount > 0) {
                delay += currdisk->overhead_data_prep;
            }

            if(currdisk->const_acctime) {
                delay = 0.0;
            }
            else {
                nextdiskreq->seg->outstate = BUFFER_CONTACTING;
            }

            currdisk->outstate = DISK_WAIT_FOR_CONTROLLER;
            disk_send_event_up_path(busioreq, (delay * currdisk->timescale));
        }

        else if(busioreq->cause == DEVICE_DATA_TRANSFER_COMPLETE) {
            disk_reconnection_or_transfer_complete(busioreq);
        }
        else {
            ddbg_assert2(0, "unexpected busioreq->cause in disk_check_bus");
        }
    }
} // disk_check_bus()


static void
disk_get_remapped_sector(disk *currdisk, ioreq_event *curr)
{
    double acctime;
    segment *seg;
    diskreq *currdiskreq = currdisk->effectivehda;

    ddbg_assert2(currdiskreq != 0, "No effectivehda");

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_remapped_sector\n", simtime );
    fflush(outputfile);
#endif

    seg = currdiskreq->seg;

    // new ltop and acctime
    {
        uint64_t nsecs;
        struct dm_pbn pbn;

        currdisk->model->layout->dm_translate_ltop(currdisk->model, curr->blkno, MAP_FULL, &pbn, 0);
        curr->cause = pbn.sector;


        // acctime = diskacctime(currdisk,
        //		  curr->tempptr1,
        //		  DISKACCTIME,
        //		  (curr->flags & READ),
        //		  simtime,
        //		  global_currcylno,
        //		  global_currsurface,
        //		  curr->cause,
        //		  1,
        //		  0);

        nsecs = currdisk->model->mech->dm_acctime(
                currdisk->model,
                &currdisk->mech_state,
                &pbn,
                1,   // len
                (curr->flags & READ),
                0,   // immed
                &currdisk->mech_state,
                0); // breakdown
        acctime = dm_time_itod(nsecs);
    }


    curr->time = simtime + acctime;
    if(currdisk->stat.latency == (double) -1.0) {
        currdisk->stat.latency = (double) 0.0;
        currdisk->stat.xfertime = acctime - currdisk->stat.seektime;
    }
    else {
        currdisk->stat.xfertime += acctime;
    }

    if(((currdiskreq != seg->recyclereq)
            && disk_buffer_block_available(currdisk, seg, curr->blkno))
            || ((curr->flags & READ) && (!currdisk->read_direct_to_buffer)))
    {
        curr->type = DEVICE_GOT_REMAPPED_SECTOR;
    } 
    else {

        // get s/t for curr->blkno
        int st = currdisk->model->layout->dm_get_sectors_lbn(currdisk->model, curr->blkno);

        double rottime = currdisk->model->mech->dm_period(currdisk->model);

        // what is this? (appears again in disk_goto_remapped_sector)
        curr->time -= ((double) 1 / ((double)st * rottime));
        curr->type = DEVICE_GOTO_REMAPPED_SECTOR;
    }
} // disk_get_remapped_sector()



static void disk_goto_remapped_sector (disk *currdisk, ioreq_event *curr)
{
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_goto_remapped_sector\n", simtime );
    fflush(outputfile);
#endif

    double rotatetime =
            dm_time_itod(currdisk->model->mech->dm_period(currdisk->model));

    diskreq *currdiskreq = currdisk->effectivehda;
    segment *seg;

    ddbg_assert2(currdiskreq != 0, "No effectivehda");

    seg = currdiskreq->seg;
    if(seg->outstate == BUFFER_PREEMPT) {
        disk_release_hda(currdisk, currdiskreq);
        return;
    }

    if((currdiskreq != seg->recyclereq)
            && disk_buffer_block_available(currdisk, seg, curr->blkno))
    {
        // XXX is curr->blkno the track we want??
        int st = currdisk->model->layout->dm_get_sectors_lbn(currdisk->model, curr->blkno);
        double rottime = currdisk->model->mech->dm_period(currdisk->model);

        // what is this?
        curr->time += ((double) 1 / ((double)st * rottime));
        curr->type = DEVICE_GOT_REMAPPED_SECTOR;
    }
    else {
        seg->time += rotatetime;
        currdisk->stat.xfertime += rotatetime;
        curr->time += rotatetime;
        curr->type = DEVICE_GOTO_REMAPPED_SECTOR;
    }
    addtointq((event *) curr);
} // disk_goto_remapped_sector()



static int
disk_initiate_seek (
        disk *currdisk,
        segment *seg,
        ioreq_event *curr,
        int firstseek,
        double delay,
        double mintime)
{
    struct dm_pbn destpbn;
    double seektime;
    int seekdist;
    int remapsector = 0;

    //   printf("%s() currcause %d blkno %d\n",
    // 	 __func__,
    //   	 curr->cause,
    // 	 curr->blkno);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "\n%f: disk_initiate_seek to devno %d, blkno %d, bcount %d, flags %x\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags);
    dump_disk_buffer_seqment ( outputfile, seg, -1 );
    fprintf (outputfile, "\n" );
    fflush(outputfile);
#endif

    double metmp;
    double rotationTimeMsec = (double)currdisk->model->mech->dm_period(currdisk->model)/1000000000.0;
    double realAngle = modf(simtime/rotationTimeMsec, &metmp) * 360.0;
    struct dm_mech_acctimes mech_acctimes;

    curr->type = NULL_EVENT;
    seg->time = simtime + delay;
    //  remapsector = FALSE;

    // new ltop
    {
        int rv;
        uint64_t nsecs;
        struct dm_mech_state end;
        struct dm_mech_state begin = currdisk->mech_state;

        rv = currdisk->model->layout->dm_translate_ltop(
                currdisk->model,
                curr->blkno,
                MAP_FULL,
                &destpbn,
                &remapsector);
        ddbg_assert(rv == DM_OK);

        simresult_update_mechanicalstate( curr->tagID, begin, realAngle, destpbn);
        disk_calc_rpo_time( currdisk, curr, &mech_acctimes );
        simresult_update_mech_acctimes( curr->tagID, &mech_acctimes );

        seekdist = abs(currdisk->mech_state.cyl - destpbn.cyl);
        end.cyl = destpbn.cyl;
        end.head = destpbn.head;
        end.theta = 0;

        curr->cause = destpbn.sector;

        // new acctime
        //  seektime = diskacctime(currdisk, curr->tempptr1, DISKSEEKTIME,
        //		 (curr->flags & READ), seg->time, global_currcylno,
        //		 global_currsurface, curr->cause, curr->bcount, 0);


        nsecs = currdisk->model->mech->dm_seek_time(
                currdisk->model,
                &begin,
                &end,
                (curr->flags & READ));
        seektime = dm_time_itod(nsecs);
    }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_initiate_seek  devno %d, blkno %d, bcount %d, flags %x, currCyl %d, currHead %d, currAngle %u, destCyl %d, destHead %d, destSector %d, distance %d, seektime %f\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, currdisk->mech_state.cyl, currdisk->mech_state.head, currdisk->mech_state.theta, destpbn.cyl, destpbn.head, destpbn.sector, seekdist, seektime );
    fflush(outputfile);
#endif

#ifdef DISKLOG_ON
    if( NULL != DISKSIM_DISK_LOG )
    {
        fprintf( DISKSIM_DISK_LOG, "\n%f %d %d %d %d (%x)   currCyl %d, currHead %d, currAngle %u, rotationTimeMSec %f, realAngle %f, destCyl %d, destHead %d, destSector %d, seekdist %d, seektime %f",
                                simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, curr->flags,
                                currdisk->mech_state.cyl, currdisk->mech_state.head, currdisk->mech_state.theta, rotationTimeMsec, realAngle, destpbn.cyl, destpbn.head, destpbn.sector, seekdist, seektime  );
        fflush( DISKSIM_DISK_LOG );
    }
#endif

    if(seektime < mintime) {
        seg->time += mintime - seektime;
    }

    curr->time = seg->time + seektime;
    if((curr->flags & BUFFER_BACKGROUND)
            && (curr->flags & READ))
    {
        if((currdisk->contread == BUFFER_READ_UNTIL_CYL_END)
                && (seekdist != 0))
        {
            return FALSE;
        }

        if((currdisk->contread == BUFFER_READ_UNTIL_TRACK_END)
                && ((seekdist != 0) |
                        (currdisk->mech_state.head != destpbn.head)))
        {
            return FALSE;
        }
    }

    if(firstseek) {
        currdisk->stat.seekdistance = seekdist;
        currdisk->stat.seektime = seektime;
        currdisk->stat.latency = (double) -1.0;
        currdisk->stat.xfertime = (double) -1.0;
    }
    else if(remapsector == 0) {
        currdisk->stat.xfertime += seektime;
    }
    curr->type = DEVICE_BUFFER_SEEKDONE;
    if(remapsector != 0) {
        disk_get_remapped_sector(currdisk, curr);
    }
    addtointq((event *) curr);

#if 0
    if( NULL != DISKSIM_DISK_LOG )
    {
        fprintf (DISKSIM_DISK_LOG, ", latency %f, xfertime %f\n", currdisk->stat.latency, currdisk->stat.xfertime );
        fflush( DISKSIM_DISK_LOG );
    }
#endif

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_initiate_seek  Stats seekdistance %d, seektime %f, latency %f, xfertime %f\n", simtime, currdisk->stat.seekdistance, currdisk->stat.seektime, currdisk->stat.latency, currdisk->stat.xfertime );
    fflush(outputfile);
#endif

    return TRUE;
} // disk_initiate_seek()


/* Attempt to start hda activity.  
 * 1.  If effectivehda already set, do nothing.
 * 2.  If currdiskreq is NULL, attempt to find the appropriate next
 * request from the queue.  
 */

static void
disk_get_effectivehda(disk *currdisk,
        diskreq *currdiskreq)
{
    diskreq     *nextdiskreq = currdiskreq;
    ioreq_event *nextioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_get_effectivehda  Enter  effectivehda = %p\n", simtime, currdisk->effectivehda );
    fflush(outputfile);
#endif

    // if we have a currenthda, make that the effectivehda
    if(currdisk->currenthda) {
        if(currdiskreq && (currdiskreq != currdisk->currenthda)) {
            fprintf(stderr, "currdiskreq != currenthda in disk_check_hda\n");
            assert(0);
        }
        currdisk->effectivehda = currdisk->currenthda;
    }

    // No currenthda.  Let's find a "next" io request
    // (nextdiskreq) and make that the current/effective hda.
    else {
        // here, we don't have a currenthda
        if(nextdiskreq && nextdiskreq->ioreqlist) {
            nextioreq = ioreq_copy(nextdiskreq->ioreqlist);
            nextioreq->ioreq_hold_diskreq = nextdiskreq;
            nextioreq->ioreq_hold_disk = currdisk;
            if(!disk_enablement_function(nextioreq)) {
                nextdiskreq = NULL;
            }
            addtoextraq((event *)nextioreq);
        }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_get_effectivehda  Checking if we have a nextdiskreq....%s\n", simtime, nextdiskreq != NULL ? "yes" : "no" );
    fflush( outputfile );
#endif

        // we don't have a nextdiskreq yet.  check the disk's ioqueue for
        // one.
        if( !nextdiskreq ) {
            nextioreq = ioqueue_show_next_request(currdisk->queue);
            if(nextioreq) {
                nextdiskreq = (diskreq*) nextioreq->ioreq_hold_diskreq;
            }
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_get_effectivehda  Checking if we have a ioqueue entry....%s\n", simtime, nextdiskreq != NULL ? "yes" : "no" );
    fflush( outputfile );
#endif
        }

        // if we don't have a nextdiskreq by now, we're going to fall out
        // of this without having figured out an effectivehda and bail
        // back in disk_check_hda()
        if(nextdiskreq) {
            if(nextdiskreq->flags & HDA_OWNED) {
                fprintf(stderr, "disk_check_hda:"
                        "  diskreq was already HDA_OWNED\n");
                assert(0);
            }

            // does this request have a disk cache segment associated with it?
            if(nextdiskreq->seg &&
                   !(nextdiskreq->flags & SEG_OWNED) &&
                   (nextdiskreq->seg->recyclereq != nextdiskreq))
            {
                disk_buffer_attempt_seg_ownership(currdisk, nextdiskreq);
            }


            if(!nextdiskreq->seg) {
                // hmmm ... no cache segment ... let's try to find one
                disk_buffer_select_segment(currdisk,nextdiskreq,FALSE);
                if(nextdiskreq->seg) {
                    disk_buffer_set_segment(currdisk, nextdiskreq);
                    disk_buffer_attempt_seg_ownership(currdisk, nextdiskreq);
                }
                else {
                    // still no cache segment.  try to "recycle" one.
                    nextdiskreq->seg =
                            disk_buffer_recyclable_segment(currdisk,
                                    (nextdiskreq->ioreqlist->flags & READ));
                    if(nextdiskreq->seg) {
                        nextdiskreq->seg->recyclereq = nextdiskreq;
                    }

                } // else { // nextdiskreq->seg == 0
            } // if(!nextdiskreq->seg)

            // So if we succeeded in finding a cache segment above,
            // do something with the ioreqs on the nextdiskreq->ioreqlist.
            if(nextdiskreq->seg) {
                nextioreq = nextdiskreq->ioreqlist;
                while(nextioreq) {
                    if(!ioqueue_get_specific_request(currdisk->queue,
                            nextioreq))
                    {
                        fprintf(stderr, "disk_get_effectivehda()"
                                "  ioreq_event not found by"
                                " ioqueue_get_specific_request call\n");
                        assert(0);
                    }

                    nextioreq = nextioreq->next;
                }
            } // if(nextdiskreq->seg)
            // what if we didn't find a segment?

            currdisk->currenthda = currdisk->effectivehda = nextdiskreq;
            nextdiskreq->flags |= HDA_OWNED;

        } // if(nextdiskreq)
    } // else { // currdisk->currenthda == 0

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "                        "
                "effectivehda = %8p\n", currdisk->effectivehda);
        fflush(outputfile);
    }

#ifdef DEBUG_DISKCTLR
    if( NULL != currdisk->effectivehda )
    {
        dumpIOReq( "disk_get_effectivehda: currdisk->effectivehda->ioreqlist", currdisk->effectivehda->ioreqlist );
    }
    fprintf (outputfile, "********** %f: disk_get_effectivehda  Exit  effectivehda = %p\n", simtime, currdisk->effectivehda );
    fflush(outputfile);
#endif
} // disk_get_effectivehda()




// handle the case for disk_check_hda when requests are pending
// (i.e. not pure prefetch)
// ok_to_check_bus set will attempt to start bus activity
static void 
disk_check_hda_pending(disk *currdisk, 
        ioreq_event *nextioreq,
        diskreq *nextdiskreq,
        segment *seg,
        int ok_to_check_bus,
        double *mintime,
        int *immediate_release,
        int *initiate_seek)
{
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_hda_pending: Enter\n", simtime );
    fprintf (outputfile, "%f:     currdisk          %p\n", simtime, currdisk );
    fprintf (outputfile, "%f:     nextioreq         %p\n", simtime, nextioreq );
    fprintf (outputfile, "%f:     nextdiskreq       %p\n", simtime, nextdiskreq );
    fprintf (outputfile, "%f:     seg               %p\n", simtime, seg );
    fprintf (outputfile, "%f:     ok_to_check_bus   %d\n", simtime, ok_to_check_bus );
    fprintf (outputfile, "%f:     mintime           %f\n", simtime, *mintime );
    fprintf (outputfile, "%f:     immediate_release %d\n", simtime, *immediate_release );
    fprintf (outputfile, "%f:     initiate_seek     %d\n", simtime, *initiate_seek );
    fflush(outputfile);
#endif

    disksim_inst_enter();

    // get the last ioreq
    nextioreq = nextdiskreq->ioreqlist;
    while (nextioreq->next) {
        nextioreq = nextioreq->next;
    }
    seg->access->flags = nextioreq->flags;

    // is the request a READ?
    if(nextioreq->flags & READ) {
        if(seg->access->type != NULL_EVENT) {
            /*
             *	if((seg->access->blkno != seg->endblkno) &&
             *	(seg->access->blkno != (seg->endblkno + 1)) &&
             *	(seg->access->blkno !=
             *	currdisk->firstblkontrack)) { fprintf(stderr,
             *	"non-NULL seg->access->type with incorrect
             *	seg->access->blkno found for read segment in
             *	disk_check_hda\n"); exit(1); }
             */
        }
        else {
            // here's our bug ... we have some bogus seg that ends on
            // the 1-past-the-end-of-the-disk block and we try
            // to access it here
            seg->access->blkno = seg->endblkno;
        }

        if(!seg->recyclereq) {
            seg->minreadaheadblkno =
                    max(seg->minreadaheadblkno,
                            min((nextioreq->blkno +
                                    nextioreq->bcount +
                                    currdisk->minreadahead),
                                    currdisk->model->dm_sectors));

            seg->maxreadaheadblkno =
                    max(seg->maxreadaheadblkno,
                            min(disk_buffer_get_max_readahead(currdisk,seg,nextioreq),
                                    currdisk->model->dm_sectors));
        }

        currdisk->immed = currdisk->immedread;
        seg->state = BUFFER_READING;
        if(seg->recyclereq) {
            seg->access->blkno = nextdiskreq->outblkno;
            *initiate_seek = TRUE;
        }
        else if((seg->endblkno < (nextioreq->blkno +
                nextioreq->bcount))
                || (seg->startblkno > nextdiskreq->outblkno))
        {
            *initiate_seek = TRUE;
        }
        else if(seg->endblkno <
                max(seg->maxreadaheadblkno,
                        disk_buffer_get_max_readahead(currdisk,
                                seg,
                                nextioreq)))
        {
            seg->access->flags |= BUFFER_BACKGROUND;
            if(currdisk->readaheadifidle) {
                /* Check for type != NULL_EVENT below negates this */
                *initiate_seek = TRUE;
            }
        } // else if(seg->endblkno < ...
        else {
            *immediate_release = TRUE;
        }

        if(seg->access->type != NULL_EVENT) {
            if(*immediate_release) {
                fprintf(stderr,"CHECK:  immediate_release reset!\n");
                fflush(stderr);
            }

            *initiate_seek = FALSE;
            *immediate_release = FALSE;
        } // if(seg->access->type != NULL_EVENT)
    } // deal with a READ req

    // ... or is the request a WRITE?
    else {
        if(seg->access->type != NULL_EVENT) {
            fprintf(stderr, "non-NULL seg->access->type found"
                    " for write segment in disk_check_hda\n");
            assert(0);
        }

        /* prepare minimum seek time for seek if not sequential writes */
        if((nextdiskreq->hittype != BUFFER_APPEND)
                || (seg->access->flags & READ)
                || (seg->access->blkno != nextdiskreq->ioreqlist->blkno)
                || (seg->access->bcount != 1)
                || (seg->access->time != simtime))
        {
            *mintime = currdisk->minimum_seek_delay;
        }
        /*
         * else { fprintf(stderr,"Sequential write stream detected
         * at %f\n", simtime); }
         */

        seg->access->blkno = nextdiskreq->inblkno;
        if(nextdiskreq->flags & COMPLETION_RECEIVED) {
            seg->access->flags |= BUFFER_BACKGROUND;
        }
        currdisk->immed = currdisk->immedwrite;
        if((seg->state != BUFFER_WRITING) && (seg->state != BUFFER_DIRTY))
        {
            currdisk->numdirty++;

            if((disk_printhack > 1)
                    && (simtime >= disk_printhacktime))
            {
                fprintf (outputfile, "                        numdirty++ = %d\n",currdisk->numdirty);
                fflush(outputfile);
            }

            assert(currdisk->numdirty >= 0);
            assert(currdisk->numdirty <= currdisk->numwritesegs);

        }

        seg->state = BUFFER_WRITING;
        if(nextdiskreq->inblkno < (nextioreq->blkno + nextioreq->bcount))
        {
            *initiate_seek = TRUE;
        }
        else {
            *immediate_release = TRUE;
        }
    }

    if(ok_to_check_bus
            && (currdisk->extradisc_diskreq != nextdiskreq))
    {
        disk_check_bus(currdisk,nextdiskreq);
    }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_check_hda_pending: Exit\n", simtime );
    fprintf (outputfile, "********** %f:     currdisk          %p\n", simtime, currdisk );
    fprintf (outputfile, "********** %f:     nextioreq         %p\n", simtime, nextioreq );
    fprintf (outputfile, "********** %f:     nextdiskreq       %p\n", simtime, nextdiskreq );
    fprintf (outputfile, "********** %f:     seg               %p\n", simtime, seg );
    fprintf (outputfile, "********** %f:     ok_to_check_bus   %d\n", simtime, ok_to_check_bus );
    fprintf (outputfile, "********** %f:     mintime           %f\n", simtime, *mintime );
    fprintf (outputfile, "********** %f:     immediate_release %d\n", simtime, *immediate_release );
    fprintf (outputfile, "********** %f:     initiate_seek     %d\n", simtime, *initiate_seek );
    fflush(outputfile);
#endif
}



/* Attempt to start hda activity.
 * 1.  If effectivehda already set, do nothing.
 * 2.  If currdiskreq is NULL, attempt to find the appropriate next request from the queue.
 * ok_to_check_bus set will attempt to start bus activity
 */
static void disk_check_hda(disk *currdisk,
        diskreq *currdiskreq,
        int ok_to_check_bus)
{
    segment     *seg;
    diskreq     *nextdiskreq = 0;
    ioreq_event *nextioreq = 0;
    int          initiate_seek = FALSE;
    int          immediate_release = FALSE;
    double       delay = 0.0;
    double       mintime = 0.0;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_hda  Enter  currdisk %p, currdiskreq %p, ok_to_check_bus %d\n", simtime, currdisk, currdiskreq, ok_to_check_bus );
    fflush(outputfile);
#endif

    disksim_inst_enter();

    // XXX set this dynamically according to the track bounds of the
    // dest track.
    //  currdisk->fpcheck = 6000;
    //      currdisk->model->layout->dm_get_sectors_lbn(currdisk->model,
    //						  currdiskreq->inblkno);

#ifdef DEBUG_DISKCTLR
    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_check_hda"
                " for disk %d\n", simtime, currdiskreq,currdisk->devno);
        fflush(outputfile);
    }
#endif

    if(currdisk->const_acctime) {
        // SIMPLEDISK device path
        return;
    }

    // DISK device path
    // 1.  If effectivehda already set, do nothing.
    if( currdisk->effectivehda ) {
        return;
    }
    else {
        // try to get an effectivehda
        disk_get_effectivehda(currdisk, currdiskreq);
        if(!currdisk->effectivehda) {
            return;
        }
    }


    nextdiskreq = currdisk->effectivehda;
    // oops.  the above code doesn't guarantee that seg is valid!
    assert(nextdiskreq->seg != 0);
    seg = nextdiskreq->seg;

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_check_hda  nextdiskreq->ioreqlist", nextdiskreq->ioreqlist );
    fflush(outputfile);
#endif

    if(nextdiskreq->ioreqlist) {
#ifdef DEBUG_DISKCTLR
        fprintf (outputfile, "%f: disk_check_hda  PENDING REQUEST??\n", simtime);
        fflush(outputfile);
#endif

        disk_check_hda_pending(currdisk,
                nextioreq,
                nextdiskreq,
                seg,
                ok_to_check_bus,
                &mintime,
                &immediate_release,
                &initiate_seek);
    }
    else {
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_hda  PREFETCHING??\n", simtime);
    fflush(outputfile);
#endif
        // there aren't any requests so we're only prefetching
        seg->state = BUFFER_READING;
        if(seg->endblkno < seg->maxreadaheadblkno) {
            if(seg->access->type != NULL_EVENT) {
                /*
                 * if((seg->access->blkno != seg->endblkno) &&
                 * (seg->access->blkno != (seg->endblkno + 1)) &&
                 * (seg->access->blkno != currdisk->firstblkontrack)) {
                 * fprintf(stderr, "non-NULL seg->access->type with
                 * incorrect seg->access->blkno found for pure read
                 * segment in disk_check_hda\n"); exit(1); }
                 */
            }
            else {
                seg->access->blkno = seg->endblkno;
            }

            seg->access->flags = READ | BUFFER_BACKGROUND;
            currdisk->immed = currdisk->immedread;

            if(seg->access->type == NULL_EVENT) {
                initiate_seek = TRUE;
            }
        }
        else {
            immediate_release = TRUE;
        }
    } // pure prefetch case

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_hda  initiate_seek %d, immediate_release %d\n", simtime, initiate_seek, immediate_release);
    fflush(outputfile);
#endif


    if(initiate_seek) {
        if(nextdiskreq->overhead_done > simtime) {
            delay += nextdiskreq->overhead_done - simtime;
        }

        if( DISK_SEEK_STOPTIME > simtime) {
            delay += DISK_SEEK_STOPTIME - simtime;
        }

        if(!disk_initiate_seek(currdisk,
                seg,
                seg->access,
                TRUE,
                delay,
                mintime))
        {
            disk_release_hda(currdisk, nextdiskreq);
        }
    }
    else if(immediate_release) {
        disk_release_hda(currdisk, nextdiskreq);
    }

    if(!immediate_release
            && (seg->state == BUFFER_READING)
            && (seg->access->flags & BUFFER_BACKGROUND))
    {
        disk_check_prefetch_swap(currdisk);
    }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_check_hda  Exit   currdisk %p, currdiskreq %p, ok_to_check_bus %d\n", simtime, currdisk, currdiskreq, ok_to_check_bus );
    fflush(outputfile);
#endif
} // static void disk_check_hda()






/* prematurely stop a prefetch */

static void disk_buffer_stop_access (disk *currdisk)
{
    diskreq* effective = currdisk->effectivehda;
    segment* seg;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_stop_access\n", simtime );
    fflush(outputfile);
#endif

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_buffer_stop_access\n", simtime, effective);
        fflush(outputfile);
    }

    ddbg_assert2(effective != 0, "disk has NULL effectivehda");

    ddbg_assert(effective == currdisk->currenthda);

    seg = effective->seg;

    ddbg_assert((seg->access->flags & BUFFER_BACKGROUND) &&
            (seg->access->flags & READ));

    if(seg->access->type != NULL_EVENT) {
        int rv = removefromintq((event *)seg->access);
        ddbg_assert3(rv != FALSE,
                ("Non-null seg->access does not appear on the intq %d\n",
                        seg->access->type));

        seg->access->type = NULL_EVENT;
    }

    seg->state = ((!(effective->flags & COMPLETION_RECEIVED)
            || currdisk->enablecache
            || seg->diskreqlist->seg_next) ? BUFFER_CLEAN : BUFFER_EMPTY);

    if(effective->flags & COMPLETION_RECEIVED) {
        ddbg_assert2(effective->ioreqlist == 0,
                "Pure prefetch with non-NULL ioreqlist detected");

        disk_buffer_remove_from_seg(effective);
        addtoextraq((event *) effective);
        if(seg->diskreqlist) {
            disk_find_new_seg_owner(currdisk,seg);
        }
    }

    currdisk->effectivehda = currdisk->currenthda = NULL;
}


/* attempt to take over control of the hda from an ongoing prefetch */

static int 
disk_buffer_attempt_access_swap(disk *currdisk, 
        diskreq *currdiskreq)
{
    diskreq     *effective = currdisk->effectivehda;
    ioreq_event *currioreq;
    segment     *seg;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_attempt_access_swap\n", simtime );
    fflush(outputfile);
#endif

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_buffer_attempt_access_swap\n", simtime, currdiskreq);
        fprintf (outputfile, "                        trying to swap %8p\n", effective);
        fflush(outputfile);
    }

    ddbg_assert2(effective != 0, "disk has NULL effectivehda");

    seg = effective->seg;

    ddbg_assert(seg->access->flags & READ);
    ddbg_assert(currdiskreq->ioreqlist != NULL);

    /* Since "free" reads are not allowed to prefetch currently, we don't
     really need the first clause.  But, better safe than sorry...
     */

    if((effective == currdisk->currenthda)
            && (seg->access->flags & BUFFER_BACKGROUND)
            && (!SWAP_FORWARD_ONLY
                    || !effective->ioreqlist
                    || (effective->ioreqlist->blkno < currdiskreq->ioreqlist->blkno)))
    {
        if(effective->flags & COMPLETION_RECEIVED) {
            ddbg_assert(effective->ioreqlist == NULL);
            disk_buffer_remove_from_seg(effective);
            addtoextraq((event *) effective);
        }
        else {
            ddbg_assert(effective->ioreqlist != NULL);
            currioreq = currdiskreq->ioreqlist;
            ddbg_assert(currioreq != NULL);
            while (currioreq->next) {
                currioreq = currioreq->next;
            }
            seg->access->flags = currioreq->flags;

            seg->minreadaheadblkno = max(seg->minreadaheadblkno, min((currioreq->blkno + currioreq->bcount + currdisk->minreadahead), currdisk->model->dm_sectors));

            seg->maxreadaheadblkno = max(seg->maxreadaheadblkno, min(disk_buffer_get_max_readahead(currdisk,seg,currioreq), currdisk->model->dm_sectors));

            if((seg->endblkno >= (currioreq->blkno + currioreq->bcount))
                    && (seg->endblkno < seg->maxreadaheadblkno))
            {
                seg->access->flags |= BUFFER_BACKGROUND;
            }
        }
        currdisk->effectivehda = currdisk->currenthda = NULL;

        if(disk_printhack && (simtime >= disk_printhacktime))
        {
            fprintf (outputfile, "                        Swap successful\n");
            fflush(outputfile);
        }

        return(TRUE);
    }

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "                        Swap unsuccessful\n");
        fflush(outputfile);
    }

    return(FALSE);
}


/* setseg indicates that currdiskreq->seg has not been permanently
 * "set", and should be nullified if the hda cannot be obtained or
 * set if it is obtained 
 */

static void 
disk_activate_read(disk *currdisk,
                   diskreq *currdiskreq,
                   int setseg,
                   int ok_to_check_bus)
{
    ioreq_event *currioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: DISK_ACTIVATE_READ  currdisk %p, currdiskreq %p, setseg %d, ok_to_check_bus %d\n",
                         simtime, currdisk, currdiskreq, setseg, ok_to_check_bus );
    fprintf (outputfile, "********** %f: DISK_ACTIVATE_READ  type %s (%d), device %d, blkno %d, bcount %d, flags %d\n",
                         simtime,
                         getEventString(currdiskreq->ioreqlist->type), currdiskreq->ioreqlist->type,
                         currdiskreq->ioreqlist->devno,
                         currdiskreq->ioreqlist->blkno,
                         currdiskreq->ioreqlist->bcount,
                         currdiskreq->ioreqlist->flags);
    fflush(outputfile);
#endif

    disksim_inst_enter();

    /* use specified access time instead of simulating mechanical activity */
    if(currdisk->const_acctime) {
        // SIMPLE_DISK path
        if(!currdisk->currenthda) {
            currdisk->currenthda =
                    currdisk->effectivehda =
                            currdisk->currentbus = currdiskreq;
            currdiskreq->overhead_done = simtime + currdisk->acctime;
            currioreq = ioreq_copy(currdiskreq->ioreqlist);
            currioreq->ioreq_hold_diskreq = currdiskreq;
            currioreq->type = DEVICE_PREPARE_FOR_DATA_TRANSFER;
            currioreq->time = currdiskreq->overhead_done;
            addtointq((event *) currioreq);
            ioqueue_set_starttime(currdisk->queue,currdiskreq->ioreqlist);
        }
        return;
    }

    /* Is there a request being serviced right now? If so, can it be interrupted? */
    if(!(currdiskreq->flags & HDA_OWNED)
            && currdisk->effectivehda
            && (currdisk->effectivehda != currdiskreq)
            && (currdiskreq->hittype != BUFFER_COLLISION)
            && (!currdiskreq->seg
                    || (currdiskreq->seg->recyclereq == currdiskreq)
                    || (currdiskreq->seg->state != BUFFER_DIRTY))
                    && disk_buffer_stopable_access(currdisk,currdiskreq))
    {
        if((currdiskreq->seg != currdisk->effectivehda->seg)
                || (currdiskreq->hittype == BUFFER_NOMATCH))
        {
            disk_buffer_stop_access(currdisk);
        }
        else {
            disk_buffer_attempt_access_swap(currdisk,currdiskreq);
        }
    }

    /* If we have a segment then try to own it */
    if(currdiskreq->seg && setseg) {
        if(currdisk->effectivehda) {
            if(currdiskreq->seg->recyclereq == currdiskreq) {
                /* I don't think the following code ever gets used... */
                fprintf(stderr,"GOT HERE!  SURPRISE, SURPRISE!\n");
                currdiskreq->seg->recyclereq = NULL;
            }
            currdiskreq->seg = NULL;
            currdiskreq->hittype = BUFFER_NOMATCH;
        }
        else {
            disk_buffer_set_segment(currdisk,currdiskreq);
        }
    }

    /* if we have a segment but don't own it, try to own it */
    if(currdiskreq->seg && !(currdiskreq->flags & SEG_OWNED)) {
        disk_buffer_attempt_seg_ownership(currdisk,currdiskreq);
    }

    if(!(currdiskreq->flags & HDA_OWNED)
            && (currdiskreq->hittype != BUFFER_COLLISION)) {
        disk_check_hda(currdisk,currdiskreq,ok_to_check_bus);
    }

    if(!(currdiskreq->flags & HDA_OWNED)
            && (currdisk->currentbus == currdiskreq)) {
        currioreq = currdiskreq->ioreqlist;
        while (currioreq) {
            ioqueue_set_starttime(currdisk->queue,currioreq);
            currioreq = currioreq->next;
        }
    }
}


/* setseg indicates that currdiskreq->seg has not been permanently
 * "set", and should be nullified if the hda cannot be obtained or set
 * if it is obtained
 * in the case of a write cache hit we don't need the HDA
 * ok_to_check_bus set will attempt to start bus activity
 */

static void 
disk_activate_write(disk *currdisk,
        diskreq *currdiskreq,
        int setseg,
        int ok_to_check_bus)
{
    ioreq_event *currioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: DISK_ACTIVATE_WRITE currdiskreq %p, setseg %d, ok_to_check_bus %d\n",
                         simtime, currdiskreq, setseg, ok_to_check_bus );
    dumpIOReq( "disk_activate_write", currdiskreq->ioreqlist );
    fflush(outputfile);
#endif

    if(!currdisk->currenthda && (currdisk->const_acctime)) {
        // SIMPLEDISK path
        currdisk->currenthda =
                currdisk->effectivehda =
                        currdisk->currentbus = currdiskreq;
        currdiskreq->overhead_done = simtime + currdisk->acctime;
        currioreq = ioreq_copy(currdiskreq->ioreqlist);
        currioreq->ioreq_hold_diskreq = currdiskreq;
        currioreq->type = DEVICE_PREPARE_FOR_DATA_TRANSFER;
        currioreq->time = currdiskreq->overhead_done;
        addtointq((event *) currioreq);
        ioqueue_set_starttime(currdisk->queue,currdiskreq->ioreqlist);
        return;
    }

    // DISK device path
    if(!(currdiskreq->flags & HDA_OWNED)
            && currdisk->effectivehda
            && (currdisk->effectivehda != currdiskreq)
            && disk_buffer_stopable_access(currdisk,currdiskreq))
    {
        if((currdiskreq->seg != currdisk->effectivehda->seg) || (currdiskreq->hittype != BUFFER_APPEND)) {
            disk_buffer_stop_access(currdisk);
        }
    }

    if(currdiskreq->seg && setseg) {
        if(currdisk->effectivehda) {
            if(currdiskreq->seg->recyclereq == currdiskreq) {
                /* I don't think the following code ever gets used... */
                fprintf(stderr,"GOT HERE!  SURPRISE, SURPRISE, SURPRISE!\n");
                currdiskreq->seg->recyclereq = NULL;
            }
            currdiskreq->seg = NULL;
            currdiskreq->hittype = BUFFER_NOMATCH;
        }
        else {
            disk_buffer_set_segment(currdisk,currdiskreq);
        }
    }

    if(currdiskreq->seg && !(currdiskreq->flags & SEG_OWNED)) {
        disk_buffer_attempt_seg_ownership(currdisk,currdiskreq);
    }

    if(!(currdiskreq->flags & HDA_OWNED)
            && (currdiskreq->hittype != BUFFER_COLLISION)) {
        disk_check_hda(currdisk,currdiskreq,ok_to_check_bus);
    }

    if(!(currdiskreq->flags & HDA_OWNED)
            && (currdisk->currentbus == currdiskreq)) {
        currioreq = currdiskreq->ioreqlist;
        while (currioreq) {
            ioqueue_set_starttime(currdisk->queue,currioreq);
            currioreq = currioreq->next;
        }
    }
}


static void disk_read_arrive();
static void disk_write_arrive();

static void 
disk_request_arrive(ioreq_event *curr)
{
    ioreq_event *intrp;
    disk *currdisk;
    int flags;
    diskreq *new_diskreq;
    segment *seg;

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_request_arrive  Entering", curr );;
    fflush(outputfile);
#endif

    flags = curr->flags;
    currdisk = getdisk(curr->devno);

#ifdef DISK_QUEUE_LOG_ON
    if( NULL != DISKSIM_DISK_QUEUE_LOG )
    {
        fprintf( DISKSIM_DISK_QUEUE_LOG, "%f %d %d %d %d (%x) %d\n", simtime, curr->devno, curr->blkno, curr->bcount, curr->flags, curr->flags, currdisk->queue->base.listlen + 1 );
        fflush( DISKSIM_DISK_QUEUE_LOG );
    }
#endif

    if((curr->blkno + curr->bcount -1) > currdisk->stat.highblkno) {
        // update high block number statistic
        currdisk->stat.highblkno = curr->blkno + curr->bcount - 1;
    }
    /* done with statistics */

    /* check for valid request. this is done in a few places; this is
     * just the first. also in lbn/pbn translation.
     */

    ddbg_assert3(
            (curr->blkno >= 0) && (curr->bcount > 0) && (curr->blkno + curr->bcount) <= currdisk->model->dm_sectors,
            ("Invalid set of blocks requested from disk - " "blkno %d, bcount %d, numblocks %d\n", curr->blkno, curr->bcount, currdisk->model->dm_sectors));

    /* done checking */

    /* create a new request, set it up for initial interrupt.  I don't
     * know what all the interrupts are for -rcohen
     */
    currdisk->busowned = disk_get_busno(curr);
    intrp = ioreq_copy(curr);
    intrp->type = IO_INTERRUPT_ARRIVE;
    currdisk->outstate = DISK_WAIT_FOR_CONTROLLER;

    /* create the diskreqlist structure that will be attached to the
     * segment and fill in values. the original unmodified request is
     * attached to this as the ioreqlist.
     */
    new_diskreq = (diskreq *) getfromextraq();
    new_diskreq->flags = 0;
    new_diskreq->ioreqlist = curr;
    new_diskreq->seg_next = NULL;
    new_diskreq->bus_next = NULL;
    new_diskreq->outblkno = new_diskreq->inblkno = curr->blkno;

#ifdef DEBUG_DISKCTLR
    //  /* debugging stuff */
    //  if(disk_printhack && (simtime >= disk_printhacktime)) {
    //    fprintf(outputfile, "%12.6f  %8p  Entering disk_request_arrive\n",
    //	    simtime, new_diskreq);
    //
    //    fprintf(outputfile, "                   busno=%d, slotno=%d, disk = %d, blkno = %d, bcount = %d, %s(%d)\n",
    //	    curr->busno, curr->slotno, curr->devno, curr->blkno, curr->bcount, (READ & curr->flags) ? "read" : "write", (READ & curr->flags) );
    //    fflush(outputfile);
    //  }
    //  /* end of debugging stuff */
#endif

    currdisk->effectivebus = new_diskreq;

    /* you may notice that a disk event has no such members. these are
     * #define'd somewhere to tmpptr1 and tmpptr2
     */
    curr->ioreq_hold_disk = currdisk;
    curr->ioreq_hold_diskreq = new_diskreq;

    /* The request must be placed in the queue before selecting a
     * segment, in order for disk_buffer_stopable_access to correctly
     * detect LIMITED_FASTWRITE possibilities.
     */

    ioqueue_add_new_request(currdisk->queue, curr);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_request_arrive  disk queue dump:\n", simtime );
    ioqueue_print_contents(currdisk->queue);
    fflush( outputfile );
#endif

    /* note: new_diskreq->hittype and ->seg both ALWAYS set in
     * select_seg (even if seg == NULL, hittype should be set to
     * BUFFER_NOMATCH)
     */
    seg = disk_buffer_select_segment(currdisk, new_diskreq, TRUE);

    if(flags & READ) {
        disk_read_arrive(currdisk, curr, new_diskreq, seg, intrp);
    }
    else {
        disk_write_arrive(currdisk, curr, new_diskreq, seg, intrp);
    }
}


static void 
disk_read_arrive(disk *currdisk,
        ioreq_event *curr,
        diskreq *new_diskreq,
        segment *seg,
        ioreq_event *intrp)
{
    double delay = 0.0;
    int enough_ready_to_go = TRUE;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_read_arrive\n", simtime );
    fflush(outputfile);
#endif

    disksim_inst_enter();

    /* set up overhead_done to delay any prefetch or completion read
     * activity.  readmiss overheads are used to be compatible with
     * the old simulator.
     */

    if(currdisk->lastflags & READ) {
        new_diskreq->overhead_done =
                currdisk->overhead_command_readmiss_afterread;
    }
    else {
        new_diskreq->overhead_done =
                currdisk->overhead_command_readmiss_afterwrite;
    }
    new_diskreq->overhead_done *= currdisk->timescale;
    new_diskreq->overhead_done += simtime;

    switch (new_diskreq->hittype) {
    case BUFFER_PARTIAL:
        if(!currdisk->immedtrans_any_readhit) {
            int readwater;

            if(currdisk->reqwater) {
                readwater = max(1, (int)((double)min(seg->size,curr->bcount) * currdisk->readhighwatermark));
            }
            else {
                readwater = (int)((((double) seg->size * currdisk->readhighwatermark)) + (double) 0.9999999999);
            }
            if((seg->endblkno <= curr->blkno)
                    || ((seg->endblkno - curr->blkno) < min(readwater,curr->bcount)))
            {
                enough_ready_to_go = FALSE;
            }
        }
        /* FALLTHROUGH */

    case BUFFER_COLLISION:
    case BUFFER_NOMATCH:
        if(new_diskreq->hittype != BUFFER_PARTIAL) {
            enough_ready_to_go = FALSE;
        }
        /* FALLTHROUGH */

    case BUFFER_WHOLE:

        /* IF(never_disconnect
         OR (enough_ready_to_go 
	   AND (sneaky read hits allowed 
	      OR (no request is active
	         OR active request is preemptable)
		  )
	      )
	  )
       THEN
         Perform appropriate read hit overhead and prepare to 
         transfer the data.
       ELSE
	 Otherwise, perform the initial command overhead portion,
         place the request on the hda and bus queues, and disconnect.
         */

        if(currdisk->neverdisconnect
                || (enough_ready_to_go
                        && !currdisk->currentbus
                        && !currdisk->const_acctime
                        && ((currdisk->sneakyfullreadhits
                                && (new_diskreq->hittype == BUFFER_WHOLE))
                                || (currdisk->sneakypartialreadhits
                                        && (new_diskreq->hittype == BUFFER_PARTIAL))
                                        || (!currdisk->pendxfer
                                                && (!currdisk->effectivehda
                                                        || disk_buffer_stopable_access(currdisk,new_diskreq)
                                                )
                                        )
                        )
                )
        )
        {
            intrp->cause = READY_TO_TRANSFER;
            currdisk->currentbus = new_diskreq;
            if(seg) {
                disk_buffer_set_segment(currdisk,new_diskreq);
                seg->outstate = BUFFER_CONTACTING;
            }

            if(!currdisk->const_acctime) {
                if(new_diskreq->hittype == BUFFER_NOMATCH) {
                    if(currdisk->lastflags & READ) {
                        delay = currdisk->overhead_command_readmiss_afterread;
                    }
                    else {
                        delay = currdisk->overhead_command_readmiss_afterwrite;
                    }
                }
                else {
                    if(currdisk->lastflags & READ) {
                        delay = currdisk->overhead_command_readhit_afterread;
                    }
                    else {
                        delay = currdisk->overhead_command_readhit_afterwrite;
                    }
                }
                delay += currdisk->overhead_data_prep;
            }
            else {
                delay = 0.0;
            }

            disk_send_event_up_path(intrp, (delay * currdisk->timescale));
            disk_activate_read(currdisk, new_diskreq, FALSE, TRUE);
        }
        else {
            intrp->cause = DISCONNECT;
            if(!currdisk->const_acctime) {
                // real disk here
                if(currdisk->lastflags & READ) {
                    delay = currdisk->overhead_command_readmiss_afterread +
                            currdisk->overhead_disconnect_read_afterread;
                }
                else {
                    delay = currdisk->overhead_command_readmiss_afterwrite +
                            currdisk->overhead_disconnect_read_afterwrite;
                }
            }
            else {
                // SIMPLE_DISK here
                delay = 0.0;
            }
            disk_send_event_up_path(intrp, (delay * currdisk->timescale));
            disk_activate_read(currdisk, new_diskreq, TRUE, TRUE);
        }
        break;

    default:
        ddbg_assert3(0, ("Invalid read hittype in disk_request_arrive - "
                "blkno %d, bcount %d, hittype %d\n",
                curr->blkno, curr->bcount, new_diskreq->hittype));
        break;
    }
}


static void disk_write_arrive(disk *currdisk,
        ioreq_event *curr,
        diskreq *new_diskreq,
        segment *seg,
        ioreq_event *intrp)
{
    double delay;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_write_arrive\n", simtime );
    fflush(outputfile);
#endif

    disksim_inst_enter();

    /* set up overhead_done to delay any mechanical activity */
    if(currdisk->fastwrites == NO_FASTWRITE) {
        if(currdisk->lastflags & READ) {
            new_diskreq->overhead_done =
                    currdisk->overhead_command_writemiss_afterread;
        }
        else {
            new_diskreq->overhead_done =
                    currdisk->overhead_command_writemiss_afterwrite;
        }
    }
    else {
        if(currdisk->lastflags & READ) {
            new_diskreq->overhead_done =
                    currdisk->overhead_command_writehit_afterread;
        }
        else {
            new_diskreq->overhead_done =
                    currdisk->overhead_command_writehit_afterwrite;
        }
    }

    new_diskreq->overhead_done *= currdisk->timescale;
    new_diskreq->overhead_done += simtime;


    /* LIMITED_FASTWRITE checking is done for "free", as there will be
     * no currentbus or pendxfer and the effectivehda access will be
     * "stoppable" in appropriate cases.
     */

    if(currdisk->neverdisconnect
            || (seg
                    && !currdisk->currentbus
                    && !currdisk->const_acctime
                    && (currdisk->writeprebuffering
                            || (!currdisk->pendxfer
                                    && (!currdisk->effectivehda
                                            || disk_buffer_stopable_access(currdisk,new_diskreq)
                                    )
                            )
                    )
            )
    )
    {
        // path if effectivehda is NULL
        if((new_diskreq->flags & EXTRA_WRITE_DISCONNECT)
                && !currdisk->const_acctime)
        {
            /* re-set up overhead_done to delay any mechanical activity */
            new_diskreq->overhead_done =
                    currdisk->extradisc_command + currdisk->extradisc_seekdelta;

            new_diskreq->overhead_done *= currdisk->timescale;
            new_diskreq->overhead_done += simtime;
            currdisk->extradisc_diskreq = new_diskreq;
            intrp->cause = DISCONNECT;
            delay =
                    currdisk->extradisc_command + currdisk->extradisc_disconnect1;

            EXTRA_WRITE_DISCONNECTS++;
            if(seg) {
                seg->outstate = BUFFER_IDLE;
            }
        }
        else {
            intrp->cause = READY_TO_TRANSFER;
            if(!currdisk->const_acctime) {
                // DISK device path
                if(currdisk->fastwrites == NO_FASTWRITE) {
                    if(currdisk->lastflags & READ) {
                        delay = currdisk->overhead_command_writemiss_afterread;
                    }
                    else {
                        delay = currdisk->overhead_command_writemiss_afterwrite;
                    }
                }
                else {
                    if(currdisk->lastflags & READ) {
                        delay = currdisk->overhead_command_writehit_afterread;
                    }
                    else {
                        delay = currdisk->overhead_command_writehit_afterwrite;
                    }
                }
                delay += currdisk->overhead_data_prep;
            }
            else {
                // SIMPLEDISK device path
                delay = 0.0;
            }

            if(seg) {
                seg->outstate = BUFFER_CONTACTING;
            }
        }
        currdisk->currentbus = new_diskreq;  // set currentbus to the diskreq


        if(seg) {
            disk_buffer_set_segment(currdisk,new_diskreq);
        }

        disk_send_event_up_path(intrp, (delay * currdisk->timescale));
        disk_activate_write(currdisk, new_diskreq, FALSE, TRUE);
    }
    else {
        // path if effectivehda contains diskreq *
        // this path issues a DISCONNECT event
        new_diskreq->flags &= ~EXTRA_WRITE_DISCONNECT;
        intrp->cause = DISCONNECT;
        if(currdisk->fastwrites == NO_FASTWRITE) {
            if(currdisk->lastflags & READ) {
                delay = currdisk->overhead_command_writemiss_afterread;
            }
            else {
                delay = currdisk->overhead_command_writemiss_afterwrite;
            }
        }
        else {
            if(!currdisk->const_acctime) {
                // i.e. not constant acctime
                if(currdisk->lastflags & READ) {
                    delay = currdisk->overhead_command_writehit_afterread;
                }
                else {
                    delay = currdisk->overhead_command_writehit_afterwrite;
                }
                delay += currdisk->overhead_disconnect_write;
            }
            else {
                delay = 0.0;
            }
        }

        disk_send_event_up_path(intrp, (delay * currdisk->timescale));
        disk_activate_write(currdisk, new_diskreq, TRUE, TRUE);
    }
}





/* intermediate disconnect 
 * add acctime >= processing 
 */

static void disk_disconnect (ioreq_event *curr)
{
    disk        *currdisk = getdisk(curr->devno);
    diskreq     *currdiskreq;
    ioreq_event *tmpioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_disconnect\n", simtime );
    fflush(outputfile);
#endif

    disksim_inst_enter();

    currdiskreq = currdisk->effectivebus;

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_disconnect for disk %d\n", simtime, currdiskreq, currdisk->devno);
        fflush(outputfile);
    }

    tmpioreq = currdiskreq->ioreqlist;
    while (tmpioreq) {
        if(tmpioreq->blkno == curr->blkno) {
            break;
        }
        tmpioreq = tmpioreq->next;
    }

    ddbg_assert2(tmpioreq != 0,
            "ioreq_event not found in effectivebus");

    addtoextraq((event *) curr);

    currdisk->effectivebus = NULL;

    if(currdisk->busowned != -1) {
        bus_ownership_release(currdisk->busowned);
        currdisk->busowned = -1;
        currdisk->outstate = ((currdisk->buswait) ? DISK_WAIT_FOR_CONTROLLER : DISK_IDLE);
    }

    if(currdiskreq == currdisk->currentbus) {
        if((currdiskreq->flags & EXTRA_WRITE_DISCONNECT)
                && (currdisk->extradisc_diskreq == currdiskreq))
        {
            tmpioreq = ioreq_copy(tmpioreq);
            tmpioreq->ioreq_hold_diskreq = currdiskreq;
            tmpioreq->type = DEVICE_PREPARE_FOR_DATA_TRANSFER;
            tmpioreq->time = simtime + currdisk->extradisc_inter_disconnect;
            addtointq((event *) tmpioreq);
        }
        else {
            currdisk->currentbus = NULL;
            currdiskreq->bus_next = currdisk->pendxfer;
            currdisk->pendxfer = currdiskreq;
            disk_check_bus(currdisk, NULL);
        }
    } 
    else {
        currdiskreq->bus_next = currdisk->pendxfer;
        currdisk->pendxfer = currdiskreq;
        disk_check_bus(currdisk, NULL);
    }
}




/* completion disconnect 
 * add acctime >= 0.0 processing 
 */

static void disk_completion (ioreq_event *curr)
{
    disk        *currdisk = getdisk(curr->devno);
    diskreq     *currdiskreq;
    ioreq_event *tmpioreq;
    segment     *seg;

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_completion Entering", curr );
    fflush(outputfile);
#endif

    simresult_update_diskCtlrTimeEnd( curr->tagID, simtime );
    simresult_update_diskCtlrQueueDepthEnd( curr->tagID,  currdisk->queue->base.listlen );

    int simResultTagID = curr->tagID - 1;  // array is zero referenced
    DISKSIM_SIM_RESULT_LIST[simResultTagID].tagID_end = ++disksim->tagIDEnd;
    DISKSIM_SIM_RESULT_LIST[simResultTagID].CCT       = DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrTimeEnd - DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrTimeBegin;

    DISKSIM_SIM_RESULT_LIST[simResultTagID].qCCT = DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrTimeEnd - DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrTimeBegin;
    if( (0 != simResultTagID) && (0 != DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrQueueDepth ))
    {
        int tagID = simresult_getSimResultContainsEndTagID( DISKSIM_SIM_RESULT_LIST[simResultTagID].tagID_end -1 );
        DISKSIM_SIM_RESULT_LIST[simResultTagID].qCCT = DISKSIM_SIM_RESULT_LIST[simResultTagID].diskCtlrTimeEnd - DISKSIM_SIM_RESULT_LIST[tagID].diskCtlrTimeEnd;
    }

    currdiskreq = currdisk->effectivebus;

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_completion for disk %d\n", simtime, currdiskreq,currdisk->devno);
        fflush(outputfile);
    }

    ddbg_assert2(currdiskreq != 0, "effectivebus is NULL");

    ddbg_assert2((currdiskreq->flags & COMPLETION_SENT),
            "effectivebus isn't marked COMPLETION_SENT");

    seg = currdiskreq->seg;

#ifdef DEBUG_DISKCTLR
    dump_disk_buffer_seqment ( outputfile, seg, -1 );
    fprintf (outputfile, "\n" );
#endif

    ddbg_assert2((seg != 0) || (currdisk->const_acctime >= 0),
            "NULL seg found in effectivebus");

    tmpioreq = currdiskreq->ioreqlist;
    while (tmpioreq) {
        if(tmpioreq->blkno == curr->blkno) {
            break;
        }
        tmpioreq = tmpioreq->next;
    }
    ddbg_assert2(tmpioreq != 0, "ioreq_event not found in effectivebus");

    while (tmpioreq->next) {
        tmpioreq = tmpioreq->next;
    }

    currdiskreq->flags |= COMPLETION_RECEIVED;
    addtoextraq((event *) curr);
    currdisk->effectivebus = NULL;
    if(currdiskreq == currdisk->currentbus) {
        currdisk->currentbus = NULL;
    }

    if(currdisk->busowned != -1) {
        bus_ownership_release(currdisk->busowned);
        currdisk->busowned = -1;
        currdisk->outstate = DISK_IDLE;
    }

    if(currdisk->const_acctime) {
        // SIMPLE_DISK_MODEL path
        addtoextraq((event *) currdiskreq->ioreqlist);
        addtoextraq((event *) currdiskreq);
        return;
    }
    // DISK device path
    else if(currdiskreq->ioreqlist->flags & READ) {
        while (currdiskreq->ioreqlist) {
            tmpioreq = currdiskreq->ioreqlist;
            currdiskreq->ioreqlist = tmpioreq->next;
            addtoextraq((event *) tmpioreq);
        }
        if((currdiskreq == currdisk->effectivehda) ||
                (currdiskreq == currdisk->currenthda)) {
            if(seg->access->type == NULL_EVENT) {
                disk_release_hda(currdisk,currdiskreq);
            }
            else {
                disk_check_prefetch_swap(currdisk);
            }
        }
        else {
            disk_buffer_remove_from_seg(currdiskreq);
            addtoextraq((event *) currdiskreq);
            if(seg->recyclereq) {
                ddbg_assert2(seg->diskreqlist == 0,
                        "non-NULL diskreqlist found for recycled seg");

                disk_buffer_set_segment(currdisk,seg->recyclereq);
                disk_buffer_attempt_seg_ownership(currdisk,seg->recyclereq);
                seg->recyclereq = NULL;
            }
            else if(seg->diskreqlist) {
                disk_find_new_seg_owner(currdisk,seg);
            }
            else if(!currdisk->enablecache) {
                seg->state = BUFFER_EMPTY;
            }
        }
    } 
    else { 			/* WRITE */
        if(currdiskreq->inblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
            if((currdiskreq == currdisk->effectivehda) ||
                    (currdiskreq == currdisk->currenthda)) {
                disk_release_hda(currdisk,currdiskreq);
            }
            else {
                while (currdiskreq->ioreqlist) {
                    tmpioreq = currdiskreq->ioreqlist;
                    currdiskreq->ioreqlist = tmpioreq->next;
                    addtoextraq((event *) tmpioreq);
                }
                disk_buffer_remove_from_seg(currdiskreq);
                addtoextraq((event *) currdiskreq);
                if(seg->recyclereq) {
                    ddbg_assert2(seg->diskreqlist == 0,
                            "non-NULL diskreqlist found for recycled seg");

                    disk_buffer_set_segment(currdisk,seg->recyclereq);
                    disk_buffer_attempt_seg_ownership(currdisk,seg->recyclereq);
                    seg->recyclereq = NULL;
                } else if(seg->diskreqlist) {
                    disk_find_new_seg_owner(currdisk,seg);
                }
            }
        }
        else if(currdiskreq->flags & SEG_OWNED) {
            seg->access->flags |= BUFFER_BACKGROUND;
        }
    }

    disk_check_hda(currdisk, NULL, TRUE);
    disk_check_bus(currdisk, NULL);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_completion Exiting\n", simtime );
    dumpIOReq( "disk_completion", curr );
    fflush(outputfile);
#endif
}




static void disk_interrupt_complete (ioreq_event *curr)
{
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_interrupt_complete  cause %s (%d)\n", simtime, iosim_decodeInterruptEvent(curr), curr->cause );
    fflush(outputfile);
#endif

    switch (curr->cause) {

    case RECONNECT:
        disk_reconnection_or_transfer_complete(curr);
        break;

    case DISCONNECT:
        disk_disconnect(curr);
        break;

    case COMPLETION:
        disk_completion(curr);
        break;

    default:
        fprintf(stderr, "Unrecognized event type at disk_interrupt_complete\n");
        exit(1);
    }
}




void disk_event_arrive (ioreq_event *curr)
{
//    struct dm_mech_acctimes mech_acctimes;
    disk *currdisk = getdisk (curr->devno);

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_event_arrive  Entering", curr );
    dump_disk_buffer_seqments( outputfile, currdisk->seglist, "disk_event_arrive::main cache" );
    fprintf (outputfile, "\n\n" );
    ioqueue_print_contents (currdisk->queue);
#endif

    switch (curr->type) {

    case IO_ACCESS_ARRIVE:
        if(currdisk->overhead > 0.0) {
            curr->time = simtime + (currdisk->overhead * currdisk->timescale);
            curr->type = DEVICE_OVERHEAD_COMPLETE;
            addtointq((event *) curr);
        }
        else {
            curr->tagID = ++disksim->tagIDBegin;
            ddbg_assert2 ( disksim->tagIDBegin < MAX_NUM_SIM_RESULTS, "Need to increase size of Sim Results");

            simresult_set_seekResultIndex( curr->tagID, 0 );
            simresult_add_new_request( curr, currdisk->queue->base.listlen );
            simresult_update_diskCtlrTimeBegin( curr->tagID, simtime );
//            disk_calc_rpo_time( currdisk, curr, &mech_acctimes );
//            simresult_update_mech_acctimes( curr->tagID, &mech_acctimes );
            disk_request_arrive(curr);
        }
        break;

    case DEVICE_OVERHEAD_COMPLETE:
        disk_request_arrive(curr);
        break;

    case DEVICE_BUFFER_SEEKDONE:
        disk_buffer_seekdone(currdisk, curr);
        break;

    case DEVICE_BUFFER_SECTOR_DONE:
        simresult_update_diskBlocksXferred( curr );
        disk_buffer_sector_done(currdisk, curr);
        break;

    case DEVICE_GOTO_REMAPPED_SECTOR:
        disk_goto_remapped_sector(currdisk, curr);
        break;

    case DEVICE_GOT_REMAPPED_SECTOR:
        disk_got_remapped_sector(currdisk, curr);
        break;

    case DEVICE_PREPARE_FOR_DATA_TRANSFER:
        disk_prepare_for_data_transfer(curr);
        break;

    case DEVICE_DATA_TRANSFER_COMPLETE:
        disk_reconnection_or_transfer_complete(curr);
        break;

    case IO_INTERRUPT_COMPLETE:
        disk_interrupt_complete(curr);
        break;

    case IO_QLEN_MAXCHECK:
        /* Used only at initialization time to set up queue stuff */
        curr->tempint1 = -1;
        curr->tempint2 = disk_get_maxoutstanding(curr->devno);
        curr->bcount = 0;
        break;

    default:
        ddbg_assert2(0, "Unrecognized event type");
        break;
    }
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_event_arrive  Exit\n", simtime );
#endif
}


void disk_write_cache_idletime_detected (void *idleworkparam, int idledevno)
{
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_write_cache_idletime_detected idleworkparam %p, devno %d\n", simtime, idleworkparam, idledevno );
    fflush(outputfile);
#endif
}

void disk_write_cache_periodic_flush (timer_event *timereq)
{
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_periodic_flush  Entering\n", simtime );
    fflush(outputfile);
#endif

    struct disk *currdisk = (struct disk *) timereq->ptr;

    timereq->time += currdisk->writeCacheFlushPeriod;
    addtointq((event *)timereq);
}


/*
 * Possible improvements:
 *
 * As write sectors complete, appending writes may be able to put
 * more data into the cache.  However, need to find a way to keep
 * from having to check everything on the bus queue every time a
 * write sector completes.  Obviously can check for appending writes
 * in the same seg, but need to be conscious of watermarks for the
 * appending write as well.  (Don't reconnect just to xfer one more
 * sector after each sector completes -- unless watermark says so.)
 *
 * As read sectors complete, sneakyintermediatereads could become
 * available.  However, need to find a way to keep from having to
 * check everything on the bus queue every time a read sector
 * completes.  
 */

/* I'm unhappy that this function does work looking at how it's
 * called, it should be made to only return a truth value, no side
 * effects -rcohen 
 */

static int 
disk_buffer_request_complete(disk *currdisk, diskreq *currdiskreq)
{
    int background;
    int reading;
    int reqdone = FALSE;
    int hold_type;
    double acctime;
    int watermark = FALSE;
    ioreq_event *tmpioreq = currdiskreq->ioreqlist;
    segment *seg = currdiskreq->seg;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_buffer_request_complete  Entering\n", simtime );
    fflush(outputfile);
#endif

    // get the last ioreq_event
    while (tmpioreq && tmpioreq->next) {
        tmpioreq = tmpioreq->next;
    }

    background = seg->access->flags & BUFFER_BACKGROUND;
    reading = (seg->state == BUFFER_READING);
    if(currdisk->outwait) {
        currdisk->outwait->time = simtime;
        currdisk->outwait->bcount = 0;
        addtointq((event *) currdisk->outwait);
        currdisk->outwait = NULL;
    }


    if(reading) {
        if(background) {
            if(seg->endblkno >= seg->maxreadaheadblkno) {
                reqdone = TRUE;
            }
        }
        else {
            if((seg->endblkno - currdiskreq->outblkno) >= currdiskreq->watermark) {
                watermark = TRUE;
            }

            if(seg->endblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
                // here, we have detected the end of the read command
                reqdone = TRUE;
            }
        }
    }
    else {                     /* (!reading) -> WRITE */
        if(background) {
            if(seg->endblkno == currdiskreq->inblkno) {
                reqdone = TRUE;
            }
        }
        else {
            if((seg->endblkno - currdiskreq->inblkno) <= currdiskreq->watermark) {
                if(seg->endblkno < (tmpioreq->blkno + tmpioreq->bcount)) {
                    watermark = TRUE;
                }
            }

            if(currdiskreq->inblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
                // here, we have detected the end of the write command
                reqdone = TRUE;
            }
        }
    }


    if((background && reqdone && reading) || (seg->outstate == BUFFER_PREEMPT))
    {
        seg->access->type = NULL_EVENT;
        disk_release_hda(currdisk, currdiskreq);
        return(TRUE);
    }

    if(!background && (seg->outstate == BUFFER_IDLE) && (reqdone | watermark) )
    {
        disk_check_bus(currdisk,currdiskreq);
    }

    if (background && reading && (seg->endblkno == seg->minreadaheadblkno)
            && ((currdisk->preseeking == PRESEEK_BEFORE_COMPLETION)
                    || (currdiskreq->flags & COMPLETION_RECEIVED)
                    || ((currdisk->preseeking == PRESEEK_DURING_COMPLETION)
                            && (currdiskreq->flags & COMPLETION_SENT))))
    {
        hold_type = seg->access->type;
        seg->access->type = NULL_EVENT;
        disk_check_prefetch_swap(currdisk);
        if ((seg->outstate == BUFFER_PREEMPT)
                || (currdisk->effectivehda != currdiskreq))
        {
            return (TRUE);
        }
        seg->access->type = hold_type;
    }
    else if ((!background || !reading) && reqdone)
    {
        // check if we are done
#ifdef DEBUG_DISKCTLR
        ioqueue_print_contents(currdisk->queue);
#endif
        if (!reading || (currdisk->contread == BUFFER_NO_READ_AHEAD)
                || (seg->endblkno >= seg->maxreadaheadblkno) || (currdisk->queue->base.listlen > 1) )
        {
            seg->access->type = NULL_EVENT;
        }
        else
        {
            seg->access->flags |= BUFFER_BACKGROUND;
            reqdone = FALSE;
        }

        if (currdisk->stat.seekdistance != -1)
        {
            acctime = currdisk->stat.seektime + currdisk->stat.latency
                    + currdisk->stat.xfertime;

            simresult_update_diskInfo( currdiskreq->ioreqlist->tagID, currdisk->stat.seekdistance, currdisk->stat.seektime, currdisk->stat.latency, currdisk->stat.xfertime, acctime );

#ifdef DISKLOG_ON
            if( NULL !=  DISKSIM_DISK_LOG )
            {
                fprintf (DISKSIM_DISK_LOG, "\n********** %f: disk_buffer_request_complete  ACCTIMESTATS: distance %d, seektime %f, latency %f, xfertime %f, acctime %f\n",
                        simtime,
                        currdisk->stat.seekdistance,
                        currdisk->stat.seektime,
                        currdisk->stat.latency,
                        currdisk->stat.xfertime,
                        acctime);
                fflush(DISKSIM_DISK_LOG);
            }
#endif

            //  printf("ACCTIMESTATS(%d, %f, %f, %f, %f)\n",
            //	     currdisk->stat.seekdistance,
            ////	     currdisk->stat.seektime,
            //	     currdisk->stat.latency,
            //	     currdisk->stat.xfertime,
            //	     acctime);

            disk_acctimestats(currdisk, currdisk->stat.seekdistance,
                    currdisk->stat.seektime, currdisk->stat.latency,
                    currdisk->stat.xfertime, acctime);
        }

        tmpioreq = currdiskreq->ioreqlist;
        while (tmpioreq)
        {
            disk_interferestats(currdisk, tmpioreq);
            tmpioreq = tmpioreq->next;
        }

        currdisk->stat.seekdistance = -1;
        currdisk->stat.xfertime     = (double) 0.0;
        currdisk->stat.latency      = (double) 0.0;
        if (!reqdone
                && ((currdisk->preseeking == PRESEEK_BEFORE_COMPLETION)
                        || (currdiskreq->flags & COMPLETION_RECEIVED)
                        || ((currdisk->preseeking == PRESEEK_DURING_COMPLETION)
                                && (currdiskreq->flags & COMPLETION_SENT))))
        {
            hold_type = seg->access->type;
            seg->access->type = NULL_EVENT;
            disk_check_prefetch_swap(currdisk);
            if ((seg->outstate == BUFFER_PREEMPT)
                    || (currdisk->effectivehda != currdiskreq))
            {
                return (TRUE);
            }
            seg->access->type = hold_type;
        }
    }

    if (reqdone)
    {
        if (!reading)
        {
            // here, we are writing
            tmpioreq = currdiskreq->ioreqlist;
            while (tmpioreq)
            {
                ddbg_assert2(
                        ioqueue_physical_access_done(currdisk->queue,tmpioreq) != NULL,
                        "disk_buffer_request_complete:  ioreq_event "
                        "not found by ioqueue_physical_access_done call");

                tmpioreq = tmpioreq->next;
            }
            if (!(currdiskreq->flags & COMPLETION_SENT)
                    && (seg->outstate == BUFFER_IDLE))
            {
                disk_check_bus(currdisk, currdiskreq);
            }
        }
        seg->access->type = NULL_EVENT;
        disk_release_hda(currdisk, currdiskreq);
    }

    if (reqdone)
    {
        //    printf("disk_buffer_request_complete %f\n", simtime);
    }

    return (reqdone);
}


static void disk_got_remapped_sector (disk *currdisk, ioreq_event *curr)
{
    diskreq *currdiskreq = currdisk->effectivehda;
    segment *seg;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_got_remapped_sector\n", simtime );
    fflush(outputfile);
#endif

    ddbg_assert2(currdiskreq != NULL, "No effectivehda");

    seg = currdiskreq->seg;
    if(seg->outstate == BUFFER_PREEMPT) {
        seg->access->type = NULL_EVENT;
        disk_release_hda(currdisk, currdiskreq);
        return;
    }

    if((currdiskreq == seg->recyclereq)
            || !disk_buffer_block_available(currdisk, seg, curr->blkno))
    {
        double rottime = dm_time_itod(currdisk->model->mech->dm_period(currdisk->model));
        seg->time += rottime;
        currdisk->stat.xfertime += rottime;
        curr->time += rottime;
        curr->type = DEVICE_GOT_REMAPPED_SECTOR;
        addtointq((event *) curr);
    } 
    else {
        if(seg->state == BUFFER_READING) {
            if(curr->blkno != (seg->endblkno+1)) {
                fprintf(stderr, "Logical address of remapped sector not next to read (#1)\n");
                assert(0);
            }
            seg->endblkno++;
        } else {
            if(curr->blkno != currdiskreq->inblkno) {
                fprintf(stderr, "Logical address of remapped sector not next to read (#2)\n");
                assert(0);
            }
            currdiskreq->inblkno++;
        }
        if(!disk_buffer_request_complete(currdisk, currdiskreq)) {
            curr->blkno++;
            if(!disk_initiate_seek(currdisk, seg, curr, 0, 0.0, 0.0)) {
                disk_release_hda(currdisk,currdiskreq);
            }
        }
    }
}



static void 
disk_buffer_update_outbuffer(disk *currdisk, segment *seg)
{
    int blks;
    diskreq* currdiskreq = currdisk->effectivebus;
    double tmptime;
    ioreq_event *tmpioreq;
    diskreq *seg_owner;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_update_outbuffer\n", simtime );
    fflush(outputfile);
#endif

    if(!currdiskreq) {
        return;
    }

    ddbg_assert(currdiskreq->seg == seg);


    seg_owner = disk_buffer_seg_owner(seg,FALSE);
    if(!seg_owner) {
        seg_owner = currdiskreq;
    }

    tmpioreq = currdiskreq->ioreqlist;
    while (tmpioreq) {
        if((currdiskreq->outblkno >= tmpioreq->blkno) &&
                (currdiskreq->outblkno < (tmpioreq->blkno + tmpioreq->bcount))) {
            break;
        }
        tmpioreq = tmpioreq->next;
    }

    ddbg_assert2(tmpioreq != 0, "Cannot find ioreq");


#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_buffer_update_outbuffer  inblkno %d, startblkno %d, endblkno %d, devno %d, blkno %d, bcount %d, flags %x\n", simtime, seg_owner->inblkno, seg->startblkno, seg->endblkno, seg->access->devno, seg->access->blkno, seg->access->bcount, seg->access->flags );
    fflush( outputfile );
#endif

    if(currdisk->outwait) {
        return;
    }

    blks = bus_get_data_transfered(tmpioreq, disk_get_depth(tmpioreq->devno));

    if(blks == -1) {
        tmptime = disk_get_blktranstime(tmpioreq);
        if(tmptime != (double) 0.0) {
            blks = (int) ((simtime - currdisk->starttrans) / tmptime);
        }
        else {
            blks = seg->outbcount;
        }
    }

    blks = min(seg->outbcount, (blks - currdisk->blksdone));
    currdisk->blksdone += blks;
    currdiskreq->outblkno += blks;
    seg->outbcount -= blks;

    if(currdiskreq->outblkno > seg->endblkno) {
        seg->endblkno = currdiskreq->outblkno;
    }

    disk_buffer_segment_wrap(seg, seg->endblkno);

    if((seg->state == BUFFER_WRITING)
            && ((seg_owner->inblkno < seg->startblkno)
                    || (seg_owner->inblkno > seg->endblkno)))
    {
        ddbg_assert3(0, ("Error with inblkno at disk_buffer_update_outbuffer: "
                "inblkno %d, startblkno %d, endblkno %d, blks %d\n",
                seg_owner->inblkno,
                seg->startblkno,
                seg->endblkno,
                blks));
    }
} // disk_buffer_update_outbuffer()


/* returns the following values in returned_ioreq->bcount:
 *    -2 if no disconnect and no valid transfer size exists
 *    -1 if disconnect requested
 *     0 if completed
 *    >0 if valid transfer size exists
 */

static ioreq_event * 
disk_buffer_transfer_size(disk *currdisk, 
        diskreq *currdiskreq,
        ioreq_event *curr)
{
    segment *seg = currdiskreq->seg;
    diskreq *seg_owner;
    ioreq_event *tmpioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_transfer_size  currdiskreq %p, devno %d, blkno %d, bcount %d, flags %X\n", simtime, currdiskreq, curr->devno, curr->blkno, curr->bcount, curr->flags );
    fflush(outputfile);
#endif

    /*
    fprintf (outputfile, "Entered disk_buffer_transfer_size - devno %d\n", curr->devno);
    fprintf (outputfile, "outstate %d, outblkno %d, outbcount %d, endblkno %d, inblkno %d\n", seg->outstate, currdiskreq->outblkno, seg->outbcount, seg->endblkno, seg->inblkno);
     */

    if(currdisk->const_acctime) {
        // SIMPLEDISK device path
        if(currdiskreq->overhead_done <= simtime) {
            curr->bcount = DISKCTLR_TRANSFER_COMPLETED;
        }
        else {
            curr->bcount = DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS;
        }
        return(curr);
    }

    // DISK device path
    if(!seg) {
        if(!currdisk->neverdisconnect) {
            fprintf(stderr, "NULL seg found at disk_buffer_transfer_size without neverdisconnect set\n");
            exit(1);
        }
        curr->bcount = DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS;
        return(curr);
    }

    tmpioreq = currdiskreq->ioreqlist;
    while (tmpioreq) {
        if(tmpioreq->blkno == curr->blkno) {
            break;
        }
        tmpioreq = tmpioreq->next;
    }
    if(!tmpioreq) {
        fprintf(stderr, "ioreq_event not found in effectivebus in disk_buffer_transfer_size\n");
        exit(1);
    }

    if(tmpioreq->flags & READ) {
        if(seg->outstate == BUFFER_TRANSFERING) {
            currdiskreq->outblkno += seg->outbcount;
        }
        if(currdiskreq->outblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
            tmpioreq = tmpioreq->next;
            if(!tmpioreq) {
                seg->outstate = BUFFER_IDLE;
                curr->bcount = DISKCTLR_TRANSFER_COMPLETED;
                return(curr);
            }
            else {
                if(currdiskreq->outblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
                    fprintf(stderr, "read ioreq_event skipped in disk_reconnection_or_transfer_complete\n");
                    exit(1);
                }
                addtoextraq((event *) curr);
                curr = ioreq_copy(tmpioreq);
            }
        }
        if(seg->endblkno > currdiskreq->outblkno) {
            seg->outstate = BUFFER_TRANSFERING;
            curr->bcount = seg->outbcount = min(seg->endblkno,(tmpioreq->blkno + tmpioreq->bcount)) - currdiskreq->outblkno;
        } else if(currdisk->hold_bus_for_whole_read_xfer ||
                currdisk->neverdisconnect) {
            seg->outstate = BUFFER_TRANSFERING;
            seg->outbcount = 0;
            curr->bcount = DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS;
        } else {
            seg->outstate = BUFFER_IDLE;
            curr->bcount = DISKCTLR_DISCONNECT_REQUESTED;
        }
    }
    else {  			/* WRITE */
        if((currdiskreq->outblkno == (tmpioreq->blkno + tmpioreq->bcount))
                && (currdiskreq->inblkno >= currdiskreq->outblkno)) {
            if(tmpioreq->next) {
                fprintf(stderr, "write ioreq_event skipped (A) in disk_reconnection_or_transfer_complete\n");
                exit(1);
            }
            seg->outstate = BUFFER_IDLE;
            curr->bcount = DISKCTLR_TRANSFER_COMPLETED;
            return(curr);
        }

        seg_owner = disk_buffer_seg_owner(seg,FALSE);
        if(!seg_owner) {
            seg_owner = currdiskreq;
        }

        if(seg->outstate == BUFFER_TRANSFERING) {
            currdiskreq->outblkno += seg->outbcount;
            if(tmpioreq->blkno < seg->startblkno) {
                seg->startblkno = tmpioreq->blkno;
            }
            if(currdiskreq->outblkno > seg->endblkno) {
                seg->endblkno = currdiskreq->outblkno;
            }
            if(seg->hold_bcount &&
                    (seg->endblkno == seg->hold_blkno)) {
                if(tmpioreq->next ||
                        (currdiskreq->outblkno != (tmpioreq->blkno + tmpioreq->bcount))) {
                    fprintf(stderr, "Error with hold fields at disk_buffer_transfer_size\n");
                    exit(1);
                }
                seg->endblkno += seg->hold_bcount;
                seg->hold_bcount = 0;
            }
            disk_buffer_segment_wrap(seg, seg->endblkno);
            ddbg_assert((seg_owner->inblkno >= seg->startblkno) &&
                    (seg_owner->inblkno <= seg->endblkno));
            if(currdiskreq->outblkno >= (tmpioreq->blkno + tmpioreq->bcount)) {
                tmpioreq = tmpioreq->next;
                if(!tmpioreq) {
                    seg->outstate = BUFFER_IDLE;
                    if(currdisk->fastwrites == NO_FASTWRITE) {
                        curr->bcount = DISKCTLR_DISCONNECT_REQUESTED;
                    }
                    else {
                        //here, is where the command complete is for fastwrites > 0????
                        curr->bcount = DISKCTLR_TRANSFER_COMPLETED;
                    }
                    return(curr);
                }
                else {
                    if(currdiskreq->outblkno >= (tmpioreq->blkno + tmpioreq->bcount))
                    {
                        fprintf(stderr, "write ioreq_event skipped (B) in disk_reconnection_or_transfer_complete\n");
                        exit(1);
                    }
                    addtoextraq((event *) curr);
                    curr = ioreq_copy(tmpioreq);
                }
            }
            if((currdiskreq->outblkno - seg_owner->inblkno) > seg->size) {
                fprintf(stderr, "Buffer needs to wrap around\n");
                exit(1);
            }

        }
        seg->outstate = BUFFER_TRANSFERING;
        curr->bcount = seg->outbcount =
                min((seg->size - (currdiskreq->outblkno - seg_owner->inblkno)),
                        (tmpioreq->blkno + tmpioreq->bcount - currdiskreq->outblkno));
        /*
      fprintf (outputfile, "%f Start transfer of %d sectors of %d (from %d)\n", simtime, seg->outbcount, seg->reqlist->bcount, seg->reqlist->blkno);
         */
        if((!(currdiskreq->flags & SEG_OWNED)
                && (seg->endblkno < currdiskreq->outblkno))
                || (seg->outbcount <= 0))
        {
            if(currdisk->hold_bus_for_whole_write_xfer
                    || currdisk->neverdisconnect)
            {
                if((disk_printhack > 1) && (simtime >= disk_printhacktime)) {
                    fprintf(stderr,"Holding bus...\n");
                }
                seg->outstate = BUFFER_TRANSFERING;
                seg->outbcount = 0;
                curr->bcount = DISKCTLR_NO_DISCONNECT_AND_NO_VALID_TRANSFER_SIZE_EXISTS;
            }
            else {
                seg->outstate = BUFFER_IDLE;
                curr->bcount = DISKCTLR_DISCONNECT_REQUESTED;
            }
        }
    } // write
    return(curr);
}


/*
 * Enablement function for ioqueue which selects the next hda access
 * to perform.
 *
 * If all segs are dirty and none are recyclable, choose a write with
 * a segment.
 *
 * Don't choose sneaky reads on write segments unless they are
 * recyclable
 *
 * If no seg, call select_segment (and possibly recyclable_segment) to
 * make sure that a seg is/will be available.  However, reset the seg
 * and hittype afterwards.
 *
 * Don't choose BUFFER_COLLISION requests.
 *
 * Note that writeseg environments cannot use recycled segments. 
 */

int disk_enablement_function(ioreq_event *currioreq)
{
    diskreq     *currdiskreq = (diskreq*) currioreq->ioreq_hold_diskreq;
    disk        *currdisk = (disk*) currioreq->ioreq_hold_disk;
    ioreq_event *tmpioreq;
    diskreq     *seg_owner;

#ifdef DEBUG_DISKCTLR
    dumpIOReq( "disk_enablement_function", currioreq);
    fflush(outputfile);
#endif

    if(!currdisk) {
        fprintf(stderr, "Disk ioqueue iobuf has NULL ioreq_hold_disk (disk*)\n");
        exit(1);
    }

    if(!currdiskreq) {
        fprintf(stderr, "Disk ioqueue iobuf has NULL ioreq_hold_diskreq (diskreq*)\n");
        exit(1);
    }

    if(currdisk->const_acctime) {
        // SIMPLEDISK device path
        return(TRUE);
    }

    // Disk device path
    if(currioreq->flags & READ) {
        /*
      if((currdisk->numdirty == currdisk->numsegs) && !currdiskreq->seg)
      return(FALSE);
      } else 
         */
        if(currdiskreq->seg) {
            ddbg_assert(currdiskreq->seg->recyclereq != currdiskreq);
            if((currdiskreq->seg->state == BUFFER_WRITING) ||
                    (currdiskreq->seg->state == BUFFER_DIRTY)) {
                if(!disk_buffer_reusable_segment_check(currdisk,currdiskreq->seg)) {
                    return(FALSE);
                }
            }
        } else {
            disk_buffer_select_segment(currdisk,currdiskreq,FALSE);
            if(currdiskreq->seg) {
                if((currdiskreq->seg->state == BUFFER_WRITING)
                        || (currdiskreq->seg->state == BUFFER_DIRTY))
                {
                    if(!disk_buffer_reusable_segment_check(currdisk,currdiskreq->seg)) {
                        currdiskreq->hittype = BUFFER_NOMATCH;
                        currdiskreq->seg = NULL;
                        return(FALSE);
                    }
                }
                currdiskreq->hittype = BUFFER_NOMATCH;
                currdiskreq->seg = NULL;
            }
            else {
                if((currdiskreq->hittype == BUFFER_COLLISION)
                        || !disk_buffer_recyclable_segment(currdisk,TRUE))
                {
                    return(FALSE);
                }
            }
        }
    } 
    else {		/* WRITE */
        if(currdiskreq->seg) {
            /*
	if((currdisk->numdirty == currdisk->numsegs) &&
             */
            ddbg_assert(currdiskreq->seg->recyclereq != currdiskreq);
            if(!(currdiskreq->flags & SEG_OWNED)) {
                seg_owner = disk_buffer_seg_owner(currdiskreq->seg,FALSE);
                if(!seg_owner) {
                    seg_owner = currdiskreq->seg->diskreqlist;
                    ddbg_assert(seg_owner->ioreqlist != NULL);
                    while (seg_owner->ioreqlist->flags & READ) {
                        seg_owner = seg_owner->seg_next;
                        ddbg_assert(seg_owner != NULL);
                        ddbg_assert(seg_owner->ioreqlist != NULL);
                    }
                }
                if(seg_owner != currdiskreq) {
                    tmpioreq = currdiskreq->ioreqlist;
                    ddbg_assert(tmpioreq != NULL);
                    while (tmpioreq->next) {
                        tmpioreq = tmpioreq->next;
                    }
                    if(((tmpioreq->blkno + tmpioreq->bcount - seg_owner->inblkno) >
                    currdiskreq->seg->size)
                            && (seg_owner->inblkno != currdiskreq->ioreqlist->blkno))
                    {
                        return(FALSE);
                    }
                }
            }
        }
        else {
            disk_buffer_select_segment(currdisk,currdiskreq,FALSE);
            if(currdiskreq->seg) {
                if(currdiskreq->hittype == BUFFER_APPEND) {
                    seg_owner = disk_buffer_seg_owner(currdiskreq->seg, FALSE);
                    ddbg_assert(seg_owner != NULL);
                    tmpioreq = currdiskreq->ioreqlist;
                    ddbg_assert(tmpioreq != NULL);
                    while (tmpioreq->next) {
                        tmpioreq = tmpioreq->next;
                    }

                    if(((tmpioreq->blkno + tmpioreq->bcount - seg_owner->inblkno) >
                    currdiskreq->seg->size)
                            && (seg_owner->inblkno != currdiskreq->ioreqlist->blkno))
                    {
                        currdiskreq->hittype = BUFFER_NOMATCH;
                        currdiskreq->seg = NULL;
                        return(FALSE);
                    }
                }
                currdiskreq->hittype = BUFFER_NOMATCH;
                currdiskreq->seg = NULL;
            }
            else {
                if((currdiskreq->hittype == BUFFER_COLLISION) ||
                        !disk_buffer_recyclable_segment(currdisk,FALSE)) {
                    return(FALSE);
                }
            }
        }
    }

    return(TRUE);
}


/*
 * "stopable" means either that the current effectivehda request can,
 * in fact, be stopped instantly, or that the current request can be
 * "merged" in some way with the effectivehda request.  Examples of
 * the latter case include read hits on prefetched data and
 * sequential writes.
 *
 * Note that a request is not stopable if the preseeking level is not
 * appropriate, or if its seg is marked as recycled, or if there is a
 * "better" request in the queue than the passed in request.  "Better"
 * is indicated via the preset scheduling algorithm and
 * ioqueue_show_next_request.  
 */

int disk_buffer_stopable_access (disk *currdisk, diskreq *currdiskreq)
{
    segment *seg;
    diskreq *effective = currdisk->effectivehda;
    ioreq_event *currioreq;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_stopable_access\n", simtime );
    fflush(outputfile);
#endif

    if(disk_printhack && (simtime >= disk_printhacktime)) {
        fprintf (outputfile, "%12.6f  %8p  Entering disk_buffer_stopable_access\n", simtime, currdiskreq);
        fprintf (outputfile, "                        checking if %8p is stoppable\n", effective);
        fflush(outputfile);
    }

    if(!effective) {
        fprintf(stderr, "Trying to stop a non-existent access\n");
        exit(1);
    }

    seg = effective->seg;

    if(!seg) {
        fprintf(stderr, "effectivehda has NULL seg in disk_buffer_stopable_access\n");
        exit(1);
    }

    if(seg->recyclereq) {
        if(seg->recyclereq != effective) {
            fprintf(stderr, "effectivehda != recyclereq in disk_buffer_stopable_access\n");
            exit(1);
        }
        return(FALSE);
    }

    ddbg_assert2(((seg->state == BUFFER_READING) || (seg->state == BUFFER_WRITING)),
            "Trying to stop a non-active access\n");


    if(seg->state == BUFFER_READING) {
#if DEBUG >= 2
        fprintf (outputfile, "%f, Inside background read portion of stopable_access - read %d\n", simtime, (curr->flags & READ));
#endif
        currioreq = currdiskreq->ioreqlist;

        /* if active request, return FALSE if minreadaheadblkno hasn't been
       reached and the current request isn't a read hit on this segment
         */

        if((effective->ioreqlist != NULL) &&
                (seg->endblkno < seg->minreadaheadblkno) &&
                (!(currioreq->flags & READ) ||
                        (currdiskreq->seg != seg) ||
                        (currioreq->blkno < seg->startblkno) ||
                        (currioreq->blkno > seg->endblkno))) {
            return(FALSE);
        }

        if(!(currioreq->flags & READ) && (currdisk->write_hit_stop_readahead)) {
            while (currioreq) {
                if(disk_buffer_overlap(seg, currioreq)) {
                    seg->maxreadaheadblkno = seg->minreadaheadblkno = seg->endblkno;
                    break;
                }
                currioreq = currioreq->next;
            }
        }

        currioreq = currdiskreq->ioreqlist;

        /* Check for read hit or "almost read hit" (first blkno == block
       currently being prefetched.  If hit is indicated, first check to
       make sure that the passed-in request is the "best" request to
       receive the hda.  (In some cases, it may be better to let the
       prefetch reach the minimum and then go to another request 
       rather than take the hit immediately.)
         */

        if((currioreq->flags & READ) &&
                currdisk->enablecache &&
                (currdiskreq->seg == seg) &&
                (currioreq->blkno >= seg->startblkno) &&
                ((currioreq->blkno < seg->endblkno) ||
                        (currdisk->almostreadhits && (currioreq->blkno == seg->endblkno)))) {
            ioreq_event *bestioreq = ioqueue_show_next_request(currdisk->queue);

            if(!bestioreq ||
                    (bestioreq->ioreq_hold_diskreq == currdiskreq)) {
                return(TRUE);
            } else {
                /* This is a temporary kludge to fix the problem with SSTF/VSCAN
	   alternating between up and down for equal diff's         */
                bestioreq = ioqueue_show_next_request(currdisk->queue);
                if(bestioreq->ioreq_hold_diskreq == currdiskreq) {
                    return(TRUE);
                } else {
                    return(FALSE);
                }
            }
        }

        if((!(effective->flags & COMPLETION_SENT)
                && (currdisk->preseeking != PRESEEK_BEFORE_COMPLETION))
                || (!(effective->flags & COMPLETION_RECEIVED)
                        && (currdisk->preseeking == NO_PRESEEK)))
        {
            return(FALSE);
        }

        if(seg->endblkno < seg->minreadaheadblkno) {
            seg->maxreadaheadblkno = seg->minreadaheadblkno;
            return(FALSE);
        }

        if((seg->access->type == DEVICE_GOT_REMAPPED_SECTOR) ||
                (seg->access->type == DEVICE_GOTO_REMAPPED_SECTOR))
        {
            seg->outstate = BUFFER_PREEMPT;
            return(FALSE);
        }
        else if(seg->access->type == NULL_EVENT) {
            return(TRUE);
        }

        if((!currdisk->disconnectinseek)
                && ((seg->access->type == DEVICE_BUFFER_SEEKDONE)
                        || (!currdisk->stopinsector)))
        {
            int rv = removefromintq((event *)seg->access);
            ddbg_assert3(rv != 0,
                    ("Non-null seg->access does not appear on the intq. "
                            "seg->access->type: %d", seg->access->type));

            if(seg->access->type == DEVICE_BUFFER_SEEKDONE) {

                {
                    uint64_t nsecs;
                    struct dm_mech_state end;
                    struct dm_pbn pbn;

                    currdisk->model->layout->dm_translate_ltop(currdisk->model,
                            seg->access->blkno,
                            MAP_FULL,
                            &pbn,
                            0);

                    seg->access->cause = pbn.sector;

                    end.head = pbn.head;
                    end.cyl = pbn.cyl;
                    end.theta = 0;

                    // seg->access->time = seg->time +
                    //   diskacctime(currdisk,
                    //               seg->access->tempptr1,
                    //               DISKSEEK,
                    //               (seg->access->flags & READ),
                    //               seg->time,
                    //               global_currcylno,
                    //               global_currsurface,
                    //               seg->access->cause,
                    //               seg->access->bcount, 0);


                    nsecs = currdisk->model->mech->
                            dm_seek_time(currdisk->model,
                                    &currdisk->mech_state,
                                    &end,
                                    (seg->access->flags & READ));

                    // DISKSEEK so we modify the state
                    currdisk->mech_state.head = end.head;
                    currdisk->mech_state.cyl = end.cyl;

                    seg->access->time = seg->time + dm_time_itod(nsecs);
                }
            }
            DISK_SEEK_STOPTIME = seg->access->time;
            seg->access->type = NULL_EVENT;
            return(TRUE);
        }

        if((seg->access->type == DEVICE_BUFFER_SEEKDONE)
                || (!currdisk->stopinsector))
        {
            /* Must be SEEKDONE or SECTOR_DONE */
            seg->outstate = BUFFER_PREEMPT;
            return FALSE;
        }
        return TRUE;

    }
    else if(seg->state == BUFFER_WRITING) {
        if((currdiskreq->hittype == BUFFER_APPEND) &&
                (currdiskreq->seg == seg) &&
                (seg->endblkno == currdiskreq->ioreqlist->blkno) &&
                ((currdisk->fastwrites != LIMITED_FASTWRITE) ||
                        (ioqueue_get_number_pending(currdisk->queue) == 1))) {
            ioreq_event *bestioreq = ioqueue_show_next_request(currdisk->queue);

            if(!bestioreq || (bestioreq->ioreq_hold_diskreq == currdiskreq)) {
                return(TRUE);
            }
        }
    }

    /* Must deal with write overlap with ongoing writes here, if care to */

    return(FALSE);
}


// disk_buffer_seekdone() used to recompute the time after the seek
// and check it against the value computed in disk_initiate_seek()
static void
dbskdone_check_times(void) {
#if 0
    {
        uint64_t nsecs;
        struct dm_mech_state end;
        struct dm_pbn pbn;
        currdisk->model->layout->dm_translate_ltop(currdisk->model,
                curr->blkno,
                MAP_FULL,
                &pbn,
                0);

        end.cyl = pbn.cyl;
        end.head = pbn.head;
        end.theta = 0;
        curr->cause = pbn.sector;


        //   seektime = diskacctime(currdisk,
        //                          curr->tempptr1,
        //                          DISKSEEK,
        //                      (curr->flags & READ),
        //                      seg->time,
        //                      global_currcylno,
        //                      global_currsurface,
        //                      curr->cause,
        //                      curr->bcount,
        //                      0);

        nsecs =
                currdisk->model->mech->dm_seek_time(currdisk->model,
                        &currdisk->mech_state,
                        &end,
                        (curr->flags & READ));
        // DISKSEEK so modify the state
        currdisk->mech_state.cyl = end.cyl;
        currdisk->mech_state.head = end.head;
        curr->time = seg->time + dm_time_itod(nsecs);
    }

    mydiff = fabs(curr->time - simtime);

    /* This is purely for self-checking.  Can be removed. */
    if((mydiff > 0.0001) && (mydiff > (0.00000000001 * simtime)))
    {
        fprintf(stderr, "Access times don't match (1) - exp. %f  actual %f  mydiff %f\n", simtime, curr->time, mydiff);
        assert(0);
    }
#endif 
}

static void 
disk_buffer_seekdone(disk *currdisk, ioreq_event *curr)
{
    int trackstart;
    int immed;
    diskreq *currdiskreq = currdisk->effectivehda;
    segment *seg;
    double mydiff;
    struct dm_pbn pbn;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: [%s] - Entering type %d, cause %d, devno %d, blkno %d, bcount %d\n", simtime, __func__, curr->type, curr->cause, curr->devno, curr->blkno, curr->bcount );
#endif

    // first and last blocks on current track
    int first, last;

    int remapsector = 0;

    seg = currdiskreq->seg;

    dbskdone_check_times();
    // state-update stolen from above
    currdisk->model->layout->dm_translate_ltop(currdisk->model,
            curr->blkno,
            MAP_FULL,
            &pbn,
            0);
    currdisk->mech_state.cyl = pbn.cyl;
    currdisk->mech_state.head = pbn.head;

    curr->time = simtime;

    if(seg->outstate == BUFFER_PREEMPT) {
        curr->type = NULL_EVENT;
        disk_release_hda(currdisk,currdiskreq);
        return;
    }

    if(currdisk->stat.xfertime == (double) -1) {
        currdisk->stat.latency = simtime;
    }

    immed = currdisk->immed;

    {
        struct dm_pbn pbn;
        dm_ptol_result_t rv;

        currdisk->model->layout->dm_translate_ltop(currdisk->model,
                curr->blkno,
                MAP_FULL,
                &pbn,
                NULL);

        rv = currdisk->model->layout->
                dm_get_track_boundaries(currdisk->model,
                        &pbn,
                        &first,
                        &last,
                        &remapsector);
        ddbg_assert(rv == DM_OK);

        currdisk->track_low = first;    // first LBN on track
        currdisk->track_high = last;    // last LBN on track

        curr->bcount = last - first + 1;
        last++;  // the controller code works in terms of 1-past-the-end

        ddbg_assert3(((first <= curr->blkno) && (curr->blkno <= last )),
                ("Mapping problem, block (%d) not within track (%d, %d)",
                        curr->blkno,
                        first,
                        last));
    }

#ifdef DEBUG_DISKCTLR
    //  fprintf (outputfile, "********** %f: [%s] - remapsector %d, endoftrack %d, firstontrack %d, bcount %d\n", simtime, __func__, remapsector, last, first, curr->bcount );
#endif

    /* If the head landed just start reading, even if the block we want
     * isn't under the head yet.  -rcohen */
    if((currdisk->readanyfreeblocks)
            && (curr->flags & READ)
            && (curr->blkno != first))
    {
        curr->blkno = first;
        immed = TRUE;
    }

    seg->time = simtime;
    {
        int sectors;
        uint64_t nsecs;
        double latency;
        struct dm_pbn tmp;

        // watch carefully: to match the screwy way we did this with
        // global_currangle/global_currtime/foo before, we're going to
        // set the angle of the state we pass in to be (simtime % period)

        dm_time_t isimtime = dm_time_dtoi(seg->time) - currdisk->currtime_i;
        dm_time_t residtime =
                isimtime % currdisk->model->mech->dm_period(currdisk->model);

        struct dm_mech_state startstate = currdisk->mech_state;
        startstate.theta += currdisk->model->mech->dm_rotate(currdisk->model, &residtime);
        nsecs = currdisk->model->mech->dm_latency(currdisk->model,
                &startstate,
                curr->cause,
                curr->bcount,
                immed,
                0);
        latency = dm_time_itod(nsecs);

        //    printf("%s(): latency %f\n", __func__, latency);

        trackstart =
                currdisk->model->mech->dm_access_block(currdisk->model,
                        &startstate,
                        curr->cause,
                        curr->bcount,
                        immed);

        tmp.cyl = startstate.cyl;
        tmp.head = startstate.head;
        tmp.sector = 0; // XXX g4 layout cares
        sectors = currdisk->model->layout->dm_get_sectors_pbn(currdisk->model, &tmp);

        //    printf("*** DEBUG disk_buffer_seekdone : simtime = %f, trackstart = %d\n",
        //	   simtime, trackstart);

        curr->time = seg->time + latency;   // latency added to the simtime here to generate next event
    }

    currdisk->immedstart = last;
    currdisk->immedend = currdisk->immedstart;

    // used to have curr->blkno = first + trackstart here
    // At the very minimum, slips will screw this up.
    {
        struct dm_pbn tmppbn =
        {
                currdisk->mech_state.cyl,
                currdisk->mech_state.head,
                trackstart
        };

        // hurst_r the following code is a type error
        curr->blkno = currdisk->model->layout->dm_translate_ptol(currdisk->model, &tmppbn, 0);
        ddbg_assert(curr->blkno >= 0);
    }

    ddbg_assert(currdisk->track_low <= curr->blkno && curr->blkno <= currdisk->track_high);


    /* NOTE: trackstart != current pbn in track, think of zero latency
     * and early-start reads
     */
    curr->cause = trackstart;
    curr->type = DEVICE_BUFFER_SECTOR_DONE;
    curr->bcount = 0;
    addtointq((event *) curr);

#ifdef DEBUG_DISKCTLR
    //  fprintf (outputfile, "********** %f: [%s] - startblkno %d, endblkno %d, outblkno %d, reqstart %d, reqend %d\n",
    //       seg->startblkno,
    //       seg->endblkno,
    //       currdiskreq->outblkno,
    //       seg->diskreqlist->blkno,
    //       (seg->reqlist->blkno + seg->reqlist->bcount));

    //  fprintf (outputfile, "********** %f: [%s] - startblkno %d, endblkno %d, outblkno %d\n",
    //       simtime,
    //       __func__,
    //       seg->startblkno,
    //       seg->endblkno,
    //       currdiskreq->outblkno );

    //  fprintf (outputfile, "********** %f: [%s] - leaving\n", simtime, __func__ );
    //  fflush( outputfile );
#endif
}



// Factored out of disk_buffer_sector_done, rewritten to
// make fewer layout assumptions.
static int
dbsd_setup(disk *cd, 
        ioreq_event *curr,
        int *blkno,
        int *cause,
        int *first,
        int *last,
        int *next,
        int *remapsector)
{
    int rv;
    struct dm_pbn pbn;


    // old way:
    // p = (mech_state,0)
    // get spt for p
    // currcause = (curr->cause) ? (curr->cause-1) : (blks_on_track-1);
    // get track boundaries for mech_state
    // currblkno = ptol(mech_state, currcause)

    // equivalent to?
    // currblkno = curr->blkno == first ? last : curr->blkno - 1
    // currcause = ltop(currblkno)

    //    pbn.cyl = cd->mech_state.cyl;
    //    pbn.head = cd->mech_state.head;
    //    pbn.sector = curr->cause;

    //   rv = cd->model->layout->dm_translate_ltop(cd->model,
    // 					    curr->blkno,
    // 					    MAP_FULL,
    // 					    &pbn,
    // 					    remapsector);

    //   ddbg_assert(pbn.cyl == cd->mech_state.cyl);
    //   ddbg_assert(pbn.head == cd->mech_state.head);

    //     rv =
    //       cd->model->layout->dm_get_track_boundaries(cd->model,
    //   					       &pbn,
    //   					       first,
    //   					       last,
    //   					       remapsector);

    //     ddbg_assert(rv == DM_OK);
    //     ddbg_assert(*first <= *last);

    //     ddbg_assert(*first == cd->track_low);
    //     ddbg_assert(*last == cd->track_high);

    *first = cd->track_low;
    *last = cd->track_high;

    ddbg_assert((*first <= curr->blkno) && (curr->blkno <= *last));

    *next = *last+1;

    if(curr->blkno > *first) {
        *blkno = curr->blkno - 1;
    }
    else {
        *blkno = *last;
    }

    rv =
            cd->model->layout->dm_translate_ltop(cd->model,
                    *blkno,
                    MAP_FULL,
                    &pbn,
                    remapsector);
    ddbg_assert(rv == DM_OK);

    *cause = pbn.sector;

    return 0;
}

// dbsd_update_state 

//  like dbskd_check_times() -- recompute and doublecheck.  
//  In this instance, the rotation time.
void dbsd_check_times(void) 
{
#if 0

    // this looks awfully belabored; all we're doing is rotating from
    // the current angle to the one for the requested sector

    uint64_t nsecs;
    struct dm_pbn pbn;

    // watch carefully: to match the screwy way we did this with
    // global_currangle/global_currtime/... before, we're going to
    // set the angle of the state we pass in to be (simtime % period)

    dm_time_t reqtime = dm_time_dtoi(seg->time) - currdisk->currtime_i;
    // dm_time_t reqtime = dm_time_dtoi(seg->time);

    dm_time_t residtime = 
            reqtime % currdisk->model->mech->dm_period(currdisk->model);

    struct dm_mech_state startstate = currdisk->mech_state;
    startstate.theta += currdisk->model->mech->dm_rotate(currdisk->model, 
            &residtime);

    pbn.cyl = currdisk->mech_state.cyl; 
    pbn.head = currdisk->mech_state.head;
    pbn.sector = curr->cause;

    // XXX evil old diskacctime() reqtime foo 
    // seg->time, 

    nsecs = currdisk->model->mech->dm_pos_time(currdisk->model, 
            &startstate,
            &pbn,
            1,
            (curr->flags & READ),
            0, // immed
            0); // breakdown
    // (was DISKPOS)
    currdisk->mech_state.cyl = pbn.cyl;
    currdisk->mech_state.head = pbn.head;

    // rotate the disk according to the positioning time
    currdisk->mech_state.theta = startstate.theta +
            currdisk->model->mech->dm_rotate(currdisk->model, &nsecs);

    // set currdisk->currtime to reqtime + postime


    currdisk->currtime = seg->time + dm_time_itod(nsecs);
    currdisk->currtime_i = dm_time_dtoi(seg->time) + nsecs;

    curr->time = seg->time + dm_time_itod(nsecs);

    mydiff = fabs(curr->time - simtime);


    /* This is purely for self-checking.  Can be removed. */
    ddbg_assert3((mydiff <= 0.00001) || (mydiff <= (0.00001 * simtime)),
            ("Times don't match -- exp %f real %f\n"
                    "devno %d, blkno %d, bcount %d, bandno (trash) %p, "
                    "blkinband %d",
                    simtime,
                    curr->time,
                    curr->devno,
                    curr->blkno,
                    curr->bcount,
                    curr->tempptr1,
                    (curr->blkno - bandstart)));


#endif
}

// dbsd_next_sector

static void 
disk_buffer_sector_done (disk *currdisk, ioreq_event *curr)
{
    segment *seg;
    diskreq *currdiskreq = currdisk->effectivehda;
    ioreq_event *tmpioreq;

    int first, last;    // first/last lbn on track
    int next; // first lbn on next track
    int firstblkno;
    int lastblkno;
    int endlat = 0;

    // As far as I can tell, these are set up to be the lbn and sector
    // prior (rotationally, on the track) to curr->blkno and curr->cause.
    int currblkno = -1;
    int currcause = -1;

    dm_time_t diff_i;
    double mydiff;
    int remapsector = 0;

    int rv;
    struct dm_pbn pbn;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_buffer_sector_done - Entering  type %d, cause %d, devno %d, blkno %d, bcount %d\n", simtime, curr->type, curr->cause, curr->devno, curr->blkno, curr->bcount );
    fflush( outputfile );
#endif

    //  currdisk->fpcheck--;
    //  ddbg_assert(currdisk->fpcheck); // see the comment in the header

    //  printf("%s %d %d\n", __func__, curr->blkno, curr->cause);

    ddbg_assert2(currdiskreq != 0, "No effectivehda");

    seg = currdiskreq->seg;

    dbsd_setup(currdisk, curr,
            &currblkno, &currcause,
            &first, &last, &next,
            &remapsector);

    //   printf("%s():%d curr->blkno %d\n", __func__, __LINE__, curr->blkno);
    //   printf("%s():%d curr->cause %d\n", __func__, __LINE__, curr->cause);
    //   printf("%s():%d currblkno %d\n", __func__, __LINE__, currblkno);
    //   printf("%s():%d currcause %d\n", __func__, __LINE__, currcause);
    //   printf("%s():%d first %d\n", __func__, __LINE__, first);
    //   printf("%s():%d last %d\n", __func__, __LINE__, next);


    if(remapsector | (currblkno < 0)) {
        /* previous pbn is slipped or relocated */
        currblkno = -1;
        if(curr->bcount) {
            curr->bcount = 2;
        }
    }


    if(!curr->bcount) { // first sector after repos (? see comment below)
        curr->time = simtime;

        diff_i = (dm_time_dtoi(seg->time) - currdisk->currtime_i) % currdisk->model->mech->dm_period(currdisk->model);
        currdisk->mech_state.theta += currdisk->model->mech->dm_rotate(currdisk->model, &diff_i);
    }


    // update the rotational state of the disk
    diff_i = dm_time_dtoi(simtime - seg->time);
    currdisk->mech_state.theta += currdisk->model->mech->dm_rotate(currdisk->model, &diff_i );

    // for bcount == 0, used to dbsd_check_times() here

    currdisk->currtime = simtime;
    currdisk->currtime_i = dm_time_dtoi(simtime);


    if(!currdisk->read_direct_to_buffer
            && (curr->flags & READ)
            && (curr->bcount == 1)
            && !(disk_buffer_block_available(currdisk, seg, currblkno)))
    {
        curr->bcount = 2;
    }


    /* OK, here comes the weird stuff. bcount doesn't mean what you think.
     * Ganger has seen fit to overload its meaning. bcount may equal 0, 1
     * or 2.
     * 0 = first block read/written after a seek?
     * 1 = middle of reading/writing?
     * 2 = previous sector is remapped or slipped?
     * 2 = writing - searching for first block
     * If you are able to confirm or deny, please fix comments.
     */
    if(seg->recyclereq == currdiskreq) {
        curr->bcount = 2;
        firstblkno = currdiskreq->inblkno;
        tmpioreq = currdiskreq->ioreqlist;
        while (tmpioreq->next) {
            tmpioreq = tmpioreq->next;
        }
        lastblkno = min((tmpioreq->blkno + tmpioreq->bcount), next);
    }
    else if(seg->state == BUFFER_READING) {
        firstblkno = seg->endblkno;
        if(curr->flags & BUFFER_BACKGROUND) {
            lastblkno = min(seg->maxreadaheadblkno, next);
        }
        else {
            tmpioreq = currdiskreq->ioreqlist;
            while (tmpioreq->next) {
                tmpioreq = tmpioreq->next;
            }
            lastblkno = min((tmpioreq->blkno + tmpioreq->bcount), next);
        }
    } 
    else {
        firstblkno = currdiskreq->inblkno;
        lastblkno = min(seg->endblkno, next);
    }

    if(currdisk->stat.xfertime == -1.0) {
        endlat++;
    }


    ddbg_assert((0 <= curr->bcount) && (curr->bcount <= 2));

    // It seems like we "dry-fire" d_b_s_d() for slips -- we simulate
    // the rotation of a sector but don't do any of the following code
    // which simulates the media xfer.
    if(curr->bcount == 1) {
        if(currblkno == firstblkno) {
            /* Oh look, we read a wanted block */

            // oops ... what if firstblkno is the last block on the disk! bucy 2001

            // ewww ... this assertion fails in disksim-2 ... so reading the
            // last+1 sector must mean something special to the rest of the
            // controller code

            firstblkno++;
            ddbg_assert(firstblkno < (currdisk->model->dm_sectors+1));
            if(firstblkno == currdisk->immedstart) {
                firstblkno = currdisk->immedend;

                currdisk->immedstart = next + 1;
                currdisk->immedend = currdisk->immedstart;
            }
            endlat++;
        }
        else if(currblkno < seg->startblkno) {
            /* We've started reading (NOT writing) before the
             * starting block. Must be buffering
             */
            ddbg_assert3(((seg->state == BUFFER_READING)
                    && currdisk->readanyfreeblocks),
                    ("seg->state 0x%x, readanyfreeblocks %d",
                            seg->state,
                            currdisk->readanyfreeblocks));

            seg->startblkno = currblkno;
            firstblkno = currblkno + 1;
            ddbg_assert(firstblkno < currdisk->model->dm_sectors);
        }
        else if((currdisk->immed)
                && (currdisk->immedstart >= next))
        {
            /* zero latency, still figuring this one out */
            if(currblkno >= lastblkno) {
                if((seg->state == BUFFER_READING)
                        && (currdisk->contread != BUFFER_NO_READ_AHEAD)
                        && (currblkno < currdiskreq->inblkno))
                {
                    currdisk->immedstart = currblkno;
                    currdisk->immedend = currblkno + 1;
                }
            }
            else if(currblkno > firstblkno) {
                currdisk->immedstart = currblkno;
                currdisk->immedend = currblkno + 1;
                endlat++;
            }
        }
        else if((currblkno == currdisk->immedend)
                && ((currblkno < lastblkno)
                        || (currblkno < currdiskreq->inblkno)))
        {
            currdisk->immedend++;
            disk_buffer_segment_wrap(seg, currdisk->immedend);
        }
        else if((currblkno > currdisk->immedend) && (currblkno < lastblkno)) {
            currdisk->immedstart = currblkno;
            currdisk->immedend = currblkno + 1;
        }
    } // if(curr->bcount == 1)

    /* This may have to do with trackacc, in which case it will be removed */
    if((currdiskreq != seg->recyclereq) &&
            (seg->outstate == BUFFER_TRANSFERING)) {
        disk_buffer_update_outbuffer(currdisk, seg);
    }


    /* Statistics gathering */
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_sector_done before - seg->time %f, seekdistance %d, seektime %f, latency %f, xfertime %f\n",
            simtime,
            seg->time,
            currdisk->stat.seekdistance,
            currdisk->stat.seektime,
            currdisk->stat.latency,
            currdisk->stat.xfertime );
    fflush( outputfile );
#endif

    if(endlat == 2) {
        currdisk->stat.latency = seg->time - currdisk->stat.latency;
        currdisk->stat.xfertime = (double) 0.0;
#ifdef DISKLOG_ON
    // this conditional concatenates it's output with the conditional in disk_initiate_seek
        if( NULL != DISKSIM_DISK_LOG )
        {
            fprintf (DISKSIM_DISK_LOG, ", latency %f, xfertime %f  SEEK\n",
                    currdisk->stat.latency,
                    currdisk->stat.xfertime );
            fflush( DISKSIM_DISK_LOG );
        }
#endif

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_buffer_sector_done after - endlat==2  seg %p, seg->time %f, seekdistance %d, seektime %f, latency %f, xfertime %f\n",
            simtime,
            seg,
            seg->time,
            currdisk->stat.seekdistance,
            currdisk->stat.seektime,
            currdisk->stat.latency,
            currdisk->stat.xfertime );
    fflush( outputfile );
#endif
    }

    if(currdisk->stat.xfertime != (double) -1) {
        currdisk->stat.xfertime += simtime - seg->time;
#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_sector_done after - xfertime!=-1.0  seg->time %f, seekdistance %d, seektime %f, latency %f, xfertime %f\n",
            simtime,
            seg->time,
            currdisk->stat.seekdistance,
            currdisk->stat.seektime,
            currdisk->stat.latency,
            currdisk->stat.xfertime );
    fflush( outputfile );
#endif
    }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_sector_done after - seg->time %f, seekdistance %d, seektime %f, latency %f, xfertime %f\n",
            simtime,
            seg->time,
            currdisk->stat.seekdistance,
            currdisk->stat.seektime,
            currdisk->stat.latency,
            currdisk->stat.xfertime );
    fflush( outputfile );
#endif

    if(currdiskreq != seg->recyclereq) {
        if(seg->state == BUFFER_READING) {
            seg->endblkno = firstblkno;
        }
        else {
            currdiskreq->inblkno = firstblkno;
        }

        disk_buffer_segment_wrap(seg, seg->endblkno);

        if(disk_buffer_request_complete(currdisk, currdiskreq)) {
            /* fprintf (outputfile, "Leaving disk_buffer_sector_done\n"); */
            return;
        }
    }


    // In the case of immediate-access, firstblkno gets fiddled above.

    // The request still isn't done.  We're either done with this
    // track in which case we'll reposition to the next
    // or we'll rotate and do another sector.
    if(firstblkno >= next) {
        // We reached the end of the track, seek to the next.

        curr->blkno = next;
        if(!(disk_initiate_seek(currdisk, seg, curr, 0, 0.0, 0.0))) {
            disk_release_hda(currdisk,currdiskreq);
            return;
        }
    } 
    // is it firstblkno > last ok?
    else {
        // Here, we need to set up curr->blkno and curr->cause to reflect
        // moving forward one sector (rotationally, on this track).

        // old way:
        // 1. subtract current sector number (curr->cause) out of current
        // lbn
        // 2. increment current sector number modulo the track length
        // 3. add the sector number back into the lbn


        if(curr->blkno < last) {
            curr->blkno++;
        }
        else {
            curr->blkno = first;
        }

        //    ddbg_assert(curr->blkno != 54281913);
        ddbg_assert(curr->blkno <= currdisk->track_high);

        rv =
                currdisk->model->layout->dm_translate_ltop(currdisk->model,
                        curr->blkno,
                        MAP_FULL,
                        &pbn,
                        &remapsector);
        // save this here
        currcause = curr->cause;
        curr->cause = pbn.sector;

        //    printf("%s():%d curr->blkno %d\n", __func__, __LINE__, curr->blkno);
        //        printf("%s():%d curr->cause %d\n", __func__, __LINE__, curr->cause);

        /* NOTE: curr->blkno may be < start_of_track
         * ie if there is a slipped blk in the track and sectpercylspare.  I
         * think this is messy and end_of_track should specify the # of
         * the last track so we don't have to do this.  -rcohen */
        /* end of bad assumption code */

        seg->time = simtime;

        curr->time = seg->time + dm_time_itod(currdisk->model->mech->dm_xfertime(currdisk->model, &currdisk->mech_state, 1));


        // this seems like a type error; currcause was a sector number, now its
        // an lbn.
        {
            int newcurrcause;
            struct dm_pbn pbn;
            pbn.cyl = currdisk->mech_state.cyl;
            pbn.head = currdisk->mech_state.head;
            pbn.sector = currcause;

            newcurrcause = currdisk->model->layout->dm_translate_ptol(
                    currdisk->model,
                    &pbn,
                    &remapsector);

            currcause = newcurrcause; // currcause becomes an LBN here
        }

        if(remapsector) {
            currcause = DM_SLIPPED; // DM_SLIPPED
        }

        if((currdiskreq != seg->recyclereq)
                && (currcause == -2) // DM_REMAPPED
                && (seg->state == BUFFER_READING)
                && (currdiskreq->ioreqlist)
                && (currdisk->readanyfreeblocks)
                && (currblkno >= 0)
                && (currblkno == (seg->endblkno-1))
                && (seg->endblkno < currdiskreq->ioreqlist->blkno)
                && (curr->cause > 0))
        {
            seg->startblkno = currdiskreq->ioreqlist->blkno;
            seg->endblkno = seg->startblkno;
            currcause = DM_SLIPPED; // DM_SLIPPED
        }

        if((currcause == DM_REMAPPED)  // DM_REMAPPED
                && (currblkno >= 0)
                && (currblkno == (firstblkno-1)))
        {
            curr->blkno--;

            if(curr->cause == 0) {
                curr->blkno = next;
            }

            disk_get_remapped_sector(currdisk, curr);
        }
        else {
            curr->bcount = 1;
            if(((currdisk->read_direct_to_buffer
                    || !(curr->flags & READ))
                    && ((currdiskreq == seg->recyclereq)
                            || !(disk_buffer_block_available(currdisk, seg, currcause))))
                    || (currcause < 0))
            {
                curr->bcount = 2;
            }
        }

        addtointq((event *) curr);

    } // else  ( firstblkno != (<) last)


} // disk_buffer_sector_done()





/*** FUNCTIONS FOR USE BY QUEUING ALGORITHMS  -rcohen ***/




/* Possible improvements:  rundelay is not used currently */
/* NOTE: this is used for calculating actual access times -rcohen */

static double 
disk_buffer_estimate_servtime(disk *currdisk, 
        ioreq_event *curr,
        int checkcache,
        double maxtime)
{
    struct dm_pbn destpbn;
    struct dm_mech_acctimes mech_acctimes;
    double tmptime;
    int tmpblkno;
    int lastontrack;
    int hittype = BUFFER_NOMATCH;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_estimate_servtime\n", simtime );
    fflush(outputfile);
#endif

    if(currdisk->const_acctime) {
        // SIMPLEDISK device path
        return(currdisk->acctime);
    }

    // DISK device path
    if(checkcache) {
        int buffer_reading = FALSE;

        hittype = disk_buffer_check_segments(currdisk,curr,&buffer_reading);
        if(hittype == BUFFER_APPEND) {
            return 0.0;
        }
        else if(hittype == BUFFER_WHOLE) {
            return(buffer_reading ? READING_BUFFER_WHOLE_SERVTIME : BUFFER_WHOLE_SERVTIME);
        }
        else if(hittype == BUFFER_PARTIAL) {
            return(buffer_reading ? READING_BUFFER_PARTIAL_SERVTIME : BUFFER_PARTIAL_SERVTIME);
        }
    }

    tmpblkno = curr->bcount;  // TODO hurst_r need to investigate this...save/restore bcount??
    tmptime = curr->time;


    {
        uint64_t nsecs;
        struct dm_mech_state end;
        //curr->tempptr1 =
        currdisk->model->layout->dm_translate_ltop(currdisk->model,
                curr->blkno,
                MAP_FULL,
                &destpbn,
                0);
        end.cyl = destpbn.cyl;
        end.head = destpbn.head;
        end.theta = 0;
        curr->cause = destpbn.sector;

        // was diskacctime(DISKSEEKTIME)
        nsecs =
                currdisk->model->mech->dm_seek_time(currdisk->model,
                        &currdisk->mech_state,
                        &end,
                        (curr->flags & READ));

        curr->time = dm_time_itod(nsecs);
    }

    if (curr->time < maxtime) {
        curr->time = tmptime;

        currdisk->model->layout->dm_get_track_boundaries(currdisk->model, &destpbn, 0, &lastontrack, 0);
        // track_boundaries new semantics
        lastontrack++;

        curr->bcount = min(curr->bcount, (lastontrack - curr->blkno));

        {
            dm_time_t nsecs;
            struct dm_pbn pbn;
            currdisk->model->layout->dm_translate_ltop(currdisk->model,
                    curr->blkno, MAP_FULL, &pbn, 0);
            curr->cause = pbn.sector;

            if (curr->flags & READ) {
                currdisk->immed = currdisk->immedread;
            } else {
                currdisk->immed = currdisk->immedwrite;
            }

            // It was decided that "servtime" was extremely confusing so we
            // are now referring to it as "non-xfer" time.  it consists of
            // all of the time taken to service the request excluding the
            // actual data transfer, i.e. additional intermediate rotational
            // latency in a zero-latency access, etc.  bucy 5/20/2002.

            // having said all that, it isn't obvious to me that non-xfer
            // time is wanted instead of access time here...

            /* tmptime = diskacctime(currdisk, */
            /*                       curr->tempptr1, */
            /*                       DISKSERVTIME, */
            /*                       (curr->flags & READ), */
            /*                       curr->time, */
            /*                       global_currcylno, */
            /*                       global_currsurface, */
            /*                       curr->cause, */
            /*                       curr->bcount, */
            /*                       currdisk->immed); */

            // dm doesn't provide a direct interface to non-xfer time; we
            // obtain it by subtracting xfertime from acctime
            nsecs = currdisk->model->mech->dm_acctime(currdisk->model,
                    &currdisk->mech_state, &pbn, curr->bcount,
                    (curr->flags & READ), currdisk->immed, 0,  // result state
                    &mech_acctimes); // breakdown
//                    0); // breakdown

            nsecs -= currdisk->model->mech->dm_xfertime(currdisk->model,
                    (struct dm_mech_state *) &pbn, curr->bcount);

            tmptime = dm_time_itod(nsecs);
        }
        curr->bcount = tmpblkno;  // TODO hurst_r need to investigate this...save/restore bcount??
        if ((!(curr->flags & READ))
                && (tmptime < currdisk->minimum_seek_delay)) {
            tmptime = currdisk->minimum_seek_delay;
        }
    }
    else { // hurst_r  if nothing else returns maxtime (passed by call) + 1
        tmptime = maxtime + 1.0;
    }

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_estimate_servtime  servtime %f, currcyl %d, currhead %d, currangle %u, destcyl %d, desthead %d, destsector %d\n",
            simtime, tmptime, currdisk->mech_state.cyl, currdisk->mech_state.head, currdisk->mech_state.theta, destpbn.cyl, destpbn.head, destpbn.sector );
    fflush(outputfile);
#endif

    return tmptime;
}



/* Possible improvements:  rundelay is not used currently */

static double
disk_buffer_estimate_seektime(
        disk *currdisk,
        ioreq_event *curr,
        int checkcache,
        double maxtime)
{
    double tmptime;
    int hittype = BUFFER_NOMATCH;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_buffer_estimate_seektime  const_seektime %d, checkcache %d\n", simtime, currdisk->const_seektime, checkcache );
    fflush(outputfile);
#endif

    if( currdisk->const_seektime ) {
        return currdisk->seektime;
    }

    if(checkcache) {
        int buffer_reading = FALSE;
        hittype = disk_buffer_check_segments(currdisk,curr,&buffer_reading);

        switch(hittype) {
        case BUFFER_APPEND:
            return 0.0;

        case BUFFER_WHOLE:
            if(buffer_reading) {
                return READING_BUFFER_WHOLE_SERVTIME;
            }
            else {
                return BUFFER_WHOLE_SERVTIME;
            }

        case BUFFER_PARTIAL:
            if(buffer_reading) {
                return READING_BUFFER_PARTIAL_SERVTIME;
            }
            else {
                return BUFFER_PARTIAL_SERVTIME;
            }

        default:
            assert(0);
            break;
        }
    }


    {
        dm_time_t nsecs;
        struct dm_pbn pbn;

        currdisk->model->layout->dm_translate_ltop(
                currdisk->model,
                curr->blkno,
                MAP_FULL,
                &pbn,
                0);

        nsecs = currdisk->model->mech->dm_seek_time(
                currdisk->model,
                &currdisk->mech_state,
                (struct dm_mech_state *)&pbn,
                (curr->flags & READ));

        tmptime = dm_time_itod(nsecs);
    }

    if(tmptime < maxtime)
    {
        if( (!(curr->flags & READ)) && (tmptime < currdisk->minimum_seek_delay) )
        {
            tmptime = currdisk->minimum_seek_delay;
        }
    }
    //    else
    //    {
    //        tmptime = maxtime + 1.0;
    //    }

    return tmptime;
}

/* Figure out the seek distance that would be incurred for given
 * request.  req is destination, exact provides the start lbn (or -1
 * indicates use disk's current <cylno,head> location), direction
 * indicates that head switches should be counted as a distance of 1.
 */

int disk_get_distance(int diskno, ioreq_event *req, int exact, int direction)
{
    disk *currdisk = getdisk (diskno);
    int cyl1, head1;
    int cyl2, head2;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_distance\n", simtime );
    fflush(outputfile);
#endif

    if(exact == -1) {
        cyl1 = currdisk->mech_state.cyl;
        head1 = currdisk->mech_state.head;
    }
    else {
        struct dm_pbn pbn;
        currdisk->model->layout->dm_translate_ltop(currdisk->model,
                exact,
                MAP_FULL,
                &pbn,
                0);

        cyl1 = pbn.cyl;
        head1 = pbn.head;
    }
    {
        struct dm_pbn pbn;
        currdisk->model->layout->dm_translate_ltop( currdisk->model, req->blkno, MAP_FULL, &pbn, 0);
        cyl2 = pbn.cyl;
        head2 = pbn.head;
    }

    if(cyl1 > cyl2) {
        return (cyl1 - cyl2);
    }
    else if(cyl1 < cyl2) {
        return (cyl2 - cyl1);
    }
    else if((direction) && (head1 != head2)) {
        /* Note: interpreting direction to mean "count headswitch as dist=1" */
        return 1;
    }

    return 0;
}


/* returns the service time for the request
 * takes into account the cache -rcohen 
 */

double 
disk_get_servtime(int diskno, 
        ioreq_event *req,
        int checkcache,
        double maxtime)
{
    double servtime;
    disk *currdisk = getdisk (diskno);

    servtime = disk_buffer_estimate_servtime(currdisk, req, checkcache, maxtime);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_servtime  devno %d, blkno %d, bcount %d, flags %x\n", simtime, req->devno, req->blkno, req->bcount, req->flags );
    fprintf (outputfile, "********** %f: disk_get_servtime  checkcache %d, maxtime %f, servtime %f\n", simtime, checkcache, maxtime, servtime );
    fflush(outputfile);
#endif

    return(servtime);
}


/* returns the seek time for the request
 * takes into account the cache -rcohen 
 */

double 
disk_get_seektime(int diskno,
        ioreq_event *req,
        int checkcache,
        double maxtime)
{
    double seektime;
    disk *currdisk = getdisk (diskno);

    seektime = disk_buffer_estimate_seektime(currdisk, req, checkcache, maxtime);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_seektime  devno %d, blkno %d, bcount %d, flags %x\n", simtime, req->devno, req->blkno, req->bcount, req->flags );
    fprintf (outputfile, "********** %f: disk_get_seektime  checkcache %d, maxtime %f, seektime %f\n", simtime, checkcache, maxtime, seektime );
    fflush(outputfile);
#endif

    return(seektime);
}



/* returns request access time, ignores the cache -rcohen */
double disk_get_acctime (int diskno, ioreq_event *req, double maxtime)
{
    double acctime;
    disk *currdisk = getdisk (diskno);

    acctime = disk_buffer_estimate_acctime(currdisk, req, maxtime);

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "********** %f: disk_get_acctime  devno %d, blkno %d, bcount %d, flags %x\n", simtime, req->devno, req->blkno, req->bcount, req->flags );
    fprintf (outputfile, "********** %f: disk_get_acctime  diskno %d, maxtime %f, acctime %f\n", simtime, diskno, maxtime, acctime );
    fflush(outputfile);
#endif

    return(acctime);
}


// this looks completely deprecated; 
// some queueing strategies in ioqueue code seem to use it ... bucy (1/02)
static double disk_buffer_estimate_acctime(disk *currdisk, ioreq_event *curr, double maxtime)
{
    double tpass = 0.0;

    /* acctime estimation is not currently supported */
    ddbg_assert(FALSE);

    return(tpass);
}


double disk_calc_rpo_time( disk *currdisk, ioreq_event *curr, struct dm_mech_acctimes *mech_acctimes )
{
    struct dm_pbn destpbn;
    dm_time_t nsecs;
    double tmptime;
    int saveblkno, savetime;
    int lastontrack;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_calc_rpo_time  currdisk %p\n", simtime, currdisk );
    dumpIOReq( "disk_calc_rpo_time", curr );
    fflush(outputfile);
#endif

    saveblkno = curr->bcount;  // TODO hurst_r need to investigate this...save/restore bcount??
    savetime = curr->time;     // time

    currdisk->model->layout->dm_translate_ltop(currdisk->model, curr->blkno, MAP_FULL, &destpbn, 0);
    currdisk->model->layout->dm_get_track_boundaries(currdisk->model, &destpbn, 0, &lastontrack, 0);
    // track_boundaries new semantics
    lastontrack++;

    curr->bcount = min(curr->bcount, (lastontrack - curr->blkno));


    nsecs = currdisk->model->mech->dm_acctime(
                currdisk->model,
                &currdisk->mech_state, &destpbn, curr->bcount,
                (curr->flags & READ), currdisk->immed, 0,  // result state
                mech_acctimes); // breakdown

    tmptime = dm_time_itod(nsecs);

    curr->bcount = saveblkno;  // TODO hurst_r need to investigate this...save/restore bcount??
    curr->time   = savetime;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_calc_rpo_time  servtime %f, currcyl %d, currhead %d, currangle %u, destcyl %d, desthead %d, destsector %d\n",
            simtime, tmptime, currdisk->mech_state.cyl, currdisk->mech_state.head, currdisk->mech_state.theta, destpbn.cyl, destpbn.head, destpbn.sector );
    fprintf (outputfile, "%f: disk_calc_rpo_time  seektime %ld, initial_latency %ld, initial_xfer %ld\n", simtime, mech_acctimes->seektime, mech_acctimes->initial_latency, mech_acctimes->initial_xfer );
    fflush(outputfile);
#endif

    return tmptime;
}


//*****************************************************************************
// Function: disk_buffer_sort_cache_segments_by_RPO
//
// Sort the DIRTY segments in the cache buffer by ascending RPO time
//*****************************************************************************

CACHE_LBA_MAP disk_buffer_sort_dirty_cache_segments_by_RPO( disk *currdisk )
{
	struct dm_mech_acctimes mech_acctimes;
    int index = 0;
    segment *currseg = currdisk->seglist;

    CACHE_LBA_MAP cacheLbaMap;

#ifdef DEBUG_DISKCACHE
    fprintf (outputfile, "%f: disk_buffer_sort_cache_segments_by_RPO  currseg %p, numdirty %d\n", simtime, currseg, currdisk->numdirty );
    fflush( outputfile );
#endif

    while (currseg) {
        if ( BUFFER_DIRTY == currseg->state ) {
            currseg->access->blkno = currseg->startblkno;
            currseg->access->bcount = currseg->outbcount;
            double servTime = disk_get_servtime( currseg->access->devno, currseg->access, FALSE, 30.0 );
            double rpoTime  = disk_calc_rpo_time( currdisk, currseg->access, &mech_acctimes );

#ifdef DEBUG_DISKCACHE
    fprintf (outputfile, "%f: disk_buffer_sort_cache_segments_by_RPO  mech_acctimes: currseg %p, seektime %ld, latency %ld, xfertime %ld\n", simtime, currseg, mech_acctimes.seektime, mech_acctimes.initial_latency, mech_acctimes.initial_xfer );
    fflush( outputfile );
#endif
            currseg->mech_acctimes = mech_acctimes;
            cacheLbaMap[rpoTime] = currseg;
        }
        currseg = currseg->next;
    }


#ifdef DEBUG_DISKCACHE
    fprintf (outputfile, "%f: disk_buffer_sort_cache_segments_by_RPO  Cache Map Dump:\n", simtime );
    for (CACHE_LBA_MAP::iterator i = cacheLbaMap.begin(); i != cacheLbaMap.end(); ++i)
    {
        fprintf( outputfile, "index %d: map[%f] = %p\n", index++, i->first, i->second );
    }
    fflush( outputfile );
#endif

    ddbg_assert2( cacheLbaMap.empty(), "disk_buffer_sort_dirty_cache_segments_by_RPO: DIRTY SEGMENTS NOT FOUND" );
    return cacheLbaMap;
}


segment * disk_flush_write_cache( disk *currdisk, diskreq *currDiskReq )
{
    dm_time_t time;
    dm_angle_t myAngle;
    double myTime;

#ifdef DEBUG_DISKCTLR
    fprintf (outputfile, "%f: disk_flush_write_cache  currdisk %p, currDiskReq %p \n", simtime, currdisk, currDiskReq );
    fflush(outputfile);
#endif
    assert(0);

    myTime = simtime;
    time = dm_time_dtoi(simtime);
    currdisk->mech_state.theta = 0;
    myAngle = currdisk->model->mech->dm_rotate( currdisk->model, &time );
    currdisk->mech_state.theta = myAngle;

    CACHE_LBA_MAP cacheLbaMap = disk_buffer_sort_dirty_cache_segments_by_RPO( currdisk );

    // here , we have a list of DIRTY segments to choose from
    segment *seg = cacheLbaMap.begin()->second;
    if( DISKSIM_READ == (seg->diskreqlist->ioreqlist->flags & DISKSIM_READ_WRITE_MASK) )
        seg->state = BUFFER_READING;
    else
        seg->state = BUFFER_WRITING;
    seg->diskreqlist->ioreqlist->bcount = seg->outbcount;
    seg->diskreqlist->ioreqlist->tagID = currDiskReq->ioreqlist->tagID;
    seg->diskreqlist->flags = 0;

    simresult_set_seekResultIndex( currDiskReq->ioreqlist->tagID, SECOND_COMMAND_INDEX );
    simresult_update_mech_acctimes( currDiskReq->ioreqlist->tagID, &seg->mech_acctimes );

    currdisk->currenthda = currdisk->effectivehda = 0;

    // chain the new I/O request to the current one
    currDiskReq->ioreqlist->next = currDiskReq->ioreqlist;
    currDiskReq->ioreqlist = seg->diskreqlist->ioreqlist;
    return seg;
}

// End of file


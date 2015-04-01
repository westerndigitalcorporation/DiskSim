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
/*
 * hurst_r 9/3/2102
 *   Functions ioqueue_print_contents and ioqueue_print_subqueue_state did not print the first queue entry
 */

#include "disksim_ioqueue.h"


#include "modules/modules.h"


// schlos - i'm going to have to modify this to account for batched queues
#define READY_TO_GO(iobufptr,queue) ( \
                                     ((iobufptr)->state == WAITING) && \
                                     (((queue)->enablement == NULL) || ((*(queue)->enablement)((iobufptr)->iolist))) && \
                                     (((queue)->sched_alg != BATCH_FCFS) || (((queue)->sched_alg == BATCH_FCFS) && ((iobufptr)->batch_complete == TRUE))) \
				    )


/* Sequential Stream Schemes */

#define IOQUEUE_CONCAT_READS		1
#define IOQUEUE_CONCAT_WRITES		2
#define IOQUEUE_CONCAT_BOTH		3
#define IOQUEUE_SEQSTREAM_READS		4
#define IOQUEUE_SEQSTREAM_WRITES	8
#define IOQUEUE_SEQSTREAM_EITHER	16

/* Sub queue identifications */

#define IOQUEUE_BASE		1
#define IOQUEUE_TIMEOUT		2
#define IOQUEUE_PRIORITY	3

/* Buffer states while in ioqueue */

#define WAITING     0   // waiting for resources to become available??
#define PENDING     1   // ??

/* Priority schemes */

#define ALLEQUAL	0
#define TWOQUEUE	1

/* Timeout schemes */

#define NOTIMEOUT	0
#define BASETIMEOUT	1
#define HALFTIMEOUT	2

/* Directions of movement for scanning policies */

#define ASC     1
#define DESC    2

/* read-only globals used during readparams phase */
static char *statdesc_accstats          = "Physical access time";
static char *statdesc_qtimestats        = "Queue time";
static char *statdesc_outtimestats      = "Response time";
static char *statdesc_intarrstats       = "Inter-arrival time";
static char *statdesc_readintarrstats   = "Read inter-arrival";
static char *statdesc_writeintarrstats  = "Write inter-arrival";
static char *statdesc_idlestats         = "Idle period length";
static char *statdesc_infopenalty       = "Sub-optimal mapping penalty";
static char *statdesc_reqsizestats      = "Request size";
static char *statdesc_readsizestats     = "Read request size";
static char *statdesc_writesizestats    = "Write request size";
static char *statdesc_instqueuelen      = "Instantaneous queue length";
static char *statdesc_batchsizestats    = "Batch size";

void print_batch_fcfs_queue(subqueue *queue)
{
    int i;
    iobuf *tmp;

    tmp = queue->list;

    printf("blkno, batchno, state, complete, next, prev\n");
    printf(" listlen = %d\n", queue->listlen);
    printf(" iobufcnt = %d\n", queue->iobufcnt);

    for (i = 0; i < queue->iobufcnt; i++) {
        printf(" %d %d %d %d %d %d\n",
                tmp->blkno,
                tmp->batchno,
                tmp->state,
                tmp->batch_complete,
                tmp->next->blkno,
                tmp->prev->blkno);

        if (tmp->batchno != -1) {
            ioreq_event *event_ptr = tmp->batch_list;

            printf("  batch_size = %d\n", tmp->batch_size);
            while (event_ptr != NULL) {
                printf("   %d\n", event_ptr->blkno);
                event_ptr = event_ptr->batch_next;
            }
        }
        tmp = tmp->next;
    }
}


static void ioqueue_print_subqueue_state (subqueue *queue)
{
   int i;
   iobuf *tmp;

   fprintf(outputfile, "%f: ioqueue_print_subqueue_state: subqueue %p, listlen %d, policy %d, iobufcnt %d, reqoutstanding %d\n",simtime,  queue, queue->listlen, queue->sched_alg, queue->iobufcnt, queue->reqoutstanding );
   if( queue->listlen > 0 )
   {
       tmp = queue->list;
       fprintf(outputfile, "%f: Contents of subqueue: listlen %d\n", simtime, queue->listlen);
       fprintf(outputfile, "%f:     Subqueue state: lastblkno %d, lastcylno %d, lastsurface %d, dir %d\n", simtime, queue->lastblkno, queue->lastcylno, queue->lastsurface, queue->dir);
       for (i = 0; i < queue->iobufcnt; i++) {
          fprintf(outputfile, "%f:         depth %d, state %d, devno %d, blkno: %d, bcount %d, flags %x, cylno %d, surface %d\n", simtime, i, tmp->state, tmp->devno, tmp->blkno, tmp->totalsize, tmp->flags, tmp->cylinder, tmp->surface);
          tmp = tmp->next;
       }
//       fprintf( outputfile, "\n" );
   }
}


void ioqueue_print_contents (ioqueue *queue)
{
   fprintf (outputfile, "\n%f: ioqueue_print_contents: queue %p\n", simtime, queue );
   fprintf (outputfile, "%f:   Contents of base queue: subqueue %p, listlen %d\n", simtime, &queue->base, queue->base.listlen );
   ioqueue_print_subqueue_state (&queue->base);

   fprintf(outputfile, "%f:   Contents of timeout queue: subqueue %p, listlen %d\n", simtime, &queue->timeout, queue->timeout.listlen );
   ioqueue_print_subqueue_state (&queue->timeout);

   fprintf(outputfile, "%f:   Contents of priority queue: subqueue %p, listlen %d\n", simtime, &queue->priority, queue->priority.listlen );
   ioqueue_print_subqueue_state (&queue->priority);
   fprintf (outputfile, "\n" );
}


int ioqueue_get_number_in_queue (ioqueue *queue)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_number_in_queue\n", simtime );
    fflush(outputfile);
#endif

    return(queue->base.listlen + queue->timeout.listlen + queue->priority.listlen);
}


int ioqueue_get_number_pending (ioqueue *queue)
{
    int numpend = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_number_pending\n", simtime );
    fflush(outputfile);
#endif

    numpend += queue->base.listlen - queue->base.numoutstanding;
    numpend += queue->timeout.listlen - queue->timeout.numoutstanding;
    numpend += queue->priority.listlen - queue->priority.numoutstanding;
    return(numpend);
}


int ioqueue_get_reqoutstanding (ioqueue *queue)
{
    int numout = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_reqoutstanding\n", simtime );
    fflush(outputfile);
#endif

    numout += queue->base.reqoutstanding;
    numout += queue->timeout.reqoutstanding;
    numout += queue->priority.reqoutstanding;
    return(numout);
}


int ioqueue_get_number_of_requests (ioqueue *queue)
{
    int numreqs = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_number_of_requests\n", simtime );
    fflush(outputfile);
#endif

    numreqs += queue->base.numcomplete + queue->base.listlen;
    numreqs += queue->timeout.numcomplete + queue->timeout.listlen;
    numreqs += queue->priority.numcomplete + queue->priority.listlen;
    return(numreqs);
}


int ioqueue_get_number_of_requests_initiated (ioqueue *queue)
{
    int numreqs = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_number_of_requests_initiated\n", simtime );
    fflush(outputfile);
#endif

    numreqs += queue->base.numcomplete + queue->base.numoutstanding;
    numreqs += queue->timeout.numcomplete + queue->timeout.numoutstanding;
    numreqs += queue->priority.numcomplete + queue->priority.numoutstanding;
    return(numreqs);
}


int ioqueue_get_dist (ioqueue *queue, int blkno)
{
    int lastblkno;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_dist\n", simtime );
    fflush(outputfile);
#endif

    if (queue->lastsubqueue == IOQUEUE_BASE) {
        lastblkno = queue->base.lastblkno;
    } else if (queue->lastsubqueue == IOQUEUE_TIMEOUT) {
        lastblkno = queue->timeout.lastblkno;
    } else if (queue->lastsubqueue == IOQUEUE_PRIORITY) {
        lastblkno = queue->priority.lastblkno;
    } else {
        fprintf(stderr, "Unknown queue identification at ioqueue_get_dist - %d\n", queue->lastsubqueue);
        exit(1);
    }
    return(abs(blkno - lastblkno));
}


static void 
ioqueue_get_cylinder_mapping(
        ioqueue *queue,
        iobuf *curr,
        int blkno,
        int *cylptr,
        int *surfptr,
        int cylmaptype)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_cylinder_mapping  Enter: ioqueue %p, iobuf %p, blkno %d, cyl %d, surface %d, cylmaptype %d\n", simtime, queue, curr, blkno, *cylptr, *surfptr, cylmaptype );
    fflush(outputfile);
#endif

    cylmaptype = (cylmaptype == -1) ? queue->cylmaptype : cylmaptype;
    switch (cylmaptype) {
    case MAP_NONE:
        *cylptr = 0;
        *surfptr = 0;
        // fprintf (outputfile, "ioqueue_get_cylinder_mapping:  MAP_NONE\n");
        break;

    case MAP_IGNORESPARING:
    case MAP_ZONEONLY:
    case MAP_ADDSLIPS:
    case MAP_FULL:
        device_get_mapping(cylmaptype,
                curr->iolist->devno,
                blkno,
                cylptr,
                surfptr,
                NULL);
        // fprintf (outputfile, "ioqueue_get_cylinder_mapping:  MAP_[%d]\n", cylmaptype);
        break;

    case MAP_FROMTRACE:
        /* this yucky cast is being allowed for convenience.  We know
         * it can't really hurt us...  */

        *cylptr = *((int *) curr->iolist->buf);
        *surfptr = 0;
        break;

    case MAP_AVGCYLMAP:
        *cylptr = blkno / queue->sectpercyl;
        *surfptr = 0;
        break;

    default:
        fprintf(stderr, "Unknown cylinder mapping type: %d\n", queue->cylmaptype);
        exit(1);
    }

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_cylinder_mapping  Exit: ioqueue %p, iobuf %p, blkno %d, cyl %d, surface %d, cylmaptype %d\n", simtime, queue, curr, blkno, *cylptr, *surfptr, cylmaptype );
    fflush(outputfile);
#endif

    if ((cylptr != NULL) && (surfptr != NULL)) {
        // fprintf (outputfile, "ioqueue_get_cylinder_mapping:  Mapped request for block %d to %d, %d\n",
        // blkno, *cylptr, *surfptr);
    } else {
        // fprintf (outputfile, "ioqueue_get_cylinder_mapping:  cylptr or surfptr were NULL\n");
    }
}


void ioqueue_set_concatok_function (ioqueue *queue, int (**concatok)(void *,int,int,int,int), void *concatokparam)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_set_concatok_function\n", simtime );
    fflush(outputfile);
#endif

    queue->concatok = concatok;
    queue->concatokparam = concatokparam;
}


void ioqueue_set_idlework_function (ioqueue *queue, void (**idlework)(void *,int), void *idleworkparam, double idledelay)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_set_idlework_function\n", simtime );
    fflush(outputfile);
#endif
    queue->idlework = idlework;
    queue->idleworkparam = idleworkparam;
    queue->idledelay = idledelay;
    queue->idledetect = NULL;
}


void ioqueue_set_enablement_function (ioqueue *queue, int (**enablement)(ioreq_event *))
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_set_enablement_function\n", simtime );
    fflush(outputfile);
#endif
    queue->base.enablement = enablement;
    queue->timeout.enablement = enablement;
    queue->priority.enablement = enablement;
}


static void ioqueue_idledetected (timer_event *timereq)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_idledetected\n", simtime );
    fflush(outputfile);
#endif

    ioqueue *queue = (ioqueue *) timereq->ptr;

    queue->idledetect = NULL;
    (*queue->idlework)(queue->idleworkparam, queue->devno);
    addtoextraq((event *)timereq);
}


//*****************************************************************************
// Function: ioqueue_reset_idledetecter
//   Eithere removes the idle timer event or places one in the intq
//
// Parameters:
//   ioqueue *queue, int timechange
//
// Returns: void
//*****************************************************************************

void
ioqueue_reset_idledetecter (ioqueue *queue, int timechange)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_reset_idledetecter:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    timer_event *idledetect = queue->idledetect;
    int listlen = queue->base.listlen + queue->timeout.listlen + queue->priority.listlen;

    if ((listlen) || ((idledetect) && (!timechange))) {
        return;
    }
    if (idledetect) {
        if (!(removefromintq((event *)idledetect))) {
            fprintf(stderr, "existing idledetect event not on intq in ioqueue_reset_idledetecter\n");
            exit(1);
        }
    } else {
        idledetect = (timer_event *) getfromextraq();
        idledetect->type = TIMER_EXPIRED;
        idledetect->func = &disksim->timerfunc_ioqueue;
        idledetect->ptr = queue;
        queue->idledetect = idledetect;
    }
    idledetect->time = simtime + queue->idledelay;
    addtointq((event *)idledetect);
}


static void ioqueue_update_arrival_stats (ioqueue *queue, ioreq_event *curr)
{
   double tdiff;

   if (queue->printintarrstats) {
      tdiff = simtime - queue->lastarr;
      queue->lastarr = simtime;
      stat_update(&queue->intarrstats, tdiff);
      if (curr->flags & READ) {
         tdiff = simtime - queue->lastread;
         queue->lastread = simtime;
	 stat_update(&queue->readintarrstats, tdiff);
      } else {
         tdiff = simtime - queue->lastwrite;
         queue->lastwrite = simtime;
	 stat_update(&queue->writeintarrstats, tdiff);
      }
   }
   if (queue->printsizestats) {
      stat_update(&queue->reqsizestats, (double) curr->bcount);
      if (curr->flags & READ) {
         stat_update(&queue->readsizestats, (double) curr->bcount);
      } else {
         stat_update(&queue->writesizestats, (double) curr->bcount);
      }
   }
   if (curr->blkno == queue->seqblkno) {
      if ((curr->flags & READ) && (queue->seqflags & READ)) {
	 queue->seqreads++;
      } else if (!(curr->flags & READ) && !(queue->seqflags & READ)) {
	 queue->seqwrites++;
      }
   }
   queue->seqblkno = curr->blkno + curr->bcount;
   queue->seqflags = curr->flags;
}


static void ioqueue_update_subqueue_statistics (subqueue *queue)
{
   double tdiff = simtime - queue->lastalt;

   queue->runlistlen += tdiff * queue->listlen;
   queue->runreadlen += tdiff * queue->readlen;
   if (queue->readlen > queue->maxreadlen) {
      queue->maxreadlen = queue->readlen;
   }
   if ((queue->listlen - queue->readlen) > queue->maxwritelen) {
      queue->maxwritelen = queue->listlen - queue->readlen;
   }
   queue->runoutstanding += tdiff * queue->numoutstanding;
   if (queue->numoutstanding > queue->maxoutstanding) {
      queue->maxoutstanding = queue->numoutstanding;
   }
   if (queue->listlen > queue->maxlistlen) {
      queue->maxlistlen = queue->listlen;
   }
   if ((queue->listlen - queue->numoutstanding) > queue->maxqlen) {
      queue->maxqlen = queue->listlen - queue->numoutstanding;
   }
   queue->lastalt = simtime;
}

static void remove_tsps(iobuf *tmp);

static void ioqueue_remove_from_subqueue (subqueue *queue, iobuf *tmp)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_remove_from_subqueue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    if(queue->sched_alg == TSPS){
        remove_tsps(tmp);
    }

    if ((queue->list == tmp) && (tmp == tmp->next)) {
        queue->list = NULL;
    } else {
        tmp->next->prev = tmp->prev;
        tmp->prev->next = tmp->next;
        if (queue->list == tmp) {
            queue->list = tmp->prev;
        }
    }
    tmp->next = NULL;
    tmp->prev = NULL;
}

static void ioqueue_remove_from_batch_fcfs_subqueue (subqueue *queue, iobuf *tmp, ioreq_event *done)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_remove_from_batch_fcfs_subqueue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    if(queue->sched_alg == TSPS){
        remove_tsps(tmp);
    }

    if ((queue->list == tmp) && (tmp == tmp->next)) {
        queue->list = NULL;
    } else {
        tmp->next->prev = tmp->prev;
        tmp->prev->next = tmp->next;
        if (queue->list == tmp) {
            queue->list = tmp->prev;
        }
    }
    tmp->next = NULL;
    tmp->prev = NULL;
}

static void ioqueue_insert_batch_fcfs_to_queue (subqueue *queue, iobuf *new_iobuf)
{
    iobuf *tmp;
    iobuf *first_in_batch = NULL;
    ioreq_event *event_ptr;
    int i;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_insert_batch_fcfs_to_queue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    //  printf("ioqueue_insert_batch_fcfs_to_queue:: inserting %d\n", new_iobuf->blkno);

    queue->iobufcnt--;
    //  printf("ioqueue_insert_batch_fcfs_to_queue:: queue contents before\n");
    // print_batch_fcfs_queue(queue);
    queue->iobufcnt++;

    // if this request is not part of a batch, then just add it at the end of the queue.
    if (new_iobuf->batchno == -1) {
        new_iobuf->next = queue->list->next;
        new_iobuf->prev = queue->list;
        new_iobuf->next->prev = new_iobuf;
        new_iobuf->prev->next = new_iobuf;
        queue->list = new_iobuf;

        new_iobuf->batch_complete = TRUE;

        // printf("ioqueue_insert_batch_fcfs_to_queue:: queue contents after\n");
        // print_batch_fcfs_queue(queue);

        return;
    }

    // if it is part of a batch, walk the entire queue and find where to insert it.
    // the new iobuf should go after the last request in its batch, or at the end of the queue
    // if this is the first request in the batch.

    tmp = queue->list;

    for (i = 0; i < (queue->iobufcnt - 1); i++) {
        if (tmp->batchno == new_iobuf->batchno) {
            // chain the new request onto this batch and free new_iobuf
            event_ptr = tmp->batch_list;

            while (event_ptr->batch_next != NULL) {
                event_ptr = event_ptr->batch_next;
            }

            event_ptr->batch_next = new_iobuf->iolist;
            event_ptr->batch_next->batch_prev = event_ptr;
            event_ptr->batch_next->batch_next = NULL;

            if (new_iobuf->iolist->batch_complete == TRUE) {
                tmp->batch_complete = TRUE;
            } else {
                tmp->batch_complete = FALSE;
            }

            tmp->batch_size++;
            tmp->reqcnt++;

            // queue->listlen--;
            // because we didn't actually add an iobuf to the queue after all
            queue->iobufcnt--;
            addtoextraq((event *)new_iobuf);

            // printf("ioqueue_insert_batch_fcfs_to_queue:: queue contents after\n");
            // print_batch_fcfs_queue(queue);

            return;
        } else {
            tmp = tmp->next;
        }
    }

    // this batch doesn't exist in the queue yet, so insert new_iobuf at the end of the queue
    new_iobuf->next = queue->list->next;
    new_iobuf->prev = queue->list;
    new_iobuf->next->prev = new_iobuf;
    new_iobuf->prev->next = new_iobuf;
    queue->list = new_iobuf;

    new_iobuf->batch_size = 1;

    // printf("ioqueue_insert_batch_fcfs_to_queue:: queue contents after\n");
    // print_batch_fcfs_queue(queue);

    return;
}


/* queue->list not NULL when entered */

static void ioqueue_insert_fcfs_to_queue (subqueue *queue, iobuf *temp)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_insert_fcfs_to_queue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    temp->next = queue->list->next;
    temp->prev = queue->list;
    temp->next->prev = temp;
    temp->prev->next = temp;
    queue->list = temp;
}


/* queue->list not NULL when entered */

static void ioqueue_insert_ordered_to_queue (subqueue *queue, iobuf *temp)
{
    iobuf *head;
    iobuf *run;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_insert_ordered_to_queue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    head = queue->list->next;

    if ((temp->cylinder < head->cylinder)       ||
            ((temp->cylinder == head->cylinder) &&
                    (temp->surface < head->surface))       ||
                    ((temp->cylinder == head->cylinder) &&
                            (temp->surface == head->surface)   &&
                            (temp->blkno < head->blkno))) {
        queue->list->next = temp;
        temp->next = head;
        temp->prev = queue->list;
        head->prev = temp;

    } else if ((temp->cylinder > queue->list->cylinder)       ||
            ((temp->cylinder == queue->list->cylinder) &&
                    (temp->surface > queue->list->surface))       ||
                    ((temp->cylinder == queue->list->cylinder) &&
                            (temp->surface == queue->list->surface)   &&
                            (temp->blkno >= queue->list->blkno))) {
        temp->next = head;
        temp->prev = queue->list;
        head->prev = temp;
        queue->list->next = temp;
        queue->list = temp;
    } else {
        run = head;
        while ((temp->cylinder > run->next->cylinder)     ||
                ((temp->cylinder == run->next->cylinder)  &&
                        (temp->surface > run->next->surface))     ||
                        ((temp->cylinder == run->next->cylinder)  &&
                                (temp->surface == run->next->surface)    &&
                                (temp->blkno >= run->next->blkno))) {
            run = run->next;
        }
        temp->next = run->next;
        temp->prev = run;
        temp->next->prev = temp;
        run->next = temp;
    }
}


static int ioqueue_check_concat (subqueue *queue, iobuf *req1, iobuf *req2, int concatmax)
{
    ioreq_event *tmp;
    int seqscheme = queue->bigqueue->seqscheme;
    int (**concatok)(void *,int,int,int,int) = queue->bigqueue->concatok;
    void *concatokparam = queue->bigqueue->concatokparam;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_check_concat:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    if (req1->next != req2) {
        fprintf(stderr, "Non-adjacent requests sent to ioqueue_check_concat\n");
        exit(1);
    }
    if (req1->flags & READ) {
        if (!(seqscheme & IOQUEUE_CONCAT_READS) || !(req2->flags & READ)) {
            return(0);
        }
    } else {
        if (!(seqscheme & IOQUEUE_CONCAT_WRITES) || (req2->flags & READ)) {
            return(0);
        }
    }
    if ((req1 == queue->list) || (req1 == req2) || !READY_TO_GO(req1,queue) || !READY_TO_GO(req2,queue) || ((req1->blkno + req1->totalsize) != req2->blkno) || ((req1->totalsize + req2->totalsize) > concatmax)) {
        return(0);
    }
    if (concatok) {
        if ((*concatok)(concatokparam, req1->blkno, req1->totalsize, req2->blkno, req2->totalsize) == 0) {
            return(0);
        }
    }
    tmp = req1->iolist;
    if (tmp == NULL) {
        req1->iolist = req2->iolist;
    } else {
        while (tmp->next) {
            tmp = tmp->next;
        }
        tmp->next = req2->iolist;
    }
    req1->next = req2->next;
    req1->next->prev = req1;
    req1->reqcnt += req2->reqcnt;
    req1->totalsize += req2->totalsize;
    if (queue->list == req2) {
        queue->list = req1;
    }
    queue->iobufcnt--;
    addtoextraq((event *) req2);
    return(1);
}


static void ioqueue_insert_new_request (subqueue *queue, iobuf *temp)
{
    int concatmax;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_insert_new_request  subqueue %p, sched_alg = %d, devno %d, blkno %d, bcount %d, flags %x\n", simtime, queue, queue->sched_alg, temp->devno, temp->blkno, temp->totalsize, temp->flags );
    fflush( outputfile );
#endif

    ioqueue_update_subqueue_statistics(queue);
    queue->iobufcnt++;
    queue->listlen++;  // COULD BE A BUG!
    if (temp->flags & READ) {
        queue->readlen++;
        queue->numreads++;
    } else {
        queue->numwrites++;
    }

    if (queue->list == NULL) {
        queue->list = temp;
        temp->next = temp;
        temp->prev = temp;
        if ((queue->sched_alg == BATCH_FCFS) && (temp->batchno != -1)) {
            temp->batch_size = 1;
            temp->batch_list = temp->iolist;
        }
    } else {
        if ((queue->sched_alg == FCFS) || (queue->sched_alg == PRI_VSCAN_LBN)) {
            ioqueue_insert_fcfs_to_queue(queue, temp);
        } else if (queue->sched_alg == BATCH_FCFS) {
            ioqueue_insert_batch_fcfs_to_queue(queue, temp);
        }
        else {
            ioqueue_insert_ordered_to_queue(queue, temp);
        }
    }
    concatmax = queue->bigqueue->concatmax;
    if (concatmax) {
        temp = temp->prev;
        if (!(ioqueue_check_concat(queue, temp, temp->next, concatmax))) {
            temp = temp->next;
        }
        ioqueue_check_concat(queue, temp, temp->next, concatmax);
    }
#ifdef DEBUG_IOQUEUE
    ioqueue_print_subqueue_state(queue);
#endif
}


/* Returns 0 if request is sequential from a previous request (and detection */
/* enabled).  Otherwise returns 1.				             */
/* Assuming no command queueing.					     */

static int ioqueue_seqstream_head (ioqueue *queue, iobuf *listhead, iobuf *current)
{
    int idiff;
    int ret = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_seqstream_head:: queue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    if (!(queue->seqscheme & (IOQUEUE_SEQSTREAM_READS|IOQUEUE_SEQSTREAM_WRITES|IOQUEUE_SEQSTREAM_EITHER))) {
        return(1);
    }
    idiff = queue->seqstreamdiff;
    if ((current == listhead) || !READY_TO_GO(current->prev,(&queue->base))) {
        ret = 1;
    }
    if (ret == 0) {
        if (current->flags & READ) {
            if (current->prev->flags & READ) {
                if (!(queue->seqscheme & (IOQUEUE_SEQSTREAM_READS|IOQUEUE_SEQSTREAM_EITHER))) {
                    ret = 1;
                }
            } else {
                if (!(queue->seqscheme & IOQUEUE_SEQSTREAM_EITHER)) {
                    ret = 1;
                }
            }
            if ((current->prev->blkno != current->blkno) && (((current->prev->blkno + current->prev->totalsize + idiff) < current->blkno))) {
                ret = 1;
            }
        } else {
            if (current->prev->flags & READ) {
                if (!(queue->seqscheme & IOQUEUE_SEQSTREAM_EITHER)) {
                    ret = 1;
                }
            } else {
                if (!(queue->seqscheme & (IOQUEUE_SEQSTREAM_WRITES|IOQUEUE_SEQSTREAM_EITHER))) {
                    ret = 1;
                }
            }
            if ((current->prev->blkno != current->blkno) && ((current->prev->blkno + current->prev->totalsize) != current->blkno)) {
                ret = 1;
            }
        }
    }
    return(ret);
}


/* Queue contains >= 2 items when called */

static iobuf * ioqueue_get_request_from_fcfs_queue (subqueue *queue)
{
    iobuf *tmp;
    int i;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_fcfs_queue:: subqueue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    tmp = queue->list->next;
    for (i = 0; i < queue->iobufcnt; i++) {
        if (queue->force_absolute_fcfs) {
            if (tmp->state == WAITING) {
                if (READY_TO_GO(tmp,queue)) {
                    return(tmp);
                } else {
                    return(NULL);
                }
            }
        } else {
            if (READY_TO_GO(tmp,queue)) {
                return(tmp);
            }
        }
        tmp = tmp->next;
    }
    return(NULL);
}


/* Queue contains >= 2 items when called */

static int ioqueue_get_priority_level (ioqueue *ioqueue, iobuf *req)
{
    int ret = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_priority_level:: queue %p\n", simtime, ioqueue );
    fflush(outputfile);
#endif

    if (ioqueue->priority_mix == 1) {
        if ((req->flags & TIME_CRITICAL) || (req->flags & READ)) {
            ret = 1;
        }
    } else if (ioqueue->priority_mix == 2) {
        if (req->flags & TIME_CRITICAL) {
            ret = 2;
        } else if (req->flags & READ) {
            ret = 1;
        }
    } else if (ioqueue->priority_mix == 3) {
        if (req->flags & READ) {
            ret = 2;
        } else if (req->flags & TIME_CRITICAL) {
            ret = 1;
        }
    } else if (ioqueue->priority_mix == 4) {
        if (req->flags & TIME_CRITICAL) {
            ret = (req->flags & READ) ? 2 : 3;
        } else {
            ret = (req->flags & READ) ? 1 : 0;
        }
    } else if (ioqueue->priority_mix == 5) {
        if (req->flags & READ) {
            ret = 1;
        }
    } else if (ioqueue->priority_mix == 6) {
        if (req->flags & TIME_CRITICAL) {
            ret = 1;
        }
    }
    return(ret);
}


static iobuf * ioqueue_get_request_from_pri_lbn_vscan_queue (subqueue *queue, int numlbns, int vscan_value)
{
    int schedalg;
    int priority_factor;
    int age_factor;
    iobuf *tmp;
    int curr_effpri = 0, best_effpri = 0;
    iobuf *ret = NULL;
    int i;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_pri_lbn_vscan_queue:: queue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    schedalg = queue->bigqueue->timeout.sched_alg;
    priority_factor = queue->bigqueue->priority.sched_alg;
    age_factor = queue->bigqueue->to_time;
    if (schedalg == ELEVATOR_LBN) {
        vscan_value = numlbns;
    } else if (schedalg == SSTF_LBN) {
        vscan_value = 0;
    }
    tmp = queue->list->next;
    for (i = 0; i < queue->iobufcnt; i++) {
        if (READY_TO_GO(tmp,queue)) {
            curr_effpri = tmp->blkno - queue->lastblkno;
            if (schedalg == CYCLE_LBN) {
                if (curr_effpri < 0) {
                    curr_effpri += numlbns;
                }
            } else if (curr_effpri) {
                if (curr_effpri < 0) {
                    curr_effpri = -curr_effpri;
                    if (queue->dir == ASC) {
                        curr_effpri += vscan_value;
                    }
                } else {
                    if (queue->dir == DESC) {
                        curr_effpri += vscan_value;
                    }
                }
            }
            curr_effpri = numlbns - curr_effpri;
            curr_effpri += (priority_factor * numlbns * ioqueue_get_priority_level(queue->bigqueue, tmp)) / 100;
            curr_effpri += (age_factor * (int) (simtime - tmp->starttime)) / 1280;
            if ((ret == NULL) || (curr_effpri > best_effpri)) {
                ret = tmp;
                best_effpri = curr_effpri;
            }
        }
        tmp = tmp->next;
    }
    return(ret);
}


/* Queue contains >= 2 items when called */

static iobuf * ioqueue_get_request_from_lbn_scan_queue (subqueue *queue)
{
    iobuf *temp;
    iobuf *stop;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_lbn_scan_queue:: queue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    temp = queue->list->next;
    stop = temp;
    if (queue->lastblkno > temp->blkno) {
        while ((temp->next != stop) && (queue->lastblkno > temp->next->blkno)) {
            temp = temp->next;
        }
        temp = temp->next;
    }
    stop = temp;
    while ((temp->next != stop) && !READY_TO_GO(temp,queue)) {
        temp = temp->next;
    }
    if ((temp->next == stop) && !READY_TO_GO(temp,queue)) {
        temp = NULL;
    }
    return(temp);
}


/* Queue contains >= 2 items when called */

static iobuf * ioqueue_get_request_from_lbn_vscan_queue (subqueue *queue, int value)
{
    iobuf *temp;
    iobuf *head;
    iobuf *top = NULL;
    iobuf *bottom = NULL;
    int diff1, diff2;
    int tmpdir;
    iobuf *bestbottom;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_lbn_vscan_queue:: queue %p, value %d\n", simtime, queue, value );
    fflush(outputfile);
#endif

    tmpdir = queue->dir;
    temp = queue->list->next;
    head = queue->list->next;

    while ((temp->next != head) && (queue->lastblkno > temp->next->blkno)) {
        temp = temp->next;
    }
    if (temp->blkno < queue->lastblkno) {
        bottom = temp;
        while ((bottom != head) && !READY_TO_GO(bottom,queue)) {
            bottom = bottom->prev;
        }
        if ((bottom == head) && !READY_TO_GO(bottom,queue)) {
            bottom = NULL;
        } else {
            bestbottom = bottom;
            while ((bottom != head) && ((bottom->prev->blkno == bestbottom->blkno) || (!(ioqueue_seqstream_head(queue->bigqueue, head, bottom))))) {
                bottom = bottom->prev;
                bestbottom = (READY_TO_GO(bottom,queue)) ? bottom : bestbottom;
            }
            bottom = bestbottom;
        }
    }
    if (temp->next != head) {
        if (temp->blkno >= queue->lastblkno) {
            top = temp;
        } else {
            top = temp->next;
        }
        while ((top->next != head) && !READY_TO_GO(top,queue)) {
            top = top->next;
        }
    }
    if ((top == NULL) || !READY_TO_GO(top,queue) || (!(ioqueue_seqstream_head(queue->bigqueue, head, top)))) {
        temp = bottom;
    } else if ((bottom == NULL) || !READY_TO_GO(bottom,queue)) {
        temp = top;
    } else {
        diff1 = diff(top->blkno, queue->lastblkno);
        diff2 = diff(queue->lastblkno, bottom->blkno);
        if (queue->dir == ASC) {
            diff2 = (diff2) ? (diff2 + value) : diff2;
        } else {
            diff1 = (diff1) ? (diff1 + value) : diff1;
        }
        if (diff1 < diff2) {
            temp = top;
        } else if ((diff1 > diff2) || (queue->sstfupdown)) {
            temp = bottom;
        } else {
            temp = top;
        }
        if (diff1 == diff2) {
            queue->sstfupdown = (queue->sstfupdown) ? 0 : 1;
            queue->sstfupdowncnt++;
        }
    }
    if (temp->blkno != queue->lastblkno) {
        queue->dir = (temp->blkno > queue->lastblkno) ? ASC : DESC;
    }
    top = (temp == top) ? bottom : top;
    if ((top) && READY_TO_GO(top,queue)) {
        int cylno1, cylno2;
        int surface1, surface2;

        ioqueue_get_cylinder_mapping(queue->bigqueue, temp, temp->blkno, &cylno1, &surface1, MAP_FULL);
        ioqueue_get_cylinder_mapping(queue->bigqueue, top, top->blkno, &cylno2, &surface2, MAP_FULL);
        diff1 = abs(cylno1 - queue->optcylno);
        diff2 = abs(cylno2 - queue->optcylno);
        if (((tmpdir == ASC) && (temp != bottom)) || ((tmpdir == DESC) && (temp == bottom))) {
            if (diff2) {
                diff2 += (int) (queue->vscan_value * (double) device_get_numcyls(temp->iolist->devno));
            }
        } else if (diff1) {
            if (diff1) {
                diff1 += (int) (queue->vscan_value * (double) device_get_numcyls(temp->iolist->devno));
            }
        }
        if (diff1 > diff2) {
            stat_update(&queue->infopenalty, (double) (diff1 - diff2));
            /*
fprintf (outputfile, "diff1 %d, diff2 %d, diffdiff %d\n", diff1, diff2, (diff1 - diff2));
             */
        } else if ((diff1 == diff2) && (diff1 == 0)) {
            if ((queue->optsurface == surface2) && (surface1 != surface2)) {
                stat_update(&queue->infopenalty, (double) 0);
            }
        }
    }
    return(temp);
}


/* Queue contains >= 2 items when called */

/* Returns pointer to "best" request on queue->lastcylno, or first request
   on next available forward cylinder, or head (in priority order). */

static iobuf *ioqueue_identify_request_on_cylinder (subqueue *queue)
{
    iobuf *temp;
    iobuf *head;
    iobuf *cylstop;
    iobuf *surfstop;

    int lastcylno;
    int lastsurface;
    int lastblkno;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_identify_request_on_cylinder:: queue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    lastcylno = queue->lastcylno;
    lastsurface = queue->lastsurface;
    lastblkno = queue->lastblkno;

    head = queue->list->next;
    temp = head;

    if ((head->cylinder < lastcylno) && (lastcylno <= queue->list->cylinder)) {
        while ((temp->next != head) && (lastcylno > temp->next->cylinder)) {
            temp = temp->next;
        }
        temp = temp->next;
    }

    if (temp->cylinder != lastcylno) {
        return(temp);
    }

    cylstop = temp;      /* first req on cylinder */

    while ((temp->next != head) && (lastcylno == temp->next->cylinder) && (lastsurface > temp->surface)) {
        temp = temp->next;
    }

    if (lastsurface > temp->surface) {
        temp = cylstop;   /* drop out and try from first req on cylinder */

    } else if (!queue->surfoverforw) {
        while ((temp->next != head) && (lastcylno == temp->next->cylinder) && (!READY_TO_GO(temp,queue) || (temp->blkno < lastblkno))) {
            temp = temp->next;
        }
        if (READY_TO_GO(temp,queue) && (temp->blkno >= lastblkno)) {
            return(temp);
        } else {
            temp = cylstop;   /* drop out and try from first req on cylinder */
        }

    } else {    /* surfoverforw */

        surfstop = temp;     /* first req on surface */

        while ((temp->next != head) && (lastcylno == temp->next->cylinder) && (lastsurface == temp->next->surface) && (!READY_TO_GO(temp,queue) || (temp->blkno < lastblkno))) {
            temp = temp->next;
        }
        if (READY_TO_GO(temp,queue) && (temp->blkno >= lastblkno) && (temp->surface == lastsurface)) {
            return(temp);
        }

        temp = surfstop;

        while ((temp->next != head) && (lastcylno == temp->next->cylinder) && !READY_TO_GO(temp,queue)) {
            temp = temp->next;
        }
        if (READY_TO_GO(temp,queue)) {
            return(temp);
        }

        temp = cylstop;   /* drop out and try from first req on cylinder */
    }

    /* drop out recovery point */

    if (!READY_TO_GO(temp,queue)) {
        while (!READY_TO_GO(temp,queue) && (temp->next != head) && (temp->next->cylinder == lastcylno)) {
            temp = temp->next;
        }
        temp = temp->next;
    }
    return(temp);
}


/* Queue contains >= 2 items when called */

static iobuf * ioqueue_get_request_from_cyl_scan_queue (subqueue *queue)
{
    iobuf *temp;
    iobuf *stop;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_cyl_scan_queue:: queue %p\n", simtime, queue );
    fflush(outputfile);
#endif

    temp = queue->list->next;
    if (queue->lastcylno >= temp->cylinder) {
        temp = ioqueue_identify_request_on_cylinder(queue);
    }
    stop = temp;
    while (!READY_TO_GO(temp,queue) && (temp->next != stop)) {
        temp = temp->next;
    }
    if ((temp->next == stop) && !READY_TO_GO(temp,queue)) {
        temp = NULL;
    }
    return(temp);
}


/* Queue contains >= 2 items when called */

static iobuf * ioqueue_get_request_from_cyl_vscan_queue (subqueue *queue, int value)
{
    iobuf *temp;
    iobuf *head;
    iobuf *top = NULL;
    iobuf *bottom = NULL;
    iobuf *bottom2 = NULL;
    iobuf *bestone;
    int diff1, diff2, diff3;
    int tmpdir;
    int cylno1, cylno2, cylno3;
    int surface1, surface2, surface3;

    tmpdir = queue->dir;
    temp = queue->list->next;
    head = queue->list->next;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_cyl_vscan_queue:: queue %p, value %d\n", simtime, queue, value );
    fflush(outputfile);
#endif

    bestone = ioqueue_identify_request_on_cylinder(queue);
    if (!READY_TO_GO(bestone,queue) || (bestone->cylinder != queue->lastcylno)) {
        bestone = NULL;
    }
    while ((temp->next != head) && (queue->lastblkno > temp->next->blkno)) {
        temp = temp->next;
    }
    if (temp->blkno < queue->lastblkno) {
        bottom = temp;
        while ((bottom != head) && !READY_TO_GO(bottom,queue)) {
            bottom = bottom->prev;
        }
        bottom2 = temp;
        if (READY_TO_GO(bottom,queue) && (!bestone)) {
            while ((bottom != head) && (bottom->prev->cylinder == bottom->cylinder)) {
                bottom = bottom->prev;
            }
            while ((bottom != temp) && !READY_TO_GO(bottom,queue)) {
                bottom = bottom->next;
            }
        }
    }
    if (temp->next != head) {
        top = (temp->blkno >= queue->lastblkno) ? temp : temp->next;
        while ((top->next != head) && !READY_TO_GO(top,queue)) {
            top = top->next;
        }
    }
    if (!bestone) {
        if ((top == NULL) || !READY_TO_GO(top,queue)) {
            temp = bottom;
        } else if ((bottom == NULL) || !READY_TO_GO(bottom,queue)) {
            temp = top;
        } else {
            diff1 = diff(top->cylinder, queue->lastcylno);
            diff2 = diff(queue->lastcylno, bottom->cylinder);
            if (queue->dir == ASC) {
                diff2 += value;
            } else {
                diff1 += value;
            }
            if (diff1 < diff2) {
                temp = top;
            } else if ((diff1 > diff2) || (queue->sstfupdown)) {
                temp = bottom;
            } else {
                temp = top;
            }
            if (diff1 == diff2) {
                queue->sstfupdown = (queue->sstfupdown) ? 0 : 1;
                queue->sstfupdowncnt++;
            }
        }
    } else {
        temp = bestone;
    }
    if (temp->cylinder != queue->lastcylno) {
        queue->dir = (temp->cylinder > queue->lastcylno) ? ASC : DESC;
    }
    ioqueue_get_cylinder_mapping(queue->bigqueue, temp, temp->blkno, &cylno1, &surface1, MAP_FULL);
    diff1 = abs(cylno1 - queue->optcylno);
    if ((top == NULL) || !READY_TO_GO(top,queue)) {
        top = temp;
    }
    ioqueue_get_cylinder_mapping(queue->bigqueue, top, top->blkno, &cylno2, &surface2, MAP_FULL);
    diff2 = abs(cylno2 - queue->optcylno);
    if ((bottom2 == NULL) || !READY_TO_GO(bottom2,queue)) {
        bottom2 = temp;
    }
    ioqueue_get_cylinder_mapping(queue->bigqueue, bottom2, bottom2->blkno, &cylno3, &surface3, MAP_FULL);
    diff3 = abs(cylno3 - queue->optcylno);
    if (tmpdir == ASC) {
        diff3 = (diff3) ? (diff3 + value) : diff3;
        diff1 = (temp->cylinder < queue->lastcylno) ? (diff1 + value) : diff1;
    } else {
        diff2 = (diff2) ? (diff2 + value) : diff2;
        diff1 = (temp->cylinder > queue->lastcylno) ? (diff1 + value) : diff1;
    }
    if ((diff3 < diff2) || ((!diff3) && (surface3 == queue->optsurface))) {
        surface2 = surface3;
        diff2 = diff3;
    }
    if (diff1 > diff2) {
        stat_update(&queue->infopenalty, (double) (diff1 - diff2));
        /*
fprintf (outputfile, "diff1 %d, diff2 %d, diffdiff %d, value %d\n", diff1, diff2, (diff1 - diff2), value);
         */
    } else if ((diff1 == diff2) && (diff1 == 0)) {
        if ((queue->optsurface == surface2) && (surface1 != surface2)) {
            stat_update(&queue->infopenalty, (double) 0);
        }
    }
    return(temp);
}


/* Queue contains >= 2 items when called */

static iobuf *ioqueue_get_request_from_opt_sptf_queue (subqueue *queue, int checkcache, int ageweight, int posonly)
{
    int i;
    iobuf *temp;
    iobuf *best = NULL;
    ioreq_event *test;
    double mintime = 100000.0;
    double readdelay;
    double writedelay;
    double delay;
    double weight;
    double temp_delay;
    ioreq_event *tmp;

    double age = 0.0;



    ASSERT((ageweight >= 0) && (ageweight <= 3));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_opt_sptf_queue:: queue %p, checkcache %d, ageweight %d, posonly %d\n", simtime, queue, checkcache, ageweight, posonly );
    fflush(outputfile);
#endif

    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;

    // fprintf(outputfile, "get_request_from_sptf::  listlen = %d\n", queue->listlen);

    for (i=0; i<queue->listlen; i++) {
        // fprintf(outputfile, "temp->state = %d\n", temp->state);
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                // scan the iolist for the event with the smallest time
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    //fprintf(outputfile, "temp_delay = %f\n",temp_delay);
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                // fprintf(outputfile, "get_request_from_sptf...::  blkno = %d, delay = %f\n", test->blkno, delay);

                //	    fprintf(stderr,"serv %f old eff = %f new %f blkno %ld\n",temp_serv, mintime, delay,temp->blkno);
                if (delay < mintime) {
                    best = temp;
                    mintime = delay;
                }
            }
        } else {
            // fprintf(outputfile, "not READY_TO_GO\n");
        }

        temp = temp->next;
    }
    addtoextraq((event *) test);

    /*
   fprintf (outputfile, "Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n",
	    mintime, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
     */

    return(best);
}



static double 
service_time(unsigned int current_cyl,
        unsigned int current_sector,
        unsigned int current_head,
        unsigned int target_cyl,
        unsigned int target_sector,
        unsigned int target_head,
        int read,
        unsigned int blocks,
        disk *currdisk)
{
    int i;
    double seektime = 0.0;
    double rotation = 0.0;
    double xfer = 0.0;
    unsigned int current_max_sectors = 0;
    unsigned int target_max_sectors = 0;
    double current_angle = 0.0;
    double target_angle = 0.0;

    double period = dm_time_itod(currdisk->model->mech->dm_period(currdisk->model));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: service_time\n", simtime );
    fflush(outputfile);
#endif

    // get the seek time
    {
        struct dm_mech_state p1, p2;
        uint64_t nsecs;
        p1.cyl = current_cyl;
        p1.head = current_head;
        p2.cyl = target_cyl;
        p2.head = target_head;

        nsecs = currdisk->model->mech->dm_seek_time(currdisk->model,
                &p1,
                &p2,
                read);
        seektime = dm_time_itod(nsecs);
    }


    /*    for(i = 0; i <currdisk->numbands; i++){ */
    /*      if(target_cyl < currdisk->bands[i].endcyl && target_max_sectors == 0){ */
    /*        target_max_sectors = currdisk->bands[i].blkspertrack; */
    /*      } */
    /*      if(current_cyl < currdisk->bands[i].endcyl && current_max_sectors == 0){ */
    /*        current_max_sectors = currdisk->bands[i].blkspertrack; */
    /*      } */
    /*    } */

    target_angle = (float)target_sector / (float)target_max_sectors;
    current_angle = (float)current_sector / (float)current_max_sectors;

    // make sure its in [0,1]
    if(target_angle < current_angle){
        target_angle += 1.0;
    }

    // get the rotation time
    {
        struct dm_mech_state p1, p2;
        uint64_t nsecs;
        p1.theta = dm_angle_dtoi(current_angle);
        p2.theta = dm_angle_dtoi(target_angle);
        nsecs = currdisk->model->mech->dm_rottime(currdisk->model,
                p1.theta,
                p2.theta);
        rotation = dm_time_itod(nsecs);
    }

    while(rotation < seektime){
        rotation += period;
    }

    xfer = ((float)blocks / (float)target_max_sectors) * period;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: service_time exit   From %d to %d: seek %f, rotate %f xfer %f\n\n", simtime, current_cyl, target_cyl, seektime, rotation, xfer );
    //  fprintf(stderr,"From %d to %d: seek %f, rotate %f xfer %f\n",current_cyl, target_cyl,seektime,rotation,xfer);
    fflush(outputfile);
#endif

    return(xfer + rotation);
}

#define MAX_TSPS 10

static double min_time;
static int current_head=0;
static int sched_count=0;

static iobuf *requests[MAX_TSPS];

static void remove_tsps(iobuf *tmp){
    int i;
    iobuf *temp;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: remove_tsps\n", simtime );
    fflush(outputfile);
#endif

    temp = tmp;
    for(i=0;i<sched_count;i++){
        if(requests[current_head+i] == temp){
            if((current_head+i+1) < MAX_TSPS){
                requests[current_head+i] = requests[current_head+i+1];
                temp = requests[current_head+i+1];
            }else{
                requests[current_head+i] = NULL;
            }
        }
    }
    if(sched_count != 0){
        sched_count--;
    }
}

static void calc_sp(iobuf **array, subqueue *queue, int checkcache, int no_requests){
    int i;
    int sector;
    ioreq_event *test;
    struct disk *singledisk;

    unsigned int last_head = 0;
    iobuf *temp = NULL;
    char start = 0;
    double acc_time=0.0;
    unsigned int last_cylinder = 0;
    unsigned int last_sector = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: calc_sp::  subqueue %p, checkcache %d, no_requests %d\n", simtime, queue, checkcache, no_requests );
    fflush(outputfile);
#endif

    //  fprintf(stderr,"calc_sp YEAH\n");
    singledisk = getdisk(queue->bigqueue->devno);
    test = (ioreq_event *) getfromextraq();
    for (i=0; i<no_requests; i++) {
        temp = array[i];

        /*    if(acc_time > min_time){
      break;
      }*/
        test->blkno = temp->blkno;
        test->bcount = temp->totalsize;
        test->devno = temp->iolist->devno;
        test->flags = temp->flags;
        test->time = simtime;
        device_get_mapping(queue->bigqueue->cylmaptype, temp->iolist->devno, temp->blkno, NULL, NULL, &sector);
        if(start == 0){
            start = 1;
            acc_time = device_get_acctime(test->devno, test, 100000);
        }else{
            acc_time += service_time(last_cylinder,
                    last_sector,
                    last_head,
                    temp->cylinder,
                    (unsigned int) sector,
                    temp->surface,
                    (temp->flags & READ),
                    temp->totalsize,
                    singledisk);
        }
        last_sector = sector;
        last_cylinder = temp->cylinder;
        last_head = temp->surface;
    }
    if(acc_time < min_time){
        //    fprintf(stderr,"time = %f\n",acc_time);
        //fprintf(stderr,"Setting up acc_time\n");
        for (i=0; i<MAX_TSPS; i++) {
            if(i<no_requests){
                //	fprintf(stderr,"i = %d %d\n",i,array[i]->blkno);
                requests[i] = array[i];
            }else{
                requests[i] = NULL;
            }
        }
        //    fprintf(stderr,"\n");
        min_time = acc_time;
        sched_count = no_requests;
        current_head = 0;
    }
    addtoextraq((event *) test);
}

static void perm(iobuf **array,
        int m,
        subqueue *queue,
        int checkcache,
        int no_requests)
{
    int i;
    iobuf *temp;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: perm::  subqueue %p, m %d, checkcache %d, no_requests %d\n", simtime, queue, m, checkcache, no_requests );
    fflush(outputfile);
#endif

    //  fprintf(stderr,"perm\n");
    if(m == 0){
        /*   for(i=0;i<no_requests;i++){
	 fprintf(stderr,"%ld ",array[i]->blkno);
      }
      fprintf(stderr,"\n");*/
        calc_sp(array, queue, checkcache, no_requests);
    }else{
        for(i=0;i<m;i++){
            temp = array[i];
            array[i] = array[m-1];
            array[m-1] = temp;
            perm(array,m-1,queue,checkcache,no_requests);
            temp = array[m-1];
            array[m-1] = array[i];
            array[i] = temp;
        }
    }
}

/* Queue contains >= 2 items when called */

static iobuf *
ioqueue_get_request_from_opt_tsps_queue(subqueue *queue, int checkcache, int ageweight, int posonly)
{
    int i;
    iobuf *temp;
    iobuf *best = NULL;
    ioreq_event *test;
    double acc_time;
    /*   iobuf *best2 = NULL;
	double mintime = 100000.0;
	double mintime2 = 100000.0;
	double mintime3 = 100000.0;
	double readdelay;
	double writedelay;
	double delay;
	double age;
	double weight;

	double acc_time=0.0;
	double serv_time=0.0;
	double seek_time=0.0;
	double effciency=0.0;
	double band_mult=0.0;
	struct disk *singledisk;
	unsigned int max_sectors=0;
	unsigned int temp_sectors=0;*/
    int request_count=0;
    iobuf *test_requests[MAX_TSPS];

    //   ioreq_event *tmp;

    ASSERT((ageweight >= 0) && (ageweight <= 3));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_opt_tsps_queue::  subqueue %p, checkcache %d, ageweight %d, posonly %d\n", simtime, queue, checkcache, ageweight, posonly );
    fflush(outputfile);
#endif

    /*   singledisk = getdisk(queue->bigqueue->devno);
   for(j=0;j<singledisk->numbands;j++){
     if(singledisk->bands[j].blkspertrack > max_sectors){
       max_sectors = singledisk->bands[j].blkspertrack;
     }
   }

   readdelay = queue->bigqueue->readdelay;
   writedelay = queue->bigqueue->writedelay;
   weight = (double) queue->bigqueue->to_time;*/
    if(sched_count != 0){
        if(requests[current_head] != NULL){
            best = requests[current_head];
            test = (ioreq_event *) getfromextraq();
            test->blkno = best->blkno;
            test->bcount = best->totalsize;
            test->devno = best->iolist->devno;
            test->flags = best->flags;
            test->time = simtime;
            acc_time = device_get_acctime(test->devno, test, 100000);
            addtoextraq((event *) test);
            current_head++;
            sched_count--;
            fprintf (stderr, "2Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n",
                    acc_time, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
            return(best);
        }else{
            fprintf(stderr,"ERROR: request NULL with count != 0\n");
            abort();
        }
    }
    temp = queue->list->next;

    //fprintf(outputfile, "get_request_from_sptf::  listlen = %d\n", queue->listlen);

    min_time = 100000;
    for (i=0; i<queue->listlen; i++) {

        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test_requests[request_count] = temp;
            request_count++;
            if(request_count == MAX_TSPS){
                break;
            }
            /*	test->blkno = temp->blkno;
	test->bcount = temp->totalsize;
	test->devno = temp->iolist->devno;
	test->flags = temp->flags;
	delay = (temp->flags & READ) ? readdelay : writedelay;
	test->time = simtime + delay;
	if (ageweight) {
	  tmp = temp->iolist;
	  age = tmp->time;
	  while (tmp) {
	    if (tmp->time < age) {
	      age = tmp->time;
	    }
	    tmp = tmp->next;
	  }
	  age = simtime - age;
	}
	if ((ageweight == 2) || 
	    ((ageweight == 3) && 
	     (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
	  delay -= age * weight * (double) 0.001;
	}
	if (delay < mintime) {
	  serv_time = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
	  seek_time = device_get_seektime(test->devno, test, checkcache, (mintime - delay));
	  acc_time = device_get_acctime(test->devno, test, checkcache, (mintime - delay));

	  for(j=0;j<singledisk->numbands;j++){
	    if(singledisk->bands[j].endcyl >= temp->cylinder){
	      temp_sectors = singledisk->bands[j].blkspertrack;
	      break;
	    }
	  }
	  band_mult = ((double)temp_sectors/(double)max_sectors);
	  if ((temp->totalsize/acc_time) > effciency) {
	    best = temp;
	    effciency = (temp->totalsize/acc_time);
	    mintime3 = serv_time;
	  }
	  if(serv_time < mintime2){
	    best2 = temp;
	    mintime2 = serv_time;
	  }
	  }*/
        } else {
            //fprintf(outputfile, "not READY_TO_GO\n");
        }

        temp = temp->next;
    }
    if(request_count != MAX_TSPS){
        for(i=request_count;i<MAX_TSPS;i++){
            test_requests[i] = NULL;
        }
    }



    /*   if(best2 != best){
     fprintf (stderr, "\n\n\n\n\nSelected request: %f, cylno %d, blkno %d, read %d, devno %d\n",
	      mintime3, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
     fprintf (stderr, "Best 2Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n\n\n\n\n\n",
	      mintime2, best2->cylinder, best2->blkno, (best2->flags & READ), best2->iolist->devno);
	      }*/
    /*   fprintf (stderr, "Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n",
     mintime, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);*/

    perm(test_requests,request_count,queue,checkcache,request_count);
    fprintf(stderr,"sched count = %d\n",sched_count);
    if(sched_count != 0){
        if(requests[current_head] != NULL){
            best = requests[current_head];
            test = (ioreq_event *) getfromextraq();
            test->blkno = best->blkno;
            test->bcount = best->totalsize;
            test->devno = best->iolist->devno;
            test->flags = best->flags;
            test->time = simtime;
            acc_time = device_get_acctime(test->devno, test, 100000);
            addtoextraq((event *) test);
            current_head++;
            sched_count--;

            fprintf (stderr, "3Selected request: %f, cylno %d, blkno %d, read %d, devno %d %f\n",
                    acc_time, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno, min_time);
            return(best);
        }else{
            fprintf(stderr,"ERROR: request NULL with count != 0\n");
            abort();
        }
    }else{
        fprintf(stderr,"ERROR: sched count = 0\n");
        abort();
    }
    fprintf(stderr, "blah\n");
    return(best);
}

static iobuf *ioqueue_get_request_from_opt_sptf_rot_weight_queue (subqueue *queue, int checkcache, int ageweight, int posonly)
{
    int i;
    iobuf *temp;
    ioreq_event *test;
    double readdelay;
    double writedelay;
    double delay;
    double weight;
    ioreq_event *tmp;

    iobuf *best = NULL;
    double mintime = 100000.0;
    double minsubtract = 0.0;
    double age = 0.0;
    double temp_delay = 0.0;
    double temp_seek = 0.0;

    ASSERT((ageweight >= 0) && (ageweight <= 3));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_opt_sptf_rot_weight_queue::  subqueue %p, checkcache %d, ageweight %d, posonly %d\n", simtime, queue, checkcache, ageweight, posonly );
    fflush(outputfile);
#endif

    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;
    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    temp_seek = device_get_seektime(test->devno, test, checkcache, (mintime - delay));
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                if((temp_delay - temp_seek) < 0.0)
                    fprintf(stderr,"Oops delay negative %f\n",temp_delay-temp_seek);

                if ((delay - (temp_delay - temp_seek) * queue->bigqueue->latency_weight) < (mintime - minsubtract)) {
                    best = temp;
                    mintime = delay;
                    minsubtract = (temp_delay - temp_seek) * queue->bigqueue->latency_weight;
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);

    /*
   fprintf (outputfile, "Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n", mintime, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
     */

    return(best);
}


static iobuf *ioqueue_get_request_from_opt_sptf_weight_queue (subqueue *queue, int checkcache, int ageweight, int posonly)
{
    int i;
    iobuf *temp;
    ioreq_event *test;
    double readdelay;
    double writedelay;
    double weight;
    ioreq_event *tmp;

    iobuf *best = NULL;
    iobuf *best2 = NULL;
    double mintime = 100000.0;
    double temp_mintime = 100000.0;
    double maxrot = 0.0;
    double delay = 0.0;
    double age = 0.0;
    double temp_delay = 0.0;
    double temp_seek = 0.0;

    ASSERT((ageweight >= 0) && (ageweight <= 3));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_opt_sptf_weight_queue::  subqueue %p, checkcache %d, ageweight %d, posonly %d\n", simtime, queue, checkcache, ageweight, posonly );
    fflush(outputfile);
#endif

    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;
    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                if (delay < temp_mintime) {
                    temp_mintime = delay;
                    best2 = temp;
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);


    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;
    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    temp_seek = device_get_seektime(test->devno, test, checkcache, (mintime - delay));
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                /*
	    fprintf(outputfile, "get_request_from_sptf...::  delay = %f\n", delay);
                 */
                if((temp_delay - temp_seek) < 0.0)
                    fprintf(stderr,"Oops delay negative %f\n",temp_delay-temp_seek);

                if (delay <= (temp_mintime * (1+queue->bigqueue->latency_weight)))  {
                    if((temp_delay - temp_seek) >= maxrot){
                        best = temp;
                        maxrot = temp_delay - temp_seek;
                    }
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);

    /*
   fprintf (outputfile, "Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n", mintime, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
     */

    return(best);
}

static iobuf *ioqueue_get_request_from_opt_sptf_seek_weight_queue (subqueue *queue, int checkcache, int ageweight, int posonly)
{
    int i;
    iobuf *temp;
    iobuf *best = NULL;
    iobuf *best2 = NULL;
    ioreq_event *test;
    double mintime = 100000.0;
    double temp_mintime = 100000.0;
    double minseek = 100000.0;
    double readdelay;
    double writedelay;
    double delay = 0.0;
    double age = 0.0;
    double weight;

    double temp_delay = 0.0;
    double temp_seek = 0.0;

    ioreq_event *tmp;

    ASSERT((ageweight >= 0) && (ageweight <= 3));

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_opt_sptf_seek_weight_queue::  subqueue %p, checkcache %d, ageweight %d, posonly %d\n", simtime, queue, checkcache, ageweight, posonly );
    fflush(outputfile);
#endif

    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;

    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    temp_seek = device_get_seektime(test->devno, test, checkcache, (mintime - delay));
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                /*
	    fprintf(outputfile, "get_request_from_sptf...::  delay = %f\n", delay);
                 */

                if (delay < temp_mintime) {
                    temp_mintime = delay;
                    best2 = temp;
                }
                if (delay <= (temp_mintime * (1+queue->bigqueue->latency_weight)))  {
                    if(temp_seek < minseek){
                        best = temp;
                        minseek = temp_seek;
                    }
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);

#if 0
    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    //fprintf(outputfile, "temp_delay = %f\n",temp_delay);
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                /*
	    fprintf(outputfile, "get_request_from_sptf...::  delay = %f\n", delay);
                 */

                if (delay < temp_mintime) {
                    temp_mintime = delay;
                    best2 = temp;
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);

    readdelay = queue->bigqueue->readdelay;
    writedelay = queue->bigqueue->writedelay;
    weight = (double) queue->bigqueue->to_time;
    test = (ioreq_event *) getfromextraq();
    temp = queue->list->next;
    for (i=0; i<queue->listlen; i++) {
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;
            delay = (temp->flags & READ) ? readdelay : writedelay;
            test->time = simtime + delay;
            if (ageweight) {
                tmp = temp->iolist;
                age = tmp->time;
                while (tmp) {
                    if (tmp->time < age) {
                        age = tmp->time;
                    }
                    tmp = tmp->next;
                }
                age = simtime - age;
            }
            if ((ageweight == 2) ||
                    ((ageweight == 3) &&
                            (test->flags & (TIME_CRITICAL | TIME_LIMITED)))) {
                delay -= age * weight * (double) 0.001;
            }
            if (delay < mintime) {
                if (posonly) {
                    temp_delay = device_get_servtime(test->devno, test, checkcache, (mintime - delay));
                    temp_seek = device_get_seektime(test->devno, test, checkcache, (mintime - delay));
                    //fprintf(outputfile, "temp_delay = %f\n",temp_delay);
                    delay += temp_delay;
                } else {
                    delay += device_get_acctime(test->devno, test, (mintime - delay));
                }
                if (ageweight == 1) {
                    delay *= (weight - age) / weight;
                }

                /*
	    fprintf(outputfile, "get_request_from_sptf...::  delay = %f\n", delay);
                 */
                if(temp_seek < 0.0)
                    fprintf(stderr,"Oops delay negative %f\n",temp_delay-temp_seek);

                if (delay <= (temp_mintime * (1+queue->bigqueue->latency_weight)))  {
                    if(temp_seek < minseek){
                        best = temp;
                        minseek = temp_seek;
                    }
                }
            }
        }
        temp = temp->next;
    }
    addtoextraq((event *) test);

#endif
    /*
   fprintf (outputfile, "Selected request: %f, cylno %d, blkno %d, read %d, devno %d\n", mintime, best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno);
     */

    return(best);
}

/*
 * 
 *  ioqueue_get_request_from_sdf_queue()
 *
 *  subqueue queue - the queue
 *  int exact      - a flag to device_get_distance: if FALSE, then calculate from the last lbn requested,
 *                   if TRUE, calculate from current device position
 *  int direction  - a flag to device_get_distance: if FALSE, don't take distance into account
 *                   if TRUE, return an "equivalent distance" taking into account the current direction
 *                   of the moving sled/disk arm
 *
 *  returns the request with the shortest distance from the current position
 *
 */
 
static iobuf *ioqueue_get_request_from_sdf_queue (subqueue *queue, int exact, int direction) {

    int i;
    iobuf *temp;
    iobuf *best = NULL;
    ioreq_event *test;

    int distance;
    int min_distance = 1000000;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_sdf_queue::  exact = %d, direction = %d\n", simtime, exact, direction );
    fflush(outputfile);
#endif

    test = (ioreq_event *) getfromextraq();

    temp = queue->list->next;
    for (i=0; i<queue->listlen; i++) {
        // fprintf(outputfile, "temp->state = %d\n", temp->state);
        if (READY_TO_GO(temp,queue) && (ioqueue_seqstream_head(queue->bigqueue, queue->list->next, temp))) {
            test->blkno = temp->blkno;
            test->bcount = temp->totalsize;
            test->devno = temp->iolist->devno;
            test->flags = temp->flags;

            if (exact) {
                distance = device_get_distance(test->devno, test, -1, direction);
            }
            else {
                distance = device_get_distance(test->devno, test, queue->lastblkno, direction);
            }

            // fprintf(outputfile, "get_from_sdf_queue::  blkno = %d, distance = %d\n", test->blkno, distance);

            if (distance < min_distance) {
                best = temp;
                min_distance = distance;
            }
        } else {
            // fprintf(outputfile, "not READY_TO_GO\n");
        }
        temp = temp->next;
    }

    addtoextraq((event *) test);

    /*
  fprintf (outputfile, "Selected request: cylno %d, blkno %d, read %d, devno %d, with distance %d\n",
	   best->cylinder, best->blkno, (best->flags & READ), best->iolist->devno, min_distance);
     */

    return(best);
}


/* Queue contains >= 2 items when called */

static iobuf *ioqueue_get_request_from_batch_fcfs_queue(subqueue *queue)
{
    iobuf *tmp;
    iobuf *ret = NULL;
    iobuf *first_in_batch = NULL;
    iobuf *last_in_batch;
    int i;
    int batchno;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_request_from_batch_fcfs_queue\n", simtime );
    fflush(outputfile);
#endif

    printf("ioqueue_get_request_from_batch_fcfs_queue:: queue contents\n");
    print_batch_fcfs_queue(queue);

    tmp = queue->list->next;
    for (i = 0; i < queue->iobufcnt; i++) {
        if (READY_TO_GO(tmp,queue)) {
            //stat_update(&queue->bigqueue->batchsizestats, tmp->batch_size);
            return (tmp);
        }
        tmp = tmp->next;
    }

    printf("ioqueue_get_request_from_batch_fcfs_queue:: none of the requests were READY_TO_GO\n");
    print_batch_fcfs_queue(queue);

    return(NULL);
}


static ioreq_event * ioqueue_show_next_request_from_subqueue (subqueue *queue)
{
    iobuf *temp = 0;
    ioreq_event *ret;

#if 0
    iobuf *temp_sdf = NULL;
    iobuf *temp_sptf = NULL;
#endif

    int tmpdir;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_show_next_request_from_subqueue\n", simtime );
    fflush(outputfile);
#endif

    tmpdir = queue->dir;
    if ((queue->iobufcnt - queue->reqoutstanding) == 0) {
        return(NULL);
    } else if (queue->iobufcnt == 1) {
        if (READY_TO_GO(queue->list,queue)) {
            temp = queue->list;
        } else {
            temp = NULL;
        }
    } else {
        if (queue->sched_alg == FCFS) {
            temp = ioqueue_get_request_from_fcfs_queue(queue);
        } else if (queue->sched_alg == PRI_VSCAN_LBN) {
            temp = ioqueue_get_request_from_pri_lbn_vscan_queue(queue, device_get_number_of_blocks(queue->list->devno), queue->vscan_cyls);
        } else if (queue->sched_alg == ELEVATOR_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, device_get_number_of_blocks(queue->list->devno));
        } else if (queue->sched_alg == CYCLE_LBN) {
            temp = ioqueue_get_request_from_lbn_scan_queue(queue);
        } else if (queue->sched_alg == SSTF_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, 0);
        } else if (queue->sched_alg == ELEVATOR_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, device_get_numcyls(queue->list->devno));
        } else if (queue->sched_alg == CYCLE_CYL) {
            temp = ioqueue_get_request_from_cyl_scan_queue(queue);
        } else if (queue->sched_alg == SSTF_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, 0);
        } else if (queue->sched_alg == SPTF_ROT_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_rot_weight_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_ROT_WEIGHT) {
            temp = ioqueue_get_request_from_opt_sptf_weight_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_SEEK_WEIGHT) {
            temp = ioqueue_get_request_from_opt_sptf_seek_weight_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == TSPS) {
            temp = ioqueue_get_request_from_opt_tsps_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);

#if 0	
            temp_sdf = ioqueue_get_request_from_sdf_queue(queue, FALSE, FALSE);

            if ((temp_sdf != NULL) && (temp != NULL)) {

                fprintf(outputfile, "sptf_scheduler::  sptf = %d, sdf = %d\n",
                        temp->blkno, temp_sdf->blkno);

                if (temp_sdf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;

                    fprintf(outputfile, "sptf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
                            temp->blkno, temp_sdf->blkno, queue->num_sptf_sdf_different);

                }
            }
#endif
        } else if (queue->sched_alg == SPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 0, TRUE);
        } else if (queue->sched_alg == SATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 0, FALSE);
        } else if (queue->sched_alg == WPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 1, TRUE);
        } else if (queue->sched_alg == WPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 1, TRUE);
        } else if (queue->sched_alg == WATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 1, FALSE);
        } else if (queue->sched_alg == ASPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 2, TRUE);
        } else if (queue->sched_alg == ASPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 2, TRUE);
        } else if (queue->sched_alg == PRI_ASPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 3, TRUE);
        } else if (queue->sched_alg == PRI_ASPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 3, TRUE);
        } else if (queue->sched_alg == ASATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 2, FALSE);
        } else if (queue->sched_alg == VSCAN_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, queue->vscan_cyls);
        } else if (queue->sched_alg == VSCAN_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, queue->vscan_cyls);
        } else if (queue->sched_alg == SDF_APPROX) {
            temp = ioqueue_get_request_from_sdf_queue(queue, FALSE, FALSE);

#if 0
            temp_sptf = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);
            if ((temp_sptf != NULL) && (temp != NULL)) {
                if (temp_sptf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;
                    /*
	    fprintf(outputfile, "sdf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
		    temp_sptf->blkno, temp->blkno, queue->num_sptf_sdf_different);
                     */
                }
            }
#endif

        } else if (queue->sched_alg == SDF_EXACT) {
            temp = ioqueue_get_request_from_sdf_queue(queue, TRUE, FALSE);

#if 0
            temp_sptf = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);

            if ((temp_sptf != NULL) && (temp != NULL)) {
                if (temp_sptf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;
                    /*
	    fprintf(outputfile, "sdf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
		    temp_sptf->blkno, temp->blkno, queue->num_sptf_sdf_different);
                     */
                }
            }
#endif
        }

    }
    queue->dir = tmpdir;
    if (temp == NULL) {
        return(NULL);
    } else if (!READY_TO_GO(temp,queue)) {
        fprintf(stderr, "Selected request is !READY_TO_GO in show_next_request_from_subqueue\n");
        exit(1);
    }
    ASSERT(temp->iolist != NULL);
    if (temp->reqcnt == 1) {
        ret = temp->iolist;
    } else if (temp->iob_un.pend.concat) {
        ret = temp->iob_un.pend.concat;
    } else {
        ret = (ioreq_event *) getfromextraq();
        temp->iob_un.pend.concat = ret;
        ret->time = simtime;
        ret->type = temp->iolist->type;
        ret->next = NULL;
        ret->prev = NULL;
        ret->bcount = temp->totalsize;
        ret->blkno = temp->blkno;
        ret->flags = temp->flags;
        ret->busno = temp->iolist->busno;
        ret->slotno = temp->iolist->slotno;
        ret->devno = temp->iolist->devno;
        ret->opid = temp->opid;
        ret->buf = temp->iolist->buf;
    }

    queue->num_scheduling_decisions++;

    return(ret);
}


static ioreq_event * ioqueue_get_next_request_from_subqueue (subqueue *queue)
{
    iobuf *temp = NULL;
#if 0
    iobuf *temp_sdf = NULL;
    iobuf *temp_sptf = NULL;
#endif

    ioreq_event *ret;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_next_request_from_subqueue\n", simtime );
    fflush(outputfile);
#endif

    ioqueue_update_subqueue_statistics(queue);
#ifdef DEBUG_IOQUEUE
    ioqueue_print_subqueue_state(queue);
#endif

    if ((queue->iobufcnt - queue->reqoutstanding) == 0) {
#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_get_next_request_from_subqueue  path 1  returning NULL\n", simtime );
        fflush(outputfile);
#endif

        return(NULL);
    } else if (queue->iobufcnt == 1) {
#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_get_next_request_from_subqueue  path 2\n", simtime );
        fflush(outputfile);
#endif

        if (READY_TO_GO(queue->list,queue)) {
            temp = queue->list;
            switch (queue->sched_alg) {
            case ELEVATOR_LBN:
            case PRI_VSCAN_LBN:
            case VSCAN_LBN:
                if (queue->lastblkno != temp->blkno) {
                    queue->dir = (queue->lastblkno < temp->blkno) ? ASC : DESC;
                }
                break;

            case ELEVATOR_CYL:
            case VSCAN_CYL:
                if (queue->lastcylno != temp->cylinder) {
                    queue->dir = (queue->lastcylno < temp->cylinder) ? ASC : DESC;
                }
                break;
                /*
	    case BATCH_FCFS:
	       if ((temp->batchno != -1) && (temp->batch_complete == 0)) {
		 temp = NULL;
	       }
	       break;
                 */
            }
        } else {                      /* !READY_TO_GO */
            temp = NULL;
        }
    } else {
#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_get_next_request_from_subqueue  path 3\n", simtime );
        fflush(outputfile);
#endif

        if (queue->sched_alg == FCFS) {
            temp = ioqueue_get_request_from_fcfs_queue(queue);
        } else if (queue->sched_alg == PRI_VSCAN_LBN) {
            temp = ioqueue_get_request_from_pri_lbn_vscan_queue(queue, device_get_number_of_blocks(queue->list->devno), queue->vscan_cyls);
        } else if (queue->sched_alg == ELEVATOR_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, device_get_number_of_blocks(queue->list->devno));
        } else if (queue->sched_alg == CYCLE_LBN) {
            temp = ioqueue_get_request_from_lbn_scan_queue(queue);
        } else if (queue->sched_alg == SSTF_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, 0);
        } else if (queue->sched_alg == ELEVATOR_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, device_get_numcyls(queue->list->devno));
        } else if (queue->sched_alg == CYCLE_CYL) {
            temp = ioqueue_get_request_from_cyl_scan_queue(queue);
        } else if (queue->sched_alg == SSTF_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, 0);
        } else if (queue->sched_alg == SPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_ROT_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_rot_weight_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_ROT_WEIGHT) {
            temp = ioqueue_get_request_from_opt_sptf_weight_queue(queue, FALSE, 0, TRUE);
        } else if (queue->sched_alg == SPTF_SEEK_WEIGHT) {
            temp = ioqueue_get_request_from_opt_sptf_seek_weight_queue(queue, FALSE, 0, TRUE);

        } else if (queue->sched_alg == TSPS) {
            temp = ioqueue_get_request_from_opt_tsps_queue(queue, FALSE, 0, TRUE);

#if 0
            temp_sdf = ioqueue_get_request_from_sdf_queue(queue, FALSE, FALSE);


            if ((temp_sdf != NULL) && (temp != NULL)) {

                fprintf(outputfile, "sptf_scheduler::  sptf = %d, sdf = %d\n",
                        temp->blkno, temp_sdf->blkno);

                if (temp_sdf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;

                    fprintf(outputfile, "sptf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
                            temp->blkno, temp_sdf->blkno, queue->num_sptf_sdf_different);

                }
            }
#endif

        } else if (queue->sched_alg == SPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 0, TRUE);
        } else if (queue->sched_alg == SATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 0, FALSE);
        } else if (queue->sched_alg == WPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 1, TRUE);
        } else if (queue->sched_alg == WPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 1, TRUE);
        } else if (queue->sched_alg == WATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 1, FALSE);
        } else if (queue->sched_alg == ASPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 2, TRUE);
        } else if (queue->sched_alg == ASPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 2, TRUE);
        } else if (queue->sched_alg == PRI_ASPTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 3, TRUE);
        } else if (queue->sched_alg == PRI_ASPCTF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, TRUE, 3, TRUE);
        } else if (queue->sched_alg == ASATF_OPT) {
            temp = ioqueue_get_request_from_opt_sptf_queue(queue, 0, 2, FALSE);
        } else if (queue->sched_alg == VSCAN_LBN) {
            temp = ioqueue_get_request_from_lbn_vscan_queue(queue, queue->vscan_cyls);
        } else if (queue->sched_alg == VSCAN_CYL) {
            temp = ioqueue_get_request_from_cyl_vscan_queue(queue, queue->vscan_cyls);
        } else if (queue->sched_alg == SDF_APPROX) {
            temp = ioqueue_get_request_from_sdf_queue(queue, FALSE, FALSE);

#if 0
            temp_sptf = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);

            if ((temp_sptf != NULL) && (temp != NULL)) {
                if (temp_sptf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;
                    /*
	    fprintf(outputfile, "sdf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
		    temp_sptf->blkno, temp->blkno, queue->num_sptf_sdf_different);
                     */
                }
            }
#endif

        } else if (queue->sched_alg == SDF_EXACT) {
            temp = ioqueue_get_request_from_sdf_queue(queue, TRUE, FALSE);

#if 0
            temp_sptf = ioqueue_get_request_from_opt_sptf_queue(queue, FALSE, 0, TRUE);

            if ((temp_sptf != NULL) && (temp != NULL)) {
                if (temp_sptf->blkno != temp->blkno) {
                    queue->num_sptf_sdf_different++;
                    /*
	    fprintf(outputfile, "sdf scheduler::  sptf = %d, sdf = %d, num_different = %d\n",
		    temp_sptf->blkno, temp->blkno, queue->num_sptf_sdf_different);
                     */
                }
            }
#endif
        } else if (queue->sched_alg == BATCH_FCFS) {
            temp = ioqueue_get_request_from_batch_fcfs_queue(queue);
        }
    }

    if (temp == NULL) {
        // fprintf(stderr, "RETURNING NULL!!\n");
        return(NULL);
    } else if (!READY_TO_GO(temp,queue)) {
        fprintf(stderr, "Selected request is !READY_TO_GO in get_next_request_from_subqueue\n");
        exit(1);
    }

#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_get_next_request_from_subqueue  Selected request: blkno %d, cylno %d, surface %d\n", simtime, temp->blkno, temp->cylinder, temp->surface );
        fflush(outputfile);
#endif

    switch (queue->sched_alg) {
    case ELEVATOR_LBN:
    case CYCLE_LBN:
    case PRI_VSCAN_LBN:
    case ELEVATOR_CYL:
    case CYCLE_CYL:
        queue->lastblkno = temp->blkno;
        break;

    case VSCAN_LBN:
    case VSCAN_CYL:
        if (queue->vscan_cyls > 0) {
            queue->lastblkno = temp->blkno;
            break;
        }
    case SSTF_LBN:
        if (queue->sched_alg != VSCAN_CYL) {
            queue->lastblkno = temp->blkno + temp->totalsize;
            if (queue->lastblkno != device_get_number_of_blocks(temp->iolist->devno)) {
                break;
            }
        }
    case BATCH_FCFS:
        stat_update(&queue->bigqueue->batchsizestats, temp->batch_size);
        break;
    default:
        queue->lastblkno = temp->blkno + temp->totalsize - 1;
        break;
    }
    ioqueue_get_cylinder_mapping(queue->bigqueue, temp, queue->lastblkno, &queue->lastcylno, &queue->lastsurface, -1);
    ioqueue_get_cylinder_mapping(queue->bigqueue, temp, queue->lastblkno, &queue->optcylno, &queue->optsurface, MAP_FULL);

    if (temp->starttime == -1.0) {
        temp->starttime = simtime;
    }
    ret = temp->iolist;
    stat_update(&queue->instqueuelen, (double)(queue->iobufcnt - queue->reqoutstanding));
    queue->reqoutstanding++;
    queue->numoutstanding += temp->reqcnt;
    while (ret) {
        stat_update(&queue->qtimestats, (temp->starttime - ret->time));
        ret = ret->next;
    }
    if (temp->iolist == NULL) {
        fprintf(stderr, "iobuf structure with no iolist chosen to be scheduled\n");
        exit(1);
    } else if (temp->reqcnt == 1) {
        ret = temp->iolist;
        if (ret->next) {
            fprintf(stderr, "More than one request when temp->reqcnt == 1\n");
            exit(1);
        }
        temp->iolist = NULL;
        temp->iob_un.time = ret->time;
    } else if (temp->iob_un.pend.concat) {
        ret = temp->iob_un.pend.concat;
        temp->iob_un.pend.concat = NULL;
    } else if (queue->sched_alg == BATCH_FCFS) {
        ret = temp->batch_list;
    } else {
        ret = (ioreq_event *) getfromextraq();
        ret->time = temp->starttime;
        ret->type = temp->iolist->type;
        ret->next = NULL;
        ret->prev = NULL;
        ret->bcount = temp->totalsize;
        ret->blkno = temp->blkno;
        ret->flags = temp->flags;
        ret->busno = temp->iolist->busno;
        ret->slotno = temp->iolist->slotno;
        ret->devno = temp->iolist->devno;
        ret->opid = temp->opid;
        ret->buf = temp->iolist->buf;
    }
    queue->current = temp;
    temp->state = PENDING;

    queue->num_scheduling_decisions++;

    return(ret);
}


static int ioqueue_request_match (ioreq_event *wanted, iobuf *curr)
{
    ioreq_event *tmp;

#ifdef DEBUG_IOQUEUE
   	dumpIOReq( "ioqueue_request_match  wanted", wanted );
   	fprintf( outputfile, "ioqueue_request_match  iobuf:\n" );
   	fprintf( outputfile, "ioqueue_request_match  iobuf: blkno %d, flags %x, opid %d, totalsize %d\n", curr->blkno, curr->flags, curr->opid, curr->totalsize );
    fflush(outputfile);
#endif

    /* bcount undefined for completing requests */

    if (curr->batch_list != NULL) {
        tmp = curr->batch_list;
        while (tmp) {
            if ((tmp->blkno == wanted->blkno) && (tmp->opid == wanted->opid)) {
                return (1);
            }
            tmp = tmp->batch_next;
        }
    } else {
        if ((curr->blkno <= wanted->blkno) && ((curr->blkno + curr->totalsize) >= wanted->blkno)) {
            if ((curr->blkno == wanted->blkno) && (curr->opid == wanted->opid) && ((curr->flags & READ) == (wanted->flags & READ))) {
                return(1);
            }
            tmp = curr->iolist;
            while (tmp) {
                if ((tmp->blkno == wanted->blkno) && (tmp->opid == wanted->opid)) {
                    return(1);
                }
                tmp = tmp->next;
            }
        }
    }
    return(0);
}


/* Note: use of this function with concat will result in iobuf containing
   the specified request */

static ioreq_event * ioqueue_get_specific_request_from_subqueue (subqueue *queue, ioreq_event *wanted)
{
    iobuf *temp;
    ioreq_event *ret;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_specific_request_from_subqueue  blkno %d\n", simtime, wanted->blkno );
    fflush(outputfile);
#endif

    ioqueue_update_subqueue_statistics(queue);
    if ((queue->iobufcnt - queue->reqoutstanding) == 0) {
        return(NULL);
    } else {
        temp = queue->list;
        while ((temp->next != queue->list) && (ioqueue_request_match(wanted, temp) == 0)) {
            temp = temp->next;
        }
    }
    if (ioqueue_request_match(wanted, temp) == 0) {
        return(NULL);
    }
    ASSERT(READY_TO_GO(temp,queue));

    switch (queue->sched_alg) {
    case ELEVATOR_LBN:
    case CYCLE_LBN:
    case PRI_VSCAN_LBN:
    case ELEVATOR_CYL:
    case CYCLE_CYL:
        queue->lastblkno = temp->blkno;
        break;

    case VSCAN_LBN:
    case VSCAN_CYL:
        if (queue->vscan_cyls > 0) {
            queue->lastblkno = temp->blkno;
            break;
        }
    case SSTF_LBN:
        if (queue->sched_alg != VSCAN_CYL) {
            queue->lastblkno = temp->blkno + temp->totalsize;
            if (queue->lastblkno != device_get_number_of_blocks(temp->iolist->devno)) {
                break;
            }
        }
        break;
    case BATCH_FCFS:
        if (temp->batchno != -1) {
            iobuf *batch_ptr = temp;
            while (batch_ptr->batchno == temp->batchno) {
                if (batch_ptr->state == PENDING) {
                    break;  // this sucks
                }
                if (batch_ptr->starttime == -1.0) {
                    batch_ptr->starttime = simtime;
                }
                queue->reqoutstanding++;
                batch_ptr->state = PENDING;
                batch_ptr = batch_ptr->next;
            }
        }

    default:
        queue->lastblkno = temp->blkno + temp->totalsize - 1;
        break;
    }
    ioqueue_get_cylinder_mapping(queue->bigqueue, temp, queue->lastblkno, &queue->lastcylno, &queue->lastsurface, -1);
    if (temp->starttime == -1.0) {
        temp->starttime = simtime;
    }
    ret = temp->iolist;
    stat_update(&queue->instqueuelen, (double)(queue->iobufcnt - queue->reqoutstanding));
    queue->reqoutstanding++;
    queue->numoutstanding += temp->reqcnt;
    while (ret) {
        stat_update(&queue->qtimestats, (temp->starttime - ret->time));
        ret = ret->next;
    }
    ASSERT(temp->iolist != NULL);
    if (temp->reqcnt == 1) {
        ret = temp->iolist;
        temp->iolist = NULL;
        temp->iob_un.time = ret->time;
    } else if (temp->iob_un.pend.concat) {
        ret = temp->iob_un.pend.concat;
        temp->iob_un.pend.concat = NULL;
    } else {
        ret = (ioreq_event *) getfromextraq();
        ret->time = temp->starttime;
        ret->type = temp->iolist->type;
        ret->next = NULL;
        ret->prev = NULL;
        ret->bcount = temp->totalsize;
        ret->blkno = temp->blkno;
        ret->flags = temp->flags;
        ret->busno = temp->iolist->busno;
        ret->slotno = temp->iolist->slotno;
        ret->devno = temp->iolist->devno;
        ret->opid = temp->opid;
        ret->buf = temp->iolist->buf;
    }
    queue->current = temp;
    temp->state = PENDING;
    return(ret);
}


static ioreq_event * ioqueue_set_starttime_in_subqueue (subqueue *queue, ioreq_event *target)
{
    iobuf *temp = queue->list;
    ioreq_event *ret;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_set_starttime_in_subqueue  blkno %d\n", simtime, target->blkno );
    fflush(outputfile);
#endif

    if ((queue->iobufcnt - queue->reqoutstanding) == 0) {
        return(NULL);
    } else {
        while ((temp->next != queue->list) && (!ioqueue_request_match(target, temp))) {
            temp = temp->next;
        }
    }
    if (!ioqueue_request_match(target, temp)) {
        return(NULL);
    }
    ASSERT(temp->state == WAITING);
    if (temp->starttime == -1.0) {
        temp->starttime = simtime;
    }
    ret = temp->iolist;
    return(ret);
}


static ioreq_event * ioqueue_remove_completed_request (subqueue *queue, ioreq_event *done)
{
    iobuf *tmp;
    iobuf *tail;
    ioreq_event *trv;

    ioqueue_update_subqueue_statistics(queue);
    tmp = queue->current;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_remove_completed_request  listlen %d, starttime %f, stoptime %f, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, queue->listlen, tmp->starttime, tmp->iob_un.time, tmp->devno, tmp->blkno, tmp->totalsize, tmp->flags );
#endif

    ASSERT (tmp != NULL);
    /* NOTE- the following match will fail with concat unless fixed properly */
    if ((tmp->state != PENDING) || (ioqueue_request_match(done, tmp) == 0)) {
        tmp = queue->list->next;
        tail = queue->list;
        while ((tmp != tail) && (ioqueue_request_match(done, tmp) == 0)) {
            tmp = tmp->next;
        }
        if ((tmp->state != PENDING) || (ioqueue_request_match(done, tmp) == 0)) {
            fprintf(stderr, "Completed event not found pending in ioqueue - blkno %d, tmp->blkno %d, tmp->totalsize %d, %d, %d, %d, %d\n", done->blkno, tmp->blkno, tmp->totalsize, (tmp == NULL), done->opid, tmp->opid, done->bcount);
            fprintf(stderr, "tmp->state %d\n", tmp->state);
            assert(0);
        }
    }

    /*
   if (queue->sched_alg == BATCH_FCFS) {
     ioqueue_remove_from_batch_fcfs_subqueue(queue, tmp, done);
   } else {
     ioqueue_remove_from_subqueue(queue, tmp);
   }
     */

    if (tmp->reqcnt == 1) {
        ioqueue_remove_from_subqueue(queue, tmp);
        trv = done;
        trv->bcount = tmp->totalsize;
        trv->time = tmp->starttime;
        done->next = NULL;
        queue->iobufcnt--;
        queue->reqoutstanding--;
        queue->listlen--;
        if (tmp->flags & READ) {
            queue->readlen--;
        }
        queue->numoutstanding--;
        queue->numcomplete++;
        stat_update(&queue->accstats, (simtime - tmp->starttime));
        disksim->lastphystime = simtime - tmp->starttime;
        stat_update(&queue->outtimestats, (simtime - tmp->iob_un.time));
        // printf("Updated outtimestats with %f - simtime = %f, iob_un.time = %f\n", simtime - tmp->iob_un.time, simtime, tmp->iob_un.time);
        if (tmp->flags & READ) {
            if (tmp->flags & TIME_CRITICAL) {
                stat_update(&queue->critreadstats, (simtime - tmp->iob_un.time));
            } else {
                stat_update(&queue->nocritreadstats, (simtime - tmp->iob_un.time));
            }
        } else {
            if (tmp->flags & TIME_CRITICAL) {
                stat_update(&queue->critwritestats, (simtime - tmp->iob_un.time));
            } else {
                stat_update(&queue->nocritwritestats, (simtime - tmp->iob_un.time));
            }
        }
    } else {
        if (queue->sched_alg == BATCH_FCFS) {
            ioreq_event *batch_ptr = tmp->batch_list;
            // trv = tmp->batch_list;
            trv = done;

            while (batch_ptr != NULL) {
                if ((batch_ptr->blkno == done->blkno) &&
                        (batch_ptr->opid == done->opid) &&
                        ((batch_ptr->flags & READ) == (done->flags & READ))) {
                    if (tmp->batch_size == 1) {
                        tmp->batch_size--;
                        break;
                    } else if (tmp->batch_list == batch_ptr) {
                        tmp->batch_list = batch_ptr->batch_next;
                        batch_ptr->batch_next->batch_prev = NULL;
                        tmp->batch_size--;
                        break;
                    } else {
                        batch_ptr->batch_prev->batch_next = batch_ptr->batch_next;
                        if (batch_ptr->batch_next != NULL) {
                            batch_ptr->batch_next->batch_prev = batch_ptr->batch_prev;
                        }
                        tmp->batch_size--;
                        break;
                    }
                }
                batch_ptr = batch_ptr->batch_next;
            }

            tmp->reqcnt--;
            queue->reqoutstanding--;
            queue->listlen--;
            if (trv->flags & READ) {
                queue->readlen--;
            }
            queue->numoutstanding--;
            queue->numcomplete++;
            stat_update(&queue->accstats, (simtime - tmp->starttime));
            disksim->lastphystime = simtime - tmp->starttime;
            stat_update(&queue->outtimestats, (simtime - tmp->iob_un.time));
            // printf("Updated outtimestats with %f - simtime = %f\n", simtime - tmp->iob_un.time, simtime);
            if (tmp->flags & READ) {
                if (tmp->flags & TIME_CRITICAL) {
                    stat_update(&queue->critreadstats, (simtime - tmp->iob_un.time));
                } else {
                    stat_update(&queue->nocritreadstats, (simtime - tmp->iob_un.time));
                }
            } else {
                if (tmp->flags & TIME_CRITICAL) {
                    stat_update(&queue->critwritestats, (simtime - tmp->iob_un.time));
                } else {
                    stat_update(&queue->nocritwritestats, (simtime - tmp->iob_un.time));
                }
            }
            return(done);
        } else {
            trv = tmp->iolist;
            ioqueue_remove_from_subqueue(queue, tmp);
            queue->iobufcnt--;
            queue->reqoutstanding--;
            queue->listlen -= tmp->reqcnt;
            if (trv->flags & READ) {
                queue->readlen -= tmp->reqcnt;
            }
            queue->numoutstanding -= tmp->reqcnt;
            queue->numcomplete += tmp->reqcnt;
            // move this to add_request, when the batch is done
            // stat_update(&queue->bigqueue->batchsizestats, tmp->batch_size);
            while (trv) {
                stat_update(&queue->accstats, (simtime - tmp->starttime));
                disksim->lastphystime = simtime - tmp->starttime;
                stat_update(&queue->outtimestats, (simtime - trv->time));
                // printf("Updated outtimestats with %f - simtime = %f\n", simtime - tmp->iob_un.time, simtime);
                if (trv->flags & READ) {
                    if (trv->flags & TIME_CRITICAL) {
                        stat_update(&queue->critreadstats, (simtime - trv->time));
                    } else {
                        stat_update(&queue->nocritreadstats, (simtime - trv->time));
                    }
                } else {
                    if (trv->flags & TIME_CRITICAL) {
                        stat_update(&queue->critwritestats, (simtime - trv->time));
                    } else {
                        stat_update(&queue->nocritwritestats, (simtime - trv->time));
                    }
                }
                trv = trv->next;
            }
            done->batch_next = NULL;
            // addtoextraq((event *) done);
            //if (queue->sched_alg == BATCH_FCFS) {
            //trv = tmp->batch_list;
            //}	else {
            trv = tmp->iolist;
            //}
        }
    }
    // tmp->batch_next = NULL;
    addtoextraq((event *) tmp);
    /*
fprintf (outputfile, "Exiting remove_completed_request\n");
     */
    return(trv);
}


ioreq_event *ioqueue_get_specific_request(ioqueue *queue, ioreq_event *wanted)
{
    ioreq_event *tmp = NULL;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_get_specific_request  blkno %d\n", simtime, wanted->blkno );
    fflush(outputfile);
#endif

    if((queue->priority.listlen - queue->priority.numoutstanding) > 0) {
        tmp = ioqueue_get_specific_request_from_subqueue(&queue->priority,
                wanted);
        queue->lastsubqueue = IOQUEUE_PRIORITY;
    }

    if ((tmp == NULL) &&
            ((queue->timeout.listlen - queue->timeout.numoutstanding) > 0)) {
        tmp = ioqueue_get_specific_request_from_subqueue(&queue->timeout, wanted);
        queue->lastsubqueue = IOQUEUE_TIMEOUT;
    }

    if ((tmp == NULL)
            && ((queue->base.listlen - queue->base.numoutstanding) > 0)) {
        tmp = ioqueue_get_specific_request_from_subqueue(&queue->base, wanted);
        queue->lastsubqueue = IOQUEUE_BASE;
    }

    if (tmp) {
        tmp->time = 0.0;
    }

    /*
     * fprintf (outputfile, "Exiting ioqueue_get_specific_request: %x\n", tmp);
     */

    return tmp;
}


/* This routine allows the queue owner to set the starttime without changing
   the state of the queue or the request (i.e., setting the state to PENDING)
   Specifically, this is used by advanced disk models which allow other
   activity to occur for a request in advance of the actual selection of the
   request to receive dedicated HDA (mechanical) activity.  Thus, the request
   needs to remain WAITING in order to enable algorithmic mechanical latency
   reduction.

   Not callable for concat queues, as a concat request may change inside
   the queue after the starttime has been set, which means the queue owner
   and the queue itself may have different ideas as to what constitutes the
   entire request.
*/

ioreq_event * ioqueue_set_starttime (ioqueue *queue, ioreq_event *target)
{
    ioreq_event *tmp = NULL;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_set_starttime  blkno %d\n", simtime, target->blkno );
    fflush(outputfile);
#endif

    /* Queue must not be concatable! */
    ASSERT((queue->seqscheme & IOQUEUE_CONCAT_BOTH) == 0);

    if ((queue->priority.listlen - queue->priority.numoutstanding) > 0) {
        tmp = ioqueue_set_starttime_in_subqueue(&queue->priority, target);
    }
    if (!tmp && ((queue->timeout.listlen - queue->timeout.numoutstanding) > 0)) {
        tmp = ioqueue_set_starttime_in_subqueue(&queue->timeout, target);
    }
    if (!tmp && ((queue->base.listlen - queue->base.numoutstanding) > 0)) {
        tmp = ioqueue_set_starttime_in_subqueue(&queue->base, target);
    }
    /*
fprintf (outputfile, "Exiting ioqueue_set_starttime: %x\n", tmp);
     */
    return(tmp);
}


ioreq_event * ioqueue_show_next_request (ioqueue *queue)
{
    ioreq_event *tmp = NULL;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_show_next_request\n", simtime );
    fflush(outputfile);
#endif

    if ((queue->priority.listlen - queue->priority.numoutstanding) > 0) {
        tmp = ioqueue_show_next_request_from_subqueue(&queue->priority);
    }
    if (!tmp && ((queue->timeout.listlen - queue->timeout.numoutstanding) > 0)) {
        tmp = ioqueue_show_next_request_from_subqueue(&queue->timeout);
    }
    if (!tmp && ((queue->base.listlen - queue->base.numoutstanding) > 0)) {
        tmp = ioqueue_show_next_request_from_subqueue(&queue->base);
    }
    /*
fprintf (outputfile, "Exiting ioqueue_show_next_request: %d\n", ((tmp) ? tmp->blkno : -1));
     */
    return(tmp);
}


ioreq_event * ioqueue_get_next_request (ioqueue *queue)
{
   ioreq_event *tmp = NULL;

#ifdef DEBUG_IOQUEUE
   fprintf (outputfile, "*** %f: Entering ioqueue_get_next_request\n", simtime );
#endif

   if ((queue->priority.listlen - queue->priority.numoutstanding) > 0) {
      if ((tmp = ioqueue_get_next_request_from_subqueue(&queue->priority))) {
         queue->lastsubqueue = IOQUEUE_PRIORITY;
      }
   }
   if (!tmp && ((queue->timeout.listlen - queue->timeout.numoutstanding) > 0)) {
      if ((tmp = ioqueue_get_next_request_from_subqueue(&queue->timeout))) {
         queue->lastsubqueue = IOQUEUE_TIMEOUT;
      }
   }
   if (!tmp && ((queue->base.listlen - queue->base.numoutstanding) > 0)) {
      if ((tmp = ioqueue_get_next_request_from_subqueue(&queue->base))) {
         queue->lastsubqueue = IOQUEUE_BASE;
      }
   }
   if (tmp) {
      int numout = queue->base.numoutstanding + queue->timeout.numoutstanding + queue->priority.numoutstanding;
      if (numout > queue->maxoutstanding) {
         queue->maxoutstanding = numout;
      }
      tmp->time = 0.0;
   }
/*
fprintf (outputfile, "Exiting ioqueue_get_next_request: %d\n", ((tmp) ? tmp->blkno : -1));
*/
   return(tmp);
}


double ioqueue_add_new_request (ioqueue *queue, ioreq_event *new_event)
{
    iobuf *tmp;
    int listlen;
    int qlen;
    int readlen;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: Entering ioqueue_add_new_request: type %d, devno %d, blkno %d, bcount %d, flags 0x%x\n", simtime, new_event->type, new_event->devno, new_event->blkno, new_event->bcount, new_event->flags );
#endif

    new_event->time = simtime;
    ioqueue_update_arrival_stats(queue, new_event);
    tmp = (iobuf *) getfromextraq();
    StaticAssert (sizeof(iobuf) <= sizeof(event));
    tmp->starttime = -1.0;
    tmp->state = WAITING;
    tmp->next = NULL;
    tmp->prev = NULL;
    tmp->tagID = new_event->tagID;
    tmp->totalsize = new_event->bcount;
    tmp->devno = new_event->devno;
    tmp->blkno = new_event->blkno;
    tmp->flags = new_event->flags;
    tmp->iolist = new_event;
    new_event->next = NULL;
    tmp->iob_un.pend.waittime = 0;
    tmp->iob_un.pend.concat = NULL;
    tmp->reqcnt = 1;
    tmp->opid = new_event->opid;
    tmp->batchno = new_event->batchno;
    tmp->batch_list = NULL;
    if (tmp->batchno == -1) {
        tmp->batch_complete = TRUE;
    } else {
        tmp->batch_complete = new_event->batch_complete;
    }
    switch(queue->pri_scheme) {
    case ALLEQUAL:
        if ((queue->base.sched_alg == ELEVATOR_LBN) ||
                (queue->base.sched_alg == CYCLE_LBN)    ||
                (queue->base.sched_alg == SSTF_LBN)     ||
                (queue->base.sched_alg == VSCAN_LBN)) {
            ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, MAP_NONE);
        } else {
            ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, -1);
        }
        ioqueue_insert_new_request(&queue->base, tmp);
        break;
    case TWOQUEUE:
        if (new_event->flags & (TIME_CRITICAL|TIME_LIMITED)) {
            if ((queue->priority.sched_alg == ELEVATOR_LBN) || 
                    (queue->priority.sched_alg == CYCLE_LBN)    ||
                    (queue->priority.sched_alg == SSTF_LBN)     ||
                    (queue->priority.sched_alg == VSCAN_LBN)) {
                ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, MAP_NONE);
            } else {
                ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, -1);
            }
            ioqueue_insert_new_request(&queue->priority, tmp);
        } else {
            if ((queue->base.sched_alg == ELEVATOR_LBN) || 
                    (queue->base.sched_alg == CYCLE_LBN)    ||
                    (queue->base.sched_alg == SSTF_LBN)     ||
                    (queue->base.sched_alg == VSCAN_LBN)) {
                ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, MAP_NONE);
            } else {
                ioqueue_get_cylinder_mapping(queue, tmp, tmp->blkno, &tmp->cylinder, &tmp->surface, -1);
            }
            ioqueue_insert_new_request(&queue->base, tmp);
        }
        break;
    default:
        fprintf(stderr, "Unknown priority scheme being employed at ioqueue_add_new_request\n");
        exit(1);
    }
    listlen = queue->base.listlen + queue->timeout.listlen + queue->priority.listlen;
    qlen = listlen - (queue->base.numoutstanding + queue->timeout.numoutstanding + queue->priority.numoutstanding);
    readlen = queue->base.readlen + queue->timeout.readlen + queue->priority.readlen;
    queue->maxlistlen = max(listlen, queue->maxlistlen);
    queue->maxqlen = max(qlen, queue->maxqlen);
    queue->maxreadlen = max(readlen, queue->maxreadlen);
    queue->maxwritelen = max((listlen - readlen), queue->maxwritelen);
    if (listlen == 1) {
        stat_update(&queue->idlestats, (simtime - queue->idlestart));
    }
    queue->idlestart = simtime;
    if (queue->idledetect) {
        if (!(removefromintq((event *)queue->idledetect))) {
            fprintf(stderr, "existing idledetect event not on intq in ioqueue_add_new_request\n");
            exit(1);
        }
        addtoextraq((event *)queue->idledetect);
        queue->idledetect = NULL;
    }
    /*
fprintf (outputfile, "Exiting ioqueue_add_new_request\n");
     */
    return(0.0);
}


static int ioqueue_raise_priority_subqueue (subqueue *queue, subqueue *priority, int opid, int pri_scheme)
{
    int stop = FALSE;
    iobuf *tmp;
    ioreq_event *trv;
    iobuf *bump;
    int found = 0;
    int iocnt;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_raise_priority_subqueue  opid %d (queue %p, priority %p, pri_scheme %d)\n", simtime, opid, queue, priority, pri_scheme );
    fflush(outputfile);
#endif

    tmp = queue->list;
    if (tmp == NULL) {
        return (FALSE);
    }
    while (stop != TRUE) {
        bump = NULL;
        if ((tmp->iolist == NULL) || (tmp->totalsize == tmp->iolist->bcount)) {
            if (tmp->opid == opid) {
                bump = tmp;
            }
        } else {
            trv = tmp->iolist;
            while (trv != NULL) {
                if (trv->opid == opid) {
                    bump = tmp;
                }
                trv = trv->next;
            }
        }
        tmp = tmp->next;
        if (tmp == queue->list) {
            stop = TRUE;
        }
        if (bump) {
            /*
fprintf (outputfile, "Raising priority of opid %d, buf %x, bump->state %d, read %d, crit %d, blkno %x\n", opid, bump->opid, bump->state, (bump->flags & READ), (bump->flags & TIME_CRITICAL), bump->blkno);
             */
            if ((bump->state == WAITING) && (pri_scheme != ALLEQUAL)) {
                ioqueue_update_subqueue_statistics(queue);
                ioqueue_remove_from_subqueue(queue, bump);
                ioqueue_insert_new_request(priority, bump);
                found = 1;
            }
            found = (bump->state == WAITING) ? 1 : 2;
            bump->flags |= TIME_CRITICAL;
            trv = bump->iolist;
            while (trv != NULL) {
                if (trv->opid == opid) {
                    trv->flags |= TIME_CRITICAL;
                }
                trv = trv->next;
            }
            if ((bump->state == WAITING) && (pri_scheme != ALLEQUAL)) {
                iocnt = bump->reqcnt;
                priority->switches += iocnt;
                queue->iobufcnt--;
                queue->listlen -= iocnt;
                if (bump->flags & READ) {
                    queue->readlen -= iocnt;
                    queue->numreads -= iocnt;
                } else {
                    queue->numwrites -= iocnt;
                }
            }
            break;
        }
    }
    /*
fprintf (outputfile, "Returning found %d\n", found);
     */
    return(found);
}


int ioqueue_raise_priority (ioqueue *queue, int opid)
{
    int found = 0;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_raise_priority\n", simtime );
    fflush(outputfile);
#endif

    found = ioqueue_raise_priority_subqueue(&queue->base, &queue->priority, opid, queue->pri_scheme);
    if (found) {
        return(found);
    }
    found = ioqueue_raise_priority_subqueue(&queue->timeout, &queue->priority, opid, queue->pri_scheme);
    return(found);
}


static int ioqueue_implement_timeouts (subqueue *queue, subqueue *timeout, int maxtime, u_int flag)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_implement_timeouts\n", simtime );
    fflush(outputfile);
#endif

    iobuf *tmp;
    iobuf *done;
    ioreq_event *trv;
    int timeouts = 0;
    int iocnt;

    if (queue->list == NULL) {
        return (0);
    }
    tmp = queue->list->next;
    while (tmp != queue->list) {
        tmp->iob_un.pend.waittime++;
        /*
fprintf (outputfile, "New waittime for opid %d is %d, maxtime %d\n", tmp->opid, tmp->iob_un.pend.waittime, maxtime);
         */
        if ((tmp->iob_un.pend.waittime > maxtime) && (tmp->state == WAITING)) {
            /*
fprintf (outputfile, "Inside one of the request moving sections 1 - blkno %d, state %d\n", tmp->blkno, tmp->state);
             */
            done = tmp;
            tmp = tmp->next;
            ioqueue_update_subqueue_statistics(queue);
            ioqueue_remove_from_subqueue(queue, done);
            ioqueue_insert_new_request(timeout, done);
            done->flags |= flag;
            trv = done->iolist;
            while (trv != NULL) {
                trv->flags |= flag;
                trv = trv->next;
            }
            iocnt = done->reqcnt;
            queue->iobufcnt--;
            queue->listlen -= iocnt;
            if (done->flags & READ) {
                queue->numreads -= iocnt;
                queue->readlen -= iocnt;
            } else {
                queue->numwrites -= iocnt;
            }
            timeouts += iocnt;
            timeout->switches += iocnt;
            if (queue->listlen == 0) {
                tmp = NULL;
            }
        } else {
            tmp = tmp->next;
        }
    }
    if (tmp != NULL) {
        tmp->iob_un.pend.waittime++;
        /*
fprintf (outputfile, "New waittime for opid %d is %d, maxtime %d\n", tmp->opid, tmp->iob_un.pend.waittime, maxtime);
         */
        if ((tmp->iob_un.pend.waittime > maxtime) && (tmp->state == WAITING)) {
            /*
fprintf (outputfile, "Inside one of the request moving sections 2 - blkno %d, state %d\n", tmp->blkno, tmp->state);
             */
            done = tmp;
            ioqueue_update_subqueue_statistics(queue);
            ioqueue_remove_from_subqueue(queue, done);
            ioqueue_insert_new_request(timeout, done);
            done->flags |= flag;
            trv = done->iolist;
            while (trv != NULL) {
                trv->flags |= flag;
                trv = trv->next;
            }
            iocnt = done->reqcnt;
            queue->iobufcnt--;
            queue->listlen -= iocnt;
            if (done->flags & READ) {
                queue->numreads -= iocnt;
                queue->readlen -= iocnt;
            } else {
                queue->numwrites -= iocnt;
            }
            timeouts += iocnt;
            timeout->switches += iocnt;
        }
    }
    return(timeouts);
}


double ioqueue_tick (ioqueue *queue)
{
    int timeouts;
    int readcnt;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_tick\n", simtime );
    fflush(outputfile);
#endif

    if (queue->to_scheme == BASETIMEOUT) {
        if (queue->pri_scheme == ALLEQUAL) {

#ifdef DEBUG_IOQUEUE
            fprintf (outputfile, "*** %f: ioqueue_tick:: base queue to timeout queue\n", simtime );
            fflush(outputfile);
#endif

            readcnt = queue->timeout.numreads;
            timeouts = ioqueue_implement_timeouts(&queue->base, &queue->timeout, queue->to_time, TIMED_OUT);
            queue->timeouts += timeouts;
            queue->timeoutreads += queue->timeout.numreads - readcnt;
        } else {
#ifdef DEBUG_IOQUEUE
            fprintf (outputfile, "*** %f: ioqueue_tick:: base queue to priority queue\n", simtime );
            fflush(outputfile);
#endif
            readcnt = queue->priority.numreads;
            timeouts = ioqueue_implement_timeouts(&queue->base, &queue->priority, queue->to_time, TIMED_OUT);
            queue->timeouts += timeouts;
            queue->timeoutreads += queue->priority.numreads - readcnt;
        }
    } else if (queue->to_scheme == HALFTIMEOUT) {
#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_tick:: timeout queue to priority queue\n", simtime );
        fflush(outputfile);
#endif
        readcnt = queue->priority.numreads;
        timeouts = ioqueue_implement_timeouts(&queue->timeout, &queue->priority, queue->to_time, TIMED_OUT);
        queue->timeouts += timeouts;
        queue->timeoutreads += queue->priority.numreads - readcnt;

#ifdef DEBUG_IOQUEUE
        fprintf (outputfile, "*** %f: ioqueue_tick:: base queue to timeout queue\n", simtime );
        fflush(outputfile);
#endif
        readcnt = queue->timeout.numreads;
        timeouts = ioqueue_implement_timeouts(&queue->base, &queue->timeout, (queue->to_time / 2), HALF_OUT);
        queue->halfouts += timeouts;
        queue->halfoutreads += queue->timeout.numreads - readcnt;
    }
    return(0.0);
}


static void ioqueue_clobber_overlaps_subqueue (subqueue *queue, ioreq_event *ret, double arrtimemax)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_clobber_overlaps_subqueue\n", simtime );
    fflush(outputfile);
#endif

    iobuf *tmp;
   ioreq_event *done;
   int read = ret->flags & READ;
   while ((tmp = queue->list)) {
      if ((tmp->state != WAITING) || (tmp->blkno != ret->blkno) || (tmp->totalsize != ret->bcount) || ((read) && !(tmp->flags & READ)) || (tmp->iolist->time > arrtimemax)) {
         tmp = tmp->next;
         while ((tmp != queue->list) && ((tmp->state != WAITING) || (tmp->blkno != ret->blkno) || (tmp->totalsize != ret->bcount) || ((read) && !(tmp->flags & READ)) || (tmp->iolist->time > arrtimemax))) {
            tmp = tmp->next;
         }
         if (tmp == queue->list) {
            return;
         }
      }
      ASSERT(tmp->reqcnt == 1);
      done = ioqueue_get_specific_request_from_subqueue(queue, tmp->iolist);
      ASSERT(done != NULL);
      done = ioqueue_remove_completed_request(queue, done);
      ASSERT(done->next == NULL);
      done->next = ret->next;
      ret->next = done;
      queue->bigqueue->overlapscombed++;
      if (done->flags & READ) {
         queue->bigqueue->overlapscombed++;
      }
   }
}


ioreq_event * ioqueue_physical_access_done (ioqueue *queue, ioreq_event *curr)
{
    ioreq_event *ret;

#ifdef DEBUG_IOQUEUE
    fprintf(outputfile, "Entering ioqueue_physical_access_done: devno %d, blkno %d, bcount %d, flags %x\n", curr->devno, curr->blkno, curr->bcount, curr->flags);
    ioqueue_print_contents(queue);
#endif
    queue->idlestart = simtime;
    if ((queue->pri_scheme != ALLEQUAL) && ((curr->flags & (TIME_CRITICAL|TIME_LIMITED)) || ((queue->to_scheme != NOTIMEOUT) && (curr->flags & TIMED_OUT)))) {
#ifdef DEBUG_IOQUEUE
        fprintf(outputfile, "ioqueue_physical_access_done: Checking the priority queue\n");
#endif
        ret = ioqueue_remove_completed_request(&queue->priority, curr);
    } else if ((queue->to_scheme != NOTIMEOUT) && (curr->flags & (TIMED_OUT|HALF_OUT))) {
#ifdef DEBUG_IOQUEUE
        fprintf(outputfile, "ioqueue_physical_access_done: Checking the timeout queue\n");
#endif
        ret = ioqueue_remove_completed_request(&queue->timeout, curr);
    } else {
#ifdef DEBUG_IOQUEUE
        fprintf(outputfile, "ioqueue_physical_access_done: Checking the base queue\n");
#endif
        ret = ioqueue_remove_completed_request(&queue->base, curr);
    }
    ASSERT(ret != NULL);
    if (queue->comboverlaps) {
        double arrtimemax = (queue->comboverlaps == 1) ? simtime : ret->time;
        ioqueue_clobber_overlaps_subqueue(&queue->priority, ret, arrtimemax);
        ioqueue_clobber_overlaps_subqueue(&queue->timeout, ret, arrtimemax);
        ioqueue_clobber_overlaps_subqueue(&queue->base, ret, arrtimemax);
    }
    if (queue->idlework) {
        ioqueue_reset_idledetecter(queue, 1);
    }

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "Exiting ioqueue_physical_access_done: %f\n", disksim->lastphystime);
#endif

    return(ret);
}


static void ioqueue_subqueue_copy (subqueue *queue, subqueue *new_subqueue)
{
   new_subqueue->sched_alg = queue->sched_alg;
   new_subqueue->surfoverforw = queue->surfoverforw;
   new_subqueue->force_absolute_fcfs = queue->force_absolute_fcfs;
   new_subqueue->vscan_value = queue->vscan_value;
   new_subqueue->list = NULL;
   new_subqueue->current = NULL;
}


ioqueue * ioqueue_copy (ioqueue *queue)
{
   ioqueue *new_ioqueue = (ioqueue *) malloc(sizeof(ioqueue));
   memcpy(new_ioqueue, queue, sizeof(ioqueue));

   ioqueue_subqueue_copy(&queue->base, &new_ioqueue->base);
   ioqueue_subqueue_copy(&queue->timeout, &new_ioqueue->timeout);
   ioqueue_subqueue_copy(&queue->priority, &new_ioqueue->priority);

   return new_ioqueue;
}


static void ioqueue_subqueue_resetstats (subqueue *queue)
{
   queue->numreads = 0;
   queue->numwrites = 0;
   queue->switches = 0;
   queue->runreadlen = 0.0;
   queue->maxreadlen = 0;
   queue->maxwritelen = 0;
   queue->maxlistlen = 0;
   queue->runlistlen = 0.0;
   queue->maxqlen = 0;
   queue->numcomplete = 0;
   queue->maxoutstanding = 0;
   queue->runoutstanding = 0.0;
   queue->sstfupdowncnt = 0;
   queue->lastalt = simtime;

   queue->num_sptf_sdf_different = 0;

   stat_reset(&queue->accstats);
   stat_reset(&queue->qtimestats);
   stat_reset(&queue->outtimestats);
   stat_reset(&queue->critreadstats);
   stat_reset(&queue->nocritreadstats);
   stat_reset(&queue->critwritestats);
   stat_reset(&queue->nocritwritestats);
   stat_reset(&queue->instqueuelen);
   stat_reset(&queue->infopenalty);
}


static void ioqueue_subqueue_initialize (subqueue *queue, int devno)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_subqueue_initialize  subqueue %p, devno %d\n", simtime, queue, devno );
    fflush(outputfile);
#endif

   addlisttoextraq((event **)&queue->list);
   queue->enablement = NULL;
   queue->dir = ASC;
   queue->lastblkno = 0;
   queue->lastsurface = 0;
   queue->lastcylno = 0;
   queue->prior = 0;
   queue->sstfupdown = 0;
   queue->current = NULL;
   queue->readlen = 0;
   queue->iobufcnt = 0;
   queue->reqoutstanding = 0;
   queue->listlen = 0;
   queue->numoutstanding = 0;
   queue->vscan_value = 0.2;

   queue->num_sptf_sdf_different = 0;

   stat_initialize(statdeffile, statdesc_accstats, &queue->accstats);
   stat_initialize(statdeffile, statdesc_qtimestats, &queue->qtimestats);
   stat_initialize(statdeffile, statdesc_outtimestats, &queue->outtimestats);
   stat_initialize(statdeffile, statdesc_outtimestats, &queue->critreadstats);
   stat_initialize(statdeffile, statdesc_outtimestats, &queue->nocritreadstats);
   stat_initialize(statdeffile, statdesc_outtimestats, &queue->critwritestats);
   stat_initialize(statdeffile, statdesc_outtimestats, &queue->nocritwritestats);
   stat_initialize(statdeffile, statdesc_instqueuelen, &queue->instqueuelen);
   stat_initialize(statdeffile, statdesc_infopenalty, &queue->infopenalty);

   if ((queue->sched_alg == VSCAN_LBN) || (queue->sched_alg == PRI_VSCAN_LBN)) {
/*        queue->vscan_cyls = queue->vscan_value * (double) device_get_number_of_blocks(devno); */
   } else {
/*        queue->vscan_cyls = queue->vscan_value * (double) device_get_numcyls(devno); */
   }
}

void ioqueue_resetstats (ioqueue *queue)
{
   queue->timeouts = 0;
   queue->timeoutreads = 0;
   queue->halfouts = 0;
   queue->halfoutreads = 0;
   queue->idlestart = simtime;
   queue->maxlistlen = 0;
   queue->maxqlen = 0;
   queue->maxoutstanding = 0;
   queue->maxreadlen = 0;
   queue->maxwritelen = 0;
   queue->overlapscombed = 0;
   queue->readoverlapscombed = 0;
   queue->seqreads = 0;
   queue->seqwrites = 0;
   stat_reset(&queue->intarrstats);
   stat_reset(&queue->readintarrstats);
   stat_reset(&queue->writeintarrstats);
   stat_reset(&queue->idlestats);
   stat_reset(&queue->reqsizestats);
   stat_reset(&queue->readsizestats);
   stat_reset(&queue->writesizestats);
   stat_reset(&queue->batchsizestats);
   ioqueue_subqueue_resetstats(&queue->base);
   ioqueue_subqueue_resetstats(&queue->timeout);
   ioqueue_subqueue_resetstats(&queue->priority);
}


void ioqueue_setcallbacks ()
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_setcallbacks\n", simtime );
    fflush(outputfile);
#endif

    disksim->timerfunc_ioqueue = ioqueue_idledetected;
}


void ioqueue_initialize (ioqueue *queue, int devno)
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_initialize:: ioqueue %p, devno %d\n", simtime, queue, devno );
    fflush(outputfile);
#endif

   StaticAssert (sizeof(iobuf) <= DISKSIM_EVENT_SIZE);
   ioqueue_setcallbacks();
   ioqueue_subqueue_initialize(&queue->base, devno);
   queue->base.bigqueue = queue;
   ioqueue_subqueue_initialize(&queue->timeout, devno);
   queue->timeout.bigqueue = queue;
   ioqueue_subqueue_initialize(&queue->priority, devno);
   queue->priority.bigqueue = queue;
   queue->devno = devno;
   queue->concatok = NULL;
   queue->idlework = NULL;
   queue->idledelay = 0.0;
   queue->idledetect = NULL;
/*     queue->sectpercyl = device_get_avg_sectpercyl(devno); */
   queue->lastsubqueue = IOQUEUE_BASE;
   queue->lastarr = 0.0;
   queue->lastread = 0.0;
   queue->lastwrite = 0.0;
   queue->seqblkno = -1;
   stat_initialize(statdeffile, statdesc_intarrstats, &queue->intarrstats);
   stat_initialize(statdeffile, statdesc_readintarrstats, &queue->readintarrstats);
   stat_initialize(statdeffile, statdesc_writeintarrstats, &queue->writeintarrstats);
   stat_initialize(statdeffile, statdesc_idlestats, &queue->idlestats);
   stat_initialize(statdeffile, statdesc_reqsizestats, &queue->reqsizestats);
   stat_initialize(statdeffile, statdesc_readsizestats, &queue->readsizestats);
   stat_initialize(statdeffile, statdesc_writesizestats, &queue->writesizestats);
   stat_initialize(statdeffile, statdesc_batchsizestats, &queue->batchsizestats);
}


void ioqueue_cleanstats (ioqueue *queue)
{
   double tpass;

   ioqueue_update_subqueue_statistics(&queue->base);
   ioqueue_update_subqueue_statistics(&queue->timeout);
   ioqueue_update_subqueue_statistics(&queue->priority);
   if ((queue->base.listlen + queue->timeout.listlen + queue->priority.listlen) == 0) {
      tpass = simtime - queue->idlestart;
      stat_update(&queue->idlestats, tpass);
   }
   queue->idlestart = simtime;
}


ioqueue * ioqueue_createdefaultqueue ()
{
#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_createdefaultqueue\n", simtime );
    fflush(outputfile);
#endif

   ioqueue * queue = (ioqueue *) calloc(1, sizeof(ioqueue));

   queue->printqueuestats  = 1;
   queue->printcritstats   = 1;
   queue->printidlestats   = 1;
   queue->printintarrstats = 1;
   queue->printsizestats   = 1;
   queue->priority_mix     = 1;

/*     queue->base.list        = NULL; */
/*     queue->timeout.list     = NULL; */
/*     queue->priority.list    = NULL; */
/*     queue->base.surfoverforw     = FALSE; */
/*     queue->timeout.surfoverforw  = FALSE; */
/*     queue->priority.surfoverforw = FALSE; */
/*     queue->base.force_absolute_fcfs     = FALSE; */
/*     queue->timeout.force_absolute_fcfs  = FALSE; */
/*     queue->priority.force_absolute_fcfs = FALSE; */

   queue->base.sched_alg = FCFS;
   queue->cylmaptype     = MAP_NONE;
/*     queue->writedelay     = 0.0; */
/*     queue->readdelay      = 0.0; */
/*     queue->seqscheme      = 0; */
/*     queue->concatmax      = 0; */
/*     queue->comboverlaps   = 0; */
/*     queue->seqstreamdiff  = 0; */
/*     queue->to_scheme      = 0; */
/*     queue->to_time        = 0; */
   queue->timeout.sched_alg  = FCFS;
/*     queue->pri_scheme     = 0; */
   queue->priority.sched_alg = FCFS;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: ioqueue_createdefaultqueue ioqueue dump: %p\n", simtime, queue );
    ioqueue_print_contents( queue );
#endif

   return(queue);
}




/* ioqueue contructor using new parser
 * returns an allocated and initialized ioqueue on success 
 * or NULL on failure */
struct ioq *disksim_ioqueue_loadparams(
        struct lp_block *b,
        int printqueuestats,
        int printcritstats,
        int printidlestats,
        int printintarrstats,
        int printsizestats)
{
    struct ioq *result;

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: disksim_ioqueue_loadparams\n", simtime );
    fflush(outputfile);
#endif

    result = (struct ioq *)calloc(1, sizeof(ioqueue));

    result->printqueuestats = printqueuestats;
    result->printcritstats = printcritstats;
    result->printidlestats = printidlestats;
    result->printintarrstats = printintarrstats;
    result->printsizestats = printsizestats;

    result->priority_mix = 1;

    if(b->name) result->name = strdup(b->name);

    /*    unparse_block(b, outputfile); */


    //#include "modules/disksim_ioqueue_param.c"
    lp_loadparams(result, b, &disksim_ioqueue_mod);

#ifdef DEBUG_IOQUEUE
    fprintf (outputfile, "*** %f: disksim_ioqueue_loadparams dump ioqueue: %p\n", simtime, result );
    ioqueue_print_contents( result );
#endif

    return result;
}

static void ioqueue_printqueuestats (ioqueue **set, int setsize, char *prefix)
{
   int maxlistlen = 0.0;
   int maxqlen = 0.0;
   int maxreadlen = 0.0;
   int maxwritelen = 0.0;
   double runlistlen = 0.0;
   double runoutstanding = 0.0;
   double runreadlen = 0.0;
   double runwritelen = 0.0;
   int listlen = 0;
   int numoutstanding = 0;
   int numcomplete = 0;
   int i;

   statgen ** statset;
   statset = (statgen **)  DISKSIM_malloc (3*setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

   for (i=0; i<setsize; i++) {
      if (set[i]->printqueuestats == FALSE)
         return;

      if (set[i]->maxlistlen > maxlistlen)
	 maxlistlen = set[i]->maxlistlen;
      if (set[i]->maxqlen > maxqlen)
	 maxqlen = set[i]->maxqlen;
      if (set[i]->maxreadlen > maxreadlen)
	 maxreadlen = set[i]->maxreadlen;
      if (set[i]->maxwritelen > maxwritelen)
	 maxwritelen = set[i]->maxwritelen;
      numcomplete += set[i]->base.numcomplete + set[i]->timeout.numcomplete + set[i]->priority.numcomplete;
      runlistlen += set[i]->base.runlistlen + set[i]->timeout.runlistlen + set[i]->priority.runlistlen;
      runoutstanding += set[i]->base.runoutstanding + set[i]->timeout.runoutstanding + set[i]->priority.runoutstanding;
      listlen += set[i]->base.listlen + set[i]->timeout.listlen + set[i]->priority.listlen;
      numoutstanding += set[i]->base.numoutstanding + set[i]->timeout.numoutstanding + set[i]->priority.numoutstanding;
      runreadlen += set[i]->base.runreadlen + set[i]->timeout.runreadlen + set[i]->priority.runreadlen;
   }
   runwritelen = runlistlen - runreadlen;

   fprintf(outputfile, "%srunlistlen:              %f\n", prefix, runlistlen);
   fprintf(outputfile, "%srunoutstanding:          %f\n", prefix, runoutstanding);
   fprintf(outputfile, "%ssimtime:                 %f\n", prefix, simtime);
   fprintf(outputfile, "%swarmuptime:              %f\n", prefix, WARMUPTIME);
   fprintf(outputfile, "%ssetsize:                 %d\n", prefix, setsize);

   fprintf(outputfile, "%sAverage # requests:      %f\n", prefix, (runlistlen/((simtime - WARMUPTIME) * (double) setsize)));
   fprintf(outputfile, "%sMaximum # requests:      %d\n", prefix, maxlistlen);
   fprintf(outputfile, "%sEnd # requests:          %d\n", prefix, listlen);
   fprintf(outputfile, "%sAverage queue length:    %f\n", prefix, ((runlistlen - runoutstanding)/((simtime - WARMUPTIME) * (double) setsize)));
   fprintf(outputfile, "%sMaximum queue length:    %d\n", prefix, maxqlen);
   fprintf(outputfile, "%sEnd queued requests:     %d\n", prefix, (listlen - numoutstanding));

   for (i=0; i<setsize; i++) {
      statset[(3*i)] = &set[i]->base.qtimestats;
      statset[((3*i)+1)] = &set[i]->priority.qtimestats;
      statset[((3*i)+2)] = &set[i]->timeout.qtimestats;
   }
   stat_print_set(statset, (3*setsize), prefix);

   fprintf (outputfile, "%sAvg # read requests:     %f\n", prefix, (runreadlen / ((simtime - WARMUPTIME) * (double) setsize)));
   fprintf (outputfile, "%sMax # read requests:     %d\n", prefix, maxreadlen);
   fprintf (outputfile, "%sAvg # write requests:    %f\n", prefix, (runwritelen / ((simtime - WARMUPTIME) * (double) setsize)));
   fprintf (outputfile, "%sMax # write requests:    %d\n", prefix, maxwritelen);

   for (i=0; i<setsize; i++) {
      statset[(3*i)] = &set[i]->base.accstats;
      statset[((3*i)+1)] = &set[i]->priority.accstats;
      statset[((3*i)+2)] = &set[i]->timeout.accstats;
   }
   stat_print_set(statset, (3*setsize), prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->batchsizestats;
   }
   fprintf (outputfile, "%sNumber of batches:  %d\n", prefix, stat_get_count_set(statset, setsize));
   stat_print_set(statset, setsize, prefix);
   free(statset);
   statset = NULL;
}


static void ioqueue_printintarrstats (ioqueue **set, int setsize, char *prefix)
{
   int i;
   statgen ** statset;

   statset = (statgen **)  DISKSIM_malloc (setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->intarrstats;
      if (set[i]->printidlestats == FALSE) {
	 return;
      }
   }
   stat_print_set(statset, setsize, prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->readintarrstats;
   }
   stat_print_set(statset, setsize, prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->writeintarrstats;
   }
   stat_print_set(statset, setsize, prefix);
   free(statset);
   statset = NULL;
}


static void ioqueue_printsizestats (ioqueue **set, int setsize, char *prefix)
{
   int i;
   statgen ** statset;

   statset = (statgen **)  DISKSIM_malloc (setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->reqsizestats;
      if (set[i]->printsizestats == FALSE) {
         return;
      }
   }
   stat_print_set(statset, setsize, prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->readsizestats;
   }
   stat_print_set(statset, setsize, prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->writesizestats;
   }
   stat_print_set(statset, setsize, prefix);
   free(statset);
   statset = NULL;
}


static void ioqueue_printidlestats (ioqueue **set, int setsize, char *prefix)
{
   int i;
   statgen ** statset;

   statset = (statgen **)  DISKSIM_malloc (setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->idlestats;
      if (set[i]->printidlestats == FALSE) {
         return;
      }
   }
   fprintf (outputfile, "%sNumber of idle periods:  %d\n", prefix, stat_get_count_set(statset, setsize));
   stat_print_set(statset, setsize, prefix);
   free(statset);
   statset = NULL;
}


static void ioqueue_printcritstats (subqueue **set, int setsize, char *prefix, int numreqs)
{
   char prefix2[256];
   int cnt;
   int i;
   statgen ** statset;

   statset = (statgen **)  DISKSIM_malloc (setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

	/* check for printability is external */

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->critreadstats;
   }
   cnt = stat_get_count_set(statset, setsize);
   fprintf (outputfile, "%sCritical Reads:      \t%6d  \t%f\n", prefix, cnt, ((double) cnt / (double) max(numreqs,1)));
   sprintf(prefix2, "%sCritical Read ", prefix);
   stat_print_set(statset, setsize, prefix2);
   
   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->nocritreadstats;
   }
   cnt = stat_get_count_set(statset, setsize);
   fprintf (outputfile, "%sNon-Critical Reads:  \t%6d  \t%f\n", prefix, cnt, ((double) cnt / (double) max(numreqs,1)));
   sprintf(prefix2, "%sNon-Critical Read ", prefix);
   stat_print_set(statset, setsize, prefix2);
   
   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->critwritestats;
   }
   cnt = stat_get_count_set(statset, setsize);
   fprintf (outputfile, "%sCritical Writes:     \t%6d  \t%f\n", prefix, cnt, ((double) cnt / (double) max(numreqs,1)));
   sprintf(prefix2, "%sCritical Write ", prefix);
   stat_print_set(statset, setsize, prefix2);
   
   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->nocritwritestats;
   }
   cnt = stat_get_count_set(statset, setsize);
   fprintf (outputfile, "%sNon-Critical Writes: \t%6d  \t%f\n", prefix, cnt, ((double) cnt / (double) max(numreqs,1)));
   sprintf(prefix2, "%sNon-Critical Write ", prefix);
   stat_print_set(statset, setsize, prefix2);
   free(statset);
   statset = NULL;
}


static void ioqueue_subqueue_printstats (subqueue **set, int setsize, char *prefix, int printcritstats)
{
   int numcomplete = 0;
   int maxlistlen = 0;
   int listlen = 0;
   int maxqlen = 0;
   int numoutstanding = 0;
   int numreqs = 0;
   double runlistlen = 0.0;
   double runoutstanding = 0.0;
   int i;
   statgen ** statset;

   statset = (statgen **)  DISKSIM_malloc (setsize*sizeof(statgen *));
   ASSERT (statset != NULL);

   for (i=0; i<setsize; i++) {
      numreqs += set[i]->numreads + set[i]->numwrites;
      numcomplete += set[i]->numcomplete;
      runlistlen += set[i]->runlistlen;
      runoutstanding += set[i]->runoutstanding;
      listlen += set[i]->listlen;
      numoutstanding += set[i]->numoutstanding;
      if (set[i]->maxqlen > maxqlen) {
	 maxqlen = set[i]->maxqlen;
      }
      if (set[i]->maxlistlen > maxlistlen) {
	 maxlistlen = set[i]->maxlistlen;
      }
   }

   if (maxlistlen == 0) {
      return;
   }

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->outtimestats;
   }
   stat_print_set(statset, setsize, prefix);

   if (printcritstats) {
      ioqueue_printcritstats (set, setsize, prefix, numreqs);
   }

   fprintf(outputfile, "%sAverage # requests:      %f\n", prefix, (runlistlen/((simtime - WARMUPTIME) * (double) setsize)));
   fprintf(outputfile, "%sMaximum # requests:      %d\n", prefix, maxlistlen);
   fprintf(outputfile, "%sEnd # requests:          %d\n", prefix, listlen);
   fprintf(outputfile, "%sAverage queue length:    %f\n", prefix, ((runlistlen - runoutstanding)/((simtime - WARMUPTIME) * (double) setsize)));
   fprintf(outputfile, "%sMaximum queue length:    %d\n", prefix, maxqlen);
   fprintf(outputfile, "%sEnd queued requests:     %d\n", prefix, (listlen - numoutstanding));

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->qtimestats;
   }
   stat_print_set(statset, setsize, prefix);

   for (i=0; i<setsize; i++) {
      statset[i] = &set[i]->accstats;
   }
   stat_print_set(statset, setsize, prefix);
   free(statset);
   statset = NULL;
}


void ioqueue_printstats (ioqueue **set, int setsize, char *sourcestr)
{
   int to_scheme = 0;
   int pri_scheme = 0;
   int numcomplete = 0;
   int i;
   int numreads = 0;
   int numwrites = 0;
   int numreqs = 0;
   int seqreads = 0;
   int seqwrites = 0;
   int printcritstats = 1;
   double idletime = 0.0;
   int subcheck = 0;
   int switches = 0;
   int timeouts = 0;
   int halfouts = 0;
   int overlapscombed = 0;
   int readoverlapscombed = 0;

   int base_num_sptf_sdf_different = 0;
   int timeout_num_sptf_sdf_different = 0;
   int priority_num_sptf_sdf_different = 0;

   int base_num_scheduling_decisions = 0;
   int timeout_num_scheduling_decisions = 0;
   int priority_num_scheduling_decisions = 0;

   char prefix[80];
   subqueue ** subset;
   statgen ** statset;

   subset  = (subqueue **) DISKSIM_malloc (3*setsize*sizeof(subqueue *));
   statset = (statgen **)  DISKSIM_malloc (3*setsize*sizeof(statgen *));
   ASSERT ((subset != NULL) && (statset != NULL));

   for (i=0; i<setsize; i++) {

      if (set[i]->printqueuestats == FALSE)
         return;

      if ((set[i]->to_scheme != NOTIMEOUT) || (set[i]->pri_scheme != ALLEQUAL)) {
	 to_scheme = set[i]->to_scheme;
	 pri_scheme = set[i]->pri_scheme;
         subcheck = 1;
      }

      statset[(3*i)] = &set[i]->base.outtimestats;
      statset[((3*i)+1)] = &set[i]->timeout.outtimestats;
      statset[((3*i)+2)] = &set[i]->priority.outtimestats;

      printcritstats &= set[i]->printcritstats;
      numreads += set[i]->base.numreads + set[i]->timeout.numreads + set[i]->priority.numreads;
/*
      numreads -= (set[i]->timeoutreads + set[i]->halfoutreads);
*/
      numwrites += set[i]->base.numwrites + set[i]->timeout.numwrites + set[i]->priority.numwrites;
/*
      numwrites -= (set[i]->timeouts - set[i]->timeoutreads);
      numwrites -= (set[i]->halfouts - set[i]->halfoutreads);
*/
      seqreads += set[i]->seqreads;
      seqwrites += set[i]->seqwrites;
      idletime += stat_get_runval(&set[i]->idlestats);
      overlapscombed += set[i]->overlapscombed;
      readoverlapscombed += set[i]->readoverlapscombed;

      base_num_sptf_sdf_different += set[i]->base.num_sptf_sdf_different;
      base_num_scheduling_decisions += set[i]->base.num_scheduling_decisions;
      timeout_num_sptf_sdf_different += set[i]->timeout.num_sptf_sdf_different;
      timeout_num_scheduling_decisions += set[i]->timeout.num_scheduling_decisions;
      priority_num_sptf_sdf_different += set[i]->priority.num_sptf_sdf_different;
      priority_num_scheduling_decisions += set[i]->priority.num_scheduling_decisions;

   }
   numreqs = (double) (numreads + numwrites);
   idletime /= (double) setsize;

   numcomplete = stat_get_count_set(statset, (3*setsize));

   fprintf(outputfile, "%sTotal Requests handled:\t%d\n", sourcestr, numcomplete);
   fprintf(outputfile, "%sRequests per second:   \t%f\n", sourcestr, ((double)1000 * (double)numcomplete / (simtime - WARMUPTIME)));
   fprintf(outputfile, "%sCompletely idle time:  \t%f   \t%f\n", sourcestr, idletime, (idletime / (simtime - WARMUPTIME)));

   stat_print_set(statset, (3*setsize), sourcestr);

   fprintf(outputfile, "%sOverlaps combined:     \t%d\t%f\n", sourcestr, overlapscombed, ((double) overlapscombed / (double) max(numcomplete,1)));
   fprintf(outputfile, "%sRead overlaps combined:\t%d\t%f\t%f\n", sourcestr, readoverlapscombed, ((double) readoverlapscombed / (double) max(overlapscombed,1)), ((double) readoverlapscombed / (double) max(numreads,1)));

   if (printcritstats) {
      for (i=0; i<setsize; i++) {
         subset[(3*i)] = &set[i]->base;
         subset[((3*i)+1)] = &set[i]->timeout;
         subset[((3*i)+2)] = &set[i]->priority;
      }
      ioqueue_printcritstats (subset, (3*setsize), sourcestr, numreqs);
   }

   fprintf(outputfile, "%sNumber of reads:    %6d  \t%f\n", sourcestr, numreads, ((double) numreads / max(numreqs,1)));
   fprintf(outputfile, "%sNumber of writes:   %6d  \t%f\n", sourcestr, numwrites, ((double) numwrites / max(numreqs,1)));
   fprintf(outputfile, "%sSequential reads:   %6d  \t%f  \t%f\n", sourcestr, seqreads, ((double) seqreads / (double) max(numreads,1)), ((double) seqreads / max(numreqs,1)));
   fprintf(outputfile, "%sSequential writes:  %6d  \t%f  \t%f\n", sourcestr, seqwrites, ((double) seqwrites / (double) max(numwrites,1)), ((double) seqwrites / max(numreqs,1)));

   fprintf(outputfile, "%sBase SPTF/SDF Different: %6d / %6d \t%f\n",
	   sourcestr, base_num_sptf_sdf_different, base_num_scheduling_decisions,
	   ((double) base_num_sptf_sdf_different / (double) max(base_num_scheduling_decisions, 1)));
   fprintf(outputfile, "%sTimeout SPTF/SDF Different: %6d / %6d \t%f\n",
	   sourcestr, timeout_num_sptf_sdf_different, timeout_num_scheduling_decisions,
	   ((double) timeout_num_sptf_sdf_different / (double) max(timeout_num_scheduling_decisions, 1)));
   fprintf(outputfile, "%sPriority SPTF/SDF Different: %6d / %6d \t%f\n",
	   sourcestr, priority_num_sptf_sdf_different, priority_num_scheduling_decisions,
	   ((double) priority_num_sptf_sdf_different / (double) max(priority_num_scheduling_decisions, 1)));

   ioqueue_printqueuestats(set, setsize, sourcestr);
   ioqueue_printintarrstats(set, setsize, sourcestr);
   ioqueue_printidlestats(set, setsize, sourcestr);
   ioqueue_printsizestats(set, setsize, sourcestr);

   for (i=0; i<setsize; i++) {
      statset[(3*i)] = &set[i]->base.instqueuelen;
      statset[((3*i)+1)] = &set[i]->timeout.instqueuelen;
      statset[((3*i)+2)] = &set[i]->priority.instqueuelen;
   }
   stat_print_set(statset, (3*setsize), sourcestr);
   for (i=0; i<setsize; i++) {
      statset[(3*i)] = &set[i]->base.infopenalty;
      statset[((3*i)+1)] = &set[i]->timeout.infopenalty;
      statset[((3*i)+2)] = &set[i]->priority.infopenalty;
   }
   stat_print_set(statset, (3*setsize), sourcestr);
   if (subcheck) {
      fprintf (outputfile, "%sBase queue statistics\n", sourcestr);
      for (i=0; i<setsize; i++) {
	 subset[i] = &set[i]->base;
      }
      sprintf(prefix, "%sbase ", sourcestr);
      ioqueue_subqueue_printstats(subset, setsize, prefix, printcritstats);
      if (to_scheme != NOTIMEOUT) {
         fprintf (outputfile, "%sTimeout queue statistics\n", sourcestr);
	 switches = 0;
         for (i=0; i<setsize; i++) {
	    subset[i] = &set[i]->timeout;
	    switches += set[i]->timeout.switches;
	    timeouts += set[i]->timeouts;
	    halfouts += set[i]->halfouts;
         }
	 sprintf(prefix, "%stimeout ", sourcestr);
	 fprintf (outputfile, "%sRequests switched to timeout queue:   %d\n", sourcestr, switches);
	 fprintf(outputfile, "%sTimed out requests:                   %d\n", sourcestr, timeouts);
	 fprintf(outputfile, "%sHalfway timed out requests:           %d\n", sourcestr, halfouts);
         ioqueue_subqueue_printstats(subset, setsize, prefix, printcritstats);
      }
      if (pri_scheme != ALLEQUAL) {
         fprintf(outputfile, "%sPriority queue statistics\n", sourcestr);
	 switches = 0;
         for (i=0; i<setsize; i++) {
	    subset[i] = &set[i]->priority;
	    switches += set[i]->priority.switches;
         }
	 sprintf(prefix, "%spriority ", sourcestr);
	 fprintf (outputfile, "%sRequests switched to priority queue:   %d\n", sourcestr, switches);
         ioqueue_subqueue_printstats(subset, setsize, prefix, printcritstats);
      }
   }

   free(subset);
   free(statset);
   subset = NULL;
   statset = NULL;
}

// End of file

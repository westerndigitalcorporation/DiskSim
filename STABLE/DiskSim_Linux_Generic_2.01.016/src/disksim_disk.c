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
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_disk.h"
#include "disksim_ioqueue.h"
#include "config.h"

#include "modules/modules.h"

static void disk_initialize_diskinfo ();

/* read-only globals used during readparams phase */
static char *statdesc_seekdiststats	=	"Seek distance";
static char *statdesc_seektimestats	=	"Seek time";
static char *statdesc_rotlatstats	=	"Rotational latency";
static char *statdesc_xfertimestats	=	"Transfer time";
static char *statdesc_postimestats	=	"Positioning time";
static char *statdesc_acctimestats	=	"Access time";


INLINE struct disk *getdisk (int diskno)
{
    disk *currdisk;
    ASSERT1((diskno >= 0) && (diskno < MAXDEVICES), "diskno", diskno);
    currdisk = disksim->diskinfo->disks[diskno];
    if (!currdisk) {
        //fprintf (stderr, "getdisk returning a non-initialized currdisk\n");
    }
    return (currdisk);
}


int disk_add(struct disk *d) {
    int c;

    if(!disksim->diskinfo) disk_initialize_diskinfo();

    for(c = 0; c < disksim->diskinfo->disks_len; c++) {
        if(!disksim->diskinfo->disks[c]) {
            disksim->diskinfo->disks[c] = d;
            NUMDISKS++;
            return c;
        }
    }

    /* note that numdisks must be equal to diskinfo->disks_len */
    disksim->diskinfo->disks = (disk **)realloc(disksim->diskinfo->disks, 2 * NUMDISKS * sizeof(disk *));
    bzero(&(disksim->diskinfo->disks[NUMDISKS]), NUMDISKS*sizeof(disk*));
    disksim->diskinfo->disks[c] = d;
    NUMDISKS++;
    disksim->diskinfo->disks_len *= 2;
    return c;
}


int disk_get_numdisks (void)
{
    return(NUMDISKS);
}


void disk_cleanstats (void)
{
    int i;

    for (i=0; i<MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if (currdisk) {
            ioqueue_cleanstats(currdisk->queue);
        }
    }
}





static void diskstatinit (int diskno, int firsttime)
{
    disk *currdisk = getdisk (diskno);
    diskstat *stat = &currdisk->stat;

    if (firsttime) {
        stat_initialize(statdeffile, statdesc_seekdiststats, &stat->seekdiststats);
        stat_initialize(statdeffile, statdesc_seektimestats, &stat->seektimestats);
        stat_initialize(statdeffile, statdesc_rotlatstats, &stat->rotlatstats);
        stat_initialize(statdeffile, statdesc_xfertimestats, &stat->xfertimestats);
        stat_initialize(statdeffile, statdesc_postimestats, &stat->postimestats);
        stat_initialize(statdeffile, statdesc_acctimestats, &stat->acctimestats);
    }
    else {
        stat_reset(&stat->seekdiststats);
        stat_reset(&stat->seektimestats);
        stat_reset(&stat->rotlatstats);
        stat_reset(&stat->xfertimestats);
        stat_reset(&stat->postimestats);
        stat_reset(&stat->acctimestats);
    }

    stat->highblkno = 0;
    stat->zeroseeks = 0;
    stat->zerolatency = 0;
    stat->writecombs = 0;
    stat->readmisses = 0;
    stat->writemisses = 0;
    stat->fullreadhits = 0;
    stat->appendhits = 0;
    stat->prependhits = 0;
    stat->readinghits = 0;
    stat->runreadingsize = 0.0;
    stat->remreadingsize = 0.0;
    stat->parthits = 0;
    stat->runpartsize = 0.0;
    stat->rempartsize = 0.0;
    stat->interfere[0] = 0;
    stat->interfere[1] = 0;
    stat->requestedbus = 0.0;
    stat->waitingforbus = 0.0;
    stat->numbuswaits = 0;
}


static void disk_syncset_init (void)
{
    int i, j;
    int synced[128];

    for(i = 1; i <= NUMSYNCSETS; i++) {
        synced[i] = 0;
    }

    for(i = 0; i < MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if(!currdisk) {
            continue;
        }

        currdisk->model->mech->dm_randomize_rpm(currdisk->model);
        //    currdisk->mech_state.theta = dm_angle_dtoi(DISKSIM_drand48());

        if(currdisk->syncset == 0) {

        }
        else if((currdisk->syncset > 0) &&
                (synced[(currdisk->syncset)] == 0))
        {
            for (j = i; j < MAXDEVICES; j++) {
                disk *currdisk2 = getdisk(j);
                if(!currdisk2) continue;

                if (currdisk2->syncset == currdisk->syncset) {
                    currdisk2->mech_state.theta = currdisk->mech_state.theta;
                }
            }
            synced[(currdisk->syncset)] = 1;
        }
    }
}


static void disk_postpass_perdisk (disk *currdisk)
{
    if ((currdisk->contread) && (currdisk->enablecache == FALSE)) {
        fprintf(stderr, "Makes no sense to use read-ahead but not use caching\n");
        ddbg_assert(0);
    }

    if ((currdisk->contread == BUFFER_NO_READ_AHEAD) &&
            (currdisk->minreadahead > 0)) {
        // fprintf(stderr, "'Minimum read-ahead (blks)' forced to zero due to 'Buffer continuous read' value\n");
        currdisk->minreadahead = 0;
    }

    if ((currdisk->maxreadahead < currdisk->minreadahead) ||
            ((currdisk->contread == BUFFER_NO_READ_AHEAD) &&
                    (currdisk->maxreadahead > 0))) {
        // fprintf(stderr, "'Maximum read-ahead (blks)' forced to zero due to 'Buffer continuous read' value\n");
        currdisk->maxreadahead = currdisk->minreadahead;
    }

    if (currdisk->keeprequestdata >= 0) {
        currdisk->keeprequestdata = abs(currdisk->keeprequestdata - 1);
    }

    if ((currdisk->readanyfreeblocks != FALSE) &&
            (currdisk->enablecache == FALSE)) {
        fprintf(stderr, "Makes no sense to read blocks with caching disabled\n");
        ddbg_assert(0);;
    }

    if ((currdisk->fastwrites) && (currdisk->enablecache == FALSE)) {
        fprintf(stderr, "Can't use fast write if not employing caching\n");
        ddbg_assert(0);;
    }

    /* ripped from the override function */

    if ((currdisk->readaheadifidle != 0) && (currdisk->readaheadifidle != 1)) {
        fprintf(stderr, "Invalid value for readaheadifidle in disk_postpass_perdisk: %d\n", currdisk->readaheadifidle);
        ddbg_assert(0);;
    }

    if ((currdisk->writecomb != 0) && (currdisk->writecomb != 1)) {
        fprintf(stderr, "Invalid value for writecomb in disk_postpass_perdisk: %d\n", currdisk->writecomb);
        ddbg_assert(0);;
    }

    if (currdisk->maxqlen < 0) {
        fprintf(stderr, "Invalid value for maxqlen in disk_postpass_perdisk: %d\n", currdisk->maxqlen);
        ddbg_assert(0);;
    }

    if ((currdisk->hold_bus_for_whole_read_xfer != 0) &&
            (currdisk->hold_bus_for_whole_read_xfer != 1)) {
        fprintf(stderr, "Invalid value for hold_bus_for_whole_read_xfer in disk_postpass_perdisk: %d\n", currdisk->hold_bus_for_whole_read_xfer);
        ddbg_assert(0);;
    }

    if ((currdisk->hold_bus_for_whole_write_xfer != 0) &&
            (currdisk->hold_bus_for_whole_write_xfer != 1)) {
        fprintf(stderr, "Invalid value for hold_bus_for_whole_write_xfer in disk_postpass_perdisk: %d\n", currdisk->hold_bus_for_whole_write_xfer);
        ddbg_assert(0);;
    }
    if ((currdisk->hold_bus_for_whole_read_xfer == 1) &&
            ((currdisk->sneakyfullreadhits == 1) ||
                    (currdisk->sneakypartialreadhits == 1) ||
                    (currdisk->sneakyintermediatereadhits == 1))) {
        fprintf(stderr, "hold_bus_for_whole_read_xfer and one or more sneakyreadhits detected disk_postpass_perdisk\n");
        ddbg_assert(0);;
    }

    if ((currdisk->almostreadhits != 0) && (currdisk->almostreadhits != 1)) {
        fprintf(stderr, "Invalid value for almostreadhits in disk_postpass_perdisk: %d\n", currdisk->almostreadhits);
        ddbg_assert(0);;
    }

    if ((currdisk->sneakyfullreadhits != 0) &&
            (currdisk->sneakyfullreadhits != 1)) {
        fprintf(stderr, "Invalid value for sneakyfullreadhits in disk_postpass_perdisk: %d\n", currdisk->sneakyfullreadhits);
        ddbg_assert(0);;
    }
    if ((currdisk->sneakyfullreadhits == 1) &&
            (currdisk->hold_bus_for_whole_read_xfer == 1)) {
        fprintf(stderr, "hold_bus_for_whole_read_xfer and sneakyfullreadhits detected disk_postpass_perdisk\n");
        ddbg_assert(0);;
    }

    if ((currdisk->sneakypartialreadhits != 0) &&
            (currdisk->sneakypartialreadhits != 1)) {
        fprintf(stderr, "Invalid value for sneakypartialreadhits in disk_postpass_perdisk: %d\n", currdisk->sneakypartialreadhits);
        ddbg_assert(0);;
    }
    if ((currdisk->sneakypartialreadhits == 1) &&
            (currdisk->hold_bus_for_whole_read_xfer == 1)) {
        fprintf(stderr, "hold_bus_for_whole_read_xfer and sneakypartialreadhits detected disk_postpass_perdisk\n");
        ddbg_assert(0);;
    }

    if ((currdisk->sneakyintermediatereadhits != 0) &&
            (currdisk->sneakyintermediatereadhits != 1)) {
        fprintf(stderr, "Invalid value for sneakyintermediatereadhits in disk_postpass_perdisk: %d\n", currdisk->sneakyintermediatereadhits);
        ddbg_assert(0);;
    }
    if ((currdisk->sneakyintermediatereadhits == 1) &&
            (currdisk->hold_bus_for_whole_read_xfer == 1)) {
        fprintf(stderr, "hold_bus_for_whole_read_xfer and sneakyintermediatereadhits detected disk_postpass_perdisk\n");
        ddbg_assert(0);;
    }

    if ((currdisk->readhitsonwritedata != 0) &&
            (currdisk->readhitsonwritedata != 1)) {
        fprintf(stderr, "Invalid value for readhitsonwritedata in disk_postpass_perdisk: %d\n", currdisk->readhitsonwritedata);
        ddbg_assert(0);;
    }

    if ((currdisk->writeprebuffering != 0) &&
            (currdisk->writeprebuffering != 1)) {
        fprintf(stderr, "Invalid value for writeprebuffering in disk_postpass_perdisk: %d\n", currdisk->writeprebuffering);
        ddbg_assert(0);;
    }

    if ((currdisk->preseeking != 0) && (currdisk->preseeking != 1) &&
            (currdisk->preseeking != 2)) {
        fprintf(stderr, "Invalid value for preseeking in disk_postpass_perdisk: %d\n", currdisk->preseeking);
        ddbg_assert(0);;
    }

    if ((currdisk->neverdisconnect != 0) && (currdisk->neverdisconnect != 1)) {
        fprintf(stderr, "Invalid value for neverdisconnect in disk_postpass_perdisk: %d\n", currdisk->neverdisconnect);
        ddbg_assert(0);;
    }

    if (currdisk->numsegs < 1) {
        fprintf(stderr, "Invalid value for numsegs in disk_postpass_perdisk: %d\n", currdisk->numsegs);
        ddbg_assert(0);;
    }

    if ((currdisk->segsize < 1) ||
            (currdisk->segsize > currdisk->model->dm_sectors)) {
        fprintf(stderr, "Invalid value for segsize in disk_postpass_perdisk: %d\n", currdisk->model->dm_sectors);
        ddbg_assert(0);;
    }

    if ((currdisk->numwritesegs > currdisk->numsegs) ||
            (currdisk->numwritesegs < 1)) {
        fprintf(stderr, "Invalid value for numwritesegs in disk_postpass_perdisk: %d\n", currdisk->numwritesegs);
        ddbg_assert(0);;
    }

    if ((currdisk->numsegs <= 1) && (currdisk->dedicatedwriteseg)) {
        fprintf(stderr, "Must have more segments than dedicated write segments\n");
        ddbg_assert(0);;
    }
    if ((currdisk->dedicatedwriteseg != NULL) &&
            (currdisk->dedicatedwriteseg != (void*)1)) {
        fprintf(stderr, "Invalid value for dedicatedwriteseg in disk_postpass_perdisk: %ld\n", (long)currdisk->dedicatedwriteseg);
        ddbg_assert(0);;
    }

    if ((currdisk->fastwrites != 0) && (currdisk->fastwrites != 1) && (currdisk->fastwrites != 2)) {
        fprintf(stderr, "Invalid value for fastwrites in disk_postpass_perdisk: %d\n", currdisk->fastwrites);
        ddbg_assert(0);;
    }
    if ((currdisk->fastwrites != 0) && (currdisk->enablecache == FALSE)) {
        fprintf(stderr, "disk_postpass_perdisk:  Can't use fast write if not employing caching\n");
        ddbg_assert(0);;
    }

    if (currdisk->model->dm_surfaces < 1) {
        fprintf(stderr, "Invalid value for numsurfaces in disk_postpass_perdisk: %d\n", currdisk->model->dm_surfaces);
        ddbg_assert(0);;
    }

    /* This is probably too difficult to be worthwhile  -rcohen
     if (currdisk->model->dm_cyls < 1) {
     fprintf(stderr, "Invalid value for numcyls in disk_postpass_perdisk: %d\n", currdisk->model->dm_cyls);
     ddbg_assert(0);;
     }
     */
}


static void disk_postpass (void)
{
    int i;

    if (NUMDISKS == 0) {
        return;
    }

    if ( READING_BUFFER_WHOLE_SERVTIME < 0.0) {
        fprintf(stderr, "Invalid value for reading_buffer_whole_servtime in disk_postpass: %f\n", READING_BUFFER_WHOLE_SERVTIME);
        ddbg_assert(0);;
    }


    ddbg_assert3( BUFFER_WHOLE_SERVTIME >= 0.0, ("Invalid value for buffer_whole_servtime in disk_postpass: %f\n", BUFFER_WHOLE_SERVTIME));


    if ( READING_BUFFER_PARTIAL_SERVTIME < 0.0) {
        fprintf(stderr, "Invalid value for reading_buffer_partial_servtime in disk_postpass: %f\n", READING_BUFFER_PARTIAL_SERVTIME);
        ddbg_assert(0);;
    }

    if ( BUFFER_PARTIAL_SERVTIME < 0.0) {
        fprintf(stderr, "Invalid value for buffer_partial_servtime in disk_postpass: %f\n", BUFFER_PARTIAL_SERVTIME);
        ddbg_assert(0);
    }

    for (i = 0; i < MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if (currdisk) {
            disk_postpass_perdisk(currdisk);
        }
    }

    /* syncset stuff, perhaps should be in its own function */
    /* unnecessary for now anyway */

    /*
    for (i = 0; i < MAXDEVICES; i++) {
    disk *currdisk = getdisk (i);
    if (currdisk) {
    currdisk->syncset = 0;
    }
    }
     */

}


void disk_setcallbacks ()
{
    disksim->enablement_disk    = disk_enablement_function;
    disksim->idlework_diskcltr  = disk_write_cache_idletime_detected;
    disksim->timerfunc_diskctlr = disk_write_cache_periodic_flush;
    ioqueue_setcallbacks();
}


static void disk_initialize_diskinfo ()
{
    disksim->diskinfo = (struct disk_info *)calloc (1, sizeof(disk_info_t));

    disksim->diskinfo->disks = (struct disk**) calloc(1, MAXDEVICES * sizeof(disk));
    disksim->diskinfo->disks_len = MAXDEVICES;

    /* important initialization of stuff that gets remapped into diskinfo */
    disk_printhack = 0;
    disk_printhacktime = 0.0;
    //  global_currtime = 0.0;
    //  global_currangle = 0.0;
    SWAP_FORWARD_ONLY = 1;
    ADDTOLATENCY                    = 0.0;
    BUFFER_PARTIAL_SERVTIME         = 0.000000001;
    READING_BUFFER_PARTIAL_SERVTIME = 0.000000001;
    BUFFER_WHOLE_SERVTIME           = 0.000000000;
    READING_BUFFER_WHOLE_SERVTIME   = 0.000000000;
}


void disk_resetstats (void)
{
    int i;

    for (i=0; i<MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if (currdisk) {
            ioqueue_resetstats(currdisk->queue);
            diskstatinit(i, 0);
        }
    }
}



void disk_set_syncset (int setstart, int setend)
{
    if ((setstart < 0) || (setend >= MAXDEVICES) || (setend < setstart)) {
        fprintf (stderr, "Bad range passed to disk_set_syncset (%d - %d)\n", setstart, setend);
        exit (0);
    }

    NUMSYNCSETS++;
    for (; setstart <= setend; setstart++) {
        disk *currdisk = getdisk (setstart);
        if (currdisk->syncset != 0) {
            fprintf (stderr, "Overlapping synchronized disk sets (%d and %d)\n", NUMSYNCSETS, currdisk->syncset);
            exit (0);
        }
        currdisk->syncset = NUMSYNCSETS;
    }
}


static void disk_interfere_printstats (int *set, int setsize, char *prefix)
{
    int seq = 0;
    int loc = 0;
    int i;

    if (device_printinterferestats == FALSE)
        return;

    for(i=0; i<setsize; i++) {
        disk *currdisk = getdisk (set[i]);
        seq += currdisk->stat.interfere[0];
        loc += currdisk->stat.interfere[1];
    }
    fprintf (outputfile, "%sSequential interference: %d\n", prefix, seq);
    fprintf (outputfile, "%sLocal interference:      %d\n", prefix, loc);
}


static void disk_buffer_printstats (int *set, int setsize, char *prefix)
{
    int writecombs = 0;
    int readmisses = 0;
    int writemisses = 0;
    int fullreadhits = 0;
    int prependhits = 0;
    int appendhits = 0;
    int readinghits = 0;
    double runreadingsize = 0.0;
    double remreadingsize = 0.0;
    int parthits = 0;
    double runpartsize = 0.0;
    double rempartsize = 0.0;
    double waitingforbus = 0.0;
    int numbuswaits = 0;
    int hits;
    int misses;
    int total;
    int reads;
    int writes;
    int i;

    if (device_printbufferstats == FALSE)
        return;

    for (i=0; i<setsize; i++) {
        disk *currdisk = getdisk (set[i]);
        writecombs += currdisk->stat.writecombs;
        readmisses += currdisk->stat.readmisses;
        writemisses += currdisk->stat.writemisses;
        fullreadhits += currdisk->stat.fullreadhits;
        prependhits += currdisk->stat.prependhits;
        appendhits += currdisk->stat.appendhits;
        readinghits += currdisk->stat.readinghits;
        runreadingsize += currdisk->stat.runreadingsize;
        remreadingsize += currdisk->stat.remreadingsize;
        parthits += currdisk->stat.parthits;
        runpartsize += currdisk->stat.runpartsize;
        rempartsize += currdisk->stat.rempartsize;
        waitingforbus += currdisk->stat.waitingforbus;
        numbuswaits += currdisk->stat.numbuswaits;
    }
    hits = fullreadhits + appendhits + prependhits + parthits + readinghits;
    misses = readmisses + writemisses;
    reads = fullreadhits + readmisses + readinghits + parthits;
    total = hits + misses;
    writes = total - reads;
    if (total == 0) {
        return;
    }
    fprintf(outputfile, "%sNumber of buffer accesses:    %d\n", prefix, total);
    fprintf(outputfile, "%sBuffer hit ratio:        %6d \t%f\n", prefix, hits, ((double) hits / (double) total));
    fprintf(outputfile, "%sBuffer miss ratio:            %6d \t%f\n", prefix, misses, ((double) misses / (double) total));
    fprintf(outputfile, "%sBuffer read hit ratio:        %6d \t%f \t%f\n", prefix, fullreadhits, ((double) fullreadhits / (double) max(1,reads)), ((double) fullreadhits / (double) total));
    fprintf(outputfile, "%sBuffer prepend hit ratio:       %6d \t%f\n", prefix, prependhits, ((double) prependhits / (double) max(1,writes)));
    fprintf(outputfile, "%sBuffer append hit ratio:       %6d \t%f\n", prefix, appendhits, ((double) appendhits / (double) max(1,writes)));
    fprintf(outputfile, "%sWrite combinations:           %6d \t%f\n", prefix, writecombs, ((double) writecombs / (double) max(1,writes)));
    fprintf(outputfile, "%sOngoing read-ahead hit ratio: %6d \t%f \t%f\n", prefix, readinghits, ((double) readinghits / (double) max(1,reads)), ((double) readinghits / (double) total));
    fprintf(outputfile, "%sAverage read-ahead hit size:  %f\n", prefix, (runreadingsize / (double) max(1,readinghits)));
    fprintf(outputfile, "%sAverage remaining read-ahead: %f\n", prefix, (remreadingsize / (double) max(1,readinghits)));
    fprintf(outputfile, "%sPartial read hit ratio: %6d \t%f \t%f\n", prefix, parthits, ((double) parthits / (double) max(1,reads)), ((double) parthits / (double) total));
    fprintf(outputfile, "%sAverage partial hit size:     %f\n", prefix, (runpartsize / (double) max(1,parthits)));
    fprintf(outputfile, "%sAverage remaining partial:    %f\n", prefix, (rempartsize / (double) max(1,parthits)));
    fprintf(outputfile, "%sTotal disk bus wait time: %f\n", prefix, waitingforbus);
    fprintf(outputfile, "%sNumber of disk bus waits: %d\n", prefix, numbuswaits);
}


static void disk_seek_printstats (int *set, int setsize, char *prefix)
{
    int i;
    int zeros = 0;
    statgen * statset[MAXDEVICES];
    double zerofrac;

    if (device_printseekstats == FALSE) {
        return;
    }

    for (i=0; i<setsize; i++) {
        disk *currdisk = getdisk (set[i]);
        zeros += currdisk->stat.zeroseeks;
        statset[i] = &currdisk->stat.seekdiststats;
    }
    if (stat_get_count_set(statset,setsize) > 0) {
        zerofrac = (double) zeros / (double) stat_get_count_set(statset, setsize);
    } else {
        zerofrac = 0.0;
    }
    fprintf (outputfile, "%sSeeks of zero distance:\t%d\t%f\n", prefix, zeros, zerofrac);
    stat_print_set(statset, setsize, prefix);

    for (i=0; i<setsize; i++) {
        disk *currdisk = getdisk (set[i]);
        statset[i] = &currdisk->stat.seektimestats;
    }
    stat_print_set(statset, setsize, prefix);
}


static void disk_latency_printstats (int *set, int setsize, char *prefix)
{
    int i;
    int zeros = 0;
    statgen * statset[MAXDEVICES];
    double zerofrac;
    disk *currdisk = NULL;

    if (device_printlatencystats == FALSE) {
        return;
    }

    for (i=0; i<setsize; i++) {
        currdisk = getdisk (set[i]);
        zeros += currdisk->stat.zerolatency;
        statset[i] = &currdisk->stat.rotlatstats;
    }
    if (setsize == 1) {
        fprintf (outputfile, "%sFull rotation time:      %f\n", prefix, dm_time_itod(currdisk->model->mech->dm_period(currdisk->model)));
    }
    if (stat_get_count_set(statset,setsize) > 0) {
        zerofrac = (double) zeros / (double) stat_get_count_set(statset, setsize);
    } else {
        zerofrac = 0.0;
    }
    fprintf (outputfile, "%sZero rotate latency:\t%d\t%f\n", prefix, zeros, zerofrac);
    stat_print_set(statset, setsize, prefix);
}


static void disk_transfer_printstats (int *set, int setsize, char *prefix)
{
    int i;
    statgen * statset[MAXDEVICES];

    if (device_printxferstats) {
        for (i=0; i<setsize; i++) {
            disk *currdisk = getdisk (set[i]);
            statset[i] = &currdisk->stat.xfertimestats;
        }
        stat_print_set(statset, setsize, prefix);
    }
}


static void disk_acctime_printstats (int *set, int setsize, char *prefix)
{
    int i;
    statgen * statset[MAXDEVICES];

    if (device_printacctimestats) {
        for (i=0; i<setsize; i++) {
            disk *currdisk = getdisk (set[i]);
            statset[i] = &currdisk->stat.postimestats;
        }
        stat_print_set(statset, setsize, prefix);
        for (i=0; i<setsize; i++) {
            disk *currdisk = getdisk (set[i]);
            statset[i] = &currdisk->stat.acctimestats;
        }
        stat_print_set(statset, setsize, prefix);
    }
}


void disk_printsetstats (int *set, int setsize, char *sourcestr)
{
    int i;
    struct ioq * queueset[MAXDEVICES];
    int reqcnt = 0;
    char prefix[80];

    sprintf(prefix, "%sdisk ", sourcestr);
    for (i=0; i<setsize; i++) {
        disk *currdisk = getdisk (set[i]);
        queueset[i] = currdisk->queue;
        reqcnt += ioqueue_get_number_of_requests(currdisk->queue);
    }
    if (reqcnt == 0) {
        fprintf (outputfile, "\nNo disk requests for members of this set\n\n");
        return;
    }
    ioqueue_printstats(queueset, setsize, prefix);

    disk_seek_printstats(set, setsize, prefix);
    disk_latency_printstats(set, setsize, prefix);
    disk_transfer_printstats(set, setsize, prefix);
    disk_acctime_printstats(set, setsize, prefix);
    disk_interfere_printstats(set, setsize, prefix);
    disk_buffer_printstats(set, setsize, prefix);
}


void disk_printstats (void)
{
    struct ioq * queueset[MAXDEVICES];
    int set[MAXDEVICES];
    int i;
    int reqcnt = 0;
    char prefix[80];
    int diskcnt;

    fprintf(outputfile, "\nDISK STATISTICS\n");
    fprintf(outputfile, "---------------\n\n");

    sprintf(prefix, "Disk ");

    diskcnt = 0;
    for (i=0; i<MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if (currdisk) {
            queueset[diskcnt] = currdisk->queue;
            reqcnt += ioqueue_get_number_of_requests(currdisk->queue);
            diskcnt++;
        }
    }
    ddbg_assert (diskcnt == NUMDISKS);

    if (reqcnt == 0) {
        fprintf(outputfile, "No disk requests encountered\n");
        return;
    }
    /*
    fprintf(outputfile, "Number of extra write disconnects:   %5d  \t%f\n", extra_write_disconnects, ((double) extra_write_disconnects / (double) reqcnt));
     */
    ioqueue_printstats(queueset, NUMDISKS, prefix);

    diskcnt = 0;
    for (i=0; i<MAXDEVICES; i++) {
        disk *currdisk = getdisk (i);
        if (currdisk) {
            set[diskcnt] = i;
            diskcnt++;
        }
    }
    ddbg_assert (diskcnt == NUMDISKS);

    disk_seek_printstats(set, NUMDISKS, prefix);
    disk_latency_printstats(set, NUMDISKS, prefix);
    disk_transfer_printstats(set, NUMDISKS, prefix);
    disk_acctime_printstats(set, NUMDISKS, prefix);
    disk_interfere_printstats(set, NUMDISKS, prefix);
    disk_buffer_printstats(set, NUMDISKS, prefix);
    fprintf (outputfile, "\n\n");

    if (NUMDISKS <= 1) {
        return;
    }

    for (i=0; i<NUMDISKS; i++) {
        disk *currdisk = getdisk (set[i]);
        if (currdisk->printstats == FALSE) {
            continue;
        }
        if (ioqueue_get_number_of_requests(currdisk->queue) == 0) {
            fprintf(outputfile, "No requests for disk #%d\n\n\n", set[i]);
            continue;
        }
        fprintf(outputfile, "Disk #%d:\n\n", set[i]);
        fprintf(outputfile, "Disk #%d highest block number requested: %d\n", set[i], currdisk->stat.highblkno);
        sprintf(prefix, "Disk #%d ", set[i]);
        ioqueue_printstats(&currdisk->queue, 1, prefix);
        disk_seek_printstats(&set[i], 1, prefix);
        disk_latency_printstats(&set[i], 1, prefix);
        disk_transfer_printstats(&set[i], 1, prefix);
        disk_acctime_printstats(&set[i], 1, prefix);
        disk_interfere_printstats(&set[i], 1, prefix);
        disk_buffer_printstats(&set[i], 1, prefix);
        fprintf (outputfile, "\n\n");
    }
}



// prototype for dm_disk_loadparams()
#include <diskmodel/modules/modules.h>

//
// Create a disk from a param parse tree.  Install it into the 
// global diskinfo table, etc.
//
disk *disksim_disk_loadparams(struct lp_block *b,
        int *num)
{
    int c;
    disk *result;
    int num2;

    device_initialize_deviceinfo();

    if(!disksim->diskinfo) disk_initialize_diskinfo();

    disk_setcallbacks();
    disk_postpass();
    disk_syncset_init();

    result = (disk *)calloc(1, sizeof(disk));
    if(!result) return 0;
    //  memset(result, 0, sizeof(struct disk));

    result->hdr = disk_hdr_initializer;
    result->hdr.device_name = strdup(b->name);
    num2 = disk_add(result);
    if(num) *num = num2;

    ((struct device_header *)result)->device_type = DEVICETYPE_DISK;

    //   #include "modules/disksim_disk_param.c"
    lp_loadparams(result, b, &disksim_disk_mod);


    device_add((struct device_header *)result, num2);

    return result;
}




struct device_header *disk_copy(struct device_header *orig) {

    struct disk *result = (disk *)malloc(sizeof(disk));
    memcpy((struct disk *)result, orig, sizeof(disk));

    //  bandcopy(&result->bands, orig->bands, orig->numbands);


    result->queue = ioqueue_copy(((struct disk *)orig)->queue);
    return (struct device_header *)result;
}


int disk_load_syncsets(struct lp_block *b) {
    int c, d;
    int type, devnum;
    struct lp_list *l;
    disk *currdisk;

    ++NUMSYNCSETS;

    for(c = 0; c < b->params_len; c++) {
        if(!b->params[c]) continue;
        else if(strcmp(b->params[c]->name, "devices")) continue;
        l = LVAL(b->params[c]);
        for(d = 0; d < l->values_len; d++) {
            if(!l->values[c]) continue;
            currdisk = (disk*)getdevbyname(l->values[c]->v.s, 0, &devnum, &type);
            if(!currdisk || (type != DEVICETYPE_DISK)) {
                fprintf(stderr, "*** error: bad syncset spec: no such device %s or %s is of the wrong type.\n", l->values[c]->v.s, l->values[c]->v.s);
                return 0;
            }

            currdisk->syncset = NUMSYNCSETS;

        }
        break;
    }


    return 1;
}




void disk_initialize (void)
{
    int i, j;
    diskreq *tmpdiskreq;
    segment *tmpseg;

    // fprintf (outputfile, "Entered disk_initialize - numdisks %d\n", NUMDISKS);

    StaticAssert (sizeof(segment) <= DISKSIM_EVENT_SIZE);
    StaticAssert (sizeof(diskreq) <= DISKSIM_EVENT_SIZE);

    if (disksim->diskinfo == NULL) {
        disk_initialize_diskinfo();
    }

    disk_setcallbacks();
    disk_postpass();
    disk_syncset_init();

    for (i = 0; i < MAXDEVICES; i++) {
        disk *currdisk = getdisk (i); if(!currdisk) continue;
        /*        if (currdisk->inited != 0) { */
        /*           continue; */
        /*        } */
        ioqueue_initialize(currdisk->queue, i);
        ioqueue_set_enablement_function (currdisk->queue, &disksim->enablement_disk);
        addlisttoextraq((event **) &currdisk->outwait);
        addlisttoextraq((event **) &currdisk->buswait);

        if (currdisk->currentbus) {
            if (currdisk->currentbus == currdisk->effectivebus) {
                currdisk->effectivebus = NULL;
            }
            tmpdiskreq = currdisk->currentbus;
            if (tmpdiskreq->seg) {
                disk_buffer_remove_from_seg(tmpdiskreq);
            }
            addlisttoextraq((event **) &tmpdiskreq->ioreqlist);
            currdisk->currentbus = NULL;
            addtoextraq((event *) tmpdiskreq);
        }

        if (currdisk->effectivebus) {
            tmpdiskreq = currdisk->effectivebus;
            if (tmpdiskreq->seg) {
                disk_buffer_remove_from_seg(tmpdiskreq);
            }
            addlisttoextraq((event **) &tmpdiskreq->ioreqlist);
            currdisk->effectivebus = NULL;
            addtoextraq((event *) tmpdiskreq);
        }

        if (currdisk->currenthda) {
            if (currdisk->currenthda == currdisk->effectivehda) {
                currdisk->effectivehda = NULL;
            }
            tmpdiskreq = currdisk->currenthda;
            if (tmpdiskreq->seg) {
                disk_buffer_remove_from_seg(tmpdiskreq);
            }
            addlisttoextraq((event **) &tmpdiskreq->ioreqlist);
            currdisk->currenthda = NULL;
            addtoextraq((event *) tmpdiskreq);
        }

        if (currdisk->effectivehda != NULL) {
            tmpdiskreq = currdisk->effectivehda;
            if (tmpdiskreq->seg) {
                disk_buffer_remove_from_seg(tmpdiskreq);
            }
            addlisttoextraq((event **) &tmpdiskreq->ioreqlist);
            currdisk->effectivehda = NULL;
            addtoextraq((event *) tmpdiskreq);
        }

        while (currdisk->pendxfer) {
            tmpdiskreq = currdisk->pendxfer;
            if (tmpdiskreq->seg) {
                disk_buffer_remove_from_seg(tmpdiskreq);
            }
            addlisttoextraq((event **) &tmpdiskreq->ioreqlist);
            currdisk->pendxfer = tmpdiskreq->bus_next;
            addtoextraq((event *) tmpdiskreq);
        }

        currdisk->outstate = DISK_IDLE;
        currdisk->busy = FALSE;
        currdisk->prev_readahead_min = -1;
        currdisk->extradisc_diskreq = NULL;

        currdisk->currtime = 0.0;
        currdisk->lastflags = READ;

        currdisk->lastgen = -1;
        currdisk->busowned = -1;
        currdisk->numdirty = 0;
        if (currdisk->seglist == NULL) {
            currdisk->seglist = (segment *) DISKSIM_malloc(sizeof(segment));
            currdisk->seglist->next = NULL;
            currdisk->seglist->prev = NULL;
            currdisk->seglist->diskreqlist = NULL;
            currdisk->seglist->recyclereq = NULL;
            currdisk->seglist->access = NULL;
            for (j = 1; j < currdisk->numsegs; j++) {
                tmpseg = (segment *) DISKSIM_malloc(sizeof(segment));
                tmpseg->next = currdisk->seglist;
                currdisk->seglist = tmpseg;
                tmpseg->next->prev = tmpseg;
                tmpseg->prev = NULL;
                tmpseg->diskreqlist = NULL;
                tmpseg->recyclereq = NULL;
                tmpseg->access = NULL;
            }
            if (currdisk->dedicatedwriteseg) {
                currdisk->dedicatedwriteseg = currdisk->seglist;
            }
        }
        tmpseg = currdisk->seglist;

        // initialize cache buffers
        for (j = 0; j < currdisk->numsegs; j++) {
            tmpseg->outstate = BUFFER_IDLE;
            tmpseg->state = BUFFER_EMPTY;
            tmpseg->size = currdisk->segsize;
            while (tmpseg->diskreqlist) {
                addlisttoextraq((event **) &tmpseg->diskreqlist->ioreqlist);
                tmpdiskreq = tmpseg->diskreqlist;
                tmpseg->diskreqlist = tmpdiskreq->seg_next;
                addtoextraq((event *) tmpdiskreq);
            }
            /* recyclereq should have already been "recycled" :) by the
	  effectivehda or currenthda recycling above */
            tmpseg->recyclereq = NULL;
            addlisttoextraq((event **) &tmpseg->access);
            tmpseg = tmpseg->next;
        }

        currdisk->writeCacheFlushIdleDelay = 0.0;
        currdisk->writeCacheFlushPeriod    = 0.0;

        if( WCE == currdisk->writeCacheEnable )
        {
            currdisk->readhitsonwritedata = 1;
            if( currdisk->writeCacheFlushIdleDelay > 0.0 )
            {
               ioqueue_set_idlework_function( currdisk->queue, &disksim->idlework_diskcltr, currdisk, currdisk->writeCacheFlushIdleDelay );
            }

            if( currdisk->writeCacheFlushPeriod > 0.0 )
            {
                timer_event *timereq = (timer_event *) getfromextraq();
                timereq->type = TIMER_EXPIRED;
                timereq->func = &disksim->timerfunc_diskctlr;
                timereq->time = currdisk->writeCacheFlushPeriod;
                timereq->ptr  = currdisk;
                addtointq((event *)timereq);
            }
        }

        diskstatinit(i, TRUE);
    }
}



void disk_acctimestats (disk *currdisk, int distance, double seektime,
        double latency, double xfertime, double acctime)
{
    int dist;

#ifdef DEBUG_DISK
    fprintf( outputfile, "********* %f: disk_acctimestats   currdisk %p, Seek: distance %d, time %f, latency %f, xfertime %f, acctime %f\n", simtime, currdisk, distance, seektime, latency, xfertime, acctime );
    fflush( outputfile );
#endif

    if (device_printseekstats == TRUE) {
        dist = abs(distance);
        if (dist == 0) {
            currdisk->stat.zeroseeks++;
        }
        stat_update(&currdisk->stat.seekdiststats, (double) dist);
        stat_update(&currdisk->stat.seektimestats, seektime);
    }
    if (device_printlatencystats == TRUE) {
        if (latency == (double) 0.0) {
            currdisk->stat.zerolatency++;
        }
        stat_update(&currdisk->stat.rotlatstats, latency);
    }
    if (device_printxferstats == TRUE) {
        stat_update(&currdisk->stat.xfertimestats, xfertime);
    }
    if (device_printacctimestats == TRUE) {
        stat_update(&currdisk->stat.postimestats, (seektime+latency));
        stat_update(&currdisk->stat.acctimestats, acctime);
    }
}


static int disk_get_number_of_blocks(int n) {
    return disksim->diskinfo->disks[n]->model->dm_sectors;
}

static int disk_get_numcyls(int n) {
    return disksim->diskinfo->disks[n]->model->dm_cyls;
}

// this is an alias for dm_translate_ltop, basically
static void 
disk_get_mapping(int maptype,
        int n,
        int lbn,
        int *c,
        int *h,
        int *s)
{
    struct dm_pbn pbn;
    struct dm_disk_if *d = disksim->diskinfo->disks[n]->model;
    d->layout->dm_translate_ltop(d, lbn, (dm_layout_maptype)maptype, &pbn, 0);

    if(c) {
        *c = pbn.cyl;
    }

    if(h) {
        *h = pbn.head;
    }

    if(s) {
        *s = pbn.sector;
    }
}


int disk_get_avg_sectpercyl(int devno)
{
    return disksim->diskinfo->disks[devno]->sectpercyl;
}


/* default disk dev header */
struct device_header disk_hdr_initializer = { 
        DEVICETYPE_DISK,
        sizeof(struct disk),
        "unnamed disk",
        disk_copy,

        disk_set_depth,
        disk_get_depth,
        disk_get_inbus,
        disk_get_busno,
        disk_get_slotno,

        disk_get_number_of_blocks,

        disk_get_maxoutstanding,
        disk_get_numcyls,

        disk_get_blktranstime,
        disk_get_avg_sectpercyl,

        disk_get_mapping,
        disk_event_arrive,
        disk_get_distance,
        disk_get_servtime,
        disk_get_seektime,
        disk_get_acctime,
        disk_bus_delay_complete,
        disk_bus_ownership_grant
};

// End of file


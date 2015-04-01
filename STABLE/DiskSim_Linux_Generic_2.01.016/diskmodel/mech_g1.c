
/* diskmodel (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
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


#include "dm_config.h"
#include "dm.h"
#include "marshal.h"
#include "mech_g1.h"
#include "mech_g1_private.h"


// Port of first generation (disksim) disk mechanics implementation


static dm_time_t 
dm_seek_time_g1(struct dm_disk_if *d, 
        struct dm_mech_state *start_track,
        struct dm_mech_state *end_track,
        int rw)
{
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    dm_time_t seektime, hst, result = 0;

    seektime = m->seekfn(d,start_track,end_track,rw);

    //  seektime -= 700 * DM_TIME_USEC;

    hst = m->hdr.dm_headswitch_time(d,start_track->head,end_track->head);

    result = hst > seektime ? hst : seektime;

    if((result != 0) && (rw == 0)) {
        result += m->seekwritedelta;
    }

    return result;
}

// computes how long to reposition from the first head position to the second
// does not change the state of the disk
static dm_time_t 
dm_pos_time_g1(struct dm_disk_if *d,
        struct dm_mech_state *initial,
        struct dm_pbn *start,
        int len,
        int rw,
        int immed,
        struct dm_mech_acctimes *breakdown)
{
    dm_angle_t seekrotate;
    struct dm_mech_state end_track, state = *initial;
    dm_time_t seektime = 0;
    dm_time_t result = 0;
    dm_time_t latency = 0;
    //  struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;

    end_track.head = start->head; end_track.cyl = start->cyl;
    seektime = d->mech->dm_seek_time(d, &state, &end_track, rw);

    state.head = start->head;
    state.cyl = start->cyl;


    // advance the angle by how far we rotated during the seek
    seekrotate = d->mech->dm_rotate(d, &seektime);
    state.theta += seekrotate;

    // initial rotational latency given our new position
    latency = d->mech->dm_latency(d,
            &state,
            start->sector,
            len,
            immed,
            0);

    result = seektime + latency;

    if(breakdown) {
        breakdown->seektime = seektime;
        breakdown->initial_latency = latency;
        breakdown->initial_xfer = 0;
        breakdown->addl_latency = 0;
        breakdown->addl_xfer = 0;
    }

    return result;
}


// computes the access time for an access time on a single track
static dm_time_t
dm_acctime_track(
        struct dm_disk_if *d,
        struct dm_mech_state *istate,
        struct dm_pbn *start,
        int len,
        int rw,
        int immed,
        struct dm_mech_state *result_state,
        struct dm_mech_acctimes *breakdown)
{
    //  struct dm_mech_state start_track;
    struct dm_mech_state temp;
    //  struct dm_pbn seekdest;
    struct dm_mech_state state = *istate;
    dm_time_t seektime, latency, xfertime, initxfer;
    dm_time_t addtolatency = 0, result = 0, *atlp;

    // start by seeking/headswitching/writesettling
    temp.cyl = start->cyl; temp.head = start->head;
    seektime = d->mech->dm_seek_time(d, &state, &temp, rw);

    // we're on track now
    state.head = start->head;
    state.cyl = start->cyl;

    // advance the angle by how far we rotated during the seek
    temp.theta = d->mech->dm_rotate(d, &seektime);
    state.theta += temp.theta;

    // initial rotational latency given our new position
    atlp = immed ? &addtolatency : 0;
    latency = d->mech->dm_latency(d,
            &state,
            start->sector,
            len,
            immed,
            atlp);

    // if zero-latency access, add intermediate rotational latency
    latency += immed ? *atlp : 0;

    // data transfer
    xfertime = d->mech->dm_xfertime(d, &state, len);

    result = seektime + latency + xfertime;

    if(result_state) {
        result_state->head = start->head;
        result_state->cyl = start->cyl;
        result_state->theta = istate->theta;
        result_state->theta += d->mech->dm_rotate(d, &result);
    }


    // little gross; need to break down the xfertime for zero-latency access
    if(immed) {
        int firstsect = d->mech->dm_access_block(d,
                &state,
                start->sector,
                len,
                immed);

        initxfer = d->mech->dm_xfertime(d, &state, len - firstsect + 1);
    }
    else {
        initxfer = xfertime;
    }

    if(breakdown) {
        breakdown->seektime = seektime;
        breakdown->initial_latency = latency - addtolatency;
        breakdown->initial_xfer = initxfer;
        breakdown->addl_latency = addtolatency;
        breakdown->addl_xfer = xfertime - initxfer;
    }


    return result;
}

// Note that this is an ESTIMATE and if there are defects,
// the result here will be SMALLER than the correct value.
// This works for accesses spanning multiple tracks.  In this case,
// the access time breakdown is as follows: single track
// accesses are normal.  Multi-track accesses
// report the initial seek and rotation as the positioning time
// before the first transfer begins.  All additional xfertime 
// and postime is reported as initial transfer time.
static dm_time_t
dm_acctime_g1(struct dm_disk_if *d, 
        struct dm_mech_state *istate,
        struct dm_pbn *start,
        int len,
        int rw,
        int immed,
        struct dm_mech_state *result_state,
        struct dm_mech_acctimes *breakdown)
{
    struct dm_pbn pbn = *start;
    int lbnhigh; // highest lbn on the current track

    int l;       // first lbn to read
    int lastlbn; // last lbn to read

    struct dm_mech_state s = *istate;
    struct dm_mech_state s2;

    dm_time_t result = 0;

    if(breakdown) {
        breakdown->seektime = 0;
        breakdown->initial_latency = 0;
        breakdown->initial_xfer = 0;
        breakdown->addl_latency = 0;
        breakdown->addl_xfer = 0;
    }

    // back translate the pbn
    l = d->layout->dm_translate_ptol(d, &pbn, 0);
    // last lbn we're going to read
    lastlbn = l+len-1;

    do {
        struct dm_mech_acctimes ibreak; // intermediate

        //    l = d->layout->dm_translate_ptol(d, &pbn, 0);

        d->layout->dm_translate_ltop(d, l, MAP_FULL, &pbn, 0);
        d->layout->dm_get_track_boundaries(d,
                &pbn,
                0,        // first lbn on track
                &lbnhigh,
                0);       // remapsector

        ddbg_assert((lbnhigh != DM_SLIPPED) && (lbnhigh != DM_REMAPPED));

        if(lbnhigh > lastlbn) { lbnhigh = lastlbn; }  // check for end of track

        // redundant??
        //    d->layout->dm_translate_ltop(d, l, MAP_FULL, &pbn, 0);

        // access from l to lbnhigh at each step

        result += dm_acctime_track(d,
                &s,
                &pbn,
                lbnhigh - l + 1,
                rw,
                immed,
                &s2,
                &ibreak);

        s = s2;

        l = lbnhigh + 1;

        if(breakdown) {
            if((pbn.cyl == start->cyl)
                    && (pbn.head == start->head)
                    && (pbn.sector == start->sector))
            {
                breakdown->seektime = ibreak.seektime;
                breakdown->initial_latency = ibreak.initial_latency;
                breakdown->initial_xfer = ibreak.initial_xfer;
                if(lbnhigh == lastlbn) {
                    // single track access
                    breakdown->addl_latency = ibreak.addl_latency;
                    breakdown->addl_xfer = ibreak.addl_xfer;
                }
                else {
                    breakdown->initial_xfer += ibreak.addl_latency;
                    breakdown->initial_xfer += ibreak.addl_xfer;
                }
            }
            else {
                /*  	breakdown->initial_xfer += ibreak.seektime; */
                /*  	breakdown->initial_xfer += ibreak.initial_latency; */
                /*  	breakdown->initial_xfer += ibreak.initial_xfer; */
                /*  	breakdown->initial_xfer += ibreak.addl_latency; */
                /*  	breakdown->initial_xfer += ibreak.addl_xfer; */
            }
        }

    } while(lastlbn > lbnhigh);

    return result;
}


// compute the rotational latency incurred for the given
// (possibly "zero-latency") access
static dm_time_t 
dm_latency_g1(struct dm_disk_if *d,
        struct dm_mech_state *begin,
        int start_sect,
        int len,
        int immed,
        dm_time_t *addtolatency)
{
    struct dm_pbn tmppbn;
    int sectors;

    //printf( "cyl %d, head %d, angle %u, start_sect %d, len %d, immed %d, addtolatency %p\n", begin->cyl, begin->head, begin->theta, start_sect, len, immed, addtolatency );
    //fflush(stdout);

    //  dm_angle_t trackstart;
    dm_angle_t start, end;
    dm_angle_t theta = begin->theta; // rotational position of the disk
    dm_time_t result;
    //  dm_time_t period = d->mech->dm_period(d);

    tmppbn.head = begin->head;
    tmppbn.cyl = begin->cyl;
    tmppbn.sector = 0; // XXX sector may make a difference to g4 layout!
    sectors = d->layout->dm_get_sectors_pbn(d, &tmppbn);

    ddbg_assert(len <= sectors);

    tmppbn.head = begin->head;
    tmppbn.cyl = begin->cyl;
    tmppbn.sector = start_sect;
    start = d->layout->dm_pbn_skew(d, &tmppbn);

    //printf( "sectors %d, start %u\n", sectors, start );
    //printf("%s() skew %f\n", __func__, dm_angle_itod(start));
    //fflush(stdout);


    // non-zero-latency
    if(!immed) {
        //printf( "In if statement\n" );
        //printf("mech_g1::latency skew = %f distance = %f\n",
        //        dm_angle_itod(start), dm_angle_itod(start - theta));

        // rottime does the Right Thing if theta > start
        result = d->mech->dm_rottime(d, theta, start);

        if(addtolatency) {
            *addtolatency = 0;
        }
        return result;
    }
    else {
        //printf( "In else statement\n" );
        dm_angle_t extra = 0;
        tmppbn.sector = (start_sect + len - 1); // % sectors;

        if(tmppbn.sector >= sectors) {
            tmppbn.sector = sectors - 1;
        }

        //    d->layout->dm_convert_ptoa(d, &tmppbn, &end, 0);
        end = d->layout->dm_pbn_skew(d, &tmppbn);


        // head is outside of the access
        if(((theta <= start) && (start < end))
                || ( (end < theta) && (theta <= start) )
                || ( (start < end) && (end <= theta))
                || (tmppbn.sector == start_sect))

        {
            extra = 0;
        }
        // head is in the middle of the access
        else {
            tmppbn.head = begin->head;
            tmppbn.cyl = begin->cyl;
            tmppbn.sector = d->mech->dm_access_block(d, begin, start_sect, len, immed);
            start = d->layout->dm_pbn_skew(d, &tmppbn);

            if(len != sectors) {
                int extrasectors = sectors - len;
                struct dm_pbn tmppbn2;

                tmppbn2.head = begin->head;
                tmppbn2.cyl = begin->cyl;
                tmppbn2.sector = extrasectors;

                extra = d->layout->dm_get_sector_width(d, &tmppbn2, extrasectors);
                ddbg_assert(extra != 0);
            }
            else {
                extra = 0;
            }
        }

        result = d->mech->dm_rottime(d, theta, start);

        if(addtolatency && extra) {
            *addtolatency = d->mech->dm_rottime(d, 0, extra);
        }

        return result;
    }

    // UNREACHED
    return 0;
}

static dm_time_t 
dm_latency_seq_g1(struct dm_disk_if *d,
        struct dm_mech_state *begin,
        int start_sect,
        int len,
        int immed,
        dm_time_t *addtolatency)
{
    ddbg_assert(0);
    return 0;
}


// initial->theta may be in unmapped space.  For the time being,
// we assume a whole-track layout and return start_sect in that case.
static int
g1_access_block(struct dm_disk_if *d,
        struct dm_mech_state *initial,
        int start_sect,
        int len,
        int immed)
{
    int sectors;
    dm_ptol_result_t rv;
    dm_angle_t start, end, skew;
    struct dm_pbn tmppbn;
    dm_angle_t theta = initial->theta;


    tmppbn.cyl = initial->cyl;
    tmppbn.head = initial->head;
    tmppbn.sector = start_sect;
    sectors = d->layout->dm_get_sectors_pbn(d, &tmppbn);


    ddbg_assert(len <= sectors);

    skew = d->layout->dm_pbn_skew(d, &tmppbn);
    start = skew;

    if(!immed) {
        return start_sect;
    }
    else {
        //    dm_angle_t extra = 0;
        tmppbn.sector = (start_sect + len - 1); // % sectors;

        if(tmppbn.sector >= sectors) {
            tmppbn.sector = sectors - 1;
        }

        //    d->layout->dm_convert_ptoa(d, &tmppbn, &end, 0);
        end = d->layout->dm_pbn_skew(d, &tmppbn);

        // head is between 0 and the start of the access
        if(((theta <= start) && (start < end))
                || ( (end < theta) && (theta <= start) )
                || ( (start < end) && (end <= theta))
                || (tmppbn.sector == start_sect)) // corner case!
        {
            ddbg_assert(start_sect < sectors);
            return start_sect;
        }
        // head is in the middle of the access
        else {
            // dm_angle_t skewed;
            dm_angle_t track_skew;
            struct dm_pbn track_endpbn;
            struct dm_mech_state skewed;
            struct dm_pbn result_pbn;
            dm_angle_t sectWidth =
                    d->layout->dm_get_sector_width(d, &tmppbn, 1);

            if(sectWidth == 0) {
                goto out_nx;
            }

            track_endpbn.cyl = initial->cyl;
            track_endpbn.head = initial->head;

            track_endpbn.sector = 0;
            track_skew = d->layout->dm_pbn_skew(d, &track_endpbn);

            skewed.cyl = initial->cyl;
            skewed.head = initial->head;
            skewed.theta = theta - track_skew;

            if((skewed.theta % sectWidth) != 0) {
                skewed.theta += (sectWidth - (skewed.theta % sectWidth));
            }


            rv = d->layout->dm_convert_atop(d, &skewed, &result_pbn);
            if(rv < 0) {
                goto out_nx;
            }

            ddbg_assert(result_pbn.sector < sectors);
            return result_pbn.sector;
        }
    }


    out_nx:
    return start_sect;

}

static dm_time_t
dm_latency_g1_average(struct dm_disk_if *d,
        struct dm_mech_state *begin,
        int start,
        int len,
        int immed)
{
    dm_time_t result;
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    result = m->rotatetime >> 1;
    return result;
}


// compute how long it will take the disk to rotate from the angle
// in the first position to that in the second position

// use an average value of half the full-rotation time
static dm_time_t 
dm_rottime_g1_average(struct dm_disk_if *d,
        dm_angle_t junk1,
        dm_angle_t junk2)
{
    dm_time_t result;
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    result = m->rotatetime >> 1;
    return result;
}

static dm_time_t 
dm_rottime_g1(struct dm_disk_if *d,
        dm_angle_t begin,
        dm_angle_t end)
{
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    dm_angle_t diff;
    if(end == begin) {
        return 0;
    }
    else if(end > begin) {
        diff = end - begin;
    }
    else {
        // Ladies and Gentlemen, please disregard the man behind the curtain.
        diff = -(begin - end);
    }

    //printf("dm_rottime_g1: rotdistance = %f\n", dm_angle_itod(diff));



    // XXX this is immensly evil
    return ((((long long)diff << 20) >> DM_ANGLE_EXP) * m->rotatetime) >> 20;
}

// Amount of time to read len sectors from the track designated by
// the 2nd argument.  This is a convience wrapper for 
// rottime.
static dm_time_t 
dm_xfertime_g1(struct dm_disk_if *d,
        struct dm_mech_state *track,
        int len)
{
    struct dm_pbn pbn;
    //  int sectors;
    dm_angle_t theta;
    //  struct dm_mech_state theta2;

    pbn.head = track->head;
    pbn.cyl = track->cyl;
    pbn.sector = len - 1;

    // actually *don't* want to use this interface; we just want
    // (len / blkspertrack) * period
    //  d->layout->dm_convert_ptoa(d, &pbn, &theta2.theta, 0);

    //  sectors = d->layout->dm_get_sectors_pbn(d, &pbn);

    // XXX have to go up to 64 bits wide to represent this
    //  theta = ((long long)len << DM_ANGLE_EXP) / sectors;
    {
        struct dm_pbn tmppbn;
        tmppbn.head = track->head;
        tmppbn.cyl = track->cyl;
        tmppbn.sector = 0; // XXX XXX
        theta = d->layout->dm_get_sector_width(d, &tmppbn, len);
    }

    return d->mech->dm_rottime(d, 0, theta);
}


// this is probably constant for now but might not be in the future
static dm_time_t 
dm_headswitch_time_g1(struct dm_disk_if *d, 
        int h1,
        int h2)
{
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    if(h1 != h2) {
        return m->headswitch;
    }
    else {
        return 0;
    }
}


// how far will the media rotate in the given amount of time
// only the angle in the result is set
static dm_angle_t
dm_rotate_g1(struct dm_disk_if *d, 
        dm_time_t *time)
{
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    if(*time == 0) {
        return 0;
    }
    else {
        // time / time/tics = tics
        //    uint64_t result1
        uint64_t result2;

        //    result1 = dm_angle_dtoi(dm_time_itod(*time) / dm_time_itod(m->rotatetime));

        // This is a little tricky; if we do time << DM_ANGLE_T, we may
        // run out of bits so we shift by a bit less, then divide and then
        // shift some more.  Consequently, this is somewhat lossy.
        // These shifts assume that DM_TIME_EXP > DM_ANGLE_EXP and that a
        // dm_time_t is 64 bits wide

        result2 =  ((*time << (64 - DM_TIME_EXP))
                / m->rotatetime)
		               << (DM_TIME_EXP - DM_ANGLE_EXP);


        return result2;
    }
}


// assuming no activity, what will the state of the disk be
// at some time in the future
static void 
dm_progress_g1(struct dm_disk_if *d, 
        struct dm_pbn *cur_state,
        dm_time_t time,
        struct dm_pbn *result_state)
{
    ddbg_assert2(0, "unimplemented");
}

static dm_time_t
dm_period_g1(struct dm_disk_if *d) { 
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    return m->rotatetime;
}



int 
mech_g1_marshaled_len(struct dm_disk_if *d) {
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;
    int result = sizeof(struct dm_mech_g1);
    result += sizeof(struct dm_marshal_hdr);
    result += m->xseekcnt * (sizeof(int) + sizeof(dm_time_t));
    return result;
}



static void dm_mech_g1_set_period(struct dm_disk_if *d) {
    //  fprintf(stderr, "*** warning: RPM randomization is disabled\n");
}


// random prototypes for the fns array
static void *mech_g1_marshal(struct dm_disk_if *d, char *b);

void *mech_g1_fns[] = {
        dm_seek_time_g1,
        g1_access_block,
        dm_latency_g1,
        dm_pos_time_g1,
        dm_acctime_g1,
        dm_rottime_g1,
        dm_xfertime_g1,
        dm_headswitch_time_g1,
        dm_rotate_g1,
        dm_period_g1,

        dm_mech_g1_set_period,

        mech_g1_marshaled_len,
        mech_g1_marshal,

        dm_mech_g1_seek_const,
        dm_mech_g1_seek_3pt_line,
        dm_mech_g1_seek_3pt_curve,
        dm_mech_g1_seek_hpl,
        dm_mech_g1_seek_1st10_plus_hpl,
        dm_mech_g1_seek_extracted,

        dm_latency_seq_g1
};


static void *
mech_g1_marshal(struct dm_disk_if *d, char *b) {
    void **fns;
    int i, fns_len, seekfns_len;
    char *ptr = b;

    struct dm_marshal_hdr *h = (struct dm_marshal_hdr *)b;
    struct dm_mech_g1 *m = (struct dm_mech_g1 *)d->mech;


    //    fns = malloc(sizeof(mech_g1_fns) + sizeof(DM_MECH_G1_MAX_SEEK * sizeof(void*)));
    //    fns_len = (sizeof(mech_g1_fns) / sizeof(void *));
    //    seekfns_len = DM_MECH_G1_MAX_SEEK+1;

    //    for(i = 0; i < fns_len; i++) {
    //      fns[i] = mech_g1_fns[i];
    //    }
    //    for(i = fns_len; i < fns_len + seekfns_len; i++) {
    //      fns[i] = dm_mech_g1_seekfns[i];
    //    }


    h->type = DM_MECH_G1_TYP;
    h->len = mech_g1_marshaled_len(d);
    ptr += sizeof(*h);


    memcpy(ptr, (char *)m, sizeof(struct dm_mech_g1));
    marshal_fns((void **)&m->hdr,
            sizeof(struct dm_mech_if) / sizeof(void *),
            ptr,
            DM_MECH_G1_TYP);

    {
        int *seekfnoffset = (int *)ptr + sizeof(struct dm_mech_if);

        marshal_fn((void *)m->seekfn, DM_MECH_G1_TYP,
                (struct marshaled_fn *)seekfnoffset);
    }

    ptr += sizeof(struct dm_mech_g1) + sizeof(void*);
    if(m->xseekcnt != 0) {
        int distsize = m->xseekcnt * sizeof(int);
        int timesize = m->xseekcnt * sizeof(dm_time_t);

        memcpy(ptr, (char *)m->xseekdists, distsize);
        ptr += distsize;
        memcpy(ptr, (char *)m->xseektimes, timesize);
        ptr += timesize;
    }

    return (void *)ptr;
}

char *
mech_g1_unmarshal(struct dm_marshal_hdr *h,
        void **result,
        void *parent)
{
    char *ptr = (char *)h;
    struct dm_mech_g1 *m = malloc(sizeof(struct dm_mech_g1));

    ptr += sizeof(*h);
    memcpy((char *)m, ptr, sizeof(struct dm_mech_g1));
    unmarshal_fns((void **)&m->hdr,
            sizeof(struct dm_mech_if) / sizeof(void *),
            ptr,
            DM_MECH_G1_TYP);

    {
        int *seekfnoffset = (int *)ptr + sizeof(struct dm_mech_if);
        m->seekfn = unmarshal_fn(seekfnoffset,
                DM_MECH_G1_TYP);
    }
    ptr += sizeof(struct dm_mech_g1) + sizeof(void*);

    if(m->xseekcnt != 0) {
        int distsize = m->xseekcnt * sizeof(int);
        int timesize = m->xseekcnt * sizeof(dm_time_t);

        m->xseekdists = malloc(distsize);
        m->xseektimes = malloc(timesize);
        memcpy((char *)m->xseekdists, ptr, distsize);
        ptr += distsize;
        memcpy((char *)m->xseektimes, ptr, timesize);
        ptr += timesize;
    }


    m->disk = parent;

    *result = m;
    return ptr;
}


struct dm_marshal_module dm_mech_g1_marshal_mod = 
{ mech_g1_unmarshal,
        mech_g1_fns,
        sizeof(mech_g1_fns) / sizeof(void *)
};


// this is an initializer; some of these functions may change
// e.g. seek_time
struct dm_mech_if dm_mech_g1 = {
        dm_seek_time_g1,
        g1_access_block,
        dm_latency_g1,
        dm_latency_seq_g1,
        dm_pos_time_g1,
        dm_acctime_g1,
        dm_rottime_g1,

        dm_xfertime_g1,
        dm_headswitch_time_g1,
        dm_rotate_g1,
        //  dm_progress_g1,
        dm_period_g1,

        dm_mech_g1_set_period,

        mech_g1_marshaled_len,
        mech_g1_marshal
};

// End of file

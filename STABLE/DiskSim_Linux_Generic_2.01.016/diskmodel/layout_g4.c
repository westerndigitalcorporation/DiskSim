/* diskmodel (version 1.1)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2003-2005
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

#include "dm.h"

#include "layout_g4.h"
#include "layout_g4_private.h"


//#define DEBUG_MODEL_G4

void dump_g4_layout( struct dm_layout_g4 * g4l, char *name );
void dump_dm_pbn( struct dm_pbn *pbn, char *name );

/*
struct dm_pbn {
  int cyl;
  int head;
  int sector;
};
*/
void dump_dm_pbn( struct dm_pbn *pbn, char *name )
{
    printf( "\n\ndm_pbn: %s", name );
    printf( "\n  cyl   : %d",  pbn->cyl );
    printf( "\n  head  : %d",  pbn->head );
    printf( "\n  sector: %d\n",  pbn->sector );
}


//void
//dump_lp_list( struct lp_list *li, char *name )
//{
//    int valueIndex = 0;
//
//    printf( "\n\nlp_list: %s", name );
//    printf( "\n  source file: %p",  li->source_file );
//    printf( "\n  values_len: %d",  li->values_len );
//    printf( "\n  values_pop: %d",  li->values_pop );
//    for( valueIndex = 0; valueIndex < li->values_pop; valueIndex++ )
//    {
//        printf( "\n  value     :" );
//        printf( "\n    index      : %d", valueIndex );
//        printf( "\n    source file: %s", li->values[valueIndex]->source_file );
//        printf( "\n    type       : %d", li->values[valueIndex]->t );
//        switch( li->values[valueIndex]->t )
//        {
//        case S:
//            printf( "\n    string     : %s", li->values[valueIndex]->v.s );
//            break;
//        case I:
//            printf( "\n    integer    : %d", li->values[valueIndex]->v.i );
//            break;
//        case D:
//            printf( "\n    float    : %f", li->values[valueIndex]->v.d );
//            break;
//        }
//    }
//    fflush( stdout );
//}

void dump_g4_layout( struct dm_layout_g4 * g4l, char *name )
{
    struct idx *pIdx;
    struct idx_ent *pEnts;
    int i, j;

    printf( "\n\nModel G4 Format: %s", name );
    printf( "\n  Track Pattern format:" );
    printf( "\n index,low,high,spt,sw");
    for( i = 0; i < g4l->track_len; i++ )
    {
        printf( "\n%d,%d,%d,%d,%d", i, g4l->track[i].low, g4l->track[i].high, g4l->track[i].spt, g4l->track[i].sw );
    }

    printf( "\n  Index Format format:" );
    for( j = 0; j < g4l->idx_len; j ++ )
    {
        pIdx = &g4l->idx[j];
        printf( "\n    Index Section: %d", j );
        printf( "\nlbn,cyl,off,len,cyllen,alen,runlen,cylrunlen,childtype,head" );
        for( i = 0; i < pIdx->ents_len; i++ )
        {
            pEnts = &pIdx->ents[i];
            if( TRACK == pEnts->childtype )
            {
                printf( "\n%d,%d,%u,%d,%d,%u,%d,%d,%s,%d"
                    , pEnts->lbn, pEnts->cyl, pEnts->off, pEnts->len, pEnts->cyllen, pEnts->alen, pEnts->runlen, pEnts->cylrunlen, (0 == pEnts->childtype) ? "IDX" : "TRACK", pEnts->head );
            }
        }
    }
    printf( "\n\n" );
    fflush( stdout );
}


static inline int min(int x, int y) {
    return x < y ? x : y;
}


// slipcount(0 and slipcount_rev() are identical except that the former
// slides the lbn forward while the latter does not.

// optimization: keep track of the points at which the running total
// of the slip/spare list is 0.  Have a fast index of those and 
// start at the highest one <= lbn.

// Or:  keep running total and binsearch.

// XXX give a succinct explanation as to why ltop() wants the one
// and ptol() the other!

// This also needs to indicate when lbn coincides with a slip, i.e.
// in sequential access, you skip a sector.

// Returns the highest entry in the slip list <= lbn??

static int
slipcount_bins(struct dm_layout_g4 *l,
        int lbn,
        int low,
        int high)
{
    ddbg_assert(low <= high);
    ddbg_assert(0 <= low);
    ddbg_assert(high <= l->slips_len);

    if(high == low+1 || high == low) {
        return low;
    }
    else {
        int midpt = low + ((high - low) / 2);
        if(l->slips[midpt].off > lbn) {
            high = midpt;
        }
        else {  // off <= lbn
            low = midpt;
        }
        return slipcount_bins(l,lbn,low,high);
    }
}

static int 
slipcount(struct dm_layout_g4 *l,
        int lbn)
{
    int i;
    int ct;
    int l2 = lbn;

    // the upper limit of l->slips_len is not in error
    i = slipcount_bins(l, lbn, 0, l->slips_len);
    ddbg_assert(0 <= i);
    ddbg_assert( l->slips_len >= i );

    ct = l->slips[i].count;

    while(i < l->slips_len && l->slips[i].off <= l2) {
        ct = l->slips[i].count;
        l2 = lbn + ct;
        i++;
    }

    return ct;
}

static int 
slipcount_rev(struct dm_layout_g4 *l,
        int lbn,
        int *slip_len)
{
    int i = 0;

    // the upper limit of l->slips_len is not in error
    i = slipcount_bins(l, lbn, 0, l->slips_len);
    ddbg_assert(0 <= i);
    ddbg_assert( l->slips_len >= i );

    if(l->slips[i].off <= lbn
            && (i > 0
                    && lbn < l->slips[i].off
                    + (l->slips[i].count - l->slips[i-1].count)))
    {
        if(slip_len) {
            ddbg_assert(0);      // XXX fixme
        }

        return -1;
    }
    else {
        return l->slips[i].count;
    }
}


struct remap *
remap_lbn(struct dm_layout_g4 *l,
        int lbn)
{
    int i;
    struct remap *r, *result = 0;

    for(i = 0, r = &l->remaps[i]; i < l->remaps_len; i++, r++) {
        if(r->off <= lbn && lbn < (r->off + r->count)) {
            result = r;
            break;
        }
    }

    return result;
}

// is p the destination of a remap?
struct remap *
remap_pbn(struct dm_layout_g4 *l,
        struct dm_pbn *p)
{
    int i;
    struct remap *r, *result = 0;
    for(i = 0, r = &l->remaps[i]; i < l->remaps_len; i++, r++) {
        if(r->dest.cyl == p->cyl
                && r->dest.head == p->head
                && r->dest.sector <= p->sector
                && p->sector < (r->dest.sector + r->count))
        {
            result = r;
            break;
        }
    }

    return result;
}


// Maybe refactor into a single set
// of "recurse" routines that take exactly one of a
// lbn or pbn and build the path to the tp containing it.

void
g4_path_append(struct g4_path *p, 
        union g4_node n,
        g4_node_t t,
        int i,
        int quot,
        int resid)
{
    ddbg_assert(p->length < G4_ALLOC_PATH);
    struct g4_path_node *pp = &p->path[p->length];

    pp->n = n;
    pp->type = t;
    pp->i = i;

    pp->quot = quot;
    pp->resid = resid;

    ++p->length;
}


// For ptol, have to keep track of the size of the enclosing thing so
// we don't map past the end of it.
struct g4_path *
g4_r(struct dm_layout_g4 *l, 
        union g4_node *n,
        g4_node_t t,
        int *lbn,
        int max,
        struct dm_pbn *p,
        struct g4_path *acc) // accumlator
{
    int i;
    union g4_node *nn; // where we're going to recurse
    g4_node_t tt;

    int quot = 0;
    int resid = 1;

    ddbg_assert(lbn || p);

    switch(t) {
    case TRACK:
        if(lbn) {
            if(n->t->low <= *lbn && *lbn <= n->t->high) {

                // ?
                resid = *lbn - n->t->low;

                nn = 0;
                goto out;
            }
            else {
                goto out_err;
            }
        }
        else {
            if(n->t->low <= p->sector && p->sector <= n->t->high) {
                if(p->sector > max) {
                    goto out_err;
                }

                // quot/resid?
                resid = p->sector - n->t->low;

                nn = 0;
                goto out;
            }
            else {
                goto out_err;
            }
        }
        break;

    case IDX:
    {
        struct idx_ent *e;
        for(i = 0, e = &n->i->ents[0]; i < n->i->ents_len; i++, e++)
        {
            if(lbn) {
                if(e->lbn <= *lbn && *lbn < (e->lbn + e->runlen)) {
                    *lbn -= e->lbn;

                    quot = *lbn / e->len;
                    // XXX
                    quot = e->cylrunlen < 0 ? -quot : quot;
                    resid = *lbn % e->len;

                    nn = &e->child;
                    tt = e->childtype;

                    *lbn = resid;
                    goto out;
                }
            }
            else {
                int low = e->cylrunlen > 0 ? e->cyl : e->cyl + e->cylrunlen + 1;
                int high = e->cylrunlen < 0 ? e->cyl : e->cyl + e->cylrunlen - 1;
                if(low <= p->cyl && p->cyl <= high
                        && (e->childtype != TRACK || p->head == e->head))

                {
                    p->cyl -= e->cyl;

                    quot = p->cyl / e->cyllen;
                    resid = p->cyl % e->cyllen;

                    max = min(max, e->lbn + e->runlen - 1);
                    max -= e->lbn;

                    // cut down again for RLE
                    quot = p->cyl / e->cyllen;
                    // XXX
                    quot = e->cylrunlen < 0 ? -quot : quot;
                    max = min(max, (quot+1) * e->len - 1);
                    max -= quot * e->len;

                    resid = p->cyl % e->cyllen;

                    p->cyl = resid;

                    nn = &e->child;
                    tt = e->childtype;
                    goto out;
                }
            }
        }
        goto out_err;
    } break;
    default:
        ddbg_assert(0);
        break;
    }

    out:
    g4_path_append(acc, *n, t, i, quot, resid);
    if(nn) {
        return g4_r(l, nn, tt, lbn, max, p, acc);
    }
    else {
        return acc;
    }

    out_err:
    free(acc);
    acc = NULL;
    return 0;
}

struct g4_path *
g4_recurse(struct dm_layout_g4 *l, int *lbn, struct dm_pbn *p) 
{
    int lbncopy;
    struct dm_pbn pbncopy;
    struct g4_path *acc = calloc(1, sizeof(*acc));
    union g4_node n;
    int max;

    if(lbn) {
        lbncopy = *lbn;
        lbn = &lbncopy;
    }

    if(p) {
        pbncopy = *p;
        p = &pbncopy;
    }

    n.i = l->root;
    g4_path_append(acc, n, IDX,
            0,  // i
            0,  // quot
            1); // resid

    max = l->parent->dm_sectors-1;
    max += slipcount_rev(l, max, 0);
    return g4_r(l, &n, IDX, lbn, max, p, acc);
}


dm_ptol_result_t
ltop(struct dm_disk_if *d,
        int lbn,
        dm_layout_maptype junk,
        struct dm_pbn *result,
        int *remapsector)
{
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    struct remap *r;

#ifdef DEBUG_MODEL_G4
    printf( "ltop: lbn %d\n", lbn );
    dump_g4_layout( l, "g4_recurse" );
#endif

    if(lbn < 0 || d->dm_sectors <= lbn) {
        return DM_NX;
    }

    if((r = remap_lbn(l, lbn))) {
        *result = r->dest;
        result->sector += (lbn - r->off);

#ifdef DEBUG_MODEL_G4
        dump_dm_pbn( result, "ltop: remapped" );
#endif

        return DM_OK;
        // not always -- spare remaps
        // return DM_REMAPPED;
    }
    else {
        int i;
        struct g4_path *p;
        struct g4_path_node *n;

        // fiddle lbn according to slips/spares
        lbn += slipcount(l, lbn);

        p = g4_recurse(l, &lbn, 0);
        if(!p) {
            return DM_NX;
        }

        for(i = 0; i < p->length; ++i) {
            n = &p->path[i];
            switch(n->type) {
            case TRACK:
                result->sector = n->n.t->low + lbn;
                break;

            case IDX:
            {
                struct idx_ent *e = &n->n.i->ents[n->i];
                lbn -= e->lbn;

                if(i > 0) {
                    result->cyl += e->cyl + (n->quot * e->cyllen);
                }
                else {
                    result->cyl = e->cyl + (n->quot * e->cyllen);
                }

                if(e->childtype == TRACK) {
                    result->head = e->head;
                }

                lbn = n->resid;
            }
            break;

            default: ddbg_assert(0); break;
            }
        }
        free(p);
        p = NULL;
    }

#ifdef DEBUG_MODEL_G4
    dump_dm_pbn( result, "ltop:" );
#endif

    return DM_OK;
}


dm_ptol_result_t
ptol(struct dm_disk_if *d,
        struct dm_pbn *pbn,
        int *remapsector)
{
    int result = 0;
    struct dm_pbn pbncopy = *pbn; // we modify this
    struct remap *r;

    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;

    pbn = &pbncopy;

    if((r = remap_pbn(l, pbn))) {
        return r->off + pbn->sector - r->dest.sector;
    }
    else {
        int rv = 0;
        int i;
        struct g4_path *path;
        struct g4_path_node *n;

        path = g4_recurse(l, 0, pbn);

        if(!path) {
            return DM_NX;
        }

        for(i = 0; i < path->length; ++i) {
            n = &path->path[i];
            switch(n->type) {
            case TRACK:
                result += (pbn->sector - n->n.t->low);
                break;
            case IDX:
            {
                struct idx_ent *e = &n->n.i->ents[n->i];

                pbn->cyl -= e->cyl;

                if(i > 0) {
                    result += e->lbn + (n->quot * e->len);
                }
                else {
                    result = e->lbn + (n->quot * e->len);
                }

                pbn->cyl = n->resid;

            } break;

            default: ddbg_assert(0); break;
            }
        }

        // if result coincides with an entry in the slip list...

        rv = slipcount_rev(l, result, NULL); // &slipct
        if(rv == -1) {
            result = DM_NX;
        }
        else {
            result -= rv;
        }

        free(path);
        path = NULL;
    }

    return result;
}


static int
g4_spt(struct dm_layout_g4 *l,
        int *lbn,
        struct dm_pbn *pbn)
{
    struct g4_path *p;
    struct g4_path_node *n;
    int rv;

    p = g4_recurse(l, lbn, pbn);

    if(p) {
        n = &p->path[p->length - 1];
        ddbg_assert(n->type == TRACK);
        rv = n->n.t->spt;
        free(p);
        p = NULL;
    }
    else {
        rv = -1;
    }

    return rv;
}

int
g4_spt_lbn(struct dm_disk_if *d,
        int lbn)
{
    int result;
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    struct remap *r;

    lbn += slipcount(l,lbn);
    r = remap_lbn(l, lbn);

    if(r) {
        return r->spt;
    }
    else {
        return g4_spt(l, &lbn, 0);
    }
}


int
g4_spt_pbn(struct dm_disk_if *d,
        struct dm_pbn *p)
{
    int result;
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    struct remap *r;
    if((r = remap_pbn(l, p))) {
        return r->spt;
    }
    else {
        return g4_spt(l, 0, p);
    }
}

//*****************************************************************************
// Function: g4_track_bound
//    Get the beginning and ending LBN's on the specified track
//
// I'm punting on what this means for non-whole-track layouts right
// now.

// This assumes that li ... li+k => (c,h,0) ... (c,h,k-1)
//*****************************************************************************

dm_ptol_result_t
g4_track_bound(struct dm_disk_if *d,
        struct dm_pbn *pbn,
        int *l0,
        int *ln,
        int *remapsector)
{
    int lbn;
    struct remap *r;
    struct dm_pbn pi;
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;

    int spt = g4_spt_pbn(d, pbn);

    /* Dushyanth: must set l0 and ln even if ptol() returns an error */
    lbn = ptol(d, pbn, 0);
    //if(lbn < 0) {
    //  return lbn;
    //}

    if(l0) {
        pi = *pbn;
        pi.sector = 0;

        while((*l0 = ptol(d, &pi, 0)) < 0) {
            pi.sector++;
        }

        ddbg_assert(*l0 < d->dm_sectors);
    }

    if(ln) {
        pi = *pbn;
        pi.sector = spt-1;

        while((*ln = ptol(d, &pi, 0)) < 0) {
            pi.sector--;
            ddbg_assert(pi.sector >= 0);
        }

        ddbg_assert(*ln < d->dm_sectors);
    }

    if (l0 && ln) {
        ddbg_assert(*l0 <= *ln);
    }

    /* Dushyanth: check error here; see comment above about setting l0, ln */
    if(lbn < 0) {
        return lbn;
    }

    return DM_OK;
}


dm_angle_t
g4_sector_width(struct dm_disk_if *d,
        struct dm_pbn *track,
        int num)
{
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    // int spt = g4_spt_pbn(d, track);
    dm_angle_t result;
    struct remap *r;

    if((r = remap_pbn(l, track))) {
        result = r->sw;
    }
    else {
        struct g4_path *p;
        struct g4_path_node *n;
        p = g4_recurse(l, 0, track);

        if(!p) {
            // DM_NX
            result = 0;
        }
        else {
            n = &p->path[p->length - 1];
            ddbg_assert(n->type == TRACK);
            result = n->n.t->sw;
            free(p);
            p = NULL;
        }
    }

    return result;
}


// Compute the starting offset of a pbn relative to 0. 
dm_angle_t
g4_skew(struct dm_disk_if *d,
        struct dm_pbn *pbn)
{
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    struct g4_path *p;
    struct g4_path_node *n;
    int i;
    dm_angle_t result;

    p = g4_recurse(l, 0, pbn);
    if(!p) {
        // DM_NX
        return 0;
    }

    for(i = 0; i < p->length; ++i) {
        n = &p->path[i];
        switch(n->type) {
        case TRACK:
            result += n->resid * n->n.t->sw;
            break;

        case IDX:
        {
            struct idx_ent *e = &n->n.i->ents[n->i];

            if(i > 0) {
                result += e->off;
            }
            else {
                result = e->off;
            }

            result += n->quot * e->alen;

        } break;

        default: ddbg_assert(0); break;
        }
    }

    free(p);
    p = NULL;
    return result;
}


// convert from an angle to a pbn
// returns a ptol_result since provided angle could be in slipped
// space, etc.  Rounds angle down to a sector starting offset
// Takes the angle in 0l.
dm_ptol_result_t
g4_atop(struct dm_disk_if *d,
        struct dm_mech_state *m,
        struct dm_pbn *p)
{

    dm_angle_t sw;
    p->cyl = m->cyl;
    p->head = m->head;
    p->sector = 0; // XXX

    sw = g4_sector_width(d, p, 1);
    p->sector = m->theta / sw;

    // p may be in unmapped space
    return d->layout->dm_translate_ptol(d, p, 0);
}

dm_ptol_result_t
g4_defect_count(struct dm_disk_if *d, 
        struct dm_pbn *track,
        int *result)
{
    // XXX
    *result = 0;
    return 0;
}


// XXX These zone apis are totally fake!  We're going to map zones to
// root ents.

// returns the number of zones for the layout
int 
g4_get_numzones(struct dm_disk_if *d) 
{
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    return l->root->ents_len;
}


int
g4_get_zone(struct dm_disk_if *d, int n, struct dm_layout_zone *z)
{
    struct dm_layout_g4 *l = (struct dm_layout_g4 *)d->layout;
    struct idx_ent *e;


    if(n >= l->root->ents_len) {
        return -1;
    }

    e = &l->root->ents[n];

    z->spt = g4_spt_lbn(d, e->lbn);
    z->lbn_low = e->lbn;
    z->lbn_high = min(d->dm_sectors, e->lbn + e->runlen);
    z->cyl_low = e->cyl;
    z->cyl_high = min(d->dm_cyls, e->cyl + e->cylrunlen);


    return 0;
}


struct dm_layout_if layout_g4 = {
#ifdef WIN32
        ltop,
        0,
        ptol,
        0,
        g4_spt_lbn,
        g4_spt_pbn,
        g4_track_bound,
        0,
        g4_skew,
        0,
        0,
        g4_atop,
        g4_sector_width,
        0,
        0,
        0,
        g4_get_numzones,
        g4_get_zone,
        g4_defect_count
#else
        .dm_translate_ltop = ltop,
        .dm_translate_ptol = ptol,

        .dm_get_sectors_lbn = g4_spt_lbn,
        .dm_get_sectors_pbn = g4_spt_pbn,

        .dm_get_track_boundaries = g4_track_bound,

        .dm_pbn_skew = g4_skew,

        .dm_convert_atop = g4_atop,
        .dm_get_sector_width = g4_sector_width,

        .dm_defect_count = g4_defect_count,
        .dm_get_numzones = g4_get_numzones,
        .dm_get_zone = g4_get_zone
#endif
};

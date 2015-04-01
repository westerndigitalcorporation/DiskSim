#include "libddbg.h"
#include "libparam\libparam.h"
#include "diskmodel\layout_g4.h"

///*
//struct dm_pbn {
//  int cyl;
//  int head;
//  int sector;
//};
//*/
//void dump_dm_pbn( struct dm_pbn *pbn, char *name )
//{
//    printf( "\n\ndm_pbn: %s", name );
//    printf( "\n  cyl   : %d",  pbn->cyl );
//    printf( "\n  head  : %d",  pbn->head );
//    printf( "\n  sector: %d\n",  pbn->sector );
//}
//
//
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
//
//void dump_g4_layout( struct dm_layout_g4 * g4l, char *name )
//{
//    struct idx *pIdx;
//    struct idx_ent *pEnts;
//    int i, j;
//
//    printf( "\n\nModel G4 Format: %s", name );
//    printf( "\n  Track Pattern format:" );
//    for( i = 0; i < g4l->track_len; i++ )
//    {
//        printf( "\n    %2d: low %d, high %d, spt %d, sw %d", i, g4l->track[i].low, g4l->track[i].high, g4l->track[i].spt, g4l->track[i].sw );
//    }
//
//    printf( "\n  Index Format format:" );
//    for( j = 0; j < g4l->idx_len; j ++ )
//    {
//        pIdx = &g4l->idx[j];
//        printf( "\n    Index Section: %d", j );
//        for( i = 0; i < pIdx->ents_len; i++ )
//        {
//            pEnts = &pIdx->ents[i];
//            if( TRACK == pEnts->childtype )
//            {
//                printf( "\n      TRACK: lbn %d, cyl %d, off %d, len %d, cyllen %d, alen %d, runlen %d, cylrunlen %d, childtype %d, head %d"
//                    , pEnts->lbn, pEnts->cyl, pEnts->off, pEnts->len, pEnts->cyllen, pEnts->alen, pEnts->runlen, pEnts->cylrunlen, pEnts->childtype, pEnts->head );
//                printf( "\n           low %d, high %d, spt %d, sw %d", pEnts->child.t->low, pEnts->child.t->high, pEnts->child.t->spt, pEnts->child.t->sw );
//            }
//            else
//            {
//                printf( "\n      INDEX: lbn %d, cyl %d, off %d, len %d, cyllen %d, alen %d, runlen %d, cylrunlen %d, childtype %d"
//                    , pEnts->lbn, pEnts->cyl, pEnts->off, pEnts->len, pEnts->cyllen, pEnts->alen, pEnts->runlen, pEnts->cylrunlen, pEnts->childtype );
//                printf( "\n         TRACK: lbn %d, cyl %d, off %d, len %d, cyllen %d, alen %d, runlen %d, cylrunlen %d, childtype %d, head %d"
//                    , pEnts->lbn, pEnts->child.i->ents->cyl, pEnts->child.i->ents->off, pEnts->child.i->ents->len, pEnts->child.i->ents->cyllen,pEnts->child.i->ents->alen,pEnts->child.i->ents->runlen, pEnts->child.i->ents->cylrunlen, pEnts->child.i->ents->childtype,  pEnts->child.i->ents->head );
//            }
//        }
//    }
//    printf( "\n\n" );
//    fflush( stdout );
//}

// End of file

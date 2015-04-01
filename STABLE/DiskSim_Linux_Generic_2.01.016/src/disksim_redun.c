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
#include "disksim_stat.h"
#include "disksim_iosim.h"
#include "disksim_orgface.h"
#include "disksim_logorg.h"
#include "disksim_ioqueue.h"


static int logorg_modulus_update (int inc, int val, int maxval)
{
   int ret;

   ret = val + inc;
   if (ret < 0) {
      ret += maxval;
   } else if (ret >= maxval) {
      ret -= maxval;
   }
   return(ret);
}


int logorg_tabular_rottype (int maptype, int reduntype, int rottype, int stripeunit)
{
   if (reduntype != PARITY_ROTATED) {
      return(FALSE);
   }
   if (maptype != STRIPED) {
      return(FALSE);
   }
   if (stripeunit == 0) {
      return(FALSE);
   }
   switch (rottype) {
      case PARITY_LEFT_SYM:
      case PARITY_LEFT_ASYM:
      case PARITY_RIGHT_SYM:
      case PARITY_RIGHT_ASYM:
      return(TRUE);
   }
   return(FALSE);
}


static void logorg_parity_rotate_ideal (logorg *currlogorg, ioreq_event *curr)
{
   if (curr->flags & READ) {
      return;
   }
   curr->next = ioreq_copy(curr);
   curr->next->devno = currlogorg->idealno;
   currlogorg->idealno = (currlogorg->idealno + 1) % currlogorg->numdisks;
}


static void logorg_parity_rotate_random (logorg *currlogorg, ioreq_event *curr)
{
   if (curr->flags & READ) {
      return;
   }
   curr->next = ioreq_copy(curr);
   while (curr->next->devno == curr->devno) {
      curr->next->devno = (int)((double)currlogorg->numdisks * DISKSIM_drand48());
   }
}


static int logorg_shadowed_get_short_dist (logorg *currlogorg, ioreq_event *curr, int numtocheck, int *checklist)
{
   int i, j;
   int dist;
   int shortdev = -1;
   int shortdist = 999999999;
   int ties[MAXCOPIES];
   int no;

   j = -1;
   for (i = 0; i < numtocheck; i++) {
      no = checklist[i];
      dist = ioqueue_get_dist(currlogorg->devs[(curr->devno + (no * currlogorg->numdisks))].queue, (curr->blkno + currlogorg->devs[(curr->devno + (no * currlogorg->numdisks))].startblkno));
      if (dist >= 999999999) {
	 fprintf(stderr, "Haven't allowed for large enough 'dist's in logorg_shadowed_get_short_dist - %d\n", dist);
	 exit(1);
      }
      if (dist == shortdist) {
	 j++;
	 if (j >= MAXCOPIES) {
	    fprintf(stderr, "Haven't allowed for enough ties in logorg_shadowed_get_short_dist - %d\n", j);
	    exit(1);
	 }
	 ties[j] = no;
      }
      if (dist < shortdist) {
	 shortdist = dist;
	 shortdev = no;
	 j = -1;
      }
   }
   if (shortdev == -1) {
      fprintf(stderr, "Illegal condition in logorg_shadowed_get_short_dist\n");
      exit(1);
   } else if (j != -1) {
      i = (int) (DISKSIM_drand48() * (double) (j+2));
      if (i != 0) {
	 shortdev = ties[(i-1)];
      }
   }
   return(shortdev);
}


static int logorg_shadowed_get_short_queue (logorg *currlogorg, ioreq_event *curr, int def)
{
   int i, j;
   int len;
   int shortdev = -1;
   int shortlen = 999999999;
   int ties[MAXCOPIES];

   j = -1;
   for (i = 0; i < currlogorg->copies; i++) {
      len = ioqueue_get_number_in_queue(currlogorg->devs[(curr->devno + (i * currlogorg->numdisks))].queue);
      if (len >= 999999999) {
	 fprintf(stderr, "Haven't allowed for large enough 'dist's in logorg_shadowed_get_short_queue - %d\n", len);
	 exit(1);
      }
      if (len == shortlen) {
	 j++;
	 if (j >= MAXCOPIES) {
	    fprintf(stderr, "Haven't allowed for enough ties in logorg_shadowed_get_short_queue - %d\n", j);
	    exit(1);
	 }
	 ties[j] = i;
      }
      if (len < shortlen) {
	 shortlen = len;
	 shortdev = i;
	 j = -1;
      }
   }
   if (shortdev == -1) {
      fprintf(stderr, "Illegal condition in logorg_shadowed_get_short_queue\n");
      exit(1);
   } else if (j != -1) {
      if (def == 1) {
         i = (int) (DISKSIM_drand48() * (double) (j+2));
         if (i != 0) {
	    shortdev = ties[(i-1)];
         }
      } else if (def == 2) {
	 j++;
	 ties[j] = shortdev;
	 shortdev = logorg_shadowed_get_short_dist(currlogorg, curr, (j+1), ties);
      } else {
	 fprintf(stderr, "Unknown default tie breaker in logorg_shadowed_get_short_queue - %d\n", def);
	 exit(1);
      }
   }
   return(shortdev);
}


int logorg_shadowed (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int i, j;
   ioreq_event *temp;
   ioreq_event *newreq;
   int checklist[MAXCOPIES];

   temp = curr;
   if (curr->flags & READ) {
      for (i = 0; i < numreqs; i++) {
	 switch (currlogorg->copychoice) {
	    case SHADOW_PRIMARY:
/* May want to move head on secondary device */
			  break;
	    case SHADOW_RANDOM:
			  temp->devno += currlogorg->numdisks * (int) (DISKSIM_drand48() * (double) currlogorg->copies);
			  break;
	    case SHADOW_ROUNDROBIN:
			  temp->devno += currlogorg->numdisks * currlogorg->reduntoggle;
			  currlogorg->reduntoggle++;
			  currlogorg->reduntoggle %= currlogorg->copies;
			  break;
	    case SHADOW_SHORTDIST:
			  for (j = 0; j < currlogorg->copies; j++)
			     checklist[j] = j;
			  temp->devno += currlogorg->numdisks * logorg_shadowed_get_short_dist(currlogorg, temp, currlogorg->copies, checklist);
			  break;
	    case SHADOW_SHORTQUEUE:
			  temp->devno += currlogorg->numdisks * logorg_shadowed_get_short_queue(currlogorg, temp, 1);
			  break;
	    case SHADOW_SHORTQUEUE2:
			  temp->devno += currlogorg->numdisks * logorg_shadowed_get_short_queue(currlogorg, temp, 2);
			  break;
	    default:
		    fprintf(stderr, "Unknown shadow choice type at logorg_shadowed\n");
		    exit(1);
	 }
	 temp = temp->next;
      }
      return(numreqs);
   } else {
      for (i = 0; i < numreqs; i++) {
         for (j = 1; j < currlogorg->copies; j++) {
            newreq = ioreq_copy(temp);
            temp->next = newreq;
            newreq->devno += currlogorg->numdisks;
            temp = newreq;
         }
	 temp = temp->next;
      }
      return(currlogorg->copies * numreqs);
   }
}


int logorg_parity_disk (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   depends *depend;

   if (curr->flags & READ) {
      return(numreqs);
   }
   if ((currlogorg->maptype == STRIPED) && (currlogorg->stripeunit == 0)) {
      curr->next = ioreq_copy(curr);
      curr->next->devno = currlogorg->numdisks;
      return(numreqs+1);
   }
   if (numreqs != 1) {
      fprintf(stderr, "Too many reqs at logorg_parity_disk - %d\n", numreqs);
      exit(1);
   }
   depend = (depends *) getfromextraq();
   depend->blkno = curr->blkno;
   depend->devno = curr->devno;
   depend->numdeps = 2;
   depend->deps[0] = ioreq_copy(curr);
   depend->deps[1] = ioreq_copy(curr);
   depend->deps[0]->opid = 2;
   depend->deps[0]->devno = currlogorg->numdisks;
   depend->next = (depends *) getfromextraq();
   depend->next->next = NULL;
   depend->next->blkno = curr->blkno;
   depend->next->devno = currlogorg->numdisks;
   depend->next->deps[0] = depend->deps[0];
   depend->next->deps[1] = depend->deps[1];
   if (currlogorg->writesync == TRUE) {
      depend->deps[1]->opid = 2;
      depend->next->numdeps = 2;
   } else {
      depend->deps[1]->opid = 1;
      depend->next->numdeps = 1;
   }
   curr->flags |= READ;
   curr->next = ioreq_copy(curr);
   curr->next->devno = currlogorg->numdisks;
   curr->prev = (ioreq_event *) depend;
   return(4);
}


static int logorg_parity_rotate_updates (logorg *currlogorg, ioreq_event *curr, int inc)
{
   int reqs = 2;
   int parityno;
   int blksleft;
   int bcount;
   ioreq_event *newreq;
   int parityunit;
   int numdisks;
   int devno;
   int parityblock;

   parityunit = currlogorg->parityunit;
   numdisks = currlogorg->numdisks;

   parityblock = curr->blkno / (parityunit * (numdisks - 1));
   parityno = (curr->blkno % (parityunit * (numdisks - 1))) / parityunit;
   curr->blkno += parityblock * parityunit;
   if (inc == -1) {
      devno = numdisks - (parityno % numdisks) - 1;
   } else {
      devno = parityno % numdisks;
   }
   if (((inc == 1) && (curr->devno <= devno)) || ((inc == -1) && (curr->devno >=devno))) {
      curr->blkno += parityunit;
      devno = logorg_modulus_update(inc, devno, numdisks);
      parityno++;
   }
   blksleft = parityunit - (curr->blkno % currlogorg->parityunit);
   bcount = curr->bcount;
   if ((parityunit != currlogorg->parityunit) && ((bcount > blksleft) || (parityunit > currlogorg->parityunit))) {
      fprintf(stderr, "Failed integrity check in logorg_parity_rotate_left\n");
      exit(1);
   }
   while (bcount > blksleft) {
      if (!(curr->flags & READ)) {
         newreq = ioreq_copy(curr);
         newreq->devno = devno;
         newreq->bcount = blksleft;
         newreq->blkno = curr->blkno + curr->bcount - bcount;
         newreq->next = curr->next;
         curr->next = newreq;
         reqs++;
      }
      bcount -= blksleft;
      devno = logorg_modulus_update(inc, devno, numdisks);
      if (devno == curr->devno) {
	 curr->bcount += parityunit;
         devno = logorg_modulus_update(inc, devno, numdisks);
	 parityno++;
      }
      parityno++;
      blksleft = parityunit;
   }
   if (curr->flags & READ) {
      return(1);
   }
   newreq = ioreq_copy(curr);
   newreq->devno = devno;
   newreq->blkno = curr->blkno + curr->bcount - bcount;
   newreq->bcount = bcount;
   newreq->next = curr->next;
   curr->next = newreq;
   if (parityno > currlogorg->numfull) {
      fprintf(stderr, "Integrity check failure in logorg_parity_rotate_left\n");
      exit(1);
   }
   return(reqs);
}


static void logorg_parity_read_old_sync (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int i, j;
   depends *depend;
   depends *tmpdep;
   ioreq_event *temp;
   int devno = 1;

   depend = (depends *) getfromextraq();
   depend->blkno = curr->blkno;
   depend->devno = curr->devno;
   depend->deps[0] = ioreq_copy(curr);
   depend->deps[0]->opid = numreqs;
   curr->prev = (ioreq_event *) depend;
   depend->numdeps = numreqs;
   temp = curr->next;
   for (i=1; i<numreqs; i++) {
      if (devno == 0) {
	 depend->cont = (depends *) getfromextraq();
	 depend = depend->cont;
      }
      depend->deps[devno] = ioreq_copy(temp);
      depend->deps[devno]->opid = numreqs;
      devno = logorg_modulus_update(1, devno, 10);
      temp = temp->next;
   }
   temp = curr->next;
   curr->flags |= READ;
   for (i=1; i<numreqs; i++) {
      depend = (depends *) ioreq_copy(curr->prev);
      depend->next = (depends *) curr->prev;
      curr->prev = (ioreq_event *) depend;
      depend->blkno = temp->blkno;
      depend->devno = temp->devno;
      tmpdep = depend->cont;
      for (j=0; j<(numreqs/10); j++) {
	 depend->cont = (depends *) ioreq_copy((ioreq_event *) tmpdep);
	 tmpdep = tmpdep->cont;
	 depend = depend->cont;
      }
      temp->flags |= READ;
      temp = temp->next;
   }
}


static void logorg_parity_read_old_nosync (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int i;
   depends *depend;
   depends *tmpdep;
   ioreq_event *temp;
   int devno = 1;

   depend = (depends *) getfromextraq();
   depend->blkno = curr->blkno;
   depend->devno = curr->devno;
   depend->numdeps = numreqs;
   depend->next = NULL;
   depend->deps[0] = ioreq_copy(curr);
   depend->deps[0]->opid = 1;
   curr->prev = (ioreq_event *) depend;
   curr->flags |= READ;
   temp = curr->next;
   for (i=1; i<numreqs; i++) {
      if (devno == 0) {
	 depend->cont = (depends *) getfromextraq();
	 depend = depend->cont;
      }
      tmpdep = (depends *) getfromextraq();
      tmpdep->blkno = temp->blkno;
      tmpdep->devno = temp->devno;
      tmpdep->numdeps = 1;
      tmpdep->next = (depends *) curr->prev;
      curr->prev = (ioreq_event *) tmpdep;
      tmpdep->deps[0] = ioreq_copy(temp);
      tmpdep->deps[0]->opid = 2;
      depend->deps[devno] = tmpdep->deps[0];
      temp->flags |= READ;
      devno = logorg_modulus_update(1, devno, 10);
      temp = temp->next;
   }
}


int logorg_parity_rotate (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int reqs = 2;

   if (currlogorg->maptype == IDEAL) {
      logorg_parity_rotate_ideal(currlogorg, curr);
   } else if (currlogorg->maptype == RANDOM) {
      logorg_parity_rotate_random(currlogorg, curr);
   } else if (currlogorg->maptype == ASIS) {
      switch (currlogorg->rottype) {

	 case PARITY_LEFT_SYM:
	 case PARITY_LEFT_ASYM:
            reqs = logorg_parity_rotate_updates(currlogorg, curr, -1);
	    break;

	 case PARITY_RIGHT_SYM:
	 case PARITY_RIGHT_ASYM:
            reqs = logorg_parity_rotate_updates(currlogorg, curr, 1);
	    break;

	 default:
	    fprintf(stderr, "Unknown rottype at logorg_parity_rotate - %d\n", currlogorg->rottype);
	    exit(1);
      }
   } else {
      fprintf(stderr, "Unknown maptype at logorg_parity_rotate - %d\n", currlogorg->maptype);
      exit(1);
   }
   if (curr->flags & READ) {
      return(1);
   }

   if (currlogorg->writesync == TRUE) {
      logorg_parity_read_old_sync(currlogorg, curr, reqs);
   } else {
      logorg_parity_read_old_nosync(currlogorg, curr, reqs);
   }

   return(2*reqs);
}


static int logorg_join_seqreqs (ioreq_event *reqlist, ioreq_event *curr, int seqgive)
{
   ioreq_event *temp;
   ioreq_event *del;
   int numreqs = 0;
   int distance;

   temp = reqlist;
   if (temp) {
      while (temp->next) {
	 distance = temp->next->blkno - temp->blkno - temp->bcount;
/*
fprintf (outputfile, "In logorg_join_seqreqs, devno %d, blkno %d, bcount %d, read %d, distance %d\n", temp->devno, temp->blkno, temp->bcount, (temp->flags & READ), distance);
*/
         if (distance < 0) {
            fprintf(stderr, "Integrity check failure at logorg_join_seqreqs - blkno %d, bcount %d, blkno %d, read %d\n", temp->blkno, temp->bcount, temp->next->blkno, (temp->flags & READ));
            exit(1);
         }
         if (((temp->flags & READ) == (temp->next->flags & READ)) && (distance <= seqgive)) {
            del = temp->next;
            temp->next = del->next;
            temp->bcount += del->bcount + distance;
	    temp->opid |= del->opid;
            addtoextraq((event *) del);
         } else {
            del = temp;
            temp = del->next;
            del->next = curr->next;
            curr->next = del;
	    del->time = curr->time;
	    del->buf = curr->buf;
            numreqs++;
         }
      }
      numreqs++;
      temp->next = curr->next;
      curr->next = temp;
      temp->time = curr->time;
      temp->buf = curr->buf;
   }
   return(numreqs);
}


static void logorg_parity_table_insert (ioreq_event **head, ioreq_event *curr)
{
   ioreq_event *temp;

   if (((*head) == NULL) || (curr->blkno < (*head)->blkno)) {
      curr->next = (*head);
      (*head) = curr;
   } else {
      temp = (*head);
      while ((temp->next) && (curr->blkno >= temp->next->blkno)) {
         temp = temp->next;
      }
      curr->next = temp->next;
      temp->next = curr;
   }
}


static void logorg_parity_table_read_old (logorg *currlogorg, ioreq_event *rowhead, ioreq_event **reqlist, int opid)
{
   ioreq_event *temp;
   ioreq_event *newreq;
   ioreq_event *prev = NULL;

   temp = rowhead;
   while (temp) {
      newreq = (ioreq_event *) getfromextraq();
      newreq->blkno = temp->blkno;
      newreq->devno = temp->devno;
      newreq->bcount = temp->bcount;
      newreq->flags = temp->flags | READ;
      newreq->opid = opid;
      logorg_parity_table_insert(&reqlist[newreq->devno], newreq);
      newreq->prev = prev;
      prev = newreq;
	/* only the last write (the parity update) must depend on the reads */
      if (temp->prev == NULL) {
         temp->opid = opid;
      }
      temp = temp->prev;
   }
}


static void logorg_parity_table_recon (logorg *currlogorg, ioreq_event *rowhead, ioreq_event **reqlist, int stripeno, int unitno, int tableadd, int opid)
{
   ioreq_event *temp;
   ioreq_event *newreq;
   int entryno;
   int minblkno;
   int maxblkno;
   int lastentry;
   int i;
   int blkno;
   int offset;

   temp = rowhead;
   if (temp == NULL) {
      fprintf(stderr, "Can't have NULL rowhead at logorg_parity_table_recon\n");
      exit(1);
   }
   lastentry = (stripeno + 1) * currlogorg->partsperstripe + stripeno;
   while (temp->prev) {
      temp = temp->prev;
   }
   temp->opid = opid;
   minblkno = temp->blkno - currlogorg->table[lastentry].blkno - tableadd;
   maxblkno = minblkno + temp->bcount;
   entryno = stripeno * currlogorg->partsperstripe + stripeno;
   for (i = 0; i < unitno; i++) {
      newreq = (ioreq_event *) getfromextraq();
      newreq->devno = currlogorg->table[entryno].devno;
      newreq->blkno = tableadd + minblkno + currlogorg->table[entryno].blkno;
      newreq->bcount = maxblkno - minblkno;
      newreq->flags = rowhead->flags | READ;
      newreq->opid = opid;
      logorg_parity_table_insert(&reqlist[newreq->devno], newreq);
      entryno++;
   }
   temp = rowhead;
   while (temp->prev) {
      blkno = currlogorg->table[entryno].blkno;
      offset = temp->blkno - tableadd - blkno;
      if (offset > minblkno) {
         newreq = (ioreq_event *) getfromextraq();
         newreq->devno = temp->devno;
         newreq->blkno = tableadd + blkno + minblkno;
         newreq->bcount = offset - minblkno;
         newreq->flags = temp->flags | READ;
	 newreq->opid = opid;
         logorg_parity_table_insert(&reqlist[newreq->devno], newreq);
      }
      if ((offset + temp->bcount) < maxblkno) {
         newreq = (ioreq_event *) getfromextraq();
         newreq->devno = temp->devno;
         newreq->blkno = temp->blkno + temp->bcount;
         newreq->bcount = maxblkno - offset - temp->bcount;
         newreq->flags = temp->flags | READ;
	 newreq->opid = opid;
         logorg_parity_table_insert(&reqlist[newreq->devno], newreq);
      }
      entryno++;
      temp = temp->prev;
   }
   for (; entryno < lastentry; entryno++) {
      newreq = (ioreq_event *) getfromextraq();
      newreq->devno = currlogorg->table[entryno].devno;
      newreq->blkno = tableadd + minblkno + currlogorg->table[entryno].blkno;
      newreq->bcount = maxblkno - minblkno;
      newreq->flags = rowhead->flags | READ;
      newreq->opid = opid;
      logorg_parity_table_insert(&reqlist[newreq->devno], newreq);
   }
}


static void logorg_parity_table_dodeps_sync (logorg *currlogorg, ioreq_event *curr, ioreq_event **redunlist, ioreq_event **reqlist)
{
   int i, j;
   depends *depend;
   depends *tmpdep;
   int opid = 0;
   int numdeps = 0;
   ioreq_event *temp;
   ioreq_event *deplist = NULL;
   int devno = 1;

   for (i=0; i<currlogorg->actualnumdisks; i++) {
      temp = redunlist[i];
      while (temp) {
	 temp->opid = curr->opid;
	 opid++;
	 if (temp->next == NULL) {
	    temp->next = curr->next;
	    curr->next = redunlist[i];
	    break;
	 }
	 temp = temp->next;
      }
      temp = reqlist[i];
      while (temp) {
	 numdeps++;
	 if (temp->next == NULL) {
	    temp->next = deplist;
	    deplist = reqlist[i];
	    break;
	 }
	 temp = temp->next;
      }
   }
   if (numdeps <= 0) {
      fprintf(stderr, "Can't have zero requests at logorg_parity_table_dodeps_sync\n");
      exit(1);
   }
   if (opid == 0) {
      curr->prev = NULL;
      curr->next = deplist;
      return;
   }
   depend = (depends *) getfromextraq();
   depend->blkno = curr->next->blkno;
   depend->devno = curr->next->devno;
   depend->numdeps = numdeps;
   curr->prev = (ioreq_event *) depend;
   depend->deps[0] = deplist;
   deplist->opid = opid;
   temp = deplist->next;
   for (i=1; i<numdeps; i++) {
      if (devno == 0) {
	 depend->cont = (depends *) getfromextraq();
	 depend = depend->cont;
      }
      depend->deps[devno] = temp;
      temp->opid = opid;
      devno = logorg_modulus_update(1, devno, 10);
      temp = temp->next;
   }
   temp = curr->next->next;
   for (i=1; i<opid; i++) {
      depend = (depends *) ioreq_copy(curr->prev);
      depend->next = (depends *) curr->prev;
      curr->prev = (ioreq_event *) depend;
      depend->blkno = temp->blkno;
      depend->devno = temp->devno;
      tmpdep = depend->cont;
      for (j=0; j<(numdeps/10); j++) {
	 depend->cont = (depends *) ioreq_copy((ioreq_event *) tmpdep);
	 tmpdep = tmpdep->cont;
	 depend = depend->cont;
      }
      temp = temp->next;
   }
}


static void logorg_parity_table_dodeps_nosync (logorg *currlogorg, ioreq_event *curr, ioreq_event **redunlist, ioreq_event **reqlist)
{
   int i;
   depends *depend;
   depends *tmpdep;
   ioreq_event *temp;
   ioreq_event *del;
   ioreq_event *tmpread;
   ioreq_event *readlist = NULL;
   ioreq_event *deplist = NULL;
   int holddep;

   for (i=0; i<currlogorg->actualnumdisks; i++) {
      temp = redunlist[i];
      while (temp) {
         depend = (depends *) getfromextraq();
         depend->blkno = temp->blkno;
         depend->devno = temp->devno;
         depend->numdeps = 0;
	 depend->next = NULL;
	 temp->prev = (ioreq_event *) depend;
	 if (temp->next == NULL) {
	    temp->next = readlist;
	    readlist = redunlist[i];
	    break;
	 }
	 temp = temp->next;
      }
      temp = reqlist[i];
      while (temp) {
	 del = temp;
	 temp = temp->next;
	 if ((redunlist[i] == NULL) && (del->opid == 0)) {
	    del->next = curr->next;
	    curr->next = del;
	 } else {
	    del->next = deplist;
	    deplist = del;
	 }
      }
   }
   if (readlist == NULL) {
      if (deplist) {
         fprintf(stderr, "Unacceptable condition in logorg_parity_table_dodeps_nosync\n");
         exit(1);
      }
      curr->prev = NULL;
      return;
   }
   temp = deplist;
   while (temp) {
      holddep = temp->opid;
      temp->opid = 0;
      tmpread = readlist;
      while (tmpread) {
	 if ((tmpread->devno == temp->devno) | (holddep & tmpread->opid)) {
            tmpdep = (depends *) tmpread->prev;
            ASSERT (tmpdep != NULL);
            if (tmpdep->numdeps > 0) {
	       for (i=0; i<((tmpdep->numdeps-1)/10); i++) {
	          tmpdep = tmpdep->cont;
	       }
	       if ((tmpdep->numdeps % 10) == 0) {
	          tmpdep->cont = (depends *) getfromextraq();
	          tmpdep = tmpdep->cont;
	       }
            }
            tmpdep->deps[(tmpdep->numdeps % 10)] = temp;
	    tmpdep->numdeps++;
            temp->opid++;
	 }
	 tmpread = tmpread->next;
      }
      temp = temp->next;
   }
   temp = readlist;
   curr->prev = NULL;
   while (temp) {
      depend = (depends *) temp->prev;
      temp->prev = NULL;
      depend->next = (depends *) curr->prev;
      curr->prev = (ioreq_event *) depend;
      if (temp->next == NULL) {
	 temp->next = curr->next;
	 curr->next = readlist;
	 break;
      }
      temp = temp->next;
   }
}


int logorg_parity_table (logorg *currlogorg, ioreq_event *curr, int numreqs)
{
   int i;
   ioreq_event *reqs[MAXDEVICES];
   ioreq_event *redunreqs[MAXDEVICES];
   int reqcnt = 0;
   ioreq_event *lastrow = NULL;
   int stripeunit;
   ioreq_event *temp;
   ioreq_event *newreq;
   int unitno;
   int stripeno;
   int entryno;
   int blkno;
   int blksinpart;
   int reqsize;
   int partsperstripe;
   int rowcnt = 1;
   int firstrow = 0;
   int opid = 0x1;
   int blkscovered;
   tableentry *table;
   int tablestart;
   int preventryno;
/*
fprintf (outputfile, "Entered logorg_parity_table - devno %d, blkno %d, bcount %d, read %d\n", curr->devno, curr->blkno, curr->bcount, (curr->flags & READ));
*/
   if (numreqs != 1) {
      fprintf(stderr, "Multiple numreqs at logorg_parity_table is not acceptable - %d\n", numreqs);
      exit(1);
   }

   stripeunit = currlogorg->stripeunit;
   partsperstripe = currlogorg->partsperstripe;
   table = currlogorg->table;
   if (currlogorg->addrbyparts) {
      curr->blkno += curr->devno * currlogorg->blksperpart;
   }
   for (i=0; i<currlogorg->actualnumdisks; i++) {
      reqs[i] = NULL;
   }
   unitno = curr->blkno / stripeunit;
   stripeno = unitno / partsperstripe;
   tablestart = (stripeno / currlogorg->tablestripes) * currlogorg->tablesize;
   stripeno = stripeno % currlogorg->tablestripes;
   blkno = tablestart + table[(stripeno*(partsperstripe+1))].blkno;
   if (blkno == currlogorg->numfull) {
      stripeunit = currlogorg->actualblksperpart - blkno;
      curr->blkno -= blkno;
      unitno = curr->blkno / stripeunit;
   }
   blksinpart = stripeunit;
   unitno = unitno % partsperstripe;
   curr->blkno = curr->blkno % stripeunit;
   reqsize = curr->bcount;
   entryno = stripeno * partsperstripe + stripeno + unitno;
   blkno = tablestart + table[entryno].blkno;
   blksinpart -= curr->blkno;
   temp = ioreq_copy(curr);
   curr->next = temp;
   temp->blkno = blkno + curr->blkno;
   temp->devno = table[entryno].devno;
   temp->opid = 0;
   blkscovered = curr->bcount;
   curr->blkno = stripeno;
   curr->devno = unitno;
   curr->bcount = tablestart;
   reqs[temp->devno] = curr->next;
   temp->next = NULL;
   temp->prev = NULL;
   if (reqsize > blksinpart) {
      temp->bcount = blksinpart;
      blkscovered = blksinpart;
      reqsize -= blksinpart;
      unitno++;
      if (unitno == partsperstripe) {
	 if (!(curr->flags & READ)) {
	    newreq = (ioreq_event *) getfromextraq();
	    newreq->devno = table[(entryno+1)].devno;
	    newreq->blkno = table[(entryno+1)].blkno;
	    newreq->blkno += tablestart + temp->blkno - blkno;
	    newreq->bcount = blkscovered;
	    newreq->flags = curr->flags;
            newreq->opid = 0;
	    reqs[newreq->devno] = newreq;
	    newreq->next = NULL;
	    temp->prev = newreq;
	    newreq->prev = NULL;
	 }
	 blkscovered = 0;
	 unitno = 0;
	 if (firstrow == 0) {
	    firstrow = 1;
	 }
	 rowcnt = 0;
	 stripeno++;
	 temp = NULL;
	 if (stripeno == currlogorg->tablestripes) {
	    stripeno = 0;
	    tablestart += currlogorg->tablesize;
	 }
      }
      entryno = (stripeno * partsperstripe) + stripeno + unitno;
      blkno = tablestart + table[entryno].blkno;
      blksinpart = (blkno != currlogorg->numfull) ? stripeunit : currlogorg->actualblksperpart - blkno;
      while (reqsize > blksinpart) {
	 rowcnt++;
	 newreq = (ioreq_event *) getfromextraq();
         newreq->blkno = blkno;
	 newreq->devno = table[entryno].devno;
	 newreq->bcount = blksinpart;
	 blkscovered = max(blkscovered, blksinpart);
	 newreq->flags = curr->flags;
         newreq->opid = 0;
	 newreq->prev = NULL;
	 if (temp) {
	    temp->prev = newreq;
	 } else {
	    lastrow = newreq;
	 }
	 temp = newreq;
	 logorg_parity_table_insert(&reqs[temp->devno], temp);
         reqsize -= blksinpart;
         unitno++;
         if (unitno == partsperstripe) {
	    if (!(curr->flags & READ)) {
	       newreq = (ioreq_event *) getfromextraq();
	       newreq->devno = table[(entryno+1)].devno;
	       newreq->blkno = table[(entryno+1)].blkno + tablestart;
	       newreq->bcount = blkscovered;
	       newreq->flags = curr->flags;
               newreq->opid = 0;
               temp->prev = newreq;
	       newreq->prev = NULL;
	       logorg_parity_table_insert(&reqs[newreq->devno], newreq);
	    }
	    blkscovered = 0;
	    unitno = 0;
	    if (firstrow == 0) {
	       firstrow = rowcnt;
	    }
	    rowcnt = 0;
	    temp = NULL;
	    stripeno++;
	    if (stripeno == currlogorg->tablestripes) {
	       stripeno = 0;
	       tablestart += currlogorg->tablesize;
	    }
         }
         entryno = (stripeno * partsperstripe) + stripeno + unitno;
         blkno = tablestart + table[entryno].blkno;
         blksinpart = (blkno != currlogorg->numfull) ? stripeunit : currlogorg->actualblksperpart - blkno;
      }
      newreq = (ioreq_event *) getfromextraq();
      newreq->blkno = blkno;
      newreq->devno = table[entryno].devno;
      newreq->bcount = reqsize;
      rowcnt++;
      blkscovered = max(blkscovered, blksinpart);
      newreq->flags = curr->flags;
      newreq->opid = 0;
      newreq->prev = NULL;
      if (temp) {
	 temp->prev = newreq;
      } else {
	 lastrow = newreq;
      }
      temp = newreq;
      logorg_parity_table_insert(&reqs[newreq->devno], newreq);
   }
   if (curr->flags & READ) {
      curr->next = curr;
      for (i=0; i<currlogorg->actualnumdisks; i++) {
	 reqcnt += logorg_join_seqreqs(reqs[i], curr, LOGORG_PARITY_SEQGIVE);
      }
   } else {
      preventryno = entryno;
      entryno = (stripeno * partsperstripe) + stripeno + partsperstripe;
      newreq = (ioreq_event *) getfromextraq();
      newreq->devno = table[entryno].devno;
      newreq->blkno = table[entryno].blkno + temp->blkno - table[preventryno].blkno;
      newreq->bcount = (rowcnt == 1) ? temp->bcount : blkscovered;
      newreq->flags = curr->flags;
      newreq->opid = 0;
      temp->prev = newreq;
      newreq->prev = NULL;
      logorg_parity_table_insert(&reqs[newreq->devno], newreq);
      if (firstrow == 0) {
	 if ((rowcnt == 2) && ((curr->next->blkno - temp->blkno - temp->bcount) > 0)) {
	    newreq->bcount = temp->bcount;
	    newreq = (ioreq_event *) getfromextraq();
	    newreq->devno = temp->prev->devno;
	    lastrow = temp;
	    temp = curr->next;
	    newreq->blkno = table[entryno].blkno + temp->blkno - table[(preventryno-1)].blkno;
	    newreq->bcount = temp->bcount;
	    newreq->flags = curr->flags;
	    newreq->opid = 0;
	    temp->prev = newreq;
	    newreq->prev = NULL;
            logorg_parity_table_insert(&reqs[newreq->devno], newreq);
	    firstrow = 1;
	    rowcnt = 1;
	 } else {
	    firstrow = rowcnt;
	    rowcnt = 0;
	 }
      }
      for (i=0; i<currlogorg->actualnumdisks; i++) {
	 redunreqs[i] = NULL;
      }
      if (firstrow < partsperstripe) {
	 if (firstrow < currlogorg->rmwpoint) {
	    logorg_parity_table_read_old(currlogorg, curr->next, redunreqs, opid);
	 } else {
	    logorg_parity_table_recon(currlogorg, curr->next, redunreqs, curr->blkno, curr->devno, curr->bcount, opid);
	 }
	 opid = opid << 1;
      }
      if ((rowcnt) && (rowcnt != partsperstripe)) {
	 if (rowcnt < currlogorg->rmwpoint) {
	    logorg_parity_table_read_old(currlogorg, lastrow, redunreqs, opid);
	 } else {
	    logorg_parity_table_recon(currlogorg, lastrow, redunreqs, stripeno, 0, tablestart, opid);
	 }
      }
      for (i=0; i<currlogorg->actualnumdisks; i++) {
	 curr->next = NULL;
	 reqcnt += logorg_join_seqreqs(redunreqs[i], curr, LOGORG_PARITY_SEQGIVE);
	 redunreqs[i] = curr->next;
	 curr->next = NULL;
	 reqcnt += logorg_join_seqreqs(reqs[i], curr, LOGORG_PARITY_SEQGIVE);
	 reqs[i] = curr->next;
      }
      curr->next = curr;
      if (currlogorg->writesync) {
	 logorg_parity_table_dodeps_sync(currlogorg, curr, redunreqs, reqs);
      } else {
	 logorg_parity_table_dodeps_nosync(currlogorg, curr, redunreqs, reqs);
      }
   }
   if (curr->next) {
      temp = curr->next;
      curr->blkno = temp->blkno;
      curr->devno = temp->devno;
      curr->bcount = temp->bcount;
      curr->flags = temp->flags;
      curr->next = temp->next;
      addtoextraq((event *) temp);
   } else {
      fprintf(stderr, "Seem to have no requests when leaving logorg_parity_table\n");
      exit(1);
   }
/*
fprintf (outputfile, "Exiting logorg_parity_table - reqcnt %d\n", reqcnt);
*/
   return(reqcnt);
}


static void logorg_create_table_left_sym (logorg *currlogorg)
{
   int i, j;
   int numdisks;
   int stripeno = 0;
   int devno = 0;
   int blkno = 0;
   int parityno;

   numdisks = currlogorg->numdisks;
   parityno = numdisks - 1;
   for (i=0; i<currlogorg->tablestripes; i++) {
      for (j=1; j<numdisks; j++) {
         currlogorg->table[stripeno].devno = devno;
         currlogorg->table[stripeno].blkno = blkno;
         stripeno++;
	 devno = logorg_modulus_update(1, devno, numdisks);
         if (devno == parityno) {
	    devno = logorg_modulus_update(1, devno, numdisks);
         }
      }
      currlogorg->table[stripeno].devno = parityno;
      currlogorg->table[stripeno].blkno = blkno;
      blkno += currlogorg->stripeunit;
      stripeno++;
      if ((blkno % currlogorg->parityunit) == 0) {
         devno = parityno;
         parityno = logorg_modulus_update(-1, parityno, numdisks);
      }
   }
}


void logorg_create_table (logorg *currlogorg)
{
   int i;
   int numdisks;
   int parityunit;
   int stripeunit;

   numdisks = currlogorg->numdisks;
   parityunit = currlogorg->parityunit;
   stripeunit = currlogorg->stripeunit;
   if (currlogorg->reduntype != PARITY_DISK) {
      if ((parityunit < stripeunit) || ((parityunit % stripeunit) != 0)) {
         fprintf(stderr, "Illegal parityunit - stripeunit combination at logorg_create_table\n");
         exit(1);
      }
   }
   if (currlogorg->reduntype == PARITY_DISK) {
      currlogorg->tablestripes = 1;
   } else {
      currlogorg->tablestripes = numdisks * parityunit / stripeunit;
   }
   currlogorg->table = (tableentry *)DISKSIM_malloc(currlogorg->tablestripes * numdisks * sizeof(tableentry));
   currlogorg->tablesize = currlogorg->tablestripes * stripeunit;
   currlogorg->partsperstripe = numdisks - 1;
   if (currlogorg->reduntype == PARITY_DISK) {
      for (i=0; i<numdisks; i++) {
         currlogorg->table[i].devno = i;
         currlogorg->table[i].blkno = 0;
      }
      return;
   }
   switch (currlogorg->rottype) {
      case PARITY_LEFT_SYM:
	      logorg_create_table_left_sym(currlogorg);
	      break;
      case PARITY_LEFT_ASYM:
      case PARITY_RIGHT_SYM:
      case PARITY_RIGHT_ASYM:
      default:
	      fprintf(stderr, "Unknown parity rotation type at logorg_create_table\n");
	      exit(1);
   }
/*
   for (i=0; i<(currlogorg->tablestripes * numdisks); i++) {
      fprintf (outputfile, "tableentry #%d: devno %d, blkno %d\n", i, currlogorg->table[i].devno, currlogorg->table[i].blkno);
   }
*/
}


int logorg_check_dependencies (logorg *currlogorg, outstand *req, ioreq_event *curr)
{
   int i;
   depends *tmpdep;
   depends *del = NULL;
   int numreqs = 0;
   int devno = 0;
   int numdeps;
   ioreq_event *temp;

   tmpdep = req->depend;
   if (tmpdep == NULL) {
      return(0);
   }
   if ((tmpdep->devno == curr->devno) && (tmpdep->blkno == curr->blkno)) {
      req->depend = tmpdep->next;
   } else {
      while (tmpdep->next) {
         if ((tmpdep->next->devno == curr->devno) && (tmpdep->next->blkno == curr->blkno)) {
            del = tmpdep->next;
            tmpdep->next = del->next;
            tmpdep = del;
            break;
         }
         tmpdep = tmpdep->next;
      }
      if (del == NULL) {
         return(0);
      }
   }
   curr->next = curr;
   numdeps = tmpdep->numdeps;
   i = 0;
   while (i < numdeps) {
      temp = tmpdep->deps[devno];
      temp->opid--;
      if (temp->opid == 0) {
         numreqs++;
         temp->next = curr->next;
         curr->next = temp;
         temp->opid = req->opid;
         temp->time = 0.0;
         if (req->flags & TIME_CRITICAL) {
            temp->flags |= TIME_CRITICAL;
         }
      }
      if ((i++) >= numdeps) {
	 break;
      }
      devno = logorg_modulus_update(1, devno, 10);
      if (devno == 0) {
         del = tmpdep;
         tmpdep = tmpdep->cont;
         addtoextraq((event *) del);
      }
   }
   addtoextraq((event *) tmpdep);
   return(numreqs);
}


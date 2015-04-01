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

#ifndef HPTRACE_H
#define HPTRACE_H

/* The trace record structure, type and flag value definitions were */
/* derived from HPLabs's SRTlite code base.                         */
/*
###########################################################################
#
# SRTlite Copyright (C) 1994 Hewlett-Packard Company, all rights reserved.
#
# Permission to use, copy, and modify this software and its
# documentation is hereby granted without fee, provided that the above
# copyright notice appear in all copies and that both that copyright
# notice and this permission notice appear in supporting documentation,
# and that the name of Hewlett-Packard Company not be used in
# advertising or publicity pertaining the software without specific,
# written prior permission.  Permission to distribute this software
# requires specific, written prior permission.
# 
# Hewlett-Packard Company makes no representations about the suitability
# of this software for any purpose. It is provided "as is" without
# express or implied warranty.
#
# HEWLETT-PACKARD COMPANY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS, IN NO EVENT SHALL HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
# SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
###########################################################################
*/

/* HPLabs Trace Record Types */

#define HPL_UNKNOWN		0
#define HPL_SHORTIO		1
#define HPL_LONGIO		2
#define HPL_TEXTIO		3
#define HPL_SUSPECTIO		4
#define HPL_BREADFS		5
#define HPL_BREADAFS		6
#define HPL_BREADAAFS		7
#define HPL_BDWRITEFS		8
#define HPL_BWRITEFS		9
#define HPL_CREATE_OPENFS	10

/*
File tr_buf.h from HPL TraceAccess code
// These buffer types are taken from HP-UX vers 8.0.  In future versions, the 
// actual bit positions of these types may change (as did in 9.0 when B_SYNC
// became 0x00800000.  So these are what the SRT files use as the flags.
// The only trick comes in converting KI files to srt files and knowing that 
// the B_SYNC in the particular version of HP-UX that was traced needs to map
// to this value of B_SYNC.

// The HPL values match one for one with the values in HP-UX 8.0.  Those B_
// values that have a 9 after them are the changes from 8.0 to 9.0 values,
// so these need to be converted back to 8.0 values before processing ki
// traces.
*/

#define	HPL_WRITE	  0x00000000	/* non-read pseudo-flag */
#define	HPL_READ	  0x00000001	/* read when I/O occurs */
#define	HPL_DONE	  0x00000002	/* transaction finished */
#define	HPL_ERROR	  0x00000004	/* transaction aborted */
#define	HPL_BUSY	  0x00000008	/* not on av_forw/back list */

#define	HPL_PHYS	  0x00000010	/* physical IO */
#define HPL_END_OF_DATA	  0x00000020
#define	HPL_WANTED        0x00000040	/* issue wakeup when BUSY goes off */
#define	HPL_NDELAY        0x00000080	/* don't retry on failures */

#define	HPL_ASYNC	  0x00000100	/* don't wait for I/O completion */
#define	HPL_DELWRITE      0x00000200	/* write at exit of avail list */
#define HPL_ORDWRI        0x00000400    /* flags ordered asynchronous writes */
#define HPL_REWRITE       0x00000800    /* re-writing buffer - call bdwrite */

#define HPL_PRIVATE       0x00001000    /* private, not part of buffers - LVM */
#define HPL_WRITEV        0x00002000    /* verification of writes  - LVM */
#define HPL_PFTIMEOUT     0x00004000    /* power failure time out - LVM */
#define HPL_CACHE         0x00008000    /* did bread find us in the cache ? */

#define	HPL_INVAL	  0x00010000	/* does not contain valid info  */
#define	HPL_LOCKED        0x00020000	/* locked in core (not reusable) */
#define	HPL_HEAD	  0x00040000	/* a buffer header, not a buffer */
#define HPL_FSYSIO        0x00080000	/* buffer from b{read,write}	*/

#define	HPL_BAD	          0x00100000	/* bad block revectoring in progress */
#define	HPL_CALL	  0x00200000	/* call b_iodone from iodone */
#define HPL_DUX_REM_REQ   0x00400000    /* Buffer is for a remote dux reqest */
#define	HPL_NOCACHE       0x00800000	/* don't cache block when released */

#define HPL_SYNC	  0x01000000	/* buffer write is synchronous */
#define HPL_RAW           0x02000000    /* raw interface (LVM/disc3/autoch) */
#define HPL_NETBUF        0x04000000    /* buffer in network pool */

typedef struct {
   u_int32_t size;    /* record size */
   u_int32_t id;      /* record type */
   u_int32_t sec;     /* time of     */
   u_int32_t usec;    /*       event */
} hpl_record_id;

typedef struct {
   hpl_record_id   rec_id;
   u_int32_t       start;      /* Start time of I/O in usec from enqueue time */
   u_int32_t       stop;       /* Stop time of I/O in usec from enqueue time */
   u_int32_t       size;       /* size in bytes */
   u_int32_t       sectno;     /* sector number on disk addressed */
   u_int32_t       devno;      /* device addressed */
   u_int32_t       drivertype; /* type of driver used by this disk */
   u_int32_t       cylno;      /* cylinder number of the access */
   u_int32_t       ioflags;    /* I/O flags for the device */
   u_int32_t       info;       /* information about where this I/O originated */
   u_int32_t       queuelen;   /* length of the queue */
   u_int32_t       susflags;   /* flags indicating error (for suspectIO) */
   char space[4];
} hpl_short_iorec_ki;

#endif /* HPTRACE_H */


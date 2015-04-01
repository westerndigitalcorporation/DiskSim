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
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */


#include "disksim_global.h"
#include "disksim_rand48.h"
#include <stdio.h>


/* FreeBSD's rand48.h */

#define	RAND48_SEED_0	(0x330e)
#define	RAND48_SEED_1	(0xabcd)
#define	RAND48_SEED_2	(0x1234)
#define	RAND48_MULT_0	(0xe66d)
#define	RAND48_MULT_1	(0xdeec)
#define	RAND48_MULT_2	(0x0005)
#define	RAND48_ADD	(0x000b)


/* stuff added to restrict global variables to the disksim structure */
typedef struct rand48_info {
   unsigned short _rand48_seed[3];
   unsigned short _rand48_mult[3];
   unsigned short _rand48_add;
} rand48_into_t;


/* #defines to remap the rand48_info variables to their original names */
#define _rand48_seed   (disksim->rand48_info->_rand48_seed)
#define _rand48_mult   (disksim->rand48_info->_rand48_mult)
#define _rand48_add    (disksim->rand48_info->_rand48_add)


static void disksim_rand48_initialize ()
{
   if (disksim->rand48_info != NULL) {
      return;
   }

   disksim->rand48_info = (struct rand48_info *)DISKSIM_malloc (sizeof(rand48_into_t));
   bzero ((char *)disksim->rand48_info, sizeof(rand48_into_t));

   /* initialization */
   _rand48_seed[0] = RAND48_SEED_0;
   _rand48_seed[1] = RAND48_SEED_1;
   _rand48_seed[2] = RAND48_SEED_2;
   _rand48_mult[0] = RAND48_MULT_0;
   _rand48_mult[1] = RAND48_MULT_1;
   _rand48_mult[2] = RAND48_MULT_2;
   _rand48_add     = RAND48_ADD;
}


/* FreeBSD's _rand48.c */

#if 0    /* replaced by rand48_info */
static unsigned short _rand48_seed[3] = {
	RAND48_SEED_0,
	RAND48_SEED_1,
	RAND48_SEED_2
};
static unsigned short _rand48_mult[3] = {
	RAND48_MULT_0,
	RAND48_MULT_1,
	RAND48_MULT_2
};
static unsigned short _rand48_add = RAND48_ADD;
#endif


static void
DISKSIM__dorand48(unsigned short xseed[3])
{
	unsigned long accu;
	unsigned short temp[2];

	disksim_rand48_initialize();

	accu = (unsigned long) _rand48_mult[0] * (unsigned long) xseed[0] +
	 (unsigned long) _rand48_add;
	temp[0] = (unsigned short) accu;	/* lower 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += (unsigned long) _rand48_mult[0] * (unsigned long) xseed[1] +
	 (unsigned long) _rand48_mult[1] * (unsigned long) xseed[0];
	temp[1] = (unsigned short) accu;	/* middle 16 bits */
	accu >>= sizeof(unsigned short) * 8;
	accu += _rand48_mult[0] * xseed[2] + _rand48_mult[1] * xseed[1] + _rand48_mult[2] * xseed[0];
	xseed[0] = temp[0];
	xseed[1] = temp[1];
	xseed[2] = (unsigned short) accu;
}


/* Made-up ldexp, based on FreeBSD manpage spec.  We have no particular    */
/* desire to make this completely robust -- it is only used for erand48(). */

/* This function should return a double that is "x" times 2 raised to */
/* the "exp" power.                                                   */

static double
DISKSIM_ldexp (double x, int exp)
{
   double start = 1.0;
   int iters = (exp > 0) ? exp : -exp;
   int i;

   disksim_rand48_initialize();

   /* hard-core optimization */
   if (iters == 16) {
      start = (exp > 0) ? (double)65536 : (double)0.0000152587890625 ;
      iters = 0;
   } else if (exp == -32) {
      start = (double)0.00000000023283064365386962890625 ;
      //start = (double)((exp > 0) ? (double)4294967296 : (double)0.00000000023283064365386962890625 );
      iters = 0;
   } else if (exp == -48) {
      start = (double)0.000000000000003552713678800500929355621337890625 ;
      //start = (exp > 0) ? (double)281474976710656 : (double)0.000000000000003552713678800500929355621337890625 ;
      iters = 0;
   }

   /* lesser optimization (basically loop-unrolling) */
   while (iters >= 8) {
      start *= (exp > 0) ? 256 : 0.00390625 ;
      iters -= 8;
   }

   /* clean-up */
   for (i=0; i<iters; i++) {
      start *= (exp > 0) ? (double)2.0 : (double) 0.5;
   }

   return (x * start);
}


/* FreeBSD's erand48.c */

static double
DISKSIM_erand48(unsigned short xseed[3])
{
	disksim_rand48_initialize();
	DISKSIM__dorand48(xseed);
	return DISKSIM_ldexp((double) xseed[0], -48) +
	       DISKSIM_ldexp((double) xseed[1], -32) +
	       DISKSIM_ldexp((double) xseed[2], -16);
}


/* FreeBSD's drand48.c */

double
DISKSIM_drand48(void)
{
  disksim_rand48_initialize(); 
  return DISKSIM_erand48(_rand48_seed); 
  //  return (double)0.5; 
}


/* FreeBSD's jrand48.c */

long
DISKSIM_jrand48(unsigned short xseed[3])
{
	disksim_rand48_initialize();
	DISKSIM__dorand48(xseed);
	return ((long) xseed[2] << 16) + (long) xseed[1];
}


/* FreeBSD's lrand48.c */

long
DISKSIM_lrand48(void)
{
  	disksim_rand48_initialize(); 
  	DISKSIM__dorand48(_rand48_seed); 
		return ((long) _rand48_seed[2] << 15) + ((long) _rand48_seed[1] >> 1); 
    // return 10; 
}


/* FreeBSD's mrand48.c */

long
DISKSIM_mrand48(void)
{
	disksim_rand48_initialize();
	DISKSIM__dorand48(_rand48_seed);
	return ((long) _rand48_seed[2] << 16) + (long) _rand48_seed[1];
}


/* FreeBSD's nrand48.c */

long
DISKSIM_nrand48(unsigned short xseed[3])
{
	disksim_rand48_initialize();
	DISKSIM__dorand48(xseed);
	return ((long) xseed[2] << 15) + ((long) xseed[1] >> 1);
}


/* FreeBSD's srand48.c */

void
DISKSIM_srand48(long seed)
{
	disksim_rand48_initialize();
	_rand48_seed[0] = RAND48_SEED_0;
	_rand48_seed[1] = (unsigned short) seed;
	_rand48_seed[2] = (unsigned short) (seed >> 16);
	_rand48_mult[0] = RAND48_MULT_0;
	_rand48_mult[1] = RAND48_MULT_1;
	_rand48_mult[2] = RAND48_MULT_2;
	_rand48_add = RAND48_ADD;
}


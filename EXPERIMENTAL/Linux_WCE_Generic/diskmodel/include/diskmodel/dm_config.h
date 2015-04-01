
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



#ifndef _DM_CONFIG_H
#define _DM_CONFIG_H

#ifdef _DM_SOURCE
#include "dm_types.h"
#else
#include <diskmodel/dm_types.h>
#endif

#include <libddbg/libddbg.h>


// undef this to turn off inlining
#define INLINE inline

// freebsd foo
#ifdef _DISKMODEL_FREEBSD
#define _KERNEL
#include <sys/types.h>
#include <sys/malloc.h>
#define malloc(x) malloc((x), M_DEVBUF, M_WAITOK)
#else
#include <stdlib.h>
#endif  // _DISKMODEL_FREEBSD

#include <string.h> // for strerror

uint32_t sqrt32(uint32_t);
uint64_t sqrt64(uint64_t);

#endif   /*  _DM_CONFIG_H */








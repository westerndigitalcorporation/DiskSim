/*
 * DIXtrac - Automated Disk Extraction Tool
 * Authors: Jiri Schindler, John Bucy, Greg Ganger
 *
 * Copyright (c) of Carnegie Mellon University, 1999-2005.
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

#include "dixtrac.h"
#include "scsi_conv.h"

__inline void
_lto2b(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = (val >> 8) & 0xff;
	bytes[1] = val & 0xff;
}

__inline void
_lto3b(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = (val >> 16) & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = val & 0xff;
}

__inline void
_lto4b(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = (val >> 24) & 0xff;
	bytes[1] = (val >> 16) & 0xff;
	bytes[2] = (val >> 8) & 0xff;
	bytes[3] = val & 0xff;
}

__inline u_int32_t
_2btol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = (bytes[0] << 8) |
	     bytes[1];
	return (rv);
}

__inline u_int32_t
_3btol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = (bytes[0] << 16) |
	     (bytes[1] << 8) |
	     bytes[2];
	return (rv);
}

__inline u_int32_t
_4btol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = (bytes[0] << 24) |
	     (bytes[1] << 16) |
	     (bytes[2] << 8) |
	     bytes[3];
	return (rv);
}

__inline void
_lto2l(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
}

__inline void
_lto3l(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = (val >> 16) & 0xff;
}

__inline void
_lto4l(u_int32_t val, u_int8_t *bytes)
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = (val >> 16) & 0xff;
	bytes[3] = (val >> 24) & 0xff;
}

__inline u_int32_t
_2ltol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8);
	return (rv);
}

__inline u_int32_t
_3ltol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16);
	return (rv);
}

__inline u_int32_t
_4ltol(u_int8_t *bytes)
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16) |
	     (bytes[3] << 24);
	return (rv);
}

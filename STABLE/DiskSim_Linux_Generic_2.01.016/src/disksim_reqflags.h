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

#ifndef _DISKSIM_REQFLAGS_H
#define _DISKSIM_REQFLAGS_H

#define DISKSIM_READ_WRITE_MASK     0x00000001
#define DISKSIM_WRITE               0x00000000
#define DISKSIM_READ                0x00000001

#define DISKSIM_TIME_CRITICAL       0x00000002
#define DISKSIM_TIME_LIMITED        0x00000004
#define DISKSIM_TIMED_OUT           0x00000008
#define DISKSIM_HALF_OUT            0x00000010
#define DISKSIM_MAPPED              0x00000020
#define DISKSIM_READ_AFTR_WRITE     0x00000040
#define DISKSIM_SYNC                0x00000080
#define DISKSIM_ASYNC               0x00000100
#define DISKSIM_IO_FLAG_PAGEIO      0x00000200
#define BUFFER_BACKGROUND           0x10000000
#define DISKSIM_LOCAL               0x20000000
#define DISKSIM_SEQ                 0x40000000
#define DISKSIM_BATCH_COMPLETE      0x80000000

#endif // _DISKSIM_REQFLAGS_H

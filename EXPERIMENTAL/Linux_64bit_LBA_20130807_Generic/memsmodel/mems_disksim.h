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


/* mems_disksim.h
 *
 * Functions integrating the memsdevice simulator with disksim.  All of
 * these are called inside disksim_device.h.
 */

#ifndef MEMS_DISKSIM_H
#define MEMS_DISKSIM_H

extern void mems_event_arrive (ioreq_event *curr);
extern void mems_bus_delay_complete (int devno, ioreq_event *curr, int sentbusno);
extern void mems_bus_ownership_grant (int devno, ioreq_event *curr, int busno, double arbdelay);

void   mems_cleanstats (void);
double mems_get_acctime (int devno, ioreq_event *req, double maxtime);
int    mems_get_avg_sectpercyl (int devno);
double mems_get_blktranstime (ioreq_event *curr);
int    mems_get_busno (ioreq_event *curr);
int    mems_get_depth (int devno);
int    mems_get_inbus (int devno);
void   mems_get_mapping (int maptype, int devno, int blkno, int *cylptr, int *surfaceptr, int *blkptr);
int    mems_get_maxoutstanding (int devno);
int    mems_get_number_of_blocks (int devno);
int    mems_get_numcyls (int devno);
double mems_get_seektime (int devno, ioreq_event *req, int checkcache, double maxtime);
double mems_get_servtime (int devno, ioreq_event *req, int checkcache, double maxtime);
int    mems_get_slotno (int devno);
void   mems_initialize (void);
void   mems_param_override (char *paramname, char *paramval, int first, int last);
void   mems_printsetstats (int *set, int setsize, char *sourcestr);
void   mems_printstats (void);
void   mems_read_specs (FILE *parfile, int devno, int copies);
void   mems_read_toprints (FILE *parfile);
void   mems_resetstats (void);
int    mems_set_depth (int devno, int inbusno, int depth, int slotno);
int    mems_get_distance (int devno, ioreq_event *req, int exact, int direction);

/* not implemented (but easily done if necessary):
 *
 * double mems_get_acctime (int devno, ioreq_event *req, double maxtime);
 * void   mems_set_syncset (int setstart, int setend);
 * void   mems_setcallbacks (void);
 */

/* default mems dev header */
extern struct device_header mems_hdr_initializer;

#endif  /* MEMS_DISKSIM_H */

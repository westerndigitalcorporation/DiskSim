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


#ifndef DISKSIM_DEVICE_H
#define DISKSIM_DEVICE_H

#include "disksim_iosim.h"



typedef enum {
  DEVICETYPE_DISK       = 1,
  DEVICETYPE_SIMPLEDISK	= 2,
  DEVICETYPE_MEMS       = 3,
  DEVICETYPE_SSD        = 4,

  // this is something of a hack so that things like synthgen can
  // refer to a logorg by name.  All we care here is that it has a
  // name and an underlying device number.
} device_type_t;
#define DEVICETYPE_MIN DEVICETYPE_DISK
#define DEVICETYPE_MAX DEVICETYPE_MEMS


typedef struct device_info {
   int numdevices;

   int *devicetypes;	/* actual type of device;
			 * this will go away with new device iface 
			 */

   int *devicenos;	/* number among devices of that device type */
  
   int maxdeviceno[DEVICETYPE_MAX];	/* maximum deviceno of each
                                         * devicetype -- may already be
					 * defunct */

   char **devicenames;  /* pointers to name fields in actual
                         * device objects */

   struct device_header **devices;

   int devs_len;        /* allocated size of above arrays */

   int device_printqueuestats;
   int device_printcritstats;
   int device_printidlestats;
   int device_printintarrstats;
   int device_printsizestats;
   int device_printseekstats;
   int device_printlatencystats;
   int device_printxferstats;
   int device_printacctimestats;
   int device_printinterferestats;
   int device_printbufferstats;
} device_info_t;


struct device_header;

/*  struct device_header { */
/*    int device_type; */
/*    int device_len; */
/*    char *device_name; */
/*    struct device_header(*dev_copy)(struct device_header *); */
/*  }; */


/* remapping #defines for some variables in device_info_t */
#define device_printqueuestats      (disksim->deviceinfo->device_printqueuestats)
#define device_printcritstats       (disksim->deviceinfo->device_printcritstats)
#define device_printidlestats       (disksim->deviceinfo->device_printidlestats)
#define device_printintarrstats     (disksim->deviceinfo->device_printintarrstats)
#define device_printsizestats       (disksim->deviceinfo->device_printsizestats)
#define device_printseekstats       (disksim->deviceinfo->device_printseekstats)
#define device_printlatencystats    (disksim->deviceinfo->device_printlatencystats)
#define device_printxferstats       (disksim->deviceinfo->device_printxferstats)
#define device_printacctimestats    (disksim->deviceinfo->device_printacctimestats)
#define device_printinterferestats  (disksim->deviceinfo->device_printinterferestats)
#define device_printbufferstats     (disksim->deviceinfo->device_printbufferstats)


/* disksim_device.c functions */

/* device-wide stuff */
void    device_setcallbacks (void);
void    device_initialize (void);

void    device_resetstats (void);
void    device_printstats (void);
void    device_printsetstats (int *set, int setsize, char *sourcestr);
void    device_cleanstats (void);
int     device_get_numdevices (void);

void device_add(struct device_header *d, int);


struct device_header *getdevbyname(char *name, 
				   int *gdevnum, /* unique for all
                                                    devices */
				   int *ldevnum, /* unique for all
                                                    devices of same
                                                    type */
				   int *type);

/* map a global device number to a device-type specific device number */
INLINE int dev_map_devno(int n);

void device_initialize_deviceinfo(void);


/* device-specific stuff ... moving into device_hdr */


struct device_header {
  int device_type;
  int device_len;  // used? necessary?
  char *device_name;
  struct device_header*(*dev_copy)(struct device_header *);

  int     (*set_depth)(int diskno, int inbusno, int depth, int slotno);
  int     (*get_depth)(int diskno);
  int     (*get_inbus)(int diskno);
  int     (*get_busno)(ioreq_event *curr);
  int     (*get_slotno)(int diskno);


  int     (*get_number_of_blocks)(int diskno);
  int     (*get_maxoutstanding)(int diskno);
  int     (*get_numcyls)(int diskno);
  double  (*get_blktranstime)(ioreq_event *curr);
  int     (*get_avg_sectpercyl)(int devno);
  
  void    (*get_mapping)(int maptype, 
			 int diskno, 
			 int blkno, 
			 int *cylptr, 
			 int *surfaceptr, 
			 int *blkptr);
  
  void    (*event_arrive)(ioreq_event *curr);
  
  int     (*get_distance)(int diskno, 
			  ioreq_event *req, 
			  int exact, 
			  int direction);
  
  double  (*get_servtime)(int diskno, 
			  ioreq_event *req, 
			  int checkcache, 
			  double maxtime);


  double  (*get_seektime)(int diskno, 
			  ioreq_event *req, 
			  int checkcache, 
			  double maxtime);
  
  double  (*get_acctime)(int diskno, 
			 ioreq_event *req, 
			 double maxtime);
  
  void    (*bus_delay_complete)(int devno, 
				ioreq_event *curr, 
				int sentbusno);
  
  void    (*bus_ownership_grant)(int devno, 
				 ioreq_event *curr, 
				 int busno, 
				 double arbdelay);
  
};


int     device_set_depth (int diskno, int inbusno, int depth, int slotno);
int     device_get_depth (int diskno);
int     device_get_inbus (int diskno);
int     device_get_busno (ioreq_event *curr);
int     device_get_slotno (int diskno);


int     device_get_number_of_blocks (int diskno);
int     device_get_maxoutstanding (int diskno);
int     device_get_numcyls (int diskno);
double  device_get_blktranstime (ioreq_event *curr);
int     device_get_avg_sectpercyl (int devno);

void    device_get_mapping (int maptype, 
			    int diskno, 
			    int blkno, 
			    int *cylptr, 
			    int *surfaceptr, 
			    int *blkptr);

void    device_event_arrive (ioreq_event *curr);

int     device_get_distance (int diskno, 
			     ioreq_event *req, 
			     int exact, 
			     int direction);

double  device_get_servtime (int diskno, 
			     ioreq_event *req, 
			     int checkcache, 
			     double maxtime);


double  device_get_seektime (int diskno, 
			     ioreq_event *req, 
			     int checkcache, 
			     double maxtime);

double  device_get_acctime (int diskno, 
			    ioreq_event *req, 
			    double maxtime);

void    device_bus_delay_complete (int devno, 
				   ioreq_event *curr, 
				   int sentbusno);

void    device_bus_ownership_grant (int devno, 
				    ioreq_event *curr, 
				    int busno, 
				    double arbdelay);




#endif   /* DISKSIM_DEVICE_H */








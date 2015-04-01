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


#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_device.h"
#include "disksim_disk.h"
#include "disksim_simpledisk.h"
//#include "memsmodel/mems_global.h"
//#include "memsmodel/mems_disksim.h"
//#include "ssdmodel/ssd.h"  /* SSD: */
#include "config.h"

#include "modules/modules.h"


/* This remaps device numbers amongst multiple types of devices (each */
/* of which keeps internal arrays based on the remapped numbers). So, */
/* the device implementations need to be careful to call back to get  */
/* the remapping info...                                              */

/* To avoid the corresponding confusion, we will simply remap devnos  */
/* to themselves for now.  So, each device implementation should      */
/* allocate MAXDEVICES structures and track which of them are used.   */


/* private remapping #defines for variables from device_info_t */
#define numdevices                  (disksim->deviceinfo->numdevices)
	/* per-devno device type */
#define devicetypes                 (disksim->deviceinfo->devicetypes)
	/* per-devno device number (for when remapping them) */
#define devicenos                   (disksim->deviceinfo->devicenos)
	/* number of devices of each type */
#define maxdeviceno                 (disksim->deviceinfo->maxdeviceno)


struct device_header *
getdevbyname(char *name,
        int *gdevnum, /* global device number */
        int *ldevnum, /* type-specific device number */
        int *type)    /* device type */
{
  int c;
  for(c = 0; c < disksim->deviceinfo->devs_len; c++) {
    if(!disksim->deviceinfo->devicenames[c]) continue;
    if(!strcmp(name, disksim->deviceinfo->devicenames[c])) {

      if(gdevnum) *gdevnum = c;
      if(ldevnum) *ldevnum = devicenos[c];
      if(type)    *type = devicetypes[c];
      switch(devicetypes[c]) {
        case DEVICETYPE_DISK:
          return (struct device_header *)getdisk(devicenos[c]);
          break;
        case DEVICETYPE_SIMPLEDISK:
          return (struct device_header *)getsimpledisk(devicenos[c]);
          break;
//        case DEVICETYPE_MEMS:
//          return (struct device_header *)getmems(devicenos[c]);
//          break;
/*
        case DEVICETYPE_SSD:  SSD:
          return (struct device_header *)getssd( (int)(devicenos[c]) );
          break;
*/
      }
    }
  }
  return 0;
}

void device_initialize_deviceinfo (void)
{
   if (disksim->deviceinfo == NULL) {
    disksim->deviceinfo = (struct device_info *)calloc (1, sizeof(device_info_t));
   }
}

void device_add(struct device_header *d, int ldevno) {
  int c, newlen;
  int zerocnt;
  device_initialize_deviceinfo();
  
  for(c = 0; c < disksim->deviceinfo->devs_len; c++) {
    if(!disksim->deviceinfo->devicenames[c]) {
      goto foundslot;
    }
  }

  /* note that numdisks must be equal to diskinfo->disks_len */
  newlen = numdevices ? (2 * numdevices) : 2;
  zerocnt = (newlen == 2) ? 2 : (newlen/2);
  disksim->deviceinfo->devicenames = 
    (char **)realloc(disksim->deviceinfo->devicenames, newlen * sizeof(char *));
  bzero(&(disksim->deviceinfo->devicenames[c]), zerocnt * sizeof(char *));

  devicenos = (int *)realloc(devicenos, newlen*sizeof(int));
  bzero(&(devicenos[c]), zerocnt * sizeof(int));

  devicetypes = (int *)realloc(devicetypes, newlen*sizeof(int));
  bzero(&(devicetypes[c]), zerocnt * sizeof(int));

  disksim->deviceinfo->devices = (struct device_header **)realloc(disksim->deviceinfo->devices,
					 newlen*sizeof(void*));
  bzero(&(disksim->deviceinfo->devices[c]), zerocnt * sizeof(void*));

  disksim->deviceinfo->devs_len = newlen;

 foundslot:
  disksim->deviceinfo->devicenames[c] = d->device_name;
  devicetypes[c] = d->device_type;
  devicenos[c] = ldevno;
  if(maxdeviceno[devicetypes[c]] < devicenos[c]) {
    maxdeviceno[devicetypes[c]] = devicenos[c];
  }
  disksim->deviceinfo->devices[c] = d;

  numdevices++;
}



int disksim_device_stats_loadparams(struct lp_block *b) {

  device_initialize_deviceinfo();
    
/*    unparse_block(b, outputfile); */

  //#include "modules/disksim_device_stats_param.c"
  lp_loadparams(0, b, &disksim_device_stats_mod);

  /* none of the devices currently have dev-specific stats so we aren't
   * going to look for them here for now */
  
  return 1;
}





void device_setcallbacks (void)
{
   /* call for each type of device */
   disk_setcallbacks ();
   simpledisk_setcallbacks ();
   /* mems checkpointing not supported */
   fprintf(stderr,"Warning: mems checkpointing not supported (yet)!\n");
}


void device_initialize (void)
{
   /* call for each type of device */
  disk_initialize ();
  simpledisk_initialize ();
//  mems_initialize ();
//  ssd_initialize ();
}

void device_resetstats (void)
{
   /* call for each type of device */
   disk_resetstats ();
   simpledisk_resetstats ();
//   mems_resetstats ();
//   ssd_resetstats ();
}


void device_printstats (void)
{
   /* call for each type of device */
   disk_printstats ();
   simpledisk_printstats ();
//   mems_printstats ();
//   ssd_printstats ();
}


void device_printsetstats (int *set, int setsize, char *sourcestr)
{
   int i;
   int devicetype = devicetypes[set[0]];

   /* verify that all set members are of same device type */
   for (i=0; i<setsize; i++) {
      if (devicetypes[set[i]] != devicetype) {
         // fprintf(stderr, "Can't have mismatching devicetypes in device_printsetstats (%d != %d)\n", devicetypes[set[i]], devicetype);
         /* Might want to just return (and put this message in the output */
         /* file) rather than exiting...                                  */
         // exit(1);
         return;
      }
   }

   /* call the appropriate one */
   switch(devicetype) {
   case DEVICETYPE_DISK:
      disk_printsetstats (set, setsize, sourcestr);
      break;
   case DEVICETYPE_SIMPLEDISK:
      simpledisk_printsetstats (set, setsize, sourcestr);
      break;
//   case DEVICETYPE_MEMS:
//      mems_printsetstats (set, setsize, sourcestr);
//      break;
//   case DEVICETYPE_SSD:
//      ssd_printsetstats (set, setsize, sourcestr);
      break;
   default:
      fprintf(stderr, "Unknown value for device type: devicetype %d\n", devicetype);
      assert(0);
      break;
   }
}


void device_cleanstats (void)
{
   /* call for each type of device */
   disk_cleanstats ();
   simpledisk_cleanstats ();
//   mems_cleanstats ();
}


INLINE int device_set_depth (int devno, int inbusno, int depth, int slotno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->set_depth(devicenos[devno], inbusno, depth, slotno);
}


INLINE int device_get_depth (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_depth(devno);
}


INLINE int device_get_inbus (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_inbus(devno);
}


INLINE int device_get_busno (ioreq_event *curr)
{
   ASSERT1 ((curr->devno >= 0) && (curr->devno < numdevices), "curr->devno", curr->devno);
   return disksim->deviceinfo->devices[curr->devno]->get_busno(curr);
}


INLINE int device_get_slotno (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_slotno(devno);
}


INLINE int device_get_number_of_blocks (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_number_of_blocks(devno);
}


INLINE int device_get_maxoutstanding (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_maxoutstanding(devno);
}


int device_get_numdevices (void)
{
   return ( numdevices );
}


INLINE int device_get_numcyls (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_numcyls(devno);
}


INLINE double device_get_blktranstime (ioreq_event *curr)
{
   ASSERT1 ((curr->devno >= 0) && (curr->devno < numdevices), "curr->devno", curr->devno);
   return disksim->deviceinfo->devices[curr->devno]->get_blktranstime(curr);
}


INLINE int device_get_avg_sectpercyl (int devno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_avg_sectpercyl(devno);
}


INLINE void device_get_mapping (int maptype, 
				int devno, 
				int blkno, 
				int *cylptr, 
				int *surfaceptr, 
				int *blkptr)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_mapping(maptype, 
							   devno, 
							   blkno, 
							   cylptr, 
							   surfaceptr, 
							   blkptr);
}


INLINE void device_event_arrive (ioreq_event *curr)
{
   ASSERT1 ((curr->devno >= 0) && (curr->devno < numdevices), "curr->devno", curr->devno);
   return disksim->deviceinfo->devices[curr->devno]->event_arrive(curr);
}


INLINE int device_get_distance (int devno, 
				ioreq_event *req, 
				int exact, 
				int direction)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_distance(devno,
							    req,
							    exact,
							    direction);
}


INLINE double device_get_servtime (int devno, 
				   ioreq_event *req, 
				   int checkcache, 
				   double maxtime)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->get_servtime(devno,
							    req,
							    checkcache,
							    maxtime);
}

double device_get_seektime (int devno, 
			    ioreq_event *req, 
			    int checkcache, 
			    double maxtime)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);

   return disksim->deviceinfo->devices[devno]->get_seektime(devno,
							    req,
							    checkcache,
							    maxtime);
}


INLINE double device_get_acctime (int devno, 
				  ioreq_event *req, 
				  double maxtime)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);

   return disksim->deviceinfo->devices[devno]->get_acctime(devno,
							   req,
							   maxtime);
}


INLINE void device_bus_delay_complete (int devno, 
				       ioreq_event *curr, 
				       int sentbusno)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->bus_delay_complete(devno,
								  curr,
								  sentbusno);
}


INLINE void device_bus_ownership_grant (int devno, 
					ioreq_event *curr, 
					int busno, 
					double arbdelay)
{
   ASSERT1 ((devno >= 0) && (devno < numdevices), "devno", devno);
   return disksim->deviceinfo->devices[devno]->bus_ownership_grant(devno,
								   curr,
								   busno,
								   arbdelay);
}

/* dummy */
void disksim_device_loadparams(void) {
  ddbg_assert2(0, "this is a dummy that isn't supposed to be called");
}


int disksim_syncset_loadparams(struct lp_block *b)
{

  int c;
    
  unparse_block(b, outputfile);
  for(c = 0; c < b->params_len; c++) {
    if(!b->params[c]) continue;
    if(PTYPE(b->params[c]) != S) continue;
    if(strcmp(b->params[c]->name, "type")) continue;
    else {
      if(!strcmp(SVAL(b->params[c]), "simpledisk")) {
	fprintf(stderr, "*** warning: no simpledisk syncsets\n");
/*  	simpledisk_load_syncsets(b); */
      }
      else if(!strcmp(SVAL(b->params[c]), "disk")) {
	return disk_load_syncsets(b);
      }
      else if(!strcmp(SVAL(b->params[c]), "mems")) {
	fprintf(stderr, "*** warning: no mems syncsets\n");
/*  	mems_load_syncsets(b); */
      }
      else {
      }
    }
  }
  return 1;
}


INLINE int dev_map_devno(int n)
{ 
  assert((n < numdevices) && (n >=0));
  return devicenos[n];
}

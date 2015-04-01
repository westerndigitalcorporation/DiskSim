
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



#ifndef _DM_DM_H
#define _DM_DM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DM_SOURCE
#include "dm_config.h"
#else
#include <diskmodel/dm_types.h>
#endif 

//#define DEBUG_LAYOUT_G1_LOAD


// mostly opaque; interface through function pointers;
// avoid type fields and switch statements
struct dm_disk_if;

// map lbns to pbns and vice versa
struct dm_layout_if;

// stuff related to disk mechanics, i.e. head positioning
struct dm_mech_if;


/*  struct dm_disk_if *dm_load_diskspec(char *filename); */
/*  int dm_write_diskspec(struct dm_disk_if *, char *filename); */



// convert fixed-point nsecs to double secs and vice versa
dm_time_t dm_time_dtoi(double);
double    dm_time_itod(dm_time_t);


// a physical block address

struct dm_pbn {
  int cyl;
  int head;
  int sector;
};

// convert the internal format to a struct dm_pbn
int dm_to_pbn(dm_pbn_t, struct dm_pbn *);

// convert a struct dm_pbn to the internal format 
int dm_from_pbn(struct dm_pbn *, dm_pbn_t *);


// the state of the mechanical elements
struct dm_mech_state {
  int cyl;
  int head;

  // divide the circle into 2^32 angles; increments of  \pi / 2^31 radians
  dm_angle_t theta;
};


// convert angles between doubles and fixed-point representation
double      dm_angle_itod(dm_angle_t);
dm_angle_t  dm_angle_dtoi(double);


// this tries to be compatible with disksim
typedef enum {
  DM_SLIPPED = -1,   // may also include unused spares XXX is this right?
  DM_REMAPPED = -2,
  DM_OK = -3,
  DM_NX = -4         // doesn't exist -- there are holes in the cyl space
                     // on e.g. atlas10k
} dm_ptol_result_t;


typedef enum {
  DM_SU_SECTORS = 0,
  DM_SU_REVOLUTIONS
} dm_skew_unit_t;


typedef enum {
  MAP_NONE                = 0,
  MAP_IGNORESPARING       = 1,
  MAP_ZONEONLY            = 2,
  MAP_ADDSLIPS            = 3,
  MAP_FULL                = 4,
  MAP_FROMTRACE           = 5,
  MAP_AVGCYLMAP           = 6
} dm_layout_maptype;
#define DM_MAXMAPTYPE MAP_AVGCYLMAP



// What is the angle relative to?

// There are three reasonable "zeroes" to speak of.  The first which
// we'll call "literal" or "absolute" zero or just 0 is a fixed point
// that is the same on every track on the disk.  The second is the
// offset of the leading edge of the first sector lying entirely past
// 0 on the track.  This will typically be the same for all of the
// tracks on a given surface within a zone.  We refer to this as 0t
// for "track."  (in the current implementation, 0t is probably the
// same as 0).  The third zero is the offset of the leading edge of
// the block whose lbn is numerically lowest of all of the blocks on
// the track.  This will also probably be the same for all tracks per
// surface,zone.  We refer to this as 0l for lbn or logical.

// 0t is not affected by skews but is affected by 
// slipping in some defect-management schemes.

// 0l is affected by: track and cylinder switch skew as well as
// slipping in some defect-management schemes.

// We try to be precise about which 0 we mean in describing the
// interface below.

// Further complicating this is how to number the sectors on a given
// track in pbns.  It would be reasonable to call either the one at 0t
// sector zero or the one at 0l sector zero.



// This is crummy because with derated heads, both SPT and TPI can
// vary.  The g4 code has to fake these.
struct dm_layout_zone {
  int spt;  // number of sectors per track

  int lbn_low;
  int lbn_high;

  int cyl_low;
  int cyl_high;
};


// 'if' because its an interface, not the real struct which has
// other data members ...
struct dm_layout_if {

  // if the lbn was remapped, *remapsector will be set to a nonzero
  // value if remapsector is non-NULL.  This emulates the behavior
  // of disksim's global of the same name.
  dm_ptol_result_t
  (*dm_translate_ltop)(struct dm_disk_if *, 
		       int lbn, 
		       dm_layout_maptype,
		       struct dm_pbn *result,
		       int *remapsector);

  dm_ptol_result_t
  (*dm_translate_ltop_0t)(struct dm_disk_if *, 
			  int lbn, 
			  dm_layout_maptype,
			  struct dm_pbn *result,
			  int *remapsector);

  
  // if the pbn is defective and remapped,
  // *remapsector will be set to a nonzero value if remapsector is non-NULL
  dm_ptol_result_t
  (*dm_translate_ptol)(struct dm_disk_if *, 
		       struct dm_pbn *p,
		       int *remapsector);

  dm_ptol_result_t
  (*dm_translate_ptol_0t)(struct dm_disk_if *, 
			  struct dm_pbn *p,
			  int *remapsector);


  // the number of physical sectors on the track containing the given lbn
  int
  (*dm_get_sectors_lbn)(struct dm_disk_if *d,
			int lbn);
  
  // as above for pbns
  int
  (*dm_get_sectors_pbn)(struct dm_disk_if *d,
			struct dm_pbn *);

  // compute lbn boundaries for track
  // This is different from disksim's idosyncratic semantic (second
  // value is first sector of next track).  Our semantic is that 
  // last_lbn is that of the last sector on the given track.
  // We set remapsector if nonzero to a nonzero value if 
  // the first or last block on the track are remapped.
  // Returns DM_NX if the whole track is unmapped, DM_OK otherwise.
  dm_ptol_result_t
  (*dm_get_track_boundaries)(struct dm_disk_if *d,
			     struct dm_pbn *,
			     int *first_lbn,
			     int *last_lbn,
			     int *remapsector);


  // Compute the seek distance in cylinders that would be incurred for
  // given request.  Returns a dm_ptol_result_t since one or both of
  // the LBNs may be slipped or remapped.
  dm_ptol_result_t
  (*dm_seek_distance)(struct dm_disk_if *,
		      int start_lbn,
		      int dest_lbn);

  // Compute the starting offset of a pbn relative to 0.  This
  // accounts for all skews, slips, etc.
  dm_angle_t
  (*dm_pbn_skew)(struct dm_disk_if *,
		 struct dm_pbn *);

  // same as pbn_skew(d, {cyl, head, 0})
  dm_angle_t
  (*dm_get_track_zerol)(struct dm_disk_if *, 
  			struct dm_mech_state *);


  // convert from a pbn to an angle.
  // first result angle is the distance from 0.
  // This accounts for slips and defects but *not* skew --
  // so this is in 0l.
  // The second result parameter is for the width of the sector --
  // currently unimplemented.
  void
  (*dm_convert_ptoa)(struct dm_disk_if *,
		     struct dm_pbn *,
		     dm_angle_t *start,
		     dm_angle_t *width);


  // convert from an angle to a pbn
  // returns a ptol_result since provided angle could be in slipped
  // space, etc.  Rounds angle down to a sector starting offset
  // Takes the angle in 0l.
  dm_ptol_result_t
  (*dm_convert_atop)(struct dm_disk_if *,
		     struct dm_mech_state *,
		     struct dm_pbn *);


  // how wide is a chunk of num sectors on the given track
  // returns 0 if num > s/t
  dm_angle_t
  (*dm_get_sector_width)(struct dm_disk_if *,
			 struct dm_pbn *track,
			 int num);

  // Compute the angular distance/offset between two logical blocks.
  dm_angle_t
  (*dm_lbn_offset)(struct dm_disk_if *,
		   int lbn1,
		   int lbn2);


  // how big will this layout struct be when marshaled
  int(*dm_marshaled_len)(struct dm_disk_if *);
  // unmarshal this layout struct into the provided buffer
  void *(*dm_marshal)(struct dm_disk_if *, char *);


  // returns the number of zones for the layout
  int (*dm_get_numzones)(struct dm_disk_if *);

  // Fetch the info about the nth zone and store it in the result
  // parameter z.  Returns 0 on success, -1 on error (bad n)
  // n should be within 0 and (dm_get_numzones() - 1)
  int (*dm_get_zone)(struct dm_disk_if *, int n, struct dm_layout_zone *z);


  // return the number of defects on the given track
  // result stored at *result
  // returns DM_OK or DM_NX for bad parameters
  dm_ptol_result_t
  (*dm_defect_count)(struct dm_disk_if *d, 
		     struct dm_pbn *track,
		     int *result);

};



// Anatomy of a mech-model access and related terminology
// (bucy 5/2002)

// +-------------------------+------------+----------+---------+----------+
// | seek                    | initial    |          | add.    |          |
// | headswitch              | rotational | xfertime | rot.    | xfertime |
// |            extra settle | latency    |          | latency |          |
// +-------------------------+------------+----------+---------+----------+

// |---------seektime--------|
// |-----------positioning-time-----------|
// |------------------------------access-time-----------------------------|

//                                                             ^
//                                                             |
//                                                  bus transfer to host
//                                                  begins now

// What disksim calls "SERVTIME" is really access time minus xfertime;
// we've agreed to call this NOXFERTIME in the future to reduce confusion.

// For zero-latency accesses, disksim's disklatency() function returns
// the initial rotational latency in this nomenclature; disksim's 
// "addtolatency" is our additional rotational latency.

struct dm_mech_acctimes {
   dm_time_t seektime;
   dm_time_t initial_latency;
   dm_time_t initial_xfer;
   dm_time_t addl_latency;
   dm_time_t addl_xfer;
};

struct dm_mech_if {

  // how long to seek from the first to the second track, possibly
  // including a head switch and additional write settling time.  This
  // is only track-to-track so the angles in the parameters are
  // ignored.
  dm_time_t(*dm_seek_time)(struct dm_disk_if *, 
			   struct dm_mech_state *start_track,
			   struct dm_mech_state *end_track,
			   int rw);


  // this function subsumes the functionality of the "trackstart"
  // global in disksim.  From the given inital condition and access,
  // it will return the first block on the track to be read.
  int(*dm_access_block)(struct dm_disk_if *,
			struct dm_mech_state *initial,
			int start,
			int len,
			int immed);

  // There are two rotational latency functions.  Read carefully to
  // make sure you're using the one you want.


  // This function implements the "disksim semantic: read *up to*
  // <len> blocks from the track starting from angle <initial> and
  // sector <start>.  This will access to the end of the track *but
  // not wrap around* e.g. for a sequential access that starts on the
  // given track and switches to another, after reaching the end of
  // the first.  The return value is the amount of time before the
  // media transfer begins.  Any additional rotational latency (as
  // defined above) will be filled into addtolatency if nonzero.
  // Initial angle is relative to 0!
  dm_time_t(*dm_latency)(struct dm_disk_if *,
			 struct dm_mech_state *initial, // initial state
			 int start,                     // start sector 
			 int len,                       // number of sectors
			 int immed,                     // zero-latency?
			 dm_time_t *addtolatency);

  // UNIMPLEMENTED
  // Compute the rotational latency incurred for the given (possibly
  // "zero-latency") access.  The access is of the form: starting from
  // the initial angle <initial>, read len blocks from the track
  // starting with start, wrapping around zero if the access is that
  // long.  The return value is only rotational latency before the
  // data transfer begins.  In the case of zero-latency accesses,
  // there may be an additional rotational latency.  This additional
  // amount is filled into the last parameter if nonzero.
  dm_time_t(*dm_latency_seq)(struct dm_disk_if *,
			     struct dm_mech_state *initial, // initial state
			     int start,                     // start sector 
			     int len,                    // number of sectors
			     int immed,                     // zero-latency?
			     dm_time_t *addtolatency);


  // Seek, any extra settling for write, initial rotational latency.
  // i.e. the amount of time before media transfer starts.
  // len must be >= 1
  dm_time_t(*dm_pos_time)(struct dm_disk_if *,
			  struct dm_mech_state *initial,
			  struct dm_pbn *start,
			  int len,
			  int rw,
			  int immed,
			  struct dm_mech_acctimes *breakdown);

  // access time is postime, xfertime and any additional rotational 
  // latency not included in postime, e.g. in the middle of a 
  // zero-latency access xfer.
  // if the last parameter is nonzero, it will be filled
  // in with the state of the disk as of the reported result time;
  // i.e. where the disk will be when the access completes.

  // Consider changing this to an lbn since most occurances in 
  // disksim seem to do ltop and pass the result here.
  // Simplifies g1 "multi-track" wrapper.
  // len must be >= 1
  dm_time_t(*dm_acctime)(struct dm_disk_if *, 
			 struct dm_mech_state *initial_state,
			 struct dm_pbn *start,
			 int len,
			 int rw,
			 int immed,
			 struct dm_mech_state *result_state,
			 struct dm_mech_acctimes *breakdown);

  // compute how long it will take the disk to rotate from the angle
  // in the first position to that in the second position
  dm_time_t(*dm_rottime)(struct dm_disk_if *,
			 dm_angle_t begin,
			 dm_angle_t end);
  

  // Amount of time to read len sectors from the track designated by
  // the 2nd argument.  This is a convience wrapper for 
  // rottime.
  dm_time_t(*dm_xfertime)(struct dm_disk_if *d,
			  struct dm_mech_state *,
			  int len);


  // This is probably constant for now but might not be in the future.
  dm_time_t(*dm_headswitch_time)(struct dm_disk_if *, 
				 int h1, 
				 int h2);


  // how far will the media rotate in the given amount of time
  // only the angle in the result is set
  dm_angle_t(*dm_rotate)(struct dm_disk_if *, 
			 dm_time_t *time);

  // how long does one revolution take
  dm_time_t(*dm_period)(struct dm_disk_if *);


  // interface to disksim for rpm randomization
  // 2nd argument is the amount of time one rotation takes
  void(*dm_randomize_rpm)(struct dm_disk_if *);

  // how big will this layout struct be when marshaled
  int(*dm_marshaled_len)(struct dm_disk_if *);

  // unmarshal this layout struct into the provided buffer
  // returns a pointer to the first byte in the buffer it didn't write to
  void *(*dm_marshal)(struct dm_disk_if *, char *);
};



// all the methods below return zero on success, nonzero on error
struct dm_disk_if {
  // some general parameters
  int dm_cyls;

  //  int dm_tracks;  // what needs this?
  int dm_surfaces;
  int dm_sectors;


  struct dm_layout_if   *layout;
  struct dm_mech_if     *mech;

};




//
// libparam related stuff (probably should live somewhere else)
//
struct lp_block;

// these are the libparam loader functions for mech and layout models
typedef struct dm_mech_if *(*dm_mech_loader_t)(struct lp_block *, struct dm_disk_if *);
typedef struct dm_layout_if *(*dm_layout_loader_t)(struct lp_block *, struct dm_disk_if *);


#ifdef __cplusplus
}
#endif


#endif  /*  _DM_DM_H  */

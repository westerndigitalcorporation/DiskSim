
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



#include "dm.h"
#include "modules/modules.h"

#include <libparam/libparam.h>
#include <libddbg/libddbg.h>

#include <stdio.h>


// translate the lbn space forwards and backwards
void layout_test_simple(struct dm_disk_if *d) {
  int c, lbn, count = 0, runlbn = 0;
  struct dm_pbn pbn, trkpbn = {0,0,0};
  printf("got a dm_disk with %d sectors!\n", d->dm_sectors);

  for(c = 0; c < d->dm_sectors; c ++) {
    int lbn2;
    dm_angle_t skew, zerol;
    struct dm_mech_state track;

    d->layout->dm_translate_ltop(d, c, MAP_FULL, &pbn, 0);
    lbn2 = d->layout->dm_translate_ptol(d, &pbn, 0);

    if(c != lbn2) {
      printf("*** test_0_t: %8d -> (%8d, %3d, %4d) -> %8d\n", 
	     c, pbn.cyl, pbn.head, pbn.sector, lbn2);
    }
    else {
      if((lbn > 0)
	 && (trkpbn.head == pbn.head)
	 && (trkpbn.cyl == pbn.cyl)
	 && ((trkpbn.sector+count) == pbn.sector)) { count++;  }
      else {
	printf("l %d %d %d %d %d\n", runlbn, trkpbn.cyl, trkpbn.head, trkpbn.sector, count);
	runlbn = c;
	trkpbn = pbn;
	count = 1;
      }


    }    
  }
}


void test_rotate(struct dm_disk_if *d) {
  double t;
  dm_angle_t a;
  dm_time_t t2;
  for(t = 0.0; t < 20.0; t += 0.1) {
    t2 = dm_time_dtoi(t);
    a = d->mech->dm_rotate(d, &t2);
    printf("time %f, angle %f\n", t, dm_angle_itod(a));
  }
  
}

void layout_test_skew(struct dm_disk_if *d) {
  int c;
  dm_angle_t skew;
  double angle;
  struct dm_pbn pbn;

  for(c = 6281; c <= 6704; c++) {
    pbn.cyl = c;
    pbn.head = 3;
    pbn.sector = 250;
      
    d->layout->dm_convert_ptoa(d, &pbn, &skew, 0);
    angle = dm_angle_itod(skew);
    printf("ptoa (%d,%d,%d): %f, %u\n",
	   pbn.cyl, pbn.head, pbn.sector, angle, skew);
  }
  
  for(c = 0; c < 1000000000; c += 10000000) {
    struct dm_mech_state a;
    a.head = 3;
    a.cyl = 0;
    a.theta = c;
    d->layout->dm_convert_atop(d, &a, &pbn);
    printf("atop (%d,%d,%d) -> (%d,%d,%d)\n",
	   a.head, a.cyl, a.theta, pbn.head, pbn.cyl, pbn.sector);
  }

}


// single-cyl switch
void mech_test_postime1(struct dm_disk_if *d) {
  struct dm_mech_state begin, end;
  dm_time_t result;
  double time;
  int c;

  begin.head = 0;
  begin.cyl = 0;
  begin.theta = 0;

  end.head = 0;
  end.theta = 0;

  for(c = 0; c < 1000; c++) {
    end.cyl = c;

    result = d->mech->dm_seek_time(d, &begin, &end, 0);
    time = dm_time_itod(result);
    printf("cyl switch %d: %lld, %e\n", c, result, time);
  }
}

// single-head switch
void mech_test_postime2(struct dm_disk_if *d) {
  struct dm_mech_state begin, end;
  dm_time_t result;
  double time;

  begin.head = 0;
  begin.cyl = 0;
  begin.theta = 0;

  end.head = 1;
  end.cyl = 0;
  end.theta = 0;

  result = d->mech->dm_seek_time(d, &begin, &end, 0);
  time = dm_time_itod(result);
  printf("single-head switch: %lld, %e\n", result, time);
}

void mech_test_xfertime(struct dm_disk_if *d) {
  int sectors;
  struct dm_mech_state m;
  dm_time_t xfertime;
  double time;

  m.head = 0; 
  m.cyl = 0;
  m.theta = 0;

  // how long to transfer half a track?  
  sectors = d->layout->dm_get_sectors_lbn(d, 0);
  sectors >>= 1;


  xfertime = d->mech->dm_xfertime(d, &m, sectors);
  time = dm_time_itod(xfertime);
  printf("half-track xfer: %lld, %e\n", xfertime, time);
}

void mech_test_access(struct dm_disk_if *d) {
  // very basic access 

  int sectors;
  struct dm_mech_state m;
  dm_time_t result;
  struct dm_pbn startpbn;
  double time;

  m.head = 0; 
  m.cyl = 0;
  m.theta = 0;


  // how long to transfer half a track?  
  sectors = d->layout->dm_get_sectors_lbn(d, 0);
  sectors /= 2;

  startpbn.head = 0;
  startpbn.cyl = 0;
  startpbn.sector = 0;



  //  result = d->mech->dm_acctime(d, &m, &startpbn, sectors, 0, 0, 0, 0);
  time = dm_time_itod(result);


  printf("half-track access: %lld, %e\n", result, time);
}

void mech_pos_vs_acc(struct dm_disk_if *d) {
  // very basic access 
  int sectors;
  struct dm_mech_state m;
  dm_time_t postime, acctime;
  struct dm_pbn startpbn;
  double time;
  struct dm_mech_acctimes bacc, bpos;

  m.head = 0; 
  m.cyl = 0;
  m.theta = 0;


  // how long to transfer half a track?  
  sectors = d->layout->dm_get_sectors_lbn(d, 0);
  sectors /= 2;

  d->layout->dm_translate_ltop(d, 8955874, MAP_FULL, &startpbn,  0);
  sectors = 1;

  bzero(&bacc, sizeof(bacc));
  bzero(&bpos, sizeof(bpos));

  acctime = d->mech->dm_acctime(d, &m, &startpbn, sectors, 0, 0, 0, &bacc);
  postime = d->mech->dm_pos_time(d, &m, &startpbn, sectors, 0, 0, &bpos);


  printf("half-track pos: %lld, acc: %lld\n", postime, acctime);
}




void mech_test_access2(struct dm_disk_if *d) {
  // half a rotation, read the rest of the track
  int sectors;
  struct dm_mech_state m;
  dm_time_t result;
  struct dm_pbn startpbn;
  double time;

  m.head = 0; 
  m.cyl = 0;
  m.theta = 4282107939;


  sectors = d->layout->dm_get_sectors_lbn(d, 0);
  sectors >>= 1;

  startpbn.head = 1;
  startpbn.cyl = 0;
  startpbn.sector = 0;


  result = d->mech->dm_acctime(d, &m, &startpbn, sectors, 0, 0, 0, 0);
  time = dm_time_itod(result);


  printf("access2 (track switch): %lld, %e\n", result, time);
}


void mech_test_access3(struct dm_disk_if *d) {
  // half a rotation, read the rest of the track
  int sectors;
  struct dm_mech_state m;
  dm_time_t result;
  struct dm_pbn startpbn;
  double time;

  m.head = 0; 
  m.cyl = 0;
  m.theta = 0;


  sectors = d->layout->dm_get_sectors_lbn(d, 0);
  sectors >>= 1;

  startpbn.head = 0;
  startpbn.cyl = 1;
  startpbn.sector = 0;


  result = d->mech->dm_acctime(d, &m, &startpbn, sectors, 0, 0, 0, 0);
  time = dm_time_itod(result);


  printf("access3 (cyl switch): %lld, %e\n", result, time);
}


void mech_test_access_big(struct dm_disk_if *d) {
  // half a rotation, read the rest of the track
  int c,h;
  int sectors;
  struct dm_mech_state m = {0,0,0}, tempm, resultstate = {0,0,0};
  dm_time_t result;
  struct dm_pbn startpbn;
  double time;
  int tracks, lbn = 0;


  //  tracks = d->dm_cyls * d->dm_surfaces;
  tracks = 10 * d->dm_surfaces;
  for(c = 0; c < tracks; c++) {
    sectors = d->layout->dm_get_sectors_lbn(d, c);
    lbn = c * sectors;
    d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &startpbn, 0);

    result = d->mech->dm_acctime(d, 
				 &m, 
				 &startpbn, 
				 1, // len
				 1, // read?
				 0, // zero-latency?
				 &resultstate,
				 0);
    time = dm_time_itod(result);

    printf("access lbn %d (%d,%d,%d) -> %f ", lbn, 
	   startpbn.cyl, startpbn.head, startpbn.sector, time);


/*      m.head = startpbn.head; */
/*      m.cyl = startpbn.cyl; */

/*      startpbn.sector = (startpbn.sector + 1) % sectors;  */
/*      d->layout->dm_convert_ptoa(d, &startpbn, &m.theta, 0);   */

/*      startpbn.sector = (startpbn.sector - 1) % sectors;  */
/*      m.theta += d->layout->dm_pbn_skew(d, &startpbn); */

    m = resultstate;

/*      d->mech->dm_rotate(d, &result, &tempm); */
/*      m.theta += tempm.theta; */


    printf("%f\n\n\n", dm_angle_itod(m.theta));
  }
}

void mech_test_access_immed(struct dm_disk_if *d) {
  // zero latency access; read half the track starting with the head over
  // the middle of the access
  int c,h;
  int sectors;
  struct dm_mech_state m = {0,0,0}, tempm, resultstate = {0,0,0};
  dm_time_t result;
  struct dm_pbn startpbn, tmppbn;
  double time;
  int tracks, lbn = 0;


  //  tracks = d->dm_cyls * d->dm_surfaces;
  tracks = 10 * d->dm_surfaces;

  for(c = 0; c < tracks; c++) {
    int len;
    sectors = d->layout->dm_get_sectors_lbn(d, c);
    lbn = c * sectors;
    len = sectors / 2;

    d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &startpbn, 0);
    
    m.head = startpbn.head;
    m.cyl = startpbn.cyl;
    
    m.theta = d->layout->dm_pbn_skew(d, &startpbn);
    m.theta += dm_angle_dtoi((double)(len / 2) / (double)sectors);

    

    result = d->mech->dm_acctime(d,         // disk
				 &m,        // start state
				 &startpbn, // first pbn to read
				 len,      
				 1,         // rw
				 1,         // immed
				 &resultstate,        // result state
				 0);
    time = dm_time_itod(result);

    printf("access_immed (%d, %d) -- %f\n\n\n",
	   m.head, m.cyl, time);


  }
}

void mech_test_seek(struct dm_disk_if *d) {
  int c;
  struct dm_mech_state s1 = {0,0,0}, s2;

  for(c = 0; c < 1000; c++) {
    dm_time_t nsecs;
    double t;
    s2.cyl = c;
    nsecs = d->mech->dm_seek_time(d, &s1, &s2, 0);
    t = dm_time_itod(nsecs);
    printf("seek dist %d took %f\n", c, t);
  }
}


void layout_test_trackbound(struct dm_disk_if *d) {
  int c;
  int lbn = 55000;

  for(c = 0; c < 1000; c++) {
    int l1, l2;
    struct dm_pbn pbn;
    int remapsector = 0;

    d->layout->dm_translate_ltop(d, lbn, MAP_FULL, &pbn, &remapsector);

    d->layout->dm_get_track_boundaries(d, &pbn, &l1, &l2, &remapsector);

    printf("test_trackbound: %d -> (%d, %d) (%d)\n", lbn, l1, l2, remapsector);
    ddbg_assert((l1 <= lbn) && (lbn <= l2));

    lbn +=  (d->layout->dm_get_sectors_lbn(d, lbn) + 13); 
  }
}

void weird_disksim_test(struct dm_disk_if *d) {
  struct dm_mech_state startstate = { 446, 1, 4221625600 };
  dm_time_t nsecs1, nsecs2;
  int trackstart1, trackstart2;

  nsecs1 = d->mech->dm_latency(d, &startstate, 145, 334, 1, 0);
  trackstart1 = d->mech->dm_access_block(d, &startstate, 145, 334, 1);
  nsecs2 = d->mech->dm_latency(d, &startstate, trackstart1, 1, 0, 0);
  trackstart2 = d->mech->dm_access_block(d, &startstate, trackstart1, 1, 0);

  printf("%lld %d %lld %d\n", nsecs1, trackstart1, nsecs2, trackstart2);

}

void test_lbn_offset(struct dm_disk_if *d) {
  double dangle;
  dm_angle_t result;
  int c;

  for(c = 0; c < 688; c++) {
    result = d->layout->dm_lbn_offset(d, 0, c);
    dangle = dm_angle_itod(result);
    printf("%8d %8d %20.10g\n", 0, c, dangle);
  }

  for(c = 0; c < 17966953; c += 10000) {
    result = d->layout->dm_lbn_offset(d, 0, c);
    dangle = dm_angle_itod(result);
    printf("%8d %8d %20.10g\n", 0, c, dangle);
  }
}


void test_breakdown(struct dm_disk_if *d) {
  struct dm_mech_acctimes breakdown;
  dm_time_t result;
  dm_time_t checkres;
  struct dm_mech_state half = {0,0, 2000000000};
  struct dm_pbn start = { 0, 0, 0 };
  
  result = d->mech->dm_pos_time(d, &half, &start, 300, 1, 0, &breakdown);
  checkres = breakdown.seektime + breakdown.initial_latency +
    breakdown.initial_xfer + breakdown.addl_latency + breakdown.addl_xfer;

  result = d->mech->dm_acctime(d, &half, &start, 3000, 1, 0, 0, &breakdown);
  checkres = breakdown.seektime + breakdown.initial_latency +
    breakdown.initial_xfer + breakdown.addl_latency + breakdown.addl_xfer;

  
}

void test_0t(struct dm_disk_if *d) {
  int lbn;
  struct dm_pbn pbn; 

  for(lbn = 0; lbn < 1000000; lbn++) {
    int lbn2;
    d->layout->dm_translate_ltop_0t(d, lbn, MAP_FULL, &pbn, 0);
    lbn2 = d->layout->dm_translate_ptol_0t(d, &pbn, 0);
    if(lbn != lbn2) {
      printf("*** test_0_t: %8d -> (%8d, %3d, %4d) -> %8d\n", 
	     lbn, pbn.cyl, pbn.head, pbn.sector, lbn2);
    }
    else {
      printf("%d <-> (%d,%d,%d) \n", lbn, pbn.cyl, pbn.head, pbn.sector);
    }
  }
}


void test_1track(struct dm_disk_if *d) {
  struct dm_mech_state s = {0,0,0};
  struct dm_pbn p;
  dm_time_t result;
  struct dm_mech_acctimes b;
  
  p.head = 1;
  p.sector = 63;
  p.cyl = 0;

  result = d->mech->dm_pos_time(d, &s, &p, 1, 1, 0, &b);

}


void test_skew2(struct dm_disk_if *d) {
  dm_angle_t result;
  struct dm_pbn pbn;
  double dangle;
  int i;


  for(i = 0; i < d->dm_sectors; i++) {
    d->layout->dm_translate_ltop(d, i, MAP_FULL, &pbn, 0);
    result = d->layout->dm_pbn_skew(d, &pbn);
    dangle = dm_angle_itod(result);
    printf("lbn %d -> %f\n", i, dangle);
  }
  
}

void test_g2_skew(struct dm_disk_if *d) {
  dm_angle_t a1, a2;
  int i;
  struct dm_pbn p = {0,0,0};

  for(i = 0; i < 1000000; i++) {
    d->layout->dm_translate_ltop(d, i, MAP_FULL, &p, 0);
    a1 = pbn_skew(d, &p);
    a2 = pbn_skew_new(d, &p);
    
    //    if(a1 != a2) {
      printf("(%d,%d,%d) -> %lf %lf\n", 
	     p.cyl, p.head, p.sector,
	     dm_angle_itod(a1), dm_angle_itod(a2));
      //    }
  }
  
}

void test_zonefoo(struct dm_disk_if *d) {
  int i;
  int nz = d->layout->dm_get_numzones(d);
  struct dm_layout_zone z;
  
  printf("got %d zones\n", nz);
  for(i = 0; i < nz; i++) {
    d->layout->dm_get_zone(d, i, &z);
    printf("zone %2d: %d (%d,%d) (%d,%d)\n", 
	   i, z.spt,
	   z.cyl_low, z.cyl_high,
	   z.lbn_low, z.lbn_high);
  }
  

}

int main(int argc, char **argv) {
  int c;
  FILE *modelfile;
  struct dm_layout_if dif;
  struct dm_disk_if *disk;
  char *modelname;
  
  ddbg_assert_setfile(stderr);

  if(argc < 2) {
    fprintf(stderr, "usage: dm_testsuite <model file> [model name]\n");
    exit(1);
  }

  //  printf("kill me now! sizeof(struct dm_layout_if) = %d\n",
  //	 sizeof(struct dm_layout_if));

  modelfile = fopen(argv[1], "r");
  if(!modelfile) {
    fprintf(stderr, "*** error: failed to open \"%s\"\n", argv[1]);
  }

  for(c = 0; c <= DM_MAX_MODULE; c++) {
    struct lp_mod *mod;

    if(c == DM_MOD_DISK) {
      mod = dm_mods[c];
/*        m.callback = disk_callback; */
    }
    else {
      mod = dm_mods[c];
    }

    lp_register_module(mod);
  }

  lp_init_typetbl();
  lp_loadfile(modelfile, 0, 0, argv[1], 0, 0);
  fclose(modelfile);
  
  modelname = argc >= 3 ? argv[2] : 0;
  
  disk = lp_instantiate("foo", modelname);

  //  test_rotate(disk);
  layout_test_simple(disk); 

/*    layout_test_skew(disk); */ 
/*    mech_test_postime1(disk);  */
/*    mech_test_postime2(disk);  */
/*    mech_test_xfertime(disk);  */
/*    mech_test_access(disk);  */
/*    mech_test_access2(disk);  */
/*    mech_test_access3(disk);  */
/*        mech_test_access_big(disk);    */
/*      mech_test_access_immed(disk);  */
/*        mech_test_seek(disk); */
  //  layout_test_trackbound(disk); 
  

  //  test_lbn_offset(disk);

  //  weird_disksim_test(disk);

  //  test_breakdown(disk);
  //test_0t(disk);

  //  mech_pos_vs_acc(disk);

  //  test_1track(disk);

  //  test_skew2(disk);

  //  test_g2_skew(disk);

  //  test_zonefoo(disk);

  return 0;
}


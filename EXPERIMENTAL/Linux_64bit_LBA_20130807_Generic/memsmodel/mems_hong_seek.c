#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mems_hong_seek.h"

#define _VERBOSE_ 0

long double
dmin(long double x, long double y) {
  return (x < y) ? x : y;
}

long double
dmax(long double x, long double y) {
  return (x > y) ? x : y;
}

long double
find_m(long double x0, long double x1,
       long double spring_factor,
       long double length_nm) {
  
  long double kFx;
  long double m;

  kFx = spring_factor / length_nm;

  m = ((x0 + x1) / 2)
    +
    ((kFx / 4) * (x1*x1 - x0*x0));

  return m;
}

long double
find_seek_time_hong_x(long double start_offset_nm, long double end_offset_nm,
		      long double spring_factor, long double acceleration_nm_s_s,
		      long double length_nm)
{
  long double xm;
  long double mkx;
  long double tx;

  long double x0 = start_offset_nm / 1e9;
  long double x1 = end_offset_nm / 1e9;
  long double l = length_nm / 1e9;
  long double a = acceleration_nm_s_s / 1e9;

  long double temp_tx;

  x0 = dmin(start_offset_nm, end_offset_nm);
  x1 = dmax(start_offset_nm, end_offset_nm);

  x0 /= 1e9;
  x1 /= 1e9;

  xm = find_m(x0, x1, spring_factor, l);

  if (_VERBOSE_) {
    printf("find_seek_time_hong_x -- xm = %0.10Lf\n",
	   xm);
  }

  if (spring_factor == 0.0) {
    mkx = 0.0;
  } else {
    if (_VERBOSE_) {
      printf("l = %0.10Lf, spring_factor = %0.10Lf, a = %0.10Lf\n",
	     l, spring_factor, a);
    }
    mkx = l / (spring_factor * a);
  }

  tx = 0.0;

  temp_tx = 
    sqrt(mkx)
    *
    acos(
	 (xm - a*mkx)
	 /
	 (x0 - a*mkx)
	 );

  tx += temp_tx;

  if (_VERBOSE_) {
    printf("(1) temp_tx = %0.10Lf, tx = %0.10Lf\n",
	   temp_tx, tx);
  }

  temp_tx = 
    sqrt(mkx)
    *
    acos(
	 (xm + a*mkx)
	 /
	 (x1 + a*mkx)
	 );

  tx += temp_tx;

  if (_VERBOSE_) {
    printf("(2) temp_tx = %0.10Lf, tx = %0.10Lf\n",
	   temp_tx, tx);
    printf("find_seek_time_hong_x -- tx = %0.10Lf\n",
	   tx);
  }

  return tx;
}

long double
find_seek_time_hong_y(long double start_offset_nm, long double end_offset_nm,
		      long double spring_factor, long double acceleration_nm_s_s,
		      long double length_nm, long double velocity_nm_s) {

  long double ym;
  long double mk;
  long double ty;

  long double y0 = start_offset_nm / 1e9;
  long double y1 = end_offset_nm / 1e9;
  long double l = length_nm / 1e9;
  long double a = acceleration_nm_s_s / 1e9;
  long double v = velocity_nm_s / 1e9;

  long double aa, bb, cc;

  long double temp_ty;

  if (_VERBOSE_) {
    printf("find_seek_time_hong_y -- a = %0.10Lf, v = %0.10Lf, spring_factor = %0.10Lf\n",
	   a, v, spring_factor);
  }

  y0 = dmin(start_offset_nm, end_offset_nm);
  y1 = dmax(start_offset_nm, end_offset_nm);

  y0 /= 1e9;
  y1 /= 1e9;

  if (_VERBOSE_) {
    printf("find_seek_time_hong_y -- y0 = %0.10Lf, y1 = %0.10Lf\n",
	   y0, y1);
  }

  ym = find_m(y0, y1, spring_factor, l);

  if (_VERBOSE_) {
    printf("find_seek_time_hong_y -- ym = %0.10Lf\n",
	   ym);
  }

  if (spring_factor == 0.0) {
    mk = 0.0;
  } else {
    if (_VERBOSE_) {
      printf("l = %0.10Lf, spring_factor = %0.10Lf, a = %0.10Lf\n",
	     l, spring_factor, a);
    }
    mk = l / (spring_factor * a);
  }

  if (_VERBOSE_) {
    printf("find_seek_time_hong -- mk = %0.10Lf\n",
	   mk);
  }

  ty = 0.0;

  temp_ty = 
    sqrt(mk)
    *
    asin(
	 (ym - a*mk)
	 /
	 sqrt(
	      (y0 - a*mk)*(y0 - a*mk)
	      +
	      (v * sqrt(mk))*(v * sqrt(mk))
	      )
	 );

  ty += temp_ty;

  if (_VERBOSE_) {
    printf("(1) temp_ty = %0.10Lf, ty = %0.10Lf\n",
	   temp_ty, ty);
  }

  temp_ty = 
    sqrt(mk)
    *
    asin(
	 (y0 - a*mk)
	 /
	 sqrt(
	      (y0 - a*mk)*(y0 - a*mk)
	      +
	      (v * sqrt(mk))*(v * sqrt(mk))
	      )
	 );
  
  ty -= temp_ty;

  if (_VERBOSE_) {
    printf("(2) temp_ty = %0.10Lf, ty = %0.10Lf\n",
	   temp_ty, ty);
  }
  
  temp_ty = 
    sqrt(mk)
    *
    asin(
	 (y1 + a*mk)
	 /
	 sqrt(
	      (y1 + a*mk)*(y1 + a*mk)
	      +
	      (v * sqrt(mk))*(v * sqrt(mk))
	      )
	 );

  ty += temp_ty;

  if (_VERBOSE_) {
    printf("(3) temp_ty = %0.10Lf, ty = %0.10Lf\n",
	   temp_ty, ty);

    aa = (ym + a*mk);
    printf("aa = %0.10Lf\n", aa);
    
    aa = (y1 + a*mk)*(y1 + a*mk);
    printf("aa = %0.10Lf\n", aa);
    
    bb = (v * sqrt(mk))*(v * sqrt(mk));
    printf("v = %0.10Lf, mk = %0.10Lf, aa = %0.20Lf\n", 
	   v, mk, bb);
    
    aa = sqrt(aa + bb);
    printf("aa = %0.10Lf\n", aa);  
    
    aa = (ym + a*mk)
      /
      sqrt(
	   (y1 + a*mk)*(y1 + a*mk)
	   +
	   (v * sqrt(mk))*(v * sqrt(mk))
	   );
    printf("aa = %0.10Lf\n", aa);  
    
    bb = modf(aa, (double *)(&cc));
    printf("BOOGA bb = %0.10Lf\n", bb);
        
    aa = asin(aa);
    printf("aa = %0.10Lf\n", aa);

    bb = asin(bb);
    printf("bb = %0.10Lf\n", bb);
  }

  temp_ty = 
    sqrt(mk)
    *
    asin(
	 (ym + a*mk)
	 /
	 sqrt(
	      (y1 + a*mk)*(y1 + a*mk)
	      +
	      (v * sqrt(mk))*(v * sqrt(mk))
	      )
    );

  ty -= temp_ty;

  if (_VERBOSE_) {
    printf("(4) temp_ty = %0.10Lf, ty = %0.10Lf\n",
	   temp_ty, ty);
    printf("find_seek_time_hong -- ty = %0.10Lf\n",
	   ty);
  }

  return ty;
}

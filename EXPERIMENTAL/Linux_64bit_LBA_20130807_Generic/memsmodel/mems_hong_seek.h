#ifndef _MEMS_HONG_SEEK_H_
#define _MEMS_HONG_SEEK_H_

long double
find_seek_time_hong_x(long double start_offset_nm, long double end_offset_nm,
		      long double spring_factor, long double acceleration_nm_s_s,
		      long double length_nm);

long double
find_seek_time_hong_y(long double start_offset_nm, long double end_offset_nm,
		      long double spring_factor, long double acceleration_nm_s_s,
		      long double length_nm, long double velocity_nm_s);

#endif

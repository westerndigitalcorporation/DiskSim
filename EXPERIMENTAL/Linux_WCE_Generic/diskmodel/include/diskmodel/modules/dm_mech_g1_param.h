
#ifndef _DM_MECH_G1_PARAM_H
#define _DM_MECH_G1_PARAM_H  

#include <libparam/libparam.h>
#ifdef __cplusplus
extern"C"{
#endif
struct dm_disk_if;

/* prototype for dm_mech_g1 param loader function */
struct dm_mech_if *dm_mech_g1_loadparams(struct lp_block *b, int *num);

typedef enum {
   DM_MECH_G1_ACCESS_TIME_TYPE,
   DM_MECH_G1_CONSTANT_ACCESS_TIME,
   DM_MECH_G1_SEEK_TYPE,
   DM_MECH_G1_AVERAGE_SEEK_TIME,
   DM_MECH_G1_CONSTANT_SEEK_TIME,
   DM_MECH_G1_SINGLE_CYLINDER_SEEK_TIME,
   DM_MECH_G1_FULL_STROBE_SEEK_TIME,
   DM_MECH_G1_FULL_SEEK_CURVE,
   DM_MECH_G1_ADD_WRITE_SETTLING_DELAY,
   DM_MECH_G1_HEAD_SWITCH_TIME,
   DM_MECH_G1_ROTATION_SPEED_,
   DM_MECH_G1_PERCENT_ERROR_IN_RPMS,
   DM_MECH_G1_FIRST_TEN_SEEK_TIMES,
   DM_MECH_G1_HPL_SEEK_EQUATION_VALUES
} dm_mech_g1_param_t;

#define DM_MECH_G1_MAX_PARAM		DM_MECH_G1_HPL_SEEK_EQUATION_VALUES
extern void * DM_MECH_G1_loaders[];
extern lp_paramdep_t DM_MECH_G1_deps[];


static struct lp_varspec dm_mech_g1_params [] = {
   {"Access time type", S, 1 },
   {"Constant access time", D, 0 },
   {"Seek type", S, 1 },
   {"Average seek time", D, 0 },
   {"Constant seek time", D, 0 },
   {"Single cylinder seek time", D, 0 },
   {"Full strobe seek time", D, 0 },
   {"Full seek curve", S, 0 },
   {"Add. write settling delay", D, 1 },
   {"Head switch time", D, 1 },
   {"Rotation speed (in rpms)", I, 1 },
   {"Percent error in rpms", D, 1 },
   {"First ten seek times", LIST, 0 },
   {"HPL seek equation values", LIST, 0 },
   {0,0,0}
};
#define DM_MECH_G1_MAX 14
static struct lp_mod dm_mech_g1_mod = { "dm_mech_g1", dm_mech_g1_params, DM_MECH_G1_MAX, (lp_modloader_t)dm_mech_g1_loadparams,  0, 0, DM_MECH_G1_loaders, DM_MECH_G1_deps };


#ifdef __cplusplus
}
#endif
#endif // _DM_MECH_G1_PARAM_H

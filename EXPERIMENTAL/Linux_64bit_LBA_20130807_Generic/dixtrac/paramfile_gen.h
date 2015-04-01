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

#ifndef __PARAMS_H
#define __PARAMS_H


void write_param_file(char *model_name);
void pars_iodriver_read_toprints(FILE *fp);
void pars_bus_read_toprints(FILE *fp);
void pars_bus_read_specs(FILE *fp);
void pars_bus_read_physorg(FILE *fp);
void pars_controller_read_toprints(FILE *fp);
void pars_controller_read_specs(FILE *fp);
void pars_controller_read_physorg(FILE *fp);
void pars_controller_read_logorg(FILE *fp);
void pars_disk_read_toprints(FILE *fp);
void pars_disk_read_specs(FILE *fp,char *brandname, char *model_name);
void pars_disk_read_syncsets(FILE *fp);
void pars_io_read_generalparms(FILE *fp);
void pars_iodriver_read_specs(FILE *fp);
void pars_ioqueue_readparams(FILE *fp);
void pars_iodriver_read_physorg(FILE *fp);
void pars_iodriver_read_logorg(FILE *fp,int numblocks);
void pars_logorg_readparams(FILE *fp,int numblocks);
void pars_pf_readparams(FILE *fp,int numblocks);
void pars_synthio_readparams(FILE *fp,int numblocks);
void pars_synthio_read_uniform(FILE *fp, float min,float max);
void pars_synthio_read_normal(FILE *fp, float mean,float variance);
void pars_synthio_read_exponential(FILE *fp,float base,float mean);
void pars_synthio_read_poisson(FILE *fp,float base,float mean);
void pars_synthio_read_twovalue(FILE *fp,float defval,float secval,float probab);
void pars_synthio_read_distr(FILE *fp,char *type,float arg1,float arg2,float arg3);

#endif

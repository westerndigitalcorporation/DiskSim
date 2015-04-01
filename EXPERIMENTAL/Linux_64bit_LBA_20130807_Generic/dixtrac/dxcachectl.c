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

#include "dixtrac.h"
#include "print_bscsi.h"
#include "extract_params.h"
#include "extract_layout.h"
#include "scsi_conv.h"
#include "read_diskspec.h"
#include "mtbrc.h"
#include "mechanics.h"
#include "trace_gen.h"
#include "paramfile_gen.h"
#include "cache.h"
#include "handle_error.h"
#include "dxopts.h"

extern diskinfo_t *thedisk;

extern char *scsi_buf;

#ifdef ADDR_TRANS_COUNTER
extern int transaddr_cntr;
#endif

#if HAVE_GETOPT_H
static struct option const longopts[] =
{
    {"cache-parameters",  required_argument, NULL, 'c'},
    {"ABPF",  required_argument, NULL, 'A'},
    {"DRA",  required_argument, NULL, 'D'},
    {"IC",   required_argument, NULL, 'I'},
    {"DISC", required_argument, NULL, 'S'},
    {"RCD", required_argument, NULL, 'R'},
    {"WCE", required_argument, NULL, 'W'},
    {"help", no_argument, NULL, 'h'},
    {0,0,0,0}
};
#endif

static char const * const option_help[] = {
    "-A <bit>   --ABPF       Sets ABPF to <bit>.", 
    "-D <bit>   --DRA        Sets DRA to <bit>.", 
    "-I <bit>   --IC         Sets IC to <bit>.", 
    "-S <bit>   --DISC       Sets DISC to <bit>.", 
    "-R <bit>   --RCD        Sets RCD to <bit>.", 
    "-W <bit>   --WCE        Sets WCE to <bit>.", 
    "-h  --help              ",
    "-c  --cache-parameters  Set the disk's read and write cache to the argument.",
    0
};

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void 
usage(FILE *fp,char *program_name) 
{
    fprintf(fp,"Usage: %s [options] -d <disk>\n",
	    program_name);
}

/*---------------------------------------------------------------------------*
 *                                                                           * 
 *---------------------------------------------------------------------------*/
static void
print_help(FILE *fp,char *program_name) 
{
    char const * const *p;
    usage(fp,program_name);
    for (p = option_help;  *p;  p++)
	fprintf(fp,"  %s\n", *p);
    fprintf(fp,"<disk> is the corresponding SCSI raw device.\n");
    fprintf(fp,"\nIf invoked without any options, print out the current ");
    fprintf(fp,"state of the cache.\n\n");
}

/****************************************************************************
 *
 *
 ****************************************************************************/
int 
main (int argc, char *argv[]) 
{
    char c;
    char *program_name;
    char dev[256];

    int cache_bits = READ_CACHE_DISABLE | WRITE_CACHE_DISABLE;
    int turn_cache = 0;

    int use_scsi_device = 1;

    int setABPF = 0;
    int setDRA = 0; 
    int setIC = 0;
    int setDISC = 0;
    int setRCD = 0;
    int setWCE = 0;

    int ABPFbit = 0;
    int DRAbit = 0; 
    int ICbit = 0;
    int DISCbit = 0;
    int RCDbit = 0;
    int WCEbit = 0;

    dev[0] = 0;

    program_name = argv[0];
    while (strpbrk(program_name++,"/")) ;
    program_name--;

    while ((c = GETOPT(argc, argv,"hd:c:A:D:I:S:R:W:")) != EOF) {
	switch (c) {
	case 'h':
	    print_help(stdout,program_name);
	    exit(0);
	    break;
	case 'd':
	    strncpy(dev, optarg, sizeof(dev));
	    break;
	case 'c':
	    turn_cache = 1;
	    {
		char *endptr;
		cache_bits =  (int) strtol(optarg, &endptr, 10);
		if (!strncmp(endptr,"ON",2)) {
		    cache_bits = 4;
		}
		else if (!strncmp(endptr,"OFF",3)) {
		    cache_bits = 1;
		}
		else if ((cache_bits < 0) || (cache_bits > 8)) {
		    error_handler("The option -c takes values ON, OFF or a number between 0 and 8!\n");
		}
	    }
	    break;
	case 'A':
	    setABPF = 1;
	    ABPFbit = atol(optarg);
	    if (ABPFbit != 1 && ABPFbit != 0) {
		error_handler("Bit value for ABPF must be either 0 or 1!\n");
	    }
	    break;
	case 'D':
	    setDRA = 1;
	    DRAbit = atol(optarg);
	    if (DRAbit != 1 && DRAbit != 0) {
		error_handler("Bit value for DRA must be either 0 or 1!\n");
	    }
	    break;
	case 'I':
	    setIC = 1;
	    ICbit = atol(optarg);
	    if (ICbit != 1 && ICbit != 0) {
		error_handler("Bit value for IC must be either 0 or 1!\n");
	    }
	    break;
	case 'S':
	    setDISC = 1;
	    DISCbit = atol(optarg);
	    if (DISCbit != 1 && DISCbit != 0) {
		error_handler("Bit value for DISC must be either 0 or 1!\n");
	    }
	    break;
	case 'R':
	    setRCD = 1;
	    RCDbit = atol(optarg);
	    if (RCDbit != 1 && RCDbit != 0) {
		error_handler("Bit value for RCD must be either 0 or 1!\n");
	    }
	    break;
	case 'W':
	    setWCE = 1;
	    WCEbit = atol(optarg);
	    if (WCEbit != 1 && WCEbit != 0) {
		error_handler("Bit value for WRE must be either 0 or 1!\n");
	    }
	    break;
	}
    }

    if ((!argv[optind]) && (!*dev)) {
	usage(stderr,program_name);
	exit(1);
    }
    else if (argv[optind]) {
	strncpy(dev, argv[optind], sizeof(dev));
    }

    disk_init(dev, use_scsi_device);
    
    if (turn_cache) {
	set_cache(cache_bits);
    }
    if (setABPF) 
	set_cache_bit(CACHE_ABPF,ABPFbit);
    
    if (setDRA) 
	set_cache_bit(CACHE_DRA,DRAbit);

    if (setIC)
	set_cache_bit(CACHE_IC,ICbit);

    if (setDISC)
	set_cache_bit(CACHE_DISC,DISCbit);

    if (setRCD)
	set_cache_bit(CACHE_RCD,RCDbit);

    if (setWCE)
	set_cache_bit(CACHE_WCE,WCEbit);

    exec_scsi_command(scsi_buf,SCSI_mode_sense(CACHE_PAGE));
    print_scsi_mode_sense_data (scsi_buf);

    disk_shutdown(use_scsi_device);
    exit (0);
}


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

#ifndef __DXTOOLS_H
#define __DXTOOLS_H

#include "config.h"

#if HAVE_GETOPT_H

#define DX_COMMON_OPTS_LIST                                  \
  {"run-all-tests",     no_argument, NULL, 'a'},             \
  {"help",              no_argument, NULL, 'h'},             \
  {"use-mappings",      required_argument, NULL, 'u'},       \
  {"generate-mappings", optional_argument, NULL, 'g'},       \
  {"raw-layout",        no_argument, NULL, 'r'},             \
  {"model-name",        required_argument, NULL, 'm'},       \
  {"layout",            no_argument, NULL, 'l'},             \
  {"read-from-file",    no_argument, NULL, 'f'},             

#define GETOPT(a,b,c)       getopt_long(a,b,c,longopts,0)
#else
#define GETOPT(a,b,c)       getopt(a,b,c)
#endif

#define DX_COMMON_OPTS_HELP                                                  \
"-a  --run-all-tests     Extract all the available information.",            \
"-u  --use-mappings      Use layout file with LBN to physical addresses.",   \
"-g  --generate-mappings Extract complete logical-to-physical disk map.",    \
"-r  --raw-layout        Read layout from disk map file.",                   \
"-m  --model-name        String used as a base for created files.",          \
"-l  --layout            Determine disk layout information.",                \
"-f  --read-from-file    Read the disk layout from a file.",                 \
"-h  --help              Output this help.",                                 


#define DX_COMMON_VARIABLES           \
   char model_name[100];              \
   int extractlayout = 0;             \
   int use_mapping_file = 0;          \
   char mapping_file[256];            \
   int gen_complete_mappings = 0;     \
   int start_lbn = 0;                 \
   int run_tests = 0;                 \
   int model_name_defined = 0;        \
   int use_scsi_device = 1;           \
   int use_layout_file = 0;           \
   int use_raw_layout = 0;            

#define DX_COMMON_OPTIONS                                     \
     case 'r':                                                \
       use_raw_layout = 1;                                    \
       break;                                                 \
     case 'l':                                                \
       extractlayout = 1;                                     \
       break;                                                 \
     case 'f':                                                \
       use_layout_file = 1;                                   \
       break;                                                 \
     case 'g':                                                \
       gen_complete_mappings = 1;                             \
       start_lbn = ((optarg) ? atoi(optarg) : 0);             \
       break;                                                 \
     case 'u':                                                \
       use_mapping_file = 1;                                  \
       use_scsi_device = 0;                                   \
       strcpy(mapping_file,optarg);                           \
       break;                                                 \
     case 'm':                                                \
       model_name_defined = 1;                                \
       strcpy(model_name,optarg);                             \
       break;                                                 \
     case 'a':                                                \
       run_tests = 1;                                         \
       break;                                                 \
     case 'h':                                                \
       print_help(stdout,program_name);                       \
       exit(0);                                               \
     case '?':                                                \
       error_handler("Try -h for help.\n");                   \
     default:                                                 \
       usage(stderr,program_name);      

#define DX_COMMON_OPTS_CHK                                                  \
   if ((argc == 1) || (!argv[optind])) {                                    \
     usage(stderr,program_name);                                            \
     exit(1);                                                               \
   }                                                                        \
   if (! model_name_defined) {                                              \
     error_handler("Model filename must be specified (use -m option)!\n");  \
   }                                                                        \
   if (gen_complete_mappings && use_mapping_file) {                         \
     error_handler("The options -g and -u are mutually exclusive!\n");      \
   }                                                                        \
   if (extractlayout && use_layout_file) {                                  \
     error_handler("The options -f and -l are mutually exclusive!\n");      \
   }                                                                        \
   if (extractlayout && use_raw_layout) {                                   \
     error_handler("The options -r and -l are mutually exclusive!\n");      \
   }

#define DX_COMMON_OPTS_HANDLE                                               \
   dxobtain_layout(extractlayout,use_layout_file,use_raw_layout,            \
		   gen_complete_mappings,use_mapping_file,model_name,       \
		   start_lbn,mapping_file);



#endif



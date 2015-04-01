# diskmodel (version 1.0)
# Authors: John Bucy, Greg Ganger
# Contributors: John Griffin, Jiri Schindler, Steve Schlosser
#
# Copyright (c) of Carnegie Mellon University, 2001-2008.
#
# This software is being provided by the copyright holders under the
# following license. By obtaining, using and/or copying this
# software, you agree that you have read, understood, and will comply
# with the following terms and conditions:
#
# Permission to reproduce, use, and prepare derivative works of this
# software is granted provided the copyright and "No Warranty"
# statements are included with all reproductions and derivative works
# and associated documentation. This software may also be
# redistributed without charge provided that the copyright and "No
# Warranty" statements are included in all redistributions.
#
# NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
# CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
# EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
# TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
# OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
# MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
# RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
# INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
# OF THIS SOFTWARE OR DOCUMENTATION.  

CC=gcc 
CXX=g++

# must provide LIBPARAM_ and LIBDDBG_ vars
include .paths

# -D_DISKMODEL_FREEBSD for the freebsd kernel (malloc foo)
# -D_BSD_SOURCE for bsd-ish userspace to get uint32 and friends from the
# right place
CFLAGS = -g -I. $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS) -D_DM_SOURCE -Wall -Wshadow -Wno-unused -MD  # -O6  -fomit-frame-pointer -fexpensive-optimizations -fschedule-insns2 -march=pentium4

CXXFLAGS = -g -I. $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS) -D_DM_SOURCE -Wall -Wno-unused  -MD

CC-DEP = $(CC)
CP = cp -p
all: modules libdiskmodel.a
	$(MAKE) -C tests

merge_skews: merge_skews.o libdiskmodel.a
	$(CC) -o $@ $^ $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS)

.PHONY: modules
modules: modules/*.o ;

modules/modules.h modules/*.o:
	$(MAKE) -C modules
	mkdir -p include/diskmodel/modules
	$(CP) dm.h dm_types.h dm_config.h marshal.h include/diskmodel
	$(CP) modules/*.h include/diskmodel/modules

# modules/modules.h modules/*.h modules/*.c: modules

-include *.d

DM_SRC = mech_g1_seektime.c mech_g1.c \
	 layout_g1.c sqrt.c marshal.c layout_g2.c \
	 layout_g4.c

# source for libparam loaders; separate for things like kernel
# which don't have libparam 
DM_USERSPACE_SRC = mech_g1_load.c layout_g1_load.c dm_load.c \
  convert.c layout_g2_load.c layout_g4_load.c

DM_OBJ = $(DM_SRC:.c=.o) 

DM_LOADER_OBJ = modules/*.o

DM_USERSPACE_OBJ =  $(DM_LOADER_OBJ) $(DM_USERSPACE_SRC:.c=.o)


# for userspace
libdiskmodel.a:	$(DM_OBJ) $(DM_USERSPACE_OBJ) 
	$(MAKE) -C modules
	@echo "Linking libdiskmodel.a $(DM_USERSPACE_OBJ)"
	ar crus $@ $(DM_OBJ) $(DM_USERSPACE_OBJ)
	mkdir -p lib
	$(CP) libdiskmodel.a lib

# for kernel
#libdiskmodel.a: modules $(DM_OBJ)
#	ar crus $@ $(DM_OBJ)

clean:
	rm -f *.o
	$(MAKE) -C modules clean
	$(MAKE) -C layout_g4_tools clean
	$(MAKE) -C tests clean

distclean: clean
	rm -f *.d *~
	rm -f libdiskmodel.a dm_testsuite
	rm -rf lib include 
	$(MAKE) -C modules distclean
	$(MAKE) -C layout_g4_tools distclean
	$(MAKE) -C tests distclean


# DiskSim Storage Subsystem Simulation Environment (Version 4.0)
# Revision Authors: John Bucy, Greg Ganger
# Contributors: John Griffin, Jiri Schindler, Steve Schlosser
#
# Copyright (c) of Carnegie Mellon University, 2001-2008.
#
# This software is being provided by the copyright holders under the
# following license. By obtaining, using and/or copying this software,
# you agree that you have read, understood, and will comply with the
# following terms and conditions:
#
# Permission to reproduce, use, and prepare derivative works of this
# software is granted provided the copyright and "No Warranty" statements
# are included with all reproductions and derivative works and associated
# documentation. This software may also be redistributed without charge
# provided that the copyright and "No Warranty" statements are included
# in all redistributions.
#
# NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
# CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
# EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
# TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
# OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
# MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
# TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE
# OR DOCUMENTATION.

include .paths

LDFLAGS = -lm -L. 

DEBUG_OFLAGS = -g -DASSERTS # -DDEBUG=1
PROF_OFLAGS = -g -DASSERTS -p
GPROF_OFLAGS = -g -DASSERTS -pg
CFLAGS = $(DEBUG_OFLAGS) $(DISKSIM_CFLAGS) -I../ -I$(DISKSIM_PREFIX)

CC = gcc -Wall -Wno-unused -MD
CP = cp -p

all: modules mems_seektest libmemsmodel.a

clean:
	rm -f *.o mems_seektest core libmems_internals.a libmemsmodel.a
	$(MAKE) -C modules clean
	$(MAKE) -C tests clean

realclean: clean
	rm -f *.d .depend
	$(MAKE) -C modules clean
	$(MAKE) -C tests clean

distclean: realclean
	rm -f *~
	rm -rf lib include
	$(MAKE) -C modules distclean
	$(MAKE) -C tests distclean

.PHONY: modules

modules: modules/*.o ;

modules/modules.h modules/*.o:
	$(MAKE) -C modules
	mkdir -p include/memsmodel/modules
	$(CP) mems_disksim.h mems_internals.h mems_mapping.h \
          mems_piecewise_seek.h mems_global.h mems_buffer.h include/memsmodel
	$(CP) modules/*.h include/memsmodel/modules

-include *.d

MEMS_SRC = mems_seektest.c mems_internals.c mems_piecewise_seek.c mems_hong_seek.c mems_buffer.c

MEMS_OBJ = $(MEMS_SRC:.c=.o) 

$(MEMS_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

mems_seektest: mems_seektest.o libmems_internals.a
	$(CC) -o $@ mems_seektest.o $(CFLAGS) -lmems_internals $(LDFLAGS)

libmems_internals.a: mems_internals.o mems_piecewise_seek.o mems_hong_seek.o
	ar cru $@ mems_internals.o mems_piecewise_seek.o mems_hong_seek.o
	ranlib $@
	mkdir -p lib
	$(CP) libmems_internals.a lib

libmemsmodel.a: mems_disksim.o mems_event.o mems_mapping.o mems_internals.o mems_piecewise_seek.o mems_hong_seek.o mems_buffer.o modules/memsmodel_mems_param.o
	ar cru $@ mems_disksim.o mems_event.o mems_mapping.o mems_internals.o mems_piecewise_seek.o mems_hong_seek.o mems_buffer.o modules/memsmodel_mems_param.o
	ranlib $@
	mkdir -p lib
	$(CP) libmemsmodel.a lib



########################################################################

# rule to automatically generate dependencies from source files
#%.d: %.c
#	set -e; $(CC-DEP) -M $(CPPFLAGS) $<  \
#		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
#		[ -s $@ ] 2>/dev/null >/dev/null || rm -f $@


# this is a little less aggressive and annoying than the above
depend: .depend 

.depend: *.c *.h
	$(MAKE) -C modules
	rm -f .depend
	$(foreach file, $(DISKSIM_SRC), \
		$(CC-DEP) $(CFLAGS) -M $(file) >> .depend; )


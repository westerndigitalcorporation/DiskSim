
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


TOP_BUILDDIR=$(shell pwd)
include .paths

SUBDIRS=libddbg libparam diskmodel memsmodel ssdmodel src valid

all: 	
	$(MAKE) -C libddbg
	$(MAKE) -C libparam
	$(MAKE) -C diskmodel
#	$(MAKE) -C memsmodel
#	$(MAKE) -C ssdmodel
	$(MAKE) -C src
#	$(MAKE) -C diskmodel/layout_g4_tools
        # If dixtrac is included, build it with the distribution
#	if [ -d dixtrac ]; then $(MAKE) -C dixtrac ; fi

clean:
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

distclean: clean
	rm -f *~
	for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

.PHONY: doc
doc:
	$(MAKE) -C libddbg
	$(MAKE) -C libparam
	$(MAKE) -C src/modules
	$(MAKE) -C diskmodel/modules
#	$(MAKE) -C memsmodel/modules
#	$(MAKE) -C ssdmodel/modules

doc-clean:
	$(MAKE) -C memsmodel/modules distclean
	$(MAKE) -C diskmodel/modules distclean
	$(MAKE) -C ssdmodel/modules distclean
	$(MAKE) -C src/modules distclean
	$(MAKE) -C libparam distclean
	$(MAKE) -C libddbg distclean

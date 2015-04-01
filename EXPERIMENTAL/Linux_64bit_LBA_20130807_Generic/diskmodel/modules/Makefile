
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


all : all-redirect

include .paths

.SECONDARY:

LIBPARAM=$(LIBPARAM_PREFIX)
PACKAGE=dm
CFLAGS = -g -D_DM_SOURCE $(LIBDDBG_CFLAGS) $(LIBPARAM_CFLAGS)

PARAM_PROTO = disk.modspec mech_g1.modspec \
	layout_g1.modspec layout_g1_zone.modspec \
	layout_g2.modspec layout_g2_zone.modspec \
	layout_g4.modspec


PARAM_CODE = $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.c)
PARAM_HEADERS = $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.h)
PARAM_OBJ =  $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.o)

$(PACKAGE)_%_param.c $(PACKAGE)_%_param.h: %.modspec 
	$(LIBPARAM)/mod.pl $(PACKAGE) $<
	indent $@ 2>/dev/null >/dev/null || true

modules.h: $(PARAM_HEADERS)
	$(LIBPARAM)/make_modules_h.pl $(PACKAGE) *.modspec > modules.h

all-redirect: $(PARAM_OBJ) modules.h

clean:
	rm -f *.h *.c *.tex *~ dm_mods.a *.o

distclean: clean

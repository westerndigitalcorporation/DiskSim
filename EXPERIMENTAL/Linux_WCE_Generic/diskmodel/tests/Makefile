# diskmodel (version 1.1)
# Authors: John Bucy, Greg Ganger
# Contributors: John Griffin, Jiri Schindler, Steve Schlosser
#
# Copyright (c) of Carnegie Mellon University, 2003-2005
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

include .paths

CFLAGS = -g $(DISKMODEL_CFLAGS) $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS)

LDFLAGS = $(DISKMODEL_LDFLAGS) $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS)

tests = layout_simple layout_simple_0t layout_track layout_raw layout_bogon layout_skew mech_seek

all: $(tests)



%:	%.c driver.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# disable the builtin rule
% : %.c

clean:
	rm -f $(tests) *.o

distclean: clean
	rm -f *~

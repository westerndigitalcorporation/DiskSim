
# libparam (version 1.0)
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



include .paths
all: libparam.a
	mkdir -p lib include/libparam
	cp -p libparam.a lib
	cp -p bitvector.h libparam.h include/libparam
	chmod 777 mod.pl
	chmod 777 make_modules_h.pl

FLEX=flex
BISON=bison
YACC=$(BISON)

CFLAGS=-g -I$(LIBDDBG_INCL) -Wall

# input file parser
lex.libparam.c:	libparam.lex libparam.tab.h libparam.h
	$(FLEX) -Plibparam $<

libparam.c libparam.tab.c libparam.tab.h: libparam.y libparam.h
	$(BISON) -plibparam --defines $<

LIBPARAM_SRC =  util.c libparam.tab.c  lex.libparam.c 

LIBPARAM_OBJ = $(LIBPARAM_SRC:.c=.o)

libparam.a: $(LIBPARAM_OBJ)
	ar crus $@ $^ 


dump_tlts: dump_tlts.o libparam.a
	$(CC) -o $@ $^ -L. -lparam $(LIBDDBG_LDFLAGS)


clean: 
	rm -f lex.libparam.c libparam.tab.c libparam.tab.h libparam.a *.o dump_tlts
	rm -rf lib include

distclean: clean


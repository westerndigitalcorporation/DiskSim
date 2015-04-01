#!/usr/bin/python

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

# Fix "obvious" slips, i.e.
# lbn 491580 cyl 540 head 0 tp (0, 79)
# lbn 491659 cyl 540 head 0 tp (80, 106)
# slipped sector 80, length 1

# Probably assumes a "whole-track" layout.
# XXX does this work for the beginning of the track slipped?

import sys
import cPickle

import layout
import copy


try:
    infile = open(sys.argv[1], "r");
except IOError:
    print "open %s" % [sys.argv[1]]

try:
    outfile = open(sys.argv[2], "w");
except IOError:
    print "open %s" % [sys.argv[2]]
    sys.exit(1)

lin = layout.Layout();
lin = cPickle.load(infile);

lout = layout.Layout();
lout.tps = lin.tps
lout.defects = lin.defects
lout.lbns = lin.lbns
lout.cyls = lin.cyls
lout.heads = lin.heads

lout.exts = []
first = 0
firste = lin.exts[first]
count = lin.exts[0].tp[1]    

tot = 0

# Assumes that every track is mapped
# lbn i ... lbn i+n-1 => sector 0 .. sector n-1
# Any unmapped sectors are accounted for with slips.

for i in range(1,len(lin.exts)):
    exti = lin.exts[i]
    prev = lin.exts[i-1]

    if exti.cyl == firste.cyl \
           and exti.head == firste.head:

        slipext = copy.copy(firste)
        slipsect = prev.tp[0] + prev.tp[1]
        slipext.tp = (slipsect, exti.tp[0] - slipsect)

        slipext.lbn = exti.lbn + tot
        tot += exti.tp[0] - slipsect


        lout.slips.append(slipext)

        print "slipped %s" % (slipext)

        
    else:
        # if the first sector is slipped
        if firste.tp[0] > 0:
            newfirst = copy.copy(firste)
            slipext = copy.copy(newfirst)
            
            sliplen = lin.exts[first].tp[0]
            
            newfirst.tp = (0, newfirst.tp[1] + sliplen)
            lout.tps[lin.exts[first].tp][0] -= 1
            if newfirst.tp not in lout.tps:
                lout.tps[newfirst.tp] = [1, newfirst.tp]
                
            lout.tps[newfirst.tp][0] += 1
            
            # Don't need it in this case because firste.lbn already
            # has it added in (below).
            # slipext.lbn = firste.lbn + tot
            slipext.tp = (0, sliplen)
            lout.slips.append(slipext)

            tot += firste.tp[0]
            
            print "%s -> %s (%s)\n" % (lin.exts[first], newfirst, slipext)

            firste = newfirst

        if i - first > 1:
            fixede = copy.copy(firste)
            # assume in ascending order, should really take the max
            fixede.tp = (firste.tp[0], prev.tp[0] + prev.tp[1])


            lout.exts.append(fixede)
            print "fixed"
            for e in (lin.exts[first:i]):
                lout.tps[e.tp][0] -= 1
                print e

            # assumes this tp already appears...
            lout.tps[fixede.tp][0] += 1

            print "->"
            print fixede
            print "\n"
                    
#            lout.exts.append(lin.exts[i])

        else:
            lout.exts.append(firste)
            
        first = i
        firste = lin.exts[i]
        firste.lbn += tot
        count = firste.tp[1]


# Don't eat the last one.  There may be a corner case here if there
# are slips on the last track.
lout.exts.append(lin.exts[-1])

for t in lout.tps.keys():
    if lout.tps[t][0] == 0:
        # print "tp ref 0 deleted %s" % lout.tps[t]
        del lout.tps[t]

cPickle.dump(lout,outfile)

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

import sys;
import cPickle;

import layout;

# Fix up "remapped discontinuities"

# Input:  the output of pass0.
# Output: 
# [ defects, exts ].  Defects is a list of
# (src,dest) extent-tuples
# exts is a list of (lbn,cyl,head,tp) and have the remaps fixed.

try:
    infile = open(sys.argv[1], "r");
except IOError:
    print "open %s" % [sys.argv[1]]

try:
    outfile = open(sys.argv[2], "w");
except IOError:
    print "open %s" % [sys.argv[2]]
    

lin = layout.Layout();
lin = cPickle.load(infile);

lout = layout.Layout();
lout.tps = lin.tps
lout.slips = lin.slips
lout.lbns = lin.lbns
lout.cyls = lin.cyls
lout.heads = lin.heads


# Next: make this work for multiple remap dests (add 3rd loop)

# Magic Constant: 30 in 2nd loop.  Maximum number of extents we can
# expect to see for a single track.

# XXX needs to work for the first lbn on a track being remapped.

# I'm not sure if this will work for
# good1 bad1 good2 bad2 ... on the same track

i = 0
ei = layout.Extent();
ej = layout.Extent();
max = len(lin.exts)-1
while (i < max):
    ei = lin.exts[i]
    if ei.cyl != lin.exts[i+1].cyl or ei.head != lin.exts[i+1].head:
        for j in range(2,min(30,max-i)):
            ej = lin.exts[i+j]
            
            if ei.cyl == ej.cyl and ei.head == ej.head:
                ebad = lin.exts[i+j-1]
                gapsize = ej.tp[0] - (ei.tp[0] + ei.tp[1]);
                if gapsize != ebad.tp[1]:
                    print "some other weird thing %s %s %s %d" % \
                          (ei, ebad, ej, gapsize)
                else:
                    badext = layout.Extent(ebad.lbn, ei.cyl, ei.head, (ei.tp[0] + ei.tp[1], gapsize));
                    dest = ebad;
                    fixed = layout.Extent(ei.lbn, ei.cyl, ei.head, (ei.tp[0], ei.tp[1] + gapsize + ej.tp[1]));

                    print "bad %s remapped to %s" % (badext, dest)
                    print "fixed %s" % (fixed)

#                    lout.tps[badext.tp][0] -= 1
                    lout.tps[fixed.tp][0] += 1

                    lout.exts.append(fixed);
                    lout.defects.append((badext,dest));

                    i += j;
                    break;
        else:
            lout.exts.append(ei)
    else:
        lout.exts.append(ei)  
    i += 1;

# XXX is this correct?
lout.exts.append(lin.exts[-1])

for t in lout.tps.keys():
    if lout.tps[t][0] == 0:
        # print "tp ref 0 deleted %s" % lout.tps[t]
        del lout.tps[t]

cPickle.dump(lout, outfile);

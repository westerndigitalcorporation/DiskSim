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



# Read in a "raw layout" and pickle it out for fast access by
# subsequent passes.

import re;
import sys;
import string;
import cPickle;

import layout;


def fuxor(x):
    if re.match('^[0-9]+$', x): return string.atoi(x)
    else: return x

try:
    outfile = open(sys.argv[1], 'w');
except IOError:
    print "open %s" % [sys.argv[1]];

lout = layout.Layout()

# The number of cylinders in the label is often a bit larger than the
# last mapped one.  Some things (dx_seeks) expect there to be mapped
# lbns on the last cylinder according to this.
maxcyl = 0

for l in sys.stdin:
    l = re.sub('[\s,]+', ' ', l, 0)
    fields = re.split('\s+', l)
    fields = map(fuxor, fields)
    if fields[0] == "lbn":
        lbn = fields[1]
        cyl = fields[4]

        if cyl > maxcyl:
            maxcyl = cyl

        head = fields[6]
        sect = fields[8]
        count = fields[10]
        if fields[9] == 'seqcnt':
            count += 1
        
        if not (sect,count) in lout.tps:
            lout.tps[(sect,count)] = [1, (sect,count)]
        else:
            lout.tps[(sect,count)][0] += 1
            
        tp = lout.tps[(sect,count)][1]

        ext = layout.Extent(lbn,cyl,head,tp)
        lout.exts.append(ext)

    elif fields[1] == "cylinders":
        lout.cyls = fields[0]
        lout.heads = fields[4]
        print "heads %d cyls %d" % (lout.heads, lout.cyls)
    elif fields[0] == "maxlbn":
        lout.lbns = fields[1]
        print "lbns %d" % (lout.lbns)
        
for tp in lout.tps.keys():
    print lout.tps[tp]


if maxcyl < lout.cyls:
    print "maxcyl %d < disk label cyls %d\n" % (maxcyl, lout.cyls)
    lout.cyls = maxcyl
    
cPickle.dump(lout, outfile, 2);

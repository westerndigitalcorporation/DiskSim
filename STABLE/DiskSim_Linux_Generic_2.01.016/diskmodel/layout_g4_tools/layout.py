
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

# Common datastructures for g4 layout analysis code.

class Extent:
    def __init__(self, lbn = 0, cyl = 0, head = 0, tp = (0,0)):
        self.lbn = lbn
        self.cyl = cyl
        self.head = head
        self.tp = tp

    def __str__(self):
        return ("lbn %d cyl %d head %d tp %s" % \
                ( self.lbn,  \
                  self.cyl,  \
                  self.head, \
                  self.tp ))

class Rect:
    def __init__(self, exts = [], cylwidth = 0, lbnwidth = 0):
        self.exts = exts # list of extents, w/rel lbns
        self.cylwidth = cylwidth
        self.lbnwidth = lbnwidth
        self.ents = []  # list of (ext, cylrunlen)
        
    def len(self):
        return len(self.exts)

# A Rect along with a range of extents matched by it.
class RectInst:
    def __init__(self, rect, start, end, size):
        self.rect = rect
        self.start = start
        self.end = end
        self.size = size
        
class Layout:
    def __init__(self, cyls = 0, heads = 0, lbns = 0, exts = [], defects = [], slips = [], spares = [], tps = {}, rects = [], rinsts = []):
        self.cyls = cyls
	self.heads = heads
	self.lbns = lbns
        self.exts = exts     # Extent list
        self.defects = defects  # (Extent,Extent) list
        self.slips = slips    # Extent list
        self.spares = spares   # Extent list
        self.tps = tps      # "hash set" keyed by (start sect, len)
        self.rects = rects
        self.rinsts = rinsts  # this is intermediate-only

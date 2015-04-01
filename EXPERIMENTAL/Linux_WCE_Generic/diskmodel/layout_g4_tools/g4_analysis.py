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

import copy

from sets import Set

try:
    infile = open(sys.argv[1], "r");
except IOError:
    print "open %s" % [sys.argv[1]]

try:
    loutfilename = sys.argv[2]
    loutfile = open(loutfilename, "w")
except IOError:
    print "open %s" % [loutfilename]

try:
    moutfilename = sys.argv[3]
    moutfile = open(moutfilename, "w")
except IOError:
    print "open %s" % [moutfilename]

cname = sys.argv[4]

lin = layout.Layout();
lin = cPickle.load(infile);

lout = copy.copy(lin)
lout.rects = []
lout.rinsts = []

# default is 2 * heads -- too small for cylrange sparing
# could manage this adaptively?
max_rect = 2 * lin.heads
assert(max_rect > 0)

# XXX I'm sure there's a slicker way to do this
def idx(x,y):
    res = None
    i = 0
    for j in y:
        if j == x:
            res = i
            break
        else:
            i += 1
    return res

# Try matching r starting at e0.  Return (good, inst).  Good number
# of extents matched successfully.
def matchRect(l, r, e0):
    good = bad = 0
    lastgood = e0

    basecyl0 = reduce(lambda x,y: min(x,y.cyl), r.exts, r.exts[0].cyl)
    basecyl = min(map(lambda x: x.cyl, l.exts[e0:e0+r.len()]))

#    print "matchrect basecyl0 %d basecyl %d cw %d" % (basecyl0, basecyl, r.cylwidth)

    
    for off in range(0,r.len()):
        if e0 + off >= len(l.exts):
            break
        
        ext0i = r.exts[off]
        exti = l.exts[e0 + off]
        
        if ext0i.head == exti.head \
               and (ext0i.cyl - basecyl0) == ((exti.cyl - basecyl) % r.cylwidth)\
               and ext0i.tp[0] == exti.tp[0] \
               and ext0i.tp[1] == exti.tp[1]:
            lastgood += 1
            good += 1
#            print "matched (%s) (%s)" % (ext0i, exti)
        else:
#            print "broke (rlen %d) (%s) (%s)" % (r.len(), ext0i, exti)
            return (0, None, 0)
        


    return (good, layout.RectInst(r, l.exts[e0], l.exts[e0 + off], off+1), lastgood-1)

    
def breakcond(bad,good):
    return bad+good > 100 and bad * 3 > good

# See how well a rect of size rlen starting at e0 works.  Return
# (good, inst list).  good is number of extents matched succesfully.
def tryRect(l, r, e0):
    inst = 0
    goodtot = 0
    badtot = 0
    insts = []
    lastgood = e0
    
    inst0 = e0
    i = 0

    nextcyl = l.exts[inst0].cyl

    while inst0+r.len() < len(l.exts):
        # Make them be contiguous/monotonic in cyls.
        # Fails for LBNs because of slips.  Not necessary anyway since the 
        # input is in ascending LBN order.
        if l.exts[inst0].cyl != nextcyl:
            break

        (good, inst, lastgoodr) = matchRect(l, r, inst0)
        goodtot += good
        badtot += r.len() - good

        if inst != None:
            lastgood = lastgoodr
            insts.append(inst)
        else:
            break;
#            i = inst0
#            if i + r.len() <= len(l.exts): max = i+r.len()
#            else: max = r.len()
#            for i in range(inst0,max):
#                insts.append(layout.RectInst(None, l.exts[i], l.exts[i], 1))

        i += 1

        inst0 += r.len()
        nextcyl += r.cylwidth

    return (goodtot, insts, lastgood)

# Come up with a leaf with len runs
def getLeaf(l, e0, llen):
    ei = e0       # ith run
    eij = e0      # loop counter 
    eijprev = e0  # one before eij
    dir = 0       # 1 or -1
    origlen = llen

    res = layout.Rect()
    
    while llen > 0:
        if ei+1 >= len(l.exts):
            break
        if l.exts[ei].head == l.exts[ei+1].head:
            dir = l.exts[ei+1].cyl - l.exts[ei].cyl
            if dir == 1 or dir == -1:
                eij = ei + 1
                eijprev = ei
                # accumulate a cyl run on a given surface
#                print "new run %s" % (l.exts[ei])
                while eij < len(l.exts) \
                          and l.exts[eij].head == l.exts[ei].head \
                          and l.exts[eij].cyl == l.exts[eijprev].cyl + dir \
                          and l.exts[eij].tp == l.exts[eijprev].tp:
#                    print "add to run %s" % (l.exts[eij])
                    eij += 1
                    eijprev += 1

                # runs must be of len > 2
                if eij - ei < 3:
                    eij = ei
                else:
                    eij -= 1
                    cylrunlen = abs(l.exts[eij].cyl - l.exts[ei].cyl) + 1
                    res.ents.append((l.exts[ei], \
                                     cylrunlen * (l.exts[ei].tp[1] - l.exts[ei].tp[0]),
                                    dir * cylrunlen))


        llen -= 1
        eij += 1
        ei = eij 

    

    if ei > e0:
        res.exts = l.exts[e0:ei]
        # if no runs -- all ents are singletons, we have to make this up here
        if ei - e0 == origlen:
            res.ents = map(lambda x: (x,x.tp[1]-x.tp[0],1), res.exts)

        res.cylwidth = max(map((lambda (x): x.cyl), res.exts)) - min(map((lambda (x): x.cyl), res.exts)) + 1 
        res.lbnwidth = reduce((lambda x,y: x + y.tp[1]), res.exts, 0)
#        print "getLeaf %d - %d" % (e0,ei)
    else:
        res = None

        
    return res

# Find a rect in layout l starting at extent e0. Return
# a pair of (Rect, RectInst) if we find one or () otherwise.
def findRect(l, e0, rects):
    ext0 = l.exts[e0]
    bestinsts = 0
    bestrlen = 0

    bestrect = None
    bestrectinsts = None
    bestlastgood = 0

    # Try known ones.
    for ri in rects:
        if ri.exts == []: # placeholder
            continue
            
        
        (good, insts, lastgood) = tryRect(l, ri, e0)

        if good > 2 * ri.len() and good > bestinsts:
            bestlastgood = lastgood
            bestrect = ri
            bestrectinsts = insts
            bestinsts = good + ri.len()
            bestrlen = ri.len()

    rlen = max_rect

    # Try to find a new one.
    # loop over rect lengths
    for rlen in range(1,max_rect+1):
        if e0 + rlen > len(l.exts): break
        
#        print "rlen %d" % (rlen)

        # Special case (e.g. ST318451FC):
        # don't do run compression of unit rects.
        
        ri = getLeaf(l,e0,rlen)
        if not ri:
            break

#        print "rlen %d cylwidth %d lbnwidth %d" % (rlen, cylwidth, lbnwidth)
        
        # loop over instances -- keep going until it
        # breaks or we get to the end

        (good, insts, lastgood) = tryRect(l, ri, e0)
        if good > 2 * rlen:
            if good >= bestinsts:
#                print "rlen %d better (or smaller) than %d (good %d)" % (rlen, bestrlen, good)
                bestlastgood = lastgood
                bestrect = ri
                bestrectinsts = insts
                bestinsts = good # + rlen
                bestrlen = rlen
#            else:
#               print "rlen %d ok (good %d)" % (rlen, good)
#c        else:
#            print "rlen %d bad %d" % (rlen,good)


    if bestrect != None and bestinsts > bestrect.len():
#        print "Best rect len %d matched %d exts" % (bestrlen, bestinsts)

#        print "Rect:"
#        for (e,cylrunlen,runlen) in bestrect.ents:
#            print "ext %s runlen %d cylrunlen %d" % (e,runlen,cylrunlen)

#        print "Insts:"
#        for i in bestrectinsts:
#            if i.rect == None:
#                print "junk %s" % (i.start)
#            else:
#                print i.start

#        print bestrectinsts[0].start
#        print bestrectinsts[-1].start

        return (bestrect, bestrectinsts, bestlastgood)
    else:
        return None
    

# find the point between low and high where it switches from
# "bad" to "good"




def printIDX(l, op, insts, e0, newidx):
    lowcyl = reduce(lambda x,y: min(x,y.cyl), op.exts, op.exts[0].cyl)
    highcyl = reduce(lambda x,y: max(x,y.cyl), op.exts, op.exts[-1].cyl)
    lowlbn = reduce(lambda x,y: min(x,y.lbn), op.exts, op.exts[0].lbn)
    highlbn = reduce(lambda x,y: max(x,y.lbn), op.exts, op.exts[-1].lbn)

#    print "printIDX (newidx %d)" % (newidx)
#    print "lowcyl %d highcyl %d l0 %d ln %d" % (lowcyl,highcyl,lowlbn,highlbn)

    if newidx:
        ln = []
        for j in op.ents:
#            print "foo %s" % (j)
            (ei,lbnlen,cyllen) = j
            li = [ ei.lbn - lowlbn,     # loff \
                    ei.cyl - lowcyl,     # coff \
                    0.0,                 # aoff \
                    ei.tp[1] - ei.tp[0], # llen \
                    1,                   # clen \
                    0.0,                 # alen \
                    lbnlen,              # lrunlen \ 
                    cyllen,              # cylrunlen \
                    "TRACK",
                    idx(ei.tp, lin.tps),\
                    ei.head ]

#            print li
            ln += li
    else:
        ln = []

    
        #    ei = op.ents[0][0]
    l0 = l.exts[e0].lbn - (op.ents[0][0].lbn - lowlbn)
    c0 = l.exts[e0].cyl - (op.ents[0][0].cyl - lowcyl)
    return ([l0, c0, 0.0, op.lbnwidth, op.cylwidth, 0.0,\
             insts * op.lbnwidth, insts * op.cylwidth, "IDX", 0 ], ln)

      
def makejunkrent(l):
    lowcyl = reduce(lambda x,y: min(x,y.cyl), l.exts, l.exts[0].cyl)
    highcyl = reduce(lambda x,y: max(x,y.cyl), l.exts, l.exts[-1].cyl)
    lowlbn = reduce(lambda x,y: min(x,y.lbn), l.exts, l.exts[0].lbn)
    highlbn = reduce(lambda x,y: max(x,y.lbn), l.exts, l.exts[-1].lbn)

    llen = reduce(lambda x,y: x + y.tp[1] - y.tp[0], l.exts, 0)

#    print "makejunkrect"
    print "lowcyl %d highcyl %d l0 %d ln %d llen %d" % (lowcyl,highcyl,lowlbn,highlbn, llen)

    ln = []

    for ei in l.exts:
#        print ei
        extlen = ei.tp[1] - ei.tp[0]
        li = [ ei.lbn - lowlbn, # loff \
                ei.cyl - lowcyl, # coff \
                0.0,    # aoff \
                extlen, # llen \
                1,      # clen \
                0.0,    # alen \
                extlen, # lrunlen \ 
                1,      # cylrunlen \
                "TRACK",
                idx(ei.tp, lin.tps),\
                ei.head ]

        ln += li

    return ([ lowlbn, lowcyl, 0.0, \
              llen, highcyl - lowcyl + 1, 0.0, \
              llen, highcyl - lowcyl + 1, "IDX", 0 ], ln)


root = []
def outer(l):
    # result list of index nodes 
    idxen = [] 
    i = 0
    junkleaf = None

    while i < len(l.exts):
        res = findRect(l, i, lout.rects)
        if res != None:
            if junkleaf:

                (rent,li) = makejunkrent(junkleaf)
#                print "junk rent %d %d" % (rent[3], rent[6])
#                print rent
                rent[-1] = len(lout.rects)
#                print "Junk leaf (len %d)" % (len(junkleaf.exts))


                root.append(rent)
                assert(li != [])
                idxen.append(libparam.List(li, 11))
                # placeholder for idx
                lout.rects.append(layout.Rect([]))
                junkleaf = None
                
            (r,inst,lastgood) = res

            newidx = not(r in lout.rects)
            if newidx:
                lout.rects.append(r)

#            for e in r.exts: print e
            (rent,li) = printIDX(lout, r, len(inst), i, newidx)

            rent[-1] = idx(r,lout.rects)
            
            if newidx:
                assert(li != [])
                idxen.append(libparam.List(li, 11))
            root.append(rent)


            i = lastgood + 1
#            print "next ext %d" % (i)
        else:
            if not junkleaf:
                junkleaf = layout.Rect([l.exts[i]])
            else:
                junkleaf.exts.append(l.exts[i])
            
#            print "gave up on %s" % (l.exts[i])
            i += 1
    
        sys.stdout.flush()

    if i > lastgood+1:
        junkleaf = layout.Rect(l.exts[lastgood+1:i])
        (rent,li) = makejunkrent(junkleaf)
#        print "junk rent %d %d" % (rent[3], rent[6])
        rent[-1] = len(lout.rects)
        assert(li != [])
        idxen.append(libparam.List(li, 11))
        root.append(rent)

    ln = []
    for i in range(0,len(root)):

        (loff,coff,aoff,llen,clen,alen,lrunlen,crunlen,childtype,child) = root[i]
        if i == len(root) - 1:
            slips = reduce(lambda x,y: x+y.tp[1], lin.slips, 0)
            slips *= 2 # XXX magic
            llen += slips
            lrunlen += slips
            
        ln += [loff,coff,aoff,llen,clen,alen,lrunlen,crunlen,childtype,child]

    assert(ln != [])
    idxen.append(libparam.List(ln, 10))

    return idxen



import libparam

# dm_disk block
dmdisk = libparam.Block(typename = "dm_disk",\
                        name = ("%s.model" % cname),\
                        d = {"Block count" : lin.lbns,\
                             "Number of data surfaces" : lin.heads,\
                             "Number of cylinders" : lin.cyls,
                             "Layout Model" : libparam.Source("layout.model")})

# dm_layout_g4 block
lb = {}
tps = []
for (off,count) in lin.tps.keys():
    spt = count  # XXX fake
    tps += [off,count-1,spt]

lb["TP"] = libparam.List(tps, 3)
idx = outer(lin)

lb["IDX"] = libparam.List(idx)

slips = []
for s in lin.slips:
    lbn = s.lbn
    count = s.tp[1]
    slips += [lbn,count]


# l is a list of (src ext, dest ext) pairs
def DoRemaps(l):
    result = []
    for (src,dest) in l:
        len = src.tp[1]
        result += [ src.lbn, len,
                    dest.cyl, dest.head, dest.tp[0], len] # fake!

   
    return result

lb["Slips"] = libparam.List(slips, 2)
lb["Remaps"] = libparam.List(DoRemaps(lin.defects))

layoutg4 = libparam.Block(typename="dm_layout_g4",
                          d = lb)


loutfile.write(libparam.marshal(layoutg4))
moutfile.write(libparam.marshal(dmdisk))


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


DISKSIM=../src/disksim
SYSSIM=../src/syssim
RMS=../src/rms

VALIDATE_DISKS = atlas_III \
barracuda \
cheetah4LP \
cheetah9LP \
ibm18es \
atlas10k \
hp_c2247a \
hp_c3323a \
hp_c2490a \
st41601n \
dec_rz26

VALIDATE_OUTV = $(VALIDATE_DISKS:%=%.outv)

SYNTH_SETUPS = syntharrays \
synthcachedev \
synthcachemem \
synthclosed \
synthopen \
synthmixed \
synthraid5 \
synthsimpledisk \
3disks \
3disks2

SYNTH_OUTV = $(SYNTH_SETUPS:%=%.outv)

MEMS_SETUPS = mems.g1 \
mems.g2 \
mems.g3 \
mems.g1.nospring \
mems.g2.nospring \
mems.g3.nospring

MEMS_OUTV = $(MEMS_SETUPS:%=%.outv)

all: $(VALIDATE_OUTV) $(SYNTH_OUTV)

clean:
	rm -f $(VALIDATE_OUTV) $(SYNTH_OUTV) $(MEMS_OUTV)

distclean: clean
	rm -f *~

$(SYNTH_OUTV): %.outv: %.parv $(DISKSIM) statdefs
	$(DISKSIM) $*.parv $@ ascii 0 1
	@grep "IOdriver Response time average" $@

$(VALIDATE_OUTV): %.outv: %.parv $(DISKSIM) %.trace statdefs
	$(DISKSIM) $*.parv $@ validate $*.trace 0
	$(RMS) $@ $@ -1 -1 1

hplajw.outv: hplajw.parv $(DISKSIM) ajw.1week.srt statdefs
	$(DISKSIM) hplajw.parv $@ hpl ajw.1week.srt 0
	@grep "IOdriver Response time average" $@

ascii.outv: ascii.parv ascii.trace $(DISKSIM) statdefs
	$(DISKSIM) ascii.parv $@ ascii ascii.trace 0
	@grep "IOdriver Response time average" $@

syssim.outv: st41601n.parv $(SYSSIM)
	$(SYSSIM) st41601n.parv $@ 2676846 0
	@grep "IOdriver Response time average" $@

syssim2.outv syssim2.outv2: st41601n.parv $(SYSSIM)
	$(SYSSIM) st41601n.parv $@ 2676846 1
	@grep "IOdriver Response time average" syssim2.outv
	@grep "IOdriver Response time average" syssim2.outv2


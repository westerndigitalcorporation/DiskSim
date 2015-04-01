# DIXtrac - Automated Disk Extraction Tool
# Authors: Jiri Schindler, John Bucy, Greg Ganger
# *
# Copyright (c) of Carnegie Mellon University, 1999-2005.
# *
# Permission to reproduce, use, and prepare derivative works of
# this software for internal use is granted provided the copyright
# and "No Warranty" statements are included with all reproductions
# and derivative works. This software may also be redistributed
# without charge provided that the copyright and "No Warranty"
# statements are included in all redistributions.
# *
# NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
# CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
# EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
# TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
# OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
# MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
# TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.
#
# Makefile to control disk parameters extraction and characterization.
#
# make 'all' or 'extract' to characterize the disk, yielding DiskSim 
# diskspecs files.
# make 'install' to install the data into diskdbms.
# make 'validate' to do RMS validation in the DiskSim style of comparing
# measured and modeled parameters. See the DiskSim manual for more 
# details.

##########################################################################
# Before proceeding, set the values for these five variables 
##########################################################################

# Disk information: can be obtained by dx_stat program

VENDOR    =
PRODUCT   =
SERIALNUM =
NAME    =               # canonical name (e.g., Quantum-AtlasV)
DEV     =               # SG device (e.g., /dev/sg3)

DS_PATH =   # path to the root directory with Disksim files  

# Optional - set this target directory for the final copy of specs if 
#            using the "install" target
INSTALLDIR = /tmp

##########################################################################
# No need to edit anything below this line
##########################################################################
-include .diskdbms.vars

# remove any trailing/leading spaces from the name
FULLNAME = $(strip $(VENDOR))_$(strip $(PRODUCT))_$(strip $(SERIALNUM))

# remove any trailing/leading spaces
DISKSIM = $(strip $(DS_PATH))/src/disksim
RMS = $(strip $(DS_PATH))/src/rms
DS_UT_PATH = $(strip $(DS_PATH))/utils/params
GEN_PARV = $(DS_UT_PATH)/gen_parv.pl
GROK_OUTV = $(strip $(DS_PATH))/utils/analysis/grok_outv.pl
REQREQ = $(strip $(DS_PATH))/utils/analysis/reqreq.pl

DX_PATH = $(strip $(DS_PATH))/dixtrac
DX_LAYOUT = $(DX_PATH)/dx_layout
DX_SEEKS = $(DX_PATH)/dx_seeks
DX_MECH = $(DX_PATH)/dx_mech
DX_SKEWS_SIMPLE = $(DX_PATH)/dx_skews_simple
DX_SKEW_CURVE = $(DX_PATH)/dx_skew_curve
DX_REST = $(DX_PATH)/dx_rest
DXCACHECTL = $(DX_PATH)/dxcachectl
DXREPLAY = $(DX_PATH)/dxreplay
TEMPLATES = $(DX_PATH)/templates

DM_PATH = $(strip $(DS_PATH))/diskmodel
G4_TOOLS = $(DM_PATH)/layout_g4_tools
PASS0=$(G4_TOOLS)/pass0.py
SLIPS=$(G4_TOOLS)/simpleslips.py
REMAPS=$(G4_TOOLS)/remaps.py
G4_ANALYZE=$(G4_TOOLS)/g4_analysis.py
G4_SKEWS = $(G4_TOOLS)/g4_skews

GNUPLOT = gnuplot
PYTHON = python

MODEL = $(NAME).model
DISKSPEC = $(NAME).diskspecs
PARV = $(NAME).parv

DATE = $(shell date +"%Y\/%m\/%d %H:%M")

SEEKPLOT_DETAILED = seekcurve-detailed.ps
SEEKPLOT_COMPLETE = seekcurve.ps

##########################################################################
# High-level relevant targets. 
##########################################################################
all: extract

extract: $(MODEL) $(DISKSPEC) skews

validate: extract random.ps random.hist.ps mixed.ps mixed.hist.ps

graphs: validate seekplots

# Create the appropriate diskdbms directory and copy the files in.
# Write some log file or something with username, hostname, date.

# the name of the installed disk layout file 
INSTALL_LAYOUT = $(NAME)-layout.model

install: $(MODEL) $(DISKSPEC) $(PARV) skews seeks layout.mappings seekplots
	mkdir -p $(INSTALLDIR)
	cp seeks $(INSTALLDIR)/$(NAME).seek
	cp $(NAME)-skews-layout.model $(INSTALLDIR)/$(INSTALL_LAYOUT)
	cat $< | \
	sed -e "s/Full seek curve = seeks/Full seek curve = $(NAME).seek /" | \
	sed -e "s/.\/layout.model/$(INSTALL_LAYOUT)/" > $(INSTALLDIR)/$(MODEL)
        # now just copy over the rest of the data
	cp $(DISKSPEC) $(PARV)  $(INSTALLDIR)

        # copy the original layout.mappings (this can be a big file)
	cp layout.mappings $(INSTALLDIR)

        # copy seek curve plots
	cp $(SEEKPLOT_DETAILED) $(SEEKPLOT_COMPLETE) $(INSTALLDIR)

        # create a short readme/log with some information
	echo Created by user `whoami` on  `date` > $(INSTALLDIR)/README
	echo Host: `uname -n` \(`uname -s -p -o`\) >> $(INSTALLDIR)/README


##########################################################################
# Intermediate or internal targets. 
##########################################################################

# To understand the steps involved in charactirizing a disk, read the 
# comments in this Makefile from the bottom up.


##########################################################################
# 7. Extras
#
# Some additional useful scripts for creating seek profile, cleaning, 
# possibly intermediate, files etc.
##########################################################################
dxclean:
	-$(RM) -rf foo *.log seeks.1* *trace *.model $(PARV) $(DISKSPEC) 

layoutclean:
	-$(RM) -rf pass* *layout.model

clean:
	-$(RM) -rf *.log pass* seeks.1* *trace

# $(NAME).model - created as a result of dx_mech, 
#                 "Layout Model" refers to layout.model file
# layout.model  - symlink really, referenced from $(MODEL) file

.PHONY: seekplots

seekplots: $(SEEKPLOT_DETAILED) $(SEEKPLOT_COMPLETE) ;

$(SEEKPLOT_DETAILED): seekplot-detailed.gnuplot
	gnuplot $<

seekplot-detailed.gnuplot: $(TEMPLATES)/seekcurve.gnuplot.in seeks
	sed -e "s/@FILE@/$(SEEKPLOT_DETAILED)/" < $< | \
	sed -e "s/@CNAME@/$(NAME)/" | \
	sed -e "s/@DATE@/$(DATE)/" | \
	sed -e "s/@DETAILED@/Detailed /" | \
	sed -e "s/@XRANGE@/30/" | \
	sed -e "s/@YRANGE@/1.2/" > $@

$(SEEKPLOT_COMPLETE): seekplot.gnuplot
	gnuplot $<

seekplot.gnuplot: $(TEMPLATES)/seekcurve.gnuplot.in seeks
	sed -e "s/@FILE@/$(SEEKPLOT_COMPLETE)/" < $< | \
	sed -e "s/@CNAME@/$(NAME)/" | \
	sed -e "s/@DATE@/$(DATE)/" | \
	sed -e "s/@DETAILED@//" | \
	sed -e "s/@XRANGE@//" | \
	sed -e "s/@YRANGE@//" > $@

##########################################################################
# 6. Validation of the extracted parameters.
#
# The validation uses a variety of synthetically generated traces and 
# replays them against the real disk. It also generates a series of graphs
# to show visually how closely the trace replayed on the read disk 
# matches the results obtained from running a disksim simulation 
# configured with the just-extracted disk parameters.
##########################################################################

# Disksim overrides to disable the disk cache during validation trace replay.
NOCACHE = disk0 "Enable caching in buffer" 0 \
 disk0 "Fast write level" 0 \
 disk0 "Buffer continuous read" 0 \
 disk0 "Read any free blocks" 0 \
 disk0 "Minimum read-ahead (blks)" 0 \
 disk0 "Maximum read-ahead (blks)" 0

# Don't delete intermediate targets
.SECONDARY:

%.hist.gnuplot: $(TEMPLATES)/hist.gnuplot.in
	sed -e "s/@FILE@/$*.hist.ps/" < $< | \
	sed -e "s/@CNAME@/$(NAME)/" | \
	sed -e "s/@FULLNAME@/$(FULLNAME)/" | \
	sed -e "s/@HIST@/$*.hist/" | \
	sed -e "s/@WORKLOAD@/$*/" | \
	sed -e "s/@DATE@/$(DATE)/" > $@


%.hist.ps: %.hist.gnuplot %.hist
	$(GNUPLOT) $<

# Generate gnuplot scripts from the validation trace replay step.
# The RMS value (or root-mean-square, is a metric of similarity between
# the trace replayed on the real disk and one replayed on the disksim
# configured with the just-extracted disk parameters.
mixed.gnuplot: RMSVAL = $(shell cat mixed.rms)
mixed.gnuplot: RMSFILE = mixed.rms
random.gnuplot:  RMSVAL = $(shell cat random.rms)
random.gnuplot:  RMSFILE = random.rms
%.gnuplot: $(TEMPLATES)/cdf.gnuplot.in %.rms %.validate.cdf %.disksim.cdf
	sed -e "s/@FILE@/$*.ps/" < $<  | \
	sed -e "s/@DSCDF@/$*.disksim.cdf/"  | \
	sed -e "s/@TRACECDF@/$*.validate.cdf/" | \
	sed -e "s/@WORKLOAD@/$*/" | \
	sed -e "s/@NAME@/$(NAME)/" | \
	sed -e "s/@FULLNAME@/$(FULLNAME)/" | \
	sed -e "s/@RMS@/$(RMSVAL)/" | \
	sed -e "s/@DATE@/$(DATE)/" > $@

# Generate graphs with gnuplot, using the gnuplot scripts.
%.ps: %.gnuplot %.disksim.cdf %.validate.cdf %.rms
	$(GNUPLOT) $<

# 6d. Extract CDFs (cumm. distr. function) from the disksim output files. 
%.validate.cdf: %.outv
	$(GROK_OUTV) 'VALIDATE Trace access time distribution' < $< > $@

%.disksim.cdf: %.outv
	$(GROK_OUTV) 'IOdriver Physical access time distribution' < $< > $@

%.latency.cdf: %.outv
	$(GROK_OUTV) 'Disk Rotational latency distribution' < $< > $@

# Extract a histogram of request resp. times from the trace output file.
%.hist: %.exectrace %.trace
	$(REQREQ) $^ > $@

# Extract the RMS value from the disksim output file (.outv).
%.rms: %.outv 
	$(RMS) $< $< -1 -1 1 > $@

# 6c. Replay the random and mixed traces with disksim in validation mode.
#
# The disksim validation mode takes the trace obtaned from the replay
# on the real disk and replays them.
random.outv random.exectrace: CACHE = $(NOCACHE)
mixed.outv mixed.exectrace:
%.outv %.exectrace: %.trace # $(PARV)
	$(DISKSIM) $*.parv $*.outv validate $< 0 $(CACHE)

# 6b. Replay the synthetic trace on the real disk with DXREPLAY
# 
random.trace: CACHE=OFF
mixed.trace: CACHE=ON
skew.trace: CACHE=OFF
%.trace:  %.synthtrace
        # using grep - so that we don't print the entire Cache Mode Page
	$(DXCACHECTL) -d $(DEV) -c $(CACHE) | grep Disk
	$(DXREPLAY) -s -q 1 -m $* $(DEV)
	$(DXCACHECTL) -d $(DEV) -c ON | grep Disk        

# 6a. Generate synthetic traces for replay on the real disk and validation.
#
# The trace is generated using disksim's synthetic trace generator properly
# configured for random requests (50% reads and 50% writes) and mixed
# requests (a mix of sequential and "local" requests in addition to random
# requests. The mixed trace stresses the disk(model) caching modules.
random.synthtrace: CACHE = $(NOCACHE)
mixed.synthtrace: CACHE = 
%.synthtrace:  %.parv
	$(DISKSIM) $< outv ascii 0 1 $(CACHE)


##########################################################################
# 5. Generation of disk specifications file for disksim.
#
# Will all pieces in place and extracted, generate the diskspecs from a
# template file.
##########################################################################
.PHONY: diskspecs

diskspecs: $(PARV) skews

$(PARV):  $(NAME).diskspecs $(TEMPLATES)/parv.in
	$(GEN_PARV) $< foo bar < $(TEMPLATES)/parv.in > $@

%.parv:  $(NAME).diskspecs
	$(GEN_PARV) $< foo bar < $(TEMPLATES)/$@.in > $@


##########################################################################
# 4. Extraction of layout skews.
#
# First, run the skews detection algorithm program. The output of this 
# program is a trace that is then replayed against the real disk, using
# a special skew replay mode.
# With the generated output from the replay, validate the trace, 
# using DiskSim validation, against the diskmodel that includes the skews. 
# See the source code for more details on the algorithm.
##########################################################################

.PHONY: skews

skews: $(NAME)-skews-layout.model
	ln -sf $< layout.model

# To re-run skews calibrarion, delete the *-skews-layout.model file
$(NAME)-skews-layout.model: $(PARV) $(DISKSPEC) skew.trace
	$(G4_SKEWS) --mode=calib --parv=$(PARV) --outv=/dev/null --trace=skew.trace --model=$@ > $@.log 2>&1

skew.synthtrace: $(DISKSPEC)
	$(G4_SKEWS) --mode=gentrace --parv=$(PARV) --outv=/dev/null --trace=$@ >$@.log 2>&1


##########################################################################
# 3. Extraction of cache and overhead parameters with DX_REST program.
#
# Once the G4 layout file and seek profile exists, we can extract various
# disk parameters such as cache behavior, prefetching and command
# processing overheads. 
##########################################################################

$(DISKSPEC): $(MODEL)
	$(DX_REST) -d $(DEV) -m $(NAME).model -o $(NAME).diskspecs --full-name $(FULLNAME)

$(MODEL): base-$(MODEL) seeks
	$(DX_MECH) -d $(DEV) -l base-$(MODEL) -o $@ -s seeks > $@.log 2>&1


##########################################################################
# 2. Gereration of the disk seek profile. 
#
# Generate a series of seek time profiles for different cylinder distance
# ranges (cyl through cylmax) with varying granularity (cylstep).
##########################################################################

seeks: seeks.1 seeks.100 seeks.1000 seeks.10000
	cat $^ > $@

seeks.10000: base-$(MODEL)
	$(DX_SEEKS) --mode=mtbrc --cyl=10000 --cylstep=1000 -d $(DEV) -l $< -s $@

seeks.1000:  base-$(MODEL)
	$(DX_SEEKS) --mode=mtbrc --cyl=1000 --cylmax=10000 --cylstep=100 -d $(DEV) -l $< -s $@

seeks.100:  base-$(MODEL)
	$(DX_SEEKS) --mode=mtbrc --cyl=100 --cylmax=1000 --cylstep=10 -d $(DEV) -l $< -s $@

seeks.1: base-$(MODEL)
	$(DX_SEEKS) --mode=mtbrc --cyl=1 --cylstep=1 --cylmax=100 -d $(DEV) -l $< -s $@


##########################################################################
# 1. Extraction and generation of the disk layout.
#
# First step in obtaining the LBN to PBN (logical to physical locations)
# mappings is to get the raw layout - translating every LBN to its physical
# locatation (generation of layout.mappings file). 
# Once the raw layout mappings are extracted from the disk, the pass0 
# through pass2 rules run pattern detection algorithms to describe the raw 
# layout mappings in a cannonical format, called G4 layout. 
##########################################################################

base-layout.model base-$(MODEL): pass2
	PYTHONPATH=$(G4_TOOLS) $(PYTHON) $(G4_ANALYZE) $< base-layout.model base-$(MODEL) $(NAME)
	ln -sf base-layout.model layout.model

pass2: pass1
	PYTHONPATH=$(G4_TOOLS) $(PYTHON) $(SLIPS) $< $@

pass1: pass0
	PYTHONPATH=$(G4_TOOLS) $(PYTHON) $(REMAPS) $< $@

pass0: layout.mappings
	PYTHONPATH=$(G4_TOOLS) $(PYTHON) $(PASS0) $@ < $<

# Extraction of the raw layout. This can take many hours. 
layout.mappings:
	$(DX_LAYOUT) -d $(DEV) -o layout.mappings

# Uncomment the next line if you dont' want to delete the intermediate 
# targets on a crash
# .PRECIOUS: layout.mappings

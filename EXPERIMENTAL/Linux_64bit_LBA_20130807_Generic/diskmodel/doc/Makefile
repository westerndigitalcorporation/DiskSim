# default targets to make for make all
TARGETS = dm_manual.pdf dm_manual.ps

###########################################################################
## Do not modify anything beyond this point                              ##
###########################################################################

export TEXINPUTS=.:$(shell pwd)/../modules/::

####################
## Change log:
## 01/07/21 - SRCFILES now created w/ wildcard function instead of
##            requiring all sources to be listed individually. Problem is
##            the modification of draft.tex will rebuild submit.dvi, but
##            that's not a big deal.

####################
## Bugs:
## If I use the code for only generating refdbms if the .aux differs, there
## is the problem that when make exits, it deletes the .bbl since it was an
## intermediate target.

#-- Programs used when runing latex etc.                       
# note: REFDIR comes from the environment, not set in Makefile
BIBTEX = bibtex -terse
LATEX = latex -interaction=batchmode
DVIPS = dvips -tletter
PS2PDF = ps2pdf

SRCFILES = $(wildcard *.tex)

#--------------------------------------------------------------------------
# Production/target rules                       
#--------------------------------------------------------------------------
.SUFFIXES:
.PHONY: clean all bibclean veryclean
.SECONDARY: %.bib

all: $(TARGETS)

# make sure that the latex files have been created from the modspec files
modspecs:
	make -C ../

#-- Creation of initial .aux file
%.aux: %.tex $(SRCFILES) modspecs
	$(LATEX) $<

#-- Creation of refdbms's .bib file
%.bib: %.aux
#   may fail if there are no citations yet, so ignore errors
	touch $@

#-- Create .bbl from .bib
%.bbl: %.bib %.aux
#	-$(BIBTEX) $*
	$(LATEX) $*.tex

#-- Build a dvi... 
%.dvi: %.tex %.bbl 
	$(LATEX) $<
#   Pull warnings (about citations and references) out of the log files
	@echo ''
	@echo '*=*=*=*=*=*=*=*=*=*=*=*  LaTeX Warnings  =*=*=*=*=*=*=*=*=*=*=*=*=*'
	@grep 'LaTeX Warning:' $(<:.tex=.log) || true
#	@echo '*=*=*=*=*=*=*=*=*=*=*=*  BibTeX Warnings  *=*=*=*=*=*=*=*=*=*=*=*=*'
#	@grep 'Warning--' $(<:.tex=.blg) || true
	@echo '*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*'
	@echo ''

#-- converting .dvi to .ps...
%.ps: %.dvi
	$(DVIPS) -o $@ $<

#-- converting .dvi to .pdf.
%.pdf: %.dvi
	$(DVIPS) -Ppdf -G0 -o - $< | $(PS2PDF) - $@

clean:
	rm -f *~ *.dvi *.ps *.pdf *.log *.aux *.blg *.bbl *.out



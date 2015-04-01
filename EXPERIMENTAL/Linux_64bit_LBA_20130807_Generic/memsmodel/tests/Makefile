
MEMS_SEEKTEST_DIR = ..

all: experiments

experiments:
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x 0 -y 0 -spring 0.75 -num 1.0 -outfile one -mode math -step 100
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x -1000 -y -1000 -spring 0.75 -num 1.0 -outfile one -mode math -step 100
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x 0 -y 0 -spring 0.75 -num 0.0 -outfile zero -mode math -step 100
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x -1000 -y -1000 -spring 0.75 -num 0.0 -outfile zero -mode math -step 100

hong_experiments:
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x 0 -y 0 -spring 0.75 -num 1.0 -outfile one_hong -mode math -step 100 -hong
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x -1000 -y -1000 -spring 0.75 -num 1.0 -outfile one_hong -mode math -step 100 -hong
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x 0 -y 0 -spring 0.75 -num 0.0 -outfile zero_hong -mode math -step 100 -hong
	$(MEMS_SEEKTEST_DIR)/mems_seektest.pl -x -1000 -y -1000 -spring 0.75 -num 0.0 -outfile zero_hong -mode math -step 100 -hong

graphs:
	cat generate_graphs.txt | math > /dev/null

graphs.ps: graphs.tex
	latex graphs.tex
	dvips graphs.dvi

clean:

distclean:
	rm -f *.eps *.spring.* *.delta.* *~ graphs.aux graphs.dvi graphs.log graphs.ps

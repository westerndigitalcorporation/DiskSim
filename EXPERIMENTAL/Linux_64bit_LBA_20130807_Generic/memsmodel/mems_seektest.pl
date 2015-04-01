#!/usr/bin/perl

use Getopt::Long;

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#
#  mems_seektest.pl - a wrapper around mems_seektest which gathers
#    seek time data for MEMS-based storage devices.  it can output in
#    several different formats:  raw data, ps or eps files using gnuplot
#    or data formatted for mathematica to produce 3d surface plots.  it
#    can also use gnuplot to display graphs directly to the screen
#
#    the script has many options:
#
#    x      - starting X position (0)
#    y      - starting Y position (0)
#
#    spring - spring factor (0.0)
#    num - number of settling constants to add to X (0.0)
#
#    outfile - specifies the base filename for the output files
#    mode - output mode
#           all modes - produce a raw data file
#                  <outfile>.x.y
#           post - produces five files using gnuplot in postscript format
#                  <outfile>.spring.<x>.<y>.ps - spring_factor set to <spring>
#                  <outfile>.zero.<x>.<y>.ps - spring_factor set to 0
#                  <outfile>.delta.<x>.<y>.ps - difference between the last 2 graphs
#                  <outfile>.x_error.<x>.<y>.ps - velocity error in the X direction
#                  <outfile>.y_error.<x>.<y>.ps - velocity error in the Y direction
#           eps - produces the same five files using gnuplot in eps format
#           screen - produce the same five graphs directly on screen using gnuplot and xv
#           math - produces two raw data files to be plotted with Mathematica
#                  <outfile>.spring.<x>.<y> - data for spring factor set to <spring>
#                  <outfile>.delta.<x>.<y> - data for difference between spring_factor set to <spring> and 0
#
#    step - the number of bit positions to step for each point in the graph.  (100)
#    loop - this is used to do seek graphs for all starting positions, stepping by whatever
#           <loop> is set to.  (unset)
#    transpose - if this is set, then x and y are reversed
#
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

%optctl = ();
GetOptions(\%optctl, "mode=s", "x=i", "y=i", "num=f", "spring=f", "outfile=s", "step=i", "loop=i", "transpose", "hong");

if ($optctl{"mode"}) {
  $MODE = $optctl{"mode"};
} else {
  die "must set -mode";
}

$GNUPLOT = "gnuplot";
$XV = "/usr/X11R6/bin/xv";
$GNUPLOT_FILE = "gnuplottemp";
$SEEK_TEST2 = "./mems_seektest";
# $SEEK_TEST2 = "./mems_seektest";
$TEST_FILENAME = "testme";

$SPRING_FACTOR = 0.0;
$STEP = 100;
$NUM = 0.0;
$x = 0;
$y = 0;
$TRANSPOSE = "";
$HONG = "";

if ($optctl{"x"}) {$x = $optctl{"x"}};
if ($optctl{"y"}) {$y = $optctl{"y"}};
if ($optctl{"spring"}) {$SPRING_FACTOR = $optctl{"spring"}};
if ($optctl{"outfile"}) {$TEST_FILENAME = $optctl{"outfile"}};
if ($optctl{"step"}) {$STEP = $optctl{"step"}};
if ($optctl{"num"}) {$NUM = $optctl{"num"}};
if ($optctl{"transpose"}) {$TRANSPOSE = "-transpose"};
if ($optctl{"hong"}) {$HONG = "-hong"};

if ($MODE eq "eps") {
  $FILE_GRAPH_OUTPUT = "set term post eps";
  $EXTENSION = "eps";
} elsif ($MODE eq "post") {
  $FILE_GRAPH_OUTPUT = "set term post";
  $EXTENSION = "ps";
}

$SCREEN_GRAPH_OUTPUT = "set term gif";

if ($optctl{"loop"}) {
  for($x = 0; $x <= 2000; $x += $optctl{"loop"}) {
    for($y = 0; $y <= 2000; $y += $optctl{"loop"}) {
      run_seek_test($x, $y);
    }
  }
}
else {
  run_seek_test($x, $y);
}

exit(0);

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

sub run_seek_test{

  local($x, $y) = @_;

  if ($MODE eq "math") {
    #  print "$SEEK_TEST2 -math 1 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE > $TEST_FILENAME.spring.$x.$y\n";
    system("$SEEK_TEST2 -math 1 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE $HONG > $TEST_FILENAME.spring.$x.$y");
    #  print "$SEEK_TEST2 -math 2 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE > $TEST_FILENAME.delta.$x.$y\n";
    system("$SEEK_TEST2 -math 2 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE $HONG > $TEST_FILENAME.delta.$x.$y");
  } else {
    #  print "$SEEK_TEST2 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE > $TEST_FILENAME.$x.$y\n";
    system("$SEEK_TEST2 -x $x -y $y -spring $SPRING_FACTOR -step $STEP -num $NUM $TRANSPOSE $HONG> $TEST_FILENAME.$x.$y");
  }

  if(($MODE eq "post") || ($MODE eq "eps") || ($MODE eq "screen")) {
    plot_data($x, $y);
  }
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

sub plot_graph{
  local ($x, $y, $system_call_filename, $titlebit, $zlabel, $using) = @_;

  local($setterm, $system_call);

  if ($MODE eq "screen") {
    $setterm = $SCREEN_GRAPH_OUTPUT;
    $system_call = "$GNUPLOT $GNUPLOT_FILE | $XV - &";
  } else {
    $setterm = $FILE_GRAPH_OUTPUT;
    $system_call = "$GNUPLOT $GNUPLOT_FILE > $system_call_filename";
  }

  $gnuplot_data=<<EOF;
$setterm
#set title \'$titlebit, num time constants $NUM\'
set xlabel '\X position (bit)'\
set ylabel '\Y position (bit)'\
set zlabel '\ $zlabel '\
splot \'$TEST_FILENAME.$x.$y\' using $using notitle
EOF
  ;

  generate_plot($gnuplot_data, $system_call);

}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

sub plot_data{
  local($x, $y) = @_;

  local($setterm, $system_call);

  plot_graph($x, $y,
	     "$TEST_FILENAME.spring.$x.$y.$EXTENSION",
	     "Seek curve from ($x, $y), spring factor $SPRING_FACTOR",
	     "Seek time (ms)",
	     "1:2:3");

  plot_graph($x, $y,
	     "$TEST_FILENAME.zero.$x.$y.$EXTENSION",
	     "Seek curve from ($x, $y), spring factor 0.0",
	     "Seek time (ms)",
	     "1:2:4");

  plot_graph($x, $y,
	     "$TEST_FILENAME.delta.$x.$y.$EXTENSION",
	     "Seek time delta from ($x, $y), spring factor $SPRING_FACTOR",
	     "Difference in seek time (ms)",
	     "1:2:5");

#  plot_graph($x, $y,
#	     "$TEST_FILENAME.x_error.$x.$y.$EXTENSION",
#	     "X switchpoint error from ($x, $y), spring factor $SPRING_FACTOR",
#	     "Switchpoint error (percent)",
#	     "1:2:6");

#  plot_graph($x, $y,
#	     "$TEST_FILENAME.y_error.$x.$y.$EXTENSION",
#	     "Y switchpoint error from ($x, $y), spring factor $SPRING_FACTOR",
#	     "Switchpoint error (percent)",
#	     "1:2:8");
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

sub generate_plot{
  local($gnuplot_data, $system_call) = @_;

  open GPFH, ">$GNUPLOT_FILE" or die "cannot open file $GNUPLOT_FILE";
  print GPFH $gnuplot_data;
  close GPFH;
  system($system_call);
  system("sleep 2");
#    print "rm $GNUPLOT_FILE\n";
  system("rm $GNUPLOT_FILE");
#  system("sleep 2");
}

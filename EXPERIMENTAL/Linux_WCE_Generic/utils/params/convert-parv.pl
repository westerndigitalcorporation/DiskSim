#!/usr/bin/perl

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


# convert disksim v2 parv files to v3 format
# provide such a file on STDIN; new file written to STDOUT

# $_[0] is the bus list, $_[1] is the controller list, 
# $_[2] is the bus to start with
# $_[3] is a list of things we've already looked at 
# $_[4] is the depth for indenting
sub outputBusTopo {
    my $c = 0;

    for $i (1 .. $_[4]) { print " "; }
    print("disksim_bus $_[2] [ \n");
    $_[2] =~ /bus([0-9]+)/;
#    print STDERR "busnum $1\n";
    $aref = $_[0]->[$1];


    foreach $child (@$aref) {
	if($c) {
	    print ",\n";
	}
	$vref = $_[3];
	foreach $v (@$vref) { 
	    if($v eq $child) { 
#		print STDERR "avoiding visited thing: $child\n"; 
		$bad = 1; 
		last;
	    } 
	    else { 
#		print STDERR "visited $v\n"; 
		$bad = 0; 
	    }
	}
	push(@$vref, $child);

	if($bad != 1) {
#	    print STDERR "buschild $child\n";
	    if($child =~ /ctlr.*/) {
		outputCtlrTopo($_[0], $_[1], $child, $_[3], $_[4]+5);
		$c++;
	    }
	    else {

		$child =~ /(mems2|disk|sd)(.*)/;
	        for $i (1 .. ($_[4] + 5)) { print " "; }
		$devtype = $1;
		$devtype =~ s/sd/simpledisk/;
		$devtype = "disksim_$devtype";
		print "$devtype $child []";
		$c++;
	    }
	}
    }
    print "\n";

    for $i (1 .. $_[4] + 5) { print " "; }
    print "# end of $_[2]\n";

    for $i (1 .. $_[4]) { print " "; }
    print "]"; 
}


sub outputCtlrTopo {
    for $i (1 .. $_[4]) { print " "; }
    print("disksim_ctlr $_[2] [ \n");
    $_[2] =~ /ctlr([0-9]+)/;
#    print STDERR "ctlrnum $1\n";
    $aref = $_[1]->[$1];
    my $c = 0;
    foreach $child (@$aref) {
	if($c) {
	    print ",\n";
	}
	$vref = $_[3];
	foreach $v (@$vref) { 
	    if($v eq $child) { 
#		print STDERR "avoiding visited thing: $child\n"; 
		$bad = 1; 
		last;
	    } 
	    else { 
#		print STDERR "visited $v\n"; 
		$bad = 0; 
	    }
	}
	push(@$vref, $child);

	if($bad != 1) {
#	    print STDERR "ctlrchild $child\n";	
	    outputBusTopo($_[0], $_[1], $child, $_[3], $_[4] + 5);

	    $c++;
	}
    }
    print "\n";

    for $i (1 .. $_[4] + 5) { print " "; }
    print "# end of $_[2]\n";

    for $i (1 .. $_[4]) { print " "; }
    print("]");

}


sub error {
    print STDERR "*** error: $_[0]\n";
    exit(1);
}


# clean out empty lines, etc.
$linenum = 1;
while (<STDIN>) {
    $line = $_;
    if(($line =~ /^\s*$/) || ($line eq "") || ($line eq undef)) { }
    else {
        # mash whitespace together
	$line =~ s/\s+/ /g;
	# eat trailing whitespace
	$line =~ s/\s*$//;
	$line =~ s/Print stats(.*):/Print stats = /;
	push(@lines, $line);
	push(@linenums, $linenum);
#	print("$line\n");
    }
    $linenum++;
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Byte Order.*: (Big|Little)/) {
    print("disksim_global Global { \n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Init Seed: ([0-9]+|TIME)/) {
    $line =~ s/:/ =/;
    print("   $line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Real Seed: ([0-9]+|TIME)/) {
    $line =~ s/:/ =/;
    print("   $line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Statistic warm-up period: ([0-9]+(\.[0-9]+)?).*/) {
    $line =~ s/:/ =/;
    print("   # $line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Checkpoint to ([^\s]+) every: ([0-9]+(\.[0-9]+)?) (seconds|I\/Os)/) 
{
    $checkpointfile = $1;
    $checkpointfreq = $2;
    $checkpointunit = $3;
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Stat \(dist\) definition file: ([^\s]+)/) {
    $statdefs = $1;
    print("   Stat definition file = $statdefs\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Output file for.*/) {
    $line =~ s/:/ =/;
    print("   # $line\n}\n\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /I\/O Subsystem Input.*/) 
{
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /-*/) {

}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /PRINTED I\/O SUBSYSTEM.*/) {
    print("disksim_stats Stats {\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver size stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("iodriver stats = disksim_iodriver_stats {\n$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver locality stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver blocking stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver interference stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver queue stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver crit stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver idle stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver intarr stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver streak stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver stamp stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print driver per-device stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line },\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print bus idle stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("bus stats = disksim_bus_stats {\n$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print bus arbwait stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line },\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller cache stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("ctlr stats = disksim_ctlr_stats {\n$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller size stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller locality stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller blocking stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller interference stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller queue stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller crit stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller idle stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller intarr stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller streak stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller stamp stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print controller per-device stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line },\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device queue stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("device stats = disksim_device_stats {\n$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device crit stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device idle stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device intarr stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device size stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device seek stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device latency stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device xfer stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device acctime stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device interfere stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line,\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Print device buffer stats\? (0|1)/) {
    $line =~ s/\?/ =/;
    print("$line },\n");
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


# find the procflow stats
$gotprocflowstat = 0;
print("process flow stats = disksim_pf_stats {\n");
for $c (0 .. ($#lines - 1)) {
    if($lines[$c] =~ /PRINTED PROCESS-FLOW STATISTICS/) {
       for $d (1..4) {
          $lines[$c + $d] =~ s/\?/ = /;
	  print("$lines[$c+$d]");
	  if($d != 4) { print(","); }
	  print("\n");
       }

       $gotprocflowstat = 1;
    }
}
if(! $gotprocflowstat) {
   error("couldn't find process flow statistics specs");
}
print("}\n");

print("} # end of stats block\n\n\n");


#
# GENERAL I/O SUBSYSTEM PARAMETERS
#

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /GENERAL I\/O SUBSYSTEM PARAMETERS/) 
{
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


# we build up a string of this and print it later;
# it has to follow component instantiation because
# iomaps refer to devices and this will fail to load
# if the devices don't exist
$iomaps = $iomaps . "iosim IS {\n";

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /I\/O Trace Time Scale: (.*)/) {
    $line =~ s/:/ =/;
    $iomaps = $iomaps . "     $line";
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of I\/O Mappings: ([0-9]+)/) {
    $numiomaps = $1;
}

if($numiomaps) {
    $iomaps = $iomaps . ",\n";
    $iomaps = $iomaps . "     I/O Mappings = [ \n";
    for($c = 0; $c < $numiomaps; $c++) {
	$line = shift(@lines); $linenum = shift(@linenums);
	print STDERR "IOMAP: $line\n";
	if($c) { $iomaps = $iomaps . ",\n"; }
	@m = split / /, $line;
	shift @m;
	# translate devno to devnames ?
	# we're just going to say disk and if they wanted something else,
	# they can fix it up.
	$simdev = "disk" . hex($m[1]);
	$iomaps = $iomaps . "     iomap { tracedev = 0x$m[0], simdev = $simdev, locScale = $m[2], sizeScale = $m[3], offset = $m[4] }";
    }
    $iomaps = $iomaps . "     ]  # end of iomap list\n";
}
else {
    print "   # no iomaps\n";
}

$iomaps = $iomaps . "}  # end of iosim spec\n\n";

#
# COMPONENT SPECIFICATIONS
#

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /COMPONENT SPECIFICATIONS/) 
{
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}


#
# Device drivers
#


$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of device drivers: ([0-9]+)/) 
{
    $numdrivers = $1;
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

for($c = 0; $c < $numdrivers; $c++) {


    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /\Device Driver Spec #.*/)
    {
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }


    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /\# drivers with Spec: ([0-9]+).*/)
    {
	if($1 > 1) {
	    $maxdrv = $c + $1;
	    $instances{"driver$c .. driver$maxdrv"} = "DRIVER$c";
	}
	else {
	    $instances{"driver$c"} = "DRIVER$c";
	}
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Device driver type.*/)
    {
	$line =~ s/Device driver type:/type =/;
	print("disksim_iodriver DRIVER$c {\n  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Constant access time.*/)
    {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Scheduling policy.*/)
    {
	$line =~ s/:/ =/;
        print("  Scheduler = disksim_ioqueue {\n");
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Cylinder mapping strategy.*/)
    {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Write initiation delay.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Read initiation delay.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Sequential stream scheme.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Maximum concat size.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Overlapping request scheme.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Sequential stream diff maximum.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Scheduling timeout scheme.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Timeout time\/weight.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Timeout scheduling.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Scheduling priority scheme.*/)
        {
	$line =~ s/:/ =/;
	print("   $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Priority scheduling.*/)
        {
	$line =~ s/:/ =/;
	print("   $line\n  }, # end of Scheduler\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Use queueing in subsystem.*/)
        {
	$line =~ s/:/ =/;
	print("  $line\n} # end of DRV$c spec\n\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
       
       

}




##
## Buses
##

@buses = ();
$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of buses: ([0-9]+)/) 
{
    $numbuses = $1;
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$specno = 0;
for($c = 0; $c < $numbuses; $c++) {
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Bus Spec \#[0-9]+/)
    {
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /\# buses with Spec: ([0-9]+)/)
    {
	$buswithspec = $1;
	if($1 > 1) {
	    $maxdrv = $c + $1 - 1;
	    $instances{"bus$c .. bus$maxdrv"} = "BUS$specno";
	}
	else {
	    $instances{"bus$c"} = "BUS$specno";
	}

	for($d = 0; $d < $1; $d++) {
	    $num = $c + $d;
	    push(@buses, "bus$num");
	}

    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    print("disksim_bus BUS$specno {\n");

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Bus Type.*/)
        {
	$line =~ s/Bus Type:/type =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Arbitration type:*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Arbitration time:.*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Read block transfer time.*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Write block transfer time:.*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Print stats.*/)
        {
	print("  $line\n} # end of BUS$specno spec\n\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }



    if($buswithspec > 1) {
	$c += ($buswithspec - 1);
    }

    $specno++;
}


##
## Controllers
##

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of controllers: ([0-9]+)/) 
{
    $numctlrs = $1;
}
else {
    print STDERR "*** error on line $linenum: $line\n";
    exit(1);
}

# build an array mapping numbers to names 
@controllers = ();
$specno = 0;
for($c = 0; $c < $numctlrs; $c++) {

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Controller Spec \#[0-9]+/)
    {
    }
    else {
	print STDERR "*** error on line $linenum: $line\n";
	exit(1);
    }

    print("disksim_ctlr CTLR$specno {\n");

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /\# controllers with Spec: ([0-9]+)/)
    {
	if($1 > 1) {
	    $ctlrswithspec = $1;
	    $maxdrv = $c + $1 - 1;

	    $instances{"ctlr$c .. ctlr$maxdrv"} = "CTLR$specno";
	}
	else {
	    $instances{"ctlr$c"} = "CTLR$c";
	}

	for($d = 0; $d < $1; $d++) {
	    $num = $c + $d;
	    push(@controllers, "CTLR$num");
	}

    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Controller Type:*/)
        {
	$line =~ s/Controller Type:/type =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Scale for delays:*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Bulk sector transfer time:*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Maximum queue length:*/)
        {
	$line =~ s/:/ =/;
	print("  $line,\n");
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Print stats.*/)
        {
	print "  $line";
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }
    
    # deal with the ioqueue
    for $d (0 .. 12) {
	# for prev. param
	if($d) { print ",\n"; }

	$line = shift(@lines); $linenum = shift(@linenums);
	if(!$d && !($line =~ /Scheduling policy.*/)) { 
	    unshift(@lines, $line); 
	    unshift(@linenums, $linenum);
	    last;
	}
	elsif(!$d) {
	    print ",\n";
	    print "  Scheduler = disksim_ioqueue {\n";
	}
	$line =~ s/:/ =/;
	print("    $line");
	if($d == 12) { print "\n    # end of ioqueue spec\n  }"; }
    }

    # deal with the cache
    $params = 1;
    for ($d = 0; $d <= $params; $d++) {
	if($d > 1) { print ",\n"; }

	$line = shift(@lines); $linenum = shift(@linenums);
	if(!$d && !($line =~ /Cache type: (.*)/)) { 
	    unshift(@lines, $line); 
	    unshift(@linenums, $linenum);
	    last;
	}
	elsif(!$d) {
	    if($1 == 2) {
		$cachetype = "disksim_cachedev"; 
		$params = 8;
	    } else { $cachetype = "disksim_cachemem"; $params = 18; }
	    print ",\n  Cache = $cachetype { \n";
	}
	else {
	    $line =~ s/:/ =/;
	    # cachemem
	    $line =~ s/Cache prefetch type \(read\)/Read prefetch type/;
	    $line =~ s/Cache prefetch type \(write\)/Write prefetch type/;
	    $line =~ s/Cache scatter\/gather max/Max gather/;
	    $line =~ s/Cache segment count \(SLRU\)/# SLRU segments/;
	    $line =~ s/Cache blocks per bit/Bit granularity/;
	    $line =~ s/Cache lock granularity/Lock granularity/;

	    # cachedev
	    if($line =~ /Cache device number = (.*)/) {
		$line = "Cache device = $devtypes[$1]$1"; 
	    }
	    elsif($line =~ /Cache data for device = (.*)/) {
		$line = "Cached device = $devtypes[$1]$1";
	    }
	    $line =~ s/\(.*\)//;

	    if(!($line =~ /Cache size/) &&
	       !($line =~ /Cache device/)) {
		# yuck! there has to be an easier way to do this!
		if($line =~ /(Cache )([^\s])(.*)=(.*)/) {
		    $letter = $2;
		    $letter =~ tr/a-z/A-Z/;
		    $line = $letter . $3 . " = " . $4;
		}
	    }
	    else {
		$line =~ s/\(.*\)//;
	    }

	    print("    $line");
	    if($d == $params) { 
		print "\n    # end of $cachetype spec\n  }"; 
		$line = shift(@lines); $linenum = shift(@linenums);
		if($line =~ "Max per-disk pending.*") {
		    $line =~ s/:/ =/;
		    print(",\n  $line\n");
		}
	    }
	}
    }



    print "} # end of CTLR$specno spec\n\n";

    if($ctlrswithspec > 1) {
	$c += ($ctlrswithspec - 1);
    }
    $specno++;
}




##
## Devices
##

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of storage devices: ([0-9]+)/) 
{
    $numdevs = $1;
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

@devices = ();
for($c = 0; $c < $numdevs; $c++) {
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Device Spec #.*/)
    {
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /# devices with Spec: ([0-9]+)/)
    {
	$numSDs = $1;
	$devswithspec = $numSDs;


    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Device type for Spec: (.*)/)
    {
	$devtype = $1;
	$devtype =~ s/simpledisk/sd/;
    }
    else {
	print STDERR "*** error on line $linenum: $line.\n";
	exit(1);
    }

       for($d = 0; $d < $devswithspec; $d++) {
	   $num = $c + $d;
	   push(@devices, "$devtype$num");
	   $devtypes[$num] = $devtype;
	   print STDERR "DEV $devtype$num\n";
       }



    if($devtype eq "disk") {

	$line = shift(@lines); $linenum = shift(@linenums);
	if($line =~ /Disk brand name: (.*)/)
	{
	    print("# $1\n");
	    
	    if($devswithspec > 1) {
		$maxdrv = $c + $numSDs - 1;
		$instances{"disk$c .. disk$maxdrv"} = "$1";
	    }
	    else {
		$instances{"disk$c"} = "$1";
	    }
	    
	}
	else {
	    print STDERR "*** error on line $linenum: $line.\n";
	    exit(1);
	}


	$line = shift(@lines); $linenum = shift(@linenums);
	if($line =~ /Disk specification file: ([^\s]+)/)
	{
	    print("source $1\n\n");
	}
	else {
	    print STDERR "*** error on line $linenum: $line.\n";
	    exit(1);
	}

	
    }
    elsif ($devtype eq "mems2") {
	for($d = 0; $d < $devswithspec; $d++) {
	    $num = $c + $d;
	    push(@devices, "mems$num");
	}

	$line = shift(@lines); $linenum = shift(@linenums);	
	if($line =~ /Device type: (.*)/) {
	    $memstype = $1;
	}
	else { 
	    print STDERR "*** error on line $linenum: $line.\n";
	    exit(1);
	}

	if($devswithspec > 1) {
	    $maxdrv = $c + $numSDs - 1;
	    $instances{"mems2$c .. mems2$maxdrv"} = "$memstype";
	}
	else {
	    $instances{"mems2$c"} = "$memstype";
	}


	$line = shift(@lines); $linenum = shift(@linenums);	
	if($line =~ /Device specification file: (.*)/) {
	    print "# source the mems2 dev spec file\n";
	    print "source $1\n\n";
	}
	else {
	    print "mems2 $memstype {\n";
	    for($d = 0; $d < 40; $d++) {
		$line = shift(@lines); $linenum = shift(@linenums);
		$line =~ s/:/ = /;
		print("     $line\n");
	    }
	    print "}  # end of $memstype spec\n\n";
	}
    }
    elsif ($devtype eq "sd") {
	print "simpledisk SD$c {\n";
	for $c (0 .. 7) {
	    if($c) { print ",\n"; }
	    $line = shift(@lines); $linenum = shift(@linenums);
	    $line =~ s/:/ = /;
	    $line =~ s/\(.*\)//;

	    $line =~ s/Number of blocks/Block count/;
	    $line =~ s/Access time (in msecs)/Constant access time/;
	    $line =~ s/Max queue length at simpledisk/Max queue length/;

	    print("   $line");
	}
	# deal with the ioqueue
	for $d (0 .. 12) {
	    # for prev. param
	    if($d) { print ",\n"; }
	    
	    $line = shift(@lines); $linenum = shift(@linenums);
	    if(!$d && !($line =~ /Scheduling policy.*/)) { 
		unshift(@lines, $line); 
		unshift(@linenums, $linenum);
		last;
	    }
	    elsif(!$d) {
		print ",\n";
		print "   Scheduler = ioqueue {\n";
	    }
	    $line =~ s/:/ =/;
	    print("      $line");
	    if($d == 12) { print "\n    # end of ioqueue spec\n  }"; }
	    }
	print "   # end of SD$c spec\n}\n\n";
	
	if($devswithspec > 1) {
	    $maxdrv = $c + $numSDs - 1;
	    $instances{"sd$c .. $sd$maxdrv"} = "SD$c";
	}
	else {
	    $instances{"sd$c"} = "SD$c";
	}
    }
    else {
	print STDERR "*** error: unknown device type $devtype\n";
	exit(1);
    }

    if($devswithspec > 1) { $numdevs -= ($devswithspec + 1); }
}





##
## Component Instantiation
##

print("# component instantiation\n");

print "instantiate [ statfoo ] as Stats\n";
foreach $inst (keys(%instances)) {
    print("instantiate [ $inst ] as $instances{$inst}\n");
    $c++;
}   

print("# end of component instantiation\n\n");

print "$iomaps";



#
# PHYSICAL ORGANIZATION
#
print "# system topology\n";
print "topology disksim_iodriver driver0 [\n";

# read and ignore driver

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /PHYSICAL ORGANIZATION/)
{
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Driver #1/) { }
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /# of connected buses.*/) { }
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Connected buses: ([0-9]+)/)
{
#    print "bus bus$1 [ \n";
    $num = $1 - 1;
    $topbus = "bus$num";
}
else {
    print STDERR "*** error on line $linenum: $line.\n";
    exit(1);
}

#
# TOPO: controllers
#
@ctlrtopo = ();
for($c = 0; $c < $numctlrs; $c++) {
    @children = ();
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Controller #([0-9]+)(-([0-9]+))?/)
       {
#	   print("$1 $2 $3\n");
	   if($2 ne undef) { $busrange = 1; $lastbus = $3; }
	   else { $busrange = 0; $lastbus = $1; }
	   $firstbus = $1;
	   # v2 counts some things from 1 instead of 0
	   $firstbus--; $lastbus--;
	   if($busrange) {
	       $c += ($lastbus - $firstbus);
	   }
       }
       else {
	   print STDERR "foo *** error on line $linenum: $line.\n";
	   exit(1);
       }

       $line = shift(@lines); $linenum = shift(@linenums);
       if($line =~ /# of connected buses: ([0-9]+)/) {
	  $ctlrbuses = $1;
       }
       else {
	 print STDERR "*** error on line $linenum: $line.\n";
	 exit(1);
       }

       for($d = 0; $d < $ctlrbuses; $d++) {
	   $line = shift(@lines); $linenum = shift(@linenums);	  
	   if($line =~ /Connected buses: (.*)/)
	   {
	       $busspec = $1;
	       if($busspec =~ /#([+-][0-9]+)/) {
		  for($e = $firstbus; $e <= $lastbus; $e++) {
		      $targetbus = $e + $1;
		      print STDERR "ctlr$e has bus$targetbus\n";
		      push(@{$ctlrtopo[$e]}, "bus$targetbus");
		  }
		  

	      } 
	       elsif($busspec =~ /([0-9]+)/) {
		   if($busrange) {
		       # error
		       print STDERR "*** error: must give relative bus number for bus range\n";
		       exit(1);
		   }
		   else {
		       $targetbus = $1 - 1;
		       print STDERR "ctlr$c has bus$targetbus\n";
		       push(@{$ctlrtopo[$c]}, "bus$targetbus");
		   }
	       }
	       else {
		   print STDERR "*** error on line $linenum: $line.\n";
		   exit(1);
	       }
	   }
	   else {
	       print STDERR "*** error on line $linenum: $line.\n";
	       exit(1);
	   }
       }

} # done with controllers


#
# TOPO: buses
#
@bustopo = ();
for($c = 0; $c < $numbuses; $c++) {
    # build up a list of children as we go
    @children = ();

    $line = shift(@lines); $linenum = shift(@linenums);
    if($line =~ /Bus #([0-9]+)(-([0-9]+))?/)
       {
#          print("$1 $2 $3\n");
	   if($2 ne undef) { $busrange = 1; $lastbus = $3; }
	   else { $busrange = 0; $lastbus = $1; }
 	   $firstbus = $1;
	   $firstbus--;
	   $lastbus--;
	   if($busrange) { $c += ($lastbus - $firstbus); }

       }
       else {
	   print STDERR "*** error on line $linenum: $line.\n";
	   exit(1);
       }

	$line = shift(@lines); $linenum = shift(@linenums);
	if($line =~ /# of utilized slots: ([0-9]+)/) {
	   $busslots = $1;
        }
	else {
	    print STDERR "*** error on line $linenum: $line.\n";
	    exit(1);
	}

        # loop over slots in bus
	for($d = 0; $d < $busslots; $d++) {
	    $line = shift(@lines); $linenum = shift(@linenums);
#	   print("bus $c slot $d\n");
	    
	    if($line =~ /Slots: (Controllers|Devices) (.*)/)
	    {
		if($1 eq "Controllers") { 
		    $devtype = "ctlr"; 
		}
		else { $devtype = ""; }
		
		$devspec = $2;
		print STDERR "DEVSPEC: $devspec\n";
		if($devspec =~ /^([0-9]+)$/) {
		    if($busrange) {
			print STDERR "*** error: must use #-notation for bus ranges.\n";
			    exit(1);
		    } 
		    
		    $devnum = $1 - 1;

		    if($devtype ne "ctlr") {
			$devices[$devnum] =~ /(mems2|sd|disk)(.*)/;
			$devtype = $1;
			print STDERR "DEVTYPE: $devtype\n";
		    }

		    print STDERR "bus$c has $devtype$devnum\n";
		    
		    push(@{$bustopo[$c]}, "$devtype$devnum");
		    
		}
		elsif($devspec =~ /([0-9]+) ?- ?([0-9]+)/) {
		    if($busrange) {
			print STDERR "*** error: must use #-notation for bus ranges.\n";
 		        exit(1);
		    }
		    
		    $first = $1 - 1;
		    $last = $2 - 1;
		    if($devtype ne "ctlr") {
			$devices[$first] =~ /(mems2|sd|disk)(.*)/;
			$devtype = $1;
		    }
		    for($e = $first; $e <= $last; $e++) {

			push(@{$bustopo[$c]}, "$devtype$e");
			print STDERR "bus$c has $devtype$e\n";
		    }

		 
		    $d += ($last - $first);
		}
		elsif($devspec =~ /#([+-][0-9]+)/) {
		      for($bus = $firstbus; $bus <= $lastbus; $bus++) {
			  $devnum = $1 + $bus;
			  if($devtype ne "ctlr") {
			      $devices[$devnum] =~ /(mems2|sd|disk)(.*)/;
			      $devtype = $1;
			  }
			  push(@{$bustopo[$bus]}, "$devtype$devnum");


			  print STDERR "bus$bus has $devtype$devnum\n";

		      }
		}
		else {
		    print STDERR "*** error on line $linenum: $line.\n";
		    exit(1);
		}
	    }
	    elsif($line =~ /Bus #/) {
		  unshift(@lines, $line);
		  unshift(@linenums, $linenum);
		  next;
	    }
	    else {
		print STDERR "*** error on line $linenum: $line.\n";
		exit(1);
	    }
	}

}


outputBusTopo(\@bustopo, \@ctlrtopo, $topbus, [$topbus], 5);


print "\n     # end of system topology\n]\n\n";
  

#
# SYNCHRONIZATION 
#
$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /SYNCHRONIZATION/) {

} else { error("bad syncset spec"); }

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /Number of synchronized sets: ([0-9]+)/) {
  $numsyncsets = $1;
} else { error("bad syncset spec"); }
if(! $numsyncsets) {
   print("# no syncsets\n\n");
}
else {
    for $c (0 .. ($numsyncsets - 1)) {
	$line = shift(@lines); $linenum = shift(@linenums);
	if($line =~ /Number of devices.*:\s*([0-9]+)/) {
	    # this parameter seems to serve no purpose ...
	    $line = shift(@lines); $linenum = shift(@linenums);
	    if($line =~ /Synchronized devices: ([0-9]+) ?- ?([0-9]+)/) {
		$d1 = $1;
		$d2 = $2;
		$d1--; $d2--;
		$devices[$d1] =~ /(mems2|sd|disk)(.*)/;
		$devtype = $1;
		print("syncset sync$c { devices = [ $devtype$d1 .. $devtype$d2 ] }\n");
	    } 
	    else { error("bad syncset spec #$c"); }
	    }
    }
}


#
# LOGICAL ORGANIZATION
#

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /LOGICAL ORGANIZATION/) {} 
else { error("bad logorg spec: $line"); }

$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /# of system-level organizations:\s*([0-9]+)/) {
   $numlogorgs = $1;	
} else { error("bad logorg spec: $line"); }

for $c (0 .. ($numlogorgs - 1)) {
   print("disksim_logorg org$c {\n");
   $line = shift(@lines); $linenum = shift(@linenums);
   if($line =~ /Organization #[0-9]+:\s*(.*)/) {
      @flags = split(/\s+/, $1);
      $numflags = @flags;
      if($numflags != 4) { error("need 4 logorg flags: \"$flags[0]\""); }
      print("   Addressing mode = $flags[0],\n");
      print("   Distribution scheme = $flags[1],\n");
      print("   Redundancy scheme = $flags[2],\n");
#      print("  Addressing mode = $flags[3],\n");
   }

   for $d (0 .. 13) {
      $line = shift(@lines); $linenum = shift(@linenums);
      if($line =~ /Devices:\s*(.*)/) {
         if($1 =~ /([0-9]+)\s*-([0-9]+)\s*/) {
	     $d1 = $1; $d2 = $2;
	     $d1--; $d2--;
	    if($d1 != $d2) { print("   devices = [ $devtypes[$d1]$d1 .. $devtypes[$d2]$d2 ]"); }
	    else { print "   devices = [ $devtypes[$d1]$d1 ]"; }
         } else { error("need a range of devices in logorg spec, not $line"); }
      }
      elsif($line =~ /Number of devices.*/) { }
      elsif($line =~ /High-level device number.*/) { }
      else {
         print ",";
         $line =~ s/:/ = /;
         $line =~ s/\(.*\)//;
	 print("\n   $line");
      }
   }
   print("\n} # end of logorg org$c spec\n\n");
}

# eat ctlr level spec number (unused)
shift(@lines);

#
# Process-Flow Input Parameters
# -----------------------------
#

$line = shift(@lines); $linenum = shift(@linenums);	
if($line =~ /Process-Flow.*/) { } else { error("bad procflow spec"); }
$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /-+/) { } else { error("bad procflow spec"); }





#
# PRINTED PROCESS-FLOW STATISTICS
#

# move this to stats block

# eat these 5 lines -- we already looked at them
for $c (1..5) { shift(@lines); }

#
# GENERAL PROCESS-FLOW PARAMETERS
#
$line = shift(@lines); $linenum = shift(@linenums);	
if($line =~ /GENERAL PROCESS-FLOW PARAMETERS/) 
{
} else { error("bad procflow params"); }
print "disksim_pf Proc {\n";
for $c (1..2) {
  $line = shift(@lines); $linenum = shift(@linenums);
  $line =~ s/:/ = /;
  print("   $line");
  if($c == 1) { print ","; }
  print("\n");
}
print("} # end of process flow spec\n\n");

#
# SYNTHETIC I/O TRACE PARAMETERS
#

print("synthio Synthio {\n");
$line = shift(@lines); $linenum = shift(@linenums);
if($line =~ /SYNTHETIC I\/O TRACE PARAMETERS/) {

} else { error("bad synthio spec"); }

for $c (1 .. 6) {
  $line = shift(@lines); $linenum = shift(@linenums);
  if($line =~ /Number of generators:\s*([0-9]+)/) {
    $numgens = $1;
  }
  else {
    $line =~ s/:/ = /;
    $line =~ s/\(.*\)//;
    print("   $line,\n");
  }
}
print("   Generators = [\n");
for $c (0 .. ($numgens - 1)) {

  $line = shift(@lines); $linenum = shift(@linenums);
  if($line =~ /Generator description.*/) {
  } 
  elsif($line eq undef) { last; }
  else { error("bad generator description line $linenum: $line"); }

  print("     disksim_synthgen { # generator $c \n");

  $line = shift(@lines); $linenum = shift(@linenums);
  if($line =~ /Generators with description:\s*([0-9]+)/) {
     $descgens = $1;
     $numgens -= $descgens; 
     $numgens++;
  }  else { error("bad generator spec line $linenum: $line"); }

  for $d (1 .. 9) {
     $line = shift(@lines); $linenum = shift(@linenums);
     if($line =~ /Number of storage devices:\s*([0-9]+)/) {
        $numdevs = $1;
     }
     elsif($line =~ /First storage device:?\s*([0-9]+)/) {
        $firstdev = $1;
        if($numdevs > 1) {
           $lastdev = $firstdev + $numdevs - 1;
           print("       devices = [ $devtypes[$firstdev]$firstdev .. $devtypes[$lastdev]$lastdev ],\n");
        }
        else {
           print("       devices = [ $devtypes[$firstdev]$firstdev ], \n");
        }
     }
     else {     
        $line =~ s/\(.*\)//;
        $line =~ s/:/ = /;
        print("       $line,\n");
     }
   }

   # distribution parameters
   for $d (1 .. 6) {
      $_ = shift(@lines); $linenum = shift(@linenums);
      s/\(.*\)//; 
#      $foo = $_;
      print("       $_ = [ ");

      $_ = shift(@lines); $linenum = shift(@linenums);
      s/Type of distribution:\s*//;
      if(/normal/) {
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_ ";
      }
      elsif(/exponential/) {
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_ ";
      } 
      elsif(/poisson/) {
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_ ";

      }
      elsif(/twovalue/) {
         $_ = shift(@lines); $linenum = shift(@linenums);
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_ ";

      }
      elsif(/uniform/) {
         $_ = shift(@lines); $linenum = shift(@linenums);
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_, ";
         $_ = shift(@lines); $linenum = shift(@linenums);
	 s/.*:\s*//;
         print "$_ ";
      }
      else { error("no such distribution $_"); }




      print " ]"; if($d != 6) { print(","); } print "\n";
      
   }

  
  
  print("     }");
  if($c != ($numgens - 1)) { print(","); }
  print(" # end of generator $c \n");
}

print("   ] # end of generator list \n");
print("} # end of synthetic workload spec\n");


print("\n\n");










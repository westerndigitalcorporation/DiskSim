#!/usr/bin/perl

# DIXtrac - Automated Disk Extraction Tool
# Authors: Jiri Schindler, John Bucy, Greg Ganger
#
# Copyright (c) of Carnegie Mellon University, 1999-2005.
#
# Permission to reproduce, use, and prepare derivative works of
# this software for internal use is granted provided the copyright
# and "No Warranty" statements are included with all reproductions
# and derivative works. This software may also be redistributed
# without charge provided that the copyright and "No Warranty"
# statements are included in all redistributions.
#
# NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
# CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
# EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
# TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
# OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
# MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT
# TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.

# $DISKDBMS="/home/diskdbms";

%vendors = ( "IBM", "IBM",
	     "QUANTUM", "Quantum",
             "SEAGATE", "Seagate",
	     "MAXTOR", "Maxtor");

# new_disk.pl <sg device> <cname> [dir]

if ($#ARGV < 1) {

    # extract the program/script name
    $0 =~ /([^\/]+$)/;
    my $pname = $1;
    print STDERR <<EOF;
Usage: $pname <sg device> <cname> [<dir>]
   <sg device>    the device of the disk (e.g., /dev/sg12)
   <cname>        canonical disk name; stem for file names (e.g., Cheetah15k4)
   <dir>          working directory where the extraction should be run
EOF
;
	    exit(-1);
}

$DEV = $ARGV[0];
$cname = $ARGV[1];  # canonical name -- cheetah15k4, etc.
$dir = $ARGV[2];

open(STAT, "./dx_stat -d $DEV|");

while(<STAT>) {
    @fields = split(/\s+:\s+/);
    $stats{$fields[0]} = $fields[1];
    $stats{$fields[0]} =~ s/^\s+|\s+$//g; # eat leading/trailing whitespace
    
    # XXX ok for all params?
    $stats{$fields[0]} =~ s/\s+/_/g; # replace spaces with underscores
}

close(STAT);

if($dir eq undef) {
    $DDIR = join("/", ($DISKDBMS, $vendors{$stats{"VENDOR"}}, $stats{"PRODUCT"}, $stats{"SERIALNUM"}));
    
    # for this product
    $PDIR = join("/", ($DISKDBMS, $vendors{$stats{"VENDOR"}}, $stats{"PRODUCT"}));

    if( -e "$DDIR") {
	print STDERR "*** error: $DDIR exists\n";
	exit(1);
    }
}
else {
    $DDIR = $dir;
    mkdir($DDIR) || die("Died at mkdir $DDIR");
}


# Setup the product directory with a symlink from the canonical name
# if we have one.
#if(! -e "$PDIR") {
#    `mkdir -p $PDIR`;
#    chdir("$PDIR") || die;
#    chdir("..");
#    if($cnames{$stats{"PRODUCT"}} ne undef) {
#	symlink($stats{"PRODUCT"}, $cnames{$stats{"PRODUCT"}}) || die;
#    }
#}


$dv = join("/", $DDIR, ".diskdbms.vars");

open(VARS, ">$dv") || die;
$vend = $stats{"VENDOR"};
$prod = $stats{"PRODUCT"};
$ser = $stats{"SERIALNUM"};

print VARS "VENDOR=$vend\n";
print VARS "PRODUCT=$prod\n";
print VARS "SERIALNUM=$ser\n";
print VARS "NAME=$cname\n";
print VARS "DEV=$DEV\n";
close(VARS);

# copy in template stuff

@templates = 
    ( "Makefile",
      "statdefs" );

	     
foreach $i (@templates) {
    `cp templates/$i $DDIR`
}

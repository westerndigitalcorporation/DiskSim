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


# run convert_diskspecs.pl and then convert_diskspecs over a disksim v2
# diskspec and yield a v3 diskspec and a model

# usage ./convert.pl <v2 diskspec> <v3 diskspec> <v3 model> <disk name>
if($#ARGV < 3) {
	print "usage ./convert.pl <v2 diskspec> <v3 diskspec> <v3 model> <disk name>\n";
	exit(1);
}

$convert1 = 'disksim_v2_to_libparam.pl';
$convert2 = 'libparam_to_v3';

$origspec = $ARGV[0];
$newspec = $ARGV[1];
$tmpspec = $newspec . ".tmp";
$model = $ARGV[2];
$name = $ARGV[3];

system("$convert1 $name < $origspec > $tmpspec");
system("$convert2 $tmpspec $name $newspec $model"); 
unlink("$tmpspec");

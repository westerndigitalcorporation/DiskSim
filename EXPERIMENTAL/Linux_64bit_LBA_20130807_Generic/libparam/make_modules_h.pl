#!/usr/bin/perl

# libparam (version 1.0)
# Authors: John Bucy, Greg Ganger
# Contributors: John Griffin, Jiri Schindler, Steve Schlosser
#
# Copyright (c) of Carnegie Mellon University, 2001-2008.
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



$package = $ARGV[0];
$PACKAGE = $package; $PACKAGE =~ tr/a-z/A-Z/;
shift(@ARGV);

print("\n#ifndef _" . "$PACKAGE" . "_MODULES_H\n");
print("#define _" . "$PACKAGE" . "_MODULES_H   \n\n\n");

while(<>) {
    tr/[\t ]+/ /;
    if(/MODULE (.*)/) {
	$_ = $1;
	s/-/_/g;
	push(@enums, join("_", ($package, "MOD", $_)));
	$_ = join("_", ($package, $_));
	push(@modnames,$_);

	$_ = $ARGV;
	s/\.modspec/_param.h/;
	$headerfile = join("_", ($package, $_));
	print "#include \"$headerfile\"\n";
    }
}
print "\n\n";

# module descr array
$c = 0;
print "static struct lp_mod *" . "$package" . "_mods[] = {\n";
foreach (@modnames) {
    $modstructname = join("_", ($_, "mod"));
    print " &$modstructname ";
    if($c != $#modnames) { print ","; }
    print "\n";
    $c++;
}
print "};\n\n";


$c = 0;
print "typedef enum {\n";
foreach (@enums) {
    tr/a-z -/A-Z__/;
    print "  $_";
    if($c != $#modnames) { print ","; }
    print "\n";
    $c++;
}
$modt = join("_", ($package, "mod_t"));
print "} $modt;\n\n";


$c--;
$maxstr = join("_", ($PACKAGE, "MAX_MODULE"));
print "#define $maxstr $c\n";

print("#endif // _" . "$PACKAGE" . "_MODULES_H\n");

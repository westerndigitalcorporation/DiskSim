#!/usr/bin/perl

# convert a v2/raw layout diskspec into a v3 diskspec and g2 model.

# usage: convert_raw.pl <v2 diskspec> <raw layout file> <name> <v3
# diskspec> <model>

if($#ARGV < 4) {
    print STDERR "usage: convert_diskspec_g2.pl <v2 diskspec> <raw layout file> <name> <v3 diskspec> <model>\n";
    exit(1);
}

($origspec, $rawfile, $name, $newspec, $model) = @ARGV;

# use convert.pl to convert origspec into "origmodel" and diskspec
# -- done with diskspec

$origmodel = $model . ".orig";
system("convert_diskspec_g1.pl $origspec $newspec $origmodel $name");
if($? >> 8) { die "*** convert.pl"; }

# compile layout.mappings into layout.comp and
# use driver_g2 to generate layout.model from raw layout file
system("compile $rawfile layout.comp");
if($? >> 8) { die "*** compile"; }

system("driver_g2 --nomech layout.comp $name layout.model");
if($? >> 8) { die "*** driver_g2"; }

# merge origmodel mech and layout.model layout -> base.model
system("merge_blocks", "$origmodel", "Layout Model", "layout.model",
       "Layout Model", "base.model");
if($? >> 8) { die "*** merge_blocks"; }

# grep skews out of "origmodel", use g2_skews to merge with base.model
# -> final model
system("grok_skews.pl $origmodel skews.out");
if($? >> 8) { die "*** grok_skews.pl"; }

system("g2_skews skews.out base.model $model");
if($? >> 8) { die "*** g2_skews"; }



unlink("skews.out layout.model base.model $origmodel layout.comp");

#!/usr/bin/perl

# read skews out of a model, generate "skews.out" for use by skew
# merging tool



($model, $out) = @ARGV;
$i = 0;
open(MODEL, "$model") || die("couldn't open $model");
open(OUT, ">$out") || die("couldn't open $out");
while(<MODEL>) {
    chomp();
    if(/\s*Skew for cylinder switch = ([0-9]+\.[0-9]+)/) {
	$cylskew = $1;
	$i++;
	print OUT "Zone #$i\n Track Skew: $trackskew\n Cylinder Skew: $cylskew\n";
    }
    elsif(/\s*Skew for track switch = ([0-9]+\.[0-9]+)/) {
	$trackskew = $1;

    }
    

}
close(OUT);
close(MODEL);

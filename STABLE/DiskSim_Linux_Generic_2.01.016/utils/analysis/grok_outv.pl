#!/usr/bin/perl

# extract the various distributions out of a parv file

$tag = $ARGV[0];
$found = 0;
while(<STDIN>) {
    if(/$tag/) {
	$found = 1;
    }
    elsif($found) {
	if(! /^[0-9\. \s]+$/) { 
	    exit(0); 
	}

	print "$_";
    }
}

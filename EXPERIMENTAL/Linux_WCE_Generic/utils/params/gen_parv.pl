#!/usr/bin/perl
# Generate a parv file from a template and some parameters

# @DISKSPEC_FILE@ -- name of the diskspec file to source
# @DISK_NAME@     -- name of the disk to instantiate from the spec
# @DISK_LBNS@     -- number of lbns on the disk (stripe unit foo)

if($#ARGV < 2) {
  print STDERR ("*** usage: gen_parv.pl <diskspec file> <disk name> <disk lbns>\n");
  exit(1);
}

%subs = ( "\@DISKSPEC_FILE\@", "$ARGV[0]",
	  "\@DISK_NAME\@", "$ARGV[1]",
	  "\@DISK_LBNS\@", "$ARGV[2]" );

$diskspec = $ARGV[0];
open(DISKSPEC, "$diskspec") || die "couldn't open diskspec $diskspec";
while(<DISKSPEC>) {
    chomp();
    if(/.*source.*model/) {
	s/.*source//;
	s/,//;
	s/^ *//;
	s/ *$//;
	$model = $_;
    }
    elsif(/disksim_disk/) {
	s/.*disksim_disk *//;
	s/^ *//;
	s/ *\{ *$//;
	$diskname = $_;
    }
}
close(DISKSPEC);
# print STDERR "*** $model $diskname\n";

open(MODEL, "$model") || die;
while(<MODEL>) {
    chomp();
    if(/Block count/) {
	s/.*Block count.*=//;
	s/,//;
	s/^ *//;
	s/ *$//;
	$lbns = $_;
    }
}
close(MODEL);
# print STDERR "*** $lbns\n";

$subs{"\@DISK_LBNS\@"} = $lbns;
$subs{"\@DISK_NAME\@"} = $diskname;



while(<STDIN>) {
    foreach $k (keys(%subs)) {
	s/$k/$subs{$k}/;
    }

    print "$_";
}

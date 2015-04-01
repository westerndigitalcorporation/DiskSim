#!/usr/bin/perl

# produce per-request prediction errors between a disksim "detailed
# execution trace" and a "validation" trace.

# This will currently only handle one outstanding request.

$reqfile = $ARGV[0];
$tracefile = $ARGV[1];

open(REQ, "$reqfile") || die;
open(TRACE, "$tracefile") || die;

$count = 0;
@buckets = ();
while(1) {
    do {
	$rl = <REQ> || last;
    } while(! ($rl =~ /Request issue/));

    @f = split(/\s+/, $rl);
    $issue = $f[9];

    do {
	$rl = <REQ> || last;
    } while(!($rl =~ /Request completion/));

    @f = split(/\s+/, $rl);
    $completion = $f[3];

    $srvtime = $completion - $issue;
#    print "srvtime $completion - $issue = $srvtime\n";

    $tl = <TRACE> || last;
    
    @tf = split(/\s+/, $tl);
    
    $diff = ($srvtime) - ($tf[4] / 1000.0);

    $idx = int(($diff * 10)) + 1000;
    print "$diff\n";

    $buckets[int(($diff * 10)) + 1000]++;
    $count++;
}


for($i = 0; $i < 2000; $i++) {

    $v = ($i / 10.0) - 100.0;
    $ct = $buckets[$i];
    if($ct eq undef) { $ct = 0.0; }
    $ct /= $count;
    print "$v $ct\n";
}

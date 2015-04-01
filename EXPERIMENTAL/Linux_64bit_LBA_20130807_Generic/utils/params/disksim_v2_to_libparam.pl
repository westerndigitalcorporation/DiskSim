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


# This does a trivial syntactic translation of a disksim v2 diskspec
# into libparam format.  It requires additional processing to make a
# valid disksim v3 spec and diskmodel v1 model from the result.

# input a diskspecs file on STDIN, new one will be output on STDOUT

sub error {
    print STDERR "*** error: $_[0]\n";
    exit(1);
}

$name = $ARGV[0];

# clean out empty lines, etc.
$linenum = 1;
# was the last param supressed -- if so, don't print an extra ,
$eaten = 0;
while (<STDIN>) {
    $line = $_;
    if(($line =~ /^\s*$/) || ($line eq "") || ($line eq undef)) { }
    else {
        # mash whitespace together
	$line =~ s/\s+/ /g;
	# eat trailing whitespace
	$line =~ s/\s*$//;
	$line =~ s/\- default value\!\!\!/# dixtrac default value/;

	push(@lines, $line);
	push(@linenums, $linenum);
#	print("$line\n");
	if($line =~ /.*mems.*/) {
           $devtype = "mems2";
        }
	elsif($line =~ /Head switch.*/) {
	   $devtype = "disk"
        }

    }
    $linenum++;
}

if($devtype eq "") { $devtype = "simpledisk"; }	

# outer loop over devices
while(1) {

# rewrite header, add type
    $line = shift(@lines); $linenum = shift(@linenums);
    if($line eq undef) {
      print STDERR "* All done\n";
      exit(0);
    }   
    if($line =~ /Disk brand name:\s*([^\s]+)/) {
	if($name ne "") {
	    $devname = $name;
	}
	else { 
	    $devname = $1;
	}
	print("$devtype $devname { \n");
    }
    elsif($line =~ /Device type name: (.*)/) {
	$devname = $1;
	print("$devtype $devname { \n");
    }
    else {
	error("on line $linenum: $line");
    }
    
    $lineno = 0;
    # inner loop over device params
    while(1) {
	if($lineno && ! $eaten) { print ",\n" }
	$eaten = 0;

	$line = shift(@lines); $linenum = shift(@linenums);
	if($line eq undef) { last; }
	if($line =~ /HPL seek equation values:\s*(.*)/)
	{
	    if($seektype == -3.0 || $seektype == -4.0) {
		@fields = split(/\s+/, $1);
		if($#fields < 5) {
		    error("need 6 args for HPL seek equation");
		}
		
		for $d (0..5) {
		    
		    if($fields[$d] =~ /^-?[0-9]+$/) {
			$fields[$d] = "$fields[$d].0";
		    }
		}
		
		
		if($fields[6] ne undef) { 
		    print "   #" ;
			for $d (6 .. $#fields) { print "$fields[d]"; }
		    print "\n";
		}
		print "   HPL seek equation values =\n      [ ";
		foreach $c (0..4) {
		    print "$fields[$c], ";
		}
		print "$fields[5] ]";
	    }
	    else {
		$eaten = 1;
	    }
	}
	elsif($line =~ /First 10 seek times:\s*(.*)/) {
	    if($seektype == -4.0) {
		@fields = split(/\s+/, $1);
		if($#fields != 9) {
		    error("need 10 args for First 10 seek times");
		}
		
		foreach $field (@fields) {
		    if(!($field =~ /-?[0-9]+\.[0-9]+(e[+-][0-9]+)?/)) {
			error("bad First 10 seek times arg: $field");
		    }
		}
		print "   First ten seek times =\n      [ ";
		foreach $c (0..8) {
		    print "$fields[$c], ";
		}
		print "$fields[9] ]";
	    }
	    else {
		$eaten = 1;
	    }
	}

        # a few other special cases
        elsif($line =~ /Blocks per disk.*/) {
           $line =~ s/Blocks per disk:/Block count = /;
	   print("   $line");
        }
	elsif($line =~ /Access time \(in msecs\):.*/) {
	   $line =~ s/Access time (in msecs):\s*//;
	   $_ = $line;
	   print("   Access time type = ");
	   if(/-1.0/) {
	      print("averageRotation");	      
	   }
	   elsif(/-2.0/) {
	      print("trackSwitchPlusRotation");	      
	   }
	   else {
	      print("constant,\n");
	      print("   Constant access time = $_");
	   }
	 }

	elsif($line =~ /Seek time \(in msecs\):\s*(.*)/) {
	   $_ = $1;
	   $seektype = $1;
	   print("   Seek type = ");
	   if(/-1.0/) {
	      print("linear");	      
	   }
	   elsif(/-2.0/) {
	      print("curve");	      
	   }
	   elsif(/-3.0/) {
	      print("hpl");	      
	   }
	   elsif(/-4.0/) {
	      print("hplplus10");	      
	   }
	   elsif(/-5.0/) {
	      print("extracted");	      
	   }
	   else {
	      print("constant\n,");
	      print("   Constant seek time = $_");
	   }
	 }
	elsif($line =~ /Average seek time:\s*(.*)/) {
	   $_ = $1;
	   if(/-?[0-9]+\.[0-9]+(e[+-][0-9]+)?/) {
	       print "   Average seek time = $_";
	   }
	   else { 
	       /([^\s]+)/;
	       print("   Full seek curve = $1");
	   }
        }
	elsif($line =~ /Scheduling policy:\s*([^\s]+)/) {
	   print("   Scheduler = ioqueue {\n   Scheduling policy = $1");
	}
	elsif($line =~ /Priority scheduling:\s*([^\s]+)/) {
	   print("   Priority scheduling = $1\n   # end of Scheduler\n   }");
	}
	elsif($line =~ /Number of bands:\s*([0-9]+)/) {

	    $numbands = $1;
	    print("   Zones = [\n");
	    for($c = 0; $c < $numbands; $c++) {
		$done = 0;
		$bandline = 0;
		$line = shift(@lines); $linenum = shift(@linenums);
		if(!($line =~ /Band #.*/)) {
		     error("bad band spec: $line");
		}
	        if($c) { print ",\n"; }
		print("   zone { # band $c \n");
		while(! $done) {
		  if($bandline) { print ",\n"; }
		  $line = shift(@lines); $linenum = shift(@linenums);
		  if($line =~ /Number of slips:\s*([0-9]+)/) {
		      $numslips = $1;
		      print("      slips =\n         [ ");
		      for($d = 0; $d < $numslips; $d++) {
			  $line = shift(@lines); $linenum = shift(@linenums);
			  if($line =~ /Slip:\s*([0-9]+)/) {
			      if($d) {
				  print ", ";
				 if(!($d % 6)) { print ("\n           "); }
			      }
			      print("$1");
			      
			  }
			  else { error("bad slip spec"); }
		      }
		      print("   ]");
		  }
		  elsif($line =~ /Number of defects:\s*([0-9]+)/) {
		      $numdefects = $1;
		      print("      defects = \n         [ ");
		      for($d = 0; $d < $numdefects; $d++) {
			  if($d) { print ", " };
			  $line = shift(@lines); $linenum = shift(@linenums);
			  
			  if($line =~ /Defect:\s*([0-9]+)\s+([0-9]+)/) {
			      print("$1, $2");
			  }
			  else { error("bad defect spec: $line"); }
			  
		      }
		      print("   ]\n");
		      $done = 1;  # done with this band
		      
		  }
		  else {
#		   $line =~ s/:/ =/;
		      $line =~ /([^:]+): ([^ ]+)(.*)?/;
		      if($3 ne undef) {
			  print "      # $3\n";
		      }
		      print "      $1 = $2";


#		   print "      $line";
		  }
		  
		  $bandline++;
	      } # band param loop
	      print("   }");	

	    } # band loop

	    print("   ] # end of band list\n");
	    last;

	    
	    }

        elsif($line =~ /Max queue length/) {
	    if($line =~ /dixtrac/) {
		$line =~ s/\# dixtrac.*//;
		print "   # dixtrac default value\n";
	    }
	    
	    $line =~ /Max queue length.*:(.*)/;

	    print "   Max queue length = $1";
	}

        elsif($line =~ /Print stats for[^:]*:(.*)/) {
	    print "   Print stats = $1";
	}
	
	# default parameter translation
	else {
#	    $line =~ s/\(.*\)//;
	    $line =~ /([^:]+): ?([^ ]+)(.*)?/;

	    if(($1 ne undef) && ($2 ne undef)) {
		if($3 ne undef) {
		    print "\n   # $3\n";
		    }
		print "   $1 = $2";
	    }
#	    print "\n";
	}

	

    $lineno++;
    } # device param loop

    print("} # end of $devname spec\n\n\n");

} # outer loop

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

$modctr = 0;
@paramlines = ();
$startenum = 0;
$linenum = 0;
$package = $ARGV[0];
$PACKAGE = $package; $PACKAGE =~ tr/a-z/A-Z/;
$filename = $ARGV[1];
open(FILE, "$filename") || die("couldn't open $filename!");




sub mkname {
    $_ = $_[0];
    # upcase
    tr/a-z/A-Z/;
    # replace whitespace and dashes with underscores
    tr/ \t-/_/;
    s/[\/\\\.]//g;
    # eat things in parens
    s/\(.*\)//g;
    # eat trailing whitespace
    s/[ \t]*$//;
    # eat leading whitespace
    s/^[ \t]*//;
    
    return "$_[1]" . "_" . "$_";
}



sub startDoc {
#    print DOC "\\begin{tabular}{|l|l|l|l|}\n";
}

sub endDoc {
##print DOC "}\\\\ \n"; # end the last multicolumn
##print DOC "\\cline{1-4}\n";
    endParamDoc();

#    print DOC "\\end{tabular}\\\\ \n";
}


sub startParamDoc {
    my $tmpmodname = $_[0]; $tmpmodname =~ s/_/\\_/g;
    my $tmpname = $_[1]; $tmpname =~ s/_/\\_/g; 
    my $type = $_[2];
    my $req = $_[3];
    
    @paramDocLines = ();

    $type =~ s/LIST/list/;
    $type =~ s/BLOCK/block/;
    $type =~ s/I/int/;
    $type =~ s/D/float/;
    $type =~ s/S/string/;

    $req =~ s/1/required/;
    $req =~ s/0/optional/;

    print DOC "\\noindent \n";
    print DOC "\\begin{tabular}{|p{\\lpmodwidth}|p{\\lpnamewidth}|p{0.5in}|p{0.5in}|}\n";

    print DOC "\\cline{1-4}\n";
    print DOC "\\texttt{$tmpmodname} & \\texttt{$tmpname} & $type & $req \\\\ \n";
    print DOC "\\cline{1-4}\n";
}

sub fillParamDoc {
    $tmp = $_[0];
    $tmp =~ s/_/\_/g;
    if(!($tmp =~ /^[ \t]*$/)) {
	push(@paramDocLines,$tmp);
    }
}

sub endParamDoc {
    if($#paramDocLines >= 0) {
	print DOC "\\multicolumn{4}{|p{6in}|}{\n";
	foreach $l (@paramDocLines) {
	    print DOC "$l\n";
	}
	print DOC "}\\\\ \n\\cline{1-4}\n\\multicolumn{4}{p{5in}}{}\\\\\n";
    }
    print DOC "\\end{tabular}\\\\ \n";
}


sub startDepFn {
    my $name = $_[0] . "_depend";
    push(@depFns, $name);
    print CODE "static int $name(char *bv) {\n";
}

sub fillDepFn {
    my $pname = $_[0];
    print CODE "if(!BIT_TEST(bv, $pname)) { return $pname; }\n";
}

sub closeDepFn {
    print CODE "return -1;\n}\n\n";
}


sub dummyDepFn {
    startDepFn($_[0]);
    closeDepFn();
}

sub startLoaderFn {
    my $name = $_[0] . "_loader";
    push(@loaderFns, $name);
    my $argthing = $_[1];
    my %argnames = ( "D", "double d",
		     "I", "int i",
		     "S", "char *s",
		     "LIST", "struct lp_list *l",
		     "BLOCK", "struct lp_block *blk" );
		     
    my $arg = $argnames{$argthing};
    if($arg eq "" ) {
	print "$argthing\n";
    }
    print CODE "static void $name($restype result, $arg) { \n";
}

sub fillLoaderTest {
    print CODE "if (! ($_[0])) { // foo \n } \n";
}

sub fillLoaderInit {
    foreach $_ (@_) {
	print CODE "$_";
    }
}

sub closeLoaderFn {
    print CODE "\n}\n\n";
}

sub dummyLoaderFn {
    startLoaderFn(@_);
    closeLoaderFn();
}


# running count of deps for this parameter
$paramdeps = 0;
# does the dep function need to be closed?
$dependopen = 0;
# is the loader function open?
$loaderopen = 0;
# did we output a depend fn for this param yet?
$diddepend = 0;

# accumulate loader, depend function names, print arrays when we're done
@loaderFns = ();
@depFns = ();

while(<FILE>) {
    $linenum++;
    chomp();
    # eat comments
    s/[^\\]#.*//;
    s/^#.*//;
    # unescape escaped #s
    s/\\#/#/g;
    # eat trailing whitespace
    s/[ \t]*$//;

    # eat leading whitespace
    s/^[ \t]*//;

    # mash whitespace together
    s/\t+/\t/g;
    s/ +/ /g;

    # eat empty lines
    if(/^[ \t]*$/) { next; }




    if(/MODULE[ \t]+(.*)/) {
	$_ = $1;
        $_ = join("_", ($package, $_));
	tr/-/_/;
	open(HEADER, ">$_"."_param.h");
	open(CODE, ">$_"."_param.c");
        open(DOC, ">$_"."_param.tex");
	$modname = $_;
	tr/a-z/A-Z/;
	s/ \t/_/g;
	$MODNAME = $_;
	
	print HEADER "\n#ifndef _" . $MODNAME ."_PARAM_H\n";
	print HEADER "#define _" .  $MODNAME ."_PARAM_H  \n\n";
	
        print HEADER "#include <libparam/libparam.h>\n";

	print HEADER "#ifdef __cplusplus\nextern\"C\"{\n#endif\n";

	print HEADER "struct dm_disk_if;\n";

	print CODE "#include \"$modname"."_param.h\"\n";
	print CODE "#include <libparam/bitvector.h>\n";
	startDoc();

#        print_head($modname, $MODNAME);
    }
    elsif(/HEADER (.*)/) {
	print CODE "$1\n";
    }
    elsif(/RESTYPE (.*)/) {
	$restype = $1;
    }
    elsif(/PROTO (.*)/) {
	print HEADER "\n/* prototype for $modname param loader function */\n";
	print HEADER "$1\n\n";
    }
    elsif(/PARAM (.*)/) {
	$paramdeps = 0;

	if($dependopen) { closeDepFn(); }
	elsif($loaderopen) { closeLoaderFn(); }
	elsif(! $loaderopen && $modctr) { dummyDepFn(); dummyLoaderFn($NAME,$type); }

	$diddepend = 0;
	$dependopen = 0;
	$loaderopen = 0;
	
	if(!$startenum) { print HEADER "typedef enum {\n"; $startenum = 1; }
	$closeEnum = 1;

	($name,$type,$req) = split(/\t+/, $1);
	$NAME = mkname($name, $MODNAME);

	print HEADER "   $NAME,\n";


#	print CODE "printf(\"$NAME\\n\");" ;
	$modctr++;
	$closeTest = 0;

        # close off the previous multicolumn if there was one
        if($modctr > 1) { 
	    endParamDoc();
        }

	startParamDoc($modname, $name, $type, $req);
	
	push(@paramlines, "{\"$name\", $type, $req }");
    }
    elsif(/TEST (.*)/) {
	if($dependopen) {
	    closeDepFn();
	    $dependopen = 0;
	}
	elsif(! $diddepend) {
	    dummyDepFn($NAME);
	    $diddepend = 1;
	}
	

	if(! $loaderopen) {
	    startLoaderFn($NAME,$type);
	    $loaderopen  = 1;
	}
	
#	print CODE "#line $linenum \"modules/$filename\"\n";
	fillLoaderTest($1);

	$closeTest = 1;
    }
    elsif(/INIT(.*)/) {
	if($dependopen) {
	    closeDepFn();
	    $dependopen = 0;
	}
	elsif(! $diddepend) {
	    dummyDepFn($NAME);
	    $diddepend = 1;
	}

	if(! $loaderopen) {
	    startLoaderFn($NAME,$type);
	    $loaderopen  = 1;
	}

	$stuff = $1;

#	print CODE "#line $linenum \"modules/$filename\"\n";
	print CODE "$stuff\n";


    }
    elsif(/DEPEND (.*)/) {
	if($paramdeps == 0) {
	    startDepFn($NAME);
	    $dependopen = 1;
	    $diddepend = 1;
	}
	$paramdeps++;
	
	$PARAMNAME = mkname($1, $MODNAME);
	fillDepFn($PARAMNAME);

    }
    else {
	if(!/^[ \t]*$/) {
	    fillParamDoc($_);
	}
    }

}



if($dependopen) { closeDependFn(); }
elsif($loaderopen) { closeLoaderFn(); }


if($modctr) { 
    # nix the trailing comma -- ssh hack!
    seek(HEADER, (tell(HEADER) - 2), SEEK_SET);
}


if($closeEnum) {
    $typename = join("_", ($modname, "param_t"));
    print HEADER "\n} $typename;\n\n";
}

print HEADER "#define $MODNAME"."_MAX_PARAM\t\t$NAME\n";
# print HEADER "#include <libparam.h>\n";
$aname = join("_", ($modname, "params"));

print HEADER "extern void * $MODNAME"."_loaders[];\n";
print HEADER "extern lp_paramdep_t $MODNAME"."_deps[];\n";
print HEADER "\n\nstatic struct lp_varspec $aname [] = {\n";

foreach (@paramlines) {
    print HEADER "   $_,\n";
    $c++;
}
print HEADER "   {0,0,0}\n};\n";


$maxstr = join("_", ($MODNAME, "MAX"));


print HEADER "#define $maxstr $modctr\n";
$structname = join("_", ($modname, "mod"));

$loaderstr = join("_", ($modname, "loadparams"));
print HEADER "static struct lp_mod $structname = { \"$modname\", $aname, $maxstr, (lp_modloader_t)$loaderstr,  0, 0, $MODNAME"."_loaders, $MODNAME"."_deps };\n";

# print HEADER "static struct lp_mod $structname = { 0, 0, 0, 0, 0, 0 };\n";

print HEADER "\n\n#ifdef __cplusplus\n}\n#endif\n";
print HEADER "#endif // _" . $MODNAME . "_PARAM_H\n";

endDoc();

print CODE "void * $MODNAME"."_loaders[] = {\n"; 
$j = 0;
foreach $i (@loaderFns) {
    if($j > 0) {
	print CODE ",\n";
    }
    print CODE "(void *)$i";
    $j++;
}
print CODE "\n};\n\n";

print CODE "lp_paramdep_t $MODNAME"."_deps[] = {\n";
$j = 0;
foreach $i (@depFns) {
    if($j > 0) {
	print CODE ",\n";
    }
    print CODE "$i";
    $j++;
}
print CODE "\n};\n\n";

close(HEADER);
close(CODE);
close(DOC);




use strict;
use Config;
use Cwd;
use Prima::Config;
use Prima::Gencls;
use Prima::Make;


my $PRJ = 'IPA';

sub cmake
{
    my ( $subdir, $cfile) = @_;
    print "Finding dependencies for $cfile...\n";
    my $ofile = $1 . $Prima::Config::Config{objext} if $cfile =~ /^(.*)\.c$/;
    my $rname = $1;
    die "Internal error: illegal c file" unless defined $ofile;
    $cfile = qd( $cfile);
    $ofile = qd( $ofile);
    push @allclean, $ofile;
    push @allobjects, $ofile;
    my @deps = find_cdeps( $cfile);
    return  "$ofile: Makefile $cfile @deps\n\t" .
            cc_command_line( $cfile, $ofile) .
            "\n\n";

}

sub clsmake
{
    my ( $subdir, $clsfile) = @_;
    print "Finding dependencies for $clsfile...";
    my $classname = $1 if $clsfile =~ /(?:^|\\|\/)([^\\\/]*)\.cls$/;
    die "Internal error: illegal cls file"
        unless defined $classname;
    if ( defined $subdir) { $subdir = "/$subdir" } else { $subdir = '' };
    my $mk = qd( "include/generic/$classname.h"
               ) . ": Makefile " .
                   "$clsfile";
    push @allclean, (
                     qd( "include/generic/$classname.h"),
                     qd( "include/generic/$classname.inc"),
                    );
    push @allinstall, qd( "include/generic/$classname.h"), $INSTALL_LIB . qd( "/CORE/generic");
    $classname =~ m/\/*(\w*)$/;
    my $fn = $1;
    $mapheader{"$fn.h"} = "include/generic/$fn.h";
    $mapheader{"$fn.inc"} = "include/generic/$fn.inc";
    my @ancestors = gencls( $clsfile, depend => 1);
    $mk .= qd( "include/generic/$_.h $_.cls ") foreach @ancestors;
    print "\n";
    $mk .= "\n\tgencls " .
        "--inc --h $clsfile include/generic\n\n";
    return $mk;
}

sub dllmake
{
   my $subdir = $_[0];
   my $LIB_EXT = $Prima::Config::Config{libext};
   my $sbd  = defined($subdir) ? "$subdir/$subdir" : $PRJ;
   my $name = qd( "auto/$PRJ/" . (defined($subdir) ? ( "$subdir/".dl_name( $PRJ.'::'.$subdir)) : dl_name($PRJ)));
   push ( @allsubtargets,  $name);
   push ( @alllibtargets,  qd( "auto/$PRJ/$sbd$LIB_EXT"));
   push ( @subtargetsdirs, qd( "auto/$PRJ" . ( defined($subdir) ? "/$subdir" : '')));
   push ( @allclean,      $name);
   local @LIBS = @LIBS;
   local @LIBPATH = @LIBPATH;
   if ( defined ( $subdir) && ( $Win32 || $OS2)) {
      push( @LIBS,    "$PRJ$LIB_EXT");
      push( @LIBPATH, cwd() . qd("/auto/$PRJ"));
   }
   my $r = '';
   my $extradep = '';
   if ( $OS2) {
      my $def = defined($subdir) ? "$subdir/$subdir.def" : "$PRJ.def";
      $extradep = qd("auto/$PRJ/$sbd$LIB_EXT");
      $r = <<LIBOS2;
$extradep:
\temximp -o $extradep $def
LIBOS2
   }
   return $r . "\n$name: @allobjects $extradep\n\t" .
      ld_command_line( $name, qd("$sbd.def"), @allobjects) . "\n\n";
}




exit unless init(@ARGV);

print "Setting up working environment.\n";

push( @INCPATH, 'include', qd('include/generic'));
setvar( 'INSTALL_LIB', $PREFIX . qd( "/$PRJ"));
setvar( 'INSTALL_DL', $PREFIX . qd( "/auto/$PRJ"));
setvar( 'INSTALL_EXAMPLES', $PREFIX . qd( "/$PRJ/examples"));


my $make = <<EOF;
# Makefile for $PRJ project under $Config{archname}
#
# THIS IS GENERATED FILE.
#
# Do not edit -- all changes will be lost.
# Edit Makefile.PL instead.

all: project

EOF

opendir D, '.' or die "Can't open current directory\n";
my @d = readdir D;
closedir D;

{
   local @allobjects = @allobjects;
   my $touch = 0;

   while ( <*.cls>) {
       $make .= clsmake( undef, $_);
       $touch = 1;
   }
   while ( <*.c>) {
       $make .= cmake( undef, $_);
       $touch = 1;
   }
   $make .= dllmake() if $touch;
   generate_def("$PRJ.def", $PRJ, "boot_$PRJ", "create_compatible_image");
}

my @prjdirs;

if (1) {
   for ( @d) {
      next unless -d $_;
      next if m/^\./;
      local @allobjects = @allobjects;
      my $subdir = $_;
      my $touch = 0;
      while ( <$subdir/*.cls>) {
         $make .= clsmake( $subdir, $_);
         $touch = 1;
      }
      while ( <$subdir/*.c>) {
          $make .= cmake( $subdir, $_);
          $touch = 1;
      }
      next unless $touch;
      push( @prjdirs, $subdir);
      $make .= dllmake( $subdir);
      generate_def("$subdir/$subdir.def", $PRJ.'::'.$subdir, "boot_${PRJ}__$subdir");
   }
}

   push @allrealclean, 'Makefile';
   push @alldirs, qd( "include/generic"), qd( "auto/$PRJ");
   push @alldirs, map {qd "auto/$PRJ/$_"} @prjdirs;
   
   my $iterm_dest = qd("$INSTALL_BIN/iterm") . ( $Win32 ? '.bat' : ( $OS2 ? '.cmd' : ''));

   print "Writing Makefile...";
   $make =~ tr[/][]s;
   open MAKE, ">Makefile" or die "Creation of Makefile failed: $!";
   print MAKE $make;

   print MAKE <<EOF;

RM=$^X Makefile.PL --rm

clean:
\t\$(RM) @allclean

realclean: clean
\t\$(RM) @allrealclean

project: dirs @allsubtargets

Makefile: Makefile.PL
\t\@echo Rebuilding Makefile...
\t\@$^X Makefile.PL $ARGV_STR
\t\@$Config{make}
\t\@echo You are safe to ignore the following error...
\t\@false

CP=$^X Makefile.PL --cp
CPBIN=$^X Makefile.PL --cpbin

install: all
\t\@\$(CPBIN) iterm.pl $iterm_dest
\t\@\$(CP) \\
EOF

    $make = "";
    my ( $i, $maxcnt) = ( 1, 10);
    for ( $i = 0; $i < @allsubtargets; $i++) {
       if ( ( $i % $maxcnt) == 5) {
          $make .= "\n\t\@\$(CP)";
       }
       $make .= "\t$allsubtargets[$i] $PREFIX/$subtargetsdirs[$i] \\\n";
       if ( $Win32 || $OS2) { # libs additional to dll
          $make .= "\t$alllibtargets[$i] $PREFIX/$subtargetsdirs[$i] \\\n";
       }
    }
    print MAKE $make;

    ( $i, $maxcnt) = ( 1, 10);
    for ( @prjdirs) {
       my $dir = $_;
       while ( <$dir/*.pm>) {
          print MAKE "\t", qd( "$_ $PREFIX/$PRJ");
          print MAKE "\n\t\@\$(CP)";
       }
    }
    while ( scalar @allinstall) {
        my ( $src, $dst) = ( shift @allinstall, shift @allinstall);
        print MAKE "\t$src $dst";
        if ( ( $i % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CP)";
        }
        $i++;
        print MAKE " \\\n";
    }
    while ( <include/*.h>) {
        my ( $dsth) = $_;
        $dsth =~ s/^include/CORE/;
        print MAKE "\t$_ ", dirname( qd( "$INSTALL_LIB/$dsth"));
        if ( ( $i % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CP)";
        }
        $i++;
        print MAKE " \\\n";
    }
    print MAKE "\t$PRJ.pm $PREFIX\n";

    print MAKE "\t\@\$(CPBIN)";
    $i = 1;
    while ( scalar @allbins) {
        my ( $src, $dst) = ( shift @allbins, shift @allbins);
        if ( ( $i % $maxcnt) == 0) {
            print MAKE "\n\t\@\$(CPBIN)";
        }
        $dst = qd( "$dst/" . basename( $src, '.pl')) . $Prima::Config::Config{scriptext};
        print MAKE " \\\n\t$src $dst"
    }
    print MAKE "\n";

    print MAKE <<EOF;

MD=$^X Makefile.PL --md

dirs: @alldirs

@alldirs:
\t\@\$(MD) @alldirs

EOF

    close MAKE;
    print "\nDone.\n";

1;

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
my $VERSION = join( '.', find_version( "$PRJ.pm"));


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
   push @allman, 'iterm.pl', manname( 'iterm.pl', 1, '');
   push @allman, 'IPA.pm', manname( 'IPA.pm', 3, '');
   
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
EOF

    print MAKE dump_command( sub { "$allsubtargets[$_[0]] $PREFIX/$subtargetsdirs[$_[0]]" }, 
        'CP', 0 .. $#allsubtargets);
    print MAKE dump_command( sub { "$alllibtargets[$_[0]] $PREFIX/$subtargetsdirs[$_[0]]" }, 
        'CP', 0 .. $#allsubtargets) if $Win32 || $OS2;
    print MAKE dump_command( sub { qd "$_[0] $PREFIX/$PRJ" }, 'CP', map { <$_/*.pm> } @prjdirs);
    print MAKE dump_command( sub { "@{$_[0]}" }, 'CP', 
         map {[@allinstall[$_*2,$_*2+1]]} 0..$#allinstall/2);
    print MAKE dump_command( sub { 
            my $dsth = $_[0];
            $dsth =~ s/^include/CORE/;
            return "$_[0] " . dirname ( qd( "$INSTALL_LIB/$dsth"));
         }, 'CP', <include/*.h>);
    print MAKE dump_command( sub { "$PRJ.pm $PREFIX" }, 'CP', 1);

    if ( $install_manuals) {
       my $cmd = "\t\@pod2man --lax --section=\%d \%s " . ( length( $Config{gzip} ) ? 
           "| $Config{gzip} -c > \%s.gz\n" : 
           "> \%s\n" );
       print MAKE "\t\@echo Installing man pages...\n";
       while ( scalar @allman ) {
          $allman[1] =~ m/\.(\d+)$/;
          printf MAKE $cmd, $1, splice( @allman, 0, 2);
       }
    }
    print MAKE "\n";

    print MAKE <<EOF;

MD=$^X Makefile.PL --md

dirs: @alldirs

@alldirs:
\t\@\$(MD) @alldirs

bindist: all
\t$^X Makefile.PL --dist bin $PRJ-$VERSION

zipdist:
\t$^X Makefile.PL --dist zip $PRJ-$VERSION

tardist:
\t$^X Makefile.PL --dist tar $PRJ-$VERSION

dist: tardist
\t\$(RM) $PRJ-$VERSION.tar.gz
\t\@gzip -9 $PRJ-$VERSION.tar

EOF

# WARNING - dist targets work only for Prima-1.06 and higher

    close MAKE;
    print "\nDone.\n";

1;

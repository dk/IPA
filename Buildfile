#! /usr/bin/perl -w
#
#  Copyright (c) 1997-1999 The Protein Laboratory, University of Copenhagen
#  All rights reserved.
#
#  $Id$
#
#.code

use strict;
use Config;
use Cwd;
use File::Path;
use File::Basename;
use Prima::Gencls;
use TracedErr;
use Carp qw( verbose);

$| = 1;

=head1
sub cls2c
{
    my ( $self, $target_name, $deps, $oodeps, $target) = @_;
    my $dirout = dirname( $target_name) || '';
    print "Making .h and .inc out of $deps->[ 0]", $dirout ? " in directory $dirout" : '', ".\n";
    gencls( $deps->[ 0],
	    genInc => 1,
	    genH => 1,
	    dirOut => $dirout,
	  );
    return 1;
}

sub cls_depend
{
    my ( $self, @deps) = @_;
    print "Generating dependencies for $deps[ 0].\n";
    my @clsdeps = gencls( $deps[ 0], depend => 1);
    $self->trace( "Dependencies for $deps[ 0]: (@clsdeps)\n");
    return @clsdeps;
}

sub c_dep_ign
{
    my ( $self, @deps) = @_;
    print "Generating dependencies for $deps[ 0].\n";
    return $cc_types{ $self->var( 'CCTYPE')}->{ cdep}->( $self->expand( $deps[ 0]),
							 $self->var( 'CC'),
							 $self->var( 'CCFLAGS'),
							 [ $self->var( 'CINCPATH')],
							 1,
						       );
}
=cut

my ( $ctype, $ccflags, $ldflags, @libs, @IPAdeps);
if ( $^O =~ /win32/i) {
   $ccflags = '-Od -MD -W3 -WX -Zi -DHAVE_IO_H=1 -D_CONSOLE -DPERL_SUBVERSION=02 -DHAVE_BOOLEAN=1 -DWIN32 -DHAVE_STRICMP=1 -DNO_STRICT -DNDEBUG -DHAVE__SNPRINTF=1 -DPERL_POLLUTE=1 -DPERL_PATCHLEVEL=5 -DHAVE_CONFIG_H=1 ';
   $ldflags =  '';
   @libs = ( 'Prima.lib', 'perl.lib', map { qpath( $_)} split ' ', $Config{ libs});
   $ctype = 'msvc';
   @IPAdeps = ( '${OUTPATH${SHOBJEXT}}/IPA${SHOBJEXT}', '${RUNDIR}/IPA.def');
}
else {
   $ccflags = '-Wall -Werror -g -O2 -DPERL_SUBVERSION=3 -DPERL_PATCHLEVEL=5 -DHAVE_STRCASECMP=1 -DHAVE_CONFIG_H=1 ';
   $ldflags = '-g';
   @libs = ();
   $ctype = 'gnu';
   @IPAdeps = ( '${OUTPATH${SHOBJEXT}}/IPA${SHOBJEXT}');
}

set_variables(
              PROJDIR => '${RUNDIR}',
	      'OUTPATH${OBJEXT}' => '${RUNDIR}/obj',
	      'OUTPATH${SHOBJEXT}' => '${RUNDIR}/obj',
	      'OUTPATH${SHLIBEXT}' => '${RUNDIR}/auto/IPA',
	      'OUTPATH.h' => '${RUNDIR}/include/generic',
	      'OUTPATH.inc' => '${RUNDIR}/include/generic',
	      'SRCPATH${SHOBJEXT}' => '${OUTPATH${SHOBJEXT}}',
	      IPADEPS => [ @IPAdeps],
	      PRIMADIR => 'c:/home/Prima/src',
 	      PREFIX => 'c:/usr/local/perl/5.00502/lib/MSWin32-x86',
	      INSTALL_SRC => [ '${OUTPATH${SHLIBEXT}}/IPA${SHLIBEXT}', '${RUNDIR}/IPA.pm',],
	      INSTALL_DST => [  '${PREFIX}/auto/IPA', '${PREFIX}',],
	      ALLDEPS => [ 'IPA.c', 'IPA.cls'],
	      ALLDIRS => [],
	     );

add_variables(
	      CCFLAGS => $ccflags,
	      LDFLAGS => $ldflags,
	      CTYPE => $ctype,
	      CINCPATH => [
			   '${RUNDIR}/include',
			   '${RUNDIR}/include/generic',
			   '${OUTPATH.h}/${SUBDIR}',
			   qpath( $Config{installarchlib}) . "/CORE",
			   '${PRIMADIR}/include',
			   '${PRIMADIR}/include/generic',
			  ],
	      LIBPATH => [
			  '${PRIMADIR}/auto/Prima',
			  qpath( $Config{installarchlib}) . "/CORE",
			  '${OUTPATH${SHLIBEXT}}',
			 ],
	      LIBS => [ @libs], #'Prima',
	     );

#print $Devel::Builder::defaultBuilderObject->expand( '${RUNDIR}/include'), "\n";
#print join( "\n", $Devel::Builder::defaultBuilderObject->expand( '${CINCPATH}')), "\n";
#exit;

set_rules(
	  [ [ '.h', '.inc'] => '.cls',
	    \&cls2c,
	    fragile => 1,
	    implicit => 1,
	  ],

	  [ '${OBJEXT}' => [ '.c', '.C', '.cxx'],
	    \&c2obj,
	    fragile => 1,
	    implicit => 1,
	  ],

	  [ '${SHOBJEXT}' => [ '.c', '.C', '.cxx'],
	    \&c2obj,
	    fragile => 1,
	    implicit => 1,
	  ],

	  [ all => [
		    'dirs',
		    'Buildfile.dep',
		    '${OUTPATH${SHLIBEXT}}/IPA${SHLIBEXT}',
		   ],
	    sub { print "\nDone...\n"; 1;},
	    phony => 1,
	    root => 1,
	  ],

	  [ 'Buildfile.dep' => 'Buildfile',
	    sub {
		my ( $self) = @_;
		my @alldeps = $self->var( 'ALLDEPS');
		my @deplist;
		foreach my $src ( @alldeps) {
		    my ( $trg, $dep);
		    $trg = $src;
		    if ( $src =~ /\.c$/) {
			$trg =~ s/\.c$/\$\{SHOBJEXT\}/;
			$trg = '${OUTPATH${SHOBJEXT}}/' . $trg;
			$dep = [ c_dep_ign( $self, $src)];
		    }
		    elsif ( $src =~ /\.cls$/) {
			$trg =~ s/\.cls//;
			$trg = [ "\${OUTPATH.h}/$trg.h", "\${OUTPATH.inc}/$trg.inc"];
			$dep = [ $src, cls_depend( $self, $src)];
		    }
		    else {
			print STDERR "Don't know how to build dependencies for $src.\n";
			return 0;
		    }
		    push @deplist, $trg, $dep;
		}
		return save_dependencies( @deplist);
	    },
	  ],

	  [ depend => 'Buildfile.dep',
	    undef,
	    begin => sub { return unlink 'Buildfile.dep';},
	  ],

	  [ install => all =>
	    sub {
		my ( $self) = @_;
		mkpath [ $self->expand( '${PREFIX}/IPA'),
			 $self->expand( '${PREFIX}/auto/IPA')], 0, 0755;
		install(
			src => [ $self->expand( '${INSTALL_SRC}')],
			dst => [ $self->expand( '${INSTALL_DST}')],
			autodir => 1,
			verbose => 1,
		       ) or die "install: $!";
		1;
	    },
	  ],

	  [ clean => '',
	    sub {
		my ( $self) = shift;
		print "Cleaning...";
		rmtree(
		       [
			$self->expand( '${OUTPATH.h}'),
			$self->expand( '${PROJDIR}/auto'),
			$self->expand( '${OUTPATH${SHOBJEXT}}'),
		       ],
		       0,
		       0,
		      );
		unlink $self->expand( '${PROJDIR}/Buildfile.dep');
		print " done.\n";
		return 1;
	    },
	  ],

	  [ dirs => '',
	    sub
	    {
		my ( $self) = @_;
		my @dirs = (
			    $self->expand( '${OUTPATH.h}'),
			    $self->expand( '${OUTPATH${SHOBJEXT}}'),
			    $self->expand( '${OUTPATH${SHLIBEXT}}'),
			    $self->expand( '${ALLDIRS}'),
			   );
		foreach my $d ( @dirs) {
		    unless ( -d $d) {
			print "Creating directory $d\n";
			scalar( mkpath $d) || ( print STDERR "$!", return 0);
		    }
		}
		return 1;
	    },
	  ],

	  [ '${OUTPATH${SHLIBEXT}}/IPA${SHLIBEXT}' => '${IPADEPS}',
	    undef,
	    end => sub
	    {
		my ( $self, $target_name, $deps, $oodeps, $target) = @_;
		if ( $target->{ _processing}->{ _succeed}) {
		    $self->add_var( 'LIBS', 'IPA${LIBEXT}') if $^O =~ /win32|os2/i;
		}
		return 1;
	    },
	  ],
	 );

print "Scanning subdirs:";
my @alldirs = qw( Geometry Local Morphology Global Misc Point);
foreach my $subdir ( @alldirs) {
    print " $subdir";
    my $cwd = cwd;
    chdir $subdir;
    my @allc = <*.c>;
    my @allcls = <*.cls>;
    add_var( 'ALLDEPS',
	     ( map { "$subdir/$_"} @allc),
	     ( map { "$subdir/$_"} @allcls),
	   );
    my @allobj = map { s/\.c$/\$\{SHOBJEXT\}/; "\${OUTPATH\${SHOBJEXT}}/$subdir/$_"} @allc;
    if ( $^O =~ /win32/i) {
       push @allobj, "$subdir/$subdir.def";
    }
    add_rules(
	      [ "\${OUTPATH\${SHLIBEXT}}/$subdir/$subdir\${SHLIBEXT}" => [ @allobj],
		undef,
		end => sub {
		    my ( $self, $target_name, $deps, $oodeps, $target) = @_;
		    $self->add_variables(
					 INSTALL_SRC => [ 
							 $target_name,
							 "\${RUNDIR}/$subdir/$subdir.pm",
							],
					 INSTALL_DST => [
							 "\${PREFIX}/auto/IPA/$subdir/$subdir\${SHLIBEXT}",
							 "\${PREFIX}/IPA",
							],
					);
		    return 1;
		},
		subdir => $subdir,
		implicit => 0,
	      ],
	      [ 'all' => "\${OUTPATH\${SHLIBEXT}}/$subdir/$subdir\${SHLIBEXT}",
		undef,
		implicit => 0,
	      ],
	     );
    chdir $cwd;
    add_var( ALLDIRS => [ '${OUTPATH.h}/' . $subdir,
			  '${OUTPATH${SHOBJEXT}}/' . $subdir,
			  '${OUTPATH${SHLIBEXT}}/' . $subdir,
			]
	   );
}

print ".\n";

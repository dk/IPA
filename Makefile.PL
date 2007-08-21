# $Id$
package MY;
use ExtUtils::MakeMaker;
use Config;

my ( $i, $d, $l, $co) = ('','','','');
eval { 
	require Prima::Config;
	$d  = join(' ', map { "-D$_" } @{$Prima::Config::Config{cdefs}});
	$i  = join(' ', map { "-I$_" } @{$Prima::Config::Config{incpaths}}, qw(include include/generic));
	$l  = $Prima::Config::Config{libname} if $^O =~ /win32|os2/i;
	$co = $Prima::Config::Config{cobjflag};
};

my @files = map {
	s/\.c$//;
	m/([^\\\/]*)$/;
	{ src => $_, name => $1 }
} <*.c>, <*/*.c>;

$_-> {is_class} = 1 for grep { -f $_-> {src} . '.cls' } @files;

# non-class c files may require h/inc from gencls build
@files = (
	(grep { $_-> {is_class}     } @files),
	(grep { not $_-> {is_class} } @files),
);

my $o = $Config{obj_ext};

sub postamble
{
	my $self = shift;
	my $t = $self->SUPER::postamble(@_);
	
	for ( @files) {
		next unless $_-> {is_class};
		my ( $n, $c) = ( $_-> {name}, $_-> {src});
	  	$t .= <<H

include/generic/$n.h: Makefile $c.cls
\tgencls --inc --h $c.cls include/generic

$c$o: Makefile $c.c include/IPAsupp.h include/generic/$n.h include/generic/$n.inc

H
	}

	return $t;
}

sub c_o
{
	my $t = shift-> SUPER::c_o(@_);
	unless ( $t =~ /.c\$\(OBJ_EXT\):\n\t.*\$\*\$\(OBJ_EXT\)/ ) {
		$t =~ s/(\.c\$\(OBJ_EXT\):\n\t.*)/$1 $co\$*\$(OBJ_EXT)/;
	}
	return $t;
}

sub clean
{
	my $self = shift;
	my $t = $self->SUPER::clean(@_);
	$t .= "\t\$(RM_F) " . join( ' ', 
		map  { "include/generic/$_" } 
		map  { $_-> {name} . '.h', $_-> {name} . '.inc' } 
		grep { $_-> {is_class} }
		@files) .
		"\n";
	$t .= "\t\$(RM_F) " . join( ' ', 
		map { $_-> {src} . $o }
		@files);
	return $t;
}


WriteMakefile(
	NAME              => 'IPA',
	VERSION_FROM      => 'IPA.pm',
	PREREQ_PM         => {
		'Prima'		=> 1,
	},
	ABSTRACT_FROM     => 'IPA.pm',
	AUTHOR            => 'Dmitry Karasik <dmitry@karasik.eu.org>',
	LIBS              => [ $l ],
	DEFINE            => $d,
	INC               => $i,
	OBJECT            => join(' ', map { $_->{src} . '$(OBJ_EXT)' } @files),
	PM                => {
		# PM
		'IPA.pm'  	                          => '$(INST_LIBDIR)/IPA.pm',
		'Global/Region.pm'  	                  => '$(INST_LIBDIR)/IPA/Region.pm',
		( map  { ( "$_->{src}.pm"                 => "\$(INST_LIBDIR)/IPA/$_->{name}.pm" ) }
		grep { $_-> {is_class} }
		@files),
		# IPA HEADERS
		'include/IPAsupp.h'  	                  => '$(INST_LIBDIR)/IPA/CORE/IPAsupp.h',
		( map  { ( "include/generic/$_->{name}.h" => "\$(INST_LIBDIR)/IPA/CORE/generic/$_->{name}.h" ) }
		grep { $_-> {is_class} }
		@files),
		# CLASS HEADERS
		( map { ( $_                              => '$(INST_LIBDIR)/IPA/' . $_ ) }
		map  { glob "$_->{name}/*.h" }
		grep { $_-> {is_class} }
		@files),
	},
	EXE_FILES         => ['bin/iterm'],
);

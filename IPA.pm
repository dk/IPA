# $Id$
package IPA;
use Prima::Classes;
use strict;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);

BEGIN {
    if ( $^O =~ /freebsd/i) {
	( my $ver = `/usr/bin/uname -r`) =~ s/^(\d+\.\d+).*$/$1/;
	if ( $ver >= 3.4) {
	    eval "sub dl_load_flags { 0x01 }";
	}
    }
}

$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw();
%EXPORT_TAGS = ();

bootstrap IPA $VERSION;

use constant combineMaxAbs       => 1;
use constant combineSumAbs       => 2;
use constant combineSum          => 3;
use constant combineSqrt         => 4;
use constant combineSignedMaxAbs => 5;

use constant conversionTruncAbs  => 1;
use constant conversionTrunc     => 2;
use constant conversionScale     => 3;
use constant conversionScaleAbs  => 4;

1;

# $Id$
package IPA::Geometry;
use IPA;
use strict;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);
$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw(mirror shift_rotate);
%EXPORT_TAGS = (one2one => [qw(mirror)]);
sub dl_load_flags { 0x01 };

use constant vertical => 1;
use constant horizontal => 2;

bootstrap IPA::Geometry $VERSION;

1;


# $Id$
package IPA::Misc;
use strict;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);
$VERSION = '1.00';
@EXPORT = qw();
@EXPORT_OK = qw( split_channels combine_channels histogram);
%EXPORT_TAGS = ();
sub dl_load_flags { 0x01 };

bootstrap IPA::Misc $VERSION;

1;


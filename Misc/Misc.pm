# $Id$
package IPA::Misc;
use strict;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '1.01';
@EXPORT = qw();
@EXPORT_OK = qw( split_channels combine_channels histogram);
%EXPORT_TAGS = ();

1;


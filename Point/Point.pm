# $Id$
package IPA::Point;
use strict;
use IPA;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);
$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw(combine threshold gamma remap subtract mask);
%EXPORT_TAGS = ();

bootstrap IPA::Point $VERSION;

1;


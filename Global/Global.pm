# $Id$
package IPA::Global;
use strict;
use IPA;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);
$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw(close_edges fill_holes area_filter identify_contours);
%EXPORT_TAGS = (tracks => [qw(close_edges)]);

bootstrap IPA::Global $VERSION;

1;


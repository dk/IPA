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
@EXPORT_OK = qw(mirror);
%EXPORT_TAGS = (one2one => [qw(mirror)]);

use constant mirrorVertical => 1;
use constant mirrorHorizontal => 2;

bootstrap IPA::Geometry $VERSION;

1;


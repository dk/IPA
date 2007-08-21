# $Id$
package IPA::Geometry;

use strict;
require Exporter;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '0.02';
@EXPORT = qw();
@EXPORT_OK = qw(mirror shift_rotate rotate90 rotate180);
%EXPORT_TAGS = (one2one => [qw(mirror)]);

use constant vertical => 1;
use constant horizontal => 2;


1;


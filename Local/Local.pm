# $Id$
package IPA::Local;
use strict;
use IPA;
require Exporter;
require DynaLoader;

use constant sobelColumn         => 1;
use constant sobelRow            => 2;
use constant sobelNWSE           => 4;
use constant sobelNESW           => 8;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);
$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw(crispening 
                sobel 
                GEF 
                SDEF 
                deriche
                filter3x3 
                median 
                unionFind
               );
%EXPORT_TAGS = (enhancement => [qw(crispening)], edgedetect => [qw(sobel GEF SDEF deriche)]);

bootstrap IPA::Local $VERSION;

1;


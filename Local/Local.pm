# $Id$
package IPA::Local;

use strict;
require Exporter;

use constant sobelColumn         => 1;
use constant sobelRow            => 2;
use constant sobelNWSE           => 4;
use constant sobelNESW           => 8;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter);
$VERSION = '0.02';
@EXPORT = qw();
@EXPORT_OK = qw(crispening 
                sobel 
                GEF 
                SDEF 
                deriche
                filter3x3 
                median 
                unionFind
		hysteresis
                gaussian
                laplacian
                gradients
                canny
                nms
                scale
                ridge
                convolution
                zerocross
               );
%EXPORT_TAGS = (enhancement => [qw(crispening)], 
                edgedetect => [qw(sobel GEF SDEF deriche hysteresis canny)]);

1;


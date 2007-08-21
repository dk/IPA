# $Id$
package IPA::Point;
use strict;
require Exporter;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter );
$VERSION = '0.02';
@EXPORT = qw();
@EXPORT_OK = qw(combine threshold gamma remap subtract mask equalize ab log exp average);
%EXPORT_TAGS = ();

# histogram equalization
sub equalize
{
   my $i = $_[0];
   my @h = ( 0, IPA::Misc::histogram( $i));
   my $sz = $i-> width * $i-> height;
   my @map = (0);
   my $j;
   for ( $j = 1; $j < @h; $j++) {
      $map[$j] = $map[$j - 1] + $h[$j];
   }
   for ( @map) {
      $_ = $_ * 255 / $sz;
      $_ = ( $_ > 255) ? 255 : int($_); 
   }
   shift @map;
   return IPA::Point::remap( $i, lookup => \@map);
}


1;


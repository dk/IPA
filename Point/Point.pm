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
@EXPORT_OK = qw(combine threshold gamma remap subtract mask equalize);
%EXPORT_TAGS = ();
sub dl_load_flags { 0x01 };

bootstrap IPA::Point $VERSION;

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


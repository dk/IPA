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
@EXPORT_OK = qw(close_edges fill_holes area_filter identify_contours fft band_filter butterworth fourier);
%EXPORT_TAGS = (tracks => [qw(close_edges)]);

bootstrap IPA::Global $VERSION;

sub pow2
{
   my ( $i, $j) = ( 1, $_[0]);
   $i <<= 1, $j >>= 1 while $j > 1;
   return $i == $_[0], $i;
}   
# adjusting image to the power of 2 for the FFT transform

sub pow2wrapper1
{
   my ($i,$profile) = @_;

   my ($oh, $ow) = $i-> size;
   my ( $okw, $w1) = pow2( $oh);
   my ( $okh, $h1) = pow2( $ow);
   my $resize = !$okw || !$okh;
   if ( $resize) {
      unless ( $profile->{lowquality}) {
         $w1 *= 2 unless $okw;
         $h1 *= 2 unless $okh;
      }
      $i = $i-> dup;
      $i-> size( $w1, $h1);
   }
   return ( $i, $oh, $ow, $resize);
}   

sub pow2wrapper2
{
   my ( $i, $oh, $ow, $resize) = @_;
   $i-> size( $ow, $oh) if $i && $resize;
   return $i;
}   

# wrapper for ::band_filter
sub butterworth
{
   my ( $i, %profile) = @_;
   die "IPA::Global::band: Not an image passed\n" unless $i;
   my @psdata;
   $profile{spatial} = 1 if ($i-> type & im::Category) != im::ComplexNumber;
   ( $i, @psdata) = pow2wrapper1( $i, \%profile) if $profile{spatial};
   $i = band_filter( $i, %profile);
   pow2wrapper2( $i, @psdata) if $profile{spatial};
   return $i;
}   

# wrapper for fft
sub fourier
{
   my ( $i, %profile) = @_;
   die "IPA::Global::fourier: Not an image passed\n" unless $i;
   my @psdata;
   ( $i, @psdata) = pow2wrapper1( $i, \%profile) if $profile{spatial};
   $i = fft( $i, %profile);
   pow2wrapper2( $i, @psdata);
   return $i;
}   


1;


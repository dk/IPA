#! /usr/bin/perl
# $Id$

use strict;
use warnings;

my @namespace;

BEGIN { @namespace = qw(
	IPA::Geometry::rotate90
	IPA::Geometry::rotate180
	IPA::Geometry::mirror
	IPA::Geometry::shift_rotate

	IPA::Global::close_edges
	IPA::Global::fill_holes
	IPA::Global::area_filter
	IPA::Global::identify_contours
	IPA::Global::identify_scanlines
	IPA::Global::fft
	IPA::Global::band_filter

	IPA::Local::crispening
	IPA::Local::sobel
	IPA::Local::GEF
	IPA::Local::SDEF
	IPA::Local::deriche
	IPA::Local::filter3x3
	IPA::Local::median
	IPA::Local::unionFind
	IPA::Local::hysteresis
	IPA::Local::gaussian
	IPA::Local::laplacian
	IPA::Local::gradients
	IPA::Local::canny
	IPA::Local::nms
	IPA::Local::scale
	IPA::Local::ridge
	IPA::Local::convolution
	IPA::Local::zerocross

	IPA::Misc::split_channels
	IPA::Misc::combine_channels
	IPA::Misc::histogram

	IPA::Morphology::BWTransform
	IPA::Morphology::dilate
	IPA::Morphology::erode
	IPA::Morphology::algebraic_difference
	IPA::Morphology::watershed
	IPA::Morphology::reconstruct
	IPA::Morphology::thinning

	IPA::Point::combine
	IPA::Point::threshold
	IPA::Point::gamma
	IPA::Point::remap
	IPA::Point::subtract
	IPA::Point::mask
	IPA::Point::average
	IPA::Point::ab
	IPA::Point::exp
	IPA::Point::log
);};

use Test::More tests => 9 + @namespace;

use_ok('Prima::noX11');
use_ok('IPA');

ok( UNIVERSAL-> can($_), $_ ) for @namespace;

use_ok('IPA::Local');
use_ok('IPA::Global');
use_ok('IPA::Point');
use_ok('IPA::Region');
use_ok('IPA::Morphology');
use_ok('IPA::Misc');
use_ok('IPA::Geometry');

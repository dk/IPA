/* $Id$ */

#include "IPAsupp.h"
#include "Misc.h"
#include "Misc.inc"
#include "MiscSupp.h"

PImage_vmt CImage;

XS( boot_IPA__Misc)
{
    dXSARGS;
    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Misc_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}


Histogram *
IPA__Misc_histogram( PImage img)
{
    const char *method = "IPA::Point::histogram";
    Histogram *histogram;
    int x, y;
    Byte *p;

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);

    if ( ( img->type & imBPP) != imbpp8) {
	croak( "%s: unsupported image type", method);
    }

    histogram = alloc1z( Histogram);
    p = img->data;
    if ( ! p) {
	croak( "%s: image doesn't contain any data", method);
    }
    for ( y = 0; y < img->h; y++, p += img->lineSize) {
	for ( x = 0; x < img->w; x++) {
	    ( *histogram)[ p[ x]]++;
	}
    }
    return histogram;
}

/* $Id$ */

#include "IPAsupp.h"
#include "IPA.h"
#include "IPA.inc"

PImage_vmt CImage;

XS( boot_IPA)
{
    dXSARGS;
    // char* file = __FILE__;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

int AV2intp(SV *asv,int **array)
{
    return 0;
}

PImage create_compatible_image(PImage img,Bool copyData)
{
    PImage oimg;
    oimg=createImage(img->w,img->h,img->type);
    if (!oimg) {
        return NULL;
    }
    memcpy(oimg->palette,img->palette,img->palSize);
    if (copyData) {
        memcpy(oimg->data,img->data,img->dataSize);
    }
    return oimg;
}

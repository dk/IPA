/* $Id$ */

#include "IPAsupp.h"
#include "Misc.h"
#include "Misc.inc"
#include "MiscSupp.h"

PImage_vmt CImage;

XS( boot_IPA__Misc)
{
    dXSARGS;
    // char* file = __FILE__;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Misc_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}


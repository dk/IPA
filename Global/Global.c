/* $Id$ */

#include "IPAsupp.h"
#include "Global.h"
#include "Global.inc"
#include "GlobalSupp.h"
#include "gsclose.h"

PImage_vmt CImage;

static SV **temporary_prf_Sv;

XS( boot_IPA__Global)
{
    dXSARGS;
    // char* file = __FILE__;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Global_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

PImage IPA__Global_close_edges(PImage img,HV *profile)
{
    const char *method="IPA::Global::close_edges";
    PImage gradient;
    int maxlen,minedgelen,mingradient;

    if (!img) {
        croak("%s: null image passed",method);
    }
    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }
    if (pexist(gradient)) {
        SV *gsv;
        gsv=pget_sv(gradient);
        if (!gsv) {
            croak("%s: NULL gradient passed",method);
        }
        gradient=(PImage)gimme_the_mate(gsv);
        if (!kind_of((Handle)gradient,CImage)) {
            croak("%s: gradient isn't an image",method);
        }
        if (gradient->type!=imByte) {
            croak("%s: unsupported type of gradient image",method);
        }
        if (gradient->w!=img->w || gradient->h!=img->h) {
            croak("%s: image and gradient have different sizes",method);
        }
    }
    else {
        croak("%s: gradient missing",method);
    }
    if (pexist(maxlen)) {
        maxlen=pget_i(maxlen);
        if (maxlen<0) {
            croak("%s: maxlen can't be negative",method);
        }
    }
    else {
        croak("%s: maximum length of new edge missing",method);
    }
    if (pexist(minedgelen)) {
        minedgelen=pget_i(minedgelen);
        if (minedgelen<0) {
            croak("%s: minedgelen can't be negative",method);
        }
    }
    else {
        croak("%s: minimum length of a present edge missing",method);
    }
    if (pexist(mingradient)) {
        mingradient=pget_i(mingradient);
        if (mingradient<0) {
            croak("%s: mingradient can't be negative",method);
        }
    }
    else {
        croak("%s: minimal gradient value missing",method);
    }

    //log_write("going to produce closed edges.");

    return gs_close_edges(img,gradient,maxlen,minedgelen,mingradient);
}

/* $Id$ */

#include "IPAsupp.h"
#include "Geometry.h"
#include "Geometry.inc"
#include "GeometrySupp.h"

static SV **temporary_prf_Sv;

PImage_vmt CImage;

XS( boot_IPA__Geometry)
{
    dXSARGS;
    // char* file = __FILE__;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Geometry_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

PImage IPA__Geometry_mirror(PImage img,HV *profile)
{
    const char *method="IPA::Geometry::mirror";
    PImage oimg;
    int mType=0;
    int y;

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);
    
    if (pexist(type)) {
        mType=pget_i(type);
    }

    switch (mType) {
        case HORIZONTAL:
            {
                Byte *pi,*po;
                oimg=createNamedImage(img->w,img->h,img->type,method);
                if ((oimg->type & imGrayScale)==0) {
                    memcpy(oimg->palette,img->palette,img->palSize*sizeof(RGBColor));
                    oimg->palSize=img->palSize;
                }
                if (!oimg) {
                    croak("%s: can't create output image",method);
                }
                //log_write("oimg: %dx%d, type:%x lineSize:<%d >%d",oimg->w,oimg->h,oimg->type,img->lineSize,oimg->lineSize);
                for (y=0,pi=img->data,po=(oimg->data+oimg->lineSize*(oimg->h-1)); y<img->h; y++,pi+=img->lineSize,po-=oimg->lineSize) {
                    memcpy(po,pi,img->lineSize);
                }
            }
            break;
        case VERTICAL:
            {
                int x;
                oimg=createNamedImage(img->w,img->h,img->type,method);
                if ((oimg->type & imGrayScale)==0) {
                    memcpy(oimg->palette,img->palette,img->palSize*sizeof(RGBColor));
                    oimg->palSize=img->palSize;
                }
                switch (img->type & imBPP) {
                    case imbpp1:
                        {
                            Byte *pi,*po;
                            int x1;
                            for (y=0,pi=img->data,po=oimg->data; y<img->h; y++,pi+=img->lineSize,po+=oimg->lineSize) {
                                po[(oimg->w-1)>>3]=0;
                                for (x=0,x1=(oimg->w-1); x<img->w; x++,x1--) {
                                    if ((x1%8)==7) {
                                        po[x1>>3]=0;
                                    }
                                    po[x1>>3]|=((pi[x>>3] >> (7-(x & 7))) & 1)<<(7-(x1 & 7));
                                }
                            }
                        }
                        break;
                    case imbpp4:
                        {
                            Byte *pi,*po,p;
                            int x1;
                            for (y=0,pi=img->data,po=oimg->data; y<img->h; y++,pi+=img->lineSize,po+=oimg->lineSize) {
                                po[(oimg->w-1)>>1]=0;
                                for (x=0,x1=(oimg->w-1); x<img->w; x++,x1--) {
                                    p=pi[x>>1];
                                    p=(x&1 ? p : (p>>4)) & 0x0f;
                                    if (x1&1) {
                                        po[x1>>1]=p;
                                    }
                                    else {
                                        po[x1>>1]|=p<<4;
                                    }
                                }
                            }
                        }
                        break;
                    case imbpp8:
                        {
                            Byte *pi,*po;
                            for (y=0,pi=img->data,po=oimg->data; y<img->h; y++,pi+=img->lineSize,po+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    po[img->w-x-1]=pi[x];
                                }
                            }
                        }
                        break;
                    case imbpp16:
                        {
                            short *pi,*po;
                            for (y=0,pi=(short*)img->data,po=(short*)oimg->data; y<img->h; y++,((Byte*)pi)+=img->lineSize,((Byte*)po)+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    po[img->w-x-1]=pi[x];
                                }
                            }
                        }
                        break;
                    case imbpp24:
                        {
                            PRGBColor pi,po;
                            for (y=0,pi=(PRGBColor)img->data,po=(PRGBColor)oimg->data; y<img->h; y++,((Byte*)pi)+=img->lineSize,((Byte*)po)+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    po[img->w-x-1]=pi[x];
                                }
                            }
                        }
                        break;
                    case imbpp32:
                        {
                            U32 *pi,*po;
                            for (y=0,pi=(U32*)img->data,po=(U32*)oimg->data; y<img->h; y++,((Byte*)pi)+=img->lineSize,((Byte*)po)+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    po[img->w-x-1]=pi[x];
                                }
                            }
                        }
                        break;
                    case imbpp64:
                        {
                            typedef Byte pix64[8];
                            pix64 *pi,*po;
                            for (y=0,pi=(pix64*)img->data,po=(pix64*)oimg->data; y<img->h; y++,((Byte*)pi)+=img->lineSize,((Byte*)po)+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    memcpy(po,pi,sizeof(pix64));
                                }
                            }
                        }
                        break;
                    case imbpp128:
                        {
                            typedef Byte pix128[8];
                            pix128 *pi,*po;
                            for (y=0,pi=(pix128*)img->data,po=(pix128*)oimg->data; y<img->h; y++,((Byte*)pi)+=img->lineSize,((Byte*)po)+=oimg->lineSize) {
                                for (x=0; x<img->w; x++) {
                                    memcpy(po,pi,sizeof(pix128));
                                }
                            }
                        }
                        break;
                    default:
                        croak("%s: unsupported image type",method);
                }
            }
            break;
        default:
            croak("%s: %d is unknown type of mirroring",method,mType);
    }

    return oimg;
}

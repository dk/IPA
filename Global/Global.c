/* $Id$ */

#include "IPAsupp.h"
#include "Global.h"
#include "Global.inc"
#include "GlobalSupp.h"
#include "gsclose.h"

PImage_vmt CImage;

XS( boot_IPA__Global)
{
    dXSARGS;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Global_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

PImage IPA__Global_close_edges(PImage img,HV *profile)
{
    dPROFILE;
    const char *method="IPA::Global::close_edges";
    PImage gradient;
    int maxlen,minedgelen,mingradient;

    if ( !img || !kind_of(( Handle) img, CImage)) {
        croak("%s: not an image passed",method);
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

    return gs_close_edges(img,gradient,maxlen,minedgelen,mingradient);
}

/* draws horizontal line from x1,y to x2,y with color.
   optimized for byte,short,long,float,double */
static void
hline( PImage image, int x1, int x2, int y, double color)
{
   int type, ls, i;
   Byte * data;
   if ( x2 < x1) {
      int x = x2;
      x2 = x1;
      x1 = x;
   }
   if ( x2 < 0 || x1 >= image-> w || y < 0 || y >= image-> h) return;

   type = image-> type;
   ls   = image-> lineSize;
   if ( type & (imComplexNumber|imTrigComplexNumber)) {
      type &= ~(imComplexNumber|imTrigComplexNumber);
      x1 *= 2;
      x2 *= 2;
   }
   data = image-> data + y * ls + x1 * ( type & imBPP) / 8;

   if ( type & imRealNumber) {
      switch ( type) {
      case imFloat:
         {
            float * d = ( float *) data;
            float c = color;
            for ( i = x1; i <= x2; i++) *(d++) = c;
         }
         break;
      case imDouble:
         {
            double * d = ( double *) data;
            for ( i = x1; i <= x2; i++) *(d++) = color;
         }
         break;
      default:
         croak("Unsupported float image type(%x)", image-> type);
      }
   } else switch ( type & imBPP) {
      case 1:
      case 4:
      case 24: 
         /* fall back to Image::pixel */
         {
            SV * sv = newSViv( color);
            for ( i = x1; i <= x2; i++) 
               image-> self-> pixel(( Handle) image, 1, i, y, sv);
            sv_free( sv);
         }
         break;
      case 8:
         {
            Byte c = ( color > 255) ? 255 : (( color < 0) ? 0 : color + .5);
            for ( i = x1; i <= x2; i++) *(data++) = c;
         }
         break;
      case 16:
         {
            Short * d = ( Short *) data;
            Short c = ( color > 32768) ? 32768 : (( color < -32767) ? -32767 : color + .5);
            for ( i = x1; i <= x2; i++) *(d++) = c;
         }
         break;
      case 32:
         {
            Long * d = ( Long *) data;
            Long c = ( color > 0x7fffffff) ? 0x7fffffff : (( color < -0x7fffffff) ? -0x7fffffff : color + .5);
            for ( i = x1; i <= x2; i++) *(d++) = c;
         }
         break;
   }
}

/* draws horizontal lines, passed as triples [ x1, x2, y] in points */
void 
IPA__Global_hlines( PImage input, int x, int y, SV * points, double color)
{
   AV * av;
   int i, count;
   if ( !SvROK( points) || ( SvTYPE( SvRV( points)) != SVt_PVAV)) 
      croak("IPA::Global::hlines: invalid array reference passed");
   av = ( AV *) SvRV( points);
   count = av_len( av) + 1;
   if ( count % 3) 
      croak("IPA::Global::hlines: number of elements in an array must be a multiple to 3");
   count /= 3;
   if ( count < 3) return;

   for ( i = 0; i < count; i++) {
       SV** psvx1 = av_fetch( av, i * 3, 0),
         ** psvx2 = av_fetch( av, i * 3 + 1, 0),
         ** psvy  = av_fetch( av, i * 3 + 2, 0);
       if (( psvx1 == nil) || ( psvy == nil) || (psvx2 == nil))
          croak("IPA::Global::hlines: array panic on triplet #%d", i);
       hline( input, x + SvIV( *psvx1), x + SvIV( *psvx2), y + SvIV( *psvy), color);
   }
   input-> self-> update_change(( Handle) input);
}

void 
IPA__Global_bar( PImage input, int x1, int y1, int x2, int y2, double color)
{
   for ( ; y1 <= y2; y1++) hline( input, x1, x2, y1, color);
   input-> self-> update_change(( Handle) input);
}

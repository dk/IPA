/* $Id$ */

#include "IPAsupp.h"
#include "Point.h"
#include "Point.inc"
#include "PointSupp.h"

static SV **temporary_prf_Sv;

#define pix( img, x, y)     ( (img)->type == imByte ? *( (img)->data + (img)->lineSize * (y) + (x)) : ( (img)->type == imShort ? ( ( short*)( (img)->data + (img)->lineSize * (y)))[(x)] : ( ( long*)( (img)->data + (img)->lineSize * (y)))[(x)]));

#ifndef PRIMA_TRUNC_PRESENT
   #define trunc(x)  ((( x) < 0) ? ceil( x) : floor( x))
#endif

PImage_vmt CImage;

XS( boot_IPA__Point)
{
    dXSARGS;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Point_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

PImage color_remap(const char *method,PImage img, unsigned char *lookup_table)
{
    int ypos,xpos;
    unsigned char *p,*po;
    PImage oimg;

    oimg=createNamedImage(img->w,img->h,imByte,method);
    if (!oimg) {
        croak("%s: can't create output image",method);
    }

    for (ypos=0,p=img->data,po=oimg->data; ypos<img->h; ypos++,p+=img->lineSize,po+=oimg->lineSize) {
        for (xpos=0; xpos<img->w; xpos++) {
            po[xpos]=lookup_table[p[xpos]];
        } /* endfor */
    } /* endfor */

    return oimg;
}

PImage IPA__Point_combine(HV *profile)
{
    const char *method="IPA::Point::combine";
    PImage *img = nil, oimg, bimg;
    int imgnum = 0;
    int conversionType=CONV_SCALE;
    int combineType=COMBINE_SUM;
    Bool rawOutput=false;
    int minval=0,maxval=0,range;
    long *bufp;
    int x,y,i;

    if (pexist(images)) {
        SV *sv=pget_sv(images);
        SV **mItem;
        AV *av;
        if (!SvROK( sv) || (SvTYPE(SvRV(sv))!=SVt_PVAV)) {
            croak("%s: images is not an array",method);
        }
        av=(AV*)SvRV(sv);
        imgnum=av_len(av)+1;
        if (imgnum<1) {
            croak("%s: images array size too small",method);
        }
        img=(PImage*)malloc(sizeof(PImage)*imgnum);
        for (i=0; i<imgnum; i++) {
            mItem=av_fetch(av,i,0);
            if (!mItem) {
                free(img);
                croak("%s: null image #%d",method,i);
            }
            img[i]=(PImage)gimme_the_mate(*mItem);
            if (!kind_of((Handle)img[i],CImage)) {
                free(img);
                croak("%s: element #%d of images array isn't image",method,i);
            }
            /*
            if (img[i]->type!=imLong) {
                free(img);
                croak("%s: image #%d of images array has wrong type",method,i);
            }
            */
            if (!img[i]->data) {
                free(img);
                croak("%s: image #%d of images array contains no data",method,i);
            }
            if (img[i]->w!=img[0]->w || img[i]->h!=img[0]->h) {
                free(img);
                croak("%s: image #%d of images array has different size",method,i);
            }
        }
    } else 
       croak("%s: 'images' array is not present", method);
    if (pexist(conversionType)) {
        conversionType=pget_i(conversionType);
        if (conversionType<CONV_FIRST || conversionType>CONV_LAST) {
            free(img);
            croak("%s: invalid conversion requested",method);
        }
    }
    if (pexist(combineType)) {
        combineType=pget_i(combineType);
        if (combineType<COMBINE_FIRST || combineType>COMBINE_LAST) {
            free(img);
            croak("%s: invalid combination method requested",method);
        }
    }
    if (pexist(rawOutput)) {
        rawOutput=pget_B(rawOutput);
    }

    bimg=createNamedImage(img[0]->w,img[0]->h,imLong,"combine buffer");
    /*
    imp=(long**)malloc(sizeof(long*)*imgnum);
    for (i=0; i<imgnum; i++) {
        imp[i]=(long*)img[i]->data;
    }
    */
    for (y=0,bufp=(long*)bimg->data; y<img[0]->h; y++,(*((Byte**)&bufp))+=bimg->lineSize) {
        for (x=0; x<img[0]->w; x++) {
            long absmax = 0, sum=0, val, absval, origmax = 0;
            for (i=0; i<imgnum; i++) {
                val = pix( img[i], x, y);
                absval = abs( val);
                switch (combineType) {
                    case COMBINE_SIGNEDMAXABS:
                    case COMBINE_MAXABS:
                        if (i==0 || absmax<absval) {
                            absmax=absval;
                            origmax=val;
                        }
                        break;
                    case COMBINE_SUMABS:
                        sum += absval;
                        break;
                    case COMBINE_SUM:
                        sum += val;
                        break;
                    case COMBINE_SQRT:
                        sum += val * val;
                        break;
                }
            }
            switch (combineType) {
                case COMBINE_SIGNEDMAXABS:
                    bufp[x]=origmax;
                    break;
                case COMBINE_MAXABS:
                    bufp[x]=absmax;
                    break;
                case COMBINE_SUMABS:
                    bufp[x]=sum;
                    break;
                case COMBINE_SUM:
                    bufp[x]=sum;
                    break;
                case COMBINE_SQRT:
                    bufp[x]=(long)sqrt(sum);
                    break;
            }
            if (!rawOutput && (conversionType==CONV_SCALEABS || conversionType==CONV_TRUNCABS)) {
                bufp[x]=abs(bufp[x]);
            }
            if (x==0 && y==0) {
                minval=maxval=bufp[x];
            }
            else {
                if (minval>bufp[x]) {
                    minval=bufp[x];
                }
                if (maxval<bufp[x]) {
                    maxval=bufp[x];
                }
            }
        }
        /*
        for (i=0; i<imgnum; i++) {
            ((Byte*)imp[i])+=img[0]->lineSize;
        }
        */
    }
    free(img);

    if (rawOutput) {
        oimg=bimg;
    }
    else {
        Byte *p;
        if (conversionType==CONV_SCALEABS) {
            maxval=abs(maxval);
            minval=abs(minval);
            if (minval>maxval) {
                long tmp=maxval;
                maxval=minval;
                minval=tmp;
            }
        }
        range=maxval-minval;
        if (range==0) {
            range=1;
        }
        oimg=createNamedImage(bimg->w,bimg->h,imByte,"combine result");
        for (y=0,bufp=(long*)bimg->data,p=oimg->data; y<bimg->h; y++,(*((Byte**)&bufp))+=bimg->lineSize,p+=oimg->lineSize) {
            for (x=0; x<bimg->w; x++) {
                switch (conversionType) {
                    case CONV_SCALE:
                    case CONV_SCALEABS:
                        p[x]=((bufp[x]-minval)*255L)/range;
                        break;
                    case CONV_TRUNCABS:
                    case CONV_TRUNC:
                        p[x]=bufp[x]>255 ? 255 : (bufp[x]<0 ? 0 : bufp[x]);
                        break;
                }
            }
        }
        destroyImage(bimg);
    }

    return oimg;
}

PImage IPA__Point_threshold(PImage img,HV *profile)
{
    const char *method="IPA::Point::threshold";
    int minvalue,maxvalue=255;
    unsigned char lookup_table[256];

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }
    if (pexist(minvalue)) {
        minvalue=pget_i(minvalue);
        if (minvalue<0 || minvalue>256) {
            croak("%s: incorrect minvalue %d",method,minvalue);
        }
    }
    else {
        croak("%s: minvalue required",method);
    }
    if (pexist(maxvalue)) {
        maxvalue=pget_i(maxvalue);
        if (maxvalue<0 || maxvalue>255) {
            croak("%s: incorrect maxvalue %d",method,minvalue);
        }
        if (maxvalue<minvalue && minvalue!=256) {
            croak("%s: maxvalue(%d) less than minvalue(%d)",method,maxvalue,minvalue);
        }
    }

    if (minvalue>0) {
        memset(lookup_table,0,minvalue);
    }
    if (minvalue<256) {
        memset(lookup_table+minvalue,255,maxvalue-minvalue+1);
        if (maxvalue<255) {
            memset(lookup_table+maxvalue+1,0,255-maxvalue);
        }
    }

    return color_remap(method,img,lookup_table);
}

PImage IPA__Point_gamma(PImage img,HV *profile)
{
    const char *method="IPA::Point::gamma";
    double origGamma=1,destGamma=1;
    unsigned char lookup_table[256];
    double i;

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);

    if (pexist(origGamma)) {
        origGamma=pget_f(origGamma);
        if (origGamma<=0) {
            croak("%s: %f is incorrect origGamma value",method,origGamma);
        }
    }
    if (pexist(destGamma)) {
        destGamma=pget_f(destGamma);
        if (destGamma<=0) {
            croak("%s: %f is incorrect destGamma value",method,destGamma);
        }
    }
    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }

    for (i=0; i<256; i++) {
        lookup_table[(int)i]=trunc(pow(i/255,1/(origGamma*destGamma))*255+0.5);
    }

    return color_remap(method,img,lookup_table);
}

PImage IPA__Point_remap(PImage img,HV *profile)
{
    const char *method="IPA::Point::remap";
    unsigned char lookup_table[256];

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);

    if ((img->type & imBPP)!=imbpp8) {
        croak("%s: unsupported image type",method);
    }
    if (pexist(lookup)) {
        SV *sv=pget_sv(lookup);
        SV **lItem;
        AV *av;
        int i,lastelem;
        if (!SvROK( sv) || (SvTYPE(SvRV(sv))!=SVt_PVAV)) {
            croak("%s: lookup is not an array",method);
        }
        av=(AV*)SvRV(sv);
        lastelem=av_len(av);
        if (lastelem>255) {
            croak("%s: lookup table contains more than 256 elements",method);
        }
        for (i=0; i<256; i++) {
            if (i>lastelem) {
                lookup_table[i]=i;
            }
            else {
                lItem=av_fetch(av,i,0);
                if (!lItem) {
                    croak("%s: empty lookup table element #%d",method,i);
                }
                if (SvIOK(*lItem) || (looks_like_number(*lItem) && strchr(SvPV(*lItem,na),'.')==NULL)) {
                    int tval=SvIV(*lItem);
                    if (tval>255) {
                        croak("%s: element #%d of lookup table too big",method,i);
                    }
                    lookup_table[i]=tval;
                }
                else {
                    croak("%s: element #%d of lookup table isn't a number but \'%s\'",method,i,SvPV(*lItem,na));
                }
            }
        }
    }

    return color_remap(method,img,lookup_table);
}

PImage IPA__Point_subtract(PImage img1,PImage img2,HV *profile)
{
    const char *method="IPA::Point::subtract";
    int ypos,xpos,xbuf;
    long minval=0,maxval=0,range;
    PImage bufimg,oimg;
    long *dbuf;
    int conversionType=CONV_SCALE;
    Bool rawOutput=false;

    if ( !img1 || !kind_of(( Handle) img1, CImage))
       croak("%s: not an image passed to 1st parameter", method);
    if ( !img2 || !kind_of(( Handle) img2, CImage))
       croak("%s: not an image passed to 2nd parameter", method);

    if (img1->type!=imByte) {
        croak("%s: unsupported format of first image",method);
    }
    if (img2->type!=imByte) {
        croak("%s: unsupported format of second image",method);
    }

    if (pexist(conversionType)) {
        conversionType=pget_i(conversionType);
        if (conversionType<CONV_FIRST || conversionType>CONV_LAST) {
            croak("%s: invalid conversion requested",method);
        }
    }
    if (pexist(rawOutput)) {
        rawOutput=pget_B(rawOutput);
    }

    bufimg=createNamedImage(img1->w,img1->h,imLong,"IPA::Point::subtract(raw)");
    if (bufimg==NULL) {
        croak("%s: can't create buffer image",method);
    }

    for (ypos=0,dbuf=(long*)bufimg->data; ypos<img1->dataSize; ypos+=img1->lineSize,(*((Byte**)&dbuf))+=bufimg->lineSize) {
        for (xpos=ypos, xbuf=0; xbuf<bufimg->w; xbuf++,xpos++) {
            dbuf[xbuf]=(long)(img1->data[xpos])-(long)(img2->data[xpos]);
            if (conversionType==CONV_SCALEABS) {
                dbuf[xbuf]=abs(dbuf[xbuf]);
            } /* endif */
            if (xpos==0) {
                minval=maxval=dbuf[xbuf];
            } /* endif */
            else {
                if (minval>dbuf[xbuf]) {
                    minval=dbuf[xbuf];
                } /* endif */
                if (maxval<dbuf[xbuf]) {
                    maxval=dbuf[xbuf];
                } /* endif */
            } /* endelse */
        } /* endfor */
    } /* endfor */

    if (rawOutput) {
        oimg=bufimg;
    }
    else {
        oimg=createNamedImage(img1->w,img1->h,imByte,method);
        range=maxval-minval;
        if (range==0) {
            range=1;
        } /* endif */

        for (ypos=0,dbuf=(long*)bufimg->data; ypos<img1->dataSize; ypos+=img1->lineSize,(*((Byte**)&dbuf))+=bufimg->lineSize) {
            for (xpos=ypos, xbuf=0; xbuf<bufimg->w; xbuf++,xpos++) {
                switch (conversionType) {
                    case CONV_TRUNC:
                        oimg->data[xpos]=(Byte)max(0,min(255L,dbuf[xbuf]));
                        break;
                    case CONV_TRUNCABS:
                        oimg->data[xpos]=(Byte)min(255,abs(dbuf[xbuf]));
                        break;
                    case CONV_SCALE:
                    case CONV_SCALEABS:
                        oimg->data[xpos]=(Byte)(((dbuf[xbuf]-minval)*255L)/range);
                        break;
                    default:
                        oimg->data[xpos]=(Byte)dbuf[xbuf];
                        break;
                } /* endswitch */
            } /* endfor */
        } /* endfor */

        destroyImage(bufimg);
    }

    return oimg;
}

PImage
IPA__Point_average( SV *list)
{
    const char *method="IPA::Point::average";
    AV *images;
    int i, imgCount;
    PImage oimg = nil, fimg = nil, img = nil;
    
    if ( ! list) {
	croak( "%s: parameter required", method);
    }
    if ( ! SvROK( list) || ( SvTYPE( SvRV( list)) != SVt_PVAV)) {
	croak( "%s: array reference required as a parameter", method);
    }
    images = ( AV *) SvRV( list);
    imgCount = av_len( images) + 1;
    if ( imgCount == 0) {
	croak( "%s: at least one image required", method);
    }

#define DO_COPY( type) \
    { \
	type *pSrc = ( type *) img->data; \
	double *pDst = ( double *) fimg->data; \
        int h = img-> h; \
        for ( ; h--; (( Byte*) pSrc) += img-> lineSize, (( Byte*) pDst) += fimg-> lineSize) {\
           register int w = img-> w;\
           register type *src = pSrc;\
           register double *dst = pDst;\
           while ( w--) *(dst++) = ( double)(*(src++));\
        } \
    }
    
#define DO_AVERAGE( type) \
    { \
	type *pSrc = ( type *) img->data; \
	double *pDst = ( double *) fimg->data; \
        int h = img-> h; \
        for ( ; h--; (( Byte*) pSrc) += img-> lineSize, (( Byte*) pDst) += fimg-> lineSize) {\
           register int w = img-> w;\
           register type *src = pSrc;\
           register double *dst = pDst;\
           while ( w--) *(dst++) += ( double)(*(src++));\
        } \
    }

#define DO_COPYBACK( type) \
    { \
	double *pSrc = ( double *) fimg->data; \
	type *pDst = ( type *) oimg->data; \
        int h = img-> h; \
        for ( ; h--; (( Byte*) pSrc) += fimg-> lineSize, (( Byte*) pDst) += oimg-> lineSize) {\
           register int w = img-> w;\
           register double *src = pSrc;\
           register type *dst = pDst;\
           while ( w--) *(dst++) = ( type) ((*(src++)) / imgCount + .5);\
        } \
    }

    for ( i = 0; i < imgCount; i++) {
	SV **mItem = av_fetch( images, i, 0);
	if ( ! mItem) {
	    croak( "%s: unexpected null element at index #%d", method, i);
	}
	if ( ! sv_isobject( *mItem) || ! sv_derived_from( *mItem, "Prima::Image")) {
	    croak( "%s: element at index #%d isn't a Prima::Image derivative", method, i);
	}
	img = ( PImage) gimme_the_mate( *mItem);
	if ( ( img->type & imGrayScale) != imGrayScale) {
	    croak( "%s: image isn't of a grayscale type at index #%d", method, i);
	}
	if ( oimg == nil) {
	    oimg = createNamedImage( img->w, img->h, img->type, method);
	    fimg = createNamedImage( img->w, img->h, imDouble, method);
	}
	if ( i == 0) {
	    switch ( img->type & imBPP) {
		case imbpp8:
		    DO_COPY( uint8_t);
		    break;
		case imbpp16:
		    DO_COPY( uint16_t);
		    break;
		case imbpp32:
		    DO_COPY( uint32_t);
		    break;
		case imbpp64:
		    DO_COPY( uint64_t);
		    break;
		default:
		    croak( "%s: method doesn't support (yet?)images of type %04x", method, img->type);
	    }
	}
	else {
	    switch ( img->type & imBPP) {
		case imbpp8:
		    DO_AVERAGE( uint8_t);
		    break;
		case imbpp16:
		    DO_AVERAGE( uint16_t);
		    break;
		case imbpp32:
		    DO_AVERAGE( uint32_t);
		    break;
		case imbpp64:
		    DO_AVERAGE( uint64_t);
		    break;
	    }
	}
    }
    switch ( img->type & imBPP) {
	case imbpp8:
	    DO_COPYBACK( uint8_t);
	    break;
	case imbpp16:
	    DO_COPYBACK( uint16_t);
	    break;
	case imbpp32:
	    DO_COPYBACK( uint32_t);
	    break;
	case imbpp64:
	    DO_COPYBACK( uint64_t);
	    break;
    }

    Object_destroy( ( Handle) fimg);

#undef DO_COPY
#undef DO_AVERAGE
#undef DO_COPYBACK

    return oimg;
}

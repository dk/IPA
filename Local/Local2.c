/* $Id$ */

#include <float.h>
#include "IPAsupp.h"
#include "Local.h"
#include "LocalSupp.h"

PImage deriche( const char *method, PImage in, float alpha)
{
#define deriche_INDEX(x,y) (y)*n+(x)
#define deriche_New(size,type) malloc( sizeof( type) * (size))
#define deriche_NewF(size) New((size),float)
#define deriche_NewFz(size) allocnz( float, (size))
#define deriche_READLINE(target,line) for ( unusedIndex = 0; unusedIndex < n; unusedIndex++) \
                                 target[ unusedIndex] = in-> data[(line)*in-> lineSize+unusedIndex];
#define deriche_WRITELINE(source,line) for ( unusedIndex = 0; unusedIndex < n; unusedIndex++) \
                                  outf[deriche_INDEX(unusedIndex,(line))] = source[unusedIndex];
   float a = (float)(-(1.0-exp(-alpha))*(1.0-exp(-alpha)));
   float b1 = (float)(-2.0 * exp(-alpha));
   float b2 = (float)exp(-2.0*alpha);
   float a0 = (float)(-a/(1.0 - alpha * b1 - b2));
   float a1 = (float)(a0*(alpha-1.0)*exp(-alpha));
   float a2 = a1 - a0*b1;
   float a3 = -a0*b2;
   float *z1, *z2, *z3, *ze, *za, *im1, *im2, *outf;
   float z;
   int n = in-> w;
   int x, y;
   int unusedIndex;
   PImage out;

   if ( n != in-> h)
       croak("%s: image width not equal to image height",method);

   z1 = deriche_NewFz(n); z2 = deriche_NewFz(n); z3 = deriche_NewFz(n);
   ze = deriche_NewFz(n); za = deriche_NewFz(n);
   im1 = deriche_NewFz(n*n); im2 = deriche_NewFz(n*n); outf = deriche_NewFz(n*n);

/* COMPUTATION of H(x,y)*/
/* run I: bottom-up*/
   for ( y = 2; y < n; y++)
   {
      deriche_READLINE(ze,y-1);
      for ( x = 0; x < n; x++)
         im1[deriche_INDEX(x,y)] = ze[x] - b1*im1[deriche_INDEX(x,y-1)] - b2*im1[deriche_INDEX(x,y-2)];
   }

/* run II: top-down*/
   for ( y = n - 3; y >= 0; y--)
   {
      deriche_READLINE(ze,y+1);
      for ( x = 0; x < n; x++)
      {
         im2[deriche_INDEX(x,y)] = ze[x] - b1*im2[deriche_INDEX(x,y+1)] - b2*im2[deriche_INDEX(x,y+2)];
         im1[deriche_INDEX(x,y)] = a*(im1[deriche_INDEX(x,y)] - im2[deriche_INDEX(x,y)]);
      }
   }

/* runs III - IV:  left to right or right to left*/
   for ( y = 0; y < n; y++)
   {
      for ( x = 0; x < n; x++)   z1[x] = im1[deriche_INDEX(x,y)];
      for ( x = 2; x < n; x++)
         z2[x] = a0*z1[x] + a1*z1[x-1] - b1*z2[x-1] - b2*z2[x-2];
      for ( x = n - 3; x >= 0; x--)
         z3[x] = a2*z1[x+1] + a3*z1[x+2] - b1*z3[x+1] - b2*z3[x+2];
      for ( x = 0; x < n; x++)   za[x] = z2[x] + z3[x];
      deriche_WRITELINE(za,y);
   }

/* COMPUTATION of V(x,y)*/
/* runs V - VI: left to right or right to left*/
   for ( y = 0; y < n; y++)
   {
      deriche_READLINE(ze,y);
      for ( x = 2; x < n; x++)
         z2[x] = ze[x-1] - b1*z2[x-1] - b2*z2[x-2];
      for ( x = n - 3; x >= 0; x--)
         z3[x] = ze[x+1] - b1*z3[x+1] - b2*z3[x+2];
      for ( x = 0; x < n; x++)  im1[deriche_INDEX(x,y)] = a*(z2[x]-z3[x]);
   }

/* run VII: bottom-up*/
   for ( y = 2; y < n; y++)
      for ( x = 0; x < n; x++)
         im2[deriche_INDEX(x,y)] = a0*im1[deriche_INDEX(x,y)] + a1*im1[deriche_INDEX(x,y)] -
                           b1*im2[deriche_INDEX(x,y-1)] - b2*im2[deriche_INDEX(x,y-2)];

/* run VIII: top-down*/
   for ( y = n - 3; y >= 0; y--)
   {
      for ( x = 0; x < n; x++)   z1[x] = im2[deriche_INDEX(x,y)];
      for ( x = 0; x < n; x++)
         im2[deriche_INDEX(x,y)] = a2*im1[deriche_INDEX(x,y+1)] + a3*im1[deriche_INDEX(x,y+2)] -
                           b1*im2[deriche_INDEX(x,y+1)] - b2*im2[deriche_INDEX(x,y+2)];
      for ( x = 0; x < n; x++)
      {
         z = im2[deriche_INDEX(x,y)]+z1[x];
         za[x] = (Byte)(sqrt(z*z+outf[deriche_INDEX(x,y)]*outf[deriche_INDEX(x,y)]));
      }
      deriche_WRITELINE(za,y);
   }

   out = createImage( in-> w, in-> h, in->type);
   for ( y = 0; y < n; y++)
      for ( x = 0; x < n; x++) {
         register int z = (int)(outf[deriche_INDEX(x,y)] + 0.5);
         if ( z < 0) z = 0;
         if ( z > 255) z = 255;
         /* out-> data[y*out-> lineSize + x] = outf[deriche_INDEX(x,y)] < 1 ? 0 : 255;*/
         out-> data[y*out-> lineSize + x] = z;
      }

   free( z1); free( z2); free( z3);
   free( ze); free( za);
   free( im1); free( im2); free( outf);

   return out;
}

PImage IPA__Local_GEF(PImage img,HV *profile)
{
    dPROFILE;
    const char *method="IPA::Local::gef";
    PImage oimg;
    double a0=1.3,s=0.7;
    int xpos,ypos,shift;
    PImage dx,dy,dtmp;
    int v,v1,w1,w2;

    if ( !img || !kind_of(( Handle) img, CImage))
      croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }

    if (pexist(a0)) {
        a0=pget_f(a0);
    }
    if (pexist(s)) {
        s=pget_f(s);
    }

    if (img==nil) {
        return nil;
    } /* endif */

    w1=img->w-1;
    w2=img->w-2;

    dx=create_compatible_image(img,false);
    dy=createImage(img->w,img->h,imByte);
    oimg=createImage(img->w,img->h,imByte);
    dtmp=createImage(img->w,img->h,imByte);
    if ((dx==nil) || (dy==nil) || (oimg==nil) || (dtmp==nil)) {
        destroyImage(dx);
        destroyImage(dy);
        destroyImage(oimg);
        destroyImage(dtmp);
        croak("%s: image creation failed",method);
    } /* endif */

    /* calculate derivatives of 1st order */

    /* derivatives on on X */

    /* scanning bottom-to-top, putting results in dx */
    for (xpos=0; xpos<img->w; xpos++) { /* columns from left to right */
        dx->data[xpos]=img->data[xpos];
        for (ypos=xpos+img->lineSize; ypos<img->dataSize; ypos+=img->lineSize) { /* rows bottom-to-top */
            v=dx->data[ypos-img->lineSize];
            v1=img->data[ypos];
            dx->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning top-to-bottom */
    shift=dx->dataSize-dx->lineSize-dx->lineSize;
    for (xpos=shift; xpos<(shift+dx->w); xpos++) { /* columns for left to right */
        for (ypos=xpos; ypos>0; ypos-=dx->lineSize) { /* rows top-top-bottom */
            v=dx->data[ypos+dx->lineSize];
            v1=dx->data[ypos];
            dx->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scannign left-to-right, storing dx data in dtmp */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dtmp->data[ypos]=dx->data[ypos];
        for (xpos=ypos+1; xpos<(ypos+dx->w); xpos++) {
            v=dtmp->data[xpos-1];
            v1=dx->data[xpos];
            dtmp->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning right-to-left, storing dx data in dy */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dy->data[ypos+w1]=dx->data[ypos+w1];
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dx->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* trying to calc dX/X by data from dx, dy, dtmp */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        for (xpos=ypos; xpos<(ypos+dx->w); xpos++) {
            v=dy->data[xpos];
            v1=dtmp->data[xpos];
            dx->data[xpos]=abs(v-v1);
        } /* endfor */
    } /* endfor */

    /* now dY's */

    /* scanning left-to-right, from img to dy */
    for (ypos=0; ypos<img->dataSize; ypos+=img->lineSize) {
        dy->data[ypos]=img->data[ypos];
        for (xpos=(ypos+1); xpos<(ypos+img->w); xpos++) {
            v=dy->data[xpos-1];
            v1=img->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning right-to-left, from dy to dy */
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dy->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* bottom-to-top, from dy to dtmp */
    for (xpos=0; xpos<dy->w; xpos++) {
        dtmp->data[xpos]=dy->data[xpos];
        for (ypos=xpos+dy->lineSize; ypos<dy->dataSize; ypos+=dy->lineSize) {
            v=dtmp->data[ypos-dy->lineSize];
            v1=dy->data[ypos];
            dtmp->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* top-to-bottom, from dy to oimg */
    shift=dy->dataSize-(dy->lineSize<<1);
    for (xpos=shift; xpos<(shift+dy->w); xpos++) {
        oimg->data[xpos]=dy->data[xpos];
        for (ypos=xpos; ypos>0; ypos-=dy->lineSize) {
            v=oimg->data[ypos+dy->lineSize];
            v1=dy->data[ypos];
            oimg->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* trying to get 2nd derivative by Y from data in ddy, dy, dtmp. Results are to dy (1st) and ddy (2nd) */
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=ypos; xpos<(ypos+dy->w); xpos++) {
            v=dtmp->data[xpos];
            v1=oimg->data[xpos];
            dy->data[xpos]=abs(v-v1);
        } /* endfor */
    } /* endfor */

    /* based on dx,dy,ddy trying to get image contours */
    for (ypos=img->lineSize; ypos<(img->dataSize-img->lineSize); ypos+=img->lineSize) {
        for (xpos=ypos+1; xpos<(ypos+img->w-1); xpos++) {
            if (dx->data[xpos]>dy->data[xpos]) {
                if ((dx->data[xpos]>dx->data[xpos-1]) && (dx->data[xpos]>=dx->data[xpos+1])) {
                    oimg->data[xpos]=dx->data[xpos];
                } /* endif */
                else {
                    oimg->data[xpos]=0;
                } /* endelse */
            } /* endif */
            else {
                if ((dy->data[xpos]>dy->data[xpos-dy->lineSize]) && (dy->data[xpos]>=dy->data[xpos+dy->lineSize])) {
                    oimg->data[xpos]=dy->data[xpos];
                } /* endif */
                else {
                    oimg->data[xpos]=0;
                } /* endelse */
            } /* endelse */
        } /* endfor */
    } /* endfor */

    destroyImage(dx);
    destroyImage(dy);
    destroyImage(dtmp);

    return oimg;
}

PImage IPA__Local_SDEF(PImage img,HV *profile)
{
    dPROFILE;
    const char *method="IPA::Local::sdef";
    PImage oimg;
    int xpos,ypos,shift;
    PImage dx,dy,ddx,ddy,dtmp;
    int v,v1,v2,w1,w2;
    double a0=1.3,s=0.7;

    if ( !img || !kind_of(( Handle) img, CImage))
       croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }

    if (pexist(a0)) {
        a0=pget_f(a0);
    }
    if (pexist(s)) {
        s=pget_f(s);
    }

    w1=img->w-1;
    w2=img->w-2;

   dx=createImage(img->w,img->h,imByte);
    dy=createImage(img->w,img->h,imByte);
    ddx=createImage(img->w,img->h,imByte);
    ddy=createImage(img->w,img->h,imByte);
    oimg=createImage(img->w,img->h,imByte);
    dtmp=createImage(img->w,img->h,imByte);
    if ((dx==nil) || (dy==nil) || (ddx==nil) || (ddy==nil) || (oimg==nil) || (dtmp==nil)) {
        destroyImage(dx);
        destroyImage(dy);
        destroyImage(ddx);
        destroyImage(ddy);
        destroyImage(oimg);
        destroyImage(dtmp);
        croak("%s: image creation failed",method);
    } /* endif */

    /* calculate derivatives of 1st and 2nd order */

    /* derivatives on on X */

    /* scanning bottom-to-top, putting results from img to dx */
    for (xpos=0; xpos<img->w; xpos++) { /* columns from left to right */
        dx->data[xpos]=img->data[xpos];
        for (ypos=xpos+img->lineSize; ypos<img->dataSize; ypos+=img->lineSize) {  /* rows bottom-to-top */
            v=dx->data[ypos-img->lineSize];
            v1=img->data[ypos];
            dx->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning top-to-bottom, dx -> dx */
    shift=dx->dataSize-dx->lineSize-dx->lineSize;
    for (xpos=shift; xpos<(shift+dx->w); xpos++) {
        for (ypos=xpos; ypos>0; ypos-=dx->lineSize) {  /* columns for left to right */
            v=dx->data[ypos+dx->lineSize];
            v1=dx->data[ypos];
            dx->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scannign left-to-right, storing dx data in ddx */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        ddx->data[ypos]=dx->data[ypos];
        for (xpos=ypos+1; xpos<(ypos+dx->w); xpos++) {
            v=ddx->data[xpos-1];
            v1=dx->data[xpos];
            ddx->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning right-to-left, storing dx data in dy */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dy->data[ypos+w1]=dx->data[ypos+w1];
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dx->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* trying to get 1st and 2nd derivative by X from data in dx,dy,ddx */
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        for (xpos=ypos; xpos<(ypos+dx->w); xpos++) {
            v=dy->data[xpos];
            v1=ddx->data[xpos];
            v2=dx->data[xpos];
            if ((v+v1-v2-v2)>=0) {
                if ((v2=v-v1)>=0) {
                    ddx->data[xpos]=3;
                    dx->data[xpos]=v2;
                } /* endif */
                else {
                    ddx->data[xpos]=2;
                    dx->data[xpos]=-v2;
                } /* endelse */
            } /* endif */
            else {
                if ((v2=v-v1)>=0) {
                    ddx->data[xpos]=1;
                    dx->data[xpos]=v2;
                } /* endif */
                else {
                    ddx->data[xpos]=0;
                    dx->data[xpos]=-v2;
                } /* endelse */
            } /* endelse */
        } /* endfor */
    } /* endfor */

    /* now dY's */

    /* scanning left-to-right, from img to dy */
    for (ypos=0; ypos<img->dataSize; ypos+=img->lineSize) {
        dy->data[ypos]=img->data[ypos];
        for (xpos=(ypos+1); xpos<(ypos+img->w); xpos++) {
            v=dy->data[xpos-1];
            v1=img->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* scanning right-to-left, from dy to dy */
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dy->data[xpos];
            dy->data[xpos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* bottom-to-top, from dy to ddy */
    for (xpos=0; xpos<dy->w; xpos++) {
        ddy->data[xpos]=dy->data[xpos];
        for (ypos=xpos+dy->lineSize; ypos<dy->dataSize; ypos+=dy->lineSize) {
            v=ddy->data[ypos-dy->lineSize];
            v1=dy->data[ypos];
            ddy->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* top-to-bottom, from dy to dtmp */
    shift=dy->dataSize-(dy->lineSize<<1);
    for (xpos=shift; xpos<(shift+dy->w); xpos++) {
        dtmp->data[xpos]=dy->data[xpos];
        for (ypos=xpos; ypos>0; ypos-=dy->lineSize) {
            v=dtmp->data[ypos+dy->lineSize];
            v1=dy->data[ypos];
            dtmp->data[ypos]=(Byte)(v+a0*(v1-v)+0.5);
        } /* endfor */
    } /* endfor */

    /* trying to get 2nd derivative by Y from data in ddy, dy, dtmp. Results are to dy (1st) and ddy (2nd) */
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=ypos; xpos<(ypos+dy->w); xpos++) {
            v=dtmp->data[xpos];
            v1=ddy->data[xpos];
            v2=dy->data[xpos];
            if ((v+v1-v2-v2)>=0) {
                if ((v2=v-v1)>=0) {
                    ddy->data[xpos]=3;
                    dy->data[xpos]=v2;
                } /* endif */
                else {
                    ddy->data[xpos]=2;
                    dy->data[xpos]=-v2;
                } /* endelse */
            } /* endif */
            else {
                if ((v2=v-v1)>=0) {
                    ddy->data[xpos]=1;
                    dy->data[xpos]=v2;
                } /* endif */
                else {
                    ddy->data[xpos]=0;
                    dy->data[xpos]=-v2;
                } /* endelse */
            } /* endelse */
        } /* endfor */
    } /* endfor */

    /* based on dx,dy,ddy trying to get image contours */
    for (ypos=img->lineSize; ypos<img->dataSize; ypos+=img->lineSize) {
        for (xpos=ypos+1; xpos<(ypos+img->w); xpos++) {
            if (dx->data[xpos]>((unsigned)(s*dy->data[xpos]))) {
                if (((ddx->data[xpos]==2) && (ddx->data[xpos-1]<2)) || ((ddx->data[xpos-1]>1) && (ddx->data[xpos]==1))) {
                    oimg->data[xpos]=dx->data[xpos];
                } /* endif */
                else {
                    oimg->data[xpos]=0;
                } /* endelse */
            } /* endif */
            else {
                if (dy->data[xpos]>((unsigned)(s*dx->data[xpos]))) {
                    if (((ddy->data[xpos]==2) && (ddy->data[xpos-img->lineSize]<2)) || ((ddy->data[xpos-img->lineSize]>1) && (ddy->data[xpos]==1))) {
                        oimg->data[xpos]=dy->data[xpos];
                    } /* endif */
                    else {
                        oimg->data[xpos]=0;
                    } /* endelse */
                } /* endif */
                else {
                    if (((ddx->data[xpos]==2) && (ddx->data[xpos-1]<2)) || ((ddx->data[xpos-1]>1) && (ddx->data[xpos]==1))) {
                        oimg->data[xpos]=dx->data[xpos];
                    } /* endif */
                    else {
                        oimg->data[xpos]=0;
                    } /* endelse */
                    if ((oimg->data[xpos]==0) || (dy->data[xpos]>dx->data[xpos])) {
                        if (((ddy->data[xpos]==2) && (ddy->data[xpos-img->lineSize]<2)) || ((ddy->data[xpos-img->lineSize]>1) && (ddy->data[xpos]==1))) {
                            oimg->data[xpos]=dy->data[xpos];
                        } /* endif */
                    } /* endif */
                } /* endelse */
            } /* endelse */
        } /* endfor */
    } /* endfor */

    destroyImage(dx);
    destroyImage(dy);
    destroyImage(ddx);
    destroyImage(ddy);
    destroyImage(dtmp);

    return oimg;
}

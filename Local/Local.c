/* $Id$ */

#include <stdarg.h>
#include "IPAsupp.h"
#include "Local.h"
#include "Local.inc"
#include "LocalSupp.h"

static SV **temporary_prf_Sv;

/* ����� ��� �������� Sobel */
#define SOBEL_COLUMN            0x0001
#define SOBEL_ROW               0x0002
#define SOBEL_NWSE              0x0004
#define SOBEL_NESW              0x0008

typedef enum {
    sobelColumn=0,
    sobelRow=1,
    sobelNWSE=2,
    sobelNESW=3
} OPERATOR_TYPE;

PImage_vmt CImage;

XS( boot_IPA__Local)
{
    dXSARGS;

    (void)items;

    XS_VERSION_BOOTCHECK;

    register_IPA__Local_Package();

    CImage = (PImage_vmt)gimme_the_vmt( "Prima::Image");

    ST(0) = &sv_yes;
    XSRETURN(1);
}

/*******************************************************************
 * Function    : crispeningByte
 * Parameters  : PImage img
 * Description : Applies crispening method to 8bpp grayscale image
 * Returns     : Newly created image with applied crispening.
 *******************************************************************/
PImage crispeningByte(PImage img)
{
    PImage oimg=createNamedImage(img->w,img->h,imByte,"crispening result");
    int x,y;
    unsigned char *p,*pu,*pd,*dst;
    if (oimg) {
        memcpy(oimg->data,img->data,img->lineSize);
        for (y=1,
             pu=img->data,
             p=(img->data+img->lineSize),
             pd=(img->data+img->lineSize*2),
             dst=(oimg->data+oimg->lineSize);
             y<(img->h-1);
             y++,
             pu+=img->lineSize,
             p+=img->lineSize,
             pd+=img->lineSize,
             dst+=oimg->lineSize) {
            dst[0]=p[0];
            dst[oimg->w-1]=p[img->w-1];
            for (x=1; x<(img->w-1); x++) {
                int v=9*(int)p[x]-p[x-1]-p[x+1]-pu[x-1]-pu[x]-pu[x+1]-pd[x-1]-pd[x]-pd[x+1];
                dst[x]=v<0 ? 0 : v>255 ? 255 : v;
            }
        }
        memcpy(dst,p,img->lineSize);
    }
    return oimg;
}

PImage IPA__Local_crispening(PImage img)
{
    PImage oimg;
    const char *method="IPA::Local::crispening";

    if ( !img || !kind_of(( Handle) img, CImage))
        croak("%s: not an image passed", method);

    switch (img->type) {
        case imByte:
            oimg=crispeningByte(img);
            break;
        default:
            croak("%s: unsupported image type: %08x",method,img->type);
    }

    if (!oimg) {
        croak("%s: can't create output image",method);
    }

    return oimg;
}

/****************************************************************************/
/* ������� Sobel-������ � ��������������� ����������� ������ ������         */
/* ��������� ������� (row, column, nwse � nesw).                            */
/* ��������� �������:                                                       */
/* srcimg      - �������� 8bpp image;                                       */
/* jobMask     - ���������� SOBEL_* ������, ��������, ����� ������ �������� */
/*               ������� ���������� ������������ ��� ������;                */
/* combineType - ���� �� COMBINE_* ��������, �������� ������ �������������� */
/*               ����������� ������ ������ ������ �������������� �������;   */
/* conv        - ������ ������ ��������� ������ ���������� � 8bpp image;    */
/* divisor     - ��������;                                                  */
/* ����������  - 8bpp image, ���� ��� ���������; ����� - nil                */
/****************************************************************************/

short sobel_combine(short *pixval,unsigned short combinetype)
{
    short comb=0;

    switch (combinetype) {
        case COMBINE_MAXABS:
            comb=max(abs(pixval[sobelColumn]),
                  max(abs(pixval[sobelRow]),
                  max(abs(pixval[sobelNWSE]),
                      abs(pixval[sobelNESW]))));
            break;
        case COMBINE_SIGNEDMAXABS:
            {
                OPERATOR_TYPE pixindex=sobelColumn;
                if (abs(pixval[sobelColumn])>abs(pixval[sobelRow])) {
                    pixindex=sobelColumn;
                } /* endif */
                if (abs(pixval[pixindex])<abs(pixval[sobelNWSE])) {
                    pixindex=sobelNWSE;
                } /* endif */
                if (abs(pixval[pixindex])<abs(pixval[sobelNESW])) {
                    pixindex=sobelNESW;
                } /* endif */
                comb=pixval[pixindex];
            }
            break;
        case COMBINE_SUMABS:
            comb=abs(pixval[sobelColumn])+
                  abs(pixval[sobelRow])+
                  abs(pixval[sobelNWSE])+
                  abs(pixval[sobelNESW]);
            break;
        case COMBINE_SUM:
            comb=pixval[sobelColumn]+
                  pixval[sobelRow]+
                  pixval[sobelNESW]+
                  pixval[sobelNWSE];
            break;
        case COMBINE_SQRT:
            {
                int sqr;
                sqr=pixval[sobelColumn]*pixval[sobelColumn]+
                    pixval[sobelRow]*pixval[sobelRow]+
                    pixval[sobelNESW]*pixval[sobelNESW]+
                    pixval[sobelNWSE]*pixval[sobelNWSE];
                comb=sqrt(sqr);
            }
            break;
    } /* endswitch */

    return comb;
}

PImage fast_sobel( PImage srcimg,
                   unsigned short jobMask,
                   unsigned short combinetype,
                   unsigned short conv,
                   unsigned short divisor
                 )
{
    PImage dstimg=nil;
    short pixval[4]={0,0,0,0},pixval1[4]={0,0,0,0};
    int ypos,xpos,y;
    unsigned char *p1,*p2,*p3; /* ��������� �� ������ � image:
                                p1 - �� ���� ���� �������
                                p2 - �������
                                p3 - �� ���� ���� ������� */
    unsigned char *p,*pu,*pd,*pl,*pr,*pur,*pul,*pdr,*pdl;
    short *imgbuf,*imgp,*imgp1;
    short minval=0,maxval=0,range=0;

    if (jobMask==0) {
        return nil;
    } /* endif */

    imgbuf=(short*)malloc(srcimg->w*srcimg->h*sizeof(short));
    if (imgbuf==nil) {
        return nil;
    } /* endif */
    memset(imgbuf,0,srcimg->w*srcimg->h*sizeof(short));

    p1=srcimg->data+(srcimg->lineSize<<1); /* <<1 - ����� �� ������� �� 2 */ 
    p2=srcimg->data+srcimg->lineSize;
    p3=srcimg->data;
    imgp=imgbuf+srcimg->w;
    for (ypos=srcimg->lineSize,y=1; ypos<(srcimg->dataSize-srcimg->lineSize); ypos+=srcimg->lineSize,y++) {
        for (xpos=1; xpos<(srcimg->w-1); xpos++) {
            imgp++;
            pu=p1+xpos,
            pd=p3+xpos,       /******************/
            pl=p2+xpos-1,     /* p1: pul pu pur */
            pr=p2+xpos+1,     /* p2: pl  X   pr */
            pul=p1+xpos-1,    /* p3: pdl pd pdr */
            pur=p1+xpos+1,    /******************/
            pdl=p3+xpos-1,
            pdr=p3+xpos+1;

            if (jobMask & SOBEL_COLUMN) {
                pixval[sobelColumn]=
                     *pul+*pu*2L+*pur-
                    (*pdl+*pd*2L+*pdr);
            } /* endif */
            if (jobMask & SOBEL_ROW) {
                pixval[sobelRow]=
                     *pul+*pl*2L+*pdl-
                    (*pur+*pr*2L+*pdr);
            } /* endif */
            if (jobMask & SOBEL_NWSE) {
                pixval[sobelNWSE]=
                     *pr+*pdr*2L+*pd-
                    (*pl+*pul*2L+*pu);
            } /* endif */
            if (jobMask & SOBEL_NESW) {
                pixval[sobelNESW]=
                     *pl+*pdl*2L+*pd-
                    (*pr+*pur*2L+*pu);
            } /* endif */

            *imgp=sobel_combine(pixval,combinetype)/divisor;

            if (conv==CONV_SCALEABS) {
                *imgp=abs(*imgp);
            } /* endif */
            if (*imgp<minval) {
                minval=*imgp;
            } /* endif */
            if (*imgp>maxval) {
                maxval=*imgp;
            } /* endif */
        } /* endfor */
        p1+=srcimg->lineSize;
        p2+=srcimg->lineSize;
        p3+=srcimg->lineSize;
        imgp++;
        imgp++;
    } /* endfor */

    /* ������������ �������������� �������. */

    pu=srcimg->data+1;                                /* ������� ������� */
    p1=srcimg->data+srcimg->lineSize+1;
    pd=srcimg->data+srcimg->lineSize*(srcimg->h-1)+1; /* ������ ������� */
    p2=srcimg->data+srcimg->lineSize*(srcimg->h-2)+1;
    imgp=imgbuf+1;                                    /* ������� ������� ���������� */
    imgp1=imgbuf+srcimg->w*(srcimg->h-1)+1;           /* ������ ������� ���������� */
    for (xpos=1; xpos<(srcimg->w-1); xpos++) {
        if (jobMask & SOBEL_COLUMN) {
            pixval[sobelColumn]=
                 *(pu-1)+*pu*2L+*(pu+1)-
                (*(p1-1)+*p1*2L+*(p1+1));

            pixval1[sobelColumn]=
                 *(p2-1)+*p2*2L+*(p2+1)-
                (*(pd-1)+*pd*2L+*(pd+1));
        } /* endif */
        if (jobMask & SOBEL_ROW) {
            pixval[sobelRow]=
                 *(pu-1)*2L+*(p1-1)-
                (*(pu+1)*2L+*(p1+1));

            pixval1[sobelRow]=
                 *(p2-1)*2L+*(pd-1)-
                (*(p2+1)*2L+*(pd+1));
        } /* endif */
        if (jobMask & SOBEL_NWSE) {
            pixval[sobelNWSE]=
                 (*(p1+1)-*(pu-1))*2L;

            pixval1[sobelNWSE]=
                 (*(pd+1)-*(p2-1))*2L;
        } /* endif */
        if (jobMask & SOBEL_NESW) {
            pixval[sobelNESW]=
                 (*(p1-1)-*(pu+1))*2L;

            pixval1[sobelNESW]=
                 (*(pd-1)-*(p2+1))*2L;
        } /* endif */

        *imgp=sobel_combine(pixval,combinetype)/divisor;
        if (conv==CONV_SCALEABS) {
            *imgp=abs(*imgp);
        } /* endif */
        if (*imgp<minval) {
            minval=*imgp;
        } /* endif */
        if (*imgp>maxval) {
            maxval=*imgp;
        } /* endif */

        *imgp1=sobel_combine(pixval1,combinetype)/divisor;
        if (conv==CONV_SCALEABS) {
            *imgp1=abs(*imgp1);
        } /* endif */
        if (*imgp1<minval) {
            minval=*imgp1;
        } /* endif */
        if (*imgp1>maxval) {
            maxval=*imgp1;
        } /* endif */

        pu++;
        p1++;
        pd++;
        p2++;
        imgp++;
        imgp1++;
    } /* endfor */

    /* ������������ ������������ ������� */

    pl=srcimg->data+srcimg->lineSize;       /* ����� �������. */
    pul=pl-srcimg->lineSize;
    pdl=pl+srcimg->lineSize;
    pr=pl+srcimg->w-1;                      /* ������ ������� */
    pur=pr-srcimg->lineSize;
    pdr=pr+srcimg->lineSize;
    imgp=imgbuf+srcimg->w;
    imgp1=imgp+srcimg->w-1;
    for (ypos=1; ypos<(srcimg->h-1); ypos++) {
        if (jobMask & SOBEL_COLUMN) {
            pixval[sobelColumn]=
                 *pul*2L+*(pul+1)-
                (*pdl*2L+*(pdl+1));

            pixval1[sobelColumn]=
                 *pur*2L+*(pur-1)-
                (*pdr*2L+*(pdr-1));
        } /* endif */
        if (jobMask & SOBEL_ROW) {
            pixval[sobelRow]=
                 *pul+*pl*2L+*pdl-
                (*(pul+1)+*(pl+1)*2L+*(pdl+1));

            pixval1[sobelRow]=
                 *(pur-1)+*(pr-1)*2L+*(pdr-1)-
                 (*pur+*pr*2L+*pdr);
        } /* endif */
        if (jobMask & SOBEL_NWSE) {
            pixval[sobelNWSE]=
                 *(pdl+1)*2L-*pl*2L;

            pixval1[sobelNWSE]=
                 *pr*2L-*(pur-1)*2L;
        } /* endif */
        if (jobMask & SOBEL_NESW) {
            pixval[sobelNESW]=
                 *pl*2L-*(pul+1)*2L;

            pixval1[sobelNESW]=
                 *(pdr-1)*2L-*pr*2L;
        } /* endif */

        *imgp=sobel_combine(pixval,combinetype)/divisor;
        if (conv==CONV_SCALEABS) {
            *imgp=abs(*imgp);
        } /* endif */
        if (*imgp<minval) {
            minval=*imgp;
        } /* endif */
        if (*imgp>maxval) {
            maxval=*imgp;
        } /* endif */

        *imgp1=sobel_combine(pixval1,combinetype)/divisor;
        if (conv==CONV_SCALEABS) {
            *imgp1=abs(*imgp1);
        } /* endif */
        if (*imgp1<minval) {
            minval=*imgp1;
        } /* endif */
        if (*imgp1>maxval) {
            maxval=*imgp1;
        } /* endif */

        pl+=srcimg->lineSize;
        pul+=srcimg->lineSize;
        pdl+=srcimg->lineSize;
        pr+=srcimg->lineSize;
        pur+=srcimg->lineSize;
        pdr+=srcimg->lineSize;
        imgp+=srcimg->w;
        imgp1+=srcimg->w;
    } /* endfor */

    /* ���������� ������� ����������� ������ � �������������� image */
    dstimg=createNamedImage(srcimg->w,srcimg->h,imByte,"sobel result");
    if (dstimg==nil) {
        return nil;
    } /* endif */

    imgp=imgbuf;
    p=dstimg->data;
    if (conv==CONV_SCALE || conv==CONV_SCALEABS) {
        range=maxval-minval;
        if (range==0) {
            range=1;
        } /* endif */
    } /* endif */
    for (ypos=0,y=0; ypos<dstimg->dataSize; ypos+=dstimg->lineSize,y++) {
        p1=p;
        for (xpos=0; xpos<dstimg->w; xpos++) {
            switch (conv) {
                case CONV_TRUNC:
                    *p1=max(min(*imgp,255),0);
                    break;
                case CONV_TRUNCABS:
                    *p1=min(abs(*imgp),255);
                    break;
                case CONV_SCALEABS:
                case CONV_SCALE:
                    *p1=(((*imgp-minval)*255L)/range);
                    break;
                default:
                    break;
            } /* endswitch */
            p1++;
            imgp++;
        } /* endfor */
        p+=dstimg->lineSize;
    } /* endfor */

    free(imgbuf);

    return dstimg;
}

PImage IPA__Local_sobel(PImage img,HV *profile)
{
    const char *method="IPA::Local::sobel";
    PImage oimg;
    unsigned short jobMask=SOBEL_NWSE|SOBEL_NESW;
    unsigned short combineType=COMBINE_MAXABS;
    unsigned short conversionType=CONV_SCALEABS;
    unsigned short divisor=1;

    if ( !img || !kind_of(( Handle) img, CImage)) 
       croak("%s: not an image passed", method);

    if (pexist(jobMask)) {
        jobMask=pget_i(jobMask);
        if ((jobMask & ~(SOBEL_NESW|SOBEL_NWSE|SOBEL_COLUMN|SOBEL_ROW))!=0) {
            croak("%s: illegal job mask defined",method);
        }
    }
    if (pexist(combineType)) {
        combineType=pget_i(combineType);
        if (combineType<1 || combineType>5) {
            croak("%s: illegal combination type value %d",method,combineType);
        }
    }
    if (pexist(conversionType)) {
        conversionType=pget_i(conversionType);
        if (conversionType<1 || conversionType>4) {
            croak("%s: illegal conversion type value %d",method,conversionType);
        }
    }
    if (pexist(divisor)) {
        divisor=pget_i(divisor);
        if (divisor==0) {
            croak("%s: divisor must not be equal to zero",method);
        }
    }

    switch (img->type) {
        case imByte:
            oimg=fast_sobel(img,jobMask,combineType,conversionType,divisor);
            break;
        default:
            croak("%s: unsupported image type",method);
    }

    if (!oimg) {
        croak("%s: can't create output image",method);
    }

    return oimg;
}

PImage IPA__Local_filter3x3(PImage img,HV *profile)
{
    const char *method="IPA::Local::filter3x3";
    PImage oimg,bufimg;
    unsigned short conversionType=CONV_SCALEABS;
    unsigned short edgecolor=0;
    Bool rawOutput=false,expandEdges=false;
    double matrix[9];
    double divisor=1;
    long minval=0,maxval=0,range;
    int x,y;
    long *bufp;
    Byte *p,*pu,*pd;

    if ( !img || !kind_of(( Handle) img, CImage)) 
       croak("%s: not an image passed", method);

    if (pexist(conversionType)) {
        conversionType=pget_i(conversionType);
        if (conversionType<1 || conversionType>4) {
            croak("%s: conversion type value %d is not valid",method,conversionType);
        }
    }
    if (pexist(rawOutput)) {
        rawOutput=pget_B(rawOutput);
    }
    if (pexist(expandEdges)) {
        expandEdges=pget_B(expandEdges);
    }
    if (pexist(edgecolor)) {
        edgecolor=pget_i(edgecolor);
        if (edgecolor>255) {
            croak("%s: edge color value %d is not valid",method,edgecolor);
        }
    }
    if (pexist(divisor)) {
        divisor=pget_f(divisor);
        if (divisor==0) {
            croak("%s: divisor cannot be equal to 0",method);
        }
    }
    if (pexist(matrix)) {
        SV *sv=pget_sv(matrix);
        SV **mItem;
        AV *av;
        int i;
        if (!SvROK(sv) || (SvTYPE(SvRV(sv))!=SVt_PVAV)) {
            croak("%s: matrix is not an array",method);
        }
        av=(AV*)SvRV(sv);
        if (av_len(av)!=8) {
            croak("%s: incorrect length of matrix array",method);
        }
        for (i=0; i<9; i++) {
            mItem=av_fetch(av,i,0);
            if (!mItem) {
                croak("%s: empty matrix element #%d",method,i);
            }
            if (SvNOK(*mItem) || looks_like_number(*mItem)) {
                matrix[i]=SvNV(*mItem);
            }
            else {
                croak("%s: matrix's element #%d are not of type double or int",method,i);
            }
        }
    }
    else {
        croak("%s: matrix required",method);
    }

    switch (img->type) {
        case imByte:
            {
                static int bufNumber=0;
                char bufImgName[256];
                sprintf(bufImgName,"filter3x3_buf#%d",++bufNumber);
                bufimg=createNamedImage(img->w,img->h,imLong,bufImgName);
                if (!bufimg) {
                    croak("%s: can't create intermediate buffer",method);
                }
                for (y=1,
                      pu=(img->data+img->lineSize*2),
                      p=(img->data+img->lineSize),
                      pd=img->data,
                      bufp=(long*)(bufimg->data+bufimg->lineSize);
                     y<(img->h-1);
                     y++,
                      pu+=img->lineSize,
                      p+=img->lineSize,
                      pd+=img->lineSize,
                      (*((Byte**)&bufp))+=bufimg->lineSize) {
                    for (x=1; x<(img->w-1); x++) {
                        bufp[x]=(matrix[0]*pu[x-1]+matrix[1]*pu[x]+matrix[2]*pu[x+1]+
                                 matrix[3]* p[x-1]+matrix[4]* p[x]+matrix[5]* p[x+1]+
                                 matrix[6]*pd[x-1]+matrix[7]*pd[x]+matrix[8]*pd[x+1])/divisor;
                        if (x==1 && y==1) {
                            minval=maxval=bufp[x];
                        }
                        else if (minval>bufp[x]) {
                            minval=bufp[x];
                        }
                        else if (maxval<bufp[x]) {
                            maxval=bufp[x];
                        }
                    }
                }

                if (expandEdges) {
                    pu=(img->data+img->lineSize*2);
                    p=(img->data+img->lineSize);
                    pd=img->data;
                    bufp=(long*)(bufimg->data+bufimg->lineSize);
                    /* processing bottom left/right corners */
                    ((long*)bufimg->data)[0]=(matrix[0]* p[0]+matrix[1]* p[0]+matrix[2]* p[1]+
                                              matrix[3]*pd[0]+matrix[4]*pd[0]+matrix[5]*pd[1]+
                                              matrix[6]*pd[0]+matrix[7]*pd[0]+matrix[8]*pd[1])/divisor;
                    minval=min(minval,((long*)bufimg->data)[0]);
                    maxval=max(maxval,((long*)bufimg->data)[0]);
                    ((long*)bufimg->data)[bufimg->w-1]=(matrix[0]* p[img->w-2]+matrix[1]* p[img->w-1]+matrix[2]* p[img->w-1]+
                                                        matrix[3]*pd[img->w-2]+matrix[4]*pd[img->w-1]+matrix[5]*pd[img->w-1]+
                                                        matrix[6]*pd[img->w-2]+matrix[7]*pd[img->w-1]+matrix[8]*pd[img->w-1])/divisor;
                    minval=min(minval,((long*)bufimg->data)[bufimg->w-1]);
                    maxval=max(maxval,((long*)bufimg->data)[bufimg->w-1]);
                    /* processing left & right edges */
                    for (y=1;
                         y<(img->h-1);
                         y++,
                          pu+=img->lineSize,
                          p+=img->lineSize,
                          pd+=img->lineSize,
                          (*((Byte**)&bufp))+=bufimg->lineSize) {
                        bufp[0]=(matrix[0]*pu[0]+matrix[1]*pu[0]+matrix[2]*pu[1]+
                                 matrix[3]* p[0]+matrix[4]* p[0]+matrix[5]* p[1]+
                                 matrix[6]*pd[0]+matrix[7]*pd[0]+matrix[8]*pd[1])/divisor;
                        if (minval>bufp[0]) {
                            minval=bufp[0];
                        }
                        else if (maxval<bufp[0]) {
                            maxval=bufp[0];
                        }
                        bufp[bufimg->w-1]=(matrix[0]*pu[img->w-2]+matrix[1]*pu[img->w-1]+matrix[2]*pu[img->w-1]+
                                           matrix[3]* p[img->w-2]+matrix[4]* p[img->w-1]+matrix[5]* p[img->w-1]+
                                           matrix[6]*pd[img->w-2]+matrix[7]*pd[img->w-1]+matrix[8]*pd[img->w-1])/divisor;
                        if (minval>bufp[bufimg->w-1]) {
                            minval=bufp[bufimg->w-1];
                        }
                        else if (maxval<bufp[bufimg->w-1]) {
                            maxval=bufp[bufimg->w-1];
                        }
                    }

                    /* processing top left/right corners (note: bufp pointing
                    at this point precisely at last image scanline as well
                    as p */
                    ((long*)bufimg->data)[0]=(matrix[0]* p[0]+matrix[1]* p[0]+matrix[2]* p[1]+
                                              matrix[3]* p[0]+matrix[4]* p[0]+matrix[5]* p[1]+
                                              matrix[6]*pd[0]+matrix[7]*pd[0]+matrix[8]*pd[1])/divisor;
                    minval=min(minval,((long*)bufimg->data)[0]);
                    maxval=max(maxval,((long*)bufimg->data)[0]);
                    ((long*)bufimg->data)[bufimg->w-1]=(matrix[0]* p[img->w-2]+matrix[1]* p[img->w-1]+matrix[2]* p[img->w-1]+
                                                        matrix[3]* p[img->w-2]+matrix[4]* p[img->w-1]+matrix[5]* p[img->w-1]+
                                                        matrix[6]*pd[img->w-2]+matrix[7]*pd[img->w-1]+matrix[8]*pd[img->w-1])/divisor;
                    minval=min(minval,((long*)bufimg->data)[bufimg->w-1]);
                    maxval=max(maxval,((long*)bufimg->data)[bufimg->w-1]);

                    /* processing top/bottom edges */
                    bufp=(long*)bufimg->data;
                    pd=p=img->data;
                    pu=img->data+img->lineSize;
                    for (x=1; x<(img->w-1); x++) {
                        bufp[x]=(matrix[0]*pu[x-1]+matrix[1]*pu[x]+matrix[2]*pu[x+1]+
                                 matrix[3]* p[x-1]+matrix[4]* p[x]+matrix[5]* p[x+1]+
                                 matrix[6]*pd[x-1]+matrix[7]*pd[x]+matrix[8]*pd[x+1])/divisor;
                        if (minval>bufp[x]) {
                            minval=bufp[x];
                        }
                        else if (maxval<bufp[x]) {
                            maxval=bufp[x];
                        }
                    }
                    bufp=(long*)(bufimg->data+bufimg->lineSize*(bufimg->h-1));
                    pd=img->data+img->lineSize*(img->h-2);
                    pu=p=pd+img->lineSize;
                    for (x=1; x<(img->w-1); x++) {
                        bufp[x]=(matrix[0]*pu[x-1]+matrix[1]*pu[x]+matrix[2]*pu[x+1]+
                                 matrix[3]* p[x-1]+matrix[4]* p[x]+matrix[5]* p[x+1]+
                                 matrix[6]*pd[x-1]+matrix[7]*pd[x]+matrix[8]*pd[x+1])/divisor;
                        if (minval>bufp[x]) {
                            minval=bufp[x];
                        }
                        else if (maxval<bufp[x]) {
                            maxval=bufp[x];
                        }
                    }

                }

                range=maxval-minval;
            }
            break;
        default:
            croak("%s: unsupported image type",method);
    }

    if (rawOutput) {
        oimg=bufimg;
        if (!expandEdges) {
            /* Filling edges */
            long edgecol;
            edgecol=(edgecolor*range)/255+minval;
            bufp=(long*)(bufimg->data+bufimg->lineSize*(bufimg->w-1));
            for (x=0; x<bufimg->w; x++) {
                ((int*)bufimg->data)[x]=bufp[x]=edgecol;
            }
            for (y=1,bufp=(long*)(bufimg->data+bufimg->lineSize); y<(bufimg->h-1); y++,(*((Byte**)&bufp))+=bufimg->lineSize) {
                bufp[0]=bufp[bufimg->w-1]=edgecol;
            }
        }
    }
    else {
        if (conversionType==CONV_SCALEABS) {
            maxval=abs(maxval);
            minval=abs(minval);
            if (minval>maxval) {
                long tmp=maxval;
                maxval=minval;
                minval=tmp;
            }
            range=maxval-minval;
        }
        oimg=createNamedImage(img->w,img->h,imByte,"filter3x3 result");
        if (oimg) {
            for (y=0,bufp=(long*)bufimg->data,p=oimg->data; y<oimg->h; y++,(*((Byte**)&bufp))+=bufimg->lineSize,p+=oimg->lineSize) {
                for (x=0; x<oimg->w; x++) {
                    switch (conversionType) {
                        case CONV_SCALE:
                            p[x]=((bufp[x]-minval)*255)/range;
                            break;
                        case CONV_SCALEABS:
                            p[x]=((abs(bufp[x])-minval)*255)/range;
                            break;
                        case CONV_TRUNCABS:
                            p[x]=abs(bufp[x])>255 ? 255 : abs(bufp[x]);
                            break;
                        case CONV_TRUNC:
                            p[x]=bufp[x]>255 ? 255 : (bufp[x]<0 ? 0 : bufp[x]);
                            break;
                    }
                }
            }
            if (!expandEdges) {
                /* Filling edges */
                p=oimg->data+oimg->lineSize*(oimg->w-1);
                for (x=0; x<oimg->w; x++) {
                    oimg->data[x]=p[x]=edgecolor;
                }
                for (y=1,p=oimg->data+oimg->lineSize; y<(oimg->h-1); y++,p+=oimg->lineSize) {
                    p[0]=p[bufimg->w-1]=edgecolor;
                }
            }
        }
        destroyImage(bufimg);
    }

    if (!oimg) {
        croak("%s: can't create output image",method);
    }

    return oimg;
}

PImage fast_median(PImage srcimg, int wx, int wy)
{
    PImage dstimg,mimg,msrcimg;
    int xpos,ypos,y,i,ltmdn=0,mdn=0;
    int wx2,wy2,w2,wh,pelshift,inshift,outshift;
    int dx=1; /* H��p������� ������ �� ��p�������� */
    unsigned histogram[256];
    unsigned char *p,*baseline,*dstpos;
    Bool need_turn=false; /* ���������� �� p����p���� ���p������� �������� ���� */

    if (srcimg==nil) {
        return nil;
    } /* endif */
    if ((wx>srcimg->w) || (wy>srcimg->h)) {
        return nil;
    } /* endif */

    msrcimg=createNamedImage(srcimg->w+wx-1,srcimg->h+wy-1,imByte,"msrcimg");
    if (!msrcimg) {
        return nil;
    }

    y=0;
    wx2=(wx/2);
    wy2=(wy/2)*msrcimg->lineSize;
    for (ypos=0; ypos<msrcimg->dataSize; ypos+=msrcimg->lineSize) {
        memset(msrcimg->data+ypos,srcimg->data[y],wx2);
        memcpy(msrcimg->data+ypos+wx2,srcimg->data+y,srcimg->w);
        memset(msrcimg->data+ypos+wx2+srcimg->w,srcimg->data[y+srcimg->w-1],wx2);
        if ((ypos>=wy2) && (ypos<(msrcimg->dataSize-wy2-msrcimg->lineSize))) {
            y+=srcimg->lineSize;
        } /* endif */
    } /* endfor */

    mimg=createNamedImage(msrcimg->w,msrcimg->h,imByte,"mimg");
    if (!mimg) {
        destroyImage(msrcimg);
        return nil;
    }
    memcpy(mimg->data,msrcimg->data,msrcimg->dataSize);

    memset(histogram,0,sizeof(unsigned)*256);

    w2=(wx*wy)/2; /* ���������� ����� � �������� ����. */

    /* ��p��� �p���� - ��������� ������� ��p���� ����. */
    p=msrcimg->data;
    for (ypos=0; ypos<wy; ypos++) {
        for (xpos=0; xpos<wx; xpos++) {
            histogram[p[xpos]]++;
        } /* endfor */
        p+=msrcimg->lineSize;
    } /* endfor */
    for (i=0; i<256; i++) {
        if ((ltmdn+histogram[i])>=w2) {
            mdn=i; /* ��� ��� ������� � ����. ltmdn � ����� ������� ����p���
                    ���������� �����, � �p����� ������ ���������� */
            break;
        } /* endif */
        ltmdn+=histogram[i]; /* � ��� ��� ���� ����� ��� ������� ������� - ����������. */
    } /* endfor */
    mimg->data[(wy/2)*mimg->lineSize+wx/2]=mdn;

    /* ����� ������ ���� � ��� �������. ����p� ���� ���������. */
    baseline=msrcimg->data; /* ������� ����� - ����� ������ � ����.
                            ����� �������� �� �� ��p� ��p�������� �� Y-���p������. */
    xpos=0;                /* �������� ������ �p�� ���� */
    wh=msrcimg->lineSize*wy; /* ����� p����p ������p��, ���p������� �����. */
    inshift=wx;            /* �������� ������������ ������ �p�� ���������� ������� */
    outshift=0;            /* �������� ������������ ������ �p�� ����������� ������� */
    pelshift=(wy/2)*msrcimg->lineSize+wx/2; /* �������� ����������� ����� ������������
                                               ������ ������� �p�� ����. */
    dstpos=mimg->data+pelshift+dx;
    for (; ; ) {
        unsigned char *pin,*pout;

        /* �p������ �� ������ ����, ���p������� �������� �������, �������� �p�������� */
        if (!need_turn) {
            pin=baseline+xpos+inshift;
            pout=baseline+xpos+outshift;
            for (ypos=0; ypos<wy; ypos++, pin+=msrcimg->lineSize, pout+=msrcimg->lineSize) {
                if (*pout<mdn) {
                    ltmdn--;
                } /* endif */
                if (*pin<mdn) {
                    ltmdn++;
                } /* endif */
                histogram[*pout]--;
                histogram[*pin]++;
            } /* endfor */
        } /* endif */

        if (ltmdn>w2) { /* ��� ������, ��� ������� _����������_ ����������, �p���� - ����. */
            /* �������� ������� */
            for (i=mdn-1; ; i--) {
                /* ����� ����� ����� �� �p���p���: p��� ��� ������ ltmdn ��� ��
                 ������ ������ w2; � "������" ������ ��� �p�������� �� 0-�
                 �����, ����� ltmdn �p���� ������ �����.
                 ������������ ��p���� �������� - ������ �p� ��������
                 ������p����, ��������� ����� ���� �������� � ��� ������
                 ������ ���� p���� wx*wy */
                ltmdn-=histogram[i];
                if (ltmdn<=w2) { /* ������ ��� ��������� ������� */
                    mdn=i;
                    break;
                } /* endif */
            } /* endfor */
        } /* endif */
        else {
            /* � ��� ���� �p���p��� - � �� "����"-�� ������� ���p�? */
            for (i=mdn; ; i++) {
                /* ����� ����� ����� ����� ����� �� �p���p��� �� ��� ��
                �p�����, ��� � ��� ������ ��������� ������p����.
                ��p���� "��������" - �� �� ��� ��. */
                if ((ltmdn+histogram[i])>w2) { /* ���� ������ - ������ i - �������� ������� */
                    mdn=i;
                    break;
                } /* endif */
                ltmdn+=histogram[i];
            } /* endfor */
        } /* endelse */
        *dstpos=mdn;

        if (need_turn) {
            need_turn=false;
            dstpos+=dx;
            continue;
        } /* endif */

        xpos+=dx; /* ���������� � ���������� ������� �� X. */
        if (dx>0) {
            if ((xpos+wx)>=msrcimg->w) { /* ���� �������� ��� p�� - �p���� �p���
                                         ���� ������� �� �p���� �p�� image */
                need_turn=true;
            } /* endif */
        } /* endif */
        else { /* dx<0; �p������ �� ���� */
            if (xpos==0) { /* ��������� ��� ������� ��� �� ����� �p�� */
                need_turn=true;
            } /* endif */
        } /* endelse */
        if (need_turn) { /* H��� �������� ���� ���p� �� image, ��������� ������� */
                         /* � ��������� ������. */
            pout=baseline+xpos;
            baseline+=msrcimg->lineSize;
            dstpos+=mimg->lineSize;
            if ((baseline+wh)>(msrcimg->data+msrcimg->dataSize)) { /* ���, ���� ��������� ��� ������ */
                break;
            } /* endif */
            pin=baseline+wh-msrcimg->lineSize+xpos;
            for (i=0; i<wx; i++,pout++,pin++) { /* �������� �� ��p���� - ���������� � ����������� */
                if (*pout<mdn) {
                    ltmdn--;
                } /* endif */
                if (*pin<mdn) {
                    ltmdn++;
                } /* endif */
                histogram[*pout]--;
                histogram[*pin]++;
            } /* endfor */
            /* ��p����� ������� ����� �p�������� �� ��������� �p����� �����. */

            /* ����� - ��p���������� ��� ��������, ����p�� ������ ���������� �p� 
            p����p���. */
            dx=-dx;
            if (dx>0) {
                inshift=wx;
                outshift=0;
            } /* endif */
            else {
                inshift=-1;
                outshift=wx-1;
            } /* endelse */
        } /* endif */
        else {
            dstpos+=dx;
        } /* endelse */
    } /* endfor */

    dstimg=createNamedImage(srcimg->w,srcimg->h,imByte,"median result");
    if (dstimg) {
        for (ypos=0,y=wy2+wx2; ypos<dstimg->dataSize; ypos+=dstimg->lineSize,y+=mimg->lineSize) {
            memcpy(dstimg->data+ypos,mimg->data+y,dstimg->w);
        } /* endfor */
    }

    destroyImage(msrcimg);
    destroyImage(mimg);

    return dstimg;
}

PImage IPA__Local_median(PImage img,HV *profile)
{
    const char *method="IPA::Local::median";
    PImage oimg;
    int wx=0,wy=0;

    if ( !img || !kind_of(( Handle) img, CImage))
      croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }

    if (pexist(w)) {
        wx=pget_i(w);
    }
    if (pexist(h)) {
        wy=pget_i(h);
    }
    if (wx==0) {
        wx=wy;
    }
    if (wy==0) {
        wy=wx;
    }
    if (wx==0 && wy==0) {
        wx=wy=3;
    }
    if (wx<1 || (wx%2)==0) {
        croak("%s: %d is incorrect value for window width",method,wx);
    }
    if (wy<1 || (wy%2)==0) {
        croak("%s: %d is incorrect value for window height",method,wy);
    }
    if (wx>img->w) {
        croak("%s: window width more than image width",method);
    }
    if (wy>img->h) {
        croak("%s: window height more than image height",method);
    }

    if (!(oimg=fast_median(img,wx,wy))) {
        croak("%s: can't create output image",method);
    }

    return oimg;
}

PImage IPA__Local_GEF(PImage img,HV *profile)
{
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

    /* H������� ������� �p��������� 1-�� ��p�����. */

    /* H������� � �p��������� �� x.*/

    /* ���� ����� ���p�. ��p�� �� img, �������� � dx*/
    for (xpos=0; xpos<img->w; xpos++) { /* ��p���p��� ������� ����� ���p���*/
        dx->data[xpos]=img->data[xpos];
        for (ypos=xpos+img->lineSize; ypos<img->dataSize; ypos+=img->lineSize) { /* � ��p��� - ����� ���p�*/
            v=dx->data[ypos-img->lineSize];
            v1=img->data[ypos];
            dx->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ���� ���p�� ����. ��p�� �� dx � �������� � dx*/
    shift=dx->dataSize-dx->lineSize-dx->lineSize;
    for (xpos=shift; xpos<(shift+dx->w); xpos++) { /* ����� ���p��� �� ��������*/
        for (ypos=xpos; ypos>0; ypos-=dx->lineSize) { /* � ���p�� ���� - �� ��p����*/
            v=dx->data[ypos+dx->lineSize];
            v1=dx->data[ypos];
            dx->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ����� ���p���. ��p�� �� dx, �������� � dtmp*/
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dtmp->data[ypos]=dx->data[ypos];
        for (xpos=ypos+1; xpos<(ypos+dx->w); xpos++) {
            v=dtmp->data[xpos-1];
            v1=dx->data[xpos];
            dtmp->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ��p��� ������. ��p�� �� dx, �������� � dy*/
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dy->data[ypos+w1]=dx->data[ypos+w1];
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dx->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* �p����� ������� 1-� �p��������� �� x.*/
    /* �������� ������ ��p�� �� dx, dy � dtmp.*/
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        for (xpos=ypos; xpos<(ypos+dx->w); xpos++) {
            v=dy->data[xpos];
            v1=dtmp->data[xpos];
            dx->data[xpos]=abs(v-v1);
        } /* endfor */
    } /* endfor */

    /* ����p� �� ���p��� �p��������� �� y*/

    /* �p���� ����� ���p���. �� img � dy*/
    for (ypos=0; ypos<img->dataSize; ypos+=img->lineSize) {
        dy->data[ypos]=img->data[ypos];
        for (xpos=(ypos+1); xpos<(ypos+img->w); xpos++) {
            v=dy->data[xpos-1];
            v1=img->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* �p���� ��p��� ������. �� dy � dy*/
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dy->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ������� ����� ���p�. �� dy � dtmp*/
    for (xpos=0; xpos<dy->w; xpos++) {
        dtmp->data[xpos]=dy->data[xpos];
        for (ypos=xpos+dy->lineSize; ypos<dy->dataSize; ypos+=dy->lineSize) {
            v=dtmp->data[ypos-dy->lineSize];
            v1=dy->data[ypos];
            dtmp->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ���� ���p�� ����. ��p�� �� dy � �������� � oimg*/
    shift=dy->dataSize-(dy->lineSize<<1);
    for (xpos=shift; xpos<(shift+dy->w); xpos++) {
        oimg->data[xpos]=dy->data[xpos];
        for (ypos=xpos; ypos>0; ypos-=dy->lineSize) {
            v=oimg->data[ypos+dy->lineSize];
            v1=dy->data[ypos];
            oimg->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ������� �������� 1-� � 2-� �p��������� �� y*/
    /* �������� ������ ��p�� � ddy, dy, dtmp*/
    /* ���������� �������� � dy (1-�) � � ddy (2-� �p���������)*/
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=ypos; xpos<(ypos+dy->w); xpos++) {
            v=dtmp->data[xpos];
            v1=oimg->data[xpos];
            dy->data[xpos]=abs(v-v1);
        } /* endfor */
    } /* endfor */

    /* � ����p�, �� ���� ���������� ����p���� � dx,dy,ddx,ddy �p����� ��������*/
    /* ������p����� image.*/
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

    /* H������� ������� �p��������� 1-�� � 2-�� ��p�����.*/

    /* H������� � �p��������� �� x.*/

    /* ���� ����� ���p�. ��p�� �� img, �������� � dx*/
    for (xpos=0; xpos<img->w; xpos++) { /* ��p���p��� ������� ����� ���p���*/
        dx->data[xpos]=img->data[xpos];
        for (ypos=xpos+img->lineSize; ypos<img->dataSize; ypos+=img->lineSize) { /* � ��p��� - ����� ���p�*/
            v=dx->data[ypos-img->lineSize];
            v1=img->data[ypos];
            dx->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ���� ���p�� ����. ��p�� �� dx � �������� � dx*/
    shift=dx->dataSize-dx->lineSize-dx->lineSize;
    for (xpos=shift; xpos<(shift+dx->w); xpos++) { /* ����� ���p��� �� ��������*/
        for (ypos=xpos; ypos>0; ypos-=dx->lineSize) { /* � ���p�� ���� - �� ��p����*/
            v=dx->data[ypos+dx->lineSize];
            v1=dx->data[ypos];
            dx->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ����� ���p���. ��p�� �� dx, �������� � ddx*/
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        ddx->data[ypos]=dx->data[ypos];
        for (xpos=ypos+1; xpos<(ypos+dx->w); xpos++) {
            v=ddx->data[xpos-1];
            v1=dx->data[xpos];
            ddx->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ��p��� ������. ��p�� �� dx, �������� � dy*/
    for (ypos=0; ypos<dx->dataSize; ypos+=dx->lineSize) {
        dy->data[ypos+w1]=dx->data[ypos+w1];
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dx->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* �p����� ������� 1-� � 2-� �p��������� �� x.*/
    /* �������� ������ ��p�� �� dx, dy � ddx.*/
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

    /* ����p� �� ���p��� �p��������� �� y*/

    /* �p���� ����� ���p���. �� img � dy*/
    for (ypos=0; ypos<img->dataSize; ypos+=img->lineSize) {
        dy->data[ypos]=img->data[ypos];
        for (xpos=(ypos+1); xpos<(ypos+img->w); xpos++) {
            v=dy->data[xpos-1];
            v1=img->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* �p���� ��p��� ������. �� dy � dy*/
    for (ypos=0; ypos<dy->dataSize; ypos+=dy->lineSize) {
        for (xpos=(ypos+w2); xpos>=ypos; xpos--) {
            v=dy->data[xpos+1];
            v1=dy->data[xpos];
            dy->data[xpos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ������� ����� ���p�. �� dy � ddy*/
    for (xpos=0; xpos<dy->w; xpos++) {
        ddy->data[xpos]=dy->data[xpos];
        for (ypos=xpos+dy->lineSize; ypos<dy->dataSize; ypos+=dy->lineSize) {
            v=ddy->data[ypos-dy->lineSize];
            v1=dy->data[ypos];
            ddy->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ���� ���p�� ����. ��p�� �� dy � �������� � dtmp*/
    shift=dy->dataSize-(dy->lineSize<<1);
    for (xpos=shift; xpos<(shift+dy->w); xpos++) {
        dtmp->data[xpos]=dy->data[xpos];
        for (ypos=xpos; ypos>0; ypos-=dy->lineSize) {
            v=dtmp->data[ypos+dy->lineSize];
            v1=dy->data[ypos];
            dtmp->data[ypos]=v+a0*(v1-v)+0.5;
        } /* endfor */
    } /* endfor */

    /* ������� �������� 1-� � 2-� �p��������� �� y*/
    /* �������� ������ ��p�� � ddy, dy, dtmp*/
    /* ���������� �������� � dy (1-�) � � ddy (2-� �p���������)*/
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

    /* � ����p�, �� ���� ���������� ����p���� � dx,dy,ddx,ddy �p����� ��������*/
    /* ������p����� image.*/
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

/*
 * Union Related
 */
static int
find_compress( int *p, int node)
{
   if ( p[node] < 0)
      return node;
   else
      return p[node] = find_compress( p, p[ node]);
}

PImage union_find_ave( PImage in, int threshold)
{
   /* Input:*/
   /*    image*/
   /*    threshold value*/

   /* Output:  image with pixel values set to region's average*/
   PImage out;

   /* Data:    pointer image     <- basic structure*/
   int *p;

   /* Data:    sums image        <- Oracle()-related structure*/
   int *sums;

   /* Data:    sizes image       <- Oracle()-related structure*/
   int *sizes;

   /* Control variables:*/
   int x, y, w, h;
   int left, up, focus;

   /* Initialize: set sums to individual values,*/
   /*             sizes to one and pointers to -1*/
   w = in-> w;    h = in-> h;
   p = malloc( sizeof( int) * w * h);
   sums = malloc( sizeof( int) * w * h);
   sizes = malloc( sizeof( int) * w * h);
   for ( y = 0; y < h; y++)
      for ( x = 0; x < w; x++)
      {
         p[ y*w + x] = -1;
         sums[ y*w + x] = in-> data[ y * in-> lineSize + x];
         sizes[ y*w + x] = 1;
      }

   /* Special treatment of the first line:*/
   for ( x = 1; x < w; x++)
   {
      /* left <- FindCompress(0,x-1);*/
      left = find_compress( p, x - 1);
      /* focus <- FindCompress(0,x);*/
      focus = find_compress( p, x);
      /* if Oracle(left,focus) then Union(left,focus);*/
      if ( fabs(sums[ left] / (float) sizes[ left] - sums[ focus] / (float) sizes[ focus]) < threshold)
      {
         sums[left] += sums[focus];
         sizes[left] += sizes[focus];
         p[focus] = left;
      }
   }
   /* Flatten(0);*/
   for ( x = 0; x < w; x++) find_compress( p, x);

   /* Main loop*/
   for ( y = 1; y < h; y++)
   {
      /* Special treatment of the first pixel on every line:*/
      /* up <- FindCompress(y-1, 0);*/
      up = find_compress( p, w*(y-1));
      /* focus <- FindCompress(y,0);*/
      focus = find_compress( p, w*y);
      /* if Oracle(up,focus) then Union(up,focus);*/
      if ( fabs(sums[ up] / (float) sizes[ up] - sums[ focus] / (float) sizes[ focus]) < threshold)
      {
         sums[up] += sums[focus];
         sizes[up] += sizes[focus];
         p[focus] = up;
      }

      /* Line processing*/
      for ( x = 1; x < w; x++)
      {
         /* left <- FindCompress(y,x-1);*/
         left = find_compress( p, w*y+x-1);
         /* up <- FindCompress(y-1, x);*/
         up = find_compress( p, w*(y-1)+x);
         /* focus <- FindCompress(y,x);*/
         focus = find_compress( p, w*y+x);
         /* if Oracle(left,focus) then focus <- Union(left,focus);*/
         if ( fabs(sums[ left] / (float) sizes[ left] - sums[ focus] / (float) sizes[ focus]) < threshold)
         {
            sums[left] += sums[focus];
            sizes[left] += sizes[focus];
            p[focus] = left;
            focus = left;
         }
         /* if Oracle(up,focus) then Union(up,focus);*/
         if ((focus != up) && ( fabs(sums[ up] / (float) sizes[ up] - sums[ focus] / (float) sizes[ focus]) < threshold))
         {
            sums[up] += sums[focus];
            sizes[up] += sizes[focus];
            p[focus] = up;
         }
      }
      /* Flatten(y);*/
      for ( x = 0; x < w; x++) find_compress( p, w*y+x);
   }

   /* Finalize: create output image and color it*/
   out = createImage( in-> w, in-> h, in-> type);
   for ( y = 0; y < h; y++)
      for ( x = 0; x < w; x++)
      {
         focus = y*w+x;
         while ( p[ focus] >= 0) focus = p[ focus];   /* Only one or zero steps, no more, actually*/
         out-> data[ y*out-> lineSize + x] = (unsigned char)(sums[ focus] / (float) sizes[ focus] + 0.5);
      }
   /* Calculate the number of regions*/
   {
      int n = 0;
      for ( y = 0; y < h; y++)
         for ( x = 0; x < w; x++)
            if ( p[ y*w+x] < 0)
               n++;
   }
   /* Finalize: destroy temporary matrices*/
   free( p); free( sums); free( sizes);
   return out;
}

PImage IPA__Local_unionFind(PImage img,HV *profile)
{
    typedef enum {
        UAve, Unknown=-1
    } UMethod;

    const char *method="IPA::Local::unionFind";
    PImage oimg;
    UMethod umethod=Unknown;
    struct {
        UMethod umethod;
        const char *name;
    } UnionMethods[] = {
            {UAve, "Ave"},
            {Unknown, NULL}
        };

    if ( !img || !kind_of(( Handle) img, CImage))
      croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: unsupported image type",method);
    }

    if (pexist(method)) {
        char *mname=pget_c(method);
        int i;

        for (i=0; UnionMethods[i].name; i++) {
            if (stricmp(mname,UnionMethods[i].name)==0) {
                umethod=UnionMethods[i].umethod;
                break;
            }
        }
        if (umethod==Unknown) {
            croak("%s: unknown method",method);
        }
    }
    switch (umethod) {
        case UAve:
            {
                int threshold;

                if (pexist(threshold)) {
                    threshold=pget_i(threshold);
                }
                else {
                    croak("%s: threshold must be specified",method);
                }
                oimg=union_find_ave(img,threshold);
            }
            break;
        default:
            croak("%s: (internal) method unknown",method);
            break;
    }

    return oimg;
}

PImage IPA__Local_deriche(PImage img,HV *profile)
{
    const char *method="IPA::Local::deriche";
    float alpha;

    if ( !img || !kind_of(( Handle) img, CImage))
      croak("%s: not an image passed", method);

    if (img->type!=imByte) {
        croak("%s: incorrect image type",method);
    }

    if (pexist(alpha)) {
        alpha=pget_f(alpha);
    }
    else {
        croak("%s: alpha must be defined",method);
    }

    return deriche(method,img,alpha);
}

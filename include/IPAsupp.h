/* $Id$ */

#ifndef __IPA_H__
#define __IPA_H__

#include <apricot.h>
#include <Image.h>

#define createImage(w,h,type)               create_object("Prima::Image","iii","width",(w),"height",(h),"type",(type))
#define createNamedImage(w,h,type,name)     create_object("Prima::Image","iiis","width",(w),"height",(h),"type",(type),"name",(name))
#define destroyImage(img)                   Object_destroy((Handle)img)

#ifndef min
#define min(x,y)                ((x)<(y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y)                ((x)>(y) ? (x) : (y))
#endif
#ifndef sign
#define sign(x)                 ((x)<0 ? -1 : ((x)>0 ? 1 : 0))
#endif

#define COMBINE_MAXABS          1
#define COMBINE_SUMABS          2
#define COMBINE_SUM             3
#define COMBINE_SQRT            4
#define COMBINE_SIGNEDMAXABS    5
#define COMBINE_FIRST           COMBINE_MAXABS
#define COMBINE_LAST            COMBINE_SIGNEDMAXABS
                                        
#define CONV_TRUNCABS           1
#define CONV_TRUNC              2
#define CONV_SCALE              3
#define CONV_SCALEABS           4
#define CONV_FIRST              CONV_TRUNCABS
#define CONV_LAST               CONV_SCALEABS

extern int                              AV2intp(SV *,int **);
extern PImage                           create_compatible_image(PImage,Bool);

#endif /* __IPA_H__ */

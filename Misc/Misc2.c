/* $Id$ */

#include "IPAsupp.h"
#include "Misc.h"
#include "MiscSupp.h"

#undef METHOD
#define METHOD "IPA::Misc::split_channels"
#define WHINE(msg) croak( "%s: %s", METHOD, (msg))

SV * 
IPA__Misc_split_channels( PImage input, char * mode)
{
   int m, count = 0;
   AV * av;
   Handle ch[16];
   
   if ( !input || !kind_of(( Handle) input, CImage))
      croak("%s: not an image passed", METHOD);
   
   if ( stricmp( mode, "rgb") == 0) m = 0;
   else WHINE("unknown mode");

      
   switch ( m) {
   case 0:  {
      Byte * src = input-> data;
      Byte * dst[3];      
      int y = input-> h, srcd = input-> lineSize - input-> w * 3, dstd;
      if ( input-> type != imbpp24) WHINE("rgb mode accepts 24 RGB images only");
      count = 3;
      for ( m = 0; m < 3; m++) {
         HV * profile = newHV();
         pset_i( type, imByte);
         pset_i( width,  input-> w);
         pset_i( height, input-> h);
         ch[m] = Object_create( "Prima::Image", profile);
         dst[m] = PImage(ch[m])-> data;
         sv_free(( SV *) profile);
      }
      dstd = PImage(ch[0])-> lineSize - input-> w;
      // printf("s[%08x]d[%08x %08x %08x], y %d delta[%d %d]\n", src, dst[0], dst[1], dst[2], y, srcd, dstd);
      while ( y--) {
         int x = input-> w;
         while ( x--) {
            *((dst[0])++) = *(src++);
            *((dst[1])++) = *(src++);
            *((dst[2])++) = *(src++);
         }   
         src += srcd;
         for ( m = 0; m < 3; m++) dst[m] += dstd;
      }
      // swap r and b
      ch[3] = ch[0];
      ch[0] = ch[2];
      ch[2] = ch[3];
      break;
   }   
   }
   
   av = newAV();
   for ( m = 0; m < count; m++)
      av_push( av, newRV( SvRV( PObject(ch[m])-> mate)));
   return newRV_noinc(( SV*) av);
}   


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
   float a = -(1.0-exp(-alpha))*(1.0-exp(-alpha));
   float b1 = -2.0 * exp(-alpha);
   float b2 = exp(-2.0*alpha);
   float a0 = -a/(1.0 - alpha * b1 - b2);
   float a1 = a0*(alpha-1.0)*exp(-alpha);
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

// COMPUTATION of H(x,y)
// run I: bottom-up
   for ( y = 2; y < n; y++)
   {
      deriche_READLINE(ze,y-1);
      for ( x = 0; x < n; x++)
         im1[deriche_INDEX(x,y)] = ze[x] - b1*im1[deriche_INDEX(x,y-1)] - b2*im1[deriche_INDEX(x,y-2)];
   }

// run II: top-down
   for ( y = n - 3; y >= 0; y--)
   {
      deriche_READLINE(ze,y+1);
      for ( x = 0; x < n; x++)
      {
         im2[deriche_INDEX(x,y)] = ze[x] - b1*im2[deriche_INDEX(x,y+1)] - b2*im2[deriche_INDEX(x,y+2)];
         im1[deriche_INDEX(x,y)] = a*(im1[deriche_INDEX(x,y)] - im2[deriche_INDEX(x,y)]);
      }
   }

// runs III - IV:  left to right or right to left
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

// COMPUTATION of V(x,y)
// runs V - VI: left to right or right to left
   for ( y = 0; y < n; y++)
   {
      deriche_READLINE(ze,y);
      for ( x = 2; x < n; x++)
         z2[x] = ze[x-1] - b1*z2[x-1] - b2*z2[x-2];
      for ( x = n - 3; x >= 0; x--)
         z3[x] = ze[x+1] - b1*z3[x+1] - b2*z3[x+2];
      for ( x = 0; x < n; x++)  im1[deriche_INDEX(x,y)] = a*(z2[x]-z3[x]);
   }

// run VII: bottom-up
   for ( y = 2; y < n; y++)
      for ( x = 0; x < n; x++)
         im2[deriche_INDEX(x,y)] = a0*im1[deriche_INDEX(x,y)] + a1*im1[deriche_INDEX(x,y)] -
                           b1*im2[deriche_INDEX(x,y-1)] - b2*im2[deriche_INDEX(x,y-2)];

// run VIII: top-down
   for ( y = n - 3; y >= 0; y--)
   {
      for ( x = 0; x < n; x++)   z1[x] = im2[deriche_INDEX(x,y)];
      for ( x = 0; x < n; x++)
         im2[deriche_INDEX(x,y)] = a2*im1[deriche_INDEX(x,y+1)] + a3*im1[deriche_INDEX(x,y+2)] -
                           b1*im2[deriche_INDEX(x,y+1)] - b2*im2[deriche_INDEX(x,y+2)];
      for ( x = 0; x < n; x++)
      {
         z = im2[deriche_INDEX(x,y)]+z1[x];
         za[x] = sqrt(z*z+outf[deriche_INDEX(x,y)]*outf[deriche_INDEX(x,y)]);
      }
      deriche_WRITELINE(za,y);
   }

   {
      float min = FLT_MAX, max = -FLT_MAX;
      for ( y = 0; y < n; y++)
         for ( x = 0; x < n; x++)
         {
            if ( outf[deriche_INDEX(x,y)] < min)
               min = outf[deriche_INDEX(x,y)];
            if ( outf[deriche_INDEX(x,y)] > max)
               max = outf[deriche_INDEX(x,y)];
         }
   }

   out = createImage( in-> w, in-> h, in->type);
   for ( y = 0; y < n; y++)
      for ( x = 0; x < n; x++)
         // out-> data[y*out-> lineSize + x] = outf[deriche_INDEX(x,y)] < 1 ? 0 : 255;
         out-> data[y*out-> lineSize + x] = outf[deriche_INDEX(x,y)] + 0.5;

   free( z1); free( z2); free( z3);
   free( ze); free( za);
   free( im1); free( im2); free( outf);

   return out;
}

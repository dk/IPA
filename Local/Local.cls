# $Id$
local $PImage => Handle;

package IPA::Local {
   PImage crispening(PImage input);
   PImage sobel(PImage input,HV *profile);
   PImage GEF(PImage input,HV *profile);
   PImage SDEF(PImage input,HV *profile);
   PImage deriche(PImage input, HV *profile);
   PImage filter3x3(PImage input,HV *profile);
   PImage median(PImage input,HV *profile);
   PImage unionFind(PImage input,HV *profile);
}


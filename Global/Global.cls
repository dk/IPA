# $Id$
local $PImage => Handle;

package IPA::Global {
    PImage close_edges(PImage input,HV *profile);
    PImage fill_holes(PImage input,HV *profile);
    PImage area_filter(PImage input,HV *profile);
    SV* identify_contours(PImage input,HV *profile);
}

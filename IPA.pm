# $Id$
package IPA;
use Prima::Classes;
use strict;
require Exporter;
require DynaLoader;

use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
@ISA = qw(Exporter DynaLoader);

sub dl_load_flags { 0x01 };

$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw();
%EXPORT_TAGS = ();

bootstrap IPA $VERSION;

use constant combineMaxAbs       => 1;
use constant combineSumAbs       => 2;
use constant combineSum          => 3;
use constant combineSqrt         => 4;
use constant combineSignedMaxAbs => 5;

use constant conversionTruncAbs  => 1;
use constant conversionTrunc     => 2;
use constant conversionScale     => 3;
use constant conversionScaleAbs  => 4;

1;

__END__

=pod

=head1 NAME

IPA - Image Processing Algorithms

=head1 DESCRIPTION

IPA stands for Image Processing Algorithms and represents
the library of image processing operators and functions.
IPA is based on the Prima toolkit ( http://www.prima.eu.org ),
which in turn is a perl-based graphic library. IPA is
designed for solving image analysis and object recognition
tasks in perl.

=head1 USAGE

IPA works mostly with grayscale images, which can be loaded
or created by means of Prima toolkit. See L<Prima::Image> for
the information about C<Prima::Image> class functionality.
IPA methods are grouped in several modules, that contain the
specific functions. The functions usually accept one or more
images and optional parameter hash. Each function has its own
set of parameters. If error occurs, the functions call C<die>,
so it is advisable to use C<eval> blocks around the calls.

A code that produces a binary thresholded image out of a 8-bit 
grayscale image is exemplified:

   use Prima;
   use IPA;
   use IPA::Point;
   my $i = Prima::Image-> load('8-bit-grayscale.gif');
   die "Cannot load:$@\n" if $@;
   my $binary = IPA::Point::threshold( $i, minvalue => 128);

The abbreviations for pixel types are used, derived from
the C<im::XXX> image type constants, as follows:

   im::Byte     - 8-bit unsigned integer
   im::Short    - 16-bit signed integer
   im::Long     - 32-bit signed integer
   im::Float    - float
   im::Double   - double
   im::Complex  - complex float
   im::DCOmplex - complex double

Each function returns the newly created image object with the result of the operation,
unless stated otherwise in L<API>.

=head1 API

=head2 IPA::Point

C<IPA::Point> module contains functions that perform single point
transformations and image arithmetic.

Single-point processing is a simple method of image enhancement. 
This technique determines a pixel value in the enhanced image dependent only 
on the value of the corresponding pixel in the input image. 
The process can be described with the mapping function 

   s = M(r)

where C<r> and C<s> are the pixel values in the input and output images, respectively.

=over   

=item combine [ images, conversionType = conversionScale, combineType = combineSum, rawOutput = 0]

Combines set of images of same dimension and bit depth into one
and returns the resulting image. 

Supported types: Byte, Short, Long.

Parameters:

=over

=item images ARRAY

Array of image objects.

=item conversionType INTEGER

An integer constant, one of the following, that indicates how the
resulting image would be adjusted in accord to the minimal and maximal
values of the result. C<Trunc> constants cut off the output values to the 
bit maximum, for example, a result vector in 8-bit image [-5,0,100,300]
would be transformed to [0,0,100,255]. C<Scale> constants scale the whole
image without the cutoff; the previous example vector would be transformed
into [0,4,88,255]. The C<Abs> suffix shows whether the range calculation would
use the whole domain, including the negative values, or the absolute values
only.

   conversionTruncAbs
   conversionTrunc
   conversionScale
   conversionScaleAbs

Default is C<conversionScale>.

=item combineType INTEGER

An integer constant, indicates the type of action performed
between pixels of same [x,y] coordinates.

   combineMaxAbs          - store the maximal absolute pixel value
   combineSignedMaxAbs    - compute the maximal absolute value, but store its original ( before abs()) value
   combineSumAbs          - store the sum of absolute pixel values
   combineSum             - store the sum of pixel values
   combineSqrt            - store the square root of the sum of the squares of the pixel values

Default is C<combineSum>.

=item rawOutput BOOLEAN

Discards C<conversionType> parameter and performs no conversion.
If set to true value, the conversion step is omitted. 

Default is 0.
 
=back

=item threshold IMAGE [ minvalue, maxvalue = 255]

Performs the binary thresholding, governed by
C<minvalue> and C<maxvalue>.
The pixels, that are below C<minvalue> and above C<maxvalue>,
are mapped to value 0; the other values mapped to 255.

Supported types: Byte

=item gamma IMAGE [ origGamma = 1, destGamma = 1]

Performs gamma correction of IMAGE by a product of
C<origGamma> and C<destGamma>.

Supported types: Byte

=item remap IMAGE [ lookup ] 

Performs image mapping by a passed C<lookup> array
of 256 integer values. Example: 

   IPA::Point::remap( $i, lookup => [ (0) x 128, (255) x 127]);

is an equivalent of

   IPA::Point::threshold( $i, minvalue => 128);

Supported types: 8-bit

=item subtract IMAGE1, IMAGE2, [ conversionType = conversionScale, rawOutput = 0]

Subtracts IMAGE2 from IMAGE1. The images must be of same dimension.
For description of C<conversionType> and C<rawOutput> see L<combine>.

Supported types: Byte

=item mask IMAGE [ test, match, mismatch ]

Test every pixel of IMAGE whether it equals to C<test>, and
assigns the resulting pixel with either C<match> or C<mismatch> value.
All C<test>, C<match>, and C<mismatch> scalars can be either integers
( in which case C<mask> operator is similar to L<threshold> ),
or image objects. If the image objects passed, they must be of the same 
dimensions and bit depth as IMAGE.

Supported types: Byte, Short, Long.

=item average LIST

Combines images of same dimensions and bit depths, passed as an
anonymous array in LIST and returns the average image.

Supported types: Byte, Short, Long, 64-bit integer.

=item equalize IMAGE

Returns a histogram-equalized image.

Supported types: Byte

=back

=head2 IPA::Local

Contains functions that operate in the vicinity of a pixel, and produce
image where every pixel is dependant on the values of the source pixel
and the values of its neighbors.
The process can be described with the mapping function 

         |r(i,j),r(i+1,j)...|
   s = M |...               |
         |r(j+1,i) ...      |

where C<r> and C<s> are the pixel values in the input and output images, respectively.

=over

=item crispening IMAGE

Applies the crispening algorithm to IMAGE and returns the result.

Supported types: Byte

=item sobel IMAGE [ jobMask = sobelNWSE|sobelNESW, conversionType = conversionScaleAbs, combineType = combineMaxAbs, divisor = 1]

Applies Sobel edge detector to IMAGE. 

Supported types: Byte

Parameters:

=over

=item jobMask INTEGER

Combination of the integer constants, that mask the pixels in Sobel 3x3 kernel.
If the kernel is to be drawn as

  | (-1,1) (0,1) (1,1) |
  | (-1,0) (0,0) (1,0) |
  | (-1,-1)(0,-1)(1,-1)|
   
Then the constants mask the following points:

   sobelRow      - (-1,0),(1,0)
   sobelColumn   - (0,1),(0,-1)
   sobelNESW     - (1,1),(-1,-1)
   sobelNWSE     - (-1,1),(1,-1)

(0,0) point is always masked.

=item divisor INTEGER 

The resulting pixel value is divided to C<divisor> value after the kernel convolution is applied.

=back

C<conversionType> and <combineType> parameters described in L<combine>.

=item GEF IMAGE [ a0 = 1.3, s = 0.7]

Applies GEF algorithm ( first derivative operator for symmetric exponential filter) to IMAGE.

Supported types: Byte

=item SDEF IMAGE [ a0 = 1.3, s = 0.7]

Applies SDEF algorithm ( second derivative operator for symmetric exponential filter) to IMAGE.

Supported types: Byte

=item deriche IMAGE [ alpha ]

Applies Deriche edge detector.

Supported types: Byte

=item filter3x3 IMAGE [ matrix, expandEdges = 0, edgecolor = 0, conversionType = conversionScaleAbs, rawOutput = 0, divisor = 1 ]

Applies convolution with a custom 3x3 kernel, passed in C<matrix>.

Supported types: Byte

Parameters:

=over

=item matrix ARRAY

Array of 9 integers, a 3x3 kernel, to be convolved with IMAGE. Indeces are:

  |0 1 2|
  |3 4 5|
  |6 7 8|

=item expandEdges BOOLEAN

If false, the edge pixels ( borders ) not used in the convolution as center
pixels. If true, the edge pixels used, and in this case C<edgecolor> value
is used to substitute the pixels outside the image.

=item edgecolor INTEGER

Integer value, used for substitution of pixel values outside IMAGE, when 
C<expandEdges> parameter is set to 1. 

=item divisor INTEGER 

The resulting pixel value is divided to C<divisor> value after the kernel convolution is applied.

=item conversionType

See L<combine>

=item rawOutput

See L<combine>

=back

=item median IMAGE [ w = 0, h = 0 ]

Performs adaptive thresholding with median filter with window dimensions C<w> and C<h>.

=item unionFind IMAGE [ method, threshold  ]

Applies a union find algorithm selected by C<method>. The only implemented
method is average-based region grow ( 'ave' string constant ). Its only
parameter is C<threshold>, integer value of the balance merger function.

Supported types: Byte

=back

=head2 IPA::Global

Contains methods that produce images, where every pixel is a function
of all pixels in the source image.
The process can be described with the mapping function 

   s = M(R)
  
where C<s> is the pixel value in the output images, and R is the source image.

=item close_edges IMAGE [ gradient, maxlen, minedgelen, mingradient ]

Closes edges of shapes on IMAGE, according to specified C<gradient> image.
The unclosed shapes converted to the closed if the gradient spot between the
suspected dents falls under C<maxlen> maximal length increment, C<mingradient>
the minimal gradient value and the edge is longer than C<minedgelen>.

Supported types: Byte

Parameters:

=over

=item gradient IMAGE

Specifies the gradient image

=item maxlen INTEGER

Maximal edge length

=item minedgelen INTEGER  

Minimal edge length

=item mingradient INTEGER

Minimal gradient value

=back

=item fill_holes IMAGE [ inPlace = 0, edgeSize = 1, backColor = 0, foreColor = 255, neighborhood = 4]

Fills closed shapes to eliminate the contours with holes in IMAGE.

Supported types: Byte

Parameters:

=over

=item inPlace BOOLEAN

If true, the original image is changed

=item edgeSize INTEGER

The edge breadth that is not touched by the algorithm

=item backColor INTEGER

The pixel value used for determination whether a pixel belongs to
the background.

=item foreColor INTEGER

The pixel value used for hole filling.

=item neighborhood INTEGER

Must be either 4 or 8.
Selects whether the algorithm must assume 4- or 8- pixel connection.

=back

=item area_filter IMAGE [ minArea = 0, maxArea = INT_MAX, inPlace = 0, edgeSize = 1, backColor = 0, foreColor = 255, neighborhood = 4]

Identifies the objects on IMAGE and filters out these that have their area less than C<minArea>
and more than C<maxArea>. The other parameters are identical to those passed to L<fill_holes>.

=item identify_contours IMAGE [ edgeSize = 1, backColor = 0, foreColor = 255, neighborhood = 4]

Identifies the objects on IMAGE and returns the contours as array of anonymous arrays of
4- or 8- connected pixel coordinates.

The parameters are identical to those passed to L<fill_holes>.

Supported types: Byte

See also L<IPA::Region>.

=item fft IMAGE [ inverse = 0 ]

Performs direct and inverse ( governed by C<inverse> boolean flag )
fast Fourier transform. IMAGE must have dimensions of power of 2.
The resulted image is always of DComplex type.

Supported types: all

=item fourier IMAGE [ inverse = 0 ]

Performs direct and inverse ( governed by C<inverse> boolean flag )
fast Fourier transform. If IMAGE dimensions not of power of 2, then
IMAGE is scaled up to the closest power of 2, and the result is scaled
back to the original dimensions.

The resulted image is always of DComplex type.

Supported types: all

=item band_filter IMAGE [ low = 0, spatial = 1, homomorph = 0, power = 2.0, cutoff = 20.0, boost = 0.7 ]

Performs band filtering of IMAGE in frequency fomain. 
IMAGE must have dimensions of power of 2.
The resulted image is always of DComplex type.

Supported types: all

Parameters:

=over

=item low BOOLEAN

Boolean flag, indicates whether the low-pass or the high-pass is to be performed.

=item spatial BOOLEAN

Boolean flag, indicates if IMAGE must be treated as if it is in the spatial domain,
and therefore conversion to the frequency domain must be performed first.

=item homomorph BOOLEAN

Boolean flag, indicates if the homomorph ( exponential ) equalization must be performed. Cannot
be set to true if the image is in frequency domain ( if C<spatial> parameter set to true ).

=item power FLOAT

Power operator applied to the input frequency.

=item cutoff FLOAT

Threshold value of the filter.

=item boost FLOAT

Multiplication factor used in homomorph equalization.

=item butterworth IMAGE [ low = 0, spatial = 1, homomorph = 0, power = 2.0, cutoff = 20.0, boost = 0.7 ]

Performs band filtering of IMAGE in frequency fomain. 
If IMAGE dimensions not of power of 2, then
IMAGE is scaled up to the closest power of 2, and the result is scaled
back to the original dimensions.

The resulted image is always of DComplex type.

Supported types: all

The parameters are same as those passed to L<band_filter>.

=back

=head2 IPA::Morphology

Quote from L<http://www.dai.ed.ac.uk/HIPR2/morops.htm>:

Morphological operators often take a binary image and a structuring element as input 
and combine them using a set operator (intersection, union, inclusion, complement). 
They process objects in the input image based on characteristics of its shape, which are 
encoded in the structuring element. 

Usually, the structuring element is sized 3x3 and has its origin at the center pixel. 
It is shifted over the image and at each pixel of the image its elements are compared with 
the set of the underlying pixels. If the two sets of elements match the condition defined 
by the set operator (e.g. if the set of pixels in the structuring element is a subset 
of the underlying image pixels), the pixel underneath the origin of the structuring 
element is set to a pre-defined value (0 or 1 for binary images). 
A morphological operator is therefore defined by its structuring element and the 
applied set operator. 

Morphological operators can also be applied to graylevel images, e.g. 
to reduce noise or to brighten the image. 

=over

=item BWTransform IMAGE [ lookup ]

Applies 512-byte C<lookup> LUT string ( look-up table ) to image and returns 
the convolution result ( hit-and-miss transform). Each byte of C<lookup> is a set 
of bits, each corresponding to the 3x3 kernel index:

   |4 3 2|
   |5 0 1|
   |6 7 8|

Thus, for example, the X-shape would be represented by offset 2**0 + 2**2 + 2**4 + 2**6 + 2**8 = 341 .
The byte value, corresponding to the offset in C<lookup> string is stored in the output
image.

C<IPA::Morphology> defines several basic LUT transforms, which can be invoked by the following
code:

    IPA::Morphological::bw_METHOD( $image);

or its alternative

    IPA::Morphology::BWTransform( $image, lookup => $IPA::Morphology::transform_luts{METHOD}->());

Where METHOD is one of the followng string constants:

=over

=item dilate

Morphological dilation

=item erode

Morphological erosion

=item isolatedremove

Remove isolated pixels

=item togray

Convert binary image to grayscale by appying the mean filter

=item invert

Inversion operator

=item prune

Removes 1-connected end points

=item break_node

Removes node points that connect 3 or more lines

=back

Supported types: Byte

=item dilate IMAGE [ neighborhood = 8 ]

Performs morphological dilation operation on IMAGE and returns the result.
C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item erode IMAGE [ neighborhood = 8 ]

Performs morphological erosion operation on IMAGE and returns the result.
C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item opening IMAGE [ neighborhood = 8 ]

Performs morphological opening operation on IMAGE and returns the result.
C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item closing IMAGE [ neighborhood = 8 ]

Performs morphological closing operation on IMAGE and returns the result.
C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item gradient IMAGE [ neighborhood = 8 ]

Returns the result or the morphological gradient operator on IMAGE.
C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item algebraic_difference IMAGE1, IMAGE2 [ inPlace = 0 ]

Performs the algebraic difference between IMAGE1 and IMAGE2.
Although this is not a morphological operator, it is often used is
conjuction with ones. If the boolean flag C<inPlace> is set, 
IMAGE1 contains the result.

Supported types: Byte, Short, Long, Float, Double

=item watershed IMAGE [ neighborhood = 4 ]

Applies the watershed segmentation to IMAGE with given C<neighborhood>.

Supported types: Byte

=item reconstruct IMAGE1, IMAGE2 [ neighborhood = 8, inPlace = 0 ]

Performs morphological reconstruction of IMAGE1 under the mask IMAGE2. Images can be two 
intensity images or two binary images with the same size. The returned image, is an intensity 
or binary image, respectively. 

If boolean C<inPlace> flag is set, IMAGE2 contains the result.

C<neighborhood> determines whether the algorithm assumes 4- or 8- pixel connectivity.

Supported types: Byte, Short, Long, Float, Double

=item thinning IMAGE

Applies the skeletonization algorithm, returning image with binary object maximal
euclidian distance points set.

Supported types: Byte

=back

=head2 IPA::Geometry

Contains function for mapping pixels from one location to another

=over

=item mirror IMAGE [ type ]

Mirrors IMAGE vertically or horizontally, depending on integer C<type>,
which can be one of the following constants:

   IPA::Geometry::vertical
   IPA::Geometry::horizontal

Supported types: all

=item shift_rotate IMAGE [ where, size ]

Shifts image in direction C<where>, which is one of the following constants

   IPA::Geometry::vertical
   IPA::Geometry::horizontal

by the offset, specified by integer C<size>.

Supported types: all, except that the horizontal transformation does not
support 1- and 4- bit images.

=back

=head2 IPA::Misc

Contains miscellaneous helper routines.

=over

=item split_channels IMAGE, MODE = 'rgb'

Splits IMAGE onto channels, with the selected MODE, which
currently is C<'rgb'> only. Returns channels as anonymous
array of image objects.

Supported types: RGB

=item histogram IMAGE

Returns anonymous array of 256 integers, each representing
number of pixels with the corresponding value for IMAGE.

Supported types: 8-bit

=back

=head1 REFERENCES

=over

=item *

M.D. Levine. Vision in Man and Machine.  McGraw-Hill, 1985. 

=item *

R. Deriche. Using canny's criteria to derive a recursively implemented optimal edge detector. 
International Journal on Computer Vision, pages 167-187, 1987. 

=item *

R. Boyle and R. Thomas Computer Vision. A First Course, 
Blackwell Scientific Publications, 1988, pp 32 - 34. 

=item *

Image Processing Learning Resources.
L<http://www.dai.ed.ac.uk/HIPR2/hipr_top.htm>

=item *

William K. Pratt.  Digital Image Processing.     
John Wiley, New York, 2nd edition, 1991

=item *

John C. Russ. The Image Processing Handbook.
CRC Press Inc., 2nd Edition, 1995

=item *

L. Vincent & P. Soille.  Watersheds in digital 
spaces:  an efficient algorithm based on immersion
simulations.  IEEE Trans. Patt. Anal. and Mach.
Intell., vol. 13, no. 6, pp. 583-598, 1991

=item *

L. Vincent. Morphological Grayscale Reconstruction in Image Analysis: 
Applications and Efficient Algorithms. 
IEEE Transactions on Image Processing, vol. 2, no. 2, April 1993, pp. 176-201.

=back

=head1 AUTHORS

Anton Berezin E<lt>tobez@tobez.orgE<gt>,
Vadim Belman E<lt>voland@lflat.orgE<gt>,
Dmitry Karasik E<lt>dmitry@karasik.eu.orgE<gt>

=head1 SEE ALSO

=over

=item *

The Prima toolkit L<http://www.prima.eu.org>

=item *

L<iterm> - interactive tool for the IPA library.

=back

=cut

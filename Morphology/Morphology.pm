# $Id$
package IPA::Morphology;
use strict;
use IPA;
use Carp;
require Exporter;
require DynaLoader;

use vars qw(
            $VERSION
            @ISA
            @EXPORT
            @EXPORT_OK
            %EXPORT_TAGS

            $AUTOLOAD

            %transform_luts
           );
@ISA = qw(Exporter DynaLoader);
$VERSION = '0.01';
@EXPORT = qw();
@EXPORT_OK = qw(BWTransform
                dilate erode opening closing
                algebraic_difference gradient
                reconstruct watershed thinning);
%EXPORT_TAGS = (binary => [qw(BWTransform)]);

bootstrap IPA::Morphology $VERSION;

sub opening {
   my $in = shift;
   my $out = erode($in,@_);
   $out = dilate($out,@_);
   $out-> name( "IPA::Morphology::opening");
   return $out;
}

sub closing {
   my $in = shift;
   my $out = dilate($in,@_);
   $out = erode($out,@_);
   $out-> name( "IPA::Morphology::closing");
   return $out;
}

sub gradient {
   my $img = shift;
   my $out = dilate($img,@_);
   my $erode = erode($img,@_);
   return algebraic_difference($out,$erode,inPlace=>'Yes');
}

sub AUTOLOAD {
    my ($subname)=$AUTOLOAD;
    $subname=~s/^IPA::Morphology:://;
    if ($subname=~/^bw_/) {
        my ($bwmethodname)=($subname);
        $bwmethodname=~s/^bw_//;
        if (ref($transform_luts{$bwmethodname}) eq 'CODE') {
            my ($lut)=$transform_luts{$bwmethodname}->();
            print length($lut);
            eval("sub $subname { return BWTransform(\$\_\[0\],lookup=>\'$lut\'); }");
            croak("IPA::Morphology::AUTOLOAD: $@") if $@;
            goto &$subname;
        }
        else {
            croak("IPA::Morphology: internal error - not a code reference in hash for $bwmethodname");
        }
    }
    else {
        croak("IPA::Morphology: unknown method $AUTOLOAD called");
    }
}

sub X0 { return ($_[0] & 0x001 ? 255 : 0); };
sub X1 { return ($_[0] & 0x002 ? 255 : 0); };
sub X2 { return ($_[0] & 0x004 ? 255 : 0); };
sub X3 { return ($_[0] & 0x008 ? 255 : 0); };
sub X4 { return ($_[0] & 0x010 ? 255 : 0); };
sub X5 { return ($_[0] & 0x020 ? 255 : 0); };
sub X6 { return ($_[0] & 0x040 ? 255 : 0); };
sub X7 { return ($_[0] & 0x080 ? 255 : 0); };
sub X8 { return ($_[0] & 0x100 ? 255 : 0); };

%transform_luts=(
                 dilate => sub {
                                my ($rstr)="";
                                my ($i);
                                for ($i=0; $i<512; $i++) {
                                    $rstr.=chr(X0($i) | X1($i) | X2($i) |
                                               X3($i) | X4($i) | X5($i) |
                                               X6($i) | X7($i) | X8($i));
                                }
                                return $rstr;
                           },
                 erode =>  sub {
                                my ($rstr)="";
                                my ($i);
                                for ($i=0; $i<512; $i++) {
                                    $rstr.=chr(X0($i) & X1($i) & X2($i) &
                                               X3($i) & X4($i) & X5($i) &
                                               X6($i) & X7($i) & X8($i));
                                }
                                return $rstr;
                           },
                 isolatedremove =>
                           sub {
                                my ($rstr)="";
                                my ($i);
                                for ($i=0; $i<512; $i++) {
                                    $rstr.=chr(X0($i) & (X1($i) | X2($i) | X3($i) | X4($i) | X5($i) | X6($i) | X7($i) | X8($i)));
                                }
                                return $rstr;
                           },
                 togray => sub {
                                my ($rstr)="";
                                my ($i);
                                for ($i=0; $i<512; $i++) {
                                    $rstr.=chr((X0($i)+X1($i)+X2($i)+X3($i)+X4($i)+X5($i)+X6($i)+X7($i)+X8($i))/9);
                                }
                                return $rstr;
                           },
                 invert => sub {
                    my ($rstr)="";
                    my ($i);
                    for ($i=0; $i<512; $i++) {
                        $rstr.=chr(255-X0($i));
                    }
                    return $rstr;
                 },
                );

1;

# $Id$

package TracedErr;

package DB;
use strict;

sub wrerr
{
    my ( $i, $calltrace) = ( 1, '');
    my @callinfo;
    do {
	@callinfo = map { defined $_ ? $_ : '*undef*'} ( caller( $i++));
	if ( @callinfo > 0) {
	    $calltrace = "!!! $callinfo[ 3](". join( ', ', @DB::args) . ") at $callinfo[ 1]\:$callinfo[ 2]\n" . $calltrace;
	}
    } while ( @callinfo > 0);
    print STDERR $calltrace, @_;
}

$SIG{ __WARN__} = \&wrerr;
$SIG{ __DIE__} = \&wrerr;

1;

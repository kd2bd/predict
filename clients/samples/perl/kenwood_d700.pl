#!/usr/bin/perl
#
# Talk to PREDICT satellite tracking and tune Kenwood D700 Radio
#
# Andrew Rich VK4TEC Aug 2005
#
use Socket;
use Net::Telnet;
use Math::Round;
$frequency = "437975";
my $port = 1210;
my ($predict_server, $satellite);
if ($#ARGV eq 1)
{
$predict_server = $ARGV[0];
$satellite = $ARGV[1];
}
else
{
print "WARNING: Use syntax \"kenwood_d700.pl hostname satellite\"
(i.e., kenwood_d700 localhost ISS)\n";
print "Substituting default arguments\n";
$predict_server = "localhost";
$satellite = "ISS";
}
my ($d1, $d2, $d3, $d4, $rawserver) = gethostbyname($predict_server);
my $serveraddr = pack("Sna4x8", 2, $port, $rawserver);
my $prototype = getprotobyname('udp');
socket(SOCKET,2,SOCK_DGRAM,$prototype) || die("No Socket\n");
$| = 1;  # no buffering
$SIG{ALRM} = \&time_out;
alarm(10);  # Force exit if no response from server
send(SOCKET, "GET_DOPPLER $satellite\0" , 0 , $serveraddr) or die("UDP send
failed $!\n");
my $server_response = '';  # required by recv function
recv(SOCKET, $server_response, 100, 0) or die "UDP recv failed $!\n";
$shift = (($server_response * 4.375) / 1000 );
$newfreq = (int($frequency + $shift));
$rounded = nearest(5,$newfreq) ;
$date = `date +%d/%m/%y" "%H:%M:%S`;
chop $date;
print "DATE: $date \n";
print "SATT: $satellite \n";
print "BASE: $frequency \n";
print "HOST: $predict_server \n";
print "PORT: $port \n";
print "TUNE: $rounded \n";
print "DOPP: $shift \n\n";
open (comport, "> /dev/ttyS0");
print comport "FQ 00".$rounded."000,0\n";
close(comport);
close(SOCKET);
sub time_out
{
die "Server not responding for satellite $satellite\n";
}

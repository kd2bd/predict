#!/usr/bin/perl -w

#############################################################################
#                                                                           #
#  This program is a template that can used for building client programs    #
#  for PREDICT.  This program works by sending a data request to the server #
#  (the machine on which PREDICT is running in server mode), and retrieves  #
#  the response from the server.                                            #
#                                                                           #
#  This program takes two arguments. If none are supplied, a default host   #
#  and satellite are assigned.  The first argument is the hostname of       #
#  the server, and the second is the satellite name.  For example:          #
#                                                                           #
#                    demo localhost OSCAR-10                                #
#                                                                           #
#  If the satellite name contains spaces, enclose the name in "quotes".     #
#                                                                           #
#############################################################################
#                                                                           #
# This program was written by Rich Parry, W9IF on September 18, 2000.       #
# This program is free software; you can redistribute it and/or modify it   #
# under the terms of the GNU General Public License as published by the     #
# Free Software Foundation; either version 2 of the License or any later    #
# version.                                                                  #
#                                                                           #
# This program is distributed in the hope that it will useful, but WITHOUT  #
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     #
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     #
# for more details.                                                         #
#                                                                           #
#############################################################################

use strict;
use Socket;

my $port = 1210;
my ($predict_server, $satellite);

# Use defaults if no command line arguments given
if ($#ARGV eq 1) {
  $predict_server = $ARGV[0];
  $satellite = $ARGV[1];
}
else {
  print "WARNING: Use syntax \"demo.pl hostname satellite\" (i.e., demo.pl localhost FO-20)\n";
  print "Substituting default arguments\n";
  $predict_server = "localhost";
  $satellite = "OSCAR-10";
}

# Setup for UDP socket commumnication
my ($d1, $d2, $d3, $d4, $rawserver) = gethostbyname($predict_server);
my $serveraddr = pack("Sna4x8", 2, $port, $rawserver);
my $prototype = getprotobyname('udp');
socket(SOCKET,2,SOCK_DGRAM,$prototype) || die("No Socket\n");
$| = 1;  # no buffering

# Setup timeout routine
$SIG{ALRM} = \&time_out;
alarm(10);  # Force exit if no response from server

# Send request to predict
send(SOCKET, "GET_SAT $satellite\0" , 0 , $serveraddr) or die("UDP send failed $!\n");

# Get response from predict
my $server_response = '';  # required by recv function
recv(SOCKET, $server_response, 100, 0) or die "UDP recv failed $!\n";

# Extract individual responses
my ($name, $lon, $lat, $az, $el, $aos_seconds, $foot) = split /\n/, $server_response;
my $aos_time_date = gmtime($aos_seconds);

# Output response
print "\nPREDICT returned the following string in response to GET_SAT $satellite:\n\n$server_response\n";
print "Values are as follows:\nName: $name\nLong: $lon\nLat: $lat\nAz: $az\nEl: $el\nNext AOS: $aos_seconds = $aos_time_date UTC\nFoorprint: $foot\n\n";

close(SOCKET);


sub time_out {
  die "Server not responding for satellite $satellite\n";
}

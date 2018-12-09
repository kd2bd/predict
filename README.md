
![PREDICT]
=================================================

PREDICT is an [open-source], multi-user satellite tracking and orbital
prediction program written under the [Linux operating system] by [John A.
Magliacane, KD2BD].  PREDICT is [free software].  Users may redistribute it
and/or modify it under the terms of the [GNU General Public License] as
published by the [Free Software Foundation], either version 2 of the License or
any later version.

[PREDICT]: https://qsl.net/kd2bd/predictlogo.jpg
[open-source]: http://www.opensource.org/
[Linux operating system]: https://qsl.net/kd2bd/linux.html
[John A. Magliacane, KD2BD]: https://qsl.net/kd2bd/index.html
[free software]: https://www.gnu.org/philosophy/free-sw.html
[GNU General Public License]: https://www.gnu.org/licenses/gpl.html
[Free Software Foundation]: https://www.fsf.org/

PREDICT Features
----------------
PREDICT provides real-time satellite tracking and orbital prediction information
to users and client applications in a variety of ways:

*	Through the system console
*	Through the command line
*	Through a network socket
*	Through the generation of audio speech

Data such as a spacecraft's sub-satellite point, azimuth and elevation headings,
Doppler shift, path loss, slant range, orbital altitude, orbital velocity,
footprint diameter, orbital phase (mean anomaly), squint angle, eclipse depth,
the time and date of the next AOS (or LOS of the current pass), orbit number,
and sunlight and visibility information are provided on a real-time basis.
PREDICT can also track (or predict the position of) the Sun and Moon.  PREDICT
also has the ability to control AZ/EL antenna rotators to maintain accurate
orientation in the direction of communication satellites.  As an aid in locating
and tracking satellites through optical means, PREDICT can articulate tracking
coordinates and visibility information as plain speech.


QUICK START GUIDE
=========================

SYSTEM REQUIREMENTS
-------------------
This program requires ncurses version 4.2 or higher.  Earlier versions have
been known to cause segmentation faults and/or odd display colors with this
program.  ncurses may be obtained via:

	https://www.gnu.org/software/ncurses/


The Linux pthreads library is also required.  Both of these libraries are
usually available in modern Linux distributions.


UNPACKING
---------
The PREDICT archive can be unpacked in any directory for which read access
is provided for all system users (such as /usr/src or /usr/local/src for
Slackware Linux).  As root, move the tar file to such a directory and
issue the command:

	tar xvfz predict-2.2.4.tar.gz

to unpack the file.


COMPILATION
-----------
PREDICT's compilation and installation procedure differs slightly from
that of most software, but should easy enough to understand and follow.
First, move (cd) into the predict directory and execute the included
"configure" script by typing:

	./configure

as root at your command prompt.  This script compiles and runs the install
program that configures the source code for PREDICT.  It then compiles and
installs PREDICT by creating symbolic links between the executables created
in the installation directory and /usr/local/bin.  If a destination directory
other than /usr/local/bin is desired, invoke configure with the desired
directory as an argument on the command line:

	./configure /usr/bin

NOTE: "configure" may stall if the system soundcard is in use.  Some
window managers (sometimes KDE) take control of the soundcard, and prevent
the configure script from opening /dev/dsp and checking its existence.


FIRST TIME USE
--------------
First time users will be asked to enter their groundstation latitude and
longitude in decimal degrees or in degree, minute, second (DMS) format.
Normally, PREDICT handles longitudes in decimal degrees WEST (0-360
degrees), and latitudes in decimal degrees NORTH.  This behavior can be
modified by passing the -east or -south command line switches to PREDICT
when it is invoked.  Your station's altitude in meters above mean sea
level, a recent set of Keplerian orbital data for the satellites of
interest, and an accurately set system clock are also required if
successful real-time satellite tracking is also desired.  Sources
for such data include http://www.celestrak.com/,
http://www.space-track.org, and http://www.amsat.org/.


FOR FURTHER INFORMATION
-----------------------
Please consult the files under the "docs" subdirectory for more
directions on the use and capabilities of PREDICT.

The latest news and information regarding PREDICT software is
available at: http://www.qsl.net/kd2bd/predict.html.


Happy Tracking!

--  
John, KD2BD  
kd2bd@amsat.org  
March 2018

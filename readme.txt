
		    **********************************
		    Welcome to PREDICT Version 2.2.1d! 
		    **********************************

PREDICT Version 2.2.1d is a satellite tracking and orbital prediction
program developed by John A. Magliacane, KD2BD.  PREDICT is free software.
You can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 2 of the License or any later version.

PREDICT is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

PREDICT Version 2.2.1d is a derivative of the full-featured Linux
version of PREDICT.  Some of the major features of PREDICT Version
2.2.1d include:

   * Fully year 2000 compliant code.

   * Compiled into highly-optimized 32-bit executable code that operates
     under a protected mode environment during execution to harness the
     full processing power of your (80386 or better) CPU.

   * Licensed under the GNU General Public License, thereby allowing anyone
     to contribute new features to the program while forever keeping the
     software, and any derivative works, non-proprietary and freely
     available.

   * Includes a *FAST* orbital prediction mode that accurately predicts
     satellite passes, providing dates, times, coordinates, slant range
     distances, and sunlight and optical visibility information.  Predictions
     are displayed in tabular form and may be saved to a log file for later
     reference or printing.

   * Includes a visual orbital prediction mode that displays satellite
     passes that are potentially visible to the ground station by optical
     means.

   * Includes a solar illumination prediction mode that calculates how much
     time a satellite will spend in sunlight per day.

   * Features a real-time single satellite tracking mode that provides
     dynamic information such as sub-satellite point, ground station azimuth
     and elevation headings, Doppler shift, path loss, slant range, orbital
     altitude, orbital velocity, footprint diameter, orbital phase, the time
     and date of the next AOS (or LOS of the current pass), orbit number,
     and sunlight and visibility information for a single satellite, while
     providing live azimuth and elevation headings for both the sun and
     moon.

   * Features a multi-satellite tracking mode that provides sub-satellite
     point, azimuth and elevation headings, sunlight and visibility, and
     slant range distance information for all 24 satellites in the program's
     current database on a real-time basis.  Azimuth and elevation headings
     for the sun and moon are also provided, as a well as a listing of the
     AOS dates and times for the next three satellites expected to come into
     range of the ground station.

   * Provides static information such as semi-major axis of ellipse, apogee
     and perigee altitudes, and anomalistic and nodal periods of satellite
     orbits.

   * Includes command line options that permit alternate ground station
     locations to be specified or alternate orbital databases to be read and
     processed by the program, effectively allowing an UNLIMITED number of
     satellites to be tracked and managed.  Additional options allow any
     orbital database file to be automatically updated using NASA Two-Line
     element data obtained via the Internet or via Pacsat satellite without
     having to enter the program and manually select menu options to update
     the database.

   * Includes support for automatic antenna rotator control via a serial
     port using the EasyComm2 protocol standard (9600 baud, 8N1, no
     handshaking).  Interfaces compliant with this standard include the
     PIC/TRACK by Vicenzo Mezzalira (IW3FOL), and TAPR's EasyTrak Jr.
     that is currently under development.  (A DOS serial port driver
     is currently lacking for full functionality of the tracking interface.)
     

PREDICT Version 2.2.1d was compiled under Caldera's DR-DOS 7.02 operating
system using the DJGPP version 2.03 32-bit protected mode software
development environment (gcc 3.2, PDCurses 2.4).

PREDICT Version 2.2.1d is 32-bit protected mode software, and as
such requires a 32-bit processor (80386 or better) along with a
floating-point numeric co-processor to run.  PREDICT Version 2.2.1d
has the ability to emulate a numeric co-processor in environments
where one is lacking.  However, the emulation may not provide results
as accurate as those possible when a hardware co-processor is present.

The protected mode driver (CWSDPMI.EXE) packaged with this software
is required for proper operation of PREDICT, and needs to be installed
either in your PREDICT subdirectory, or in a subdirectory within your
OSs search path.  (Type "path" on your command line to inspect your
search path.)  Users of Windows 9.x and later versions do not need
to install the protected mode driver included with this software.

The latest news and information on PREDICT is available via the
World Wide Web at: http://www.qsl.net/kd2bd/predict.html.

Further information on the free DJGPP software development environment
used to compile this program is available at: http://www.delorie.com/djgpp/

Happy Tracking!

--
73, de John, KD2BD
kd2bd@amsat.org
November 2002

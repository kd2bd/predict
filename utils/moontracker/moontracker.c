/***************************************************************************\
*                  MoonTracker: A Moon AutoTracking Utility                 *
*            Created by John A. Magliacane, KD2BD, on 29-Nov-2002           *
*                          Last update: 04-May-2018                         *
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
* General Public License for more details.                                  *
*                                                                           *
\***************************************************************************/

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

char	callsign[20], output[21];
int	qthalt, pid;
double	qthlat, qthlong, moon_az, moon_el, moonrise,
	PI=3.141592653589793, TWOPI=6.28318530717958623,
	deg2rad=1.745329251994330e-2;

double FixAngle(x)
double x;
{
	/* This function reduces angles greater than
	   two pi by subtracting two pi from the angle */

	while (x>TWOPI)
		x-=TWOPI;

	return x;
}

double PrimeAngle(x)
double x;
{
	x=x-360.0*floor(x/360.0);
	return x;
}

double CurrentDaynum()
{
	/* Read the system clock and return the number
	   of days since 31Dec79 00:00:00 UTC (daynum 0) */

	struct timeval tptr;
	/* int x; */

	/* x=gettimeofday(&tptr,NULL); */
	gettimeofday(&tptr,NULL);

	return ((((double)tptr.tv_sec+0.000001*(double)tptr.tv_usec)/86400.0)-3651.0);
}

char *Daynum2String(daynum)
double daynum;
{
	/* This function takes the given epoch as a fractional number of
	   days since 31Dec79 00:00:00 UTC and returns the corresponding
	   date as a string of the form "Tue 12Oct99 17:22:37". */

	char timestr[25];
	time_t t;
	int x;

	/* Convert daynum to Unix time (seconds since 01-Jan-70) */
	t=(time_t)rint(86400.0*(daynum+3651.0));

	sprintf(timestr,"%s",asctime(gmtime(&t)));

	if (timestr[8]==' ')
		timestr[8]='0';

	for (x=0; x<=3; output[x]=timestr[x], x++);

	output[4]=timestr[8];
	output[5]=timestr[9];
	output[6]=timestr[4];
	output[7]=timestr[5];
	output[8]=timestr[6];
	output[9]=timestr[22];
	output[10]=timestr[23];
	output[11]=' ';

	for (x=12; x<=19; output[x]=timestr[x-1], x++);

	output[20]=0;
	return output;
}

char ReadQTH(qthfile)
char *qthfile;
{
	/* This function reads the tracking station's latitude
	   and longitude information from PREDICT's predict.qth file. */

	FILE *fd;
	char success=0;

	fd=fopen(qthfile,"r");
	
	if (fd!=NULL)
	{
		fgets(callsign,16,fd);
		callsign[strlen(callsign)-1]=0;
		fscanf(fd,"%lf", &qthlat);
		fscanf(fd,"%lf", &qthlong);
		fscanf(fd,"%d", &qthalt);
		fclose(fd);

		success=1;
	}

	return success;
}

void TrackDataOut(antfd, elevation, azimuth)
int antfd;
double elevation, azimuth;
{
	/* This function sends Azimuth and Elevation data
	   to an antenna tracker connected to the serial port */

	int n, port;
	char message[30]="\n";

	port=antfd;

	sprintf(message, "AZ%3.1f EL%3.1f \x0D\x0A", azimuth,elevation);
	n=write(port,message,30);

	if (n<0)
	{
		fprintf(stderr,"%c*** Error: Problem Writing To Antenna Port!\n",7);		kill(pid,1);
		exit(-1);
	}
}

void FindMoon(daynum)
double daynum;
{
	/* This function determines the azimuth and elevation headings
	   for the moon, relative to the latitude and longitude of
	   the tracking station.  This code was derived from a
	   Javascript implementation of the Meeus method for
	   determining the exact position of the Moon found at:
	   http://www.geocities.com/s_perona/ingles/poslun.htm. */

	double	jd, s, t, t2, t3, d, f, l1, m, m1, ex, om, l,
		b, w1, w2, bt, p, lm, h, ra, dec, z, ob, n,
		el, az, teg, th;

	jd=daynum+2444238.5;

	t=(jd-2415020.0)/36525.0;
	t2=t*t;
	t3=t2*t;
	l1=270.434164+481267.8831*t-0.001133*t2+0.0000019*t3;
	m=358.475833+35999.0498*t-0.00015*t2-0.0000033*t3;
	m1=296.104608+477198.8491*t+0.009192*t2+0.0000144*t3;
	d=350.737486+445267.1142*t-0.001436*t2+0.0000019*t3;
	f=11.250889+483202.0251*t-0.003211*t2-0.0000003*t3;
	om=259.183275-1934.142*t+0.002078*t2+0.0000022*t3;
	om=om*deg2rad;

	/* Additive terms */

	l1=l1+0.000233*sin((51.2+20.2*t)*deg2rad);
	s=0.003964*sin((346.56+132.87*t-0.0091731*t2)*deg2rad);
	l1=l1+s+0.001964*sin(om);
	m=m-0.001778*sin((51.2+20.2*t)*deg2rad);
	m1=m1+0.000817*sin((51.2+20.2*t)*deg2rad);
	m1=m1+s+0.002541*sin(om);
	d=d+0.002011*sin((51.2+20.2*t)*deg2rad);
	d=d+s+0.001964*sin(om);
	f=f+s-0.024691*sin(om);
	f=f-0.004328*sin(om+(275.05-2.3*t)*deg2rad);
	ex=1.0-0.002495*t-0.00000752*t2;
	om=om*deg2rad;

	l1=PrimeAngle(l1);
	m=PrimeAngle(m);
	m1=PrimeAngle(m1);
	d=PrimeAngle(d);
	f=PrimeAngle(f);
	om=PrimeAngle(om);

	m=m*deg2rad;
	m1=m1*deg2rad;
	d=d*deg2rad;
	f=f*deg2rad;

	/* Ecliptic Longitude */

	l=l1+6.28875*sin(m1)+1.274018*sin(2.0*d-m1)+0.658309*sin(2.0*d);
	l=l+0.213616*sin(2.0*m1)-ex*0.185596*sin(m)-0.114336*sin(2.0*f);
	l=l+0.058793*sin(2.0*d-2.0*m1)+ex*0.057212*sin(2.0*d-m-m1)+0.05332*sin(2.0*d+m1);
	l=l+ex*0.045874*sin(2.0*d-m)+ex*0.041024*sin(m1-m)-0.034718*sin(d);
	l=l-ex*0.030465*sin(m+m1)+0.015326*sin(2.0*d-2.0*f)-0.012528*sin(2.0*f+m1);
	
	l=l-0.01098*sin(2.0*f-m1)+0.010674*sin(4.0*d-m1)+0.010034*sin(3.0*m1);
	l=l+0.008548*sin(4.0*d-2.0*m1)-ex*0.00791*sin(m-m1+2.0*d)-ex*0.006783*sin(2.0*d+m);
	
	l=l+0.005162*sin(m1-d)+ex*0.005*sin(m+d)+ex*0.004049*sin(m1-m+2.0*d);
	l=l+0.003996*sin(2.0*m1+2.0*d)+0.003862*sin(4.0*d)+0.003665*sin(2.0*d-3.0*m1);

	l=l+ex*0.002695*sin(2.0*m1-m)+0.002602*sin(m1-2.0*f-2.0*d)+ex*0.002396*sin(2.0*d-m-2.0*m1);

	l=l-0.002349*sin(m1+d)+ex*ex*0.002249*sin(2.0*d-2.0*m)-ex*0.002125*sin(2.0*m1+m);

	l=l-ex*ex*0.002079*sin(2.0*m)+ex*ex*0.002059*sin(2.0*d-m1-2.0*m)-0.001773*sin(m1+2.0*d-2.0*f);

	l=l+ex*0.00122*sin(4.0*d-m-m1)-0.00111*sin(2.0*m1+2.0*f)+0.000892*sin(m1-3.0*d);

	l=l-ex*0.000811*sin(m+m1+2.0*d)+ex*0.000761*sin(4.0*d-m-2.0*m1)+ex*ex*.000717*sin(m1-2.0*m);

	l=l+ex*ex*0.000704*sin(m1-2.0*m-2.0*d)+ex*0.000693*sin(m-2.0*m1+2.0*d)+ex*0.000598*sin(2.0*d-m-2.0*f)+0.00055*sin(m1+4.0*d);

	l=l+0.000538*sin(4.0*m1)+ex*0.000521*sin(4.0*d-m)+0.000486*sin(2.0*m1-d);

	l=l-0.001595*sin(2.0*f+2.0*d);

	/* Ecliptic latitude */

	b=5.128189*sin(f)+0.280606*sin(m1+f)+0.277693*sin(m1-f)+0.173238*sin(2.0*d-f);
	b=b+0.055413*sin(2.0*d+f-m1)+0.046272*sin(2.0*d-f-m1)+0.032573*sin(2.0*d+f);

	b=b+0.017198*sin(2.0*m1+f)+9.266999e-03*sin(2.0*d+m1-f)+0.008823*sin(2.0*m1-f);
	b=b+ex*0.008247*sin(2.0*d-m-f)+0.004323*sin(2.0*d-f-2.0*m1)+0.0042*sin(2.0*d+f+m1);

	b=b+ex*0.003372*sin(f-m-2.0*d)+ex*0.002472*sin(2.0*d+f-m-m1)+ex*0.002222*sin(2.0*d+f-m);

	b=b+0.002072*sin(2.0*d-f-m-m1)+ex*0.001877*sin(f-m+m1)+0.001828*sin(4.0*d-f-m1);

	b=b-ex*0.001803*sin(f+m)-0.00175*sin(3.0*f)+ex*0.00157*sin(m1-m-f)-0.001487*sin(f+d)-ex*0.001481*sin(f+m+m1)+ex*0.001417*sin(f-m-m1)+ex*0.00135*sin(f-m)+0.00133*sin(f-d);

	b=b+0.001106*sin(f+3.0*m1)+0.00102*sin(4.0*d-f)+0.000833*sin(f+4.0*d-m1);

	b=b+0.000781*sin(m1-3.0*f)+0.00067*sin(f+4.0*d-2.0*m1)+0.000606*sin(2.0*d-3.0*f);

	b=b+0.000597*sin(2.0*d+2.0*m1-f)+ex*0.000492*sin(2.0*d+m1-m-f)+0.00045*sin(2.0*m1-f-2.0*d);

	b=b+0.000439*sin(3.0*m1-f)+0.000423*sin(f+2.0*d+2.0*m1)+0.000422*sin(2.0*d-f-3.0*m1);

	b=b-ex*0.000367*sin(m+f+2.0*d-m1)-ex*0.000353*sin(m+f+2.0*d)+0.000331*sin(f+4.0*d);

	b=b+ex*0.000317*sin(2.0*d+f-m+m1)+ex*ex*0.000306*sin(2.0*d-2.0*m-f)-0.000283*sin(m1+3.0*f);

	w1=0.0004664*cos(om*deg2rad);
	w2=0.0000754*cos((om+275.05-2.3*t)*deg2rad);
	bt=b*(1.0-w1-w2);

	/* Parallax calculations */

	p=0.950724+0.051818*cos(m1)+0.009531*cos(2.0*d-m1)+0.007843*cos(2.0*d)+0.002824*cos(2.0*m1)+0.000857*cos(2.0*d+m1)+ex*0.000533*cos(2.0*d-m)+ex*0.000401*cos(2.0*d-m-m1);

	p=p+0.000173*cos(3.0*m1)+0.000167*cos(4.0*d-m1)-ex*0.000111*cos(m)+0.000103*cos(4.0*d-2.0*m1)-0.000084*cos(2.0*m1-2.0*d)-ex*0.000083*cos(2.0*d+m)+0.000079*cos(2.0*d+2.0*m1);

	p=p+0.000072*cos(4.0*d)+ex*0.000064*cos(2.0*d-m+m1)-ex*0.000063*cos(2.0*d+m-m1);

	p=p+ex*0.000041*cos(m+d)+ex*0.000035*cos(2.0*m1-m)-0.000033*cos(3.0*m1-2.0*d);

	p=p-0.00003*cos(m1+d)-0.000029*cos(2.0*f-2.0*d)-ex*0.000029*cos(2.0*m1+m);

	p=p+ex*ex*0.000026*cos(2.0*d-2.0*m)-0.000023*cos(2.0*f-2.0*d+m1)+ex*0.000019*cos(4.0*d-m-m1);

	b=bt*deg2rad;
	lm=l*deg2rad;

	/* Convert ecliptic coordinates to equatorial coordinates */

	z=(jd-2415020.5)/365.2422;
	ob=23.452294-(0.46845*z+5.9e-07*z*z)/3600.0;
	ob=ob*deg2rad;
	dec=asin(sin(b)*cos(ob)+cos(b)*sin(ob)*sin(lm));
	ra=acos(cos(b)*cos(lm)/cos(dec));

	if (lm>PI)
		ra=TWOPI-ra;

	/* ra = right ascension */
	/* dec = declination */

	n=qthlat*deg2rad;    /* North latitude of tracking station */
	/* e=-qthlong*deg2rad; */  /* East longitude of tracking station */

	/* Find siderial time in radians */

	t=(jd-2451545.0)/36525.0;
	teg=280.46061837+360.98564736629*(jd-2451545.0)+(0.000387933*t-t*t/38710000.0)*t;

	while (teg>360.0)
		teg-=360.0;

	th=FixAngle((teg-qthlong)*deg2rad);
	h=th-ra;

	/* Determine local coordinates */

	az=atan2(sin(h),cos(h)*sin(n)-tan(dec)*cos(n))+PI;
	el=asin(sin(n)*sin(dec)+cos(n)*cos(dec)*cos(h));

	moon_az=az/deg2rad;
	moon_el=el/deg2rad;
}

double FindMoonRise()
{
	/* This function finds the daynum of moonrise. */

	double daynum2;

	daynum2=CurrentDaynum();
	FindMoon(daynum2);
	moonrise=0.0;

	while (moonrise==0.0)
	{
		if (fabs(moon_el)<0.03)
			moonrise=daynum2;
		else
		{
			daynum2-=(0.003*moon_el);
			FindMoon(daynum2);
		}
	}

	return moonrise;
}

int main(argc,argv)
char argc, *argv[];
{
	char qthfile[80], serial_port[15], once_per_second=0, *env=NULL;
	int x, y, z, antfd=-1, iel, iaz, oldaz=0, oldel=0;
	unsigned sleeptime=0;
	double daynum;
	struct termios oldtty, newtty;

	if (argc==1)
	{
		fprintf(stderr,"\n\t\t    --==[ MoonTracker v1.1 by KD2BD ]==--\n");
		fprintf(stderr,"\n\t\t\t   ...Available options...\n\n");
		fprintf(stderr,"\t-a serial port (/dev/ttyS0) -- sends data as it changes\n");
		fprintf(stderr,"\t-a1 serial port (/dev/ttyS0) -- sends data at one-second intervals\n");
		fprintf(stderr,"\t-q alternate qth file (site.qth)\n");
		fprintf(stderr,"\nOnce started, MoonTracker spawns itself as a background process.\n\n"); 
		exit(0);
	}

	y=argc-1;
	serial_port[0]=0;
	env=getenv("HOME");
	sprintf(qthfile,"%s/.predict/predict.qth",env);

	/* Interpret command-line options */

	for (x=1; x<=y; x++)
	{
		if (strcmp(argv[x],"-q")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(qthfile,argv[z],78);
		}

		if (strcmp(argv[x],"-a")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(serial_port,argv[z],13);
		}

		if (strcmp(argv[x],"-a1")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
			{
				strncpy(serial_port,argv[z],13);
				once_per_second=1;
			}
		}
	}

	if (serial_port[0]==0)
	{
		fprintf(stderr,"%c*** Error: Serial port not specified!\n",7);
		exit(-1);
	}

	antfd=open(serial_port, O_RDWR|O_NOCTTY);

	if (antfd!=-1)
	{
		/* Set up serial port */

		tcgetattr(antfd, &oldtty);
		bzero(&newtty, sizeof(newtty));

		/* 9600 baud, 8-bits, no parity,
		   1-stop bit, no handshaking */

		newtty.c_cflag=B9600|CS8|CLOCAL;
		newtty.c_oflag=0;
		newtty.c_lflag=0;

		tcflush(antfd, TCIFLUSH);
		tcsetattr(antfd, TCSANOW, &newtty);
	}

	else
	{
		fprintf(stderr, "%c*** Error: Unable To Open Antenna Port\n",7);
		exit(-1);
	}

	if (ReadQTH(qthfile)==0)
	{
		fprintf(stderr,"%c*** Error: QTH file \"%s\" could not be loaded!\n",7,qthfile);
		exit(-1);
	}

	moonrise=FindMoonRise();
	daynum=CurrentDaynum();

	if (!(pid=fork()))
	{
		while (1)
		{
			daynum=CurrentDaynum();
			FindMoon(daynum);

			if (moon_el<0.0)
			{
				/* Go to sleep until moon rise. */

				moonrise=FindMoonRise();

				if (daynum<moonrise)
				{
					sleeptime=(unsigned)(86400.0*(moonrise-daynum));
					sleep(sleeptime);
				}
			}

			if (moon_el>=0.0 && antfd!=-1)
			{
				iaz=(int)rint(moon_az);
				iel=(int)rint(moon_el);

				if ((oldel!=iel || oldaz!=iaz) || (once_per_second))
				{
					TrackDataOut(antfd,(float)iel,(float)iaz);
					oldel=iel;
					oldaz=iaz;
				}
			}

			if (once_per_second)
				sleep (1);
			else
				sleep (60);
		}
	}

	else
	{
		fprintf(stderr,"MoonTracker (pid %d) running on %s!\n",pid,serial_port);
		if (daynum<moonrise)
			fprintf(stderr,"MoonTracker will sleep until moonrise at: %s UTC\n",Daynum2String(moonrise));

		exit(0);
	}
}

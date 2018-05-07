/***************************************************************************\
*                                                                           *
*  EARTHTRACK:  A Real-Time Satellite Tracking Display Client for PREDICT   *
*    Original concept by Wade Hampton.  This implimentation was written     *
*    by John A. Magliacane, KD2BD in November 2000.  The -x switch code     *
*           was contributed by Tom Busch, WB8WOR in October 2001.           *
*                                                                           *
*         To build: "cc -Wall -O3 -s -lm earthtrack -o earthtrack"          *
*        then:  "ln -s earthtrack earthtrack2".  Invoke earthtrack          *
*       to run with xearth.  Invoke earthtrack2 to run with xplanet         *
*                           version 1.0 or above.                           *
*                                                                           *
*****************************************************************************
*                                                                           *
*           This code was last modified by KD2BD on 04-May-2018.            *
*                                                                           *
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     *
* for more details.                                                         *
*                                                                           *
\***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

#ifndef PI
#define PI 3.141592653589793
#endif

char	string[625], error;
double	R0=6378.16, TWOPI=6.283185307179586,
	HALFPI=1.570796326794896, deg2rad=1.74532925199e-02;
FILE	*markerfd=NULL, *greatarcfd=NULL;

void handler()
{
	/* This is a function that is called when the response function
	   times out.  This is in case the server fails to respond. */

	signal(SIGALRM,handler);
}

int connectsock(char *host, char *service, char *protocol)
{
	/* This function is used to connect to the server.  "host" is the
	   name of the computer on which PREDICT is running in server mode.
	   "service" is the name of the socket port.  "protocol" is the
	   socket protocol (UDP). */

	struct hostent *phe;
	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	
	int s, type;
	
	memset((char *)&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family=AF_INET;
	
	if ((pse=getservbyname(service,protocol)))
		sin.sin_port=pse->s_port;

	else if ((sin.sin_port=htons((unsigned short)atoi(service)))==0)
	{
		printf("Can't get services\n");
		return -1;
	}

	if ((phe=gethostbyname(host)))
		bcopy(phe->h_addr,(char *)&sin.sin_addr,phe->h_length);

	else if ((sin.sin_addr.s_addr=inet_addr(host))==INADDR_NONE)
	{
		printf("Can't get host: %c%s%c!\n",34,host,34);
		return -1;
	}
	
	if ((ppe=getprotobyname(protocol))==0)
		return -1;

	if (strcmp(protocol,"udp")==0)
		type=SOCK_DGRAM;
	else
		type=SOCK_STREAM;
	
	s=socket(PF_INET,type,ppe->p_proto);

	if (s<0)
	{
		printf("Can't get socket.\n");
		return -1;
	}

	if (connect(s,(struct sockaddr *)&sin,sizeof(sin))<0)
	{
		printf("Can't connect to socket.\n");
		return -1;
	}

	return s;
}

void get_response(int sock, char *buf)
{
	/* This function gets a response from the
	   server in the form of a character string. */

	int n;

	n=read(sock,buf,625);

	if (n<0)
	{
		if (errno==EINTR)
			return;

		if (errno==ECONNREFUSED)
		{
			fprintf(stderr, "Connection refused - PREDICT server not running\n");
			exit (1);
		}
	}

	buf[n]='\0';
}

char *send_command(host, command)
char *host, *command;
{
	int sk;

	/* This function sends "command" to PREDICT running on
	   machine "host", and returns the result of the command
	   as a pointer to a character string. */

	/* Open a network socket */
	sk=connectsock(host,"predict","udp");

	if (sk<0)
	{
		error=1;
		string[0]=0;
	}

	else
	{
		/* Build a command buffer */
		sprintf(string,"%s\n",command);

		/* Send the command to the server */
   		write(sk,command,strlen(string));

		/* clear string[] so it can be re-used for the response */
		string[0]=0;

		/* Get the response */
   		get_response(sk,string);

		/* Close the connection */
   		close(sk);
	}

	return string;
}

double arccos(x,y)
double x, y;
{
	/* This function implements the arccosine function,
	   returning a value between 0 and two pi. */

	double result=0.0;

	if (y>0.0)
		result=acos(x/y);

	if (y<0.0)
		result=PI+acos(x/y);

	return result;
}

double convertlong(longitude)
double longitude;
{
	/* This function converts west longitudes (0-360 degrees)
	   to a value between -180 and 180 degrees, as required
	   by xearth and xplanet. */

	if (longitude<180.0)
		longitude=-longitude;

	else if (longitude>180.0)
		longitude=360.0-longitude;

	return longitude;
}

char *vis2color(visibility)
int visibility;
{
	/* This function generates a character string based on the
	   visibility information returned by PREDICT.  It is used
	   to set the colors of the markers and range circles
	   plotted by xearth and xplanet. */ 

	switch (visibility)
	{
		case 'D':
		strcpy(string,"color=white");
		break;

		case 'N':
		strcpy(string,"color=blue");
		break;

		case 'V':
		strcpy(string,"color=yellow");
		break;
	}

	return(string);
}

void rangecircle(ssplat,ssplong,footprint,visibility)
double ssplat, ssplong, footprint;
char visibility;
{
	/* This function generates a sequence of latitude and
	   longitude positions used to plot range circles of
	   satellites based on the spacecraft's sub-satellite
	   latitude, longitude, and footprint.  The visibility
	   information is used to set the range circle to an
	   appropriate color.  Output is written to ~/.greatarcfile,
	   and read and processed by xplanet. */

	double beta, den, num, rangelat, rangelong, azimuth;
	int azi;

	ssplat=ssplat*deg2rad;
	ssplong=ssplong*deg2rad;
	beta=(0.5*footprint)/R0;

	for (azi=0; azi<360; azi++)
	{
		azimuth=deg2rad*(double)azi;
		rangelat=asin(sin(ssplat)*cos(beta)+cos(azimuth)*sin(beta)*cos(ssplat));
		num=cos(beta)-(sin(ssplat)*sin(rangelat));
		den=cos(ssplat)*cos(rangelat);

		if (azi==0 && (beta > HALFPI-ssplat))
			rangelong=ssplong+PI;

		else if (azi==180 && (beta > HALFPI+ssplat))
			rangelong=ssplong+PI;

		else if (fabs(num/den)>1.0)
			rangelong=ssplong;

		else
		{
			if ((180-azi)>=0)
				rangelong=ssplong-arccos(num,den);
			else
				rangelong=ssplong+arccos(num,den);
		}
	
		while (rangelong<0.0)
			rangelong+=TWOPI;

		while (rangelong>TWOPI)
			rangelong-=TWOPI;
 
		rangelat=rangelat/deg2rad;
		rangelong=rangelong/deg2rad;

		rangelong=convertlong(rangelong);

		/* Write range circle data to greatarcfile */

		fprintf(greatarcfd,"%8.3f %8.3f ",rangelat,rangelong);

		if (azi%2)
			fprintf(greatarcfd,"%s\n",vis2color(visibility));
	}
}

char AppendFile(source, destination)
char *source;
char *destination;
{
	/* This function appends file "source" to file "destination"
	   in 64k chunks.  A 1 is returned if there is a problem writing
	   to the destination file.  A 2 indicates a problem reading
	   the source file.  */

	int x, sd, dd;
	char error=0, buffer[65536];

	sd=open(source,O_RDONLY);

	if (sd!=-1)
	{
		dd=open(destination,O_WRONLY | O_CREAT | O_APPEND, 0644);

		if (dd!=-1)
		{
			x=read(sd,&buffer,65536);

			while (x)
			{
				write(dd,&buffer,x);
				x=read(sd,&buffer,65536);
			}

			close(dd);
		}
		else
			error=1;

		close(sd);
	}
	else
		error+=2;

	return error;
}

int main(argc,argv)
char argc, *argv[];
{
	int x, y, z, updateinterval, satnum=0, radius=50, sleeptime=20;
	char buf[128], command[255], satlist[625], /* satname[26], */
	     satnamelist[26][26], callsign[15], markerfile[50],
	     greatarcfile[50], configfile[50], sat2track[30],
	     hostname[50], color[35], xtra[50], xplanet=0,
	     xearth=0, visibility, circledrawn=0, zoom=0;
	float az, el, slat, slong, qthlat,
	      qthlong, footprint, range, altitude,
	      velocity, mapcenterlat, mapcenterlong;
	long next_event_time, orbitnum, starttime, endtime, current_time;
	time_t t;
	FILE *fd=NULL;

	/* Parse command-line arguments */

	z=strlen(argv[0]);

	for (x=z-10, y=0; x<z; x++, y++)
		buf[y]=argv[0][x];

	buf[y]=0;

	if (strncmp(buf,"arthtrack2",10)==0)
		xplanet=1;

	if (strncmp(buf,"earthtrack",10)==0)
		xearth=1;

	y=argc-1;
	xtra[0]=0;
	hostname[0]=0;
	sat2track[0]=0;
	updateinterval=0;

	for (x=1; x<=y; x++)
	{
		if (strcmp(argv[x],"-h")==0)
		{
			/* Hostname */

			z=x+1;

			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(hostname,argv[z],50);
		}

		if (strcmp(argv[x],"-c")==0 || strcmp(argv[x],"-C")==0)
		{
			/* Center map on this satellite */

			z=x+1;

			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(sat2track,argv[z],30);

			if (argv[x][1]=='C')
				zoom=1;
		}

		if (strcmp(argv[x],"-u")==0)
		{
			/* Screen update interval */

			z=x+1;

			if (z<=y && argv[z][0] && argv[z][0]!='-')
				sscanf(argv[z],"%d",&updateinterval);
		}

		if (strcmp(argv[x],"-x")==0)
		{
			/* Extra commands to pass to xplanet or xearth */

			z=x+1;

			if (z<=y && argv[z][0])
				strncpy(xtra,argv[z],50);
		}
	}

	/* Set some reasonable defaults */

	if (hostname[0]==0)
		strcpy(hostname,"localhost");

	if (updateinterval<5)
		updateinterval=20;

	if (updateinterval>120)
		updateinterval=20;

	error=0;  /* Another reasonable default :-) */

	/* Write xearth/xplanet working files
	   to the user's home directory */

	sprintf(markerfile,"%s/.markerfile",getenv("HOME"));
	sprintf(greatarcfile,"%s/.greatarcfile",getenv("HOME"));
	sprintf(configfile,"%s/.xplanetconfig",getenv("HOME"));

	if (xplanet)
	{
		fd=fopen(configfile,"w");
		fprintf(fd,"## \"earthtrack2\" parameters.  Please edit\n");
		fprintf(fd,"## earthtrack.c to modify these parameters!\n\n");

		/* Edit the following lines to */
		/* modify your .xplanetconfig file */

		fprintf(fd,"marker_file=%s\n",markerfile);
		fprintf(fd,"arc_file=%s\n",greatarcfile);
		fprintf(fd,"marker_font=/usr/local/share/xplanet/fonts/FreeMonoBold.ttf\n");
		fprintf(fd,"\n## Your \"/usr/local/share/xplanet/config/default\" configuration file follows:\n\n");
		fclose(fd);

		x=AppendFile("/usr/local/share/xplanet/config/default",configfile);

		if (x==1)
		{
			fprintf(stderr,"%c*** Error: Could not write to %s\n",7,configfile);
			exit(-1);
		}

		if (x==2)
		{
			fprintf(stderr,"%c*** Error: Could not read \"/usr/local/share/xplanet/config/default\"\nIs xplanet installed?\n\n",7);
			exit(-1);
		}
	}

	/* Get QTH info from PREDICT */

	strcpy(buf, send_command(hostname,"GET_QTH"));
	sscanf(buf,"%s%f%f",callsign,&qthlat,&qthlong);

	qthlong=convertlong(qthlong);

	mapcenterlat=qthlat;
	mapcenterlong=qthlong;

	/* Get the list of satellite names from PREDICT */

	strcpy(satlist, send_command(hostname,"GET_LIST"));

	if (satlist[0]!=0)
	{
		/* Parse the response of GET_LIST, and place each
		   name in the character array satnamelist[]. */

		for (x=0, y=0, z=0; y<strlen(satlist); y++)
		{
			if (satlist[y]!='\n')
			{
				satnamelist[z][x]=satlist[y];
				x++;
			}

			else
			{
				satnamelist[z][x]=0;
				z++;
				x=0;
			}
		}

		satnum=z;
	}

	else
		error=1;

	while (error==0)
	{
		/* Main program loop.  Continue until either
		   PREDICT or this process is terminated. */

		markerfd=fopen(markerfile,"w");
		greatarcfd=fopen(greatarcfile,"w");

		fprintf(markerfd,"%8.3f %8.3f %c%s%c\n",qthlat,qthlong,34,callsign,34);

		for (z=0; z<satnum; z++)
		{
			sprintf(command,"GET_SAT %s",satnamelist[z]);
			strcpy(buf,send_command(hostname,command));

			if (buf[0]==0)
				error=1;

			/* The first element of the response is the satellite
			   name.  It is ended with a '\n' character and may
			   contain spaces. */


			/**
			for (x=0; buf[x]!='\n'; x++)
				satname[x]=buf[x];

			satname[x]=0
			**/

			for (x=0; buf[x]!='\n'; x++);

			x++;

			/* The rest of the data from GET_SAT is numerical,
		 	   and can be parsed using the sscanf() function.
			   First, the satellite name is removed from "buf",
			   and then "buf" is parsed for numerical data
			   using an sscanf(). */

			for (y=0; buf[x+y]!=0; y++)
				buf[y]=buf[x+y];

			buf[y]=0;
			color[0]=0;
 
			sscanf(buf,"%f %f %f %f %ld %f %f %f %f %ld %c",&slong,&slat,&az,&el,&next_event_time,&footprint,&range,&altitude,&velocity,&orbitnum,&visibility);

			if (strcmp(satnamelist[z],sat2track)==0 && range>0.0)
			{
				mapcenterlat=slat;
				mapcenterlong=convertlong(slong);
				rangecircle(slat,slong,footprint,visibility);
				circledrawn=1;

				if (zoom)
					radius=(int)(100.0*(R0/footprint));
				else
					radius=50;

				if (radius<50)
					radius=50;
			}

			if (range>0.0)
			{
				if (xplanet)
					strcpy(color,vis2color(visibility));
				else
					color[0]=0;

				fprintf(markerfd,"%8.3f %8.3f %c%s%c %s\n",slat,convertlong(slong),34,satnamelist[z],34,color);


				/* Get current time from PREDICT server */

				strcpy(buf, send_command(hostname,"GET_TIME"));
				sscanf(buf,"%ld",&current_time);

				/* Draw range circle if satellite is in range,
				   or will be in range within 5 minutes. */

				if (xplanet && zoom==0 && (el>=0.0 || ((next_event_time-current_time)<300)))
				{
					rangecircle(slat,slong,footprint,visibility);
					circledrawn=1;
				}
			}
		}

		fclose(greatarcfd);
		fclose(markerfd);

		if (error==0)
		{
			t=time(NULL);
			starttime=(long)t;

			if (xearth)
				sprintf(command,"xearth -proj orth -grid -night 30 -bigstars 40 -markerfile %s -pos %cfixed %f %f%c %s -once",markerfile,34,mapcenterlat,mapcenterlong,34,xtra);

			if (xplanet)
			{
				if (circledrawn)
					sprintf(command,"xplanet -config %s -projection orth -longitude %f -latitude %f -radius %d -num_times 1 -starfreq 0.005 %s",configfile,mapcenterlong,mapcenterlat,radius,xtra);
				else
					sprintf(command,"xplanet -config %s -projection orth -latitude %f -longitude %f -num_times 1 %s -starfreq 0.005",configfile,mapcenterlat,mapcenterlong,xtra);
			}

			if (xplanet|xearth)
			{
				error=system(command);
				t=time(NULL);
				endtime=(long)t;

				sleeptime=updateinterval-(int)(endtime-starttime);

				if (sleeptime>0)
					sleep(sleeptime);
			}
		}
	}

	if (error)
	{
		unlink(markerfile);

		if (xplanet)
		{
			unlink(greatarcfile);
			unlink(configfile);
		}
	}

	exit(0);
}

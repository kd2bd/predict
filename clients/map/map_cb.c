/****************************************************************************
*          MAP: A satellite tracking/orbital display program                *
*               Copyright Ivan J. Galysh, KD4HBO 2000                       *
*                      Project started: 18-Dec-1999                         *
*                        Last update: 05-Nov-2001                           *
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
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include "forms.h"
#include "map.h"

#ifndef PI
#define PI 3.141592653589793
#endif

double arccos(x,y)
double x, y;
{
	/* This function implements the arccosine function,
	   returning a value between 0 and two pi. */

	double result, fraction;

	fraction=x/y;

	if ((x>0.0) && (y>0.0))
		result=acos(fraction);

	if ((x<0.0) && (y>0.0))
		result=acos(fraction);

	if ((x<0.0) && (y<0.0))
		result=PI+acos(fraction);

	if ((x>0.0) && (y<0.0))
		result=PI+acos(fraction);

	return result;
}


void handler()
{
	signal(SIGALRM,handler);
}

int connectsock(char *host, char *service, char *protocol)
{
	struct hostent *phe;
	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	
	int s, type;
	
	bzero((char *)&sin,sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	
	if (pse=getservbyname(service,protocol))
		sin.sin_port=pse->s_port;

	else if ((sin.sin_port=htons((unsigned short)atoi(service)))==0)
	{
	   printf("Can't get services\n");
	   return -1;
	}

	if (phe=gethostbyname(host))
		bcopy(phe->h_addr,(char *)&sin.sin_addr,phe->h_length);

	else if ((sin.sin_addr.s_addr = inet_addr(host))==INADDR_NONE)
	{
	   printf("Can't get host.\n");
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
	int n;

	alarm(10); 
	n=read(sock,buf,625);

	if (errno==EINTR)
	{
		buf[0] = '\0';
		return;
	}

	if (n<0)
	{
	   alarm(0); 
	   return;
	}
	
	buf[n]='\0';
	alarm(0); 
}


/* callbacks and freeobj handles for form map */
void j(FL_OBJECT *ob, long data)
{
	float latitude, longitude, az, el, footprint;
	char lats[32], longs[32], azs[32], els[32], s_aos[32],
	     buf[64], timebuf[30], satname[32], timestr[26];
	long aos, *ptr_aos;
   	int x, y, xx, yy, sk, azi;
	struct tm gmt, *ptr_gmt;

	double rangelat, rangelong, azimuth, ssplat, ssplong, TWOPI,
	       HALFPI, deg2rad=1.74532925199e-02, R0=6378.16, beta, num, dem;

	TWOPI=2.0*PI;
	HALFPI=PI/2.0;

   	sk=connectsock(servername,"predict","udp");

   	if (sk<0)
		return;

	sprintf(buf,"GET_SAT %s\n",satnum);
   	write(sk,buf,strlen(buf));
   	get_response(sk,buf);
   	close(sk);
	ptr_aos=&aos;
	ptr_gmt=&gmt;
		
	/* Parse the response.  The first element is the satellite name.
	   It is ended with a '\n' character and many contain spaces. */

	for (x=0; buf[x]!='\n' && buf[x]!=0; x++)
		satname[x]=buf[x];

	satname[x]=0;
	x++;

	/* The rest of the data can be parsed using the sscanf()
	   function.  First, the satellite name is removed from
	   "buf", and then "buf" is parsed for numerical data
	   using an sscanf(). */

	for (y=0; buf[x+y]!=0; y++)
		buf[y]=buf[x+y];

		buf[y]=0;
 
	sscanf(buf,"%f %f %f %f %ld %f",&longitude,&latitude,&az,&el,&aos,&footprint);
	sscanf(buf,"%s %s %s %s %s",longs,lats,azs,els,s_aos);

	ptr_gmt=gmtime(ptr_aos);
	fl_set_object_label(fdmap->b,azs);
	fl_set_object_label(fdmap->c,els);
	fl_set_object_label(fdmap->f,longs);
	fl_set_object_label(fdmap->h,lats);

	strncpy(timebuf,asctime(ptr_gmt),29);

	if (timebuf[8]==' ')
		timebuf[8]='0';

	if (el>=0.0)
		timestr[0]='L';
	else
		timestr[0]='A';

	timestr[1]='O';
	timestr[2]='S';
	timestr[3]=':';
	timestr[4]=' ';

	for (x=0; x<=3; timestr[x+5]=timebuf[x], x++);

	timestr[9]=timebuf[8];
	timestr[10]=timebuf[9];
	timestr[11]=timebuf[4];
	timestr[12]=timebuf[5];
	timestr[13]=timebuf[6];
	timestr[14]=timebuf[22];
	timestr[15]=timebuf[23];
	timestr[16]=' ';

	for (x=12; x<=19; timestr[x+5]=timebuf[x-1], x++);

	timestr[25]=0;

	fl_set_object_label(fdmap->s,timestr);

	/* Range Circle Calculations by KD2BD */

	ssplat=latitude*deg2rad;
	ssplong=longitude*deg2rad;
	beta=(0.5*footprint)/R0;

	fl_drawmode(GXxor);
	fl_points(xa,360,FL_WHITE);

	for (azi=0; azi<360; azi++)
	{
		azimuth=deg2rad*(double)azi;
		rangelat=asin(sin(ssplat)*cos(beta)+cos(azimuth)*sin(beta)*cos(ssplat));
		num=cos(beta)-(sin(ssplat)*sin(rangelat));
		dem=cos(ssplat)*cos(rangelat);

		if (azi==0 && (beta > HALFPI-ssplat))
			rangelong=ssplong+PI;

		else if (azi==180 && (beta > HALFPI+ssplat))
			rangelong=ssplong+PI;

		else if (fabs(num/dem)>1.0)
			rangelong=ssplong;

		else
		{
			if ((180-azi)>=0)
				rangelong=ssplong-arccos(num,dem);
			else
				rangelong=ssplong+arccos(num,dem);
		}

		while (rangelong<0.0)
			rangelong+=TWOPI;

		while (rangelong>(2.0*PI))
			rangelong-=TWOPI;
 
		rangelat=rangelat/deg2rad;
		rangelong=rangelong/deg2rad;

		/* Convert range circle data to map-based coordinates */

		if (rangelong<=180.0)
			xx=(int)(270.0-(500.0*(rangelong/360.0)));
		else
		{
			rangelong=rangelong-180.0;
			xx=(int)(520.0-(500.0*(rangelong/360.0)));
		}

		yy=(int)(145.0-(125.0*(rangelat/90.0)));
		xa[azi].x=xx;
		xa[azi].y=yy;
	}

	fl_drawmode(GXxor);
        fl_points(xa,360,FL_WHITE);

	if (longitude<=180.0)
		x=(int)(270.0-(500.0*(longitude/360.0)));
	else
	{
		longitude=longitude-180.0;
		x=(int)(520.0-(500.0*(longitude/360.0)));
	}

	y=(int)(145.0-(125.0*(latitude/90.0)));

	fl_drawmode(GXcopy);
	fl_ovalf(x,y,2,2,FL_RED);
	oldx=x;
	oldy=y;
  	fl_set_timer(ob,1); 
}

void l(FL_OBJECT *obj, long data)
{
	const char *sat;
	sat=fl_get_choice_text(obj);
	strcpy(satnum,sat);
}


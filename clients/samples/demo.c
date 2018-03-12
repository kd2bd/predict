/****************************************************************************
*                                                                           *
*  This program is a template that can used for building client programs    *
*  for PREDICT.  This program functions by sending a data request to the    *
*  server (the machine on which PREDICT is running in server mode), and     *
*  retrieves the response from the server.  Some parsing is required to     *
*  extract information from PREDICT's response and fill variables as        *
*  needed.  Examples of how this is done are shown in this sample program.  *
*                                                                           *
*  This program defaults to "localhost" as the name of machine on which     *
*  PREDICT is running in server mode.                                       *
*                                                                           *
*****************************************************************************
*                                                                           *
*  This program was originally written by Ivan Galysh, KD4HBO, 24-Jan-2000  *
*  and has been modified extensively since.  Most recently, Bent Bagger,    *
*  OZ6BL, provided a patch to detect the absence of a PREDICT server,       *
*  and print an error message to that effect.                               *
*                                                                           *
*                    Last modified on 30-March-2003                         *
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
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

char string[625];

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
	   socket protocol.  It should be set to UDP. */

	struct hostent *phe;
	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	
	int s, type;
	
	bzero((char *)&sin,sizeof(struct sockaddr_in));
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
		printf("Can't get host: \"%s\".\n",host);
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
		exit(-1);

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

	return string;
}

void send_command2(host, command)
char *host, *command;
{
	int sk;

	/* This function sends "command" to PREDICT running on
	   machine "host", and displays the result of the command
	   on the screen as text.  It reads the data sent back
	   from PREDICT until an EOF marker (^Z) is received. */

	/* Open a network socket */
	sk=connectsock(host,"predict","udp");

	if (sk<0)
		exit(-1);

	/* Build a command buffer */
	sprintf(string,"%s\n",command);

	/* Send the command to the server */
   	write(sk,command,strlen(string));

	/* clear string[] so it can be re-used for the response */
	string[0]=0;

	/* Read and display the response until a ^Z is received */
   	get_response(sk,string);

	while (string[0]!=26)  /* Control Z */
	{
		printf("%s",string);
		string[0]=0;
   		get_response(sk,string);
	}

	printf("\n");

	/* Close the connection */
   	close(sk);
}

int main()
{
	int x, y, z, satnum;
	char buf[128], command[128], satlist[625], satname[26],
	     visibility, satnamelist[26][26], event[15], squint_string[15];
	long aostime, orbitnum, start;
	float az, el, slong, slat, footprint, range, altitude,
	      velocity, phase, eclipse_depth, squint;
	time_t t;

	/* Get the list of satellite names from PREDICT */

	strcpy(satlist, send_command("localhost","GET_LIST"));

	printf("\nPREDICT returned the following string in response to GET_LIST:\n%s\n",satlist);

	/* Parse the response and place each name
	   in the character array satnamelist[]. */

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

	for (z=0; z<satnum; z++)
	{
		sprintf(command,"GET_SAT %s",satnamelist[z]);
		strcpy(buf, send_command("localhost",command));
		printf("The following string was returned in response to %s:\n%s\n",command,buf);

		/* Parse the response from GET_SAT */

		/* The first element of the response is the satellite name.
		   It is ended with a '\n' character and many contain spaces. */

		for (x=0; buf[x]!='\n'; x++)
			satname[x]=buf[x];

		satname[x]=0;
		x++;

		/* The rest of the data from GET_SAT is numerical, and
		   can be parsed using the sscanf() function.  First, the
		   satellite name is removed from "buf", and then "buf"
		   is parsed for numerical data using an sscanf(). */

		for (y=0; buf[x+y]!=0; y++)
			buf[y]=buf[x+y];

		buf[y]=0;
 
		sscanf(buf,"%f %f %f %f %ld %f %f %f %f %ld %c %f %f %f",&slong,&slat,&az,&el,&aostime,&footprint,&range,&altitude,&velocity,&orbitnum,&visibility,&phase,&eclipse_depth,&squint);

		t=(time_t)aostime;

		if (el>0.0)
			strcpy(event,"LOS at");

		else
			strcpy(event,"Next AOS at");

		if (squint==360.0)
			sprintf(squint_string,"N/A");
		else
			sprintf(squint_string,"%.2f degrees",squint);
		
		printf("Values are as follows:\nName: %s\nLong: %.2f degrees\nLat: %.2f degrees\nAz: %.2f degrees\nEl: %+-6.2f degrees\n%s: %ld = %sFootprint: %.2f km\nRange: %.2f km\nAltitude: %.2f km\nVelocity: %.2f km/hr\nEclipse Depth: %.2f\nPhase: %.2f\nSquint: %s\nOrbit #: %ld\n\n",satname,slong,slat,az,el,event,aostime,asctime(gmtime(&t)),footprint,range,altitude,velocity,eclipse_depth,phase,squint_string,orbitnum);
	}

	/* Some other commands... */

	strcpy(buf, send_command("localhost","GET_MOON"));
	printf("\nThe following string was returned in response to GET_MOON:\n%s\n",buf);

	strcpy(buf, send_command("localhost","GET_SUN"));
	printf("The following string was returned in response to GET_SUN:\n%s\n",buf);

	strcpy(buf, send_command("localhost","GET_VERSION"));
	printf("The following string was returned in response to GET_VERSION:\n%s\n",buf);

	strcpy(buf, send_command("localhost","GET_QTH"));
	printf("The following string was returned in response to GET_QTH:\n%s\n",buf);
	strcpy(buf, send_command("localhost","GET_MODE"));
	printf("The following string was returned in response to the GET_MODE command:\n%s\n",buf);

	sprintf(command,"GET_TLE %s",satnamelist[9]);
	strcpy(buf, send_command("localhost",command));
	printf("The following string was returned in response to %s:\n%s\n",command,buf);

	strcpy(buf, send_command("localhost","GET_TIME$"));
	printf("The following string was returned in response to GET_TIME$:\n%s\n",buf);

	strcpy(buf, send_command("localhost","GET_TIME"));
	printf("The following string was returned in response to GET_TIME:\n%s\n",buf);

	sscanf(buf,"%ld",&start);
	sprintf(command,"GET_SAT_POS \"%s\" %ld +10m",satnamelist[0],start);
	printf("The following string was returned in response to:\n\n\t%s:\n\n",command);
	send_command2("localhost",command);

	sprintf(command,"PREDICT \"%s\" %ld",satnamelist[23],start);
	printf("The following string was returned in response to:\n\n\t%s:\n\n",command);
	send_command2("localhost",command);

	printf("If that flew by too fast, consider piping the output of\n");
	printf("this demo program through 'less'.  (ie: demo | less)  :-)\n\n");

	exit(0);
}

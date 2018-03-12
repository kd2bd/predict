/****************************************************************************
*                                                                           *
*  This program allows socket commands to be interactively sent to PREDICT  *
*  for testing or educational purposes.  It was derived from the "demo.c"   *
*  program also located in this directory.                                  *
*                                                                           *
*  This program defaults to "localhost" as the name of machine on which     *
*  PREDICT is running in server mode.                                       *
*                                                                           *
*****************************************************************************
*                                                                           *
*                    Last modified on 10-September-2005                     *
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
	char command[128];

	/* The following command is used to initially "ping"
	   the PREDICT server to see if connectivity exists.
	   If this test fails, the program will exit with an
	   error message. */

	send_command("localhost", "GET_VERSION");

	printf("\nEnter the command you wish to send PREDICT.\n\n");

	printf("Enter command, or QUIT to exit >> ");

	fgets(command, 80, stdin);

	while (strncmp(command, "QUIT", 4) && strncmp(command, "quit", 4) && strncmp(command, "EXIT", 4) && strncmp(command, "exit", 4))
	{
		command[strlen(command)-1]=0;

		if (strncmp(command, "PREDICT",7)==0 || strncmp(command, "GET_SAT_POS",11)==0)
			send_command2("localhost", command);
		else if (command[0])
			printf("%s\n",send_command("localhost", command));

		printf("Enter command, or QUIT to exit >> ");

		fgets(command, 80, stdin);
	}

	exit(0);
}


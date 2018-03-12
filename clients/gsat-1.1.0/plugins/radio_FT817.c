/*
 * Yaesu FT-817 plugin for gsat
 *
 * Copyright (C) 2001 by Xavier Crehueras, EB3CZS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Look at the README for more information on the program.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <math.h>

/*  #define DEBUG 1 */

char * plugin_info( void );
int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

int port;

void clearDTR( int fd )
{
  int lineData;

  ioctl(fd, TIOCMGET, &lineData);
  lineData &= ~TIOCM_DTR;
  ioctl(fd, TIOCMSET, &lineData);
}

void setDTR( int fd )
{
  int lineData;

  ioctl(fd, TIOCMGET, &lineData);
  lineData |= TIOCM_DTR;
  ioctl(fd, TIOCMSET, &lineData);
}

char * plugin_info( void )
{
  return "Yaesu FT-817 V0.1";
}

int plugin_open_rig( char * config )
{
  struct termios ios;
  tcflag_t bauds;
  char dummy[64];
  char tty[12];
  char speed[8];
  char *ptr, *parm;
  int portbauds, n;

  tty[0]='\0';
  speed[0]='\0';
  civid[0]='\0';

  if(config) {
    strncpy(dummy,config,64);

    ptr=dummy;
    parm=ptr;
    while( parm != NULL ) {
      parm=strsep(&ptr,":");
      if(parm==NULL)
	break;
      if(strlen(parm)!=0) {
	switch( *parm ) {
	case 'D':
	  strcpy(tty,parm+1);
	  break;
	case 'S':
	  strcpy(speed,parm+1);
	  break;
	}
      }
    }
  }
    
  if(strlen(tty)==0)
    strcpy(tty,"/dev/ttyS0");
  if(strlen(speed)==0)
    strcpy(speed,"4800");

  portbauds=atoi(speed);
  switch( portbauds ) {
  case 300:
    bauds=B300;
    break;
  case 1200:
    bauds=B1200;
    break;
  case 2400:
    bauds=B2400;
    break;
  case 4800:
    bauds=B4800;
    break;
  case 9600:
    bauds=B9600;
    break;
  case 19200:
    bauds=B19200;
    break;
  default:
    bauds=B9600;
  }

#ifdef DEBUG
  fprintf(stderr,"FT817: trying to open rig with config: %s.\n",config);
  fprintf(stderr,"FT817: config tty: %s\n",tty);
  fprintf(stderr,"FT817: config speed: %s\n",speed);
#endif

  /* Open the tty and configure it */

  if ((port = open(tty, O_RDWR | O_NOCTTY)) == -1) {
    perror("open");
    return 0;
  }

  if (!isatty(port)) {
    close(port);
    fprintf(stderr, "FT817: %s: not a tty\n", tty);
    return 0;
  }

  memset(&ios, 0, sizeof(ios));

  cfsetispeed(&ios, bauds);
  cfsetospeed(&ios, bauds);

  ios.c_cflag |=  (CLOCAL | CREAD);
  ios.c_cflag &= ~HUPCL ;
  ios.c_cflag &= ~PARENB ;
  ios.c_cflag &= ~CSTOPB ;
  ios.c_cflag &= ~CSIZE;
  ios.c_cflag |= CS8;

  /* ICANON  : enable canonical input disable all echo functionality,
     and don't send signals to calling program */
  ios.c_lflag |= ICANON;
  ios.c_lflag &= ~(ECHO | ECHOCTL);

  /* ignore bytes with parity errors */
  ios.c_iflag |= IGNPAR;

  /* Raw output.  */
  ios.c_oflag &= ~OPOST;

  if (tcsetattr(port, TCSANOW, &ios) == -1) {
    perror("tcsetattr");
    return 0;
  }

  setDTR( port );

#ifdef DEBUG
  fprintf(stderr,"FT817: open rig succeed.\n");
#endif

  return 1;
}

void plugin_close_rig( void )
{
  if( port ) {
      clearDTR( port );
      close(port);
  }
  port = 0;

#ifdef DEBUG
  fprintf(stderr,"FT817: close rig succeed.\n");
#endif
}

void send_freq_to_FT817( double frequency )
{
  fd_set fds;
  int count;
  struct timeval tv;
  unsigned char command[12];
  char ghz,hmhz,dmhz,mhz,hkhz,dkhz,khz,hhz;

  frequency=frequency*10.0;

  ghz =(frequency-fmod(frequency,10000000))/10000000;
  frequency=fmod(frequency,10000000);
  hmhz=(frequency-fmod(frequency,1000000))/1000000;
  frequency=fmod(frequency,1000000);
  dmhz=(frequency-fmod(frequency,100000))/100000;
  frequency=fmod(frequency,100000);
  mhz =(frequency-fmod(frequency,10000))/10000;
  frequency=fmod(frequency,10000);
  hkhz=(frequency-fmod(frequency,1000))/1000;
  frequency=fmod(frequency,1000);
  dkhz=(frequency-fmod(frequency,100))/100;
  frequency=fmod(frequency,100);
  khz =(frequency-fmod(frequency,10))/10;
  frequency=fmod(frequency,10);
  hhz =frequency;
  frequency=fmod(frequency,10);
  hhz =frequency;

  command[0]=(hmhz<<4)|dmhz;
  command[1]=(mhz<<4)|hkhz;
  command[2]=(dkhz<<4)|khz;
  command[3]=(hhz<<4)|dhz;
  command[4]=0x01;

  /* Send it */
#ifdef DEBUG
  fprintf( stderr, "FT817: Sending command ... ");
  {int i;
  for(i=0;i<11;i++) fprintf(stderr," %02X",command[i]);
  fprintf(stderr,"\n");
  }
#endif

  FD_ZERO(&fds);
  FD_SET(port, &fds);
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  if (select(port+1, NULL, &fds, NULL, &tv)) {
    if ((count = write(port, command, 11)) == -1) {
      perror("write");
      return;
    }
  }
  else {
    fprintf( stderr,"FT817: Timeout sending command !!!\n");
    perror("select");
    return;
  }
}

void plugin_set_downlink_frequency( double frequency )
{
  send_freq_to_FT817( frequency );
}

void plugin_set_uplink_frequency( double frequency )
{
  send_freq_to_FT817( frequency );
}

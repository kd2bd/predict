/*
 * Icom R-10 receiver plugin for gsat
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
  return "Icom R10 Receiver V0.9";
}

int plugin_open_rig( char * config )
{
  struct termios ios;
  tcflag_t bauds;
  char *tty;
  char *ptr;

  if( config != NULL )
    tty=config;
  else
    tty="/dev/ttyS0";

#ifdef DEBUG
  fprintf(stderr,"ICR10: trying to open rig on %s with config: %s.\n",tty,config);
#endif

  /* Open the tty and configure it */

  if ((port = open(tty, O_RDWR | O_NOCTTY)) == -1) {
    perror("open");
    return 0;
  }

  if (!isatty(port)) {
    close(port);
    fprintf(stderr, "ICR10: %s: not a tty\n", tty);
    return 0;
  }

  memset(&ios, 0, sizeof(ios));

  cfsetispeed(&ios, B9600);
  cfsetospeed(&ios, B9600);

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
  fprintf(stderr,"ICR10: open rig succeed.\n");
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
  fprintf(stderr,"ICR10: close rig succeed.\n");
#endif
}

void send_freq_to_ICR10( double frequency )
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

  command[0]=0xfe;
  command[1]=0xfe;
  command[2]=0x52;
  command[3]=0xe0;
  command[4]=0x05;
  command[5]=0x00;
  command[6]=(khz<<4)|hhz;
  command[7]=(hkhz<<4)|dkhz;
  command[8]=(dmhz<<4)|mhz;
  command[9]=(ghz<<4)|hmhz;
  command[10]=0xfd;
  command[11]=0x00;

  /* Send it */
#ifdef DEBUG
  fprintf( stderr, "ICR10: Sending command ... ");
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
    fprintf( stderr,"ICR10: Timeout sending command !!!\n");
    perror("select");
    return;
  }
}

void plugin_set_downlink_frequency( double frequency )
{
  send_freq_to_ICR10( frequency );
}

void plugin_set_uplink_frequency( double frequency )
{
  send_freq_to_ICR10( frequency );
}

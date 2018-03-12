/*
 * PicTrack rotor plugin for gsat
 *
 * (c)2001 by Xavier Crehueras, EB3CZS
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char * plugin_info( void );
int plugin_open_rotor( char * config );
void plugin_close_rotor( void );
void plugin_set_rotor( double azimuth, double elevation );

int antfd;

char * plugin_info( void )
{
  return "PicTrack V1.0";
}

int plugin_open_rotor( char * config )
{
  antfd=open(config, O_RDWR | O_NOCTTY | O_NDELAY);

  if (antfd!=-1)
    fcntl(antfd, F_SETFL, 0);
  else {
    fprintf(stderr,"Unable To Open Antenna Port\n");
    return 0;
  }

  return 1;
}

void plugin_close_rotor( void )
{
  close(antfd);
}

void plugin_set_rotor( double azimuth, double elevation )
{
  /* This function sends Azimuth and Elevation data
     to an antenna tracker connected to the serial port */

  int n;
  char message[30]="\n";

  sprintf(message, "AZ%3.1f EL%3.1f \x0D\x0A", azimuth,elevation);
  n=write(antfd,message,30);

  if (n<0)
    fprintf(stderr, "Error Writing To Antenna Port\n");

  return;
}

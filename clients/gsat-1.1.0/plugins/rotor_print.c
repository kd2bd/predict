/*
 * Print Azimuth/Elevation demo rotor plugin for gsat
 *
 * (c)2001 by Xavier Crehueras, EB3CZS
 */

#include <stdio.h>

char * plugin_info( void );
int plugin_open_rotor( char * config );
void plugin_close_rotor( void );
void plugin_set_rotor( double azimuth, double elevation );

char * plugin_info( void )
{
  return "PrintAzimuth/Elevation V1.0";
}

int plugin_open_rotor( char * config )
{
  printf("PrintAzimuth/Elevation plugin opened.\n");

  return 1;
}

void plugin_close_rotor( void )
{
  printf("PrintAzimuth/Elevation plugin closed.\n");
}

void plugin_set_rotor( double azimuth, double elevation )
{
  printf("Azimuth: %f deg - Elevation: %f deg\n", azimuth, elevation);
}

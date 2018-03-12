/*
 * Print Frequency demo plugin for gsat
 *
 * (c)2001 by Xavier Crehueras, EB3CZS
 */

#include <stdio.h>

char * plugin_info( void );
int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

char * plugin_info( void )
{
  return "PrintFrequency V1.0";
}

int plugin_open_rig( char * config )
{
  printf("PrintFrequency plugin opened.\n");

  return 1;
}

void plugin_close_rig( void )
{
  printf("PrintFrequency plugin closed.\n");
}

void plugin_set_downlink_frequency( double frequency )
{
  printf("Downlink Frequency: %f kHz\n", frequency);
}

void plugin_set_uplink_frequency( double frequency )
{
  printf("Uplink Frequency: %f kHz\n", frequency);
}

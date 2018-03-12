#include <stdio.h>
#include <math.h>

char * plugin_info( void );
int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

char * plugin_info( void )
{
  return "Test V0.1";
}

int plugin_open_rig( char * config )
{
  printf("Test plugin opened.\n");

  return 1;
}

void plugin_close_rig( void )
{
  printf("Test plugin closed.\n");
}

void plugin_set_downlink_frequency( double frequency )
{
  double freq;

  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

  printf("Downlink Frequency: %f kHz -> %f kHz\n", frequency, freq);
}

void plugin_set_uplink_frequency( double frequency )
{
  double freq;

  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

  printf("Uplink Frequency: %f kHz -> %f kHz\n", frequency, freq);
}

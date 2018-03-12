/*
 * Kenwood TM-D700 transceiver plugin for gsat
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


#include <stdio.h>
#include <math.h>

#include "libD700.h"

#define TRUE 1
#define FALSE 0
 
#define DEBUG 1

char * plugin_info( void );
int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

D700rig rig;

char * plugin_info( void )
{
  return "Kenwood TM-D700 V0.2";
}

int plugin_open_rig( char * config )
{
#ifdef DEBUG
  fprintf(stderr,"Opening Kenwood TM-D700 plugin.\n");
#endif

  if(d700_OpenD700( &rig, "/dev/ttyS0", 9600 )==1) {
    fprintf(stderr,"TMD700: Error opening rig.\n");
    return FALSE;
  }
}

void plugin_close_rig( void )
{
#ifdef DEBUG
  fprintf(stderr,"Closing Kenwood TM-D700 plugin.\n");
#endif

  d700_CloseD700( &rig );
}

void plugin_set_downlink_frequency( double frequency )
{
  double freq;

  printf("TMD700: Set Frequency\n");
  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

/*    d700_FrequencyUp( &rig ); */
/*    d700_SetControlBand( &rig, BAND_A ); */
/*    d700_SetFrequencyandStep( &rig, freq*1000, 0 ); */
  d700_SetFrequency( &rig, BAND_A, freq*1000 );
}

void plugin_set_uplink_frequency( double frequency )
{
  double freq;

  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

#ifdef DEBUG
  fprintf(stderr,"TMD700: Frequency: %f -> %f\n",frequency, freq);
#endif

  d700_SetControlBand( &rig, BAND_B );
  d700_SetFrequencyandStep( &rig, freq*1000, 0 );
}

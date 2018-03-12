/*
 * Icom PCR100 receiver plugin for gsat
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
#include <string.h>

#include "libpcr100.h"

/*  #define DEBUG 1 */

char * plugin_info( void );
int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

PCRinfo rig;

char * plugin_info( void )
{
  return "Icom PCR100 receiver V1.0";
}

int plugin_open_rig( char * config )
{
  char dummy[10];
  char *ptr;
  char tty[10];
  char mode[4];
  char filter[5];
  ushort volume;
  ushort squelch;
  int n;

#ifdef DEBUG
  printf("Opening Icom PCR100 plugin.\n");
#endif

  ptr=config;
  if( config[0] != '\0' ) {
    while( *ptr != '\0' ) {
      switch( *ptr ) {
      case 'D':
	ptr++;
	n=0;
	while( *ptr != '\0' && *ptr != ':' && n<9 ) {
	  tty[n++]=*ptr++;
	}
	tty[n]='\0';
	break;
      case 'M':
	ptr++;
	n=0;
	while( *ptr != '\0' && *ptr != ':' && n<3 ) {
	  mode[n++]=*ptr++;
	}
	mode[n]='\0';
	break;
      case 'V':
	ptr++;
	n=0;
	while( *ptr != '\0' && *ptr != ':' && n<9 ) {
	  dummy[n++]=*ptr++;
	}
	dummy[n]='\0';
	volume=atoi(dummy);
	break;
      case 'S':
	ptr++;
	n=0;
	while( *ptr != '\0' && *ptr != ':' && n<9 ) {
	  dummy[n++]=*ptr++;
	}
	dummy[n]='\0';
	squelch=atoi(dummy);
	break;
      case 'F':
	ptr++;
	n=0;
	while( *ptr != '\0' && *ptr != ':' && n<4 ) {
	  filter[n++]=*ptr++;
	}
	filter[n]='\0';
	break;
      case ':':
	ptr++;
	break;
      }
    }
  }
  else {
    strcpy(tty,"/dev/ttyS0");
    strcpy(mode,"FM");
    strcpy(filter,"15K");
    volume=160;
    squelch=100;
  }

#ifdef DEBUG
  printf("Config: %s\n", config);
  printf("tty: %s mode: %s filter: %s volume: %d squelch: %d\n",
	 tty, mode, filter, volume, squelch);
#endif

  pcr_OpenPCR( &rig, tty, 9600 );
  pcr_PowerOn( &rig );
  pcr_SetMode( &rig, mode );
  pcr_SetFilter( &rig, filter );
  pcr_SetVolume( &rig, volume );
  pcr_SetSquelch( &rig, squelch );

  return 1;
}

void plugin_close_rig( void )
{
#ifdef DEBUG
  printf("Closing Icom PCR100 plugin.\n");
#endif

  pcr_PowerOff( &rig );
  pcr_ClosePCR( &rig );
}

void plugin_set_downlink_frequency( double frequency )
{
  pcr_SetFrequency( &rig, (ulong)(frequency*1000) );
}

void plugin_set_uplink_frequency( double frequency )
{
  pcr_SetFrequency( &rig, (ulong)(frequency*1000) );
}

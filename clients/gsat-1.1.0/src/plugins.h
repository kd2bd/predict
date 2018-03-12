/*
 * gsat - a realtime satellite tracking graphical frontend to predict
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

/* Plugin functions */

void close_uplink_plugin( void );
int open_uplink_plugin( char * plugin );
void close_downlink_plugin( void );
int open_downlink_plugin( char * plugin );
void close_beacon_plugin( void );
int open_beacon_plugin( char * plugin );
int check_radio_plugin_file( const void * plugdir );
void search_radio_plugins( void );
void close_rotor_plugin( void );
int open_rotor_plugin( char * plugin );
int check_rotor_plugin_file( const void * plugdir );
void search_rotor_plugins( void );

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

/* User preferences functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "support.h"
#include "plugins.h"

#include "globals.h"

void open_preferences( void )
{
  char filename[256];
  FILE * fd;
  int n, state;
  GtkWidget *widget;

  strncpy(filename,getenv("HOME"),256);
  strcat(filename,"/.gsat.prefs");

  if((fd=fopen( filename, "r" ))==NULL) {
    strncpy(filename,GSATLIBDIR"/gsat.prefs",256);
    if((fd=fopen( filename, "r" ))==NULL)
      return;
  }

  prefs_grid[0]=fgetc(fd);
  prefs_utctime[0]=fgetc(fd);
  prefs_satfootprint[0]=fgetc(fd);
  prefs_qthfootprint[0]=fgetc(fd);
  prefs_track[0]=fgetc(fd);
  prefs_autofreq[0]=fgetc(fd);
  fgetc(fd);
  fgets(prefs_aos_command, 1024, fd);
  prefs_aos_command[strlen(prefs_aos_command)-1]='\0';
  fgets(prefs_los_command, 1024, fd);
  prefs_los_command[strlen(prefs_los_command)-1]='\0';
  fgets(prefs_down_plugin, 256, fd);
  prefs_down_plugin[strlen(prefs_down_plugin)-1]='\0';
  fgets(prefs_down_plugin_config, 256, fd);
  prefs_down_plugin_config[strlen(prefs_down_plugin_config)-1]='\0';
  fgets(prefs_up_plugin, 256, fd);
  prefs_up_plugin[strlen(prefs_up_plugin)-1]='\0';
  fgets(prefs_up_plugin_config, 256, fd);
  prefs_up_plugin_config[strlen(prefs_up_plugin_config)-1]='\0';
  fgets(prefs_beacon_plugin, 256, fd);
  prefs_beacon_plugin[strlen(prefs_beacon_plugin)-1]='\0';
  fgets(prefs_beacon_plugin_config, 256, fd);
  prefs_beacon_plugin_config[strlen(prefs_beacon_plugin_config)-1]='\0';
  fgets(prefs_rotor_plugin, 256, fd);
  prefs_rotor_plugin[strlen(prefs_rotor_plugin)-1]='\0';
  fgets(prefs_rotor_plugin_config, 256, fd);
  prefs_rotor_plugin_config[strlen(prefs_rotor_plugin_config)-1]='\0';
  fclose( fd );

  if(strcmp(prefs_down_plugin, "None")!=0)
    for(n=0; n<g_list_length( pluginlist ); n++) {
      if( strncmp(prefs_down_plugin, pluginfiles[n],255)==0 ) {
	strcpy( filename, pluginsdir );
	strcat( filename, prefs_down_plugin );
	if(open_downlink_plugin( filename )==FALSE)
	  fprintf(stderr,"Openprefs: error opening downlink plugin.\n");
      }
    }
  
  if(strcmp(prefs_up_plugin, "None")!=0)
    for(n=0; n<g_list_length( pluginlist ); n++) {
      if( strncmp(prefs_up_plugin, pluginfiles[n],255)==0) {
	strcpy( filename, pluginsdir );
	strcat( filename, prefs_up_plugin );
	if(open_uplink_plugin( filename )==FALSE)
	  fprintf(stderr,"Openprefs: error opening uplink plugin.\n");
      }
    }

  if(strcmp(prefs_beacon_plugin, "None")!=0)
    for(n=0; n<g_list_length( pluginlist ); n++) {
      if( strncmp(prefs_beacon_plugin, pluginfiles[n],255)==0) {
	strcpy( filename, pluginsdir );
	strcat( filename, prefs_beacon_plugin );
	if(open_beacon_plugin( filename )==FALSE)
	  fprintf(stderr,"Openprefs: error opening beacon plugin.\n");
      }
    }

  if(strcmp(prefs_rotor_plugin, "None")!=0)
    for(n=0; n<g_list_length( rotorpluginlist ); n++) {
      if( strncmp(prefs_rotor_plugin, rotorpluginfiles[n],255)==0) {
	strcpy( filename, pluginsdir );
	strcat( filename, prefs_rotor_plugin );
	if(open_rotor_plugin( filename )==FALSE)
	  fprintf(stderr,"Openprefs: error opening rotor plugin.\n");
      }
    }

  if( prefs_grid[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_grid" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  if( prefs_utctime[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_utctime" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  if( prefs_satfootprint[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_footpsat" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  if( prefs_qthfootprint[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_footpqth" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  if( prefs_track[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_track" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  if( prefs_autofreq[0]=='1' )
    state=TRUE;
  else
    state=FALSE;
  widget=lookup_widget( mainwindow, "tb_autofreq" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );
}

void save_preferences( void )
{
  char prefsfile[256];
  FILE * fd;

  strncpy(prefsfile,getenv("HOME"),256);
  strcat(prefsfile,"/.gsat.prefs");

  fd=fopen( prefsfile, "w" );
  fputc(prefs_grid[0], fd);
  fputc(prefs_utctime[0], fd);
  fputc(prefs_satfootprint[0], fd);
  fputc(prefs_qthfootprint[0], fd);
  fputc(prefs_track[0], fd);
  fputc(prefs_autofreq[0], fd);
  fputc('\n', fd);
  fputs(prefs_aos_command, fd);
  fputc('\n', fd);
  fputs(prefs_los_command, fd);
  fputc('\n', fd);
  fputs(prefs_down_plugin, fd);
  fputc('\n', fd);
  fputs(prefs_down_plugin_config, fd);
  fputc('\n', fd);
  fputs(prefs_up_plugin, fd);
  fputc('\n', fd);
  fputs(prefs_up_plugin_config, fd);
  fputc('\n', fd);
  fputs(prefs_beacon_plugin, fd);
  fputc('\n', fd);
  fputs(prefs_beacon_plugin_config, fd);
  fputc('\n', fd);
  fputs(prefs_rotor_plugin, fd);
  fputc('\n', fd);
  fputs(prefs_rotor_plugin_config, fd);
  fputc('\n', fd);
  fclose( fd );
}

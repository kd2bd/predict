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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <gtk/gtk.h>

#include "support.h"
#include "comms.h"

#include "globals.h"

void close_uplink_plugin( void )
{
  if(uppluginenable==FALSE)
    return;

  /* close rig if it was open before closing plugin */
  if( enableupdoppler==TRUE ) {
    (*plugin_close_rig_uplink)();
    enableupdoppler=FALSE;
  }

  dlclose(upplugin_handle);

  uppluginenable=FALSE;
}

int open_uplink_plugin( char * plugin )
{
  char * error;

  /* if it's already open, close it first */
  if( uppluginenable==TRUE )
    close_uplink_plugin();

  upplugin_handle=dlopen( plugin, RTLD_NOW );
  if( upplugin_handle==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error opening uplink plugin: %s\n",error);
      return FALSE;
    }

  plugin_info_uplink=dlsym(upplugin_handle,"plugin_info");
  if( plugin_info_uplink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_info_uplink: %s\n",error);
      return FALSE;
    }

  plugin_open_rig_uplink=dlsym(upplugin_handle,"plugin_open_rig");
  if( plugin_open_rig_uplink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_open_rig_uplink: %s\n",error);
      return FALSE;
    }

  plugin_close_rig_uplink=dlsym(upplugin_handle,"plugin_close_rig");
  if( plugin_close_rig_uplink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_close_rig_uplink: %s\n",error);
      return FALSE;
    }

  plugin_set_uplink_frequency=dlsym(upplugin_handle,"plugin_set_uplink_frequency");
  if( plugin_set_uplink_frequency==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_set_uplink_frequency: %s\n",error);
      return FALSE;
    }

  uppluginenable=TRUE;

  return TRUE;
}

void close_downlink_plugin( void )
{
  if(downpluginenable==FALSE)
    return;

  /* close rig if it was open before closing plugin */
  if( enabledowndoppler==TRUE ) {
    (*plugin_close_rig_downlink)();
    enabledowndoppler=FALSE;
  }

  dlclose(downplugin_handle);

  downpluginenable=FALSE;
}

int open_downlink_plugin( char * plugin )
{
  char * error;

  /* if it's already open, close it first */
  if( downpluginenable==TRUE )
    close_downlink_plugin();

  downplugin_handle=dlopen( plugin, RTLD_NOW );
  if( downplugin_handle==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error opening downlink plugin: %s\n",error);
      return FALSE;
    }

  plugin_info_downlink=dlsym(downplugin_handle,"plugin_info");
  if( plugin_info_downlink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_info_downlink: %s\n",error);
      return FALSE;
    }
  plugin_open_rig_downlink=dlsym(downplugin_handle,"plugin_open_rig");
  if( plugin_open_rig_downlink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_open_rig_downlink: %s\n",error);
      return FALSE;
    }
  plugin_close_rig_downlink=dlsym(downplugin_handle,"plugin_close_rig");
  if( plugin_close_rig_downlink==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_close_rig_downlink: %s\n",error);
      return FALSE;
    }
  plugin_set_downlink_frequency=dlsym(downplugin_handle,"plugin_set_downlink_frequency");
  if( plugin_set_downlink_frequency==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_set_downlink_frequency: %s\n",error);
      return FALSE;
    }

  downpluginenable=TRUE;

  return TRUE;
}

void close_beacon_plugin( void )
{
  if(beaconpluginenable==FALSE)
    return;

  /* close rig if it was open before closing plugin */
  if( enablebeacondoppler==TRUE ) {
    (*plugin_close_rig_beacon)();
    enablebeacondoppler=FALSE;
  }

  dlclose(beaconplugin_handle);

  beaconpluginenable=FALSE;
}

int open_beacon_plugin( char * plugin )
{
  char * error;

  /* if it's already open, close it first */
  if( beaconpluginenable==TRUE )
    close_beacon_plugin();

  beaconplugin_handle=dlopen( plugin, RTLD_NOW );
  if( beaconplugin_handle==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error opening beacon plugin: %s\n",error);
      return FALSE;
    }

  plugin_info_beacon=dlsym(beaconplugin_handle,"plugin_info");
  if( plugin_info_beacon==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_info_beacon: %s\n",error);
      return FALSE;
    }
  plugin_open_rig_beacon=dlsym(beaconplugin_handle,"plugin_open_rig");
  if( plugin_open_rig_beacon==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_open_rig_beacon: %s\n",error);
      return FALSE;
    }
  plugin_close_rig_beacon=dlsym(beaconplugin_handle,"plugin_close_rig");
  if( plugin_close_rig_beacon==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_close_rig_beacon: %s\n",error);
      return FALSE;
    }
  plugin_set_beacon_frequency=dlsym(beaconplugin_handle,"plugin_set_downlink_frequency");
  if( plugin_set_beacon_frequency==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_set_beacon_frequency: %s\n",error);
      return FALSE;
    }

  beaconpluginenable=TRUE;

  return TRUE;
}

int check_radio_plugin_file( const struct dirent * plugdir )
{
  return ! strncmp(plugdir->d_name,"radio_",6);
}

void search_radio_plugins( void )
{
  struct dirent **namelist;
  int n, i;
  GtkWidget * widget;
  char filename[256];
  DIR * dir;

  /* empty plugin list if it was created */
  n = g_list_length(pluginlist);
  if( n > 0 ) {
    pluginlist=g_list_first( pluginlist );
    for( i=0; i<n; i++ )
      pluginlist=g_list_remove( pluginlist, pluginlist->data );
    for( i=0; i<n; i++ ) {
      pluginfiles[i][0]='\0';
      plugindescriptions[i][0]='\0';
    }
  }

  strcpy( pluginfiles[0], "None");
  strcpy( plugindescriptions[0], "No Plugin");
  pluginlist=g_list_append( pluginlist, plugindescriptions[0] );
  
  /* check plugin directory */
  if( (dir=opendir( pluginsdir ))==NULL ) {
    switch( errno ) {
    case EACCES:
      fprintf(stderr, "Error: You don't have access to plugin directory.\n");
      error_dialog("You don't have access to plugin directory.");
      break;
    case ENOENT:
      fprintf(stderr, "Error: Plugin directory doesn't exist.\n");
      error_dialog("Plugin directory doesn't exist.");
      break;
    case ENOTDIR:
      fprintf(stderr, "Error: Plugin directory is not a directory.\n");
      error_dialog("Can't open plugin directory.");
      break;
    default:
      fprintf(stderr, "Error: Can't open plugin directory.\n");
      error_dialog("Can't open plugin directory.");
      break;
    }
    /* Attach plugin list */
    widget=lookup_widget( dialog_preferences, "combo_upplugin" );
    gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
    widget=lookup_widget( dialog_preferences, "combo_downplugin" );
    gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
    widget=lookup_widget( dialog_preferences, "combo_beaconplugin" );
    gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
    return;
  }
  closedir( dir );

  /* scan plugin directory and create plugin list */
  n = scandir(pluginsdir, &namelist, check_radio_plugin_file, alphasort);
  if (n < 0) {
    perror("scandir");
    return;
  }

  for( i=0; i<n; i++ ) {
    strcpy(filename, pluginsdir);
    strcat(filename, namelist[i]->d_name);
    if(open_downlink_plugin(filename)==TRUE) {
      strncpy( pluginfiles[i+1], namelist[i]->d_name, 29);
      strncpy( plugindescriptions[i+1], (*plugin_info_downlink)(), 29);
      pluginlist=g_list_append( pluginlist, plugindescriptions[i+1] );
      close_downlink_plugin();
    }
  }

  /* Attach plugin list */
  widget=lookup_widget( dialog_preferences, "combo_upplugin" );
  gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
  widget=lookup_widget( dialog_preferences, "combo_downplugin" );
  gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
  widget=lookup_widget( dialog_preferences, "combo_beaconplugin" );
  gtk_combo_set_popdown_strings( GTK_COMBO(widget), pluginlist);
}

void close_rotor_plugin( void )
{
  if(rotorpluginenable==FALSE)
    return;

  /* close rotor if it was open before closing plugin */
  if( enablerotor==TRUE ) {
    (*plugin_close_rotor)();
    enablerotor=FALSE;
  }

  dlclose(rotorplugin_handle);

  rotorpluginenable=FALSE;
}

int open_rotor_plugin( char * plugin )
{
  char * error;

  /* if it's already open, close it first */
  if( rotorpluginenable==TRUE )
    close_rotor_plugin();

  rotorplugin_handle=dlopen( plugin, RTLD_NOW );
  if( rotorplugin_handle==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error opening rotor plugin: %s\n",error);
      return FALSE;
    }

  plugin_info_rotor=dlsym(rotorplugin_handle,"plugin_info");
  if( plugin_info_rotor==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_info_rotor: %s\n",error);
      return FALSE;
    }
  plugin_open_rotor=dlsym(rotorplugin_handle,"plugin_open_rotor");
  if( plugin_open_rotor==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_open_rotor: %s\n",error);
      return FALSE;
    }
  plugin_close_rotor=dlsym(rotorplugin_handle,"plugin_close_rotor");
  if( plugin_close_rotor==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_close_rotor: %s\n",error);
      return FALSE;
    }
  plugin_set_rotor=dlsym(rotorplugin_handle,"plugin_set_rotor");
  if( plugin_set_rotor==NULL )
    if( (error=dlerror()) != NULL ) {
      fprintf(stderr,"Error: plugin_set_rotor: %s\n",error);
      return FALSE;
    }

  rotorpluginenable=TRUE;

  return TRUE;
}

int check_rotor_plugin_file( const struct dirent * plugdir )
{
  return ! strncmp(plugdir->d_name,"rotor_",6);
}

void search_rotor_plugins( void )
{
  struct dirent **namelist;
  int n, i;
  GtkWidget * widget;
  char filename[256];
  DIR * dir;

  /* empty plugin list if it was created */
  n = g_list_length(rotorpluginlist);
  if( n > 0 ) {
    rotorpluginlist=g_list_first( rotorpluginlist );
    for( i=0; i<n; i++ )
      rotorpluginlist=g_list_remove( rotorpluginlist, rotorpluginlist->data );
    for( i=0; i<n; i++ ) {
      rotorpluginfiles[i][0]='\0';
      rotorplugindescriptions[i][0]='\0';
    }
  }

  strcpy( rotorpluginfiles[0], "None");
  strcpy( rotorplugindescriptions[0], "No Plugin");
  rotorpluginlist=g_list_append( rotorpluginlist, rotorplugindescriptions[0] );
  
  /* check plugin directory */
  if( (dir=opendir( pluginsdir ))==NULL ) {
    switch( errno ) {
    case EACCES:
      fprintf(stderr, "Error: You don't have access to plugin directory.\n");
      error_dialog("You don't have access to plugin directory.");
      break;
    case ENOENT:
      fprintf(stderr, "Error: Plugin directory doesn't exist.\n");
      error_dialog("Plugin directory doesn't exist.");
      break;
    case ENOTDIR:
      fprintf(stderr, "Error: Plugin directory is not a directory.\n");
      error_dialog("Can't open plugin directory.");
      break;
    default:
      fprintf(stderr, "Error: Can't open plugin directory.\n");
      error_dialog("Can't open plugin directory.");
      break;
    }
    /* Attach rotor plugin list */
    widget=lookup_widget( dialog_preferences, "combo_rotorplugin" );
    gtk_combo_set_popdown_strings( GTK_COMBO(widget), rotorpluginlist);

    return;
  }
  closedir( dir );

  /* scan plugin directory and create plugin list */
  n = scandir(pluginsdir, &namelist, check_rotor_plugin_file, alphasort);
  if (n < 0) {
    perror("scandir");
    return;
  }

  for( i=0; i<n; i++ ) {
    strcpy(filename, pluginsdir);
    strcat(filename, namelist[i]->d_name);
    if(open_rotor_plugin(filename)==TRUE) {
      strncpy( rotorpluginfiles[i+1], namelist[i]->d_name, 29);
      strncpy( rotorplugindescriptions[i+1], (*plugin_info_rotor)(), 29);
      rotorpluginlist=g_list_append(rotorpluginlist, rotorplugindescriptions[i+1]);
      close_rotor_plugin();
    }
  }

  /* Attach plugin list */
  widget=lookup_widget( dialog_preferences, "combo_rotorplugin" );
  gtk_combo_set_popdown_strings( GTK_COMBO(widget), rotorpluginlist);
}

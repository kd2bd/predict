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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "db.h"

#include "globals.h"

void error_dialog(gchar *message);
void show_status( const gchar *statusmsg );
int get_response(int sock, char *buf);
int connect_server( void );
void disconnect_server( void );
int open_downlink_plugin( char * plugin );
int open_uplink_plugin( char * plugin );
int open_beacon_plugin( char * plugin );
void search_radio_plugins( void );
int open_rotor_plugin( char * plugin );
void search_rotor_plugins( void );
void save_preferences( void );
void lookup_sat_db( void );
void save_sat_db( void );

gboolean
mainwindow_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_main_quit();

  return FALSE;
}


gboolean
maparea_expose_event                   (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  /* Copy internal buffer to screen area */
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  drawmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void
combo_tx_satname_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
  int i;
  GtkWidget *widget;
  GdkRectangle updatewin;

  /* copy original map to draw map */
  if( drawmap )
    gdk_draw_pixmap(drawmap,yellow_gc,sourcemap,0,0,0,0,MAPSIZEX,MAPSIZEY);

  /* Reset dots array */
  for( i=0; i<MAXDOTS; i++ ) {
    dots[i].x=0;
    dots[i].y=0;
  }
  ndots=0;

  /* copy empty az/el graphic to draw pixmap */
  if( drawazel ) {
    gdk_draw_pixmap(drawazel,yellow_gc,sourceazel,0,0,0,0,AZELSIZEX,AZELSIZEY);

    /* Force AZ/EL redraw */
    widget = lookup_widget( dialog_azel_graph, "azelgraph" );
    updatewin.x = 0;
    updatewin.y = 0;
    updatewin.width = widget->allocation.width;
    updatewin.height = widget->allocation.height;
    gtk_widget_draw ( widget, &updatewin);
  }

  /* Reset dots array */
  for( i=0; i<AZELMAXDOTS; i++ ) {
    azeldots[i].x=0;
    azeldots[i].y=0;
  }
  nazeldots=0;

  newsat=1;
  lastel=-90;

  if(autofreq==TRUE)
    lookup_sat_db();

/*    doprediction=TRUE; */
}

void
on_cb_connect_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), FALSE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_connect ), TRUE );
  gtk_widget_show( GTK_WIDGET( dialog_connect ) );
}


void
on_cb_disconnect_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;

  disconnect_server();
  widget=lookup_widget( mainwindow, "cb_disconnect" );
  gtk_widget_set_sensitive( widget, FALSE );
  widget=lookup_widget( mainwindow, "cb_connect" );
  gtk_widget_set_sensitive( widget, TRUE );
  widget=lookup_widget( mainwindow, "tb_footpsat" );
  gtk_widget_set_sensitive( widget, FALSE );
  widget=lookup_widget( mainwindow, "tb_footpqth" );
  gtk_widget_set_sensitive( widget, FALSE );
  widget=lookup_widget( mainwindow, "tb_track" );
  gtk_widget_set_sensitive( widget, FALSE );
  widget=lookup_widget( mainwindow, "tb_autofreq" );
  gtk_widget_set_sensitive( widget, FALSE );
  widget=lookup_widget( mainwindow, "bt_azelgraph" );
  gtk_widget_set_sensitive( widget, FALSE );
}


void
on_tb_grid_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) )
    drawgrid=TRUE;
  else
    drawgrid=FALSE;
}


void
on_tb_utctime_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) )
    timeUTC=TRUE;
  else
    timeUTC=FALSE;
}


void
on_bt_preferences_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget * widget;
  int state, n;

  /* refresh plugin list */
  search_radio_plugins();
  search_rotor_plugins();

  /* set up downlink plugin widget */
  widget = lookup_widget( dialog_preferences, "entry_downplugin" );  
  gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[0]);
  for(n=0; n<g_list_length( pluginlist ); n++)
    if( strncmp(prefs_down_plugin, pluginfiles[n],256)==0 ) {
      gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[n]);
      break;
    }
  
  /* set up downlink plugin config widget */
  widget = lookup_widget( dialog_preferences, "tx_downpluginconfig" );
  gtk_entry_set_text(GTK_ENTRY(widget),prefs_down_plugin_config);

  /* set up uplink plugin widget */
  widget = lookup_widget( dialog_preferences, "entry_upplugin" );  
  gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[0]);
  for(n=0; n<g_list_length( pluginlist ); n++)
    if( strncmp(prefs_up_plugin, pluginfiles[n],256)==0) {
      gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[n]);
      break;
    }

  /* set up uplink plugin config widget */
  widget = lookup_widget( dialog_preferences, "tx_uppluginconfig" );
  gtk_entry_set_text(GTK_ENTRY(widget),prefs_up_plugin_config);

  /* set up beacon plugin widget */
  widget = lookup_widget( dialog_preferences, "entry_beaconplugin" );  
  gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[0]);
  for(n=0; n<g_list_length( pluginlist ); n++)
    if( strncmp(prefs_beacon_plugin, pluginfiles[n],256)==0) {
      gtk_entry_set_text(GTK_ENTRY(widget),plugindescriptions[n]);
      break;
    }

  /* set up beacon plugin config widget */
  widget = lookup_widget( dialog_preferences, "tx_beaconpluginconfig" );
  gtk_entry_set_text(GTK_ENTRY(widget),prefs_beacon_plugin_config);

  /* set up rotor plugin widget */
  widget = lookup_widget( dialog_preferences, "entry_rotorplugin" );  
  gtk_entry_set_text(GTK_ENTRY(widget),rotorplugindescriptions[0]);
  for(n=0; n<g_list_length( rotorpluginlist ); n++)
    if( strncmp(prefs_rotor_plugin, rotorpluginfiles[n],256)==0) {
      gtk_entry_set_text(GTK_ENTRY(widget),rotorplugindescriptions[n]);
      break;
    }

  /* set up rotor plugin config widget */
  rotorpluginconfig=prefs_rotor_plugin_config;
  widget = lookup_widget( dialog_preferences, "tx_rotorpluginconfig" );
  gtk_entry_set_text(GTK_ENTRY(widget),rotorpluginconfig);

  /* set up grid widget */
  if( prefs_grid[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_gridon" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_gridoff" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up utctime widget */
  if( prefs_utctime[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_utc" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_local" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up satellite footprint widget */
  if( prefs_satfootprint[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_fpsaton" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_fpsatoff" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up qth footprint widget */
  if( prefs_qthfootprint[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_fpqthon" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_fpqthoff" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up track widget */
  if( prefs_track[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_trackon" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_trackoff" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up autofreq widget */
  if( prefs_autofreq[0]=='1' ) {
    widget = lookup_widget( dialog_preferences, "rd_prefs_autofreqon" );
    state=TRUE;
  }
  else {
    widget = lookup_widget( dialog_preferences, "rd_prefs_autofreqoff" );
    state=FALSE;
  }
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), TRUE );

  /* set up AOS and LOS command */
  widget = lookup_widget( dialog_preferences, "tx_aoscommand" );
  gtk_entry_set_text(GTK_ENTRY(widget),prefs_aos_command);
  widget = lookup_widget( dialog_preferences, "tx_loscommand" );
  gtk_entry_set_text(GTK_ENTRY(widget),prefs_los_command);

  /* Disable main window and show preferences dialog */
  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), FALSE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_preferences ), TRUE );
  gtk_widget_show( GTK_WIDGET( dialog_preferences ) );
}


void
on_bt_prefs_apply_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget * widget;
  char * plugin;
  int listidx;
  char filename[256];
  GList * list;
  int i,state;
  char * dummy;

  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_preferences ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_preferences ) );

  /* Setup downlink plugin */
  widget = lookup_widget( dialog_preferences, "entry_downplugin" );
  plugin=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_preferences, "tx_downpluginconfig" );
  downpluginconfig=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  list=g_list_first( pluginlist );
  listidx=0;
  for( i=0; i<g_list_length( pluginlist ); i++ ) {
    if(strncmp( list->data,plugin, 29 )==0) {
      listidx=i;
      break;
    }
    list=g_list_next( list );
  }
  if( listidx > 0 ) {
    strcpy( filename, pluginsdir );
    strcat( filename, pluginfiles[ listidx ] );
    open_downlink_plugin( filename );
  }
  strncpy( prefs_down_plugin, pluginfiles[ listidx ], 255 );
  strncpy( prefs_down_plugin_config, downpluginconfig, 255 );

  /* Setup uplink plugin */
  widget = lookup_widget( dialog_preferences, "entry_upplugin" );
  plugin=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_preferences, "tx_uppluginconfig" );
  uppluginconfig=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  list=g_list_first( pluginlist );
  listidx=0;
  for( i=0; i<g_list_length( pluginlist ); i++ ) {
    if(strncmp( list->data,plugin, 29 )==0) {
      listidx=i;
      break;
    }
    list=g_list_next( list );
  }
  if( listidx > 0 ) {
    strcpy( filename, pluginsdir );
    strcat( filename, pluginfiles[ listidx ] );
    open_uplink_plugin( filename );
  }
  strncpy( prefs_up_plugin, pluginfiles[ listidx ], 255 );
  strncpy( prefs_up_plugin_config, uppluginconfig, 255 );

  /* Setup beacon plugin */
  widget = lookup_widget( dialog_preferences, "entry_beaconplugin" );
  plugin=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_preferences, "tx_beaconpluginconfig" );
  beaconpluginconfig=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  list=g_list_first( pluginlist );
  listidx=0;
  for( i=0; i<g_list_length( pluginlist ); i++ ) {
    if(strncmp( list->data,plugin, 29 )==0) {
      listidx=i;
      break;
    }
    list=g_list_next( list );
  }
  if( listidx > 0 ) {
    strcpy( filename, pluginsdir );
    strcat( filename, pluginfiles[ listidx ] );
    open_beacon_plugin( filename );
  }
  strncpy( prefs_beacon_plugin, pluginfiles[ listidx ], 255 );
  strncpy( prefs_beacon_plugin_config, beaconpluginconfig, 255 );

  /* Setup rotor plugin */
  widget = lookup_widget( dialog_preferences, "entry_rotorplugin" );
  plugin=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_preferences, "tx_rotorpluginconfig" );
  rotorpluginconfig=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  list=g_list_first( rotorpluginlist );
  listidx=0;
  for( i=0; i<g_list_length( rotorpluginlist ); i++ ) {
    if(strncmp( list->data,plugin, 29 )==0) {
      listidx=i;
      break;
    }
    list=g_list_next( list );
  }
  if( listidx > 0 ) {
    strcpy( filename, pluginsdir );
    strcat( filename, rotorpluginfiles[ listidx ] );
    open_rotor_plugin( filename );
  }
  strncpy( prefs_rotor_plugin, rotorpluginfiles[ listidx ], 255 );
  strncpy( prefs_rotor_plugin_config, rotorpluginconfig, 255 );

  /* Setup program options */
  widget = lookup_widget( dialog_preferences, "rd_prefs_gridon" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_grid[0]='1';
  }
  else {
    state=FALSE;
    prefs_grid[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_grid" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  widget = lookup_widget( dialog_preferences, "rd_prefs_utc" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_utctime[0]='1';
  }
  else {
    state=FALSE;
    prefs_utctime[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_utctime" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  widget = lookup_widget( dialog_preferences, "rd_prefs_fpsaton" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_satfootprint[0]='1';
  }
  else {
    state=FALSE;
    prefs_satfootprint[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_footpsat" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  widget = lookup_widget( dialog_preferences, "rd_prefs_fpqthon" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_qthfootprint[0]='1';
  }
  else {
    state=FALSE;
    prefs_qthfootprint[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_footpqth" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  widget = lookup_widget( dialog_preferences, "rd_prefs_trackon" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_track[0]='1';
  }
  else {
    state=FALSE;
    prefs_track[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_track" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  widget = lookup_widget( dialog_preferences, "rd_prefs_autofreqon" );
  if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( widget ) ) == TRUE ) {
    state=TRUE;
    prefs_autofreq[0]='1';
    if(connected==TRUE)
      lookup_sat_db();
  }
  else {
    state=FALSE;
    prefs_autofreq[0]='0';
  }
  widget=lookup_widget( mainwindow, "tb_autofreq" );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( widget ), state );

  /* Setup AOS and LOS commands */
  widget = lookup_widget( dialog_preferences, "tx_aoscommand" );
  dummy=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  strncpy( prefs_aos_command, dummy, 1024 );
  widget = lookup_widget( dialog_preferences, "tx_loscommand" );
  dummy=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  strncpy( prefs_los_command, dummy, 1024 );
}

void
on_bt_prefs_save_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  on_bt_prefs_apply_clicked( button, NULL );

  save_preferences();
}

void
on_bt_prefs_cancel_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_preferences ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_preferences ) );
}


void
on_bt_con_connect_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget * widget;

  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_connect ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_connect ) );

  widget = lookup_widget( dialog_connect, "tx_con_server" );
  predicthost=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_connect, "tx_con_port" );
  predictport=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  /* Setup status bar */
  sprintf(statusmsg,"Connecting to predict server %s ...", predicthost);
  show_status(statusmsg);

  if(connect_server()==0) {
    /* if connected, disable connect button enable disconnect button */
    widget=lookup_widget( mainwindow, "cb_connect" );
    gtk_widget_set_sensitive( widget, FALSE );
    widget=lookup_widget( mainwindow, "cb_disconnect" );
    gtk_widget_set_sensitive( widget, TRUE );
    widget=lookup_widget( mainwindow, "tb_footpsat" );
    gtk_widget_set_sensitive( widget, TRUE );
    widget=lookup_widget( mainwindow, "tb_footpqth" );
    gtk_widget_set_sensitive( widget, TRUE );
    widget=lookup_widget( mainwindow, "tb_track" );
    gtk_widget_set_sensitive( widget, TRUE );
    widget=lookup_widget( mainwindow, "tb_autofreq" );
    gtk_widget_set_sensitive( widget, TRUE );
    widget=lookup_widget( mainwindow, "bt_azelgraph" );
    gtk_widget_set_sensitive( widget, TRUE );

    connected=TRUE;
  }
  else {
    show_status("Error connecting to predict server.");
    connected=FALSE;
  }
}


void
on_bt_con_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_connect ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_connect ) );
}


gboolean
on_dialog_preferences_delete_event     (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_preferences ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_preferences ) );

  return TRUE;
}


gboolean
on_dialog_connect_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  gtk_widget_set_sensitive( GTK_WIDGET( mainwindow ), TRUE );
  gtk_widget_set_sensitive( GTK_WIDGET( dialog_connect ), FALSE );
  gtk_widget_hide( GTK_WIDGET( dialog_connect ) );

  return TRUE;
}


void
on_tb_footpsat_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) )
    satfootprint=TRUE;
  else
    satfootprint=FALSE;
}


void
on_tb_footpqth_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) )
    qthfootprint=TRUE;
  else
    qthfootprint=FALSE;
}


void
on_tb_track_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) )
    drawtrack=TRUE;
  else
    drawtrack=FALSE;
}


void
on_cb_updoppler_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) ) {
    if( uppluginenable==TRUE ) {
      if( (*plugin_open_rig_uplink)( uppluginconfig )==TRUE )
	enableupdoppler=TRUE;
      else
	error_dialog("Error opening uplink plugin");
    }
  }
  else {
    if( uppluginenable==TRUE ) {
      (*plugin_close_rig_uplink)();
      enableupdoppler=FALSE;
    }
  }
}


void
on_cb_dwdoppler_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) ) {
    if( downpluginenable==TRUE ) {
      if( (*plugin_open_rig_downlink)( downpluginconfig )==TRUE )
	enabledowndoppler=TRUE;
      else
	error_dialog("Error opening downlink plugin");
    }
  }
  else {
    if( downpluginenable==TRUE ) {
      (*plugin_close_rig_downlink)();
      enabledowndoppler=FALSE;
    }
  }
}


void
on_cb_beacondoppler_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) ) {
    if( beaconpluginenable==TRUE ) {
      if( (*plugin_open_rig_beacon)( beaconpluginconfig )==TRUE )
	enablebeacondoppler=TRUE;
      else
	error_dialog("Error opening beacon plugin");
    }
  }
  else {
    if( beaconpluginenable==TRUE ) {
      (*plugin_close_rig_beacon)();
      enablebeacondoppler=FALSE;
    }
  }
}


gboolean
on_dialog_azel_graph_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  GtkWidget *mywidget;


  gtk_widget_hide( GTK_WIDGET( dialog_azel_graph ) );
  mywidget=lookup_widget( mainwindow, "bt_azelgraph" );
  gtk_widget_set_sensitive( mywidget, TRUE );
  azelgraph=FALSE;

  return TRUE;
}


gboolean
on_azelgraph_expose_event              (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  /* Copy internal buffer to screen area */
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  drawazel,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

void
on_tb_autofreq_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) ) {
    autofreq=TRUE;
    if(connected==TRUE)
      lookup_sat_db();
  }
  else
    autofreq=FALSE;
}


void
on_bt_azelgraph_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;

  gtk_widget_show(dialog_azel_graph);
  widget=lookup_widget( mainwindow, "bt_azelgraph" );
  gtk_widget_set_sensitive( widget, FALSE );
  azelgraph=TRUE;
}

void
on_bt_dbedit_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;
  char filename[256], inbuf[50], *bp, *row[5];
  FILE * fd;

  widget=lookup_widget( dialog_dbedit, "clist_satdb" );
  gtk_clist_clear(GTK_CLIST(widget));
  gtk_clist_set_sort_type(GTK_CLIST(widget),GTK_SORT_ASCENDING);
  gtk_clist_set_auto_sort(GTK_CLIST(widget),TRUE);

  strncpy(filename,getenv("HOME"),256);
  strcat(filename,"/.gsat.db");

  if((fd=fopen( filename, "r" ))==NULL) {
    /* If db file doesn't exist in HOME, copy it from global sat db */
    copy_sat_db(filename);
    if((fd=fopen( filename, "r" ))==NULL) {
      error_dialog("ERROR: Can't open satellite database file");
      return;
    }
  }

  while(fgets(inbuf, 50, fd)) {
    bp=inbuf;
    row[0]=strsep(&bp,":\n");
    row[1]=strsep(&bp,":\n");
    row[2]=strsep(&bp,":\n");
    row[3]=strsep(&bp,":\n");
    row[4]=strsep(&bp,":\n");
    gtk_clist_append(GTK_CLIST(widget), row);
  }
  fclose( fd );

  gtk_widget_show(dialog_dbedit);
  widget=lookup_widget( mainwindow, "bt_dbedit" );
  gtk_widget_set_sensitive( widget, FALSE );
}

void
on_cb_rotor_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if( gtk_toggle_button_get_active( togglebutton ) ) {
    if( rotorpluginenable==TRUE ) {
      if( (*plugin_open_rotor)( rotorpluginconfig )==TRUE )
	enablerotor=TRUE;
      else
	error_dialog("Error opening rotor plugin");
    }
  }
  else {
    if( rotorpluginenable==TRUE ) {
      (*plugin_close_rotor)();
      enablerotor=FALSE;
    }
  }
}

void
on_clist_satdb_select_row              (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  GtkWidget *widget;
  char *sat, *mode, *uplink, *downlink, *beacon;

  selectedrow = row;

  gtk_clist_get_text(clist, row, 0, &sat);
  gtk_clist_get_text(clist, row, 1, &mode);
  gtk_clist_get_text(clist, row, 2, &uplink);
  gtk_clist_get_text(clist, row, 3, &downlink);
  gtk_clist_get_text(clist, row, 4, &beacon);
  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  gtk_entry_set_text(GTK_ENTRY(widget), sat);
  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  gtk_entry_set_text(GTK_ENTRY(widget), mode);
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  gtk_entry_set_text(GTK_ENTRY(widget), uplink);
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  gtk_entry_set_text(GTK_ENTRY(widget), downlink);
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  gtk_entry_set_text(GTK_ENTRY(widget), beacon);
}


void
on_bt_dbadd_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;
  char *row[5];

  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  row[0]=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  if(strlen(row[0])==0)
     return;

  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  row[1]=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  row[2]=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  row[3]=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  row[4]=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  widget=lookup_widget( dialog_dbedit, "clist_satdb" );
  gtk_clist_append(GTK_CLIST(widget), row);

  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  save_sat_db();
}


void
on_bt_dbupdate_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;
  char *sat, *mode, *uplink, *downlink, *beacon;

  if(selectedrow < 0)
    return;

  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  sat=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  mode=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  uplink=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  downlink=(char *)gtk_entry_get_text(GTK_ENTRY(widget));
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  beacon=(char *)gtk_entry_get_text(GTK_ENTRY(widget));

  widget=lookup_widget( dialog_dbedit, "clist_satdb" );
  gtk_clist_set_text(GTK_CLIST(widget), selectedrow, 0, sat);
  gtk_clist_set_text(GTK_CLIST(widget), selectedrow, 1, mode);
  gtk_clist_set_text(GTK_CLIST(widget), selectedrow, 2, uplink);
  gtk_clist_set_text(GTK_CLIST(widget), selectedrow, 3, downlink);
  gtk_clist_set_text(GTK_CLIST(widget), selectedrow, 4, beacon);
  gtk_clist_unselect_row(GTK_CLIST(widget), selectedrow, 0);

  selectedrow=-1;

  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  save_sat_db();
}


void
on_bt_dbdelete_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;

  if(selectedrow < 0)
    return;

  widget=lookup_widget( dialog_dbedit, "clist_satdb" );
  gtk_clist_remove(GTK_CLIST(widget), selectedrow);

  widget = lookup_widget( dialog_dbedit, "tx_dbsat" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbmode" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbuplink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbdownlink" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  widget = lookup_widget( dialog_dbedit, "tx_dbbeacon" );
  gtk_entry_set_text(GTK_ENTRY(widget), "");

  save_sat_db();
}

gboolean
on_dialog_dbedit_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  GtkWidget *mywidget;


  gtk_widget_hide( GTK_WIDGET( dialog_dbedit ) );
  mywidget=lookup_widget( mainwindow, "bt_dbedit" );
  gtk_widget_set_sensitive( mywidget, TRUE );

  return TRUE;
}

void
on_bt_about_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_show( GTK_WIDGET(dialog_about) );
}

int
compare_mode(const gpointer *mode1, const gpointer *mode2 )
{
  return( strcmp( (char *)mode1, (char *)mode2 ));
}

void
combo_tx_mode_changed                  (GtkEditable     *editable,
                                        gpointer         user_data)
{
  GtkWidget *widget;
  char *mode, *upfreq, *dwfreq, *bcnfreq;
  GList *lmode;
  gint n;

  widget = lookup_widget( mainwindow, "combo_mode" );
  mode = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));

  modelist=g_list_first( modelist );
  lmode = g_list_find_custom( modelist, mode, (GCompareFunc)&compare_mode );
  n=g_list_position( modelist, lmode );
  if( n > -1 ) {
    upfreq=g_list_nth_data( uplinklist, n );
    dwfreq=g_list_nth_data( downlinklist, n );
    bcnfreq=g_list_nth_data( beaconlist, n );
    widget = lookup_widget( mainwindow, "tx_uplink" );
    gtk_entry_set_text(GTK_ENTRY(widget),upfreq);
    widget = lookup_widget( mainwindow, "tx_downlink" );
    gtk_entry_set_text(GTK_ENTRY(widget),dwfreq);
    widget = lookup_widget( mainwindow, "tx_beacon" );
    gtk_entry_set_text(GTK_ENTRY(widget),bcnfreq);
  }
  else {
    widget = lookup_widget( mainwindow, "tx_uplink" );
    gtk_entry_set_text(GTK_ENTRY(widget),"");
    widget = lookup_widget( mainwindow, "tx_downlink" );
    gtk_entry_set_text(GTK_ENTRY(widget),"");
    widget = lookup_widget( mainwindow, "tx_beacon" );
    gtk_entry_set_text(GTK_ENTRY(widget),"");
  }
}


gboolean
on_dialog_about_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_widget_hide( GTK_WIDGET(dialog_about) );

  return TRUE;
}


void
on_bt_about_ok_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_widget_hide( GTK_WIDGET(dialog_about) );
}







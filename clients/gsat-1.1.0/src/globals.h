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

/* Global definitions */

#define GSATVERSION "1.1.0"
#define WINTITLE(VERS)  "gsat #VERS: Real-Time Satellite Tracking Display"

#ifndef GSATLIBDIR
#define GSATLIBDIR "/usr/local/lib/gsat"
#endif

#define MAXPLUGINS 20
#define DEFAULTPLUGINSDIR GSATLIBDIR"/plugins/"

/* Buffer size for network reads */
#define NETBUFSIZE 1024

/* Network timeout */
#define NETTIMEOUT 10

/* Network maximum number of timeouts */
#define MAXTIMEOUTS 3

/* Size of world map */
#define MAPSIZEX 700
#define MAPSIZEY 350

/* Maximum number of dots for drawing orbits */
#define MAXDOTS 2800

/* Size of az/el graphic */
#define AZELSIZEX 250
#define AZELSIZEY 250

/* Maximum number of dots for drawing az/el graphic */
#define AZELMAXDOTS AZELSIZEX*2

int errno;

/* Network parameters */
char *predicthost;
char *predictport;
int netsocket;

/* Main Widgets */
GtkWidget * mainwindow;
GtkWidget * dialog_connect;
GtkWidget * dialog_preferences;
GtkWidget * dialog_azel_graph;
GtkWidget * dialog_dbedit;
GtkWidget * dialog_about;

/* Main control parameters */
int connected;
int timeUTC;
int drawgrid;
int satfootprint;
int qthfootprint;
int drawtrack;
int autofreq;
int azelgraph;
int enableupdoppler;
int enabledowndoppler;
char predictversion[10];
GList *satlist;
char statusmsg[256];
float lastel;
gint selectedrow;
GList *modelist;
GList *uplinklist;
GList *downlinklist;
GList *beaconlist;

/* Graphics parameters */
GdkPixmap *drawmap;
GdkPixmap *sourcemap;
GdkPoint dots[MAXDOTS];
gint ndots;
GdkFont *drawfont;
GdkColor yellowclr, redclr, purpleclr, cyanclr, blueclr;
GdkGC *yellow_gc, *red_gc, *purple_gc, *cyan_gc, *blue_gc;
GdkPixmap *drawazel;
GdkPixmap *sourceazel;
GdkPoint azeldots[AZELMAXDOTS];
gint nazeldots;

/* Program flags */
int doprediction;
int newsat;

/* Plugin interface functions */
char * (*plugin_info_uplink)( void );
int (*plugin_open_rig_uplink)( char * config );
void (*plugin_close_rig_uplink)( void );
void (*plugin_set_uplink_frequency)( double frequency );
char * (*plugin_info_downlink)( void );
int (*plugin_open_rig_downlink)( char * config );
void (*plugin_close_rig_downlink)( void );
void (*plugin_set_downlink_frequency)( double frequency );
char * (*plugin_info_beacon)( void );
int (*plugin_open_rig_beacon)( char * config );
void (*plugin_close_rig_beacon)( void );
void (*plugin_set_beacon_frequency)( double frequency );
char * (*plugin_info_rotor)( void );
int (*plugin_open_rotor)( char * config );
void (*plugin_close_rotor)( void );
void (*plugin_set_rotor)( double azimuth, double elevation );

/* Plugin control parameters */
int uppluginenable;
int enableupdoppler;
char * uppluginconfig;
void * upplugin_handle;

int downpluginenable;
int enabledowndoppler;
char * downpluginconfig;
void * downplugin_handle;

int beaconpluginenable;
int enablebeacondoppler;
char * beaconpluginconfig;
void * beaconplugin_handle;

int rotorpluginenable;
int enablerotor;
char * rotorpluginconfig;
void * rotorplugin_handle;

GList *pluginlist;
char pluginfiles[MAXPLUGINS][30];
char plugindescriptions[MAXPLUGINS][30];
char pluginsdir[256];

GList *rotorpluginlist;
char rotorpluginfiles[MAXPLUGINS][30];
char rotorplugindescriptions[MAXPLUGINS][30];

/* Preferences */
char prefs_grid[1];
char prefs_utctime[1];
char prefs_satfootprint[1];
char prefs_qthfootprint[1];
char prefs_track[1];
char prefs_autofreq[1];
char prefs_aos_command[1024];
char prefs_los_command[1024];
char prefs_up_plugin[256];
char prefs_up_plugin_config[256];
char prefs_down_plugin[256];
char prefs_down_plugin_config[256];
char prefs_beacon_plugin[256];
char prefs_beacon_plugin_config[256];
char prefs_rotor_plugin[256];
char prefs_rotor_plugin_config[256];

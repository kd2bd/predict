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
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "interface.h"
#include "support.h"

#include "comms.h"
#include "graphics.h"
#include "prefs.h"
#include "plugins.h"

#include "globals.h"

int
main (int argc, char *argv[])
{
  GtkWidget *maparea;
  GtkWidget *widget;
  GtkAdjustment *vadjust;
  char servername[256];
  int optn;
  gpointer *satname;
  GdkPixmap *ao27pixmap;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

/*    gtk_set_locale (); */
  gtk_init (&argc, &argv);

  /* Set a small font size */
  gtk_rc_parse_string("style \"customfonts\" \n{\n font = \"-adobe-helvetica-medium-r-normal-*-10-*-*-*-p-*-iso8859-1\"\n }\n widget_class \"*GtkLabel\" style \"customfonts\"\n widget_class \"*GtkEntry\" style \"customfonts\"\n");

  /* default time mode = localtime */
  timeUTC=FALSE;
  drawgrid=FALSE;

  /* Get hostname of the predict server from ENV variable */
  if( getenv("PREDICTHOST")!=NULL ) {
    strncpy( servername, getenv("PREDICTHOST"),255 );
    servername[255]='\0';
    predicthost=servername;
  }
  else {
    strcpy( servername, "localhost" );
    predicthost=servername;
  }

  /* Get plugins directory from ENV variable */
  if( getenv("GSATPLUGINSDIR")!=NULL ) {
    strncpy( pluginsdir, getenv("GSATPLUGINSDIR"),255 );
    pluginsdir[255]='\0';
  }
  else {
    strcpy( pluginsdir, DEFAULTPLUGINSDIR );
  }

  /* Parse command line options */
  optn = 1;
  if( argc > 1 )
    while( optn < argc ) {
      if( argv[optn][0] == '-' )
	switch (argv[optn][1]) {
	case 'u':
	  timeUTC = TRUE;
	  break;
	case 'l':
	  timeUTC = FALSE;
	  break;
	case 's':
	  if( argc > optn+1 ) {
	    strncpy( servername, argv[optn+1],255 );
	    servername[255]='\0';
	    predicthost=servername;
	    optn++;
	  }
	  else {
	    fprintf( stderr, "ERROR: you must specify a predict host name.\n");
	    exit( 1 );
	  }
	  break;
	case 'g':
	  drawgrid=TRUE;
	  break;
	case 'h':
	default:
	  printf("Usage: gsat [-u] [-l] [-g] [-h] [-s predictservername]\n");
	  exit( 1 );
	}
      optn++;
    }

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /* Load drawing font */
  drawfont=gdk_font_load("-adobe-helvetica-medium-r-normal-*-8-*-*-*-p-*-iso8859-1");

  /*
   * Main window creation and graphics initialization
   * 
   */

  mainwindow = create_mainwindow();
  dialog_connect = create_dialog_connect();
  dialog_preferences = create_dialog_preferences();
  dialog_azel_graph = create_dialog_azel_graph();
  dialog_dbedit = create_dialog_dbedit();
  dialog_about = create_dialog_about();

  /* Set program version in main window title and about dialog */
  gtk_window_set_title( GTK_WINDOW(mainwindow), 
			"gsat "GSATVERSION": Real-Time Satellite Tracking Display");
  widget=lookup_widget( dialog_about, "lb_about_version" );
  gtk_label_set_text(GTK_LABEL(widget),"Version "GSATVERSION);

  /* Set initial status of control buttons */
  widget=lookup_widget( mainwindow, "cb_connect" );
  gtk_widget_set_sensitive( widget, TRUE );
  widget=lookup_widget( mainwindow, "cb_disconnect" );
  gtk_widget_set_sensitive( widget, FALSE );
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
  widget=lookup_widget( mainwindow, "bt_dbedit" );
  gtk_widget_set_sensitive( widget, TRUE );

  /* Now we are ready to show the main window */
  gtk_widget_show(mainwindow);

  /* Setup status bar */
  show_status("Not connected");

  /* Satellite DB edit window setup */
  widget=lookup_widget( dialog_dbedit, "vscrollbar_satdb" );
  vadjust=gtk_range_get_adjustment(GTK_RANGE(widget));
  widget=lookup_widget( dialog_dbedit, "clist_satdb" );
  gtk_clist_set_vadjustment(GTK_CLIST(widget), vadjust);
  gtk_clist_column_titles_passive(GTK_CLIST(widget));
  selectedrow=-1;

  /* Init graphic buffers */
  /* Create Source map */
  maparea=lookup_widget( mainwindow, "maparea" );
  sourcemap = gdk_pixmap_create_from_xpm_d( maparea->window, NULL, NULL, worldmap );

  /* Create the draw map */
  drawmap = gdk_pixmap_new( maparea->window, MAPSIZEX, MAPSIZEY, -1 );

  /* Create the az/el draw area */
  sourceazel = gdk_pixmap_new( maparea->window, AZELSIZEX, AZELSIZEY, -1 );
  drawazel = gdk_pixmap_new( maparea->window, AZELSIZEX, AZELSIZEY, -1 );

  widget=lookup_widget( dialog_about, "about_pixmap" );
  ao27pixmap = gdk_pixmap_create_from_xpm_d(mainwindow->window, NULL, NULL, (gchar **)&ao27_small_xpm );
  gtk_pixmap_set( GTK_PIXMAP(widget), ao27pixmap, NULL );

  /* setup some graphics contexts to draw to drawmap */
  yellowclr.red=0xffff;
  yellowclr.green=0xffff;
  yellowclr.blue=0x0000;
  gdk_color_alloc(gdk_colormap_get_system (), &yellowclr);
  yellow_gc = gdk_gc_new( drawmap );
  gdk_gc_set_foreground( yellow_gc, &yellowclr );
  redclr.red=0xaaaa;
  redclr.green=0x0000;
  redclr.blue=0x0000;
  gdk_color_alloc(gdk_colormap_get_system (), &redclr);
  red_gc = gdk_gc_new( drawmap );
  gdk_gc_set_foreground( red_gc, &redclr );
  purpleclr.red=0xffff;
  purpleclr.green=0x0000;
  purpleclr.blue=0xffff;
  gdk_color_alloc(gdk_colormap_get_system (), &purpleclr);
  purple_gc = gdk_gc_new( drawmap );
  gdk_gc_set_foreground( purple_gc, &purpleclr );
  cyanclr.red=0x0000;
  cyanclr.green=0xffff;
  cyanclr.blue=0xffff;
  gdk_color_alloc(gdk_colormap_get_system (), &cyanclr);
  cyan_gc = gdk_gc_new( drawmap );
  gdk_gc_set_foreground( cyan_gc, &cyanclr );
  blueclr.red=0x0000;
  blueclr.green=0x8888;
  blueclr.blue=0xffff;
  gdk_color_alloc(gdk_colormap_get_system (), &blueclr);
  blue_gc = gdk_gc_new( drawmap );
  gdk_gc_set_foreground( blue_gc, &blueclr );

  /* Draw initial AZ/EL graphic */
  gdk_draw_rectangle( sourceazel, mainwindow->style->white_gc, TRUE, 0, 0, AZELSIZEX, AZELSIZEY );
  gdk_draw_arc( sourceazel, mainwindow->style->black_gc, FALSE, 0, 0,
		AZELSIZEX-1, AZELSIZEY-1, 0, 23040);
  gdk_draw_arc( sourceazel, mainwindow->style->black_gc, FALSE,
		AZELSIZEX/2/3, AZELSIZEY/2/3,
		AZELSIZEX*2/3, AZELSIZEY*2/3, 0, 23040);
  gdk_draw_arc( sourceazel, mainwindow->style->black_gc, FALSE,
		AZELSIZEX/3, AZELSIZEY/3,
		AZELSIZEX/3, AZELSIZEY/3, 0, 23040);
  gdk_draw_line( sourceazel, red_gc, 0, AZELSIZEY/2, AZELSIZEX, AZELSIZEY/2 );
  gdk_draw_line( sourceazel, red_gc, AZELSIZEX/2, 0, AZELSIZEX/2, AZELSIZEY );
  gdk_draw_string( sourceazel, drawfont, mainwindow->style->black_gc, AZELSIZEX/2+2, 8, "N" );
  gdk_draw_string( sourceazel, drawfont, mainwindow->style->black_gc, AZELSIZEX/2+2, AZELSIZEY-4, "S" );
  gdk_draw_string( sourceazel, drawfont, mainwindow->style->black_gc, 2, AZELSIZEY/2-4, "W" );
  gdk_draw_string( sourceazel, drawfont, mainwindow->style->black_gc, AZELSIZEX-10, AZELSIZEY/2-4, "E" );
  azelgraph=FALSE;

  /* copy original map to draw map */
  gdk_draw_pixmap( drawmap, yellow_gc, sourcemap, 0, 0, 0, 0, MAPSIZEX, MAPSIZEY );

  /* Initialize plugins */
  search_radio_plugins();
  search_rotor_plugins();

  /* Open preferences */
  open_preferences();

  /* enable processing */
  gtk_timeout_add( 500, timeout_callback, mainwindow );

  /* go into main loop */
  gtk_main ();

  /* don't leave plugins open */
  close_uplink_plugin();
  close_downlink_plugin();

  /* if we are connected, close network socket */
  if( connected == TRUE ) {
    /* Empty the satellite list and free memory */
    satlist=g_list_first( satlist );
    while( g_list_length( satlist ) > 0 ) {
      satname=satlist->data;
      satlist=g_list_remove( satlist, satname );
      g_free( satname );
    }
    close(netsocket);
  }

  return 0;
}

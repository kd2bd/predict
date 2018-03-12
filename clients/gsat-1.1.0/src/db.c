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

/* Satellite database functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "support.h"
#include "comms.h"

#include "globals.h"

void lookup_sat_db( void )
{
  char filename[256], inbuf[50], *bp, *satn, *moden, *upfreqn, *dwfreqn, *bcnfreqn,
    satname[26], *sat, *mode, *upfreq, *dwfreq, *bcnfreq, *item;
  FILE * fd;
  GtkWidget *widget, *newitem;
  int found, i, itemlistlen;
  GList *itemlist=NULL;

  upfreq=dwfreq=bcnfreq=NULL;

  /* Clear mode combo */
  widget=lookup_widget( mainwindow, "combo_mode" );
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(widget)->entry), "");
  gtk_list_clear_items(GTK_LIST(GTK_COMBO(widget)->list), 0, -1);

  /* Empty the mode and frequencies lists and free memory */
  modelist=g_list_first( modelist );
  while( g_list_length( modelist ) > 0 ) {
    mode=modelist->data;
    modelist=g_list_remove( modelist, mode );
    g_free( mode );
  }
  uplinklist=g_list_first( uplinklist );
  while( g_list_length( uplinklist ) > 0 ) {
    upfreq=uplinklist->data;
    uplinklist=g_list_remove( uplinklist, upfreq );
    g_free( upfreq );
  }
  downlinklist=g_list_first( downlinklist );
  while( g_list_length( downlinklist ) > 0 ) {
    dwfreq=downlinklist->data;
    downlinklist=g_list_remove( downlinklist, dwfreq );
    g_free( dwfreq );
  }
  beaconlist=g_list_first( beaconlist );
  while( g_list_length( beaconlist ) > 0 ) {
    bcnfreq=beaconlist->data;
    beaconlist=g_list_remove( beaconlist, bcnfreq );
    g_free( bcnfreq );
  }
  itemlist=g_list_first( itemlist );
  while( g_list_length( itemlist ) > 0 ) {
    item=itemlist->data;
    modelist=g_list_remove( itemlist, item );
    g_free( item );
  }

  /* Get Satellite name from combo entry */
  widget = lookup_widget( mainwindow, "combo" );
  sat = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));
  strncpy( satname, sat, 25 );
  satname[ 26 ] = '\0';

  strncpy(filename,getenv("HOME"),256);
  strcat(filename,"/.gsat.db");

  if((fd=fopen( filename, "r" ))==NULL) {
    strncpy(filename,GSATLIBDIR"/gsat.db",256);
    if((fd=fopen( filename, "r" ))==NULL)
      return;
  }

  found=0;
  while(fgets(inbuf, 50, fd)) {
    bp=inbuf;
    satn=strsep(&bp,":\n");
    moden=strsep(&bp,":\n");
    upfreqn=strsep(&bp,":\n");
    dwfreqn=strsep(&bp,":\n");
    bcnfreqn=strsep(&bp,":\n");
    if(strncmp(satname,satn,26)==0) {
      mode=g_malloc0(strlen(moden)+1);
      strncpy((char *)mode,(const char*)moden,strlen(moden));
      modelist=g_list_append(modelist,mode);
      upfreq=g_malloc0(strlen(upfreqn)+1);
      strncpy((char *)upfreq,(const char*)upfreqn,strlen(upfreqn));
      uplinklist=g_list_append(uplinklist,upfreq);
      dwfreq=g_malloc0(strlen(dwfreqn)+1);
      strncpy((char *)dwfreq,(const char*)dwfreqn,strlen(dwfreqn));
      downlinklist=g_list_append(downlinklist,dwfreq);
      bcnfreq=g_malloc0(strlen(bcnfreqn)+1);
      strncpy((char *)bcnfreq,(const char*)bcnfreqn,strlen(bcnfreqn));
      beaconlist=g_list_append(beaconlist,bcnfreq);
      item=g_malloc0(strlen(moden)+1);
      strncpy((char *)item,(const char*)moden,strlen(moden));
      itemlist=g_list_append(itemlist,item);

      found=1;
    }
  }
  fclose( fd );

  /* Attach mode list */
  itemlistlen=g_list_length( itemlist );
  if( itemlistlen > 0 ) {
    itemlist=g_list_first( itemlist );
    widget=lookup_widget( mainwindow, "combo_mode" );
    for( i=0; i < itemlistlen; i++ ) {
      newitem=gtk_list_item_new_with_label(itemlist->data);
      gtk_widget_show(newitem);
      gtk_container_add(GTK_CONTAINER(GTK_COMBO(widget)->list), newitem);
      itemlist=g_list_next(itemlist);
    }
  }

  if(found==1) {
      widget = lookup_widget( mainwindow, "tx_uplink" );
      gtk_entry_set_text(GTK_ENTRY(widget),g_list_first(uplinklist)->data);
      widget = lookup_widget( mainwindow, "tx_downlink" );
      gtk_entry_set_text(GTK_ENTRY(widget),g_list_first(downlinklist)->data);
      widget = lookup_widget( mainwindow, "tx_beacon" );
      gtk_entry_set_text(GTK_ENTRY(widget),g_list_first(beaconlist)->data);
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

void save_sat_db( void )
{
  GtkWidget *clist;
  char filename[256], *sat, *mode, *uplink, *downlink, *beacon;
  FILE * fd;
  int nrows, n;

  clist=lookup_widget( dialog_dbedit, "clist_satdb" );
  nrows = GTK_CLIST(clist)->rows;

  strncpy(filename,getenv("HOME"),256);
  strcat(filename,"/.gsat.db");

  fd=fopen( filename, "w" );

  for(n=0; n < nrows; n++) {
    gtk_clist_get_text(GTK_CLIST(clist), n, 0, &sat);
    gtk_clist_get_text(GTK_CLIST(clist), n, 1, &mode);
    gtk_clist_get_text(GTK_CLIST(clist), n, 2, &uplink);
    gtk_clist_get_text(GTK_CLIST(clist), n, 3, &downlink);
    gtk_clist_get_text(GTK_CLIST(clist), n, 4, &beacon);
    fputs(sat, fd);
    fputc(':', fd);
    fputs(mode, fd);
    fputc(':', fd);
    fputs(uplink, fd);
    fputc(':', fd);
    fputs(downlink, fd);
    fputc(':', fd);
    fputs(beacon, fd);
    fputc('\n', fd);
  }

  fclose( fd );
}

void copy_sat_db( char *filename )
{
  FILE *fdin, *fdout;
  char buf[50];

  if((fdout=fopen( filename, "w" ))==NULL) {
    error_dialog("ERROR: Can't create satellite database file");
    return;
  }

    /* If global satellite db file does exist, create a copy in HOME */
  if((fdin=fopen( GSATLIBDIR"/gsat.db", "r" ))!=NULL) {
    while(fgets(buf, 50, fdin)) {
      fputs(buf,fdout);
    }
    fclose( fdin );
  }
  
  fclose( fdout );
}

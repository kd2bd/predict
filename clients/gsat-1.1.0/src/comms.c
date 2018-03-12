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

/* Main processing loop and network functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <math.h>
#include <dlfcn.h>
#include <dirent.h>

#include "support.h"
#include "comms.h"

#include "globals.h"

#ifndef PI
#define PI 3.141592653589793
#endif

void on_cb_disconnect_clicked(void);

double arccos(x,y)
double x, y;
{
  /* This function implements the arccosine function,
     returning a value between 0 and two pi. */

  double result=0.0, fraction;

  fraction=x/y;

  if ((x>0.0) && (y>0.0))
    result=acos(fraction);

  if ((x<0.0) && (y>0.0))
    result=acos(fraction);

  if ((x<0.0) && (y<0.0))
    result=PI+acos(fraction);

  if ((x>0.0) && (y<0.0))
    result=PI+acos(fraction);

  return result;
}

/* Function to open a dialog box displaying error messages. */

void error_dialog(gchar *message)
{

  GtkWidget *dialog, *frame, *box, *vbox1, *vbox2, *vbox3, *label, *okay_button;

  /* Create the widgets */

  dialog = gtk_dialog_new();
  frame = gtk_frame_new(NULL);
  box = gtk_vbox_new( TRUE, 2 );
  vbox1 = gtk_vbox_new( FALSE, 2 );
  vbox2 = gtk_vbox_new( TRUE, 2 );
  vbox3 = gtk_vbox_new( FALSE, 2 );
  label = gtk_label_new (message);
  okay_button = gtk_button_new_with_label("Okay");

  /* Ensure that the dialog box is destroyed when the user clicks ok. */

  gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(dialog));
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);

  /* Add the label, and show everything we've added to the dialog. */

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), frame);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX(box), vbox1, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX(box), vbox2, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX(box), vbox3, FALSE, FALSE, 2);
  gtk_container_add (GTK_CONTAINER (vbox2), label);
  gtk_window_set_title( GTK_WINDOW(dialog),"gsat - ERROR");
  gtk_widget_show_all (dialog);
}

void show_status( const gchar * statusmsg )
{
  GtkWidget *widget;
  gint ctxid;

  widget = lookup_widget( mainwindow, "statusbar" );
  ctxid = gtk_statusbar_get_context_id(GTK_STATUSBAR(widget),"Status");
  gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
  gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,statusmsg);
}

/* Establish network conection */

int connectsock(char *host, char *service, char *protocol)
{
  /* This function is used to connect to the server.  "host" is the
     name host is the name of the computer the server program resides
     on.  "service" is the name of the socket port.  "protocol" is the
     socket protocol.  It should be set to UDP. */

  struct hostent *phe;
  struct servent *pse;
  struct protoent *ppe;
  struct sockaddr_in sin;

  int s, type;

  bzero((char *)&sin,sizeof(struct sockaddr_in));
  sin.sin_family=AF_INET;

  if ((pse=getservbyname(service,protocol)))
    sin.sin_port=pse->s_port;
  else if ((sin.sin_port=htons((unsigned short)atoi(service)))==0) {
    fprintf(stderr,"WARNING: Can't get predict services. Check /etc/services. Trying with default port.\n");
    sin.sin_port=htons(1210);
  }

  if ((phe=gethostbyname(host)))
    bcopy(phe->h_addr,(char *)&sin.sin_addr,phe->h_length);

  else if ((sin.sin_addr.s_addr=inet_addr(host))==INADDR_NONE) {
    fprintf(stderr,"ERROR: Can't get host address.\n");
    error_dialog("ERROR: Can't get host address.\n");
    return -1;
  }

  if ((ppe=getprotobyname(protocol))==0)
    return -1;

  if (strcmp(protocol,"udp")==0)
    type=SOCK_DGRAM;
  else
    type=SOCK_STREAM;

  s=socket(PF_INET,type,ppe->p_proto);

  if (s<0) {
    fprintf(stderr,"ERROR: Can't get socket.\n");
    return -1;
  }

  if (connect(s,(struct sockaddr *)&sin,sizeof(sin))<0) {
    fprintf(stderr,"ERROR: Can't connect to socket.\n");
    return -1;
  }

  return s;
}

/* Send the command to the server */

int send_command(int sock, char *command)
{
  int len;

  len=write(sock,command,strlen(command));

  return len;
}

/* Read from network socket */

int read_socket(int sock, char *buf)
{
  int n;
  struct timeval tv;
  fd_set rfds;
  time_t now;

  FD_ZERO( &rfds );
  FD_SET( sock, &rfds );
  tv.tv_sec=NETTIMEOUT;
  tv.tv_usec=0;

  if( select(sock+1,&rfds,NULL,NULL,&tv))
    n=read(sock,buf,NETBUFSIZE);
/*      n=recvfrom(sock,buf,strlen(buf),0,NULL,NULL); */
  else {
    now=time(NULL);
    fprintf(stderr,"Timeout reading from socket.\n");
    return -1;
  }

  buf[n]='\0';

  return n;
}

/* Get response to command from server */

int get_response(int sock, char *buf)
{
  int len;
  int timeouts;

  len=0;
  timeouts=0;
  while(timeouts<MAXTIMEOUTS) {
    if((len=read_socket(sock,buf))<0) {
      timeouts++;
      if(timeouts==MAXTIMEOUTS) {
	fprintf(stderr,"ERROR: predict server %s is not responding to commands.\n", predicthost);
	return -1;
      }
    }
    else
      break;
  }

  return len;
}


/* Get prediction data */

#define BUFSIZE 60*MAXDOTS

void get_orbitdata(char *satname)
{
  int i,cnt,sx,sy;
  long int begin,now;
  float slat,slong,saz,sel,phase;
  char buf[BUFSIZE],weekday[4],adate[8],atime[9];

  /* Get satellite data */
  /* Build a command buffer */
  now=time(NULL);
  begin=now-MAXDOTS*60;
  cnt=0;
  
  fprintf(stderr,"Now: %ld From: %ld\n",now,now-MAXDOTS);

  sprintf(buf,"GET_SAT_POS %s %ld +90m\n",satname,begin);
  fprintf(stderr,"Command: %s",buf);

  /* Send the command to the server */
  send_command(netsocket,buf);

  /* Calculate sub-satellite points */

  for( i=0; i<MAXDOTS+1; i++ ) {
    /* Get the response */
    if(get_response(netsocket,buf)==-1) {
      error_dialog("Too many errors receiving response from server, disconnecting");
      on_cb_disconnect_clicked();
    }

    if(buf[0]==26)
      break;
    fprintf(stderr,"Index: %d Data: %s",i,buf);

    sscanf(buf,"%ld %s %s %s %f %f %f %f %f",&now,weekday,adate,atime,
	   &slat,&slong,&phase,&saz,&sel);

    if(slong>180.0) {
      slong = slong - 180.0;
      sx = (int)(MAPSIZEX - (slong * MAPSIZEX / 360.0));
    }
    else {
      sx = (int)((MAPSIZEX/2.0) - (slong * MAPSIZEX / 360.0));
    }
    sy = (int)((MAPSIZEY/2.0) - (slat * MAPSIZEY / 180.0));

    /* do not draw repeated dots in the orbit */
    if( dots[cnt-1].x!=sx || dots[cnt-1].y!=sy ) {
      dots[cnt].x=sx;
      dots[cnt].y=sy;
      cnt++;
    }
  }

  fprintf(stderr,"Count: %d\n",cnt);
  ndots=cnt;
}


/* Main processing loop */

gint
timeout_callback( gpointer data )
{
  int i, j, azi, ctxid;
  char buf[NETBUFSIZE], buf2[NETBUFSIZE], buf3[NETBUFSIZE],
    satname[26], *sat, callsign[16], param[30], *dopplerptr;
  long aostime,orbitnumber;
  float az, el, slong, slat, footprint, range, doppler, altitude,
    velocity, qthlat, qthlong, qthalt, phase, eclipse, squint;
  time_t t, now;
  GtkWidget *widget;
  double 	uplink, downlink, beacon, updoppler, downdoppler, beacondoppler,
    dopplershiftup, dopplershiftdown, dopplershiftbeacon,
    rangelat, rangelong, azimuth, ssplat,
    ssplong, TWOPI, HALFPI, deg2rad=1.74532925199e-02,
    R0=6378.16, beta, num, dem;
  GdkRectangle updatewin;
  gint sx, sy;
  GdkPoint footprintdots[360], qthfootprintdots[360];
  char visible;
  static int flash, azflash;

  TWOPI=2.0*PI;
  HALFPI=PI/2.0;

  /* Show date and time */
  now = time(NULL);
  if( timeUTC )
    sprintf(param,"%s",asctime(gmtime(&now)));
  else
    sprintf(param,"%s",asctime(localtime(&now)));
  param[strlen(param)-1]='\0';
  widget = lookup_widget( mainwindow, "tx_date" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  /* if we are not connected, don't do anything more */
  if( connected == FALSE )
    return TRUE; /* allow more timeouts to ocur */

  /* Get Satellite name from combo entry */
  widget = lookup_widget( mainwindow, "combo" );
  sat = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));
  strncpy( satname, sat, 25 );
  satname[ 26 ] = '\0';
  if( azelgraph==TRUE) {
    widget = lookup_widget( dialog_azel_graph, "tx_azel_sat" );
    gtk_entry_set_text(GTK_ENTRY(widget),satname);
  }

/*    if(doprediction==TRUE) { */
/*      get_orbitdata(satname); */
/*      doprediction=FALSE; */
/*    } */

  /* Get satellite data */
  /* Build a command buffer */
  sprintf(buf,"GET_SAT %s\n",satname);

  /* Send the command to the server */
  send_command(netsocket,buf);

  /* Get the response */
  if(get_response(netsocket,buf)==-1) {
    error_dialog("Too many errors receiving response from server, disconnecting");
    on_cb_disconnect_clicked();
    return TRUE;
  }

  /* Get doppler data */
  /* Build a command buffer */
  sprintf(buf2,"GET_DOPPLER %s\n",satname);

  /* Send the command to the server */
  send_command(netsocket,buf2);

  /* Get the response */
  if(get_response(netsocket,buf2)==-1) {
    error_dialog("Too many errors receiving response from server, disconnecting");
    on_cb_disconnect_clicked();
    return TRUE;
  }

  /* Parse the satellite data */

  /* The first element of the response is the satellite name.
     It is ended with a '\n' character and may contain spaces. */

  for (i=0; buf[i]!='\n'; i++)
    satname[i]=buf[i];

  satname[i]=0;
  i++;

  /* The rest of the data can be parsed using the sscanf()
     function.  First, the satellite name is removed from
     "buf", and then "buf" is parsed for numerical data
     using an sscanf(). */

  for (j=0; buf[i+j]!=0; j++)
    buf[j]=buf[i+j];

  buf[j]=0;
 
  sscanf(buf,"%f %f %f %f %ld %f %f %f %f %ld %c %f %f %f",
	 &slong, &slat, &az, &el, &aostime, &footprint, &range,
	 &altitude, &velocity, &orbitnumber, &visible, &phase, &eclipse, &squint);

  /* Get doppler shift */
  sscanf(buf2,"%f",&doppler );

  /* Get qth data */
  /* Build a command buffer */
  sprintf(buf3,"GET_QTH\n");

  /* Send the command to the server */
  send_command(netsocket,buf3);

  /* Get the response */
  if(get_response(netsocket,buf3)==-1) {
    error_dialog("Too many errors receiving response from server, disconnecting");
    on_cb_disconnect_clicked();
    return TRUE;
  }

  /* Parse the satellite data */

  /* The first element of the response is the callsign.
     It is ended with a '\n' character and may contain spaces. */

  for (i=0; buf3[i]!='\n'; i++)
    callsign[i]=buf3[i];

  callsign[i]=0;
  i++;

  /* The rest of the data can be parsed using the sscanf()
     function.  First, the callsign is removed from
     "buf3", and then "buf3" is parsed for numerical data
     using an sscanf(). */

  for (j=0; buf3[i+j]!=0; j++)
    buf3[j]=buf3[i+j];

  buf3[j]=0;
 
  sscanf(buf3,"%f %f %f",&qthlat,&qthlong,&qthalt);

  /* Check if satellite has decayed */
  if(aostime==0 && altitude==0) {
    clear_widgets();
    clear_map();
    plot_QTH(qthlat,qthlong,callsign);
    widget = lookup_widget( data, "tx_aos" );
    gtk_entry_set_text(GTK_ENTRY(widget),"Never");
    widget = lookup_widget( data, "satbar" );
    ctxid = gtk_statusbar_get_context_id(GTK_STATUSBAR(widget),"Status");
    gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
    gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
		       "Satellite has decayed");
    return TRUE;
  }

  /* Now display all the satellite data on the widgets */
  sprintf(param,"%7.2f %c",(slong>180?360.0-slong:slong),(slong>180?'E':'W'));
  widget = lookup_widget( data, "tx_longitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%7.2f %c",fabs((double)slat),(slat<0?'S':'N'));
  widget = lookup_widget( data, "tx_latitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%7.2f deg",az);
  widget = lookup_widget( data, "tx_azimuth" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);
  if( azelgraph==TRUE) {
    sprintf(param,"%7.2f",az);
    widget = lookup_widget( dialog_azel_graph, "tx_azel_azimuth" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);
  }

  sprintf(param,"%+-6.2f deg",el);
  widget = lookup_widget( data, "tx_elevation" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);
  if( azelgraph==TRUE) {
    sprintf(param,"%+-6.2f",el);
    widget = lookup_widget( dialog_azel_graph, "tx_azel_elevation" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);
  }

  if(enablerotor==TRUE)
    (*plugin_set_rotor)(az, el);

  sprintf(param,"%7.2f km",footprint);
  widget = lookup_widget( data, "tx_footprint" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%7.2f km",range);
  widget = lookup_widget( data, "tx_range" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%7.2f km",altitude);
  widget = lookup_widget( data, "tx_altitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  if(strncmp(predictversion,"2.1",3)==0)
    sprintf(param,"%7.2f km/h",velocity);
  else
    sprintf(param,"%7.2f km/s",velocity);
  
  widget = lookup_widget( data, "tx_velocity" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%ld",orbitnumber);
  widget = lookup_widget( data, "tx_orbit" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  sprintf(param,"%3.1f deg",phase);
  widget = lookup_widget( data, "tx_ma" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  /* Check if we have a valid squint angle (360=no squint) */
  if(squint==360.0) {
    sprintf(param,"N/A");
  } else {
    sprintf(param,"%3.1f deg",squint);
  }
  widget = lookup_widget( data, "tx_squint" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  /* change label of AOS widget to AOS/LOS */
  widget = lookup_widget( data, "lb_aos" );
  if( el>=0 )
    gtk_label_set_text(GTK_LABEL(widget),"    LOS at");
  else
    gtk_label_set_text(GTK_LABEL(widget),"Next AOS");

  t=(time_t)aostime;
  if( timeUTC )
    sprintf(param,"%s",asctime(gmtime(&t)));
  else
    sprintf(param,"%s",asctime(localtime(&t)));
  param[strlen(param)-1]='\0';
  widget = lookup_widget( data, "tx_aos" );
  gtk_entry_set_text(GTK_ENTRY(widget),param);

  /* execute AOS and LOS commands */
  if( newsat==0 ) {
    if( (lastel < 0 && el >= 0)||(lastel >=0 && el < 0) ) {
      if( el >= 0 ) {
	if(strlen(prefs_aos_command)>0)
	  system(prefs_aos_command);
      }
      else {
	if(strlen(prefs_los_command)>0)
	  system(prefs_los_command);
      }
    }
  }
  newsat=0;

  /* Show doppler correction if elevation > 0 */
  widget = lookup_widget( data, "tx_uplink" );
  dopplerptr=(gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  uplink=atof(dopplerptr);
  if(el >= 0 && uplink > 0.0) {
    dopplershiftup=(-uplink)/100000.0*doppler;
    sprintf(param,"%7.2f Hz",dopplershiftup);
    widget = lookup_widget( data, "tx_upshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);
    updoppler=uplink+(dopplershiftup/1000.0);
    sprintf(param,"%7.2f kHz",updoppler);
    widget = lookup_widget( data, "tx_updoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);

    /* Set Uplink frequency on the radio */
    if( enableupdoppler==TRUE )
      (*plugin_set_uplink_frequency)(updoppler);
  }
  else {
    widget = lookup_widget( data, "tx_upshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
    widget = lookup_widget( data, "tx_updoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
  }

  widget = lookup_widget( data, "tx_downlink" );
  dopplerptr=(gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  downlink=atof(dopplerptr);
  if(el >= 0 && downlink > 0.0) {
    dopplershiftdown=downlink/100000.0*doppler;
    sprintf(param,"%7.2f Hz",dopplershiftdown);
    widget = lookup_widget( data, "tx_downshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);
    downdoppler=downlink+(dopplershiftdown/1000.0);
    sprintf(param,"%7.2f kHz",downdoppler);
    widget = lookup_widget( data, "tx_dwdoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);

    /* Set Downlink frequency on the radio */
    if( enabledowndoppler==TRUE )
      (*plugin_set_downlink_frequency)(downdoppler);
  }
  else {
    widget = lookup_widget( data, "tx_downshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
    widget = lookup_widget( data, "tx_dwdoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
  }

  widget = lookup_widget( data, "tx_beacon" );
  dopplerptr=(gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  beacon=atof(dopplerptr);
  if(el >= 0 && beacon > 0.0) {
    dopplershiftbeacon=beacon/100000.0*doppler;
    sprintf(param,"%7.2f Hz",dopplershiftbeacon);
    widget = lookup_widget( data, "tx_beaconshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);
    beacondoppler=beacon+(dopplershiftbeacon/1000.0);
    sprintf(param,"%7.2f kHz",beacondoppler);
    widget = lookup_widget( data, "tx_beacondoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),param);

    /* Set Beacon frequency on the radio */
    if( enablebeacondoppler==TRUE )
      (*plugin_set_beacon_frequency)(beacondoppler);
  }
  else {
    widget = lookup_widget( data, "tx_beaconshift" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
    widget = lookup_widget( data, "tx_beacondoppler" );
    gtk_entry_set_text(GTK_ENTRY(widget),"--------");
  }

  /* Show satellite visibility in 2nd status bar */
  widget = lookup_widget( data, "satbar" );
  ctxid = gtk_statusbar_get_context_id(GTK_STATUSBAR(widget),"Status");
  switch( visible ) {
  case 'D':
    gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
    gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
		       "Satellite is in daylight");
    break;
  case 'N':
    gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
    gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
		       "Satellite is in darkness");
    break;
  case 'V':
    gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
    gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
		       "Satellite is visible");
    break;
  default:
    gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
    gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
		       "No visibility information available");
  }

  /* Graphical representation */
  
  /* Range Circle Calculations by KD2BD */

  if( satfootprint ) {
    ssplat=slat*deg2rad;
    ssplong=slong*deg2rad;
    beta=(0.5*footprint)/R0;

    for (azi=0; azi<360; azi++)
      {
	azimuth=deg2rad*(double)azi;
	rangelat=asin(sin(ssplat)*cos(beta)+cos(azimuth)*sin(beta)*cos(ssplat));
	num=cos(beta)-(sin(ssplat)*sin(rangelat));
	dem=cos(ssplat)*cos(rangelat);

	if (azi==0 && (beta > HALFPI-ssplat))
	  rangelong=ssplong+PI;

	else if (azi==180 && (beta > HALFPI+ssplat))
	  rangelong=ssplong+PI;

	else if (fabs(num/dem)>1.0)
	  rangelong=ssplong;

	else
	  {
	    if ((180-azi)>=0)
	      rangelong=ssplong-arccos(num,dem);
	    else
	      rangelong=ssplong+arccos(num,dem);
	  }

	while (rangelong<0.0)
	  rangelong+=TWOPI;

	while (rangelong>(2.0*PI))
	  rangelong-=TWOPI;
 
	rangelat=rangelat/deg2rad;
	rangelong=rangelong/deg2rad;

	/* Convert range circle data to map-based
	   coordinates and draw on map */

	if (rangelong>180.0)
	  {
	    rangelong=rangelong-180.0;
	    sx=(int)(MAPSIZEX-(rangelong*MAPSIZEX/360.0));
	  }

	else
	  sx=(int)((MAPSIZEX/2.0)-(rangelong*MAPSIZEX/360.0));

	sy=(int)((MAPSIZEY/2.0)-(rangelat*MAPSIZEY/180.0));

	/* store position for Plot Range Circle */
	footprintdots[azi].x=sx;
	footprintdots[azi].y=sy;
      }
  }

  if( qthfootprint ) {
    ssplat=qthlat*deg2rad;
    ssplong=qthlong*deg2rad;
    beta=(0.5*footprint)/R0;

    for (azi=0; azi<360; azi++)
      {
	azimuth=deg2rad*(double)azi;
	rangelat=asin(sin(ssplat)*cos(beta)+cos(azimuth)*sin(beta)*cos(ssplat));
	num=cos(beta)-(sin(ssplat)*sin(rangelat));
	dem=cos(ssplat)*cos(rangelat);

	if (azi==0 && (beta > HALFPI-ssplat))
	  rangelong=ssplong+PI;

	else if (azi==180 && (beta > HALFPI+ssplat))
	  rangelong=ssplong+PI;

	else if (fabs(num/dem)>1.0)
	  rangelong=ssplong;

	else
	  {
	    if ((180-azi)>=0)
	      rangelong=ssplong-arccos(num,dem);
	    else
	      rangelong=ssplong+arccos(num,dem);
	  }

	while (rangelong<0.0)
	  rangelong+=TWOPI;

	while (rangelong>(2.0*PI))
	  rangelong-=TWOPI;

	rangelat=rangelat/deg2rad;
	rangelong=rangelong/deg2rad;

	/* Convert range circle data to map-based
	   coordinates and draw on map */

	if (rangelong>180.0)
	  {
	    rangelong=rangelong-180.0;
	    sx=(int)(MAPSIZEX-(rangelong*MAPSIZEX/360.0));
	  }

	else
	  sx=(int)((MAPSIZEX/2.0)-(rangelong*MAPSIZEX/360.0));

	sy=(int)((MAPSIZEY/2.0)-(rangelat*MAPSIZEY/180.0));

	/* store position for Plot Range Circle */
	qthfootprintdots[azi].x=sx;
	qthfootprintdots[azi].y=sy;
      }
  }

  /* AZ/EL dots */
  if( lastel < 0 && el >= 0 )    /* if sat is beginning a new pass ... */
    nazeldots=0;                 /* reset az/el dots counter */

  /* save last elevation value */
  lastel=el;

  if(el>=0) {
    azimuth=(double)(az-90.0)*deg2rad;
    sx=(AZELSIZEX/2)+cos(azimuth)*((double)(AZELSIZEX/2)-el*(double)(AZELSIZEX/2)/90.0);
    sy=(AZELSIZEY/2)+sin(azimuth)*((double)(AZELSIZEY/2)-el*(double)(AZELSIZEY/2)/90.0);

    if( nazeldots > 0 ) {
      if( azeldots[nazeldots-1].x!=sx || azeldots[nazeldots-1].y!=sy) {
	azeldots[nazeldots].x=sx;
	azeldots[nazeldots].y=sy;
	nazeldots++;
      }
    }
    else {
      azeldots[0].x=sx;
      azeldots[0].y=sy;
      nazeldots=1;
    }
  }
    
  /* First draw the empty map */
  gdk_draw_pixmap( drawmap, yellow_gc, sourcemap, 0, 0, 0, 0, MAPSIZEX, MAPSIZEY );

  /* Draw grid if needed */
  if(drawgrid==TRUE) {
    draw_grid();
  }

  /* Draw orbit */
  if( drawtrack==TRUE )
    if( ndots>0 )
      gdk_draw_points(drawmap,red_gc,dots,ndots);

  /* Draw footprints */
  if( satfootprint==TRUE) {
    for(i=0; i<359; i++) {
      if( abs( footprintdots[i].x - footprintdots[i+1].x ) < MAPSIZEX/2 )
	gdk_draw_line(drawmap, yellow_gc, footprintdots[i].x,footprintdots[i].y,
		      footprintdots[i+1].x,footprintdots[i+1].y);
    }
    if( abs( footprintdots[0].x - footprintdots[i].x ) < MAPSIZEX/2 )
      gdk_draw_line(drawmap, yellow_gc, footprintdots[0].x,footprintdots[0].y,
		    footprintdots[i].x,footprintdots[i].y);
  }

  if( qthfootprint==TRUE) {
    for(i=0; i<359; i++) {
      if( abs( qthfootprintdots[i].x - qthfootprintdots[i+1].x ) < MAPSIZEX/2 )
	gdk_draw_line(drawmap, cyan_gc, qthfootprintdots[i].x,qthfootprintdots[i].y,
		      qthfootprintdots[i+1].x,qthfootprintdots[i+1].y);
    }
    if( abs( qthfootprintdots[0].x - qthfootprintdots[i].x ) < MAPSIZEX/2 )
      gdk_draw_line(drawmap, cyan_gc, qthfootprintdots[0].x,qthfootprintdots[0].y,
		    qthfootprintdots[i].x,qthfootprintdots[i].y);
  }

  /* Plot sub-satellite point */

  if(slong>180.0) {
    slong = slong - 180.0;
    sx = (int)(MAPSIZEX - (slong * MAPSIZEX / 360.0));
  }
  else {
    sx = (int)((MAPSIZEX/2.0) - (slong * MAPSIZEX / 360.0));
  }
  sy = (int)((MAPSIZEY/2.0) - (slat * MAPSIZEY / 180.0));

  /* do not draw repeated dots in the orbit */
  if( dots[ndots-1].x!=sx || dots[ndots-1].y!=sy ) {
    if( ndots<MAXDOTS ) {
      dots[ndots].x=sx;
      dots[ndots].y=sy;
      ndots++;
    }
    else {
      for( i=0;i<MAXDOTS-1;i++ ) {
	dots[i].x=dots[i+1].x;
	dots[i].y=dots[i+1].y;
      }
      dots[MAXDOTS-1].x=sx;
      dots[MAXDOTS-1].y=sy;
    }
  }

  /* Plot qth point */
  plot_QTH(qthlat,qthlong,callsign);

  /* the flashing square */
  if( flash==0 ) {
    gdk_draw_rectangle( drawmap, purple_gc, TRUE, sx-1, sy-1, 3, 3 );
    flash=1;
  }
  else {
    gdk_draw_rectangle( drawmap, yellow_gc, TRUE, sx-1, sy-1, 3, 3 );
    flash=0;
  }

  /* Force map redraw */
  widget = lookup_widget( data, "maparea" );
  updatewin.x = 0;
  updatewin.y = 0;
  updatewin.width = widget->allocation.width;
  updatewin.height = widget->allocation.height;
  gtk_widget_draw ( widget, &updatewin);

  /* draw AZ/EL graphic */
  if(azelgraph==TRUE) {
    /* First draw the empty az/el graphic */
    gdk_draw_pixmap( drawazel, blue_gc, sourceazel,
		       0, 0, 0, 0, AZELSIZEX, AZELSIZEY );
    /* Then draw the track */
    if( nazeldots>0 )
      gdk_draw_lines(drawazel,blue_gc,azeldots,nazeldots);

    /* and a flashing square */
    if(el>=0) {
      sx=azeldots[nazeldots-1].x;
      sy=azeldots[nazeldots-1].y;
      if( azflash==0 ) {
	gdk_draw_rectangle( drawazel, cyan_gc, TRUE, sx-1, sy-1, 3, 3 );
	azflash=1;
      }
      else {
	gdk_draw_rectangle( drawazel, blue_gc, TRUE, sx-1, sy-1, 3, 3 );
	azflash=0;
      }
    }

    /* Force AZ/EL redraw */
    widget = lookup_widget( dialog_azel_graph, "azelgraph" );
    updatewin.x = 0;
    updatewin.y = 0;
    updatewin.width = widget->allocation.width;
    updatewin.height = widget->allocation.height;
    gtk_widget_draw ( widget, &updatewin);
  }

  return TRUE;
}

/* Connect to predict server */

#define LISTBUFSIZE 2048

int connect_server( void )
{
  GtkWidget * widget;
  int i;
  char bufr[LISTBUFSIZE];
  char *sname;
  char *bufp;
  gpointer *satname;

  /* Get satellite list from server */
  netsocket=connectsock(predicthost,predictport,"udp");

  if (netsocket<0) {
    fprintf(stderr, "ERROR: Can't connect to the predict server on %s.\n", predicthost);
    error_dialog("Can't connect to the predict server.");
    return 1;
  }

  send_command(netsocket,"GET_LIST");

  if(get_response(netsocket,bufr)==-1) {
    error_dialog("Too many errors receiving response from server, disconnecting");
    on_cb_disconnect_clicked();
    return 1;
  }

  /* Parse the response and place each name
     in the GList satlist. */

  bufp=bufr;
  while( (sname=strsep(&bufp,"\n")) != NULL) {
    if(strlen(sname)>0) {
      satname=g_malloc0(strlen(sname)+1);
      strncpy((char *)satname,(const char*)sname,strlen(sname));
      satlist=g_list_append(satlist,satname);
    }
  }

  /* Attach satellite list */
  widget=lookup_widget( mainwindow, "combo" );
  gtk_combo_set_popdown_strings( GTK_COMBO(widget), satlist);

  /* Get predict version from server */
  send_command(netsocket,"GET_VERSION");

  if(get_response(netsocket,predictversion)==-1) {
    error_dialog("Too many errors receiving response from server, disconnecting");
    on_cb_disconnect_clicked();
    return TRUE;
  }

  for(i=0;i<strlen(predictversion);i++)
    if(predictversion[i]=='\n') {
      predictversion[i]='\0';
      break;
    }

  /* Setup status bar */
  sprintf(statusmsg,"Connected to predict server version %s on %s", \
	  predictversion, predicthost);
  show_status( statusmsg );

  /* Enable main loop processing */
  connected=TRUE;
  lastel=0;

  return 0;
}

void clear_map( void )
{
  GtkWidget *widget;
  GdkRectangle updatewin;

  /* copy original map to draw map */
  gdk_draw_pixmap( drawmap, yellow_gc, sourcemap, 0, 0, 0, 0, MAPSIZEX, MAPSIZEY );

  /* draw grid if needed */
  if(drawgrid==TRUE) {
    draw_grid();
  }

  /* Force map redraw */
  widget = lookup_widget( mainwindow, "maparea" );
  updatewin.x = 0;
  updatewin.y = 0;
  updatewin.width = widget->allocation.width;
  updatewin.height = widget->allocation.height;
  gtk_widget_draw ( widget, &updatewin);
}

void draw_grid( void )
{
  GtkWidget *widget;
  gint sx, sy, i;

  widget=lookup_widget(mainwindow, "maparea");
  for(i=-90; i<90; i=i+30) {
    sy = (int)((MAPSIZEY/2.0) - (i * MAPSIZEY / 180.0));
    gdk_draw_line(drawmap,widget->style->black_gc,0,sy,MAPSIZEX,sy);
  }
  for(i=-180; i<180; i=i+30) {
    sx = (int)((MAPSIZEX/2.0) - (i * MAPSIZEX / 360.0));
    gdk_draw_line(drawmap,widget->style->black_gc,sx,0,sx,MAPSIZEY);
  }
}

void plot_QTH( float qthlat, float qthlong, char *callsign )
{
  gint qthx, qthy;

  if(qthlong>180.0) {
    qthlong = qthlong - 180.0;
    qthx = (int)(MAPSIZEX - (qthlong * MAPSIZEX / 360.0));
  }
  else {
    qthx = (int)((MAPSIZEX/2.0) - (qthlong * MAPSIZEX / 360.0));
  }
  qthy = (int)((MAPSIZEY/2.0) - (qthlat * MAPSIZEY / 180.0));

  gdk_draw_rectangle( drawmap, cyan_gc, TRUE, qthx-1, qthy-1, 3, 3 );
  gdk_draw_string( drawmap, drawfont, cyan_gc, qthx+4, qthy+4, callsign );
}
 
void clear_widgets( void )
{
  GtkWidget *widget;

  widget = lookup_widget( mainwindow, "tx_longitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_latitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_azimuth" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_elevation" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_footprint" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_range" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_altitude" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_velocity" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_ma" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_squint" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_orbit" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "lb_aos" );
  gtk_label_set_text(GTK_LABEL(widget),"Next AOS");

  widget = lookup_widget( mainwindow, "tx_aos" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_upshift" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_updoppler" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_downshift" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");
  widget = lookup_widget( mainwindow, "tx_dwdoppler" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( mainwindow, "tx_beaconshift" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");
  widget = lookup_widget( mainwindow, "tx_beacondoppler" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( dialog_azel_graph, "tx_azel_sat" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( dialog_azel_graph, "tx_azel_azimuth" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");

  widget = lookup_widget( dialog_azel_graph, "tx_azel_elevation" );
  gtk_entry_set_text(GTK_ENTRY(widget),"");
}

void disconnect_server( void )
{
  GtkWidget *widget;
  gint ctxid;
  gpointer *satname;

  /* if we are not connected, don't do anything more */
  if( connected == FALSE )
    return;

  /* Disable main loop processing */
  connected=FALSE;

  /* Close network socket */
  close(netsocket);

  /* Setup status bar */
  show_status("Not connected");

  /* Clear all data from widgets */
  widget=lookup_widget( mainwindow, "combo" );
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(widget)->entry), "");
  gtk_list_clear_items(GTK_LIST(GTK_COMBO(widget)->list), 0, -1);

  clear_map();
  clear_widgets();

  /* Empty the satellite list and free memory */
  satlist=g_list_first( satlist );
  while( g_list_length( satlist ) > 0 ) {
    satname=satlist->data;
    satlist=g_list_remove( satlist, satname );
    g_free( satname );
  }

  widget = lookup_widget( mainwindow, "satbar" );
  ctxid = gtk_statusbar_get_context_id(GTK_STATUSBAR(widget),"Status");
  gtk_statusbar_pop(GTK_STATUSBAR(widget),ctxid);
  gtk_statusbar_push(GTK_STATUSBAR(widget),ctxid,
			     "No visibility information available");
}

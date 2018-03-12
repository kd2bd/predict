/*
 * YAESU FT-736 plugin for gsat
 *
 * Copyright (C) 2003 by Hiroshi Iwamoto, JH4XSY
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

FILE *radioFileA;

static void send_freq(char freq[])
{
    int i, j, frbuf;

    i = 0; /* counter for CAT */
    j = 0; /* string counter */

    if ( strncmp(freq, "12", 2) == 0 ) {  /* 1200MHz */
	frbuf  = 0xc0;                
	j = 2;
	frbuf += freq[j]-'0';
    } else {                              /* 430/144MHz */
	frbuf  = ( freq[j]-'0' ) << 4;
	++j;
	frbuf +=   freq[j]-'0';
    }
    putc(frbuf, radioFileA);		  /* Send 1Byte */

    ++j;
    for( i=1; i<4; ++i) {                 /* Send +3Bytes */
	frbuf = (freq[j]-'0') << 4; 
	++j;
	if ( freq[j] == '.' ) {
	    ++j;                          /* skip */
	}
	frbuf += freq[j]-'0'; 
	putc(frbuf, radioFileA);
	++j;
    }

}
    
char * plugin_info( void )
{
  return "YAESU FT736 V0.1";
}

int plugin_open_rig( char * config )
{
    char dumm[64];
    char tty[12];
    char Smod[4];
    char Qmod[4];
    char *ptr, *parm;
    char cmd[64];
 
    int dummy = 0;
    int saton = 0x0E;
    int modLSB = 0x00;
    int modUSB = 0x01;
    int modCW = 0x02;
    int modFM = 0x08;
    int modRX = 0x17;
    int modTX = 0x27;
    int freqRX = 0x1e;
    int freqTX = 0x2e;
    int term;

    int upMode, downMode;

    char freq[12];
    char upFreq[12], downFreq[12], tmp1Freq[12], tmp2Freq[12];
    double V = 145.950;
    double U = 435.850;
    double T1 = 52.000;
    double T2 = 1260.000; 

    tty[0]='\0';
    Smod[0]='\0';
    Qmod[0]='\0';

    printf("FT736 plugin opened.\n");

    if(config) {
      strncpy(dumm,config,64);

      ptr=dumm;
      parm=ptr;
      while( parm != NULL ) {
        parm=strsep(&ptr,":");
        if(parm==NULL)
          break;
        if(strlen(parm)!=0) {
          switch( *parm ) {
          case 'D':			/* tty port */
            strcpy(tty,parm+1);
            break;
          case 'S':			/* Up/Down Freq. */
            strcpy(Smod,parm+1);
            break;
          case 'Q':			/* QSO Mode */
            strcpy(Qmod,parm+1);
            break;
          }
        }
      }
    }

    if(strlen(tty)==0)
        strcpy(tty,"/dev/ttyS0");
    if(strlen(Smod)==0)
        strcpy(Smod,"UV");
    if(strlen(Qmod)==0)
        strcpy(Qmod,"CW");

/* Open CAT port */
    radioFileA = fopen(tty, "a");
    if ( radioFileA == NULL ) {
	fprintf(stderr, "can't open %s\n", tty);
        return 0;
    }

/* CAT port initialize */
   sprintf(cmd, "/bin/stty speed 4800 < %s", tty);
   system(cmd);
   sprintf(cmd, "/bin/stty cstopb < %s", tty);
   system(cmd);

/* Send CMD: CAT ON */
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    fflush(radioFileA);

/* Send CMD: SAT ON */
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(saton, radioFileA);
    fflush(radioFileA);
    sleep(1);

/* set QSO MODE  */
    upMode = downMode = modCW;

    if ( strcmp(Qmod, "CW") == 0 ) {
      upMode = downMode = modCW;  
    }
    if ( strcmp(Qmod, "SSB") == 0 ) {
      upMode = modLSB;
      downMode = modUSB;
    }
    if ( strcmp(Qmod, "FM") == 0 ) {
      upMode = downMode = modFM;
    }

/* Send CMD: SAT RX MODE */
    putc(downMode, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(modRX, radioFileA);
    fflush(radioFileA);

/* Send CMD: SAT TX MODE */
    putc(upMode, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(dummy, radioFileA);
    putc(modTX, radioFileA);
    fflush(radioFileA);
    sleep(1);

/* set SAT MODE */
    sprintf(upFreq, "%.6f", U);
    sprintf(downFreq, "%.6f", V);
    sprintf(tmp1Freq, "%.6f", T1);
    sprintf(tmp2Freq, "%.6f", T2);

    if ( strcmp(Smod, "UV") == 0 ) {	/* ^ 430MHz v 144MHz */
      sprintf(upFreq, "%.6f", U);
      sprintf(downFreq, "%.6f", V);
    }
    if ( strcmp(Smod, "VU") == 0 ) {	/* ^ 144MHz v 430MHz */
      sprintf(upFreq, "%.6f", V);
      sprintf(downFreq, "%.6f", U);
    }
    if ( strcmp(Smod, "US") == 0 ) {	/* ^ 430MHz v 2.4GHz */
      sprintf(upFreq, "%.6f", U);
      sprintf(downFreq, "%.6f", V);
    }
    term = strlen(upFreq);
    upFreq[term] = '\0';
    term = strlen(downFreq);
    downFreq[term] = '\0';
    term = strlen(tmp1Freq);
    tmp1Freq[term] = '\0';
    term = strlen(tmp2Freq);
    tmp2Freq[term] = '\0';

/* Send CMD: dummy downlink Freq */
    send_freq(tmp2Freq);
    putc(freqRX, radioFileA);
    fflush(radioFileA);
    sleep(1);
 
/* Send CMD: dummy uplink Freq */
    send_freq(tmp1Freq);
    putc(freqTX, radioFileA);
    fflush(radioFileA);

/* Send CMD: uplink Freq */
    send_freq(upFreq);
    putc(freqTX, radioFileA);
    fflush(radioFileA);
    sleep(1);

/* Send CMD: downlink Freq */
    send_freq(downFreq);
    putc(freqRX, radioFileA);
    fflush(radioFileA);

  return 1;
}

void plugin_close_rig( void )
{
  int dummy = 0;
  int catof = 0x80;

  printf("FT736 plugin closed.\n");

/* Send CMD: CAT Off */
  putc(dummy, radioFileA);
  putc(dummy, radioFileA);
  putc(dummy, radioFileA);
  putc(dummy, radioFileA);
  putc(catof, radioFileA);
  fflush(radioFileA);

  fclose(radioFileA);

}

void plugin_set_downlink_frequency( double frequency )
{
  double freq;
  char downFreq[12];
  int freqRX = 0x1e;
  int term;

  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

/* printf("Downlink Frequency: %f kHz -> %f kHz\n", frequency, freq); */

  frequency = frequency / 1000;			/* Convert: kHz -> MHz */
  sprintf(downFreq, "%.6f", frequency);
  term = strlen(downFreq);
  downFreq[term] = '\0';

  send_freq(downFreq);
  putc(freqRX, radioFileA); 
  fflush(radioFileA);

}

void plugin_set_uplink_frequency( double frequency )
{
  double freq;
  char upFreq[12];
  int freqTX = 0x2e;
  int term;

  freq=frequency-fmod(frequency,5);
  if(fmod(frequency,5)>2.5)
    freq+=5;

/* printf("Uplink Frequency: %f kHz -> %f kHz\n", frequency, freq); */

  frequency = frequency / 1000;		/* Convert: kHz -> MHz */
  sprintf(upFreq, "%.6f", frequency);
  term = strlen(upFreq);
  upFreq[term] = '\0';

  send_freq(upFreq);
  putc(freqTX, radioFileA);
  fflush(radioFileA);

}

/****************************************************************************
*                  VOCALIZER: A speech articulation program                 *
*		Copyright John A. Magliacane, KD2BD 1999-2003               *
*	   		  Last update: 21-Oct-2015                          *
*****************************************************************************
* This program is used by PREDICT to annunciate satellite coordinates       *
* through the system's soundcard (/dev/dsp) using .wav files containing     *
* uncompressed "raw" PCM data.  It is called as a background process.       *
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     *
* for more details.                                                         *
*                                                                           *
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "vocalizer.h"

int dsp;
char numstr[20][10];
unsigned char buffer[65536];

unsigned long buffer2long(indx)
int indx;
{
	unsigned long byte0, byte1, byte2, byte3;

	if ((indx+3)<65536)
	{
		byte0=(unsigned long)buffer[indx];
		byte1=(unsigned long)buffer[indx+1];
		byte2=(unsigned long)buffer[indx+2];
		byte3=(unsigned long)buffer[indx+3];

		return (byte0|(byte1<<8)|(byte2<<16)|(byte3<<24));
	}
	else
		return 0L;
}

unsigned int buffer2int(indx)
int indx;
{
	unsigned int byte0, byte1;

	if ((indx+1)<65536)
	{
		byte0=(unsigned int)buffer[indx];
		byte1=(unsigned int)buffer[indx+1];

		return (byte0|(byte1<<8));
	}
	else
		return 0;
}

int wavplay(filename)
char *filename;
{
	int x, y, format, bits, bytes, channels, dsp, fd;
	unsigned long total_samples=0L, rate=0L, running_total=0L;
	char filenpath[80];

	strncpy(filenpath,path,79);
	strncat(filenpath,filename,79);
	strncat(filenpath,".wav",79);

	fd=open(filenpath,O_RDONLY);

	if (fd==-1)
	{
		fprintf(stderr,"*** vocalizer: Cannot open \"%s\"\n",filenpath);
		return -1;
	}

	bytes=read(fd,&buffer,65536);

	y=((buffer[0]^'R')  | (buffer[1]^'I') |
  	   (buffer[2]^'F')  | (buffer[3]^'F') |
	   (buffer[8]^'W')  | (buffer[9]^'A') |
	   (buffer[10]^'V') | (buffer[11]^'E'));

	if (y)
	{
		fprintf(stderr,"*** vocalizer: No RIFF header found!\n");
		return -1;
	}

	for (x=12, y=1; x<65535 && y; x++)
		y=((buffer[x]^'f')  | (buffer[x+1]^'m') |
		  (buffer[x+2]^'t') | (buffer[x+3]^' '));

	if (y==0)
	{
		x+=3;
		x+=4;
		format=buffer2int(x);
		x+=2;
		channels=buffer2int(x);
		x+=2;
		rate=buffer2long(x);
		x+=4;
		x+=4;
		/* int block_size=buffer2int(x); */
		buffer2int(x);
		x+=2;
		bits=buffer2int(x);
		x+=2;

		if (format!=1)
		{
			fprintf(stderr,"*** vocalizer: Not PCM Wave Format.\n");
			return -1;
		}

		for (x=0, y=1; x<65535 && y; x++)
			y=((buffer[x]^'d') | (buffer[x+1]^'a') |
			 (buffer[x+2]^'t') | (buffer[x+3]^'a'));

		if (y==0)
		{
			x+=3;
			total_samples=buffer2long(x);

			if (total_samples!=0L)
			{
				dsp=open("/dev/dsp",O_RDWR);

				if (dsp==-1)
				{
					fprintf(stderr,"*** vocalizer: Cannot open /dev/dsp!\n");
					return -1;
				}

				ioctl(dsp,SOUND_PCM_WRITE_CHANNELS,&channels);
				ioctl(dsp,SOUND_PCM_WRITE_RATE,&rate);
				ioctl(dsp,SOUND_PCM_WRITE_BITS,&bits);

				lseek(fd,(long)(x+4),SEEK_SET);
				bytes=read(fd,&buffer,65536);
				running_total+=bytes;

				if (running_total>total_samples)
					bytes-=(running_total-total_samples);

				while (bytes>0)
				{
					write(dsp,&buffer,bytes);

					bytes=read(fd,&buffer,65536);
					running_total+=bytes;

					if (running_total>total_samples)
						bytes-=(running_total-total_samples);
				}

				close(dsp);
				close(fd);
			}
		}
		else
			fprintf(stderr,"*** vocalizer: Could not find 'data' chunk.\n");
	}
	else
		fprintf(stderr,"*** vocalizer: Could not extract format from header.\n");

	return 0;
}

void saynumber(num)
int num;
{
	char string[10];

	sprintf(string,"%4d",num);

	if (string[1]!=32)
	{
		wavplay(numstr[string[1]-48]);
		wavplay("hundred");
	}

	switch (string[2])
	{
		case '1':
			wavplay(numstr[(10+(string[3]-48))]);
			break;

		case '2':	
			wavplay("twenty");
			break;

		case '3':
			wavplay("thirty");
			break;

		case '4':
			wavplay("forty");
			break;

		case '5':
			wavplay("fifty");
			break;

		case '6':
			wavplay("sixty");
			break;

		case '7':
			wavplay("seventy");
			break;

		case '8':
			wavplay("eighty");
			break;

		case '9':
			wavplay("ninety");
			break;
	}

	if (string[3]!=32 && string[3]!=0 && string[2]!='1' && string[3]!='0')
		wavplay(numstr[string[3]-48]);

	if (string[3]=='0' && string[2]==32 && string[1]==32)
		wavplay("zero"); 
}

int main(argc,argv)
int argc;
char *argv[];
{
	int number;

	strcpy(numstr[1], "one\0");
	strcpy(numstr[2], "two\0");
	strcpy(numstr[3], "three\0");
     	strcpy(numstr[4], "four\0");
	strcpy(numstr[5], "five\0");
	strcpy(numstr[6], "six\0");
     	strcpy(numstr[7], "seven\0");
	strcpy(numstr[8], "eight\0");
	strcpy(numstr[9], "nine\0");
     	strcpy(numstr[0], "zero\0");
	strcpy(numstr[10],"ten\0");
	strcpy(numstr[11],"eleven\0");
	strcpy(numstr[12],"twelve\0");
	strcpy(numstr[13],"thirteen\0");
	strcpy(numstr[14],"fourteen\0");
	strcpy(numstr[15],"fifteen\0");
	strcpy(numstr[16],"sixteen\0");
	strcpy(numstr[17],"seventeen\0");
	strcpy(numstr[18],"eighteen\0");
	strcpy(numstr[19],"nineteen\0");

	/* Start talking! */

	if (argc==2)
	{
		if (strncmp(argv[1],"eclipse",7)==0)
		{
			wavplay("alarm");
			wavplay("eclipse");
		}

		if (strncmp(argv[1],"sunlight",8)==0)
		{
			wavplay("alarm");
			wavplay("sunlight");
		}

		if (strncmp(argv[1],"los",3)==0)
			wavplay("los");
	}

	if (argc>=3)
	{
		wavplay("intro");

		number=atoi(argv[1]);
		saynumber(number);
		wavplay("azimuth");

		number=atoi(argv[2]);
		saynumber(number);
		wavplay("elevation");

		if (argc>=4)
		{
			if (argv[3][0]=='+')
				wavplay("approaching");

			if (argv[3][0]=='-')
				wavplay("receding");
		}

		if (argc==5)
		{
			if (argv[4][0]=='V')
				wavplay("visible");
		}
	}

	return 0;
}

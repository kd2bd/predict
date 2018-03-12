/****************************************************************************
*          MAP: A satellite tracking/orbital display program                *
*               Copyright Ivan J. Galysh, KD4HBO 2000                       *
*                       Project started: 18-Dec-99                          *
*                         Last update: 02-Jan-01                            *
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
#include "forms.h"
#include "map.h"
#include <sys/timeb.h>

extern int connectsock();
extern void handler();
extern void get_response();

char pathname[128];

int main(int argc, char *argv[])
{
	int i, skt, x, y, z;
	char cmd[16], bufr[625], satnamelist[26][26];
	const char *ptrsat;
	FD_map *fd_map;
	struct timeb tptr;

	if (argc==1)
	{
		printf("\n%c*** ERROR!  System name missing! (map system_name)\n\n",7);
		exit(-1);
	}

	for (i=0; i<360; i++)
	{
		xa[i].x=0;
		xa[i].y=0;
	}

	x=ftime(&tptr);
	time_offset=tptr.timezone;

	fl_initialize(&argc, argv, 0, 0, 0);
	fd_map=create_form_map();
	strcpy(servername,argv[1]);
	skt=connectsock(servername,"predict","udp");

	if (skt<0)
		exit(-1);

	write(skt,"GET_LIST",9);
	get_response(skt,bufr);
	close(skt);

	/* Parse response */

	for (x=0, y=0, z=0; y<strlen(bufr); y++)
	{
		if (bufr[y]!='\n')
		{
			satnamelist[z][x]=bufr[y];
			x++;
		}

		else
		{
			satnamelist[z][x]=0;
			z++;
			x=0;
		}
	}

	for (i=1; i<=z; i++)
		fl_replace_choice(fd_map->k,i,satnamelist[i-1]);

	ptrsat=fl_get_choice_item_text(fd_map->k,1);
	strcpy(satnum,ptrsat); 

	/* fill-in form initialization code */
	fdmap=fd_map;

	/* show the first form */
	fl_show_form(fd_map->map,FL_PLACE_CENTERFREE,FL_FULLBORDER,"map: Real-Time Satellite Tracking Display");
	fl_do_forms();

	return 0;
}

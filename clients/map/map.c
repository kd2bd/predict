/****************************************************************************
*          MAP: A satellite tracking/orbital display program                *
*               Copyright Ivan J. Galysh, KD4HBO 2000                       *
*                       Project started: 18-Dec-99                          *
*                     Last update: 05-Jan-01 (KD2BD)                        *
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
/* Form definition file generated with fdesign. */

#include "forms.h"
#include <stdlib.h>
#include "map.h"
#include "world_fill.h"

static FL_PUP_ENTRY __[] =
{ 
	/*  itemtext   callback  shortcut   mode */
	{ "OSCAR-10",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-11",	0,	"",	 FL_PUP_NONE},
	{ "PACSAT",	0,	"",	 FL_PUP_NONE},
	{ "WEBERSAT",	0,	"",	 FL_PUP_NONE},
	{ "LUSAT",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-20",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-22",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-23",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-25",	0,	"",	 FL_PUP_NONE},
	{ "ITAMSAT",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-27",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-29",	0,	"",	 FL_PUP_NONE},
	{ "OSCAR-36",	0,	"",	 FL_PUP_NONE},
	{ "TECHSAT",	0,	"",	 FL_PUP_NONE},
	{ "TMSAT",	0,	"",	 FL_PUP_NONE},
	{ "SEDSAT-1",	0,	"",	 FL_PUP_NONE},
	{ "RS-12/13",	0,	"",	 FL_PUP_NONE},
	{ "ASUSAT-1",	0,	"",	 FL_PUP_NONE},
	{ "SUNSAT",	0,	"",	 FL_PUP_NONE},
	{ "MIR",	0,	"",	 FL_PUP_NONE},
	{ "OPAL",	0,	"",	 FL_PUP_NONE},
	{ "ISS",	0,	"",	 FL_PUP_NONE},
	{ "Stensat",	0,	"",	 FL_PUP_NONE},
	{ "OCS",	0,	"",	 FL_PUP_NONE},
	{0}
};

FD_map *create_form_map(void)
{
	FL_OBJECT *obj;

	FD_map *fdui = (FD_map *) fl_calloc(1, sizeof(*fdui));

	fdui->map = fl_bgn_form(FL_NO_BOX, 540, 450);
	obj = fl_add_box(FL_UP_BOX,0,0,540,450,"");
	fdui->t = obj = fl_add_bitmap(FL_NORMAL_BITMAP,20,20,500,250,"");
	fl_set_bitmap_data(obj, 500,250,world_bits);
	fl_set_object_color(obj,FL_BLUE,FL_GREEN);
	fl_set_object_lcol(obj,FL_PALEGREEN);
	fdui->g = obj = fl_add_timer(FL_NORMAL_TIMER,470,290,60,30,"timer");
	fl_set_object_callback(obj,j,0);
	fl_set_timer(obj,1);
	obj = fl_add_clock(FL_DIGITAL_CLOCK,10,290,90,30,"");
	fl_set_object_color(obj,FL_LIGHTER_COL1,FL_BLACK);
	fl_set_clock_adjustment(obj,(60*time_offset));
	fl_set_object_lcolor(obj,36);
	fl_set_object_lsize(obj,FL_NORMAL_SIZE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->a = obj = fl_add_text(FL_NORMAL_TEXT,20,340,30,30,"AZ");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->b = obj = fl_add_text(FL_NORMAL_TEXT,50,340,70,30,"");
	fl_set_object_boxtype(obj,FL_DOWN_BOX);
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	obj = fl_add_text(FL_NORMAL_TEXT,130,340,30,30,"EL");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->c = obj = fl_add_text(FL_NORMAL_TEXT,160,340,70,30,"");
	fl_set_object_boxtype(obj,FL_DOWN_BOX);
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	obj = fl_add_text(FL_NORMAL_TEXT,140,290,100,30,"Satellite");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	obj = fl_add_text(FL_NORMAL_TEXT,235,340,50,30,"Long");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->f = obj = fl_add_text(FL_NORMAL_TEXT,280,340,80,30,"");
	fl_set_object_boxtype(obj,FL_DOWN_BOX);
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	obj = fl_add_text(FL_NORMAL_TEXT,370,340,30,30,"Lat");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->h = obj = fl_add_text(FL_NORMAL_TEXT,400,340,80,30,"");
	fl_set_object_boxtype(obj,FL_DOWN_BOX);
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fdui->k = obj = fl_add_choice(FL_NORMAL_CHOICE2,230,290,120,30,"");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fl_set_object_callback(obj,l,0);
	fl_set_choice_entries(obj, __);
	obj = fl_add_text(FL_NORMAL_TEXT,100,390,100,30,"Next Event");
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	fl_set_object_lstyle(obj,FL_BOLD_STYLE);
	fdui->s = obj = fl_add_text(FL_NORMAL_TEXT,190,390,190,30,"");
	fl_set_object_boxtype(obj,FL_DOWN_BOX);
	fl_set_object_lsize(obj,FL_MEDIUM_SIZE);
	fl_set_object_lalign(obj,FL_ALIGN_LEFT|FL_ALIGN_INSIDE);                    
	fl_end_form();

	fdui->map->fdui = fdui;

	return fdui;
}
/*---------------------------------------*/


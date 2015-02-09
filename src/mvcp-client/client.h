/*
 * client.h -- MVCP client
 * Copyright (C) 2002-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MVCP_CLIENT_H_
#define _MVCP_CLIENT_H_

#include <stdio.h>
#include <pthread.h>
#include <mvcp/mvcp.h>

/** Queue for unit playback
*/

typedef struct 
{
	int mode;
	int unit;
	int position;
	int head;
	int tail;
	char list[ 50 ][ PATH_MAX + NAME_MAX ];
	int ignore;
}
*client_queue, client_queue_t;

/** Structure for storing app state. 
*/

typedef struct
{
	int disconnected;
	mvcp_parser parser;
	mvcp dv;
	int selected_unit;
	char current_directory[ 512 ];
	char last_directory[ 512 ];
	int showing;
	int terminated;
	pthread_t thread;
	client_queue_t queues[ MAX_UNITS ];
}
*client, client_t;

extern client client_init( mvcp_parser );
extern void client_run( client );
extern void client_close( client );

#endif

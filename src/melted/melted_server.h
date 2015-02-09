/*
 * melted_server.h
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

#ifndef _MELTED_SERVER_H_
#define _MELTED_SERVER_H_

/* System header files */
#include <pthread.h>

/* Application header files */
#include <mvcp/mvcp_parser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Servers default port
*/

#define DEFAULT_TCP_PORT 5250

/** Structure for the server
*/

typedef struct
{
	struct mlt_properties_s parent;
	char *id;
	int port;
	int socket;
	mvcp_parser parser;
	pthread_t thread;
	int shutdown;
	int proxy;
	char remote_server[ 50 ];
	int remote_port;
	char *config;
}
*melted_server, melted_server_t;

/** API for the server
*/

extern melted_server melted_server_init( char * );
extern const char *melted_server_id( melted_server );
extern void melted_server_set_config( melted_server, const char * );
extern void melted_server_set_port( melted_server, int );
extern void melted_server_set_proxy( melted_server, char * );
extern int melted_server_execute( melted_server );
extern mlt_properties melted_server_fetch_unit( melted_server, int );
extern void melted_server_shutdown( melted_server );
extern void melted_server_close( melted_server );

#ifdef __cplusplus
}
#endif

#endif

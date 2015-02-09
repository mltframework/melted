/*
 * melted_connection.c -- Connection Handler
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <arpa/inet.h>

#include <mvcp/mvcp_socket.h>

/* Application header files */
#include "melted_commands.h"
#include "melted_connection.h"
#include "melted_server.h"
#include "melted_log.h"

/** This is a generic replacement for fgets which operates on a file
   descriptor. Unlike fgets, we can also specify a line terminator. Maximum
   of (max - 1) chars can be read into buf from fd. If we reach the
   end-of-file, *eof_chk is set to 1. 
*/

int fdgetline( int fd, char *buf, int max, char line_terminator, int *eof_chk )
{
	int count = 0;
	char tmp [1];
	*eof_chk = 0;
	
	if (fd)
		while (count < max - 1) {
			if (read (fd, tmp, 1) > 0) {
				if (tmp [0] != line_terminator)
					buf [count++] = tmp [0];
				else
					break;

/* Is it an EOF character (ctrl-D, i.e. ascii 4)? If so we definitely want
   to break. */

				if (tmp [0] == 4) {
					*eof_chk = 1;
					break;
				}
			} else {
				*eof_chk = 1;
				break;
			}
		}
		
	buf [count] = '\0';
	
	return count;
}

static int connection_initiate( int );
static int connection_send( int, mvcp_response );
static int connection_read( int, char *, int );
static void connection_close( int );

static int connection_initiate( int fd )
{
	int error = 0;
	mvcp_response response = mvcp_response_init( );
	mvcp_response_set_error( response, 100, "VTR Ready" );
	error = connection_send( fd, response );
	mvcp_response_close( response );
	return error;
}

static int connection_send( int fd, mvcp_response response )
{
	int error = 0;
	int index = 0;
	int code = mvcp_response_get_error_code( response );

	if ( code != -1 )
	{
		int items = mvcp_response_count( response );

		if ( items == 0 )
			mvcp_response_set_error( response, 500, "Unknown error" );

		if ( code == 200 && items > 2 )
			mvcp_response_set_error( response, 201, "OK" );
		else if ( code == 200 && items > 1 )
			mvcp_response_set_error( response, 202, "OK" );

		code = mvcp_response_get_error_code( response );
		items = mvcp_response_count( response );

		for ( index = 0; !error && index < items; index ++ )
		{
			char *line = mvcp_response_get_line( response, index );
			int length = strlen( line );
			if ( length == 0 && index != mvcp_response_count( response ) - 1 && write( fd, " ", 1 ) != 1 )
				error = -1;
			else if ( length > 0 && write( fd, line, length ) != length )
				error = -1;
			if ( write( fd, "\r\n", 2 ) != 2 )
				error = -1;			
		}

		if ( ( code == 201 || code == 500 ) && strcmp( mvcp_response_get_line( response, items - 1 ), "" ) )
			if ( write( fd, "\r\n", 2 ) != 2 )
				melted_log( LOG_ERR, "write(\"\\r\\n\") failed!" );
	}
	else
	{
		const char *message = "500 Empty Response\r\n\r\n";
		if ( write( fd, message, strlen( message ) ) != strlen( message ))
			melted_log( LOG_ERR, "write(%s) failed!", message );
	}

	return error;
}

static int connection_read( int fd, char *command, int length )
{
	int eof_chk;
	int nchars = fdgetline( fd, command, length, '\n', &eof_chk );
	char *cr = strchr( command, '\r');
	if ( cr != NULL ) 
		cr[0] = '\0';
	if ( eof_chk || strncasecmp( command, "BYE", 3 ) == 0 ) 
		nchars = 0;
	return nchars;
}

int connection_status( int fd, mvcp_notifier notifier )
{
	int error = 0;
	int index = 0;
	mvcp_status_t status;
	char text[ 10240 ];
	mvcp_socket socket = mvcp_socket_init_fd( fd );
	
	for ( index = 0; !error && index < MAX_UNITS; index ++ )
	{
		mvcp_notifier_get( notifier, &status, index );
		mvcp_status_serialise( &status, text, sizeof( text ) );
		error = mvcp_socket_write_data( socket, text, strlen( text )  ) != strlen( text );
	}

	while ( !error )
	{
		if ( mvcp_notifier_wait( notifier, &status ) == 0 )
		{
			mvcp_status_serialise( &status, text, sizeof( text ) );
			error = mvcp_socket_write_data( socket, text, strlen( text ) ) != strlen( text );
		}
		else
		{
			struct timeval tv = { 0, 0 };
			fd_set rfds;

		    FD_ZERO( &rfds );
		    FD_SET( fd, &rfds );

			if ( select( socket->fd + 1, &rfds, NULL, NULL, &tv ) )
				error = 1;
		}
	}

	mvcp_socket_close( socket );
	
	return error;
}

static void connection_close( int fd )
{
	close( fd );
}

void *parser_thread( void *arg )
{
	struct hostent *he;
	connection_t *connection = arg;
	mlt_properties owner = connection->owner;
	char address[ 512 ];
	char command[ 1024 ];
	int fd = connection->fd;
	mvcp_parser parser = connection->parser;
	mvcp_response response = NULL;

	/* Get the connecting clients ip information */
	he = gethostbyaddr( (char *) &( connection->sin.sin_addr.s_addr ), sizeof(u_int32_t), AF_INET); 
	if ( he != NULL )
		strcpy( address, he->h_name );
	else
		inet_ntop( AF_INET, &( connection->sin.sin_addr.s_addr), address, 32 );

	melted_log( LOG_NOTICE, "Connection established with %s (%d)", address, fd );

	/* Execute the commands received. */
	if ( connection_initiate( fd ) == 0 )
	{
		int error = 0;

		while( !error && connection_read( fd, command, 1024 ) )
		{
			response = NULL;

			if ( !strcmp( command, "" ) )
			{
				// Ignore blank lines
				continue;
			}
			if ( !strncmp( command, "PUSH ", 5 ) )
			{
				// Append XML as clip
				char temp[ 20 ];
				int bytes;
				char *buffer = NULL;
				int total = 0;
				mlt_service service = NULL;

				connection_read( fd, temp, 20 );
				bytes = atoi( temp );
				buffer = malloc( bytes + 1 );
				while ( total < bytes )
				{
					int count = read( fd, buffer + total, bytes - total );
					if ( count >= 0 )
						total += count;
					else
						break;
				}
				buffer[ bytes ] = '\0';
				if ( bytes > 0 && total == bytes )
				{
					if ( mlt_properties_get( owner, "push-parser-off" ) == 0 )
					{
						mlt_profile profile = mlt_profile_init( NULL );
						profile->is_explicit = 1;
						service = ( mlt_service )mlt_factory_producer( profile, "xml-string", buffer );
						if ( service )
						{
							mlt_properties_set_data( MLT_SERVICE_PROPERTIES( service ), "melted_profile", profile,
								0, (mlt_destructor) mlt_profile_close, NULL );
							mlt_events_fire( owner, "push-received", &response, command, service, NULL );
							if ( response == NULL )
								response = mvcp_parser_push( parser, command, service );
						}
						else
						{
							response = mvcp_response_init();
							mvcp_response_set_error( response, RESPONSE_BAD_FILE, "Failed to load XML" );
						}
					}
					else
					{
						response = mvcp_parser_received( parser, command, buffer );
					}
				}
				melted_log( LOG_INFO, "%s \"%s\" %d", address, command, mvcp_response_get_error_code( response ) );
				error = connection_send( fd, response );
				mvcp_response_close( response );
				mlt_service_close( service );
				free( buffer );
			}
			else if ( strncmp( command, "STATUS", 6 ) )
			{
				// All other commands
				mlt_events_fire( owner, "command-received", &response, command, NULL );
				if ( response == NULL )
					response = mvcp_parser_execute( parser, command );
				melted_log( LOG_INFO, "%s \"%s\" %d", address, command, mvcp_response_get_error_code( response ) );
				error = connection_send( fd, response );
				mvcp_response_close( response );
			}
			else
			{
				// Start sending status repeatedly
				error = connection_status( fd, mvcp_parser_get_notifier( parser ) );
			}
		}
	}

	/* Free the resources associated with this connection. */
	connection_close( fd );

	melted_log( LOG_NOTICE, "Connection with %s (%d) closed", address, fd );

	free( connection );

	return NULL;
}

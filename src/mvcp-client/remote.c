/*
 * remote.c -- Remote MVCP client
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

/* System header files */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <mvcp/mvcp_remote.h>

/* Application header files */
#include "client.h"
#include "io.h"

static void show_usage( char *program_name )
{
	printf(
"Usage: %s [options] [mvcp-command* | file | -]\n"
"Options:\n"
"  -s host[:port]       Set server address and port (default 5250)\n"
"  -push unit           Push MLT XML from stdin/pipe\n"
"  -h                   Display usage help\n"
"No arguments initiates interactive text client.\n"
"The -s option is required to process MVCP commands whether in file, pipe, or command line.\n"
"MVCP commands on the command line that contain spaces must be quoted.\n",
		program_name
	);
}

/** Connect to a remote server.
*/

static mvcp_parser create_parser_prompt( )
{
	char server[ 132 ];
	int port;
	mvcp_parser parser = NULL;

	printf( "Connecting to a Server\n\n" );

	printf( "Server [localhost]: " );

	if ( io_get_string( server, sizeof( server ), "localhost" ) != NULL )
	{
		printf( "Port        [5250]: " );

		if ( get_int( &port, 5250 ) != NULL )
			parser = mvcp_parser_init_remote( server, port );
	}

	printf( "\n" );

	return parser;
}

static mvcp_parser create_parser_arg( char *arg )
{
	int port = 5250;
	char *portstr = strchr( arg, ':' );

	if ( portstr )
	{
		port = atoi( &portstr[1] );
		*portstr = 0;
	}
	printf( "Connecting to %s:%d\n", arg, port );
	return mvcp_parser_init_remote( arg, port );
}

void report( mvcp_response response )
{
	int index = 0;
	if ( response != NULL )
		for ( index = 0; index < mvcp_response_count( response ); index ++ )
			printf( "%s\n", mvcp_response_get_line( response, index ) );
}

static int parse_command_line( mvcp_parser parser, int argc, char **argv )
{
	mvcp_response response = mvcp_parser_connect( parser );
	int i;
	int interactive = 1;

	if ( response )
	{
		mvcp_response_close( response );
		response = NULL;
		for ( i = 1; i < argc; i++ )
		{
			// If pipe run stdin
			if ( !strcmp( argv[i], "-" ) )
			{
				response = mvcp_parser_run_file( parser, stdin );
				report( response );
				mvcp_response_close( response );
				interactive = 0;
				break;
			}
			// If '-push unit' option
			else if ( !strcmp( argv[i], "-push" ) && ( i + 1 ) < argc )
			{
				char command[10];
				char line[1024];
				char *buffer = NULL;
				size_t size = 0;

				sprintf( command, "PUSH U%s", argv[++i] );
				while ( fgets( line, 1024, stdin ) )
				{
					if ( !strcmp( line, "" ) )
						break;
					buffer = realloc( buffer, size + strlen( line ) );
					strcat( buffer, line);
					size += strlen( line ) + 1;
				}
				response = mvcp_parser_received( parser, command, buffer );
				if ( buffer )
					free( buffer );
				interactive = 0;
			}
			// Skip -s
			else if ( !strcmp( argv[i], "-s" ) && ( i + 1 ) < argc )
			{
				i++;
			}
			// If next arg is a readable file run it
			else if ( argv[i][0] != '-' && !access( argv[i], R_OK ) )
			{
				response = mvcp_parser_run( parser, argv[i] );
				interactive = 0;
			}
			// Otherwise execute command
			else if ( argv[i][0] != '-' )
			{
				response = mvcp_parser_execute( parser, argv[i] );
				interactive = 0;
			}
			if ( response )
			{
				report( response );
				mvcp_response_close( response );
				response = NULL;
			}
		}
	}
	else
	{
		printf( "Failed to connect\n" );
	}
	return interactive;
}

/** Main function.
*/

int main( int argc, char **argv )
{
	mvcp_parser parser = NULL;
	int i;
	int interactive = 1;

	for ( i = 1; i < argc; i++ )
	{
		if ( !strncmp( argv[i], "-h", 2 ) || !strncmp( argv[i], "--h", 3 ) )
		{
			show_usage( argv[0] );
			return 0;
		}
		else if ( !strcmp( argv[i], "-s" ) && ( i + 1 ) < argc )
		{
			parser = create_parser_arg( argv[++i] );
			if ( parser && argc > 3 )
				interactive = parse_command_line( parser, argc, argv );
		}
	}
	if ( !parser )
	{
		// Prompt for the server
		parser = create_parser_prompt( );
	}
	if ( interactive && parser)
	{
		client demo = client_init( parser );
		client_run( demo );
		client_close( demo );
	}
	mvcp_parser_close( parser );
	return 0;
}

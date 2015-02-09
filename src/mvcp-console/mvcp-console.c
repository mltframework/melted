/*
 * mvcp-console.c -- Local MVCP Test Utility
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
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#ifdef __DARWIN__
#include <SDL.h>
#endif

/* Application header files */
#include <melted/melted_local.h>
#include <melted/melted_commands.h>
#include <melted/melted_unit.h>
#include <mvcp/mvcp_remote.h>
#include <mvcp/mvcp_util.h>

char *prompt( char *command, int length )
{
	printf( "> " );
	return fgets( command, length, stdin );
}

void report( mvcp_response response )
{
	int index = 0;
	if ( response != NULL )
		for ( index = 0; index < mvcp_response_count( response ); index ++ )
			printf( "%4d: %s\n", index, mvcp_response_get_line( response, index ) );
}

static void cleanup()
{
	melted_delete_all_units();
}

int main( int argc, char **argv  )
{
	mvcp_parser parser = NULL;
	mvcp_response response = NULL;
	char temp[ 1024 ];
	int index = 1;

	atexit( cleanup );
	if ( argc > 2 && !strcmp( argv[ 1 ], "-s" ) )
	{
		printf( "Melted Client Instance\n" );
		parser = mvcp_parser_init_remote( argv[ 2 ], 5250 );
		response = mvcp_parser_connect( parser );
		index = 3;
	}
	else
	{
		printf( "Melted Standalone Instance\n" );
		parser = melted_parser_init_local( );
		response = mvcp_parser_connect( parser );
	}

	if ( response != NULL )
	{
		/* process files on command lines before going into console mode */
		for ( ; index < argc; index ++ )
		{
			mvcp_response_close( response );
			response = mvcp_parser_run( parser, argv[ index ] );
			report( response );
		}
	
		while ( response != NULL && prompt( temp, 1024 ) )
		{
			mvcp_util_trim( mvcp_util_chomp( temp ) );
			if ( !strcasecmp( temp, "BYE" ) )
			{
				break;
			}
			else if ( strcmp( temp, "" ) )
			{
				mvcp_response_close( response );
				response = mvcp_parser_execute( parser, temp );
				report( response );
			}
		}
	}
	else
	{
		fprintf( stderr, "Unable to connect to a melted instance.\n" );
	}

	printf( "\n" );
	mvcp_parser_close( parser );

	return 0;
}

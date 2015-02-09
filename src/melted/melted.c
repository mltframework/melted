/*
 * melted.c -- MLT Video TCP Server
 *
 * Copyright (C) 2002-2015 Meltytech, LLC
 * Authors:
 *     Dan Dennedy <dan@dennedy.org>
 *     Charles Yates <charles.yates@pandora.be>
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
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

#include <framework/mlt.h>
#include <mvcp/mvcp_notifier.h>
#include <mvcp/mvcp_status.h>

/* Application header files */
#include "melted_server.h"
#include "melted_log.h"
#include "melted_commands.h"
#include "melted_unit.h"

/** Our server context.
*/

static melted_server server = NULL;

/** atexit shutdown handler for the server.
*/

static void main_cleanup( )
{
	melted_server_close( server );
}

/** Report usage and exit.
*/

void usage( char *app )
{
	fprintf( stderr, "Usage: %s [-prio NNNN|max] [-test] [-port NNNN] [-c config-file]\n", app );
	exit( 0 );
}

/** The main function.
*/

int main( int argc, char **argv )
{
	int error = 0;
	int index = 0;
	int background = 1;
	int test = 0;
	struct timespec tm = { 1, 0 };
	mvcp_status_t status;
	struct {
		int clip_index;
		int is_logged;
	} asrun[ MAX_UNITS ];
	const char *config_file = "/etc/melted.conf";

#ifndef __DARWIN__
	for ( index = 1; index < argc; index ++ )
	{
		if ( !strcmp( argv[ index ], "-prio" ) )
		{
			struct sched_param scp;
			char* prio = argv[ ++ index ];

			memset( &scp, 0, sizeof( scp ) );

			if( !strcmp( prio, "max" ) )
				scp.sched_priority = sched_get_priority_max( SCHED_FIFO ) - 1;
			else
				scp.sched_priority = atoi(prio);

			sched_setscheduler( 0, SCHED_FIFO, &scp );
		}
	}
#endif

	mlt_factory_init( NULL );

	server = melted_server_init( argv[ 0 ] );

	for ( index = 1; index < argc; index ++ )
	{
		if ( !strcmp( argv[ index ], "-port" ) )
			melted_server_set_port( server, atoi( argv[ ++ index ] ) );
		else if ( !strcmp( argv[ index ], "-proxy" ) )
			melted_server_set_proxy( server, argv[ ++ index ] );
		else if ( !strcmp( argv[ index ], "-test" ) )
		{
			test = 1;
			background = 0;
		}
		else if ( !strcmp( argv[ index ], "-nodetach" ) )
			background = 0;
		else if ( !strcmp( argv[ index ], "-c" ) )
			config_file = argv[ ++ index ];
		else if ( !strcmp( argv[ index ], "-prio" ) )
			index++;
		else
			usage( argv[ 0 ] );
	}

	/* Optionally detatch ourselves from the controlling tty */

	if ( background )
	{
		if ( fork() )
			return 0;
		setsid();
		melted_log_init( log_syslog, LOG_NOTICE );
	}
	else if ( test )
	{
		mlt_log_set_level( MLT_LOG_VERBOSE );
		melted_log_init( log_stderr, LOG_DEBUG );
	}
	else
	{
		melted_log_init( log_syslog, LOG_NOTICE );
	}

	atexit( main_cleanup );

	/* Set the config script */
	melted_server_set_config( server, config_file );

	/* Execute the server */
	error = melted_server_execute( server );

	/* Initialize the as-run log tracking */
	for ( index = 0; index < MAX_UNITS; index ++ )
		asrun[ index ].clip_index = -1;

	/* We need to wait until we're exited.. */
	while ( !server->shutdown )
	{
		nanosleep( &tm, NULL );

		/* As-run logging */
		for ( index = 0; !error && index < MAX_UNITS; index ++ )
		{
			melted_unit unit = melted_get_unit( index );

			if ( unit && melted_unit_get_status( unit, &status ) == 0 )
			{
				int length = status.length - 60;

				/* Reset the logging if needed */
				if ( status.clip_index != asrun[ index ].clip_index || status.position < length || status.status == unit_not_loaded )
				{
					asrun[ index ].clip_index = status.clip_index;
					asrun[ index ].is_logged = 0;
				}
				/* Log as-run only once when near the end */
				if ( ! asrun[ index ].is_logged && status.length > 0 && status.position > length )
				{
					melted_log( LOG_NOTICE, "AS-RUN U%d \"%s\" len %d pos %d", index, status.clip, status.length, status.position );
					asrun[ index ].is_logged = 1;
				}
			}
		}
	}

	return error;
}

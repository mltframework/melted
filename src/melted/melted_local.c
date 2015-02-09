/*
 * melted_local.c -- Local Melted Parser
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Needed for backtrace on linux */
#ifdef linux
#include <execinfo.h>
#endif

/* mvcp header files */
#include <mvcp/mvcp_util.h>

/* MLT header files. */
#include <framework/mlt_factory.h>

/* Application header files */
#include "melted_local.h"
#include "melted_connection.h"
#include "melted_commands.h"
#include "melted_unit_commands.h"
#include "melted_log.h"

/** Private melted_local structure.
*/

typedef struct
{
	mvcp_parser parser;
	char root_dir[1024];
}
*melted_local, melted_local_t;

/** Forward declarations.
*/

static mvcp_response melted_local_connect( melted_local );
static mvcp_response melted_local_execute( melted_local, char * );
static mvcp_response melted_local_push( melted_local, char *, mlt_service );
static mvcp_response melted_local_receive( melted_local, char *, char * );
static void melted_local_close( melted_local );
response_codes melted_help( command_argument arg );
response_codes melted_run( command_argument arg );
response_codes melted_shutdown( command_argument arg );

/** MVCP Parser constructor.
*/

mvcp_parser melted_parser_init_local( )
{
	mvcp_parser parser = malloc( sizeof( mvcp_parser_t ) );
	melted_local local = malloc( sizeof( melted_local_t ) );

	if ( parser != NULL )
	{
		memset( parser, 0, sizeof( mvcp_parser_t ) );

		parser->connect = (parser_connect)melted_local_connect;
		parser->execute = (parser_execute)melted_local_execute;
		parser->push = (parser_push)melted_local_push;
		parser->received = (parser_received)melted_local_receive;
		parser->close = (parser_close)melted_local_close;
		parser->real = local;

		if ( local != NULL )
		{
			memset( local, 0, sizeof( melted_local_t ) );
			local->parser = parser;
			local->root_dir[0] = '/';
		}

		// Construct the factory
		mlt_factory_init( getenv( "MLT_REPOSITORY" ) );
	}
	return parser;
}

/** response status code/message pair 
*/

typedef struct 
{
	int code;
	const char *message;
} 
responses_t;

/** response messages 
*/

static responses_t responses [] = 
{
	{RESPONSE_SUCCESS, "OK"},
	{RESPONSE_SUCCESS_N, "OK"},
	{RESPONSE_SUCCESS_1, "OK"},
	{RESPONSE_UNKNOWN_COMMAND, "Unknown command"},
	{RESPONSE_TIMEOUT, "Operation timed out"},
	{RESPONSE_MISSING_ARG, "Argument missing"},
	{RESPONSE_INVALID_UNIT, "Unit not found"},
	{RESPONSE_BAD_FILE, "Failed to locate or open clip"},
	{RESPONSE_OUT_OF_RANGE, "Argument value out of range"},
	{RESPONSE_TOO_MANY_FILES, "Too many files open"},
	{RESPONSE_ERROR, "Server Error"}
};

/** Argument types.
*/

typedef enum 
{
	ATYPE_NONE,
	ATYPE_FLOAT,
	ATYPE_STRING,
	ATYPE_INT,
	ATYPE_PAIR
} 
arguments_types;

/** A command definition.
*/

typedef struct 
{
/* The command string corresponding to this operation (e.g. "play") */
	const char *command;
/* The function associated with it */
	response_codes (*operation) ( command_argument );
/* a boolean to indicate if this is a unit or global command
   unit commands require a unit identifier as first argument */
	int is_unit;
/* What type is the argument (RTTI :-) ATYPE_whatever */
	int type;
/* online help information */
	const char *help;
} 
command_t;

/* The following define the queue of commands available to the user. The
   first entry is the name of the command (the string which must be typed),
   the second command is the function associated with it, the third argument
   is for the type of the argument, and the last argument specifies whether
   this is something which should be handled immediately or whether it
   should be queued (only robot motion commands need to be queued). */

static command_t vocabulary[] = 
{
	{"BYE", NULL, 0, ATYPE_NONE, "Terminates the session. Units are not removed and task queue is not flushed."},
	{"HELP", melted_help, 0, ATYPE_NONE, "Display this information!"},
	{"NLS", melted_list_nodes, 0, ATYPE_NONE, "List the AV/C nodes on the 1394 bus."},
	{"UADD", melted_add_unit, 0, ATYPE_STRING, "Create a new playout unit (virtual VTR) to transmit to receiver specified in GUID argument."},
	{"ULS", melted_list_units, 0, ATYPE_NONE, "Lists the units that have already been added to the server."},
	{"CLS", melted_list_clips, 0, ATYPE_STRING, "Lists the clips at directory name argument."},
	{"SET", melted_set_global_property, 0, ATYPE_PAIR, "Set a server configuration property."},
	{"GET", melted_get_global_property, 0, ATYPE_STRING, "Get a server configuration property."},
	{"RUN", melted_run, 0, ATYPE_STRING, "Run a batch file." },
	{"LIST", melted_list, 1, ATYPE_NONE, "List the playlist associated to a unit."},
	{"LOAD", melted_load, 1, ATYPE_STRING, "Load clip specified in absolute filename argument."},
	{"INSERT", melted_insert, 1, ATYPE_STRING, "Insert a clip at the given clip index."},
	{"REMOVE", melted_remove, 1, ATYPE_NONE, "Remove a clip at the given clip index."},
	{"CLEAN", melted_clean, 1, ATYPE_NONE, "Clean a unit by removing all but the currently playing clip."},
	{"WIPE", melted_wipe, 1, ATYPE_NONE, "Clean a unit by removing everything before the currently playing clip."},
	{"CLEAR", melted_clear, 1, ATYPE_NONE, "Clear a unit by removing all clips."},
	{"MOVE", melted_move, 1, ATYPE_INT, "Move a clip to another clip index."},
	{"APND", melted_append, 1, ATYPE_STRING, "Append a clip specified in absolute filename argument."},
	{"PLAY", melted_play, 1, ATYPE_NONE, "Play a loaded clip at speed -2000 to 2000 where 1000 = normal forward speed."},
	{"STOP", melted_stop, 1, ATYPE_NONE, "Stop a loaded and playing clip."},
	{"PAUSE", melted_pause, 1, ATYPE_NONE, "Pause a playing clip."},
	{"REW", melted_rewind, 1, ATYPE_NONE, "Rewind a unit. If stopped, seek to beginning of clip. If playing, play fast backwards."},
	{"FF", melted_ff, 1, ATYPE_NONE, "Fast forward a unit. If stopped, seek to beginning of clip. If playing, play fast forwards."},
	{"STEP", melted_step, 1, ATYPE_INT, "Step argument number of frames forward or backward."},
	{"GOTO", melted_goto, 1, ATYPE_INT, "Jump to frame number supplied as argument."},
	{"SIN", melted_set_in_point, 1, ATYPE_INT, "Set the IN point of the loaded clip to frame number argument. -1 = reset in point to 0"},
	{"SOUT", melted_set_out_point, 1, ATYPE_INT, "Set the OUT point of the loaded clip to frame number argument. -1 = reset out point to maximum."},
	{"USTA", melted_get_unit_status, 1, ATYPE_NONE, "Report information about the unit."},
	{"USET", melted_set_unit_property, 1, ATYPE_PAIR, "Set a unit configuration property."},
	{"UGET", melted_get_unit_property, 1, ATYPE_STRING, "Get a unit configuration property."},
	{"XFER", melted_transfer, 1, ATYPE_STRING, "Transfer the unit's clip to another unit specified as argument."},
	{"SHUTDOWN", melted_shutdown, 0, ATYPE_NONE, "Shutdown the server."},
	{NULL, NULL, 0, ATYPE_NONE, NULL}
};

/** Usage message 
*/

static char helpstr [] = 
	"melted -- A Multimedia Playout Server\n" 
	"	Copyright (C) 2002-2015 Meltytech, LLC\n"
	"	Authors:\n"
	"		Dan Dennedy <dan@dennedy.org>\n"
	"		Charles Yates <charles.yates@pandora.be>\n"
	"Available commands:\n";

/** Lookup the response message for a status code.
*/

inline const char *get_response_msg( int code )
{
	int i = 0;
	for ( i = 0; responses[ i ].message != NULL && code != responses[ i ].code; i ++ ) ;
	return responses[ i ].message;
}

/** Tell the user the melted command set
*/

response_codes melted_help( command_argument cmd_arg )
{
	int i = 0;
	
	mvcp_response_printf( cmd_arg->response, 10240, "%s", helpstr );
	
	for ( i = 0; vocabulary[ i ].command != NULL; i ++ )
		mvcp_response_printf( cmd_arg->response, 1024,
							"%-10.10s%s\n", 
							vocabulary[ i ].command, 
							vocabulary[ i ].help );

	mvcp_response_printf( cmd_arg->response, 2, "\n" );

	return RESPONSE_SUCCESS_N;
}

/** Execute a batch file.
*/

response_codes melted_run( command_argument cmd_arg )
{
	mvcp_response temp = mvcp_parser_run( cmd_arg->parser, (char *)cmd_arg->argument );

	if ( temp != NULL )
	{
		int index = 0;

		mvcp_response_set_error( cmd_arg->response, 
							   mvcp_response_get_error_code( temp ),
							   mvcp_response_get_error_string( temp ) );

		for ( index = 1; index < mvcp_response_count( temp ); index ++ )
			mvcp_response_printf( cmd_arg->response, 10240, "%s\n", mvcp_response_get_line( temp, index ) );

		mvcp_response_close( temp );
	}

	return mvcp_response_get_error_code( cmd_arg->response );
}

response_codes melted_shutdown( command_argument cmd_arg )
{
	exit( 0 );
	return RESPONSE_SUCCESS;
}

/** Processes 'thread' id
*/

static pthread_t self;

/* Signal handler to deal with various shutdown signals. Basically this
   should clean up and power down the motor. Note that the death of any
   child thread will kill all thrads. */

void signal_handler( int sig )
{
	if ( pthread_equal( self, pthread_self( ) ) )
	{

#ifdef _GNU_SOURCE
		melted_log( LOG_DEBUG, "Received %s - shutting down.", strsignal(sig) );
#else
		melted_log( LOG_DEBUG, "Received signal %i - shutting down.", sig );
#endif

		exit(EXIT_SUCCESS);
	}
}

static void sigsegv_handler()
{
#ifdef linux
	void *array[ 10 ];
	size_t size;
	char **strings;
	size_t i;

	melted_log( LOG_CRIT, "\a\nmelted experienced a segmentation fault.\n"
		"Dumping stack from the offending thread\n\n" );
	size = backtrace( array, 10 );
	strings = backtrace_symbols( array, size );

	melted_log( LOG_CRIT, "Obtained %zd stack frames.\n", size );

	for ( i = 0; i < size; i++ )
		 melted_log( LOG_CRIT, "%s", strings[ i ] );

	free( strings );

	melted_log( LOG_CRIT, "\nDone dumping - exiting.\n" );
#else
	melted_log( LOG_CRIT, "\a\nmelted experienced a segmentation fault.\n" );
#endif
	exit( EXIT_FAILURE );
}



/** Local 'connect' function.
*/

static mvcp_response melted_local_connect( melted_local local )
{
	mvcp_response response = mvcp_response_init( );

	self = pthread_self( );

	mvcp_response_set_error( response, 100, "VTR Ready" );

	signal( SIGHUP, signal_handler );
	signal( SIGINT, signal_handler );
	signal( SIGTERM, SIG_DFL );
	signal( SIGSTOP, signal_handler );
	signal( SIGPIPE, signal_handler );
	signal( SIGALRM, signal_handler );
	signal( SIGCHLD, SIG_IGN );
	if ( getenv( "MLT_SIGSEGV" ) )
		signal( SIGSEGV, sigsegv_handler );

	return response;
}

/** Set the error and determine the message associated to this command.
*/

void melted_command_set_error( command_argument cmd, response_codes code )
{
	mvcp_response_set_error( cmd->response, code, get_response_msg( code ) );
}

/** Parse the unit argument.
*/

int melted_command_parse_unit( command_argument cmd, int argument )
{
	int unit = -1;
	char *string = mvcp_tokeniser_get_string( cmd->tokeniser, argument );
	if ( string != NULL && ( string[ 0 ] == 'U' || string[ 0 ] == 'u' ) && strlen( string ) > 1 )
		unit = atoi( string + 1 );
	return unit;
}

/** Parse a normal argument.
*/

void *melted_command_parse_argument( command_argument cmd, int argument, arguments_types type, char *command )
{
	void *ret = NULL;
	char *value = mvcp_tokeniser_get_string( cmd->tokeniser, argument );

	if ( value != NULL )
	{
		switch( type )
		{
			case ATYPE_NONE:
				break;

			case ATYPE_FLOAT:
				ret = malloc( sizeof( float ) );
				if ( ret != NULL )
					*( float * )ret = atof( value );
				break;

			case ATYPE_STRING:
				ret = strdup( value );
				break;
					
			case ATYPE_PAIR:
				if ( strchr( command, '=' ) )
				{
					char *ptr = strchr( command, '=' );
					while ( *( ptr - 1 ) != ' ' ) 
						ptr --;
					ret = strdup( ptr );
					ptr = ret;
					while( ptr[ strlen( ptr ) - 1 ] == ' ' )
						ptr[ strlen( ptr ) - 1 ] = '\0';
				}
				break;

			case ATYPE_INT:
				ret = malloc( sizeof( int ) );
				if ( ret != NULL )
					*( int * )ret = atoi( value );
				break;
		}
	}

	return ret;
}

/** Get the error code - note that we simply the success return.
*/

response_codes melted_command_get_error( command_argument cmd )
{
	response_codes ret = mvcp_response_get_error_code( cmd->response );
	if ( ret == RESPONSE_SUCCESS_N || ret == RESPONSE_SUCCESS_1 )
		ret = RESPONSE_SUCCESS;
	return ret;
}

/** Execute the command.
*/

static mvcp_response melted_local_execute( melted_local local, char *command )
{
	command_argument_t cmd;
	cmd.parser = local->parser;
	cmd.response = mvcp_response_init( );
	cmd.tokeniser = mvcp_tokeniser_init( );
	cmd.command = command;
	cmd.unit = -1;
	cmd.argument = NULL;
	cmd.root_dir = local->root_dir;

	/* Set the default error */
	melted_command_set_error( &cmd, RESPONSE_UNKNOWN_COMMAND );

	/* Parse the command */
	if ( mvcp_tokeniser_parse_new( cmd.tokeniser, command, " " ) > 0 )
	{
		int index = 0;
		char *value = mvcp_tokeniser_get_string( cmd.tokeniser, 0 );
		int found = 0;

		/* Strip quotes from all tokens */
		for ( index = 0; index < mvcp_tokeniser_count( cmd.tokeniser ); index ++ )
			mvcp_util_strip( mvcp_tokeniser_get_string( cmd.tokeniser, index ), '\"' );

		/* Search the vocabulary array for value */
		for ( index = 1; !found && vocabulary[ index ].command != NULL; index ++ )
			if ( ( found = !strcasecmp( vocabulary[ index ].command, value ) ) )
				break;

		/* If we found something, the handle the args and call the handler. */
		if ( found )
		{
			int position = 1;

			melted_command_set_error( &cmd, RESPONSE_SUCCESS );

			if ( vocabulary[ index ].is_unit )
			{
				cmd.unit = melted_command_parse_unit( &cmd, position );
				if ( cmd.unit == -1 )
					melted_command_set_error( &cmd, RESPONSE_MISSING_ARG );
				position ++;
			}

			if ( melted_command_get_error( &cmd ) == RESPONSE_SUCCESS )
			{
				cmd.argument = melted_command_parse_argument( &cmd, position, vocabulary[ index ].type, command );
				if ( cmd.argument == NULL && vocabulary[ index ].type != ATYPE_NONE )
					melted_command_set_error( &cmd, RESPONSE_MISSING_ARG );
				position ++;
			}

			if ( melted_command_get_error( &cmd ) == RESPONSE_SUCCESS )
			{
				response_codes error = vocabulary[ index ].operation( &cmd );
				melted_command_set_error( &cmd, error );
			}

			free( cmd.argument );
		}
	}

	mvcp_tokeniser_close( cmd.tokeniser );

	return cmd.response;
}

static mvcp_response melted_local_receive( melted_local local, char *command, char *doc )
{
	command_argument_t cmd;
	cmd.parser = local->parser;
	cmd.response = mvcp_response_init( );
	cmd.tokeniser = mvcp_tokeniser_init( );
	cmd.command = command;
	cmd.unit = -1;
	cmd.argument = NULL;
	cmd.root_dir = local->root_dir;

	/* Set the default error */
	melted_command_set_error( &cmd, RESPONSE_SUCCESS );

	/* Parse the command */
	if ( mvcp_tokeniser_parse_new( cmd.tokeniser, command, " " ) > 0 )
	{
		int index = 0;
		int position = 1;

		/* Strip quotes from all tokens */
		for ( index = 0; index < mvcp_tokeniser_count( cmd.tokeniser ); index ++ )
			mvcp_util_strip( mvcp_tokeniser_get_string( cmd.tokeniser, index ), '\"' );

		cmd.unit = melted_command_parse_unit( &cmd, position );
		if ( cmd.unit == -1 )
			melted_command_set_error( &cmd, RESPONSE_MISSING_ARG );
		position ++;

		melted_receive( &cmd, doc );
		melted_command_set_error( &cmd, RESPONSE_SUCCESS );

		free( cmd.argument );
	}

	mvcp_tokeniser_close( cmd.tokeniser );

	return cmd.response;
}

static mvcp_response melted_local_push( melted_local local, char *command, mlt_service service )
{
	command_argument_t cmd;
	cmd.parser = local->parser;
	cmd.response = mvcp_response_init( );
	cmd.tokeniser = mvcp_tokeniser_init( );
	cmd.command = command;
	cmd.unit = -1;
	cmd.argument = NULL;
	cmd.root_dir = local->root_dir;

	/* Set the default error */
	melted_command_set_error( &cmd, RESPONSE_SUCCESS );

	/* Parse the command */
	if ( mvcp_tokeniser_parse_new( cmd.tokeniser, command, " " ) > 0 )
	{
		int index = 0;
		int position = 1;

		/* Strip quotes from all tokens */
		for ( index = 0; index < mvcp_tokeniser_count( cmd.tokeniser ); index ++ )
			mvcp_util_strip( mvcp_tokeniser_get_string( cmd.tokeniser, index ), '\"' );

		cmd.unit = melted_command_parse_unit( &cmd, position );
		if ( cmd.unit == -1 )
			melted_command_set_error( &cmd, RESPONSE_MISSING_ARG );
		position ++;

		melted_push( &cmd, service );
		melted_command_set_error( &cmd, RESPONSE_SUCCESS );

		free( cmd.argument );
	}

	mvcp_tokeniser_close( cmd.tokeniser );

	return cmd.response;
}

/** Close the parser.
*/

static void melted_local_close( melted_local local )
{
	melted_delete_all_units();
#ifdef linux
	//pthread_kill_other_threads_np();
	melted_log( LOG_DEBUG, "Clean shutdown." );
	//free( local );
	//mlt_factory_close( );
#endif
}

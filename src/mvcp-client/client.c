/*
 * client.c -- MVCP client
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

/* Application header files */
#include "client.h"
#include "io.h"

/** Clip navigation enumeration.
*/

typedef enum
{
	absolute,
	relative
}
client_whence;

/** Function prototype for menu handling. 
*/

typedef mvcp_error_code (*demo_function)( client );

/** The menu structure. 
*/

typedef struct
{
	const char *description;
	struct menu_item
	{
		const char *option;
		demo_function function;
	}
	array[ 50 ];
}
*client_menu, client_menu_t;

/** Forward reference to menu runner.
*/

extern mvcp_error_code client_run_menu( client, client_menu );

/** Foward references. 
*/

extern mvcp_error_code client_list_nodes( client );
extern mvcp_error_code client_add_unit( client );
extern mvcp_error_code client_select_unit( client );
extern mvcp_error_code client_execute( client );
extern mvcp_error_code client_load( client );
extern mvcp_error_code client_transport( client );
static void *client_status_thread( void * );

/** Connected menu definition. 
*/

client_menu_t connected_menu =
{
	"Connected Menu",
	{
		{ "Add Unit", client_add_unit },
		{ "Select Unit", client_select_unit },
		{ "Command Shell", client_execute },
		{ NULL, NULL }
	}
};

/** Initialise the demo structure.
*/

client client_init( mvcp_parser parser )
{
	client this = malloc( sizeof( client_t ) );
	if ( this != NULL )
	{
		int index = 0;
		memset( this, 0, sizeof( client_t ) );
		strcpy( this->last_directory, "/" );
		for ( index = 0; index < 4; index ++ )
		{
			this->queues[ index ].unit = index;
			this->queues[ index ].position = -1;
		}
		this->parser = parser;
	}
	return this;
}

/** Display a status record.
*/

void client_show_status( client demo, mvcp_status status )
{
	if ( status->unit == demo->selected_unit && demo->showing )
	{
		char temp[ 1024 ] = "";

		sprintf( temp, "U%d ", demo->selected_unit );

		switch( status->status )
		{
			case unit_offline:
				strcat( temp, "offline   " );
				break;
			case unit_undefined:
				strcat( temp, "undefined " );
				break;
			case unit_not_loaded:
				strcat( temp, "unloaded  " );
				break;
			case unit_stopped:
				strcat( temp, "stopped   " );
				break;
			case unit_playing:
				strcat( temp, "playing   " );
				break;
			case unit_paused:
				strcat( temp, "paused    " );
				break;
			case unit_disconnected:
				strcat( temp, "disconnect" );
				break;
			default:
				strcat( temp, "unknown   " );
				break;
		}

		sprintf( temp + strlen( temp ), " %9d %9d %9d ", status->in, status->position, status->out );
		strcat( temp, status->clip );

		printf( "%-80.80s\r", temp );
		fflush( stdout );
	}
}

/** Determine action to carry out as dictated by the client unit queue.
*/

void client_queue_action( client demo, mvcp_status status )
{
	client_queue queue = &demo->queues[ status->unit ];

	/* SPECIAL CASE STATUS NOTIFICATIONS TO IGNORE */

	/* When we've issued a LOAD on the previous notification, then ignore this one. */
	if ( queue->ignore )
	{
		queue->ignore --;
		return;
	}

	if ( queue->mode && status->status != unit_offline && queue->head != queue->tail )
	{
		if ( ( status->position >= status->out && status->speed > 0 ) || status->status == unit_not_loaded )
		{
			queue->position = ( queue->position + 1 ) % 50;
			if ( queue->position == queue->tail )
				queue->position = queue->head;
			mvcp_unit_load( demo->dv, status->unit, queue->list[ queue->position ] );
			if ( status->status == unit_not_loaded )
				mvcp_unit_play( demo->dv, queue->unit );
			queue->ignore = 1;
		}
		else if ( ( status->position <= status->in && status->speed < 0 ) || status->status == unit_not_loaded )
		{
			if ( queue->position == -1 )
				queue->position = queue->head;
			mvcp_unit_load( demo->dv, status->unit, queue->list[ queue->position ] );
			if ( status->status == unit_not_loaded )
				mvcp_unit_play( demo->dv, queue->unit );
			queue->position = ( queue->position - 1 ) % 50;
			queue->ignore = 1;
		}
	}
}

/** Status thread.
*/

static void *client_status_thread( void *arg )
{
	client demo = arg;
	mvcp_status_t status;
	mvcp_notifier notifier = mvcp_get_notifier( demo->dv );

	while ( !demo->terminated )
	{
		if ( mvcp_notifier_wait( notifier, &status ) == 0 )
		{
			client_queue_action( demo, &status );
			client_show_status( demo, &status );
			if ( status.status == unit_disconnected )
				demo->disconnected = 1;
		}
	}

	return NULL;
}

/** Turn on/off status display.
*/

void client_change_status( client demo, int flag )
{
	if ( demo->disconnected && flag )
	{
		mvcp_error_code error = mvcp_connect( demo->dv );
		if ( error == mvcp_ok )
			demo->disconnected = 0;
		else
			beep();
	}

	if ( flag )
	{
		mvcp_status_t status;
		mvcp_notifier notifier = mvcp_get_notifier( demo->dv );
		mvcp_notifier_get( notifier, &status, demo->selected_unit );
		demo->showing = 1;
		client_show_status( demo, &status );
	}
	else
	{
		demo->showing = 0;
		printf( "%-80.80s\r", " " );
		fflush( stdout );
	}
}

/** Add a unit.
*/

mvcp_error_code client_add_unit( client demo )
{
	mvcp_error_code error = mvcp_ok;
	mvcp_nodes nodes = mvcp_nodes_init( demo->dv );
	mvcp_units units = mvcp_units_init( demo->dv );

	if ( mvcp_nodes_count( nodes ) != -1 && mvcp_units_count( units ) != -1 )
	{
		char pressed;
		mvcp_node_entry_t node;
		mvcp_unit_entry_t unit;
		int node_index = 0;
		int unit_index = 0;

		printf( "Select a Node\n\n" );

		for ( node_index = 0; node_index < mvcp_nodes_count( nodes ); node_index ++ )
		{
			mvcp_nodes_get( nodes, node_index, &node );
			printf( "%d: %s - %s ", node_index + 1, node.guid, node.name );
			for ( unit_index = 0; unit_index < mvcp_units_count( units ); unit_index ++ )
			{
				mvcp_units_get( units, unit_index, &unit );
				if ( !strcmp( unit.guid, node.guid ) )
					printf( "[U%d] ", unit.unit );
			}
			printf( "\n" );
		}

		printf( "0. Exit\n\n" );

		printf( "Node: " );

		while ( ( pressed = get_keypress( ) ) != '0' )
		{
			node_index = pressed - '1';
			if ( node_index >= 0 && node_index < mvcp_nodes_count( nodes ) )
			{
				int unit;
				printf( "%c\n\n", pressed );
				mvcp_nodes_get( nodes, node_index, &node );
				if ( mvcp_unit_add( demo->dv, node.guid, &unit ) == mvcp_ok )
				{
					printf( "Unit added as U%d\n", unit );
					demo->selected_unit = unit;
				}
				else
				{
					int index = 0;
					mvcp_response response = mvcp_get_last_response( demo->dv );
					printf( "Failed to add unit:\n\n" );
					for( index = 1; index < mvcp_response_count( response ) - 1; index ++ )
						printf( "%s\n", mvcp_response_get_line( response, index ) );
				}
				printf( "\n" );
				wait_for_any_key( NULL );
				break;
			}
			else
			{
				beep( );
			}
		}
	}
	else
	{
		printf( "Invalid response from the server.\n\n" );
		wait_for_any_key( NULL );
	}

	mvcp_nodes_close( nodes );
	mvcp_units_close( units );

	return error;
}

/** Select a unit.
*/

mvcp_error_code client_select_unit( client demo )
{
	int terminated = 0;
	int refresh = 1;

	while ( !terminated )
	{
		mvcp_units units = mvcp_units_init( demo->dv );

		if ( mvcp_units_count( units ) > 0 )
		{
			mvcp_unit_entry_t unit;
			int index = 0;
			char key = '\0';

			if ( refresh )
			{
				printf( "Select a Unit\n\n" );

				for ( index = 0; index < mvcp_units_count( units ); index ++ )
				{
					mvcp_units_get( units, index, &unit );
					printf( "%d: U%d - %s [%s]\n", index + 1, 
												   unit.unit, 
												   unit.guid, 
												   unit.online ? "online" : "offline" );
				}
				printf( "0: Exit\n\n" );

				printf( "Unit [%d]: ", demo->selected_unit + 1 );
				refresh = 0;
			}

			key = get_keypress( );

			if ( key == '\r' )
				key = demo->selected_unit + '1';

			if ( key != '0' )
			{
				if ( key >= '1' && key < '1' + mvcp_units_count( units ) )
				{
					demo->selected_unit = key - '1';
					printf( "%c\n\n", key );
					client_load( demo );
					refresh = 1;
				}
				else
				{
					beep( );
				}					
			}
			else
			{
				printf( "0\n\n" );
				terminated = 1;
			}
		}
		else if ( mvcp_units_count( units ) == 0 )
		{
			printf( "No units added - add a unit first\n\n" );
			client_add_unit( demo );
		}
		else
		{
			printf( "Unable to obtain Unit List.\n" );
			terminated = 1;
		}

		mvcp_units_close( units );
	}

	return mvcp_ok;
}

/** Execute an arbitrary command.
*/

mvcp_error_code client_execute( client demo )
{
	mvcp_error_code error = mvcp_ok;
	char command[ 10240 ];
	int terminated = 0;

	printf( "Melted Shell\n" );
	printf( "Enter an empty command to exit.\n\n" );

	while ( !terminated )
	{
		terminated = 1;
		printf( "Command> " );

		if ( chomp( io_get_string( command, 10240, "" ) ) != NULL )
		{
			if ( strcmp( command, "" ) )
			{
				int index = 0;
				mvcp_response response = NULL;
				error = mvcp_execute( demo->dv, 10240, command );
				printf( "\n" );
				response = mvcp_get_last_response( demo->dv );
				for ( index = 0; index < mvcp_response_count( response ); index ++ )
				{
					char *line = mvcp_response_get_line( response, index );
					printf( "%4d: %s\n", index, line );
				}
				printf( "\n" );
				terminated = 0;
			}
		}
	}

	printf( "\n" );

	return error;
}

/** Add a file to the queue.
*/

mvcp_error_code client_queue_add( client demo, client_queue queue, char *file )
{
	mvcp_status_t status;
	mvcp_notifier notifier = mvcp_get_notifier( demo->dv );

	if ( ( queue->tail + 1 ) % 50 == queue->head )
		queue->head = ( queue->head + 1 ) % 50;
	strcpy( queue->list[ queue->tail ], file );
	queue->tail = ( queue->tail + 1 ) % 50;

	mvcp_notifier_get( notifier, &status, queue->unit );
	mvcp_notifier_put( notifier, &status );

	return mvcp_ok;
}

/** Basic queue maintenance and status reports.
*/

mvcp_error_code client_queue_maintenance( client demo, client_queue queue )
{
	printf( "Queue Maintenance for Unit %d\n\n", queue->unit );

	if ( !queue->mode )
	{
		char ch;
		printf( "Activate queueing? [Y] " );
		ch = get_keypress( );
		if ( ch == 'y' || ch == 'Y' || ch == '\r' )
			queue->mode = 1;
		printf( "\n\n" );
	}

	if ( queue->mode )
	{
		int terminated = 0;
		int last_position = -2;

		term_init( );

		while ( !terminated )
		{
			int first = ( queue->position + 1 ) % 50;
			int index = first;

			if ( first == queue->tail )
				index = first = queue->head;

			if ( queue->head == queue->tail )
			{
				if ( last_position == -2 )
				{
					printf( "Queue is empty\n" );
					printf( "\n" );
					printf( "0 = exit, t = turn off queueing\n\n" );
					last_position = -1;
				}
			}
			else if ( last_position != queue->position )
			{
				printf( "Order of play\n\n" );

				do 
				{
					printf( "%c%02d: %s\n", index == first ? '*' : ' ', index, queue->list[ index ] + 1 );
					index = ( index + 1 ) % 50;
					if ( index == queue->tail )
						index = queue->head;
				}
				while( index != first );
	
				printf( "\n" );
				printf( "0 = exit, t = turn off queueing, c = clear queue\n\n" );
				last_position = queue->position;
			}

			client_change_status( demo, 1 );
			
			switch( term_read( ) )
			{
				case -1:
					break;
				case '0':
					terminated = 1;
					break;
				case 't':
					terminated = 1;
					queue->mode = 0;
					break;
				case 'c':
					queue->head = queue->tail = 0;
					queue->position = -1;
					last_position = -2;
					break;
			}

			client_change_status( demo, 0 );
		}

		term_exit( );
	}

	return mvcp_ok;
}

/** Load a file to the selected unit. Horrible function - sorry :-/. Not a good
	demo....
*/

mvcp_error_code client_load( client demo )
{
	mvcp_error_code error = mvcp_ok;
	int terminated = 0;
	int refresh = 1;
	int start = 0;

	strcpy( demo->current_directory, demo->last_directory );

	term_init( );

	while ( !terminated )
	{
		mvcp_dir dir = mvcp_dir_init( demo->dv, demo->current_directory );

		if ( mvcp_dir_count( dir ) == -1 )
		{
			printf( "Invalid directory - retrying %s\n", demo->last_directory );
			mvcp_dir_close( dir );
			dir = mvcp_dir_init( demo->dv, demo->last_directory );
			if ( mvcp_dir_count( dir ) == -1 )
			{
				printf( "Invalid directory - going back to /\n" );
				mvcp_dir_close( dir );
				dir = mvcp_dir_init( demo->dv, "/" );
				strcpy( demo->current_directory, "/" );
			}
			else
			{
				strcpy( demo->current_directory, demo->last_directory );
			}
		}

		terminated = mvcp_dir_count( dir ) == -1;

		if ( !terminated )
		{
			int index = 0;
			int selected = 0;
			int max = 9;
			int end = 0;

			end = mvcp_dir_count( dir );

			strcpy( demo->last_directory, demo->current_directory );

			while ( !selected && !terminated )
			{
				mvcp_dir_entry_t entry;
				int pressed;

				if ( refresh )
				{
					const char *action = "Load & Play";
					if ( demo->queues[ demo->selected_unit ].mode )
						action = "Queue";
					printf( "%s from %s\n\n", action, demo->current_directory );
					if ( strcmp( demo->current_directory, "/" ) )
						printf( "-: Parent directory\n" );
					for ( index = start; index < end && ( index - start ) < max; index ++ )
					{
						mvcp_dir_get( dir, index, &entry );
						printf( "%d: %s\n", index - start + 1, entry.name );
					}
					while ( ( index ++ % 9 ) != 0 )
						printf( "\n" );
					printf( "\n" );
					if ( start + max < end )
						printf( "space = more files" );
					else if ( end > max )
						printf( "space = return to start of list" );
					if ( start > 0 )
						printf( ", b = previous files" );
					printf( "\n" );
					printf( "0 = abort, t = transport, x = execute command, q = queue maintenance\n\n" );
					refresh = 0;
				}

				client_change_status( demo, 1 );

				pressed = term_read( );
				switch( pressed )
				{
					case -1:
						break;
					case '0':
						terminated = 1;
						break;
					case 'b':
						refresh = start - max >= 0;
						if ( refresh )
							start = start - max;
						break;
					case ' ':
						refresh = start + max < end;
						if ( refresh )
						{
							start = start + max;
						}
						else if ( end > max )
						{
							start = 0;
							refresh = 1;
						}
						break;
					case '-':
						if ( strcmp( demo->current_directory, "/" ) )
						{
							selected = 1;
							( *strrchr( demo->current_directory, '/' ) ) = '\0';
							( *( strrchr( demo->current_directory, '/' ) + 1 ) ) = '\0';
						}
						break;
					case 't':
						client_change_status( demo, 0 );
						term_exit( );
						client_transport( demo );
						term_init( );
						selected = 1;
						break;
					case 'x':
						client_change_status( demo, 0 );
						term_exit( );
						client_execute( demo );
						term_init( );
						selected = 1;
						break;
					case 'q':
						client_change_status( demo, 0 );
						term_exit( );
						client_queue_maintenance( demo, &demo->queues[ demo->selected_unit ] );
						term_init( );
						selected = 1;
						break;
					default:
						if ( pressed >= '1' && pressed <= '9' )
						{
							if ( ( start + pressed - '1' ) < end )
							{
								mvcp_dir_get( dir, start + pressed - '1', &entry );
								selected = 1;
								strcat( demo->current_directory, entry.name );
							}
						}
						break;
				}

				client_change_status( demo, 0 );
			}

			mvcp_dir_close( dir );
		}

		if ( !terminated && demo->current_directory[ strlen( demo->current_directory ) - 1 ] != '/' )
		{
			if ( demo->queues[ demo->selected_unit ].mode == 0 )
			{
				error = mvcp_unit_load( demo->dv, demo->selected_unit, demo->current_directory );
				mvcp_unit_play( demo->dv, demo->selected_unit );
			}
			else
			{
				client_queue_add( demo, &demo->queues[ demo->selected_unit ], demo->current_directory );
				printf( "File %s added to queue.\n", demo->current_directory );
			}
			strcpy( demo->current_directory, demo->last_directory );
			refresh = 0;
		}
		else
		{
			refresh = 1;
			start = 0;
		}
	}

	term_exit( );

	return error;
}

/** Set the in point of the clip on the select unit.
*/

mvcp_error_code client_set_in( client demo )
{
	int position = 0;
	mvcp_status_t status;
	mvcp_notifier notifier = mvcp_parser_get_notifier( demo->parser );
	mvcp_notifier_get( notifier, &status, demo->selected_unit );
	position = status.position;
	return mvcp_unit_set_in( demo->dv, demo->selected_unit, position );
}

/** Set the out point of the clip on the selected unit.
*/

mvcp_error_code client_set_out( client demo )
{
	int position = 0;
	mvcp_status_t status;
	mvcp_notifier notifier = mvcp_parser_get_notifier( demo->parser );
	mvcp_notifier_get( notifier, &status, demo->selected_unit );
	position = status.position;
	return mvcp_unit_set_out( demo->dv, demo->selected_unit, position );
}

/** Clear the in and out points on the selected unit.
*/

mvcp_error_code client_clear_in_out( client demo )
{
	return mvcp_unit_clear_in_out( demo->dv, demo->selected_unit );
}

/** Goto a user specified frame on the selected unit.
*/

mvcp_error_code client_goto( client demo )
{
	int frame = 0;
	printf( "Frame: " );
	if ( get_int( &frame, 0 ) )
		return mvcp_unit_goto( demo->dv, demo->selected_unit, frame );
	return mvcp_ok;
}

/** Manipulate playback on the selected unit.
*/

mvcp_error_code client_transport( client demo )
{
	mvcp_error_code error = mvcp_ok;
	int refresh = 1;
	int terminated = 0;
	mvcp_status_t status;
	mvcp_notifier notifier = mvcp_get_notifier( demo->dv );

	while ( !terminated )
	{
		if ( refresh )
		{
			printf( "  +----+ +------+ +----+ +------+ +---+ +-----+ +------+ +-----+ +---+  \n" );
			printf( "  |1=-5| |2=-2.5| |3=-1| |4=-0.5| |5=1| |6=0.5| |7=1.25| |8=2.5| |9=5|  \n" );
			printf( "  +----+ +------+ +----+ +------+ +---+ +-----+ +------+ +-----+ +---+  \n" );
			printf( "\n" );
			printf( "+----------------------------------------------------------------------+\n" );
			printf( "|              0 = quit, x = eXecute, 'space' = pause                  |\n" );
			printf( "|              g = goto a frame, q = queue maintenance                 |\n" );
			printf( "|     h = step -1, j = end of clip, k = start of clip, l = step 1      |\n" );
			printf( "|        eof handling: p = pause, r = repeat, t = terminate            |\n" );
			printf( "|       i = set in point, o = set out point, c = clear in/out          |\n" );
			printf( "|       u = use point settings, d = don't use point settings           |\n" );
			printf( "+----------------------------------------------------------------------+\n" );
			printf( "\n" );
			term_init( );
			refresh = 0;
		}

		client_change_status( demo, 1 );

		switch( term_read( ) )
		{
			case '0':
				terminated = 1;
				break;
			case -1:
				break;
			case ' ':
				error = mvcp_unit_pause( demo->dv, demo->selected_unit );
				break;
			case '1':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, -5000 );
				break;
			case '2':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, -2500 );
				break;
			case '3':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, -1000 );
				break;
			case '4':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, -500 );
				break;
			case '5':
				error = mvcp_unit_play( demo->dv, demo->selected_unit );
				break;
			case '6':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, 500 );
				break;
			case '7':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, 1250 );
				break;
			case '8':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, 2500 );
				break;
			case '9':
				error = mvcp_unit_play_at_speed( demo->dv, demo->selected_unit, 5000 );
				break;
			case 's':
				error = mvcp_unit_goto( demo->dv, demo->selected_unit, 0 );
				break;
			case 'h':
				error = mvcp_unit_step( demo->dv, demo->selected_unit, -1 );
				break;
			case 'j':
				mvcp_notifier_get( notifier, &status, demo->selected_unit );
				error = mvcp_unit_goto( demo->dv, demo->selected_unit, status.tail_out );
				break;
			case 'k':
				mvcp_notifier_get( notifier, &status, demo->selected_unit );
				error = mvcp_unit_goto( demo->dv, demo->selected_unit, status.in );
				break;
			case 'l':
				error = mvcp_unit_step( demo->dv, demo->selected_unit, 1 );
				break;
			case 'p':
				error = mvcp_unit_set( demo->dv, demo->selected_unit, "eof", "pause" );
				break;
			case 'r':
				error = mvcp_unit_set( demo->dv, demo->selected_unit, "eof", "loop" );
				break;
			case 't':
				error = mvcp_unit_set( demo->dv, demo->selected_unit, "eof", "stop" );
				break;
			case 'i':
				error = client_set_in( demo );
				break;
			case 'o':
				error = client_set_out( demo );
				break;
			case 'g':
				client_change_status( demo, 0 );
				term_exit( );
				error = client_goto( demo );
				refresh = 1;
				break;
			case 'c':
				error = client_clear_in_out( demo );
				break;
			case 'u':
				error = mvcp_unit_set( demo->dv, demo->selected_unit, "points", "use" );
				break;
			case 'd':
				error = mvcp_unit_set( demo->dv, demo->selected_unit, "points", "ignore" );
				break;
			case 'x':
				client_change_status( demo, 0 );
				term_exit( );
				client_execute( demo );
				refresh = 1;
				break;
			case 'q':
				client_change_status( demo, 0 );
				term_exit( );
				client_queue_maintenance( demo, &demo->queues[ demo->selected_unit ] );
				refresh = 1;
				break;
		}

		client_change_status( demo, 0 );
	}

	term_exit( );

	return error;
}

/** Recursive menu execution.
*/

mvcp_error_code client_run_menu( client demo, client_menu menu )
{
	const char *items = "123456789abcdefghijklmnopqrstuvwxyz";
	int refresh_menu = 1;
	int terminated = 0;
	int item_count = 0;
	int item_selected = 0;
	int index = 0;
	char key;

	while( !terminated )
	{

		if ( refresh_menu )
		{
			printf( "%s\n\n", menu->description );
			for ( index = 0; menu->array[ index ].option != NULL; index ++ )
				printf( "%c: %s\n", items[ index ], menu->array[ index ].option );
			printf( "0: Exit\n\n" );
			printf( "Select Option: " );
			refresh_menu = 0;
			item_count = index;
		}

		key = get_keypress( );

		if ( demo->disconnected && key != '0' )
		{
			mvcp_error_code error = mvcp_connect( demo->dv );
			if ( error == mvcp_ok )
				demo->disconnected = 0;
			else
				beep();
		}

		if ( !demo->disconnected || key == '0' )
		{
			item_selected = strchr( items, key ) - items;

			if ( key == '0' )
			{
				printf( "%c\n\n", key );
				terminated = 1;
			}
			else if ( item_selected >= 0 && item_selected < item_count )
			{
				printf( "%c\n\n", key );
				menu->array[ item_selected ].function( demo );
				refresh_menu = 1;
			}
			else
			{
				beep( );
			}
		}
	}

	return mvcp_ok;
}

/** Entry point for main menu.
*/

void client_run( client this )
{
	this->dv = mvcp_init( this->parser );
	if ( mvcp_connect( this->dv ) == mvcp_ok )
	{
		pthread_create( &this->thread, NULL, client_status_thread, this );
		client_run_menu( this, &connected_menu );
		this->terminated = 1;
		pthread_join( this->thread, NULL );
		this->terminated = 0;
	}
	else
	{
		printf( "Unable to connect." );
		wait_for_any_key( "" );
	}

	mvcp_close( this->dv );

	printf( "Demo Exit.\n" );
}

/** Close the demo structure.
*/

void client_close( client demo )
{
	free( demo );
}

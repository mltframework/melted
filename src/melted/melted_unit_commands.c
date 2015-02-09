/*
 * melted_unit_commands.c
 * Copyright (C) 2002-2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "melted_unit.h"
#include "melted_commands.h"
#include "melted_log.h"


void get_fullname( command_argument cmd_arg, char *fullname, size_t len, char *filename )
{
	char *service = strchr( filename, ':' );

	if ( service != NULL )
	{
		service = filename;
		filename = strchr( service, ':' );
		*filename ++ = '\0';

		if ( strlen( cmd_arg->root_dir ) && filename[0] == '/' )
			filename++;

		snprintf( fullname, len, "%s:%s%s", service, cmd_arg->root_dir, filename );
	}
	else
	{
		if ( strlen( cmd_arg->root_dir ) && filename[0] == '/' )
			filename++;

		snprintf( fullname, len, "%s%s", cmd_arg->root_dir, filename );
	}
}

int melted_load( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];
	int flush = 1;

	if ( filename[0] == '!' )
	{
		flush = 0;
		filename ++;
	}
	get_fullname( cmd_arg, fullname, sizeof(fullname), filename );

	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		int32_t in = -1, out = -1;
		if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) == 5 )
		{
			in = atol( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 3 ) );
			out = atol( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
		}
		if ( melted_unit_load( unit, fullname, in, out, flush ) != mvcp_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int melted_list( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit( cmd_arg->unit );

	if ( unit != NULL )
	{
		melted_unit_report_list( unit, cmd_arg->response );
		return RESPONSE_SUCCESS;
	}

	return RESPONSE_INVALID_UNIT;
}

static int parse_clip( command_argument cmd_arg, int arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	int clip = melted_unit_get_current_clip( unit );
	
	if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) > arg )
	{
		char *token = mvcp_tokeniser_get_string( cmd_arg->tokeniser, arg );
		if ( token[ 0 ] == '+' )
			clip += atoi( token + 1 );
		else if ( token[ 0 ] == '-' )
			clip -= atoi( token + 1 );
		else
			clip = atoi( token );
	}
	
	return clip;
}

int melted_insert( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];

	get_fullname( cmd_arg, fullname, sizeof(fullname), filename );

	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		long in = -1, out = -1;
		int index = parse_clip( cmd_arg, 3 );
		
		if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) == 6 )
		{
			in = atoi( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
			out = atoi( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 5 ) );
		}
		
		switch( melted_unit_insert( unit, fullname, index, in, out ) )
		{
			case mvcp_ok:
				return RESPONSE_SUCCESS;
			default:
				return RESPONSE_BAD_FILE;
		}
	}
	return RESPONSE_SUCCESS;
}

int melted_remove( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		int index = parse_clip( cmd_arg, 2 );
			
		if ( melted_unit_remove( unit, index ) != mvcp_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int melted_clean( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		if ( melted_unit_clean( unit ) != mvcp_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int melted_wipe( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		if ( melted_unit_wipe( unit ) != mvcp_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int melted_clear( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		if ( melted_unit_clear( unit ) != mvcp_ok )
			return RESPONSE_BAD_FILE;
	}
	return RESPONSE_SUCCESS;
}

int melted_move( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if ( unit != NULL )
	{
		if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) > 2 )
		{
			int src = parse_clip( cmd_arg, 2 );
			int dest = parse_clip( cmd_arg, 3 );
			
			if ( melted_unit_move( unit, src, dest ) != mvcp_ok )
				return RESPONSE_BAD_FILE;
		}
		else
		{
			return RESPONSE_MISSING_ARG;
		}
	}
	else
	{
		return RESPONSE_INVALID_UNIT;
	}

	return RESPONSE_SUCCESS;
}

int melted_append( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	char *filename = (char*) cmd_arg->argument;
	char fullname[1024];

	get_fullname( cmd_arg, fullname, sizeof(fullname), filename );

	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		int32_t in = -1, out = -1;
		if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) == 5 )
		{
			in = atol( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 3 ) );
			out = atol( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 4 ) );
		}
		switch ( melted_unit_append( unit, fullname, in, out ) )
		{
			case mvcp_ok:
				return RESPONSE_SUCCESS;
			default:
				return RESPONSE_BAD_FILE;
		}
	}
	return RESPONSE_SUCCESS;
}

int melted_push( command_argument cmd_arg, mlt_service service )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( !unit )
		return RESPONSE_INVALID_UNIT;
	if ( service != NULL )
		if ( melted_unit_append_service( unit, service ) == mvcp_ok )
			return RESPONSE_SUCCESS;
	return RESPONSE_BAD_FILE;
}

int melted_receive( command_argument cmd_arg, char *doc )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else
	{
		// Get the consumer's profile
		mlt_consumer consumer = mlt_properties_get_data( unit->properties, "consumer", NULL );
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
		mlt_producer producer = mlt_factory_producer( profile, "xml-string", doc );
		if ( producer != NULL )
		{
			if ( melted_unit_append_service( unit, MLT_PRODUCER_SERVICE( producer ) ) == mvcp_ok )
			{
				mlt_producer_close( producer );
				return RESPONSE_SUCCESS;
			}
			mlt_producer_close( producer );
		}
	}
	return RESPONSE_BAD_FILE;
}

int melted_play( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if ( unit == NULL )
	{
		return RESPONSE_INVALID_UNIT;
	}
	else
	{
		int speed = 1000;
		if ( mvcp_tokeniser_count( cmd_arg->tokeniser ) == 3 )
			speed = atoi( mvcp_tokeniser_get_string( cmd_arg->tokeniser, 2 ) );
		melted_unit_play( unit, speed );
	}

	return RESPONSE_SUCCESS;
}

int melted_stop( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else 
		melted_unit_terminate( unit );
	return RESPONSE_SUCCESS;
}

int melted_pause( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else 
		melted_unit_play( unit, 0 );
	return RESPONSE_SUCCESS;
}

int melted_rewind( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else if ( melted_unit_has_terminated( unit ) )
		melted_unit_change_position( unit, 0, 0 );
	else
		melted_unit_play( unit, -2000 );
	return RESPONSE_SUCCESS;
}

int melted_step( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
	{
		melted_unit_play( unit, 0 );
		melted_unit_step( unit, *(int*) cmd_arg->argument );
	}
	return RESPONSE_SUCCESS;
}

int melted_goto( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	
	if (unit == NULL || melted_unit_is_offline(unit))
		return RESPONSE_INVALID_UNIT;
	else
	{
		int clip = parse_clip( cmd_arg, 3 );
		melted_unit_change_position( unit, clip, *(int*) cmd_arg->argument );
	}
	return RESPONSE_SUCCESS;
}

int melted_ff( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else if ( melted_unit_has_terminated( unit ) )
		melted_unit_change_position( unit, 0, 0 );
	else
		melted_unit_play( unit, 2000 );
	return RESPONSE_SUCCESS;
}

int melted_set_in_point( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	int clip = parse_clip( cmd_arg, 3 );

	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else
	{
		int position = *(int *) cmd_arg->argument;

		switch( melted_unit_set_clip_in( unit, clip, position ) )
		{
			case -1:
				return RESPONSE_BAD_FILE;
			case -2:
				return RESPONSE_OUT_OF_RANGE;
		}
	}
	return RESPONSE_SUCCESS;
}

int melted_set_out_point( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	int clip = parse_clip( cmd_arg, 3 );
	
	if ( unit == NULL )
		return RESPONSE_INVALID_UNIT;
	else
	{
		int position = *(int *) cmd_arg->argument;

		switch( melted_unit_set_clip_out( unit, clip, position ) )
		{
			case -1:
				return RESPONSE_BAD_FILE;
			case -2:
				return RESPONSE_OUT_OF_RANGE;
		}
	}

	return RESPONSE_SUCCESS;
}

int melted_get_unit_status( command_argument cmd_arg )
{
	mvcp_status_t status;
	int error = melted_unit_get_status( melted_get_unit( cmd_arg->unit ), &status );

	if ( error )
		return RESPONSE_INVALID_UNIT;
	else
	{
		char text[ 10240 ];
		mvcp_response_printf( cmd_arg->response, sizeof( text ), mvcp_status_serialise( &status, text, sizeof( text ) ) );
		return RESPONSE_SUCCESS_1;
	}
	return 0;
}


int melted_set_unit_property( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	char *name_value = (char*) cmd_arg->argument;
	if (unit == NULL)
		return RESPONSE_INVALID_UNIT;
	else
		melted_unit_set( unit, name_value );
	return RESPONSE_SUCCESS;
}

int melted_get_unit_property( command_argument cmd_arg )
{
	melted_unit unit = melted_get_unit(cmd_arg->unit);
	char *name = (char*) cmd_arg->argument;
	if (unit == NULL)
	{
		return RESPONSE_INVALID_UNIT;
	}
	else
	{
		char *value = melted_unit_get( unit, name );
		if ( value != NULL )
			mvcp_response_printf( cmd_arg->response, 1024, "%s\n", value );
	}
	return RESPONSE_SUCCESS;
}


int melted_transfer( command_argument cmd_arg )
{
	melted_unit src_unit = melted_get_unit(cmd_arg->unit);
	int dest_unit_id = -1;
	char *string = (char*) cmd_arg->argument;
	if ( string != NULL && ( string[ 0 ] == 'U' || string[ 0 ] == 'u' ) && strlen( string ) > 1 )
		dest_unit_id = atoi( string + 1 );
	
	if ( src_unit != NULL && dest_unit_id != -1 )
	{
		melted_unit dest_unit = melted_get_unit( dest_unit_id );
		if ( dest_unit != NULL && !melted_unit_is_offline(dest_unit) && dest_unit != src_unit )
		{
			melted_unit_transfer( dest_unit, src_unit );
			return RESPONSE_SUCCESS;
		}
	}
	return RESPONSE_INVALID_UNIT;
}

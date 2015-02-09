/**
 * MltMelted.cpp - MLT Melted Wrapper
 * Copyright (C) 2004-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
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

#include "MltMelted.h"
#include "MltService.h"
#include "MltResponse.h"
using namespace Mlt;

#include <time.h>

static mvcp_response mlt_melted_execute( void *arg, char *command )
{
	Melted *melted = ( Melted * )arg;
	if ( melted != NULL )
	{
		Response *response = melted->execute( command );
		mvcp_response real = mvcp_response_clone( response->get_response( ) );
		delete response;
		return real;
	}
	else
	{
		mvcp_response response = mvcp_response_init( );
		mvcp_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

static mvcp_response mlt_melted_received( void *arg, char *command, char *doc )
{
	Melted *melted = ( Melted * )arg;
	if ( melted != NULL )
	{
		Response *response = melted->received( command, doc );
		if ( response != NULL )
		{
			mvcp_response real = mvcp_response_clone( response->get_response( ) );
			delete response;
			return real;
		}
		return NULL;
	}
	else
	{
		mvcp_response response = mvcp_response_init( );
		mvcp_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

static mvcp_response mlt_melted_push( void *arg, char *command, mlt_service service )
{
	Melted *melted = ( Melted * )arg;
	if ( melted != NULL )
	{
		Service input( service );
		Response *response = melted->push( command, &input );
		mvcp_response real = mvcp_response_clone( response->get_response( ) );
		delete response;
		return real;
	}
	else
	{
		mvcp_response response = mvcp_response_init( );
		mvcp_response_set_error( response, 500, "Invalid server" );
		return response;
	}
}

Melted::Melted( char *name, int port, char *config ) :
	Properties( false )
{
	server = melted_server_init( name );
	melted_server_set_port( server, port );
	melted_server_set_config( server, config );
}

Melted::~Melted( )
{
	melted_server_close( server );
}

mlt_properties Melted::get_properties( )
{
	return &server->parent;
}

bool Melted::start( )
{
	if ( melted_server_execute( server ) == 0 )
	{
		_real = server->parser->real;
		_execute = server->parser->execute;
		_received = server->parser->received;
		_push = server->parser->push;
		server->parser->real = this;
		server->parser->execute = mlt_melted_execute;
		server->parser->received = mlt_melted_received;
		server->parser->push = mlt_melted_push;
	}
	return server->shutdown == 0;
}

bool Melted::is_running( )
{
	return server->shutdown == 0;
}

Response *Melted::execute( char *command )
{
	return new Response( _execute( _real, command ) );
}

Response *Melted::received( char *command, char *doc )
{
	return new Response( _received( _real, command, doc ) );
}

Response *Melted::push( char *command, Service *service )
{
	return new Response( _push( _real, command, service->get_service( ) ) );
}

void Melted::wait_for_shutdown( )
{
	struct timespec tm = { 1, 0 };
	while ( !server->shutdown )
		nanosleep( &tm, NULL );
}

void Melted::log_level( int threshold )
{
	melted_log_init( log_stderr, threshold );
}

Properties *Melted::unit( int index )
{
	mlt_properties properties = melted_server_fetch_unit( server, index );
	return properties != NULL ? new Properties( properties ) : NULL;
}

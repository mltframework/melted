/**
 * MltResponse.cpp - MLT MVCP Wrapper
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

#include <string.h>
#include "MltResponse.h"
using namespace Mlt;

Response::Response( mvcp_response response ) :
	_response( response )
{
}

Response::Response( int error, const char *message ) :
	_response( NULL )
{
	_response = mvcp_response_init( );
	if ( _response != NULL )
		mvcp_response_set_error( _response, error, message );
}

Response::~Response( )
{
	mvcp_response_close( _response );
}

mvcp_response Response::get_response( )
{
	return _response;
}

int Response::error_code( )
{
	return mvcp_response_get_error_code( get_response( ) );
}

const char *Response::error_string( )
{
	return mvcp_response_get_error_string( get_response( ) );
}

char *Response::get( int index )
{
	return mvcp_response_get_line( get_response( ), index );
}

int Response::count( )
{
	return mvcp_response_count( get_response( ) );
}

int Response::write( const char *data )
{
	return mvcp_response_write( get_response( ), data, strlen( data ) );
}


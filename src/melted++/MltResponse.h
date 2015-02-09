/**
 * MltResponse.h - MLT MVCP Wrapper
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

#ifndef _MLTPP_RESPONSE_H_
#define _MLTPP_RESPONSE_H_

#include <mvcp/mvcp_response.h>

namespace Mlt
{
	class Response
	{
		private:
			mvcp_response _response;
		public:
			Response( mvcp_response response );
			Response( int error, const char *message );
			~Response( );
			mvcp_response get_response( );
			int error_code( );
			const char *error_string( );
			char *get( int );
			int count( );
			int write( const char *data );
	};
}

#endif

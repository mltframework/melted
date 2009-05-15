/**
 * melted.i - Swig Bindings for melted++
 * Copyright (C) 2004-2009 Charles Yates
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

%module melted
%include "carrays.i"
%array_class(unsigned char, unsignedCharArray);

%{
#include <Mlt.h>
#include <MltMelted.h>
#include <MltResponse.h>
%}

/** These methods return objects which should be gc'd.
 */

namespace Mlt {
%newobject Melted::execute( char * );
%newobject Melted::received( char *, char * );
%newobject Melted::push( char *, Service & );
%newobject Melted::unit( int );
}

/** Classes to wrap.
 */

%include <MltMelted.h>
%include <MltResponse.h>

#if defined(SWIGRUBY)

%{

static void ruby_listener( mlt_properties owner, void *object );

class RubyListener
{
	private:
		VALUE callback;
		Mlt::Event *event;

	public:
		RubyListener( Mlt::Properties &properties, char *id, VALUE callback ) : 
			callback( callback ) 
		{
			event = properties.listen( id, this, ( mlt_listener )ruby_listener );
		}

		~RubyListener( )
		{
			delete event;
		}

    	void mark( ) 
		{ 
			((void (*)(VALUE))(rb_gc_mark))( callback ); 
		}

    	void doit( ) 
		{
        	ID method = rb_intern( "call" );
        	rb_funcall( callback, method, 0 );
    	}
};

static void ruby_listener( mlt_properties owner, void *object )
{
	RubyListener *o = static_cast< RubyListener * >( object );
	o->doit( );
}

void markRubyListener( void* p ) 
{
    RubyListener *o = static_cast<RubyListener*>( p );
    o->mark( );
}

%}

// Ruby wrapper
%rename( Listener )  RubyListener;
%markfunc RubyListener "markRubyListener";

class RubyListener 
{
	public:
		RubyListener( Mlt::Properties &properties, char *id, VALUE callback );
};

#endif

/**
 * config.h - Convenience header file for all melted++ objects
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

#ifndef MELTEDPP_CONFIG_H_
#define MELTEDPP_CONFIG_H_

#ifdef WIN32
    #ifdef MELTEDPP_EXPORTS
        #define MELTEDPP_DECLSPEC __declspec( dllexport )
    #else
        #define MELTEDPP_DECLSPEC __declspec( dllimport )
    #endif
#else
	#define MELTEDPP_DECLSPEC
#endif

#endif

/*
 * melted_local.h -- Local Melted Parser
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

#ifndef _MELTED_LOCAL_H_
#define _MELTED_LOCAL_H_

/* Application header files */
#include <mvcp/mvcp_parser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Local parser API.
*/

extern mvcp_parser melted_parser_init_local( );

#ifdef __cplusplus
}
#endif

#endif

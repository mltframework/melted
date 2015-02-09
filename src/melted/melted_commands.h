/*
 * melted_commands.h - global commands
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


#ifndef _MELTED_COMMANDS_H_
#define _MELTED_COMMANDS_H_

#include <mvcp/mvcp_status.h>
#include "melted_unit.h"
#include "melted_connection.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern melted_unit melted_get_unit( int );
extern void melted_delete_unit( int );
extern void melted_delete_all_units( void );
//extern void raw1394_start_service_threads( void );
//extern void raw1394_stop_service_threads( void );

extern response_codes melted_add_unit( command_argument );
extern response_codes melted_list_nodes( command_argument );
extern response_codes melted_list_units( command_argument );
extern response_codes melted_list_clips( command_argument );
extern response_codes melted_set_global_property( command_argument );
extern response_codes melted_get_global_property( command_argument );

#ifdef __cplusplus
}
#endif

#endif

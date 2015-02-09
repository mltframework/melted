/*
 * melted_unit.h -- Playout Unit Header
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

#ifndef _MELTED_UNIT_H_
#define _MELTED_UNIT_H_

#include <pthread.h>

#include <framework/mlt_properties.h>
#include <mvcp/mvcp.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	mlt_properties properties;
} 
melted_unit_t, *melted_unit;

extern melted_unit         melted_unit_init( int index, char *arg );
extern void 				melted_unit_report_list( melted_unit unit, mvcp_response response );
extern void                 melted_unit_allow_stdin( melted_unit unit, int flag );
extern mvcp_error_code   melted_unit_load( melted_unit unit, char *clip, int32_t in, int32_t out, int flush );
extern mvcp_error_code 	melted_unit_insert( melted_unit unit, char *clip, int index, int32_t in, int32_t out );
extern mvcp_error_code   melted_unit_append( melted_unit unit, char *clip, int32_t in, int32_t out );
extern mvcp_error_code   melted_unit_append_service( melted_unit unit, mlt_service service );
extern mvcp_error_code 	melted_unit_remove( melted_unit unit, int index );
extern mvcp_error_code 	melted_unit_clean( melted_unit unit );
extern mvcp_error_code 	melted_unit_wipe( melted_unit unit );
extern mvcp_error_code 	melted_unit_clear( melted_unit unit );
extern mvcp_error_code 	melted_unit_move( melted_unit unit, int src, int dest );
extern int                  melted_unit_transfer( melted_unit dest_unit, melted_unit src_unit );
extern void                 melted_unit_play( melted_unit_t *unit, int speed );
extern void                 melted_unit_terminate( melted_unit );
extern int                  melted_unit_has_terminated( melted_unit );
extern int                  melted_unit_get_nodeid( melted_unit unit );
extern int                  melted_unit_get_channel( melted_unit unit );
extern int                  melted_unit_is_offline( melted_unit unit );
extern void                 melted_unit_set_notifier( melted_unit, mvcp_notifier, char * );
extern int                  melted_unit_get_status( melted_unit, mvcp_status );
extern void                 melted_unit_change_position( melted_unit, int, int32_t position );
extern void                 melted_unit_change_speed( melted_unit unit, int speed );
extern int                  melted_unit_set_clip_in( melted_unit unit, int index, int32_t position );
extern int                  melted_unit_set_clip_out( melted_unit unit, int index, int32_t position );
extern void                 melted_unit_step( melted_unit unit, int32_t offset );
extern void                 melted_unit_close( melted_unit unit );
extern void                 melted_unit_suspend( melted_unit );
extern void                 melted_unit_restore( melted_unit );
extern int					melted_unit_set( melted_unit, char *name_value );
extern char *				melted_unit_get( melted_unit, char *name );
extern int					melted_unit_get_current_clip( melted_unit );


#ifdef __cplusplus
}
#endif

#endif

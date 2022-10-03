/**
 * @file hula_lib.h
 *
 * Implements lua wrappers for using hsm_machine in lua
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_LUA_LIB_H__
#define __HSM_LUA_LIB_H__

/**
 * default name of the type within lua
 */
#define HULA_LIB        "hsm_statechart"

/**
 * default name of the HULA_LIB's metatable
 */
#define HULA_METATABLE  "hsm.hula"

/**
 * name inside the metatable of the user event matching function
 */
#define HULA_EVENT_TEST "is_user_event"   


/*---------------------------------------------------------------------------*/
/**
 * @page hula_event_matching Lua Event Matching
 *
 * For a chart described in lua, ex:
 * @code
 * chart = { 
 *    state1= {
 *       entry= function....
 *       exit = function.....
 *       user_defined_event = function....
 *    }
 * }
 * @endcode
 *
 * The lua function signature for entry and exit is:
 * @code
 *   entry= function( context, event name, event parameters, ..., event_table )
 * @endcode
 * 
 * The lua function signature for user defined events removes the event name:
 *
 * @code
 *   event= function( context, event parameters, ..., event_table )
 * @endcode
 *
 * Internally, to pass the "event name" and "event parameters" as a unit, 
 * Hula conslidates those in a single "event table" of the format:
 *
 * @code
 *   { event name, event parameter 1, ..., event parameter n, keys=optional }
 * @endcode
 *
 * For machines written and executed in Lua, 
 * Hula itself handles the packing and unpacking of the event table.
 *
 * If your state charts are written in Lua, but executed in C, 
 * you will have to create the event table yourself. Please see 
 * the samples and the google code website for more details.
 */
/*---------------------------------------------------------------------------*/

/**
 * event table: event name
 */
#define HULA_EVENT_NAME    1
#define HULA_EVENT_PAYLOAD 2

#endif // #ifndef __HSM_LUA_LIB_H__

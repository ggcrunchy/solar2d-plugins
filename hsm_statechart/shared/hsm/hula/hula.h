/**
 * @file hula.h
 *
 * Statemachine builder extensions for lua.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_LUA_H__
#define __HSM_LUA_H__

// i dont want to make assumptions about the relative include paths for users in the headers
// to include hula though you will need these files
//#include <hsm/hsm_machine.h>
//#include <lua.h>

typedef const char *  hula_error;

/**
 * Control whether the event being processed matches an event defined in lua.
 *
 * @param L Active lua state.
 * @param spec Event name specified in the lua defined statechart.
 * @param event Event being processed by the machine.
 * @return HSM_TRUE if hsm_event matches the event spec.
 *
 * @see HulaMatchEvent, HulaRegister, HsmSignalEvent
 */
typedef hsm_bool (*hula_callback_is_event)( lua_State*L, const char * spec, hsm_event event );

/**
 * Match an event specification to a triggered event.
 * The matching takes into account hierarchical events.
 * This behavior is the default behavior of lua statecharts,
 * user code can customize this behavior via hula_callback_is_event.
 * 
 * @param spec Event name specified in the lua defined statechart.
 * @param test Event name signaled by lua.
 * @return HSM_TRUE if they match
 * 
 * @see hula_callback_is_event
 * 
 * event.item       <- specified event(s) we want to handle
 * event.item.click <- a triggered event we match
 * event.item       <- another match
 * event.mouse.drag <- no match
 * event.items      <- no match
 * event            <- no match
 */
hsm_bool HulaMatchEvent( const char * spec, const char *test );

/**
 * Create an hsm-statechart state from a lua state description.
 * ( Uses hsm-builder to accomplish the task )
 *
 * @param L Lua state
 * @param idx Index on the stack of the table containing the state description
 * @param pId When return code is 0, filled with the built state(tree) id
 * @return error code
 *
 * @code
 *    top_state_example= { top_state_example_name= {...} }
 * @endcode
 *
 *
 * @see hsmBegin, HulaBuildNamedState
 */
hula_error HulaBuildState( lua_State*L, int idx, int *pId );

/**
 * Create an hsm-statechart state from a lua state description.
 * ( Uses hsm-builder to accomplish the task )
 *
 * @code
 *    no_top_level_example = { init = 's1', s1 = {...}, s2= {...} }
 * @endcode
 *
 * @param L Lua state
 * @param idx Index on the stack of the table containing the state description
 * @param name Name of the state.
 * @param namelen if non-zero, the string is copied for safekeeping
 * @param pId When return code is 0, filled with the built state(tree) id
 * @return error code
 *
 * @see hsmBegin, HulaBuildState
 */
hula_error HulaBuildNamedState( lua_State*L, int idx, const char * name, int namelen, int *pId );

/**
 * Create the "hsm_statechart" type for using hierarchical statemachines in lua.
 * @param L Lua state to register ( luaL_register ) the hula interface
 * @param is_user_event Optional callback to control how c-defined event structs match up with lua-declared event names.
 */
void HulaRegister( lua_State* L, hula_callback_is_event is_user_event );

/**
 * Same as HulaRegister but allows user code to rename "hsm_statechart" to something else.
 * @see HulaRegister
 */
void HulaNamedRegister( lua_State* L, const char * name, hula_callback_is_event is_user_event );

#endif // #ifndef __HSM_LUA_H__

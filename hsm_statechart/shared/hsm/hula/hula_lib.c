/**
 * @file hula_lib.c
 *
 * Implements lua wrappers for using hsm_machine in lua
 *
 * A note on naming:
 *    hula_*() are functions called from lua
 *    the rest, with the exception of luaopen_hsm_statechart, 
 *    are called from the c-side.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */

#include <hsm/hsm_machine.h>
// STEVE CHANGE
#include "CoronaLua.h"
/*
#include <lua.h>
#include <lauxlib.h>
*/
// /STEVE CHANGE

#include "hula.h" // for build during new.
#include "hula_lib.h"
#include "hula_types.h"
#include <hsm/builder/hsm_builder.h> // resolve needed for looking up top state names during new

#include <assert.h>
#include <string.h>

#define HULA_REC_IDX 1
#define HULA_EVENT_IDX 2
#define HULA_PAYLOAD_IDX 3
#define HULA_CHART_IDX 2

//---------------------------------------------------------------------------
/**
 * @internal 
 * @return value hula hsm object
 */
static hula_machine_t* check_hula( lua_State* L, int idx )
{
  void * ud= luaL_checkudata( L, idx, HULA_METATABLE );
  return (hula_machine_t*) ud;
}

//---------------------------------------------------------------------------
/**
 * @internal 
 * create a new table and fill it with a list of items from the stack
 * expected: event name, event parameters...
 * @return index of new table
 */
static int pack_hula( lua_State * L, int copyfrom, int copyto )
{
  int count=0; 
  int copy=copyfrom;
  lua_newtable( L );
  while( copy<=copyto ) {
    lua_pushvalue( L, copy++ );
    lua_rawseti( L, -2, ++count );
  }
  return count;
}

//---------------------------------------------------------------------------
/**
 * simple validation of the input
 * @internal
 */
static void verify_chart( lua_State * L ) 
{
  const char * error= "expected a chart containing one key:value pair; the top state and its definition.";
  int table= lua_gettop(L);
  int tablelen= lua_objlen(L, table);
  if (tablelen) {
    luaL_error(L, error);
  }
  lua_pushnil(L);
  lua_next(L,table);
  if (!lua_istable(L,-1)) {
    luaL_error(L, error);
  }
  lua_pop(L,1);
  if (lua_next(L,table)) {
    luaL_error(L, error);
  }
  lua_settop(L,table);
}

//---------------------------------------------------------------------------
/**
 * Create a new hsm machine.
 *
 * machine= hsm_statechart.new{ chart, init= 'name of first state', context= data }
 * or, machine= hsm_statechart.new( chart )
 *
 * @see HsmStart, HsmMachineWithContext 
 */
static int hula_new(lua_State* L)
{
  int param_table= 1;
  int id=0;
  hula_error err;
  hsm_state init_state;

  //
  // verify the input table
  //
  int param_len= lua_objlen(L, param_table);
  if (!lua_istable(L,param_table)) {
    luaL_error( L, "hsm_statechart: expected a table for new()" );
  }

  //
  // get & build the chart
  //

  // if they passed a table of parameters, then the first element is the chart
  if (param_len) {
    lua_rawgeti( L, param_table, 1 );
  }
  verify_chart( L );  // doesn't return if it detects an erro
  err= HulaBuildState( L, HULA_CHART_IDX, &id );
  if (err) {
    luaL_error( L, err ); // doesnt return
  }      
  if (param_len) {
    lua_pop( L,1 ); // pop the chart ( if we fetched it from the param_table )
  }

  //
  // get init(ial) state
  //
  lua_getfield( L, param_table, "init" );

  // if they didn't specify one, that's cool: use the topstate as the inital sate
  if (!param_len || !lua_isstring( L, -1)) {
    init_state= hsmResolveId( id );
  }
  // otherwise lookup the state
  else {
    const char * statename= lua_tostring( L, -1 );
    init_state=hsmResolve( statename );
  }
  lua_pop( L, 1 ); // pop init

  if (!init_state) {
    luaL_error(L, "resolve error during init");
  }
  else {
    hula_machine_t* hula;

    // get context:
    int ctx= LUA_REFNIL;
    if (param_len) { 
      lua_getfield( L, param_table, "context" );
      ctx= luaL_ref(L, LUA_REGISTRYINDEX);
    }

    //
    // create the machine
    //
    hula= (hula_machine_t*) lua_newuserdata( L, sizeof(hula_machine_t) );
    HSM_ASSERT( hula );
    if (hula) {
      // setup the machine:
      memset( hula, 0, sizeof(hula_machine_t) );
      hula->topstate= id;
      hula->ctx.L= L;
      hula->ctx.lua_ref= ctx;          
      luaL_getmetatable( L, HULA_METATABLE );
      lua_setmetatable( L, -2 );

      //
      // start the machine
      //
      if (HsmMachineWithContext( &hula->hsm, &hula->ctx.core )) {
        hula->hsm.core.flags|= HSM_FLAGS_HULA;
        
        // create the event table with "init" as the event name
        lua_newtable( L );
        lua_pushstring( L,"init" );
        lua_rawseti( L, -2, 1 );
        
        // start the machine
        if (!HsmStart( (hsm_machine) &hula->hsm, init_state )) {
          luaL_error( L, "couldnt start machine");
        }            
        lua_pop( L, 1 ); // remove the event table
      }            
    }          
  }        
  return 1; 
}

/**
 * Send an event to the statemachine.
 * boolean= hsm.signal( event, payload )
 * @see HsmSignalEvent
 */
static int hula_signal(lua_State* L)
{
  hsm_bool okay= HSM_FALSE;
  hula_machine_t* hula= check_hula(L, HULA_REC_IDX);
  if (hula) {
    const char * event_name= luaL_checkstring(L, HULA_EVENT_IDX);
    if (event_name) {
      pack_hula( L, HULA_EVENT_IDX, lua_gettop(L) );
      okay= HsmSignalEvent( (hsm_machine) &hula->hsm, 0 );
    }
  }    
  lua_pushboolean( L, okay );
  return 1;
}

/**
 * Return a complete listing of the machine's current states.
 * table= hsm.get_states()
 * @see HsmIsInState
 */
static int hula_states(lua_State *L)
{
  int count=0;
  hula_machine_t* hula= check_hula(L,HULA_REC_IDX);
  if (hula) {
    hsm_state state;
    for ( state= hula->hsm.core.current; state; state=state->parent, ++count ) {
      lua_pushstring( L, state->name );
    }
  }    
  return count;
}

/**
 * Determine whether the state machine has finished.
 * boolean= hsm.is_running() 
 * 
 * @return true if the machine is not in a final or error state
 * @see HsmIsRunning
 */
static int hula_is_running(lua_State *L)
{
  hsm_bool okay= HSM_FALSE;
  hula_machine_t* hula= check_hula(L,HULA_REC_IDX);
  if (hula) {
    okay= HsmIsRunning( (hsm_machine) &hula->hsm );
  }    
  lua_pushboolean( L, okay );
  return 1;
}

/**
 * Print the hsm in a handy debug form.
 * string= hsm.__tostring()
 * 
 * @return "hsm(topState:leafState)
 * @see hula_states
 */
static int hula_tostring(lua_State *L)
{
  int ret=0;
  hula_machine_t* hula= check_hula(L,HULA_REC_IDX);
  if (hula) {
    // hrmm... breaking into the lower level api....
    hsm_state topstate= hsmResolveId( hula->topstate );
    lua_pushfstring(L, "HsmStatechart: %s:%s", 
            topstate ? topstate->name : "nil",
            hula->hsm.core.current ? hula->hsm.core.current->name : "nil" );
    ret=1;                    
  }    
  return ret;
}

//---------------------------------------------------------------------------
// Registration
//---------------------------------------------------------------------------
static void _HulaNamedRegister( lua_State* L, const char * type, hula_callback_is_event is_event )
{
  static luaL_Reg hula_class_fun[]= {
    { "new", hula_new },
    { 0 }
  };

  static luaL_Reg hula_member_fun[]= {
    { "__tostring", hula_tostring },
    { "signal", hula_signal },
    { "states", hula_states },
    { "is_running", hula_is_running },
    { 0 }
  };

  // an object with metatable, when it sees it doesn't have t[k] uses __index to see what to do
  // __index could have a function, value, or table; 
  // tables say: here's a table of names and functions for you. 
  // all meta methods start with __, http://lua-users.org/wiki/MetatableEvents
  luaL_newmetatable( L, HULA_METATABLE );  // registry.HULA_METATABLE= {}
  
  lua_pushstring( L, HULA_EVENT_TEST );
  lua_pushlightuserdata( L, is_event );
  lua_settable( L, -3 );

  lua_pushstring( L, "__index" ); 
  lua_pushvalue( L, -2 );  // copy the metatable to the top of the stack
  lua_settable( L, -3 );  // metatable.__index== metatable;
  
  luaL_register( L, NULL, hula_member_fun );  // use the table @ top (metatable), and assign the passed named c-functions
  luaL_register( L, type, hula_class_fun );   // create a table in the registry @ 'type' with the passed named c-functions
  lua_pop(L,2);
}

//---------------------------------------------------------------------------
void HulaNamedRegister( lua_State* L, const char * type, hula_callback_is_event is_event )
{
  _HulaNamedRegister( L, type, is_event );
}

//---------------------------------------------------------------------------
void HulaRegister( lua_State* L, hula_callback_is_event is_event  )
{
  _HulaNamedRegister( L, HULA_LIB, is_event );
}

//---------------------------------------------------------------------------
// for luarocks
CORONA_EXPORT int luaopen_hsmstatechart( lua_State* L ) // <- STEVE CHANGE
{
  // this leaks 
  // it might be interesting to determine:
  // can you put a user data in the registry and get a gc when the lua state is destroyed?
  if (!hsmStartup()) {
    luaL_error(L, "luaopen_plugin_hsmstatechart: unknown error" ); // <- STEVE CHANGE
  }
  _HulaNamedRegister(L,HULA_LIB, NULL);
  return 0;
}

/**
 * @file hsm_state.h
 * 
 * Macros and typedefs used for declaring states.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_STATE_H__
#define __HSM_STATE_H__

#include "hsm_forwards.h"

/**
 * A state's event handler callback.
 * A function of this signature is required for every state declared via #HSM_STATE.
 * 
 * @param status Current state of the machine.
 * @return NULL, a state to transition to, or a predefined state token.
 *
 * @note You should return NULL from an event handler by default to indicate the event was not handled.
 * To transition to a new state, return the name of the new state as previously declared via a #HSM_STATE macro.
 * To indicate an event has been handled, but no transition is required: you should return HsmStateHandled().
 * Returning HsmStateError() indicates a critical error has occured and the statemachine can no longer function.
 * Returning HsmStateFinal() indicates the statemachine has terminated and event callbacks are no longer desired.
 */
typedef hsm_state(*hsm_callback_process_event)( hsm_status status );

/**
 * A state's enter callback.
 * A function of this signature is required for every state declared via #HSM_STATE_ENTER.
 *
 * @param status Current state of the machine. 
 * The context member of the status is either the parent state's #hsm_callback_enter, 
 * or if no parent state: the context passed to HsmMachineWithContext().
 *
 * @param evt The #hsm_event that needs handling.
 * 
 * @return Optionally: a new #hsm_context_rec. The context will be passed to all 
 *         subsequent calls to the state's #hsm_callback_process_event and the state's #hsm_callback_exit.
 *
 * @note The return value must not be null if the status.ctx was not null. 
 *       Returning null when status.ctx is valid indicates a critical error has occured. 
 *       ex. enter could not allocate context memory
 * 
 */
typedef hsm_context (*hsm_callback_enter)( hsm_status status );

/**
 * A state's action callback.
 * Also used for exit actions.
 *
 * @param status Current state of the machine. 
 */
typedef void(*hsm_callback_action)( hsm_status status );

/**
 * Guard callback.
 *
 * @param status Current state of the machine. 
 * @return Return #HSM_TRUE if the guard passes and the transition,actions should be handled; #HSM_FALSE if the guard filters the transition,actions.
 */
typedef hsm_bool(*hsm_callback_guard)( hsm_status status);

/**
 * A state's exit callback.
 *
 * @param status Current state of the machine. 
 */
typedef hsm_callback_action hsm_callback_exit;


typedef struct hsm_state_rec hsm_state_t;

//---------------------------------------------------------------------------
/**
 * a state descriptor
 * http://dev.ionous.net/2012/06/state-descriptors.html
 */
struct hsm_state_rec
{
    /**
     * name of the state ( useful for debugging )
     */
    const char * name;

    /**
     * the state's event handler callback
     */
    hsm_callback_process_event process;
    
    /**
     * enter action handler
     */
    hsm_callback_enter enter; 

    /**
     * exit action handler
     */
    hsm_callback_exit exit;

    /**
     * default sub-state (if any) for this state
     */
    hsm_state initial;

    /**
     * parent of this state
     */
    hsm_state parent;

    /**
     * distance to the root state
     * this->depth = this->parent->depth+1; 
     * root most state's depth == 0
     */
    int depth;
};

/**
 * Macro for declaring a state.
 *
 * Requires:
 * @li A event handler function: MyStateEvent
 * 
 * @include hsm_state.c
 *
 * @param state      User defined name for the state.
 * @param parent     HsmTopState or a previously declared user defined state name.
 * @param initial    First state this state should enter. can be NULL.
 * 
 * @see #hsm_callback_process_event
 */
#define HSM_STATE( state, parent, initial ) \
        hsm_state state##Event( hsm_status ); \
        _HSM_STATE( state, parent, state##Event, 0, 0, initial )

/**
 * Macro for declaring a state + enter callback.
 *
 * Requires:
 * @li An event handler function: MyStateEvent
 * @li An entry function        : MyStateEnter
 *
 * @param state      User defined name for the state.
 * @param parent     a user defined state name, or HsmTopState.
 * @param initial    First state this state should enter. can be NULL.
 *
 * @see #hsm_callback_enter, #hsm_callback_process_event
 */
#define HSM_STATE_ENTER( state, parent, initial ) \
        hsm_state state##Event( hsm_status ); \
        hsm_context state##Enter( hsm_status ); \
        _HSM_STATE( state, parent, state##Event, state##Enter, 0, initial );

/**
 * Macro for declaring a state + enter and exit callbacks
 *
 * Requires:
 * @li An event handler function: MyStateEvent
 * @li An entry function        : MyStateEnter
 * @li An exit function         : MyStateExit
 *
 * @param state      User defined name for the state.
 * @param parent     HsmTopState or a previously declared user defined state name.
 * @param initial    First state this state should enter. can be NULL.
 *
 * @see #hsm_callback_exit, #hsm_callback_enter, #hsm_callback_process_event
 */
#define HSM_STATE_ENTERX( state, parent, initial ) \
        hsm_state state##Event( hsm_status ); \
        hsm_context state##Enter( hsm_status ); \
        void state##Exit ( hsm_status ); \
        _HSM_STATE( state, parent, state##Event, state##Enter, state##Exit, initial );

/**
 * Verbose macro for declaring a state.
 * This macro is used by #HSM_STATE, #HSM_STATE_ENTER, #HSM_STATE_ENTERX.
 * @include .\hsm_state.txt
 *
 * @param State      User defined name for the state.
 * @param Parent     A user defined state name, or HsmTopState.
 * @param Process    Event handler function.
 * @param Enter      Function to call on state enter.
 * @param Exit       Function to call on state exit.
 * @param Initial    Initial state for this state to enter, or 0.
 */
#define _HSM_STATE( State, Parent, Process, Enter, Exit, Initial ) \
        hsm_state Parent(); \
        hsm_state State##Lookup##Initial(); \
        hsm_state State##Lookup##0() { return 0; } \
        hsm_state State() { \
            static struct hsm_state_rec myinfo= { 0 }; \
            if (!myinfo.name) { \
                myinfo.name= #State; \
                myinfo.process= Process; \
                myinfo.enter= Enter; \
                myinfo.exit= Exit; \
                myinfo.parent= Parent(); \
                myinfo.depth= myinfo.parent ? myinfo.parent->depth+1 : 0; \
                myinfo.initial= State##Lookup##Initial(); \
            } \
            return &myinfo; \
        } \
        hsm_state Parent##Lookup##State() { return State(); }
 

#endif // #ifndef __HSM_STATE_H__


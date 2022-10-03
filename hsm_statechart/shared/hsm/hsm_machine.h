/**
 * @file hsm_machine.h
 *
 * Statemachine structures and functions.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */

/** @mainpage hsmstate-chart
 *
 * @section Introduction
 *
 * <b>hsm-statechart</b> is an easy to use hiearchical state machine for C and C++. It's 
 * hosted on google code at http://code.google.com/p/hsm-statechart/.\n
 * 
 * This doxygen generated documentation is intended as a API reference.
 * For tutorials and how to get started, please see the website.
 *
 * @section Overview
 *
 * @li See #HSM_STATE for macro declaration of new states.
 * @li See hsm_machine_rec or hsm_context_machine_rec on creating a state machine.
 * @li See hsm_info.h to spy on the internal processing of running machines.
 * @li See hsm_builder.h for the new builder interface.
 * @li See hula.h for the lua bindings.
 *
 * @section License
 *
 * All code in hsm-statechart copyright (c) 2012, everMany, LLC, and licensed under the "New BSD" (BSD 3-Clause) License,
 * except hash.c, and hash.h which come from the University of California, Berkeley and its contributors,
 * and the FNVa crc geneneration code which is in the public domain. See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_MACHINE_H__
#define __HSM_MACHINE_H__

#include "hsm_info.h"
#include "hsm_stack.h"
#include "hsm_state.h"
#include "hsm_status.h"

typedef struct hsm_machine_rec hsm_machine_t;
typedef struct hsm_context_machine_rec hsm_context_machine_t;

// give the lower 16 to user flags
#define HSM_FLAGS_CTX      (1<<16)   // is the machine a context machine
#define HSM_FLAGS_HULA     (1<<17)   // is the machine a hula machine
//#define HSM_FLAGS_INFO   (1<<17)   // flags per thing to log?
//#define HSM_FLAGS_REGION (1<<18)

//---------------------------------------------------------------------------
/**
 * The statemachine object. 
 *
 * 1. Initialize with HsmMachine()
 * 2. Start the machine with HsmStart()
 * 3. Send events with HsmSignalEvent()
 */
struct hsm_machine_rec
{
    /**
     * Per state machine flags
     */
    hsm_uint32 flags;
    
    /**
     * Inner-most state currently active.
     * NULL until HsmStart() called.
     */
    hsm_state current;
};

/**
 * Extends hsm_machine_rec with a context stack to provide per-state instance data.
 */
struct hsm_context_machine_rec
{
    /**
     * Core machine data
     */
    hsm_machine_t core;

    /**
     * Context stack for tracking optional per-state instance data.
     */
    hsm_context_stack_t stack;
};


//---------------------------------------------------------------------------
/**
 * Initialize a statemachine to its default values
 *
 * @param machine hsm_machine_rec to initialize.
 *
 * @return #hsm_machine  pointer
 */
hsm_machine HsmMachine( struct hsm_machine_rec* machine );

/**
 * Initialize a statemachine with a context stack to its default values
 *
 * @param machine hsm_context_machine_rec to initialize.
 * @param ctx Optional context for the entire machine.
 *
 * @return #hsm_machine pointer
 */
hsm_machine HsmMachineWithContext( struct hsm_context_machine_rec* machine, hsm_context ctx );

/**
 * Start a machine.
 *
 * @param hsm The #hsm_machine to start.
 * @param state The first state to move to.
 * @return #HSM_FALSE on error ( ex. the machine was already started )
 */
hsm_bool HsmStart( hsm_machine hsm, hsm_state state );

/**
 * Send the passed event to the machine.
 * 
 * @param hsm The #hsm_machine targeted. 
 * @param evt A user defined event.
 *
 * The system will launch actions, trigger transitions, etc.
 * @return #HSM_TRUE if handled
 */
hsm_bool HsmSignalEvent( hsm_machine hsm, hsm_event evt );

/**
 * Determine if a machine has been started, and has not reached a terminal, nor an error state.
 *
 * @param hsm #hsm_machine
 * @return #HSM_TRUE if running
 */
hsm_bool HsmIsRunning( const hsm_machine hsm );

/**
 * Traverses the active state hierarchy to determine if hsm is possibly in the passed state.
 *
 * @param hsm  #hsm_machine
 * @param state #hsm_state
 */
hsm_bool HsmIsInState( const hsm_machine hsm, hsm_state state );

/**
 * A machine in a final state has deliberately killed itself.
 * HsmSignalEvent() will no longer trigger event callbacks for this machine.
 * 
 * @return The globally shared final state token.
 */
hsm_state HsmStateFinal();

/**
 * Token state for when a machine has inadvertently killed itself. 
 * HsmSignalEvent() will no longer trigger event callbacks for this machine.
 *
 * @return The globally shared error pseduo-state.
 */
hsm_state HsmStateError();

/**
 * Token state that can be used by event handler functions to indicate the event was handled
 * and no further processing of this event is required.
 *
 * @return The globally shared "its okay" token.
 */
hsm_state HsmStateHandled();

/**
 * Token state for use with the #HSM_STATE macros to represent the outer most state
 * @return The globally shared top most state token.
 */
hsm_state HsmTopState();


#endif // #ifndef __HSM_MACHINE_H__

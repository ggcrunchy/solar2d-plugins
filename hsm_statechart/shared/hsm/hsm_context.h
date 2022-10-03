/**
 * @file hsm_context.h
 *
 * Support for optional per-state instance data.
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#pragma once
#ifndef __HSM_CONTEXT_H__
#define __HSM_CONTEXT_H__

#include "hsm_forwards.h"

typedef struct hsm_context_rec hsm_context_t;
typedef struct hsm_context_stack_rec hsm_context_stack_t;
typedef struct hsm_context_stack_rec *hsm_context_stack;

/**
 * Hear about your context object after just after its been popped.
 * Can be used to free memory for state instance data you allocated during state entry.
 * It's designed like a destructor and so doesn't provide any parameters other than itself.
 *
 * @param hsm_context_t* The context being freed
 *
 * @see HsmContextStack, HSM_STATE_ENTER
 */
typedef void (*hsm_callback_popped_context)( hsm_context_t* );

/**
 * A per state instance context object.
 * You can "derive" from this structure by making the first member of your own structure
 *
 * @note lifetime must be >= duration of its associated state(s).
 */
struct hsm_context_rec
{
    /**
     * @internal: pointer to next highest unique context
     * managed by the system
     */
    hsm_context_t* parent;

    /**
     * per context callback
     */
    hsm_callback_popped_context popped;
};

/**
 * Helper to create a new context object.
 * It will be auto-freed when popped.
 * 
 * @param size Total size of the user context structure.
 * @return Newly allocated context data, NULL if the allocation failed.
 * 
 * Sets the hsm_context_rec::popped handler to an internal free function.
 * You can also allocate your objects on your own if you want more control.
 */
hsm_context HsmContextAlloc( size_t size );

//---------------------------------------------------------------------------
/**
 * Per state machine instance manager of context data.
 *
 * note: the most effienct implementation depends on the sparsity of the stack
 *  if it's very sparse: a list of (context,depth) tuples might be best
 *  if it's very full: an array of [context,...] with blank elements(*) might be best
 *  flagging is somewhere in between, though, all methods are probably fine for most people.
 */
struct hsm_context_stack_rec
{
    /**
     * most recently added unique context pointer
     */
    hsm_context context;

    /**
     * total number of pushes that have occured
     * ( always >= the depth of the statemachine's deepest state )
     */
    hsm_uint16 count;

    /**
     * bit flags for whether a push added unique data
     */
    hsm_uint16 presence;
};

/**
 * Resets the hsm_context_stack structure.
 * @param stack Stack to initialize.
 * @param init  Optional starting state; pushes record differences from this.
 * @return The stack passed in.
 * @see HsmContextPush
 */
hsm_context_stack HsmContextStack( hsm_context_stack_t* stack, hsm_context init );

#endif // __HSM_CONTEXT_H__

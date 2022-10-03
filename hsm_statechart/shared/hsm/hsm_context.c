/**
 * @file hsm_context.c
 *
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#include "hsm_context.h"
#include "hsm_stack.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

//---------------------------------------------------------------------------
// HsmContext
//---------------------------------------------------------------------------
static void HsmContextFree( hsm_context_t* ctx )
{
    free(ctx);
}

//---------------------------------------------------------------------------
hsm_context HsmContextAlloc( size_t size )
{
    hsm_context_t * ctx= calloc( 1, size );
    if (ctx) {
        // doesn't point to free directly to work better with msdev's crtdebug macros
        ctx->popped= HsmContextFree;
    }        
    return ctx;
}

//---------------------------------------------------------------------------
// HsmContextStack
//---------------------------------------------------------------------------
hsm_context_stack HsmContextStack( hsm_context_stack_t* stack, hsm_context ctx )
{
    if(stack) {
        memset(stack, 0, sizeof(hsm_context_stack_t));
        stack->context= ctx;
    }        
    return stack;
}

//---------------------------------------------------------------------------
void HsmContextPush( hsm_context_stack stack, hsm_context ctx )
{
    if (stack) {
        // invalid to push NULL once valid data exists
        HSM_ASSERT( !stack->context || ctx ); 
        if (ctx && (stack->context != ctx)) {
            ctx->parent    = stack->context;
            stack->context = ctx;
            stack->presence |= ( 1<< stack->count );
        }
        // regardless alway update the count
        // the presence bits start at zero, so presence[count] by default ==0
        ++stack->count;
    }
}

// ---------------------------------------------------------------
hsm_context HsmContextPop( hsm_context_stack stack )
{
    hsm_context bye= NULL;
    if (stack && stack->count > 0) {
        // get the presence tester
        hsm_uint32 bit= (1 << --stack->count);
        // was that a unique piece of data in that spot
        if ((stack->presence & bit) !=0) {
            // clear that bit
            stack->presence &= ~bit;
            // get the most recent thing pushed
            bye= stack->context;
            // and remove it 
            if (bye) {
                stack->context= bye->parent;
            }                
        }
    }
    return bye;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void HsmContextIterator( hsm_context_iterator_t* it, hsm_context_stack stack )
{
    HSM_ASSERT( it );
    if (it) {
        if (!stack) {
            memset( it, 0, sizeof(hsm_context_iterator_t));
        }
        else {
            it->stack= stack;
            it->context = stack->context;
            it->sparse_index= stack->count; // start at *back* ( newest pushed )
        }
    }
}

/* ---------------------------------------------------------------------------
   a bit in the 0th column: topmost returned a value different than the machine's outer context
   a bit in the 1st column: child of topmost returned a value different than its parent
   and so on.

   |..|...|.. <- sparse stack:
   0123456789 <- 9 states deep in the presence; _top=9
   0  1   2   <- 3 contexts in the store; _store.size()=3
   to traverse the stack you start with _it=9, and the location of the last context
   whenever you cross a new set bit, you move the context location.
   an iterator across a series of bits also needs the source bits.
 * --------------------------------------------------------------------------- */ 
hsm_context_t* HsmParentContext( hsm_context_iterator it )
{
    // pending pointer possess potential parent presence? perhaps.
    if (it->sparse_index>0){
        hsm_uint32 bit= (1 << --it->sparse_index);
        if ((it->stack->presence & bit) !=0) {
            it->context= it->context->parent;
        }
    }
    return it->context;
}

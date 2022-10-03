/**
 * @file hsm_machine.c
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#include "hsm_machine.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>

#include "hsm_context.h"
#include "hsm_state.h"
#include "hsm_stack.h"

// alloca is technically not an ANSI-C function, though it exists on most platforms
#ifdef WIN32
#ifndef alloca
#define alloca _alloca 
#endif
#endif

/**
 * macro for making a pseudostate:
 * 1. a function that does nothing
 * 2. a state info function to describe that state
 */
#define PSEUDO_STATE( Pseudo ) \
  static hsm_state _##Pseudo( hsm_status status ) { \
    return Pseudo(); \
  } \
  hsm_state Pseudo() { \
    static struct hsm_state_rec info= { #Pseudo, _##Pseudo };\
    return &info;   \
  }

PSEUDO_STATE( HsmStateFinal );
PSEUDO_STATE( HsmStateError );
PSEUDO_STATE( HsmStateHandled );

static hsm_state HsmDoNothing( hsm_status status ) { 
  return NULL; 
}

hsm_state HsmTopState() { 
  return 0;
}

#define HSM_STACK( hsm ) ((((hsm)->flags & HSM_FLAGS_CTX)==HSM_FLAGS_CTX) ? &((hsm_context_machine_t*)(hsm))->stack : 0)

/**
 * @internal
 * @param hsm The #hsm_machine processing the event.
 * @param source State causing the transition
 * @param target State the machine is moving into
 * @param evt Event which caused the transition
 * @warning This modifies the stack and the machine as it processes
 */
static hsm_bool HsmTransition( hsm_machine hsm, hsm_state source, hsm_state target, hsm_event evt );

/**
 * @internal
 * Used to start the statemachine going
 * walks up the tree to the top, then 'enter's from top down-to (and including) 'state'
 
 * @param hsm The #hsm_machine processing the event.
 * @param state Desired first state of the state chart
 */
static void HsmRecursiveEnter( hsm_machine hsm, hsm_state state, hsm_event evt );

/**
 * @internal
 * Enter the passed state 
 * @param hsm The #hsm_machine processing the event.
 * @param state State transitioing to
 * @param evt Event which caused the transition
 */
static hsm_bool HsmEnter( hsm_machine hsm, hsm_state state, hsm_event evt );

/* 
 * @internal
 * Transitions to the current states tree of initial states
 * @return HSM_FALSE on error.
 * @note in statecharts, init happens after enter, and it can move ( often does move ) the current state to a new state
 */
static hsm_bool HsmInit( hsm_machine hsm, hsm_event evt );

/**
 * @internal
 * Exit the current state
 *
 * @param hsm The #hsm_machine processing the event.
 * @param evt Event which caused the transition.
 */
//static  <- disabled for regions test. TODO: renable here and below.
void HsmExit( hsm_machine hsm, hsm_event evt );

//---------------------------------------------------------------------------
static hsm_info_t hsm_global_callbacks= {0};

void HsmSetInfoCallbacks( hsm_info_t* info, hsm_info_t* old_callbacks )
{
  if (old_callbacks) {
    *old_callbacks= hsm_global_callbacks;
  }
  hsm_global_callbacks= *info;
}

//---------------------------------------------------------------------------
hsm_machine HsmMachine( hsm_machine_t* hsm )
{
  HSM_ASSERT (hsm);
  if (hsm) {
    hsm->flags=0;
    hsm->current= NULL;
  }
  return hsm;
}

//---------------------------------------------------------------------------
hsm_machine HsmMachineWithContext( hsm_context_machine_t* hsm, hsm_context ctx )
{
  HSM_ASSERT( hsm );
  if (hsm && HsmMachine( &(hsm->core) )) {
    // flag that this has a context stack
    hsm->core.flags|= HSM_FLAGS_CTX;
    // initialize the stack & set the outermost context
    // we later use push to detect differences from this starting point
    HsmContextStack( &(hsm->stack), ctx );
  }
  return &(hsm->core);
}

//---------------------------------------------------------------------------
hsm_bool HsmIsRunning( const hsm_machine hsm )
{
  return hsm && ((hsm->current != HsmStateFinal()) &&
           (hsm->current != HsmStateError()));
}

//---------------------------------------------------------------------------
hsm_bool HsmIsInState( const hsm_machine hsm, hsm_state state )
{
  hsm_bool res=HSM_FALSE;
  if (hsm && state) {
    hsm_state test;
    for (test= hsm->current;  test; test=test->parent) {
      if (res=(test==state)) {
        break;
      }
    }
  }    
  return res;
}

//---------------------------------------------------------------------------
hsm_bool HsmStart( hsm_machine hsm, hsm_state first_state )
{
  HSM_ASSERT( hsm );
  HSM_ASSERT( first_state && "expected valid first state for init" );
  HSM_ASSERT( (!hsm || !hsm->current) && "already ran init" );
  
  if (hsm && !hsm->current && first_state ) 
  {
    // the specified starting state isn't necessarily the *top* state:
    // we need to walk up to the top state, then walk down to and including our first state
    HsmRecursiveEnter( hsm, first_state, NULL );

    // statecharts run enter *then* init
    // ( note: init can move us into a new state )
    HsmInit( hsm, NULL );
  }

  return HsmIsRunning( hsm );
}

//---------------------------------------------------------------------------
hsm_bool HsmSignalEvent( hsm_machine hsm, hsm_event evt )
{
  hsm_bool okay= HSM_FALSE;
  if (hsm && hsm->current) 
  {
    // bubble the event up the hierarchy until we get a valid respose 
    // ( or until we run off the top of the tree. )
    hsm_state next_state= NULL;
    hsm_state handler= hsm->current;
    hsm_context_iterator_t it;
    HsmContextIterator( &it, HSM_STACK( hsm ) );
    do {
      if (handler->process) {
        hsm_status_t status= { hsm, handler, it.context, evt };
        next_state= handler->process( &status ) ;
        if (next_state) {
          break;
        }
      }
      HsmParentContext( &it );
      handler= handler->parent;
    }
    while (handler);       

    // handlers are supposed to return HsmStateHandled
    if (!next_state) {
      if (hsm_global_callbacks.on_unhandled_event) {
        hsm_status_t status= { hsm, NULL, NULL, evt };
        hsm_global_callbacks.on_unhandled_event( &status, hsm_global_callbacks.user_data );
      }
    }
    else 
    if (next_state== HsmStateHandled()) {
      okay= HSM_TRUE;
    }
    else 
    if (next_state== HsmStateFinal() || next_state==HsmStateError()) {
      // we say this is okay, in the sense that something happened.
      // trigger a callback to inform the user?
      if (hsm->current != next_state) {
        hsm->current = next_state;
        okay= HSM_TRUE;
      }
    }
    else {
      // transition, and if all is well, init
      if (!HsmTransition( hsm, handler, next_state, evt )) {
        hsm->current= HsmStateError();
      }
      else {
        okay= HsmInit( hsm, evt );
      }
    }
  }  
  return okay;
}

//---------------------------------------------------------------------------
// warning: this indirectly alters the current state and context stack ( via HsmEnter )
static hsm_bool HsmInit( hsm_machine hsm, hsm_event cause )
{
  hsm_bool okay= HSM_TRUE;
  hsm_state initial_state;
  while ( initial_state= hsm->current->initial ) 
  {
    hsm_bool init_moves_to_child= initial_state->parent == hsm->current;
    HSM_ASSERT( init_moves_to_child && "malformed statechart: init doesnt move to child state" );

    if (hsm_global_callbacks.on_init) {
      hsm_context_stack_t* stack= HSM_STACK( hsm );
      hsm_status_t status= { hsm, hsm->current, stack ? stack->context: 0, cause };
      hsm_global_callbacks.on_init( &status, hsm_global_callbacks.user_data );
    }     
    
    if (!init_moves_to_child) {
      hsm->current= HsmStateError();
      okay= HSM_FALSE;
      break;
    }
    else {
      // continue the enter=>init pattern 
      // ( the spec says enter runs before init. it sounds insane, but works out well. )
      if (!HsmEnter( hsm, initial_state, cause )) {
        hsm->current= HsmStateError();
        okay= HSM_FALSE;
        break;  
      }
    }
  }
  return okay;
}

//---------------------------------------------------------------------------
// warning: this directly alters the current state ( and also modifies the context stack. )
// the original code did not, but this change help simplify: init, exit, and transition.
static hsm_bool HsmEnter( hsm_machine hsm, hsm_state state, hsm_event cause )
{
  //FIXME-stravis: handle invalid states better?
  const hsm_bool valid_state= (state && state->depth >=0);
  HSM_ASSERT( valid_state );
  if (valid_state) {
    // note: each state gets the context of its parent in entry
    // and can optionally generate a new context in turn
    hsm_context_stack_t* stack= HSM_STACK( hsm );
    hsm_status_t status= { hsm, state, stack ? stack->context: 0, cause };
  
    if (state->enter) {
      status.ctx= state->enter( &status );
    }
    
    // push the new context, the stack handles dupes.
    HsmContextPush( stack, status.ctx ); 
    hsm->current= state;

    // informational callback, passing in new context
    if (hsm_global_callbacks.on_entered) {
      hsm_global_callbacks.on_entered( &status, hsm_global_callbacks.user_data );
    }
  }
  return valid_state;
}

//---------------------------------------------------------------------------
// warning: this directly alters the current state ( and also modifies the context stack. )
// static 
void HsmExit( hsm_machine hsm, hsm_event cause )
{
  // note: exit, just like process, gets the context enter created
  hsm_context popped;
  hsm_state state= hsm->current;
  hsm_context_stack_t * stack= HSM_STACK( hsm );
  hsm_status_t status= { hsm, state, stack ? stack->context: 0, cause };

  // informational callback, passing the old context
  if (hsm_global_callbacks.on_exiting) {
    hsm_global_callbacks.on_exiting( &status, hsm_global_callbacks.user_data );
  }
  
  if (state->exit) {
    state->exit( &status );
  }

  // exit pops the context that enter had created.
  hsm->current= state->parent;
  popped= HsmContextPop( stack );

  // finally: let the user know
  if (popped) {
    if (hsm_global_callbacks.on_context_popped) {
      hsm_global_callbacks.on_context_popped( &status, hsm_global_callbacks.user_data );
    }
    // call the "hsm_callback_context_popped" of the item thats being popped
    if (popped->popped) {
      popped->popped( popped );
    }
  }
}

//---------------------------------------------------------------------------
// warning: this indirectly alters the current state and context stack ( via HsmEnter  )
static void HsmRecursiveEnter( hsm_machine hsm, hsm_state state, hsm_event cause )
{
  if (state) {
    HsmRecursiveEnter( hsm, state->parent, cause);
    HsmEnter( hsm, state, cause );
  }
}

//---------------------------------------------------------------------------
/**
 According to the http://www.w3.org/TR/scxml/#SelectingTransitions:
 I. """ The behavior of a transition with 'type' of "external" (the default) is defined in terms of the transition's source state 
   (which is the state that contains the transition), the transition's target state(or states), 
  and the Least Common Compound Ancestor (LCCA) of the source and target states 
  When a transition is taken, the state machine will exit all active states that are proper descendants of the LCCA, 
  starting with the innermost one(s) and working up to the immediate descendant(s) of the LCCA. 

  Then the state machine enters the target state(s), plus any states that are between it and the LCCA, 
  starting with the outermost one (i.e., the immediate descendant of the LCCA) and working down to the target state(s). 
   
  As states are exited, their onexit handlers are executed. Then the executable content in the transition is executed, 
  followed by the onentry handlers of the states that are entered. If the target state(s) of the transition is not atomic, 
  the state machine will enter their default initial states recursively until it reaches an atomic state(s).

  Note that the LCCA is neither entered nor exited."""
  
 II. When target is a descendant of source....
   "an internal transition will not exit and re-enter its source state, while an external one will."

 III. """ [For] a transition whose target is its source state.... the state is exited and reentered, 
   triggering execution of its onentry and onexit executable content."""
 */
#define ERROR_IF_FALSE( x, msg ) while(!(x)) { HSM_ASSERT(msg); return HSM_FALSE; }

static hsm_bool HsmTransition( hsm_machine hsm, hsm_state source, hsm_state target, hsm_event  cause )
{
  // above ( I. ) specifically designates the lca of 'source' and 'target'.
  // which can only, then, be the same node as 'source' or, more likely, a node less deep than 'source'.
  // since 'source' is the node that handled the event, and since events propogate from 'current' up to the top of the tree 
  // that also means 'source' is either the same node as, or higher than, 'current'.
  // ie. lca->depth <= source->depth <= current->depth
  // therefore: as a first step, bring 'current' to 'source', and work towards lca from there.
  while( hsm->current != source ) {
    HsmExit( hsm, cause );
    ERROR_IF_FALSE( hsm->current, "jumped past top" );
  }        

  // quick check for self transition: the source targeted itself ( III. above )
  if ( hsm->current == target ) {
    HsmExit( hsm, cause );
    // ( <--- note: in uml transitions actions would take place here )
    // ( in hsm_statechart, they've already happened by now )
    HsmEnter( hsm, target, cause );
  }
  else {
    // the easiest entry path record is an object on the stack... 
    // doesnt worry too much about stack overflow, 
    // instead it lets the user control it via HSM_MAX_DEPTH...
    int pt=0;
    hsm_state track= target;
    // change to help simplify lua rocks
    //hsm_state* path_to_target=(hsm_state*) alloca( target->depth * sizeof(hsm_state) );
    hsm_state path_to_target[ HSM_MAX_DEPTH ];
    //ERROR_IF_FALSE( path_to_target, "out of space" );
    ERROR_IF_FALSE( target->depth < HSM_MAX_DEPTH, "transition max depth exceeded" );
 
    // source deep than target?
    if (hsm->current->depth > track->depth) {
      // *exit* up to the same level
      while( hsm->current->depth > track->depth) {
        HsmExit( hsm, cause ); // we exit, then move, b/c, if we don't leave the node we don't want an exit 
        ERROR_IF_FALSE( hsm->current, "jumped past top" );  
      }
    }
    // target deeper than source?
    else {
      // *record* its path up to the same level
      while (track->depth > hsm->current->depth ) {
        path_to_target[pt++]= track;
        track= track->parent;
        ERROR_IF_FALSE( track, "jumped past top" );
      }

    // trigger an external transition? (re: II. above)
    // by design, in hsm-statechart all transitions are internal.
    #ifdef HSM_USE_EXTERNAL_TRANSITIONS
      // since track has now risen along the path of target-to-root
      // and has reached the the level of source....
      // if they are the same node, then source was an ancestor of target.
      if (hsm->current == track) {
        HsmExit( hsm, cause );     // this bumps the current state up
        path_to_target[pt++]= track; // record that we need to renter it
        track= hsm->current;     // and bump track up to the same level
      }
    #endif      
    }
      
    // keep going up together till current and track have found each other
    // ( keep exiting 'current' as it goes up; keep recording 'track' as *it* goes up )
    while (hsm->current!= track) {
      HsmExit( hsm, cause ); 
      path_to_target[pt++]= track;
      track= track->parent;
      ERROR_IF_FALSE( hsm->current&&track, "jumped past top" );
    }      

    // ( <--- note: in uml transitions actions would take place here )
    // ( in hsm_statechart, they've already happened by now )

    // now have current use path the we just recorded:
    // it's turtles all the way down.
    while (pt>0) {
      track= path_to_target[--pt];
      HsmEnter( hsm, track, cause );
    }
  }
  // note: on error we will have already returned HSM_FALSE
  return HSM_TRUE;
}

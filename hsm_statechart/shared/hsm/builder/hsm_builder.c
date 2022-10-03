/**
 * @file hsm_builder.c
 *
 * \internal
 * Copyright (c) 2012, everMany, LLC.
 * All rights reserved.
 * 
 * Code licensed under the "New BSD" (BSD 3-Clause) License
 * See License.txt for complete information.
 */
#include <hsm/hsm_machine.h>
#include "hash.h"
#include "hsm_builder.h"

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

extern const unsigned char HsmLowerTable[];

//---------------------------------------------------------------------------
// FNVa is in the public domain, http://isthe.com/chongo/tech/comp/fnv/
// http://dev.ionous.net/2007/03/string-hashes.html for more about string hashing
hsm_uint32 hsmStringHash(const char *string, hsm_uint32 hval) 
{
    const hsm_uint32 prime32= ((hsm_uint32)0x01000193);
    const char *p=string;
    if (p) {
        char ch;
        for (ch=*p; ch; ch=*(++p)) {
          hsm_uint32 val= (hsm_uint32) HsmLowerTable[ch];
          hval= (hval ^ val) * prime32;
        }
    }        
    return hval;
}

// oh the things we'll build: a hat, a cat, a series of typedefs, all that.
typedef struct state_rec    state_t;
typedef struct guard_rec    guard_t;
typedef struct guard_rec_ud  guard_ud_t;
typedef struct guard_rec_raw guard_raw_t;
typedef struct action_rec   action_t;
typedef struct process_rec  process_t;
typedef struct process_rec_ud  process_ud_t;
typedef struct process_rec_raw process_raw_t;
typedef struct handler_rec  handler_t;

//---------------------------------------------------------------------------
/**
 * one each state defined via the hsmBuilder interface.
 * it augments the 'common' state descriptor.
 * with data to generically handle events, guards, actions, and transitions.
 */
struct state_rec
{
    hsm_state_t desc;       // the common statedescriptor

    // TODO? add flags to avoid thunks for enter, and exit when using user_data?
    // ( see process for what it does )

    hsm_callback_enter_ud enter;
    void * enter_ud;

    hsm_callback_action_ud exit;
    void * exit_ud;

    process_t* process;   // list of processors to handle events
};

// querries for build status
#define Entry_ReadyToBuild( e )     ((e) && !(e)->clientData)
#define Entry_FinishedBuilding( e ) ((e) &&  (e)->clientData && (((hsm_state)(e)->clientData)->process== RunGenericEvent))
#define Entry_BuildInProgress( e )  ((e) &&  (e)->clientData && (((hsm_state)(e)->clientData)->process!= RunGenericEvent))

//---------------------------------------------------------------------------
/**
 * 
 */
struct action_rec
{
    int flags;               // tbd, might control how run is called
    hsm_callback_action_ud run; // the action is the same as 'handler', the caller can translate from there.
    void *action_data;         // passed to run
    action_t* next;           // we are a linked list, b/c there can be more than one action.
};


//---------------------------------------------------------------------------
/**
 * eventually want a full blown decision tree to allow ands and ors
 * right now, it's all ands
 */
enum guard_type
{
    GuardUd= 1,
    GuardRaw=2,
};
 
struct guard_rec
{
    int type;                     
    guard_t * next;
};

struct guard_rec_raw 
{
    guard_t core;
    hsm_callback_guard match;      // function to determine if this rec handles some any particular event
    void * guard_data;             // passed to match    
};

struct guard_rec_ud
{
    guard_t core;
    hsm_callback_guard_ud match;   // function to determine if this rec handles some any particular event
    void * guard_data;             // passed to match    
};

#define CALL_GUARD( rec, status ) ( ((rec)->type == GuardUd) ? \
                                        ( ((guard_ud_t* )(rec))->match( status, ((guard_ud_t*) (rec))->guard_data ) ):\
                                        ( ((guard_raw_t*)(rec))->match( status )) )

//---------------------------------------------------------------------------
/**
 * event processor flags.
 * the flags allow us to avoid a thunk for user data
 * without the flags we'd have to call a generic function, which would then call a user data function
 * ( exit_ud works like that right now )
 *
 * @see process_rec 
 */
enum process_flags 
{
    ProcessCallback=1<<0,           // user has specified a callback
    ProcessHandler= 1<<1,           // user has constructed a list of guards and actions via the builder interface
    ProcessUd     = 1<<2,           // there's user data to send to the callback
    // now: you can already see, and i've already thought: 
    // can't the guards and actions all share the ud of the processor?
    // maybe so -- needs some investigation.
};

#define CALL_PROCESS( rec, status ) ( ((rec)->flags & ProcessUd) ? \
                                        ( ((process_ud_t* )(rec))->process( status, ((process_ud_t*) (rec))->process_data ) ):\
                                        ( ((process_raw_t*)(rec))->process( status )) )

/**
 * event processing data
 * one state process becomes a tree of processing, scoped by entry and exit.
 */
struct process_rec
{
    enum process_flags flags;
    process_t *next;                // if not us, who? if not now, next?
};

/**
 * event processing with a callback :}
 */
struct process_rec_raw
{
    process_t core;
    hsm_callback_process_event process;
};

/**
 * extends process with user data.
 */
struct process_rec_ud
{
    process_t core;
    hsm_callback_process_ud process;
    void * process_data;
};

/**
 * extends process with automatic processing data:
 * actions, guards, etc.
 
 * one each event defined via the hsmBuilder interface.
 * may well be used for guards and events.
 *
 * re: target.
 *
 * when a user calls hsmGoto/Id the state in question might not yet exist:
 * so we can't store hsm_state directly; so instead we store the hash entry
 * ( could also store the id, but entry is safe, and fast, so why not. )
 *
 * now, its desired that hsmBuilder and core states interoperate
 * that's where a future hsmRegisterState comes from
 */
struct handler_rec
{
    process_t core;
    guard_t *guard;
    action_t *actions;      // if we do match: we may have things to do
    hash_entry_t* target;   //             not to mention, places to '.
};

/**
 * state_t extends hsm_status directly
 */
#define StateFromStatus( status ) ((state_t*) status->state)

//---------------------------------------------------------------------------
/**
 * run time helper to reflect singal event calls to the right guards and actions
 */
static hsm_context RunGenericEnter( hsm_status status )
{
    state_t* state= StateFromStatus( status );
    return state->enter( status, state->enter_ud );
}

//---------------------------------------------------------------------------
/**
 * run time helper to reflect singal event calls to the right guards and actions
 */
static void RunGenericExit( hsm_status status )
{   
    state_t* state= StateFromStatus( status );
    state->exit( status, state->exit_ud );
}

//---------------------------------------------------------------------------
/**
 * run time helper to reflect singal event calls to the right guards and actions
 */
static hsm_state RunGenericEvent( hsm_status status )
{
    hsm_state next_state=NULL;
    state_t* state= StateFromStatus( status );
    const process_t* et;
    // look through the specified event processors
    for (et= state->process; et && !next_state; et=et->next) {
        if (et->flags & ProcessCallback) {
            next_state= CALL_PROCESS( et, status );
        }
        // this block of code could have been installed as a custom process callback
        // but this avoids a separate function call, and is pretty straight forward
        // dont know, maybe i will revist for improved code aethetics
        else {
            handler_t * handler= (handler_t*)et;
            // determine if some guard blocks the event from running
            guard_t* guard;
            for (guard= handler->guard; guard; guard=guard->next) {
                if (!CALL_GUARD( guard, status )) {
                    break;
                }
            }
            // no guard blocks this handler from running,:
            if (!guard) {
                // run action(s)
                action_t* at;
                for (at= handler->actions; at; at=at->next) {
                    at->run( status, at->action_data );
                }
                // transition to target, or flag as handled.
                if (handler->target) {
                    next_state= (hsm_state) handler->target->clientData;
                }
                else {
                    next_state= HsmStateHandled();
                }
            }
        }            
    }
    return next_state;
}
//---------------------------------------------------------------------------
/**
 * @internal construct a new state object
 * @param name State's name
 * @param namelen if non-zero, that number of name chars gets copied.
 */
static state_t* NewState( state_t* parent, const char * name, int namelen )
{   
    // note: calloc is keeping strncpy null terminated
    state_t* state=(state_t*) calloc( 1, sizeof( state_t ) + namelen + 1 );
    if (state) {
        if (!namelen) {
            state->desc.name= name;
        }
        else {
            // point the name to just have the block of size memory
            char* dest= (char*) ((size_t)state) + sizeof( state_t );
            HSM_ASSERT( name && "namelen needs name" );
            if (name) {
                strncpy( dest, name, namelen );
                state->desc.name= dest;
            }                
        }

        // setup the parent info of new state
        if (parent) {
            state->desc.parent= &(parent->desc);
            state->desc.depth = parent->desc.depth+1;

            // default init state for 'parent' is the first child specified
            if (!parent->desc.initial) {
                parent->desc.initial= &(state->desc);
            }
        }
    }
    return state;
}

//---------------------------------------------------------------------------
/**
 * @internal construct a new event object
 */
static process_t* NewProcessUD( state_t* state, hsm_callback_process_ud process, void * process_data )
{
    process_t* ret= NULL;
    if (state && process) {
        process_ud_t * processor= (process_ud_t*) calloc( 1, sizeof( process_ud_t ) );
        if (processor) {
            ret= &(processor->core);
            // set up the processor cbs
            processor->process= process;
            processor->process_data= process_data;
            // link to the list of (other) processors for this state:
            ret->flags= ProcessCallback|ProcessUd;
            ret->next= state->process;
            state->process= ret;
        }        
    }        
    return ret;
}

//---------------------------------------------------------------------------
/**
 * @internal construct a new event object
 */
static process_t* NewProcessRaw( state_t* state, hsm_callback_process_event process )
{
    process_t* ret= NULL;
    if (state && process) {
        process_raw_t* processor= (process_raw_t*) calloc( 1, sizeof( process_raw_t ) );
        if (processor) {
            ret= &(processor->core);
            // set up the processor cbs
            processor->process= process;
            // link to the list of (other) processors for this state:
            ret->flags= ProcessCallback;
            ret->next= state->process;
            state->process= ret;
        }        
    }        
    return ret;
}

//---------------------------------------------------------------------------
/**
 * @internal construct a new event object
 */
static process_t* NewHandler( state_t* state )
{   
    process_t* ret=0;
    if (state) {
        handler_t* handler= (handler_t*) calloc( 1, sizeof( handler_t ) );
        if (handler) {
            ret= &(handler->core);
            // link to the list of (other) processors for this state:
            ret->flags= ProcessHandler;
            ret->next= state->process;
            state->process= ret;
        }        
    }        
    return ret;
}

//---------------------------------------------------------------------------
/**
 * @internal construct a new action object
 */
static action_t* NewAction( handler_t* handler, hsm_callback_action_ud run, void * action_data )
{
    action_t* action= NULL;
    if (handler && run) {
        action= (action_t*) calloc( 1, sizeof( action_t ) );
        if (action) {
            // set:
            action->run= run;
            action->action_data= action_data;
            //link:
            action->next= handler->actions;
            handler->actions= action;
        }
    }        
    return action;
}

//---------------------------------------------------------------------------
/**
 * @internal construct a new guard
 */
static guard_t* NewGuardUD( handler_t* handler, hsm_callback_guard_ud match, void * guard_data )
{   
    guard_t *ret= NULL;
    if (handler && match) {
        guard_ud_t* guard= (guard_ud_t*) calloc( 1, sizeof( guard_ud_t ) );
        if (guard) {
            // set the default matching function
            guard->match= match;
            guard->guard_data= guard_data;
            // WARNING: link order doesnt matter right now because they are all "and" but it will for "or" and "and"
            ret= &guard->core;
            ret->type= GuardUd;
            ret->next= handler->guard;
            handler->guard= ret;
        }            
    }        
    return ret;
}

static guard_t* NewGuardRaw( handler_t* handler, hsm_callback_guard match )
{   
    guard_t *ret= NULL;
    if (handler && match) {
        guard_raw_t* guard= (guard_raw_t*) calloc( 1, sizeof( guard_raw_t ) );
        if (guard) {
            // set the default matching function
            guard->match= match;
            // WARNING: link order doesnt matter right now because they are all "and" but it will for "or" and "and"
            ret= &guard->core;
            ret->type= GuardRaw;
            ret->next= handler->guard;
            handler->guard= ret;
        }            
    }        
    return ret;
}

//---------------------------------------------------------------------------
// Builder Machine
//---------------------------------------------------------------------------

typedef struct builder_rec builder_t;
typedef enum builder_events builder_events_t;

/**
 * the handlers that the builder interface uses to build hsms.
 * ~= one per interface function.
 * why *shouldn't* a statemachine build a statemachine after all?
 */
enum builder_events 
{
    _hsm_begin,
    _hsm_end,

    _hsm_enter_ud, 
    _hsm_enter_raw,

    _hsm_exit_ud,
    _hsm_exit_raw,

    _hsm_process_ud,
    _hsm_process_raw,

    _hsm_guard_ud,
    _hsm_guard_raw,
    
    _hsm_action_ud,
    //_hsm_action_raw,
    _hsm_goto,
};

/**
 * the builder interface is secretly backed by one of these.
 */
struct builder_rec
{
    hsm_context_t ctx;          // we are used as context in the builder machine
    const char *error;
    state_t * current;          // inner most state that's b/t begin,end.
    int count;                  // nested count of states
    hash_table_t hash;          // a hash of states
};

/**
 * builder_rec helper macro to return the state that's currently getting built
 */
#define Builder_CurrentState( b ) (state_t*)((b)->current)

/**
 * builder_rec helper macro to record an error string
 */
static void Builder_Error( builder_t*builder, const char * error ) 
{
    builder->error=error; // handy spot for a breakpoint
}

/**
 * 
 */
#define Builder_Valid( b ) ((b)->hash.bucketPtr !=0)

//---------------------------------------------------------------------------
/**
 * data associated with the builder_events_t
 * essentially: the interface functions repacked a function object.
 */
typedef struct hsm_event_rec BuildEvent; 
struct hsm_event_rec
{
    builder_events_t type;
};

typedef struct begin_event_rec BeginEvent;
struct begin_event_rec
{
    BuildEvent core;
    int id;
    const char * name;
    int namelen;
};

typedef struct state_event_rec StateEvent;
struct state_event_rec
{
    BuildEvent core;
    int id;
};

typedef struct enter_event_rec EnterEvent;
struct enter_event_rec
{
    BuildEvent core;
    hsm_callback_enter_ud enter;
    void * enter_data;   
};

typedef struct raw_enter_event_rec RawEnterEvent;
struct raw_enter_event_rec
{
    BuildEvent core;
    hsm_callback_enter enter;
};

typedef struct raw_process_event_rec RawProcessEvent;
struct raw_process_event_rec
{
    BuildEvent core;
    hsm_callback_process_event process;
};


typedef struct process_event_ud_rec ProcessEventUd;
struct process_event_ud_rec
{
    BuildEvent core;
    hsm_callback_process_ud process;
    void * process_data;
};

typedef struct guard_event_rec GuardEvent;
struct guard_event_rec
{
    BuildEvent core;
    hsm_bool append;
};

typedef struct raw_guard_event_rec RawGuardEvent;
struct raw_guard_event_rec
{
    GuardEvent core;
    hsm_callback_guard  guard;
};

typedef struct guard_event_ud_rec GuardEventUD;
struct guard_event_ud_rec
{
    GuardEvent core;
    hsm_callback_guard_ud  guard;
    void * guard_data;   
};

typedef struct raw_action_event_rec RawActionEvent;
struct raw_action_event_rec
{
    BuildEvent core;
    hsm_callback_action  action;
};

typedef struct action_event_rec ActionEvent;
struct action_event_rec
{
    BuildEvent core;
    hsm_callback_action_ud  action;
    void * action_data;
};


HSM_STATE( HsmBuilding, HsmTopState, HsmBuildingIdle );
    HSM_STATE( HsmBuildingIdle, HsmBuilding, 0 );
    HSM_STATE_ENTER( HsmBuildingState, HsmBuilding, HsmBuildingBody );
        HSM_STATE( HsmBuildingBody, HsmBuildingState, 0 );
        HSM_STATE_ENTER( HsmBuildingHandler, HsmBuildingState, 0 );


//---------------------------------------------------------------------------
// Build:
//---------------------------------------------------------------------------
hsm_state HsmBuildingEvent( hsm_status status )
{
    hsm_state ret= HsmStateError(); // at the top level, by default, everythings an error.
    builder_t * builder= ((builder_t*)status->ctx);
    switch (status->evt->type) {
        case _hsm_begin: 
        {
            StateEvent*event=(StateEvent*)status->evt;
            hash_entry_t* entry= Hash_FindEntry( &builder->hash, event->id );
            if (Entry_ReadyToBuild( entry )) {
                ret= HsmBuildingState();
            }
            else {
                if (!entry) {
                    Builder_Error( builder, "hsmBegin for unknown state." );
                }
                else 
                if (Entry_FinishedBuilding( entry )) {
                    Builder_Error( builder, "state already finished building via hsmEnd.");    
                }
                else {
                    Builder_Error( builder, "state already being built via hsmBegin.");
                }
            }
        }            
        break;    
        default:
            Builder_Error( builder, "invalid function for state" );
        break;
    }
    return ret;
}

//---------------------------------------------------------------------------
// Idle:
//---------------------------------------------------------------------------
hsm_state HsmBuildingIdleEvent( hsm_status status )
{
    return NULL;  
}

//---------------------------------------------------------------------------
// State:
//---------------------------------------------------------------------------
hsm_state HsmBuildingBodyEvent(hsm_status status )
{   
    return NULL;
}

//---------------------------------------------------------------------------
hsm_context HsmBuildingStateEnter( hsm_status status )
{
    builder_t * builder= ((builder_t*)status->ctx);
    BeginEvent * evt= (BeginEvent*)(status->evt);
    state_t* parent= Builder_CurrentState( builder );     // parent of the new state is the current state
    hash_entry_t* entry;
    state_t* new_state;
    
    HSM_ASSERT( status->evt->type == _hsm_begin );
    entry= Hash_CreateEntry( &(builder->hash), evt->id, 0 );
    new_state= NewState( parent, evt->name, evt->namelen );
        
    HSM_ASSERT( new_state && entry->clientData == 0);
    if (new_state && entry->clientData == 0) 
    {
        entry->clientData= new_state;
        builder->current= new_state;      // later, we'll use the parent state to unwind
        ++builder->count;                 // 
    }        
    else {
        if (!new_state) {
            Builder_Error( builder, "couldn't allocate new state");
        }
        if (!entry->clientData) {
            Builder_Error( builder, "state data already exists");
        }
        else {
            Builder_Error( builder, "unknown error");
        }
    }

    // keep the builder as context
    return status->ctx;
}

//---------------------------------------------------------------------------
/**
 * note: transitioning to BuildingBody() pulls the user out of BuildingHandler if that's where they are
 * without disrupting the fact we are still building a states; it correctly ends the event handler build.
 */
hsm_state HsmBuildingStateEvent( hsm_status status )
{
    hsm_state ret= NULL;
    builder_t * builder= ((builder_t*)status->ctx);
    state_t* current= Builder_CurrentState( builder );
    switch (status->evt->type) 
    {
        case _hsm_enter_ud: {
            const EnterEvent* event= (const EnterEvent*)status->evt;
            if (!current->desc.enter && event->enter) {
                current->desc.enter= RunGenericEnter;
                current->enter= event->enter;
                current->enter_ud= event->enter_data;
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, current->desc.enter ? "enter already specified." : "enter is null." );
                ret= HsmStateError();
            }
        }
        break;
        case _hsm_enter_raw: {
            const RawEnterEvent* event= (const RawEnterEvent*)status->evt;
            if (!current->desc.enter && event->enter) {
                current->desc.enter= event->enter;
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, current->desc.enter ? "enter already specified." : "enter is null." );
                ret= HsmStateError();
            }
        }
        break;
        case _hsm_exit_raw: {
            const RawActionEvent* event= (const RawActionEvent*)status->evt;
            if (!current->desc.exit && event->action) {
                current->desc.exit= event->action;
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, current->desc.exit ? "exit already specified." : "exit is null." );
                ret= HsmStateError();
            }
        }
        break;
        case _hsm_exit_ud: {
            const ActionEvent* event= (const ActionEvent*)status->evt;
            if (!current->desc.exit && event->action) {
                current->desc.exit= RunGenericExit;
                current->exit= event->action;
                current->exit_ud= event->action_data;
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, current->desc.exit ? "exit already specified." : "exit is null." );
                ret= HsmStateError();
            }
        }
        break;
        case _hsm_process_raw: {
            const RawProcessEvent* event= (const RawProcessEvent*)status->evt;
            if (event->process) {
                NewProcessRaw( current, event->process );
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, "process is null." );
                ret= HsmStateError();
            }
        }
        break; 
        case _hsm_process_ud: {
            const ProcessEventUd* event= (const ProcessEventUd*)status->evt;
            if (event->process) {
                NewProcessUD( current, event->process, event->process_data );
                ret= HsmBuildingBody();
            }
            else {
                Builder_Error( builder, "process is null." );
                ret= HsmStateError();
            }
        }
        break; 
        // start building a event handler
        case _hsm_guard_raw: 
        case _hsm_guard_ud: 
        {
            ret= HsmBuildingHandler();
        }
        break;
        case _hsm_end: {
            // setup the event handler, this also a key that the state is good to go.
            current->desc.process= RunGenericEvent;

            // if this was the last matching begin/end pair, we're done
            // ( we dont use !current, b/c of potential containment in external states )
            if (--builder->count==0) {
                builder->current= 0;
                ret= HsmBuilding();
            }
            else {
                builder->current= (state_t*) current->desc.parent;                    
                ret= HsmBuildingBody();
            }
        }            
        break;
    }
    return ret;
}

//---------------------------------------------------------------------------
// If, and And: 
//---------------------------------------------------------------------------

static guard_t* NewGuardFromEvent( hsm_status status, handler_t* handler )
{
    guard_t* guard=0;
    if (handler) {
        if (status->evt->type == _hsm_guard_ud) {
            const GuardEventUD* event= (const GuardEventUD*)status->evt;
            guard= NewGuardUD( handler, event->guard, event->guard_data );
        }
        else 
        if (status->evt->type ==  _hsm_guard_raw) {
            const RawGuardEvent * event= (const RawGuardEvent*)status->evt;
            guard= NewGuardRaw( handler, event->guard  );
        }
        else {
            HSM_ASSERT(0 && "unexpected event");
        }
    }
    return guard;
}

hsm_context HsmBuildingHandlerEnter( hsm_status status )
{
    builder_t * builder= ((builder_t*)status->ctx);
    state_t* state= Builder_CurrentState( builder );
    guard_t* guard=0;
    handler_t* handler= (handler_t*) NewHandler( state );
    HSM_ASSERT( handler );
    guard= NewGuardFromEvent( status, handler );
    // keep the original context
    return status->ctx;
}

//---------------------------------------------------------------------------
hsm_state HsmBuildingHandlerEvent( hsm_status status )
{
    hsm_state ret=NULL;
    builder_t* builder= ((builder_t*)status->ctx);
    state_t* state= Builder_CurrentState( builder );
    HSM_ASSERT(state);
    if (state) {
        // we shouldnt be in this state unless the process is a handler...
        handler_t* handler= (handler_t*) state->process;
        HSM_ASSERT( state->process->flags & ProcessHandler );

        switch (status->evt->type) {
            case _hsm_goto: {
                const StateEvent* event= (const StateEvent*)status->evt;
                const int go= event->id;
                hash_entry_t* target= Hash_FindEntry( &(builder->hash), go );
                if (!handler->target && target) {
                    handler->target= target;
                    ret= HsmStateHandled();
                }
                else {
                    Builder_Error( builder, handler->target ? "goto already specified" : "unknown goto target" );
                    ret= HsmStateError();
                }                
            }
            break;
            case _hsm_guard_raw: 
            case _hsm_guard_ud: 
            {
                const GuardEvent* guard_event= (const GuardEvent*)status->evt;
                // on append, we want to add this guard to us
                // otherwise we let the parent state start a new event handler
                if (guard_event->append) {
                    guard_t* guard= NewGuardFromEvent( status, handler );
                    if (guard) {
                        ret= HsmStateHandled();
                    }
                    else {
                        Builder_Error( builder, "couldnt create guard");
                        ret= HsmStateError();
                    }                    
                }
            }
            break;
            case _hsm_action_ud: {
                const ActionEvent* action_event= (const ActionEvent*)status->evt;
                if (NewAction( handler, action_event->action, action_event->action_data )) {
                    ret= HsmStateHandled();
                }
                else {
                    Builder_Error( builder, !action_event->action ? "no action specified" : "couldnt allocate action");
                    ret= HsmStateError();
                }                
            }
            break;
        };        
    }        
    return ret;
}

//---------------------------------------------------------------------------
//
// Builder Interface:
//
//---------------------------------------------------------------------------

// there'd be one of each of these per thread context if such a thing were needed
static hsm_context_machine_t gMachine= {0};
static builder_t gBuilder= {0};
static int gStartCount=0;

//---------------------------------------------------------------------------
int hsmStartup()
{
    if (!gStartCount) {
        Hash_InitTable( &gBuilder.hash );
        HsmStart( HsmMachineWithContext( &gMachine, &(gBuilder.ctx) ), HsmBuilding() );
    }
    return ++gStartCount;
}

//---------------------------------------------------------------------------
int hsmShutdown()
{
    if (gStartCount>0) {
        const hsm_bool free_client_data= HSM_TRUE;
        Hash_DeleteTable( &gBuilder.hash, free_client_data );
        --gStartCount;
    }        
    return gStartCount;
}

//---------------------------------------------------------------------------
hsm_state hsmResolveId( int id ) 
{
    hsm_state ret=0;
    HSM_ASSERT( gStartCount );
    if ( gStartCount && HsmIsRunning(&gMachine.core) ) {
        const hash_entry_t* entry= Hash_FindEntry( &(gBuilder.hash), id );
        ret= Entry_FinishedBuilding( entry ) ? (hsm_state) entry->clientData : (hsm_state) 0;
    }        
    return ret;
}

//---------------------------------------------------------------------------
hsm_state hsmResolve( const char * name ) 
{
    return hsmResolveId( hsmState( name ) );
}

//---------------------------------------------------------------------------
hsm_bool hsmStartId( hsm_machine hsm, int id )
{
    return HsmStart( hsm, hsmResolveId( id ) );
}

//---------------------------------------------------------------------------
hsm_bool hsmStart( hsm_machine hsm, const char * name )
{
    return hsmStartId( hsm, hsmState( name ) );
}

//---------------------------------------------------------------------------
int hsmState( const char * name )
{
    int ret=0;
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        hsm_uint32 id= HSM_HASH32( name );
        /** 
         * note: i actually tried pre-allocating hsmState objects
         * and storing those in hsmGoto, but if the user code is using string names, 
         * the allocation has to know how/whether to copy the string;
         * that's not always obvious until hsmBegin()
         */
        hash_entry_t* hash= Hash_CreateEntry( &gBuilder.hash, id, 0 );
        if (hash) {
            ret= id;
        }
    }        
    return ret;
}

//---------------------------------------------------------------------------
// all of the following functions generate handlers into the builder statemachine
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int hsmBegin( const char * name, int len )
{
    int ret=0;
    const int id= hsmState( name );
    if (id) {
        BeginEvent evt= { _hsm_begin, id, name, len };
        if (HsmSignalEvent( &gMachine.core, &evt.core )) {
            ret= id;
        }
    }        
    return ret;
}

//---------------------------------------------------------------------------
#if 0 // not sure that builder will work properly without the string version
int hsmBeginId( int id )
{
    int ret=0;
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        BeginEvent evt= { _hsm_begin, id };
        if( HsmSignalEvent( &gMachine.core, &evt.core ) ) {
            ret=id;
        }            
    }        
    return ret;
}
#endif

//---------------------------------------------------------------------------
void hsmOnEnterUD( hsm_callback_enter_ud entry, void *enter_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        EnterEvent evt= { _hsm_enter_ud, entry, enter_data };
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}

//---------------------------------------------------------------------------
void hsmOnEnter( hsm_callback_enter entry )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        RawEnterEvent evt= { _hsm_enter_raw, entry };
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}

//---------------------------------------------------------------------------
void hsmOnExit( hsm_callback_action action )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        RawActionEvent evt= { _hsm_exit_raw, action };
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}
//---------------------------------------------------------------------------
void hsmOnExitUD( hsm_callback_action_ud action, void *exit_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        ActionEvent evt= { _hsm_exit_ud, action, exit_data };
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}

//---------------------------------------------------------------------------
void hsmOnEvent( hsm_callback_process_event process )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        RawProcessEvent evt= { _hsm_process_raw, process }; 
        HsmSignalEvent( &gMachine.core, &(evt.core) );
    }        
}

//---------------------------------------------------------------------------
void hsmOnEventUD( hsm_callback_process_ud process, void* process_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        ProcessEventUd evt= { _hsm_process_ud, process, process_data }; 
        HsmSignalEvent( &gMachine.core, &(evt.core) );
    }        
}

//---------------------------------------------------------------------------
void hsmIf( hsm_callback_guard guard )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        RawGuardEvent evt= { _hsm_guard_raw, 0, guard }; 
        HsmSignalEvent( &gMachine.core, &(evt.core.core) );
    }        
}

//---------------------------------------------------------------------------
void hsmIfUD( hsm_callback_guard_ud guard, void *guard_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        GuardEventUD evt= { _hsm_guard_ud, 0, guard, guard_data }; 
        HsmSignalEvent( &gMachine.core, &(evt.core.core) );
    }        
}

//---------------------------------------------------------------------------
void hsmAndUD( hsm_callback_guard_ud guard, void *guard_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        GuardEventUD evt= { _hsm_guard_ud, HSM_TRUE, guard, guard_data }; 
        HsmSignalEvent( &gMachine.core, &(evt.core.core) );
    }        
}

//---------------------------------------------------------------------------
void hsmGoto( const char * name )
{
    hsmGotoId( hsmState( name ) );
}

//---------------------------------------------------------------------------
void hsmGotoId( int id )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        StateEvent evt= { _hsm_goto, id };
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}

//---------------------------------------------------------------------------
void hsmRunUD( hsm_callback_action_ud action, void *action_data )
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        ActionEvent evt= { _hsm_action_ud, action, action_data }; 
        HsmSignalEvent( &gMachine.core, &evt.core );
    }        
}

//---------------------------------------------------------------------------
void hsmEnd()
{
    HSM_ASSERT( gStartCount );
    if ( gStartCount ) {
        BuildEvent evt= { _hsm_end };
        HsmSignalEvent( &gMachine.core, &evt );
    }        
}

/*
** luaproc API
** See Copyright Notice in luaproc.h
*/

#include <pthread.h>
#include <stdlib.h>
// STEVE CHANGE
/*
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
*/
#include <string.h>
#include "CoronaLibrary.h"
#include "CoronaLua.h"
#include "extensions.h"
// /STEVE CHANGE

#include "luaproc.h"
#include "lpsched.h"

#define FALSE 0
#define TRUE  !FALSE
#define LUAPROC_CHANNELS_TABLE "channeltb"
#define LUAPROC_RECYCLE_MAX 0

#if (LUA_VERSION_NUM == 501)

#define lua_pushglobaltable( L )    lua_pushvalue( L, LUA_GLOBALSINDEX )
#define luaL_newlib( L, funcs )     { lua_newtable( L ); \
  luaL_register( L, NULL, funcs ); }
#define isequal( L, a, b )          lua_equal( L, a, b )
#define requiref( L, modname, f, glob ) {\
  lua_pushcfunction( L, f ); /* push module load function */ \
  lua_pushstring( L, modname );  /* argument to module load function */ \
  lua_call( L, 1, 1 );  /* call 'f' to load module */ \
  /* register module in package.loaded table in case 'f' doesn't do so */ \
  lua_getfield( L, LUA_GLOBALSINDEX, LUA_LOADLIBNAME );\
  if ( lua_type( L, -1 ) == LUA_TTABLE ) {\
    lua_getfield( L, -1, "loaded" );\
    if ( lua_type( L, -1 ) == LUA_TTABLE ) {\
      lua_getfield( L, -1, modname );\
      if ( lua_type( L, -1 ) == LUA_TNIL ) {\
        lua_pushvalue( L, 1 );\
        lua_setfield( L, -3, modname );\
	        }\
      lua_pop( L, 1 );\
	    }\
    lua_pop( L, 1 );\
    }\
  lua_pop( L, 1 );\
  if ( glob ) { /* set global name? */ \
    lua_setglobal( L, modname );\
    } else {\
    lua_pop( L, 1 );\
    }\
}

#else

#define isequal( L, a, b )                 lua_compare( L, a, b, LUA_OPEQ )
#define requiref( L, modname, f, glob ) \
    { luaL_requiref( L, modname, f, glob ); lua_pop( L, 1 ); }

#endif

#if (LUA_VERSION_NUM >= 503)
#define dump( L, writer, data, strip )     lua_dump( L, writer, data, strip )
#define copynumber( Lto, Lfrom, i ) {\
  if ( lua_isinteger( Lfrom, i )) {\
    lua_pushinteger( Lto, lua_tonumber( Lfrom, i ));\
    } else {\
    lua_pushnumber( Lto, lua_tonumber( Lfrom, i ));\
    }\
}
#else
#define dump( L, writer, data, strip )     lua_dump( L, writer, data )
#define copynumber( Lto, Lfrom, i ) \
  lua_pushnumber( Lto, lua_tonumber( Lfrom, i ))
#endif

// STEVE CHANGE

/***********
* structs *
***********/

/* lua process */
struct stluaproc {
	lua_State *lstate;
	int status;
	int args;
	channel *chan;
	luaproc *next;
};

/* communication channel */
struct stchannel {
	list send;
	list recv;
	pthread_mutex_t mutex;
	pthread_cond_t can_be_used;
};

// /STEVE CHANGE

/********************
* global variables *
*******************/

/* channel list mutex */
static pthread_mutex_t mutex_channel_list = PTHREAD_MUTEX_INITIALIZER;

/* recycle list mutex */
static pthread_mutex_t mutex_recycle_list = PTHREAD_MUTEX_INITIALIZER;

/* recycled lua process list */
static list recycle_list;

/* maximum lua processes to recycle */
static int recyclemax = LUAPROC_RECYCLE_MAX;

/* lua_State used to store channel hash table */
static lua_State *chanls = NULL;

/* lua process used to wrap main state. allows main state to be queued in
channels when sending and receiving messages */
static luaproc mainlp;

/* main state matched a send/recv operation conditional variable */
pthread_cond_t cond_mainls_sendrecv = PTHREAD_COND_INITIALIZER;

/* main state communication mutex */
static pthread_mutex_t mutex_mainls = PTHREAD_MUTEX_INITIALIZER;

//
// STEVE CHANGE
//

static void GlobalsDestructor (void)
{
/*
	pthread_mutex_destroy(&mutex_channel_list);
	pthread_mutex_destroy(&mutex_recycle_list);
	pthread_mutex_destroy(&mutex_mainls);

	mutex_channel_list = PTHREAD_MUTEX_INITIALIZER;
	mutex_recycle_list = PTHREAD_MUTEX_INITIALIZER;
	mutex_mainls = PTHREAD_MUTEX_INITIALIZER;

	pthread_cond_destroy(&cond_mainls_sendrecv);

	cond_mainls_sendrecv = PTHREAD_COND_INITIALIZER;
    */
	recyclemax = LUAPROC_RECYCLE_MAX;
	chanls = NULL;
}

//
// /STEVE CHANGE
//

/***********************
* register prototypes *
***********************/

static void luaproc_openlualibs(lua_State *L);
static int luaproc_create_newproc(lua_State *L);
static int luaproc_wait(lua_State *L);
static int luaproc_send(lua_State *L);
static int luaproc_receive(lua_State *L);
static int luaproc_create_channel(lua_State *L);
static int luaproc_destroy_channel(lua_State *L);
static int luaproc_set_numworkers(lua_State *L);
static int luaproc_get_numworkers(lua_State *L);
static int luaproc_recycle_set(lua_State *L);
LUALIB_API int luaopen_luaproc(lua_State *L);
static int luaproc_loadlib(lua_State *L);

/***********
* structs *
***********/

#if 0 // STEVE CHANGE
/* lua process */
struct stluaproc {
	lua_State *lstate;
	int status;
	int args;
	channel *chan;
	luaproc *next;
};

/* communication channel */
struct stchannel {
	list send;
	list recv;
	pthread_mutex_t mutex;
	pthread_cond_t can_be_used;
};
#endif // /STEVE CHANGE

// STEVE CHANGE
#define UPDATE_CHANNEL_NAME "LUAPROC_UPDATE_CHANNEL"

static int luaproc_receive_allow_from_main (lua_State * L);
static int luaproc_send_allow_from_main (lua_State * L);

static int sAlertDispatcherRef = LUA_NOREF;

static void AlertDestructor (void)
{
	sAlertDispatcherRef = LUA_NOREF;
}

static int luaproc_Alert (lua_State * L)	// N.B. Main state version
{
	lua_settop(L, 2);	// message, payload

	if (sAlertDispatcherRef == LUA_NOREF) printf("Warning: invalid alert dispatcher\n");

	lua_pushvalue(L, lua_upvalueindex(1));	// message, payload, event
    lua_insert(L, 1);	// event, message, payload
    lua_setfield(L, 1, "payload");	// event = { payload = payload }, message
	lua_setfield(L, 1, "name");	// event = { name = message, payload }
	lua_getref(L, sAlertDispatcherRef);	// event, dispatcher
	lua_insert(L, 1);	// dispatcher, event
	lua_getfield(L, 1, "dispatchEvent");// dispatcher, event, dispatcher.dispatchEvent
	lua_insert(L, 1);	// dispatcher.dispatchEvent, dispatcher, event
	lua_call(L, 2, 0);	// (empty)

	return 0;
}

static int luaproc_CreateInteger (lua_State * L)
{
	AddInteger(L, luaL_optinteger(L, 1, 0));// [init], name

	return 1;
}

static int luaproc_CreateNumber (lua_State * L)
{
	AddNumber(L, luaL_optnumber(L, 1, 0));	// [init], name

	return 1;
}

static int luaproc_DestroyInteger (lua_State * L)
{
	RemoveInteger(luaL_checkstring(L, 1));

	return 0;
}

static int luaproc_DestroyNumber (lua_State * L)
{
	RemoveNumber(luaL_checkstring(L, 1));

	return 0;
}

static int luaproc_EstimateConcurrency (lua_State * L)
{
	lua_pushinteger(L, EstimateConcurrency());	// nconc

	return 1;
}

static int luaproc_GetAlertDispatcher (lua_State * L)
{
	if (L != mainlp.lstate) luaL_error(L, "get_alert_dispatcher() called outside main state");

	if (sAlertDispatcherRef == LUA_NOREF) printf("Warning: invalid alert dispatcher\n");

	lua_getref(L, sAlertDispatcherRef);

	return 1;
}

static int luaproc_GetInteger (lua_State * L)
{
	lua_Integer i;

	if (GetInteger(luaL_checkstring(L, 1), &i)) lua_pushinteger(L, i);	// name, i

	else lua_pushnil(L);// name, nil

	return 1;
}

static int luaproc_GetNumber (lua_State * L)
{
	lua_Number n;

	if (GetNumber(luaL_checkstring(L, 1), &n)) lua_pushnumber(L, n);// name, n

	else lua_pushnil(L);// name, nil

	return 1;
}

static pthread_mutex_t mutex_suspend = PTHREAD_MUTEX_INITIALIZER;

static int is_suspended = FALSE;

static void SuspendDestructor (void)
{
/*
	pthread_mutex_destroy(&mutex_suspend);

	mutex_suspended = PTHREAD_MUTEX_INITIALIZER;
    */
	is_suspended = FALSE;
}

static int luaproc_GetPhase (lua_State * L)
{
	int suspended;

	pthread_mutex_lock(&mutex_suspend);

	suspended = is_suspended;

	pthread_mutex_unlock(&mutex_suspend);

	if (suspended) lua_pushliteral(L, "suspended"); // "suspended"

	else
	{
		if (sched_is_waiting(FALSE)) lua_pushliteral(L, "waiting");// "waiting"
		else if (sched_wants_to_close(FALSE)) lua_pushliteral(L, "closing");	// "closing"
		else lua_pushliteral(L, "normal");	// "normal"
	}

	return 1;
}

static int luaproc_IsMainState (lua_State * L)
{
	lua_pushboolean(L, L == mainlp.lstate);

	return 1;
}

static int luaproc_IsWaiting (lua_State * L)
{
	lua_pushboolean(L, sched_is_waiting(TRUE));

	return 1;
}

// Forward declaration
static int luaproc_Preload (lua_State * L);

static int luaproc_Sleep (lua_State * L)
{
	if (L == mainlp.lstate) luaL_error(L, "sleep() not allowed in main state");

	SleepFor((unsigned int)luaL_checkinteger(L, 1));

	return 0;
}

static char TEST_STRING[4096];

static int TEST_STRING_LENGTH;

static int TEST_RANDOM[4096];

static int TEST_RANDOM_RPOS, TEST_RANDOM_WPOS;

static pthread_mutex_t TEST_MUTEX = PTHREAD_MUTEX_INITIALIZER;

static int luaproc_TEST_GET_RANDOM_INTEGER (lua_State * L)
{
	int i;

	pthread_mutex_lock(&TEST_MUTEX);

	i = TEST_RANDOM[TEST_RANDOM_RPOS++];

	pthread_mutex_unlock(&TEST_MUTEX);

	lua_pushinteger(L, i);

	return 1;
}

static int luaproc_TEST_PUSH_RANDOM_INTEGER (lua_State * L)
{
	int i = luaL_checkint(L, 1);

	pthread_mutex_lock(&TEST_MUTEX);

	TEST_RANDOM[TEST_RANDOM_WPOS++] = i;

	pthread_mutex_unlock(&TEST_MUTEX);

	return 0;
}

static int luaproc_TEST_APPEND_STRING (lua_State * L)
{
	size_t len = lua_objlen(L, 1);
	int cur, to, add_zero;

	pthread_mutex_lock(&TEST_MUTEX);

	cur = TEST_STRING_LENGTH;
	TEST_STRING_LENGTH += len;

	if (TEST_STRING_LENGTH > 4096) TEST_STRING_LENGTH = 4096;

	add_zero = TEST_STRING_LENGTH < 4096;
	to = TEST_STRING_LENGTH;
	
	if (add_zero) ++TEST_STRING_LENGTH;

	pthread_mutex_unlock(&TEST_MUTEX);

	if (len && cur < 4096)
	{
		strncpy(&TEST_STRING[cur], lua_tostring(L, 1), (size_t)(to - cur));

		if (add_zero) TEST_STRING[to] = '\0';
	}

	return 0;
}

static int luaproc_TEST_PRINT_FROM_TO (lua_State * L)
{
	int from = luaL_checkint(L, 1) - 1, n = luaL_checkint(L, 2), nstrs = 0;
	char buf[4096];

	if (from < 0) from = 0;
	if (n > 4096) n = 4096;

	lua_newtable(L);

	while (from < n)
	{
		int i;

		for (i = 0; (from + i) < 4096 && TEST_STRING[from + i]; ++i)
		{
			buf[i] = TEST_STRING[from + i];
		}

		++nstrs;

		lua_pushlstring(L, buf, (size_t)i);
		lua_rawseti(L, -2, nstrs);

		from += i + 1;
	}

	return 1;
}

static int luaproc_TEST_STRING_LENGTH (lua_State * L)
{
	pthread_mutex_lock(&TEST_MUTEX);

	lua_pushinteger(L, TEST_STRING_LENGTH);

	pthread_mutex_unlock(&TEST_MUTEX);

	return 1;
}

static int GetOp (lua_State * L)
{
	const char * names[] = { "assign", "add", "sub", "and", "or", "xor", NULL };
	int op[] = { eUpdateAssign, eUpdateAdd, eUpdateSub, eUpdateAnd, eUpdateOr, eUpdateXor };

	return op[luaL_checkoption(L, 2, "assign", names)];
}

static int luaproc_UpdateInteger (lua_State * L)
{
	lua_Integer result;

	if (UpdateInteger(luaL_checkstring(L, 1), GetOp(L), luaL_checkinteger(L, 3), &result)) lua_pushinteger(L, result);	// name, opt, i, result

	else lua_pushnil(L);// name, op, i, nil

	return 1;
}

static int luaproc_UpdateNumber (lua_State * L)
{
	lua_Number result;

	if (UpdateNumber(luaL_checkstring(L, 1), GetOp(L), luaL_checknumber(L, 3), &result)) lua_pushnumber(L, result);	// name, opt, n, result

	else lua_pushnil(L);// name, op, n, nil

	return 1;
}

static int luaproc_WantsToClose (lua_State * L)
{
	lua_pushboolean(L, sched_wants_to_close(TRUE));

	return 1;
}

static int luaproc_CreateEvent (lua_State * L)
{
	if (!CreateEventX(L, lua_toboolean(L, 1), lua_toboolean(L, 2))) lua_pushnil(L);// [manual_reset[, initial_value]], name / nil
	
	return 1;
}

static int luaproc_DestroyEvent (lua_State * L)
{
	lua_pushboolean(L, DestroyEventX(luaL_checkstring(L, 1)) == eEventOK ? 1 : 0);	// name, ok

	return 1;
}

static int luaproc_SetEvent (lua_State * L)
{
	lua_pushboolean(L, SetEventX(luaL_checkstring(L, 1)) == eEventOK ? 1 : 0);	// name, ok

	return 1;
}

static int luaproc_ResetEvent (lua_State * L)
{
	lua_pushboolean(L, ResetEventX(luaL_checkstring(L, 1)) == eEventOK ? 1 : 0);// name, ok

	return 1;
}

static unsigned int GetMS (lua_State * L, int arg)
{
	if (lua_isnil(L, 2)) return -1;

	else
	{
		lua_Integer ms = luaL_checkinteger(L, 2);

		return ms >= 0 ? (unsigned int)ms : -1;
	}
}

static void PushWaitResult (lua_State * L, int res)
{
	if (res == eEventTimeout) lua_pushliteral(L, "timeout");// ..., "timeout"

	else lua_pushboolean(L, res == eEventOK ? 1 : 0);	// ..., ok
}

static int luaproc_WaitForEvent (lua_State * L)
{
	PushWaitResult(L, WaitForEventX(luaL_checkstring(L, 1), GetMS(L, 2)));	// name[, ms], "timeout" / ok

	return 1;
}

static const char ** GetNames (lua_State * L)
{
	static const char * names[9];
	const char ** pnames = names;
	int i, n = (int)lua_objlen(L, 1) + 1;

	if (n > 8) pnames = lua_newuserdata(L, sizeof(const char *) * n);	// names[, ms], nt

	for (i = 1; i < n; ++i, lua_pop(L, 1))
	{
		lua_rawgeti(L, 1, i);	// names[, ms][, nt], name

		pnames[i - 1] = luaL_checkstring(L, i);
	}

	pnames[n] = NULL;

	return pnames;
}

static int WaitForEvents (lua_State * L, int all)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	uint64_t wait = GetMS(L, 2);
	const char ** names = GetNames(L);	// names[, ms][, nt]
	int index, res = WaitForMultipleEventsX(names, wait, all, &index);

	PushWaitResult(L, res);	// names[, ms][, nt], "timeout" / ok

	if (res == eEventOK) lua_rawgeti(L, 1, index + 1);	// names[, ms][, nt], "timeout" / ok, name

	return res == eEventOK ? 2 : 1;
}

static int luaproc_WaitForAllEvents (lua_State * L)
{
	return WaitForEvents(L, TRUE);
}

static int luaproc_WaitForAnyEvents (lua_State * L)
{
	return WaitForEvents(L, FALSE);
}
// /STEVE CHANGE

/* luaproc function registration array */
static const struct luaL_Reg luaproc_funcs[] = {
	{ "newproc", luaproc_create_newproc },
	{ "wait", luaproc_wait },
	{ "send", luaproc_send },
	{ "receive", luaproc_receive },
	{ "newchannel", luaproc_create_channel },
	{ "delchannel", luaproc_destroy_channel },
	{ "setnumworkers", luaproc_set_numworkers },
	{ "getnumworkers", luaproc_get_numworkers },
	{ "recycle", luaproc_recycle_set },
// STEVE CHANGE
	{ "create_event", luaproc_CreateEvent },
	{ "create_integer", luaproc_CreateInteger },
	{ "create_number", luaproc_CreateNumber },
	{ "destroy_event", luaproc_DestroyEvent },
	{ "destroy_integer", luaproc_DestroyInteger },
	{ "destroy_number", luaproc_DestroyNumber },
	{ "estimate_concurrency", luaproc_EstimateConcurrency },
	{ "get_alert_dispatcher", luaproc_GetAlertDispatcher },
	{ "get_integer", luaproc_GetInteger },
	{ "get_number", luaproc_GetNumber },
	{ "get_phase", luaproc_GetPhase },
	{ "is_main_state", luaproc_IsMainState },
	{ "is_waiting", luaproc_IsWaiting },
	{ "preload", luaproc_Preload },
	{ "receive_allow_from_main", luaproc_receive_allow_from_main },
	{ "reset_event", luaproc_ResetEvent },
	{ "send_allow_from_main", luaproc_send_allow_from_main },
	{ "set_event", luaproc_SetEvent },
	{ "sleep", luaproc_Sleep },
	{ "TEST_APPEND_STRING", luaproc_TEST_APPEND_STRING },
	{ "TEST_GET_RANDOM_INTEGER", luaproc_TEST_GET_RANDOM_INTEGER },
	{ "TEST_PRINT_FROM_TO", luaproc_TEST_PRINT_FROM_TO },
	{ "TEST_PUSH_RANDOM_INTEGER", luaproc_TEST_PUSH_RANDOM_INTEGER },
	{ "TEST_STRING_LENGTH", luaproc_TEST_STRING_LENGTH },
	{ "update_integer", luaproc_UpdateInteger },
	{ "update_number", luaproc_UpdateNumber },
	{ "wait_for_all_events", luaproc_WaitForAllEvents },
	{ "wait_for_any_events", luaproc_WaitForAnyEvents },
	{ "wait_for_event", luaproc_WaitForEvent },
	{ "wants_to_close", luaproc_WantsToClose },
// /STEVE CHANGE
	{ NULL, NULL }
};

/******************
* list functions *
******************/

/* insert a lua process in a (fifo) list */
void list_insert(list *l, luaproc *lp) {
	if (l->head == NULL) {
		l->head = lp;
	}
	else {
		l->tail->next = lp;
	}
	l->tail = lp;
	lp->next = NULL;
	l->nodes++;
}

/* remove and return the first lua process in a (fifo) list */
luaproc *list_remove(list *l) {
	if (l->head != NULL) {
		luaproc *lp = l->head;
		l->head = lp->next;
		l->nodes--;
		return lp;
	}
	else {
		return NULL; /* if list is empty, return NULL */
	}
}

/* return a list's node count */
int list_count(list *l) {
	return l->nodes;
}

/* initialize an empty list */
void list_init(list *l) {
	l->head = NULL;
	l->tail = NULL;
	l->nodes = 0;
}

/*********************
* channel functions *
*********************/

/* create a new channel and insert it into channels table */
static channel *channel_create(const char *cname) {

	channel *chan;

	/* get exclusive access to channels list */
	pthread_mutex_lock(&mutex_channel_list);

	/* create new channel and register its name */
	lua_getglobal(chanls, LUAPROC_CHANNELS_TABLE);
	chan = (channel *)lua_newuserdata(chanls, sizeof(channel));
	lua_setfield(chanls, -2, cname);
	lua_pop(chanls, 1);  /* remove channel table from stack */

	/* initialize channel struct */
	list_init(&chan->send);
	list_init(&chan->recv);
	pthread_mutex_init(&chan->mutex, NULL);
	pthread_cond_init(&chan->can_be_used, NULL);

	/* release exclusive access to channels list */
	pthread_mutex_unlock(&mutex_channel_list);

	return chan;
}

/*
return a channel (if not found, return null).
caller function MUST lock 'mutex_channel_list' before calling this function.
*/
static channel *channel_unlocked_get(const char *chname) {

	channel *chan;

	lua_getglobal(chanls, LUAPROC_CHANNELS_TABLE);
	lua_getfield(chanls, -1, chname);
	chan = (channel *)lua_touserdata(chanls, -1);
	lua_pop(chanls, 2);  /* pop userdata and channel */

	return chan;
}

/*
return a channel (if not found, return null) with its (mutex) lock set.
caller function should unlock channel's (mutex) lock after calling this
function.
*/
static channel *channel_locked_get(const char *chname) {

	channel *chan;

	/* get exclusive access to channels list */
	pthread_mutex_lock(&mutex_channel_list);

	/*
	try to get channel and lock it; if lock fails, release external
	lock ('mutex_channel_list') to try again when signaled -- this avoids
	keeping the external lock busy for too long. during the release,
	the channel may be destroyed, so it must try to get it again.
	*/
	while (((chan = channel_unlocked_get(chname)) != NULL) &&
		(pthread_mutex_trylock(&chan->mutex) != 0)) {
		pthread_cond_wait(&chan->can_be_used, &mutex_channel_list);
	}

	/* release exclusive access to channels list */
	pthread_mutex_unlock(&mutex_channel_list);

	return chan;
}

/********************************
* exported auxiliary functions *
********************************/

/* unlock access to a channel and signal it can be used */
void luaproc_unlock_channel(channel *chan) {

	/* get exclusive access to channels list */
	pthread_mutex_lock(&mutex_channel_list);
	/* release exclusive access to operate on a particular channel */
	pthread_mutex_unlock(&chan->mutex);
	/* signal that a particular channel can be used */
	pthread_cond_signal(&chan->can_be_used);
	/* release exclusive access to channels list */
	pthread_mutex_unlock(&mutex_channel_list);

}

/* insert lua process in recycle list */
void luaproc_recycle_insert(luaproc *lp) {

	/* get exclusive access to recycled lua processes list */
	pthread_mutex_lock(&mutex_recycle_list);

	/* is recycle list full? */
	if (list_count(&recycle_list) >= recyclemax) {
		/* destroy state */
		lua_close(luaproc_get_state(lp));
	}
	else {
		/* insert lua process in recycle list */
		list_insert(&recycle_list, lp);
	}

	/* release exclusive access to recycled lua processes list */
	pthread_mutex_unlock(&mutex_recycle_list);
}

/* queue a lua process that tried to send a message */
void luaproc_queue_sender(luaproc *lp) {
	list_insert(&lp->chan->send, lp);
}

/* queue a lua process that tried to receive a message */
void luaproc_queue_receiver(luaproc *lp) {
	list_insert(&lp->chan->recv, lp);
}

/********************************
* internal auxiliary functions *
********************************/
static void luaproc_loadbuffer(lua_State *parent, luaproc *lp,
	const char *code, size_t len) {

	/* load lua process' lua code */
	int ret = luaL_loadbuffer(lp->lstate, code, len, code);

	/* in case of errors, close lua_State and push error to parent */
	if (ret != 0) {
		lua_pushstring(parent, lua_tostring(lp->lstate, -1));
		lua_close(lp->lstate);
		luaL_error(parent, lua_tostring(parent, -1));
	}
}

/* copies values between lua states' stacks */
static int luaproc_copyvalues(lua_State *Lfrom, lua_State *Lto) {

	int i;
	int n = lua_gettop(Lfrom);
	const char *str;
	size_t len;

	/* ensure there is space in the receiver's stack */
	if (lua_checkstack(Lto, n) == 0) {
		lua_pushnil(Lto);
		lua_pushstring(Lto, "not enough space in the stack");
		lua_pushnil(Lfrom);
		lua_pushstring(Lfrom, "not enough space in the receiver's stack");
		return FALSE;
	}

	/* test each value's type and, if it's supported, copy value */
	for (i = 2; i <= n; i++) {
		switch (lua_type(Lfrom, i)) {
		case LUA_TBOOLEAN:
			lua_pushboolean(Lto, lua_toboolean(Lfrom, i));
			break;
		case LUA_TNUMBER:
			copynumber(Lto, Lfrom, i);
			break;
		case LUA_TSTRING: {
			str = lua_tolstring(Lfrom, i, &len);
			lua_pushlstring(Lto, str, len);
			break;
		}
		case LUA_TNIL:
			lua_pushnil(Lto);
			break;
		default: /* value type not supported: table, function, userdata, etc. */
			lua_settop(Lto, 1);
			lua_pushnil(Lto);
			lua_pushfstring(Lto, "failed to receive value of unsupported type "
				"'%s'", luaL_typename(Lfrom, i));
			lua_pushnil(Lfrom);
			lua_pushfstring(Lfrom, "failed to send value of unsupported type "
				"'%s'", luaL_typename(Lfrom, i));
			return FALSE;
		}
	}
	return TRUE;
}

/* return the lua process associated with a given lua state */
static luaproc *luaproc_getself(lua_State *L) {

	luaproc *lp;

	lua_getfield(L, LUA_REGISTRYINDEX, "LUAPROC_LP_UDATA");
	lp = (luaproc *)lua_touserdata(L, -1);
	lua_pop(L, 1);

	return lp;
}

/* create new lua process */
static luaproc *luaproc_new(lua_State *L) {

	luaproc *lp;
	lua_State *lpst = luaL_newstate();  /* create new lua state */

	/* store the lua process in its own lua state */
	lp = (luaproc *)lua_newuserdata(lpst, sizeof(struct stluaproc));
	lua_setfield(lpst, LUA_REGISTRYINDEX, "LUAPROC_LP_UDATA");
	luaproc_openlualibs(lpst);  /* load standard libraries and luaproc */
	/* register luaproc's own functions */
	requiref(lpst, "luaproc", luaproc_loadlib, TRUE);
	lp->lstate = lpst;  /* insert created lua state into lua process struct */

	return lp;
}

/* join schedule workers (called before exiting Lua) */
static int luaproc_join_workers(lua_State *L) {
	sched_join_workers(L); // <- STEVE CHANGE
	lua_close(chanls);
// STEVE CHANGE
    TEST_STRING_LENGTH = TEST_RANDOM_RPOS = TEST_RANDOM_WPOS = 0;
	ExtDestructors();
	ProcDestructors();
	SchedDestructors();
// /STEVE CHANGE
	return 0;
}

/* writer function for lua_dump */
static int luaproc_buff_writer(lua_State *L, const void *buff, size_t size,
	void *ud) {
	(void)L;
	luaL_addlstring((luaL_Buffer *)ud, (const char *)buff, size);
	return 0;
}

/* copies upvalues between lua states' stacks */
static int luaproc_copyupvalues(lua_State *Lfrom, lua_State *Lto,
	int funcindex) {

	int i = 1;
	const char *str;
	size_t len;

	/* test the type of each upvalue and, if it's supported, copy it */
	while (lua_getupvalue(Lfrom, funcindex, i) != NULL) {
		switch (lua_type(Lfrom, -1)) {
		case LUA_TBOOLEAN:
			lua_pushboolean(Lto, lua_toboolean(Lfrom, -1));
			break;
		case LUA_TNUMBER:
			copynumber(Lto, Lfrom, -1);
			break;
		case LUA_TSTRING: {
			str = lua_tolstring(Lfrom, -1, &len);
			lua_pushlstring(Lto, str, len);
			break;
		}
		case LUA_TNIL:
			lua_pushnil(Lto);
			break;
			/* if upvalue is a table, check whether it is the global environment
			(_ENV) from the source state Lfrom. in case so, push in the stack of
			the destination state Lto its own global environment to be set as the
			corresponding upvalue; otherwise, treat it as a regular non-supported
			upvalue type. */
			// STEVE CHANGE: do likewise with the luaproc table
		case LUA_TTABLE:
			lua_pushglobaltable(Lfrom);
			if (isequal(Lfrom, -1, -2)) {
				lua_pop(Lfrom, 1);
				lua_pushglobaltable(Lto);
				break;
			}
			// STEVE CHANGE
//			lua_pop(Lfrom, 1);
			{
				const char * names[] = { "package", "loaded", Lfrom == mainlp.lstate ? "plugin_luaproc" : "luaproc", NULL };
				int j, top = lua_gettop(Lfrom) - 1;// remove the global table plus whatever we add

				for (j = 0; names[j] && lua_istable(Lfrom, -1); ++j) lua_getfield(Lfrom, -1, names[j]); // ..., luaproc?, _G[, package[, package.loaded[, luaproc]]]

				if (isequal(Lfrom, -1, top))
				{
					lua_settop(Lfrom, top);	// ..., luaproc?
					lua_pushglobaltable(Lto);	// ..., _G
					lua_getfield(Lto, -1, "luaproc");	// ..., G, luaproc
					lua_replace(Lto, -2);	// ..., luaproc

					break;
				}

				lua_settop(Lfrom, top);	// ..., luaproc?
			}
			// /STEVE CHANGE
			/* FALLTHROUGH */
		default: /* value type not supported: table, function, userdata, etc. */
			lua_pushnil(Lfrom);
			lua_pushfstring(Lfrom, "failed to copy upvalue of unsupported type "
				"'%s'", luaL_typename(Lfrom, -2));
			return FALSE;
		}
		lua_pop(Lfrom, 1);
		if (lua_setupvalue(Lto, 1, i) == NULL) {
			lua_pushnil(Lfrom);
			lua_pushstring(Lfrom, "failed to set upvalue");
			return FALSE;
		}
		i++;
	}

	return TRUE;
}

// STEVE CHANGE
typedef struct CustomPreloadValue {
	int mType;
	union {
		int mB;
		lua_Number mN;
		const char * mS;
	} mValue;
} CustomPreloadValue;

typedef struct CustomPreload {
	struct CustomPreload * mNext;
	const char * mName;
	int mUpvalueCount;	// -1 = cfunc, 0+ = Lua
	size_t mLen;
	union {
		lua_CFunction mC;
		const char * mLua;
	} mFunc;
	CustomPreloadValue mValues[1];
} CustomPreload;

static pthread_mutex_t mutex_preload = PTHREAD_MUTEX_INITIALIZER;

static CustomPreload * sPreloadChain;

static void PreloadDestructor (void)
{
/*
	pthread_mutex_destroy(&mutex_preload);

	mutex_preload = PTHREAD_MUTEX_INITIALIZER;
    */
    sPreloadChain = NULL;
}

static int CheckUpvalues (lua_State * L, int * pos)
{
	int count = 0;

	if (lua_isfunction(L, 2))
	{
		for (; lua_getupvalue(L, 2, count + 1) != NULL; ++count)	// name, func, t, upvalue
		{
			switch (lua_type(L, -1))
			{
			case LUA_TBOOLEAN:
			case LUA_TNIL:
			case LUA_TNUMBER:
				lua_pop(L, 1);	// name, func, t

				break;
			case LUA_TSTRING:
				lua_rawseti(L, -2, ++*pos);	// name, func, t = { name, ..., str #i }

				break;
			default: /* value type not supported: table, function, userdata, etc. */
				lua_pushfstring(L, "upvalue of unsupported type '%s'", luaL_typename(L, -1));

				return -1;
			}
		}
	}

	return count;
}

static void GatherUpvalues (lua_State * L, CustomPreload * entry)
{
	int i;
	
	if (!lua_isfunction(L, 2)) return;

	for (i = 0; lua_getupvalue(L, 2, i + 1) != NULL; ++i)	// name, func, t, upvalue
	{
		entry->mValues[i].mType = lua_type(L, -1);

		switch (entry->mValues[i].mType)
		{
		case LUA_TBOOLEAN:
			entry->mValues[i].mValue.mB = lua_toboolean(L, -1);

			break;
		case LUA_TNIL:
			break;
		case LUA_TNUMBER:
			entry->mValues[i].mValue.mN = lua_tonumber(L, -1);

			break;
		case LUA_TSTRING:
			entry->mValues[i].mValue.mS = lua_tostring(L, -1);

			break;
		default:
			luaL_error(L, "Invalid type!");
		}

		lua_pop(L, 1);	// name, func, t
	}
}

static int sDeferredRef = LUA_NOREF, sLoadlibRef = LUA_NOREF;

static void LoadlibDestructor (void)
{
	sDeferredRef = sLoadlibRef = LUA_NOREF;
}

static int luaproc_Preload (lua_State * L)
{
	const char * name = luaL_checkstring(L, 1), * path;
	CustomPreload * entry, * cur;
	int count, pos = 0, is_function = lua_isfunction(L, 2);
	void ** ptr = NULL;
	
	if (L != mainlp.lstate) luaL_error(L, "preload() called outside main state");

	luaL_argcheck(L, is_function || lua_isnoneornil(L, 2), 2, "non-nil argument to preload()");

	// Anchor any strings while counting and verifying upvalues. Allocate and configure a
	// preload entry that can either store those values or hold the string pointers.
	lua_createtable(L, 2, 0);	// name[, func], t
	lua_pushvalue(L, 1);// name[, func], t, name
	lua_rawseti(L, -2, ++pos);	// name[, func], t = { name }

	count = CheckUpvalues(L, &pos);

	if (count < 0) lua_error(L);

	entry = (CustomPreload *)lua_newuserdata(L, sizeof(CustomPreload) + (count - 1) * sizeof(CustomPreloadValue));	// name, func, t, entry

	lua_rawseti(L, -2, pos + 1);// name[, func], t = { name, ..., str #i, ..., entry }

	entry->mName = name;

	// In the case of a Lua function, serialize it much like a proc.
	if (is_function && !lua_iscfunction(L, 2))
	{
		luaL_Buffer buff;
		int d;

		lua_pushvalue(L, 2);// name, func, t, func
		luaL_buffinit(L, &buff);

		d = dump(L, luaproc_buff_writer, &buff, FALSE);

		if (d != 0) luaL_error(L, "error %d dumping function to binary string", d);

		luaL_pushresult(&buff);	// name, func, t, func, code
		lua_replace(L, -2);	// name, func, t, code

		entry->mUpvalueCount = count;
		entry->mFunc.mLua = lua_tolstring(L, -1, &entry->mLen);

		lua_rawseti(L, -2, pos + 2);// name, func, t = { name, ..., str #n, entry, code }
	}

	// Otherwise, we want a C function. We either have a C reloader or will be reusing the
	// native plugin's entry point. In both these cases the plugin must have already been
    // loaded in the main thread, so search for the library reference and full path.
	else
	{
		size_t loop, found = 0U, len = lua_objlen(L, 1);

        lua_getglobal(L, "package");// name[, loader], package?
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_getfield(L, -1, "loaded");  // name[, loader], package, package.loaded
        luaL_checktype(L, -1, LUA_TTABLE);

		for (loop = 0; loop < 2 && !found; ++loop, lua_pop(L, 1))
		{
			lua_pushstring(L, loop == 0 ? "plugin." : "plugin_");  // name[, loader], package, package.loaded, "plugin?"
			lua_pushvalue(L, 1);// name[, loader], package, package.loaded, "plugin?", name
			lua_concat(L, 2);   // name[, loader], package, package.loaded, "plugin?" .. name
			lua_rawget(L, -2);  // name[, loader], package, package.loaded, module?

			found = !lua_isnil(L, -1);
		}

        luaL_argcheck(L, found, -1, "Attempting to preload not-yet-loaded native module");
        lua_pop(L, 2);  // name[, loader]
        
		entry->mUpvalueCount = -count - 1;	// The count is later used to distinguish C functions:
											// -1 -> no upvalues, -2 -> 1 upvalue, -3 -> 2 upvalues, etc.
		
		if (is_function) entry->mFunc.mC = lua_tocfunction(L, 2);

        // TODO: Is this a no-op on iOS and tvOS? (And is deferring unnecessary there?)

		for (lua_pushnil(L); lua_next(L, LUA_REGISTRYINDEX); lua_pop(L, 1))
		{
			if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TUSERDATA)
			{
				const char * key = lua_tostring(L, -2);

				if (strncmp(key, "LOADLIB: ", sizeof("LOADLIB: ") - 1) == 0)
				{
					const char * pre_ext = strrchr(key, '.'), * sep = pre_ext;

					if (!sep) continue;

					do {
						--sep;
					} while (sep != key && *sep != '\\' && *sep != '/' && *sep != '_');

					if (sep == key || pre_ext - (sep + 1) != len) continue;

                    #ifdef __ANDROID__
                        #define PLUGIN_PREFIX "libplugin"
                    #else
                        #define PLUGIN_PREFIX "plugin"
                    #endif
                    
                    
					if (strncmp(sep - sizeof(PLUGIN_PREFIX) + 1, PLUGIN_PREFIX, sizeof(PLUGIN_PREFIX) - 1) == 0 && strncmp(sep + 1, name, len) == 0)
					{
						path = key + sizeof("LOADLIB: ") - 1;
						ptr = (void **)lua_touserdata(L, -1);

						break;
					}
                    
                    #undef PLUGIN_PREFIX
				}
			}
		}
	}

	// Accumulate any upvalues into the preload entry.
	GatherUpvalues(L, entry);

	// Search for any existing preload entry with the requested name. If none exists, link
	// the new entry into the chain.
	pthread_mutex_lock(&mutex_preload);

    cur = sPreloadChain;
    
    pthread_mutex_unlock(&mutex_preload);

    while (cur && cur->mName != name) cur = cur->mNext;

	if (!cur)
	{
        pthread_mutex_lock(&mutex_preload);
        
        entry->mNext = sPreloadChain;
		sPreloadChain = entry;
        
        pthread_mutex_unlock(&mutex_preload);
    }

	// Raise an error if the name was already in used. Put our anchored strings into the
	// registry to make sure they persist during lua_close() until luaproc has run its
	// course. Along the same lines, move the library reference (if the module is native)
	// into the deferred list and dummy the current box out.
	if (!cur)
	{
		lua_pushboolean(L, TRUE);	// name[, func], t, true
		lua_rawset(L, LUA_REGISTRYINDEX);	// name[, func]; registry = { ..., [t] = true }

		if (ptr)
		{
			lua_getref(L, sDeferredRef);// name[, func], deferred
			void ** ud = (void **)lua_newuserdata(L, sizeof(const void *));	// name[, func], deferred, ud

			lua_rawseti(L, -2, lua_objlen(L, -2) + 1);	// name[, func], deferred = { ..., ud }
			lua_pop(L, 1);	// name[, func]

			*ud = *ptr;
			*ptr = NULL;

			// If we are reusing the plugin entry point, find it again and save it. If we got
			// this far, the module was presumably already loaded, so this should be safe.
			if (!is_function)
			{
				lua_getref(L, sLoadlibRef);	// name, package.loadlib
				lua_pushstring(L, path);// name, package.loadlib, path
				lua_pushliteral(L, "luaopen_plugin_");	// name, package.loadlib, path, prefix
				lua_pushvalue(L, 1);// name, package.loadlib, path, prefix, name
				lua_concat(L, 2);	// name, package.loadlib, path, prefix .. name
				lua_call(L, 2, 1);	// name, func

				entry->mFunc.mC = lua_tocfunction(L, -1);
			}
		}
	}

	else luaL_error(L, "%s already present in loader chain", name);

	return 0;
}

static int CustomPreload_Loader (lua_State * L)
{
	const char * name = luaL_checkstring(L, 1);
	CustomPreload * cur;
	int found = FALSE;

	// Find the entry with the requested name (using lua_equal() rather than comparing
	// pointers because this is called from an arbitrary Lua state). Search is performed
	// under the assumption that the module has been preloaded, i.e. it will not pick it
	// up if it happens to be added to the chain along the way.
	pthread_mutex_lock(&mutex_preload);

	cur = sPreloadChain;

	pthread_mutex_unlock(&mutex_preload);

	while (cur)
	{
		lua_pushstring(L, cur->mName);	// name, cur_name

		found = lua_equal(L, 1, -1);

		lua_pop(L, 1);	// name

		if (found) break;

		else cur = cur->mNext;
	}

	if (!found) lua_pushfstring(L, "No loader found for %s\n", name);

	else
	{
		int i, count = cur->mUpvalueCount;

		// Load the module launcher, preparing for any upvalues.
		if (cur->mUpvalueCount < 0)
		{
			count = -count - 1;

			for (i = 0; i < count; ++i) lua_pushnil(L);	// name, ..., nil, ...

			lua_pushcclosure(L, cur->mFunc.mC, count);	// name, func
		}

		else 
		{
			if (luaL_loadbuffer(L, cur->mFunc.mLua, cur->mLen, cur->mFunc.mLua) != 0) lua_error(L);	// name, func / err

		//	AssignDummyUpvalues(L, -1, count);
		}

		// Bind any upvalues.
		for (i = 0; i < count; ++i)
		{
			switch (cur->mValues[i].mType)
			{
			case LUA_TBOOLEAN:
				lua_pushboolean(L, cur->mValues[i].mValue.mB);	// name, func, b

				break;
			case LUA_TNIL:
				lua_pushnil(L);	// name, func, nil

				break;
			case LUA_TNUMBER:
				lua_pushnumber(L, cur->mValues[i].mValue.mN);	// name, func, n

				break;
			case LUA_TSTRING:
				lua_pushstring(L, cur->mValues[i].mValue.mS);	// name, func, s

				break;
			default:
				luaL_error(L, "Invalid upvalue");
			}

			lua_setupvalue(L, -2, i + 1);	// name, func
		}
	}

	return 1;
}
// /STEVE CHANGE

/*********************
* library functions *
*********************/

/* set maximum number of lua processes in the recycle list */
static int luaproc_recycle_set(lua_State *L) {

	luaproc *lp;

	/* validate parameter is a non negative number */
	lua_Integer max = luaL_checkinteger(L, 1);
	luaL_argcheck(L, max >= 0, 1, "recycle limit must be positive");

	/* get exclusive access to recycled lua processes list */
	pthread_mutex_lock(&mutex_recycle_list);

	recyclemax = max;  /* set maximum number */

	/* remove extra nodes and destroy each lua processes */
	while (list_count(&recycle_list) > recyclemax) {
		lp = list_remove(&recycle_list);
		lua_close(lp->lstate);
	}
	/* release exclusive access to recycled lua processes list */
	pthread_mutex_unlock(&mutex_recycle_list);

	return 0;
}

/* wait until there are no more active lua processes */
static int luaproc_wait(lua_State *L) {
	sched_wait();
	return 0;
}

/* set number of workers (creates or destroys accordingly) */
static int luaproc_set_numworkers(lua_State *L) {

	/* validate parameter is a positive number */
	lua_Integer numworkers = luaL_checkinteger(L, -1);
	luaL_argcheck(L, numworkers > 0, 1, "number of workers must be positive");

	/* set number of threads; signal error on failure */
	if (sched_set_numworkers(numworkers) == LUAPROC_SCHED_PTHREAD_ERROR) {
		luaL_error(L, "failed to create worker");
	}

	return 0;
}

/* return the number of active workers */
static int luaproc_get_numworkers(lua_State *L) {
	lua_pushnumber(L, sched_get_numworkers());
	return 1;
}

/* create and schedule a new lua process */
static int luaproc_create_newproc(lua_State *L) {

	size_t len;
	luaproc *lp;
	luaL_Buffer buff;
	const char *code;
	int d;
	int lt = lua_type(L, 1);

	/* check function argument type - must be function or string; in case it is
	a function, dump it into a binary string */
	if (lt == LUA_TFUNCTION) {
		lua_settop(L, 1);
		luaL_buffinit(L, &buff);
		d = dump(L, luaproc_buff_writer, &buff, FALSE);
		if (d != 0) {
			lua_pushnil(L);
			lua_pushfstring(L, "error %d dumping function to binary string", d);
			return 2;
		}
		luaL_pushresult(&buff);
		lua_insert(L, 1);
	}
	else if (lt != LUA_TSTRING) {
		lua_pushnil(L);
		lua_pushfstring(L, "cannot use '%s' to create a new process",
			luaL_typename(L, 1));
		return 2;
	}

	/* get pointer to code string */
	code = lua_tolstring(L, 1, &len);

	/* get exclusive access to recycled lua processes list */
	pthread_mutex_lock(&mutex_recycle_list);

	/* check if a lua process can be recycled */
	if (recyclemax > 0) {
		lp = list_remove(&recycle_list);
		/* otherwise create a new lua process */
		if (lp == NULL) {
			lp = luaproc_new(L);
		}
	}
	else {
		lp = luaproc_new(L);
	}

	/* release exclusive access to recycled lua processes list */
	pthread_mutex_unlock(&mutex_recycle_list);

	/* init lua process */
	lp->status = LUAPROC_STATUS_IDLE;
	lp->args = 0;
	lp->chan = NULL;

	/* load code in lua process */
	luaproc_loadbuffer(L, lp, code, len);

	/* if lua process is being created from a function, copy its upvalues and
	remove dumped binary string from stack */
	if (lt == LUA_TFUNCTION) {
		// STEVE CHANGE
	//	AssignDummyUpvalues(lp->lstate, -1, CountUpvalues(L, 2));
		// /STEVE CHANGE
		if (luaproc_copyupvalues(L, lp->lstate, 2) == FALSE) {
			luaproc_recycle_insert(lp);
			return 2;
		}
		lua_pop(L, 1);
	}

	sched_inc_lpcount();   /* increase active lua process count */
	sched_queue_proc(lp);  /* schedule lua process for execution */
	lua_pushboolean(L, TRUE);

	return 1;
}

/* send a message to a lua process */
static int luaproc_send_aux(lua_State *L, int allow_send_from_main_state) {

	int ret;
	channel *chan;
	luaproc *dstlp, *self;
	const char *chname = luaL_checkstring(L, 1);

	chan = channel_locked_get(chname);
	/* if channel is not found, return an error to lua */
	if (chan == NULL) {
		lua_pushnil(L);
		lua_pushfstring(L, "channel '%s' does not exist", chname);
		return 2;
	}

	/* remove first lua process, if any, from channel's receive list */
	dstlp = list_remove(&chan->recv);

	if (dstlp != NULL) { /* found a receiver? */
		/* try to move values between lua states' stacks */
		ret = luaproc_copyvalues(L, dstlp->lstate);
		/* -1 because channel name is on the stack */
		dstlp->args = lua_gettop(dstlp->lstate) - 1;
		if (dstlp->lstate == mainlp.lstate) {
			/* if sending process is the parent (main) Lua state, unblock it */
			pthread_mutex_lock(&mutex_mainls);
			pthread_cond_signal(&cond_mainls_sendrecv);
			pthread_mutex_unlock(&mutex_mainls);
		}
		else {
			/* schedule receiving lua process for execution */
			sched_queue_proc(dstlp);
		}
		/* unlock channel access */
		luaproc_unlock_channel(chan);
		if (ret == TRUE) { /* was send successful? */
			lua_pushboolean(L, TRUE);
			return 1;
		}
		else { /* nil and error msg already in stack */
			return 2;
		}

	}
	else {
		if (L == mainlp.lstate) {
// STEVE CHANGE
	if (!allow_send_from_main_state)
	{
		lua_pushnil(L);
		lua_pushliteral(L, "Sending from main state in Corona requires explicit permission");

		return 2;
	}
// /STEVE CHANGE
			/* sending process is the parent (main) Lua state - block it */
			mainlp.chan = chan;
			luaproc_queue_sender(&mainlp);
			luaproc_unlock_channel(chan);
			pthread_mutex_lock(&mutex_mainls);
			pthread_cond_wait(&cond_mainls_sendrecv, &mutex_mainls);
			pthread_mutex_unlock(&mutex_mainls);
			return mainlp.args;
		}
		else {
			/* sending process is a standard luaproc - set status, block and yield */
			self = luaproc_getself(L);
			if (self != NULL) {
				self->status = LUAPROC_STATUS_BLOCKED_SEND;
				self->chan = chan;
			}
			/* yield. channel will be unlocked by the scheduler */
			return lua_yield(L, lua_gettop(L));
		}
	}
}

static int luaproc_send (lua_State * L)
{
    return luaproc_send_aux(L, 0);
}

static int luaproc_send_allow_from_main (lua_State * L)
{
    return luaproc_send_aux(L, 1);
}

/* receive a message from a lua process */
static int luaproc_receive_aux(lua_State *L, int allow_receive_from_main_state) {

	int ret, nargs;
	channel *chan;
	luaproc *srclp, *self;
	const char *chname = luaL_checkstring(L, 1);

	/* get number of arguments passed to function */
	nargs = lua_gettop(L);

	chan = channel_locked_get(chname);
	/* if channel is not found, return an error to Lua */
	if (chan == NULL) {
		lua_pushnil(L);
		lua_pushfstring(L, "channel '%s' does not exist", chname);
		return 2;
	}

	/* remove first lua process, if any, from channels' send list */
	srclp = list_remove(&chan->send);

	if (srclp != NULL) {  /* found a sender? */
		/* try to move values between lua states' stacks */
		ret = luaproc_copyvalues(srclp->lstate, L);
		if (ret == TRUE) { /* was receive successful? */
			lua_pushboolean(srclp->lstate, TRUE);
			srclp->args = 1;
		}
		else {  /* nil and error_msg already in stack */
			srclp->args = 2;
		}
		if (srclp->lstate == mainlp.lstate) {
			/* if sending process is the parent (main) Lua state, unblock it */
			pthread_mutex_lock(&mutex_mainls);
			pthread_cond_signal(&cond_mainls_sendrecv);
			pthread_mutex_unlock(&mutex_mainls);
		}
		else {
			/* otherwise, schedule process for execution */
			sched_queue_proc(srclp);
		}
		/* unlock channel access */
		luaproc_unlock_channel(chan);
		/* disconsider channel name, async flag and any other args passed
		to the receive function when returning its results */
		return lua_gettop(L) - nargs;

	}
	else {  /* otherwise test if receive was synchronous or asynchronous */
		if (lua_toboolean(L, 2)) { /* asynchronous receive */
			/* unlock channel access */
			luaproc_unlock_channel(chan);
			/* return an error */
			lua_pushnil(L);
			lua_pushfstring(L, "no senders waiting on channel '%s'", chname);
			return 2;
		}
		else { /* synchronous receive */
			if (L == mainlp.lstate) {
// STEVE CHANGE
	if (!allow_receive_from_main_state)
	{
		lua_pushnil(L);
		lua_pushliteral(L, "Receiving synchronously from main state in Corona requires explicit permission");

		return 2;
	}
// /STEVE CHANGE
				/*  receiving process is the parent (main) Lua state - block it */
				mainlp.chan = chan;
				luaproc_queue_receiver(&mainlp);
				luaproc_unlock_channel(chan);
				pthread_mutex_lock(&mutex_mainls);
				pthread_cond_wait(&cond_mainls_sendrecv, &mutex_mainls);
				pthread_mutex_unlock(&mutex_mainls);
				return mainlp.args;
			}
			else {
				/* receiving process is a standard luaproc - set status, block and
				yield */
				self = luaproc_getself(L);
				if (self != NULL) {
					self->status = LUAPROC_STATUS_BLOCKED_RECV;
					self->chan = chan;
				}
				/* yield. channel will be unlocked by the scheduler */
				return lua_yield(L, lua_gettop(L));
			}
		}
	}
}

static int luaproc_receive (lua_State * L)
{
    return luaproc_receive_aux(L, 0);
}

static int luaproc_receive_allow_from_main (lua_State * L)
{
    return luaproc_receive_aux(L, 1);
}

/* create a new channel */
static int luaproc_create_channel(lua_State *L) {

	const char *chname = luaL_checkstring(L, 1);

	channel *chan = channel_locked_get(chname);
	if (chan != NULL) {  /* does channel exist? */
		/* unlock the channel mutex locked by channel_locked_get */
		luaproc_unlock_channel(chan);
		/* return an error to lua */
		lua_pushnil(L);
		lua_pushfstring(L, "channel '%s' already exists", chname);
		return 2;
	}
	else {  /* create channel */
		channel_create(chname);
		lua_pushboolean(L, TRUE);
		return 1;
	}
}

/* destroy a channel */
static int luaproc_destroy_channel(lua_State *L) {

	channel *chan;
	list *blockedlp;
	luaproc *lp;
	const char *chname = luaL_checkstring(L, 1);

	/* get exclusive access to channels list */
	pthread_mutex_lock(&mutex_channel_list);

	/*
	try to get channel and lock it; if lock fails, release external
	lock ('mutex_channel_list') to try again when signaled -- this avoids
	keeping the external lock busy for too long. during this release,
	the channel may have been destroyed, so it must try to get it again.
	*/
	while (((chan = channel_unlocked_get(chname)) != NULL) &&
		(pthread_mutex_trylock(&chan->mutex) != 0)) {
		pthread_cond_wait(&chan->can_be_used, &mutex_channel_list);
	}

	if (chan == NULL) {  /* found channel? */
		/* release exclusive access to channels list */
		pthread_mutex_unlock(&mutex_channel_list);
		/* return an error to lua */
		lua_pushnil(L);
		lua_pushfstring(L, "channel '%s' does not exist", chname);
		return 2;
	}

	/* remove channel from table */
	lua_getglobal(chanls, LUAPROC_CHANNELS_TABLE);
	lua_pushnil(chanls);
	lua_setfield(chanls, -2, chname);
	lua_pop(chanls, 1);

	pthread_mutex_unlock(&mutex_channel_list);

	/*
	wake up workers there are waiting to use the channel.
	they will not find the channel, since it was removed,
	and will not get this condition anymore.
	*/
	pthread_cond_broadcast(&chan->can_be_used);

	/*
	dequeue lua processes waiting on the channel, return an error message
	to each of them indicating channel was destroyed and schedule them
	for execution (unblock them).
	*/
	if (chan->send.head != NULL) {
		lua_pushfstring(L, "channel '%s' destroyed while waiting for receiver",
			chname);
		blockedlp = &chan->send;
	}
	else {
		lua_pushfstring(L, "channel '%s' destroyed while waiting for sender",
			chname);
		blockedlp = &chan->recv;
	}
	while ((lp = list_remove(blockedlp)) != NULL) {
		/* return an error to each process */
		lua_pushnil(lp->lstate);
		lua_pushstring(lp->lstate, lua_tostring(L, -1));
		lp->args = 2;
		sched_queue_proc(lp); /* schedule process for execution */
	}

	/* unlock channel mutex and destroy both mutex and condition */
	pthread_mutex_unlock(&chan->mutex);
	pthread_mutex_destroy(&chan->mutex);
	pthread_cond_destroy(&chan->can_be_used);

	lua_pushboolean(L, TRUE);
	return 1;
}

/***********************
* get'ers and set'ers *
***********************/

/* return the channel where a lua process is blocked at */
channel *luaproc_get_channel(luaproc *lp) {
	return lp->chan;
}

/* return a lua process' status */
int luaproc_get_status(luaproc *lp) {
	return lp->status;
}

/* set lua a process' status */
void luaproc_set_status(luaproc *lp, int status) {
	lp->status = status;
}

/* return a lua process' state */
lua_State *luaproc_get_state(luaproc *lp) {
	return lp->lstate;
}

/* return the number of arguments expected by a lua process */
int luaproc_get_numargs(luaproc *lp) {
	return lp->args;
}

/* set the number of arguments expected by a lua process */
void luaproc_set_numargs(luaproc *lp, int n) {
	lp->args = n;
}

/**********************************
* register structs and functions *
**********************************/

static void luaproc_reglualib(lua_State *L, const char *name,
	lua_CFunction f) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, f);
	lua_setfield(L, -2, name);
	lua_pop(L, 2);
}

// STEVE CHANGE
static int sLoadlibMetaRef = LUA_NOREF, sGCRef = LUA_NOREF;

static void MetaDestructor (void)
{
	sLoadlibMetaRef = sGCRef = LUA_NOREF;
}

void CommitUnloads (lua_State * L)
{
	int i, n;

	lua_getref(L, sLoadlibMetaRef);	// ..., mt
	lua_getref(L, sGCRef);	// ..., mt, gc
	lua_getref(L, sDeferredRef);// ..., mt, gc, deferred

	for (i = 1, n = lua_objlen(L, -1); i <= n; ++i)
	{
		lua_rawgeti(L, -1, i);	// ..., mt, gc, deferred, lib
		lua_pushvalue(L, -4);	// ..., mt, gc, deferred, lib, mt
		lua_setmetatable(L, -2);// ..., mt, gc, deferred, lib
		lua_pushvalue(L, -3);	// ..., mt, gc, deferred, lib, gc
		lua_pushvalue(L, -2);	// ..., mt, gc, deferred, lib, gc, lib
		lua_pcall(L, 1, 0, 0);	// ..., mt, gc, deferred, lib
		lua_pushnil(L);	// ..., mt, gc, deferred, lib, nil
		lua_setmetatable(L, -2);// ..., mt, gc, deferred, lib
		lua_pop(L, 1);	// ..., mt, gc, deferred
	}

	lua_pop(L, 3);	// ...
}

static int LateLoader (lua_State * L)
{
	const char ** names = (const char **)lua_touserdata(L, lua_upvalueindex(2));
	int i;

	for (i = 0; names[i]; ++i)
	{
		lua_getglobal(L, "require");// name, require
		lua_pushstring(L, names[i]);// name, require, req_name
		lua_call(L, 1, 0);	// name
	}

	lua_pushvalue(L, lua_upvalueindex(1));	// name, func
	lua_insert(L, -2);	// func, name
	lua_call(L, 1, 1);	// result

	return 1;
}

static void luaproc_reglualib_with_requires (lua_State *L, const char *name,
	lua_CFunction f, const char * names[]) {
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, f);
	lua_pushlightuserdata(L, (void *)names);
	lua_pushcclosure(L, LateLoader, 2);
	lua_setfield(L, -2, name);
	lua_pop(L, 2);
}

static void * sResDatum, * sDocsDatum, * sCachesDatum, * sTempDatum;
static const char * sDocsPath, * sCachesPath, * sTempPath;
static lua_Number sTime;

static void DirsDestructor (void)
{
	sResDatum = sDocsDatum = sCachesDatum = sTempDatum = NULL;
	sDocsPath = sCachesPath = sTempPath = NULL;
}

static void * BadDatum ()
{
	return (void *)&sDocsPath;
}

static int CanDoDirs (lua_State * L)
{
	sResDatum = BadDatum();
	sDocsDatum = BadDatum();
	sCachesDatum = BadDatum();
	sTempDatum = BadDatum();

	lua_getglobal(L, "system");	// ..., system

	if (lua_istable(L, -1))
	{
		//
		lua_getfield(L, -1, "getTimer");// ..., system, system.getTimer

		if (lua_isfunction(L, -1)) 
		{
			lua_call(L, 0, 1);	// ..., system, time

			sTime = luaL_checknumber(L, -1);
		}

		else printf("Warning: unable to create seed timer while initializing luaproc\n");

		lua_pop(L, 1);	// ..., system

		//
		lua_getfield(L, -1, "pathForFile");	// ..., system, system.pathForFile

		return lua_isfunction(L, -1);
	}

	printf("Warning: non-table `system` found while initializing luaproc\n");

	return 0;
}

static const char * LookupDir (lua_State * L, const char * name, void ** ud)
{
	lua_pushvalue(L, -1);	// ..., system, system.pathForFile, system.pathForFile
	lua_pushnil(L);// ..., system, system.pathForFile, system.pathForFile, nil
	lua_getfield(L, -4, name);	// ..., system, system.pathForFile, system.pathForFile, nil, system[name]

	if (lua_isuserdata(L, -1)) *ud = lua_touserdata(L, -1);

	else printf("Warning: non-userdata `system.%s` while initializing luaproc\n", name);

	lua_call(L, 2, 1);	// ..., system, system.pathForFile, path?

	if (lua_isstring(L, -1))
	{
		const char * path = lua_tostring(L, -1);

		lua_pushlightuserdata(L, *ud);	// ..., system, system.pathForFile, path, ud
		lua_insert(L, -2);	// ..., system, system.pathForFile, ud, path
		lua_settable(L, LUA_REGISTRYINDEX);	// system, system.pathForFile; registry = { ud = path }

		return path;
	}

	printf("Warning: `system.%s` does not yield a path while initializing luaproc\n", name);

	lua_pop(L, 1);	// ..., system, system.pathForFile

	return NULL;
}

/* update access mutex */
pthread_mutex_t mutex_update = PTHREAD_MUTEX_INITIALIZER;

static int sAlertRef = LUA_NOREF;

static void AlertRefDestructor (void)
{
/*
	pthread_mutex_destroy(&mutex_update);

	mutex_update = PTHREAD_MUTEX_INITIALIZER;
    */
	sAlertRef = LUA_NOREF;
}

static int AuxAlertReport (lua_State * L)
{
	crash_report * cur = (crash_report *)lua_touserdata(L, 1);

	lua_getref(L, sAlertRef);// report, Alert
	lua_pushliteral(L, "crash_report");	// report, Alert, "crash_report"
	lua_pushlstring(L, cur->mData, cur->mLength);	// report, Alert, "crash_report", payload
	lua_call(L, 2, 0);	// report[, err]

	return 0;
}

static void ProcessReports (lua_State * L, crash_report * cur)
{
	while (TRUE)
	{
		if (lua_cpcall(L, AuxAlertReport, cur) != 0) lua_pop(L, 1);// ...

		if (cur->mNext) cur = cur->mNext;

		else break;
	}
	
	pthread_mutex_lock(&mutex_update);

	cur->mNext = processed_reports;
	processed_reports = cur;

	pthread_mutex_unlock(&mutex_update);
}

static int Update (lua_State * L)
{
	lua_Number time;
	crash_report * filed;

	// Update timer.
	lua_getfield(L, 1, "time");	// event, time

	time = lua_tonumber(L, -1);

	pthread_mutex_lock(&mutex_update);

	sTime = time;
	filed = filed_reports;
	filed_reports = NULL;

	pthread_mutex_unlock(&mutex_update);

	if (filed) ProcessReports(L, filed);

	// Resolve alerts and paths.
	lua_pushvalue(L, lua_upvalueindex(1));	// event, time, update
	lua_call(L, 0, 0);	// event, time

	return 0;
}

static int GetFunction (lua_State * L)
{
	lua_CFunction func;
	void * payload;

	if (PopFunction(lua_tostring(L, 1), &func, &payload))
	{
		lua_pushcfunction(L, func);

		if (payload)
		{
			lua_pushlightuserdata(L, payload);

			return 2;
		}
	}

	else lua_pushnil(L);

	return 1;
}

static const char UpdateLua[] =
	"local luaproc, GetFunction = ...\n"
	"local pcall, system, receive, send, alert_dispatcher, pathForFile = pcall, system, luaproc.receive, luaproc.send\n"

	"if type(system) == 'table' then -- if missing, would already have gotten warning\n"
	"	pathForFile = type(system.pathForFile) == 'function' and system.pathForFile\n"

	"	if type(system.newEventDispatcher) == 'function' then\n"
	"		alert_dispatcher = system.newEventDispatcher()\n"
	"	end\n"
	"end\n"

	"local AlertEvent = {}\n"

	"return function()\n"
	"	while true do\n"
	"		local what, a, b = receive('" UPDATE_CHANNEL_NAME "', true)\n"

	"		if what == 'pathForFile' then -- a: temporary channel, b: name\n"
	"			if pathForFile then -- pathForFile() exists\n"
	"				send(a, pathForFile(b))\n"
	"			else -- otherwise, send nil to unblock waiting proc\n"
	"				send(a, nil)\n"
	"			end\n"
	"		elseif what == 'alert' and alert_dispatcher then -- a: message, b: payload\n"
	"			AlertEvent.name, AlertEvent.payload = a, b\n"

	"			alert_dispatcher:dispatchEvent(AlertEvent)\n"
	"		elseif what == 'call_func' then -- a: name\n"
	"			pcall(GetFunction(a))\n"

	"			send(a, nil)\n"
	"		elseif what == nil then\n"
	"			break\n"
	"		end\n"
	"	end\n"
	"end, alert_dispatcher";

static void PrepareRuntimeEvent (lua_State * L, const char * name)
{
	CoronaLuaPushRuntime(L);// ..., Runtime

	lua_getfield(L, -1, "addEventListener");// ..., Runtime, Runtime.addEventListener
	lua_insert(L, -2);	// ..., Runtime.addEventListener, Runtime
	lua_pushstring(L, name);// ..., Runtime.addEventListener, Runtime, name
}

/* Suspend logic */
// http://stackoverflow.com/a/13662972
static pthread_cond_t cond_suspended;

void sched_try_to_suspend (void)
{
	pthread_mutex_lock(&mutex_suspend);

	while (is_suspended) pthread_cond_wait(&cond_suspended, &mutex_suspend);

	pthread_mutex_unlock(&mutex_suspend);
}

void Resume (lua_State * L) // used on closing, too
{
	pthread_mutex_lock(&mutex_suspend);

	is_suspended = FALSE;

	pthread_cond_broadcast(&cond_suspended);
	pthread_mutex_unlock(&mutex_suspend);
}

static void Suspend (lua_State * L)
{
	pthread_mutex_lock(&mutex_suspend);

	is_suspended = TRUE;

	pthread_mutex_unlock(&mutex_suspend);
}

static int SystemEvent (lua_State * L)
{
	lua_getfield(L, 1, "type");	// event, type

	if (strcmp(lua_tostring(L, -1), "applicationSuspend") == 0) Suspend(L);
	else if (strcmp(lua_tostring(L, -1), "applicationResume") == 0) Resume(L);

	return 0;
}

static void AddSystem (lua_State * L)
{
	int top = lua_gettop(L);

	if (CanDoDirs(L)) // luaproc, system, system.pathForFile
	{
		//
		lua_getfield(L, -2, "ResourceDirectory");	// luaproc, system, system.pathForFile, system.ResourceDirectory

		if (lua_isuserdata(L, -1)) sResDatum = lua_touserdata(L, -1);
		
		else printf("Warning: non-userdata `system.ResourceDirectory` while initializing luaproc\n");

		lua_pop(L, 1);	// luaproc, system, system.pathForFile

		//
		sDocsPath = LookupDir(L, "DocumentsDirectory", &sDocsDatum);
		sCachesPath = LookupDir(L, "CachesDirectory", &sCachesDatum);
		sTempPath = LookupDir(L, "TemporaryDirectory", &sTempDatum);
	}

	else printf("Warning: non-function `system.pathForFile()` found while initializing luaproc\n");

	lua_settop(L, top);	// luaproc

	//
	PrepareRuntimeEvent(L, "enterFrame");	// luaproc, Runtime.addEventListener, Runtime, "enterFrame"

	if (luaL_loadstring(L, UpdateLua) != 0) lua_error(L); // luaproc, Runtime.addEventListener, Runtime, "enterFrame", UpdateLua code?

	lua_pushvalue(L, -5);	// luaproc, Runtime.addEventListener, Runtime, "enterFrame", UpdateLua code, luaproc
	lua_pushcfunction(L, GetFunction);	// luaproc, Runtime.addEventListener, Runtime, "enterFrame", UpdateLua code, luaproc, GetFunction
	lua_call(L, 2, 2);	// luaproc, Runtime.addEventListener, Runtime, "enterFrame", UpdateLua func, alert_dispatcher

	sAlertDispatcherRef = lua_ref(L, 1);	// luaproc, Runtime.addEventListener, Runtime, "enterFrame", UpdateLua func

	lua_pushcclosure(L, Update, 1);	// luaproc, Runtime.addEventListener, Runtime, "enterFrame", Update
	lua_call(L, 3, 0);	// luaproc

	//
	PrepareRuntimeEvent(L, "system");	// luaproc, Runtime.addEventListener, Runtime, "system"

	lua_pushcfunction(L, SystemEvent);	// luaproc, Runtime.addEventListener, Runtime, "system", SystemEvent
	lua_call(L, 3, 0);	// luaproc

	//
	pthread_cond_init(&cond_suspended, NULL);
}

static int GetTimer (lua_State * L)
{
	pthread_mutex_lock(&mutex_update);

	lua_pushnumber(L, sTime);

	pthread_mutex_unlock(&mutex_update);

	return 1;
}

static const char PathForFileLua[] =
	"local luaproc, ToString, IsNonResFile, DoNonResFile = ...\n"
	"local receive, send, newchannel, delchannel = luaproc.receive, luaproc.send, luaproc.newchannel, luaproc.delchannel\n"

	"return function(filename, datum)\n"
	"	if IsNonResFile(filename, datum) then\n"
	"		return DoNonResFile(filename, datum)\n"
	"	elseif filename == nil then\n"
	"		return nil\n"
	"	end\n"

	"	local dummy, path = {}\n"
	"	local temp_channel = ToString(dummy)\n"
	"	local ok, err = newchannel(temp_channel)\n"

	"	if ok then\n"
	"		ok, err = send('" UPDATE_CHANNEL_NAME "', 'pathForFile', temp_channel, filename)\n"

	"		if ok then\n"
	"			path, err = receive(temp_channel) -- nil (as error) ok\n"
	"		end\n"

	"		delchannel(temp_channel)\n"
	"	end\n"

	"	if err then\n"
	"		return nil, err\n"
	"	else\n"
	"		return path\n"
	"	end\n"
	"end";

static int IsNonResFile (lua_State * L)
{
	void * ud = NULL;

	if (lua_isuserdata(L, 2)) ud = lua_touserdata(L, 2);

	lua_pushboolean(L, ud == BadDatum() || (ud && ud != sResDatum));

	return 1;
}

static int DoNonResFile (lua_State * L)
{
	void * ud = lua_touserdata(L, 2);

	if (ud == BadDatum()) lua_pushnil(L);	// filename[, base], nil

	else
	{
		const char * base;

		if (ud == sDocsDatum) base = sDocsPath;
		else if (ud == sCachesDatum) base = sCachesPath;
		else base = sTempPath;

		if (lua_isnil(L, 1) || lua_objlen(L, 1) == 0) lua_pushstring(L, base); // filename[, base], nil

		else
		{
			luaL_Buffer B;

			luaL_buffinit(L, &B);
			luaL_addstring(&B, base);
			luaL_addchar(&B, '/');
			luaL_addstring(&B, luaL_checkstring(L, 1));
			luaL_pushresult(&B);// filename[, base], path
		}
	}

	return 1;
}

static int ToString (lua_State * L)
{
	lua_pushfstring(L, "LUAPROC_TEMP_%p", lua_topointer(L, 1));	// item, str

	return 1;
}

static int luaopen_system (lua_State * L)
{
	const char * names[] = { "ResourceDirectory", "DocumentsDirectory", "CachesDirectory", "TemporaryDirectory" };
	void * ud[] = { sResDatum, sDocsDatum, sCachesDatum, sTempDatum };
	int i, n = sizeof(names) / sizeof(names[0]);

	lua_createtable(L, 0, n + 2);	// system

	for (i = 0; i < n; ++i)
	{
		lua_pushlightuserdata(L, ud[i]);// system, ud
		lua_setfield(L, -2, names[i]);	// system = { ..., name = ud }
	}

	lua_pushcfunction(L, GetTimer);	// system, GetTimer
	lua_setfield(L, -2, "getTimer");// system = { ..., getTimer = GetTimer }

	if (luaL_loadstring(L, PathForFileLua) != 0) lua_error(L); // system, PathForFileLua code?

	lua_getglobal(L, "package");// system, PathForFileLua code, package
	lua_getfield(L, -1, "loaded");	// system, PathForFileLua code, package, package.loaded
	lua_getfield(L, -1, "luaproc");	// system, PathForFileLua code, package, package.loaded, luaproc
	lua_replace(L, -3);	// system, PathForFileLua code, luaproc, package.loaded
	lua_pop(L, 1);	// system, PathForFileLua code, luaproc
	lua_pushcfunction(L, ToString);	// system, PathForFileLua func, luaproc, ToString
	lua_pushcfunction(L, IsNonResFile);	// system, PathForFileLua func, luaproc, ToString, IsNonResFile
	lua_pushcfunction(L, DoNonResFile);	// system, PathForFileLua func, luaproc, ToString, IsNonResFile, DoNonResFile
	lua_call(L, 4, 1);	// system, PathForFileLua func
	lua_setfield(L, -2, "pathForFile");	// system = { ..., getTimer, pathForFile = PathForFile }, 
	lua_pushvalue(L, -1);	// system, system
	lua_setglobal(L, "system");	// system; _G.system = system

	return 1;
}

static int CoronaLibrary_New (lua_State * L)
{
	lua_newtable(L);// {}

	return 1;
}

static int luaopen_CoronaLibrary (lua_State * L) // todo: might actually be possible to do this right
{
	lua_createtable(L, 0, 1);	// CoronaLibrary

	lua_pushcfunction(L, CoronaLibrary_New);// CoronaLibrary, New
	lua_setfield(L, -2, "new");	// CoronaLibrary = { new = New }

	return 1;
}

static const char * sCPath, * sPath;

static const char * GetLuaPath (lua_State * L, const char * name)
{
	const char * path = NULL;

	lua_getfield(L, -1, name);	// ..., package, path

	if (lua_isstring(L, -1))
	{
		path = lua_tostring(L, -1);

		lua_pushlightuserdata(L, (void *)path);	// ..., package, path, pathstr_ptr
		lua_pushboolean(L, 1);	// ..., package, path, pathstr_ptr, true
		lua_settable(L, LUA_REGISTRYINDEX);	// ..., package, path; registry = { ..., [pathstr_ptr] = true }
	}

	lua_pop(L, 1);	// ..., package

	return path;
}

static void SetLuaPath (lua_State * L, const char * name, const char * str)
{
	if (str) lua_pushstring(L, str);// ..., package, str

	else lua_pushnil(L);// ..., package, nil

	lua_setfield(L, -2, name);	// ..., package
}

#define PRELOAD(n) { #n, NULL }
#define PRELOAD_WITH_REQUIRES(n, f) { #n, f }

// Point these at some Lua functions, which will be used to associate require chains where
// necessary. The choice of functions doesn't matter, so long as they're all distinct.
#define FuncReqM luaproc_Alert
#define FuncReqMST luaproc_create_newproc

static luaL_Reg Preloads[] = {
	//PRELOAD(CoronaLibrary),
	//PRELOAD(CoronaPrototype),
	//PRELOAD(CoronaProvider),
	PRELOAD(crypto),
	PRELOAD(dkjson),
	PRELOAD_WITH_REQUIRES(easing, FuncReqM),
	PRELOAD_WITH_REQUIRES(json, FuncReqMST),
	PRELOAD(lfs),
	PRELOAD(ltn12),
	PRELOAD(lpeg),
	PRELOAD(mime),
	PRELOAD(mime.core),
	PRELOAD(re),
	PRELOAD(socket),
	PRELOAD(socket.core),
	PRELOAD(socket.ftp),
	PRELOAD(socket.headers),
	PRELOAD(socket.http),
	PRELOAD(socket.mbox),
	PRELOAD(socket.smtp),
	PRELOAD(socket.tp),
	PRELOAD(socket.url),
	PRELOAD(sqlite3),
	{ NULL, NULL }
};

static const char ** sReqs[sizeof(Preloads) / sizeof(luaL_Reg) - 1];

#undef PRELOAD

static const char * sReqM[] = { "math", NULL };
static const char * sReqMST[] = { "math", "string", "table", NULL };

static lua_CFunction * sLoaders;

void ProcDestructors (void)
{
#ifndef _WIN32
	AlertDestructor();
	AlertRefDestructor();
	DirsDestructor();
	GlobalsDestructor();
	LoadlibDestructor();
	MetaDestructor();
	PreloadDestructor();
	SuspendDestructor();
#endif
}

// /STEVE CHANGE

static void luaproc_openlualibs(lua_State *L) {
	int i;	// <- STEVE CHANGE

	requiref(L, "_G", luaopen_base, FALSE);
	requiref(L, "package", luaopen_package, TRUE);
// STEVE CHANGE
	lua_getglobal(L, "package");// ..., package

	SetLuaPath(L, "cpath", sCPath);
	SetLuaPath(L, "path", sPath);
	/*
	lua_newtable(L);// ..., package, new_loaders

	for (i = 0; sLoaders[i]; ++i)
	{
		lua_pushcfunction(L, sLoaders[i]);	// ..., package, new_loaders, loader
		lua_pushvalue(L, -3);	// ..., package, new_loaders, loader, package
		lua_setfenv(L, -2);	// ..., package, new_loaders, loader
		lua_rawseti(L, -2, i + 1);	// ..., package, new_loaders = { ..., loader }
	}

	lua_setfield(L, -2, "loaders");	// ..., package = { loaders = new_loaders }
	*/
	lua_getfield(L, -1, "loaders");	// ..., package, package.loaders
	lua_pushcfunction(L, CustomPreload_Loader);	// ..., package, package.loaders, CustomPreloadLoader
	lua_rawseti(L, -2, lua_objlen(L, -2) + 1);	// ..., package, package.loaders = { ..., CustomPreloadLoader }
	lua_pop(L, 2);	// ...
// /STEVE CHANGE
	luaproc_reglualib(L, "io", luaopen_io);
	luaproc_reglualib(L, "os", luaopen_os);
	luaproc_reglualib(L, "table", luaopen_table);
	luaproc_reglualib(L, "string", luaopen_string);
	luaproc_reglualib(L, "math", luaopen_math);
	luaproc_reglualib(L, "debug", luaopen_debug);
// STEVE CHANGE
	luaproc_reglualib(L, "system", luaopen_system);
	luaproc_reglualib(L, "CoronaLibrary", luaopen_CoronaLibrary);

	for (i = 0; Preloads[i].name; ++i)
	{
		if (!sReqs[i]) luaproc_reglualib(L, Preloads[i].name, Preloads[i].func);

		else luaproc_reglualib_with_requires(L, Preloads[i].name, Preloads[i].func, sReqs[i]);
	}
// /STEVE CHANGE
#if (LUA_VERSION_NUM == 502)
	luaproc_reglualib(L, "bit32", luaopen_bit32);
#endif
#if (LUA_VERSION_NUM >= 502)
	luaproc_reglualib(L, "coroutine", luaopen_coroutine);
#endif
#if (LUA_VERSION_NUM >= 503)
	luaproc_reglualib(L, "utf8", luaopen_utf8);
#endif

}

/*LUALIB_API*/ CORONA_EXPORT int luaopen_plugin_luaproc(lua_State *L) { // <- STEVE CHANGE
	/* register luaproc functions */
// STEVE CHANGE
//	luaL_newlib(L, luaproc_funcs);

	CoronaLibraryNew(L, "luaproc", "com.xibalbastudios", 1, 0, luaproc_funcs, NULL);

	//
	lua_createtable(L, 0, 3);	// luaproc, event
	lua_pushcclosure(L, luaproc_Alert, 1);	// luaproc, Alert				
	lua_pushvalue(L, -1);	// luaproc, Alert, Alert

	sAlertRef = lua_ref(L, 1);	// luaproc, Alert

	lua_setfield(L, -2, "alert");	// luaproc

	//
	luaL_getmetatable(L, "_LOADLIB");	// luaproc, _LOADLIB_mt
	lua_getfield(L, -1, "__gc");// luaproc, _LOADLIB_mt, _LOADLIB_gc
	lua_newtable(L);// luaproc, _LOADLIB_mt, _LOADLIB_gc, deferred

	sDeferredRef = lua_ref(L, 1);	// luaproc, _LOADLIB_mt, _LOADLIB_gc
	sGCRef = lua_ref(L, 1);	// luaproc, _LOADLIB_mt
	sLoadlibMetaRef = lua_ref(L, 1);// luaproc
// /STEVE CHANGE
	/* wrap main state inside a lua process */
	mainlp.lstate = L;
	mainlp.status = LUAPROC_STATUS_IDLE;
	mainlp.args = 0;
	mainlp.chan = NULL;
	mainlp.next = NULL;
	/* initialize recycle list */
	list_init(&recycle_list);
	/* initialize channels table and lua_State used to store it */
	chanls = luaL_newstate();
	lua_newtable(chanls);
	lua_setglobal(chanls, LUAPROC_CHANNELS_TABLE);
	/* create finalizer to join workers when Lua exits */
	lua_newuserdata(L, 0);
	lua_setfield(L, LUA_REGISTRYINDEX, "LUAPROC_FINALIZER_UDATA");
	luaL_newmetatable(L, "LUAPROC_FINALIZER_MT");
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, luaproc_join_workers);
	lua_rawset(L, -3);
	lua_pop(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, "LUAPROC_FINALIZER_UDATA");
	lua_getfield(L, LUA_REGISTRYINDEX, "LUAPROC_FINALIZER_MT");
	lua_setmetatable(L, -2);
	lua_pop(L, 1);
// STEVE CHANGE
	InitExtensions();
	AddSystem(L);

	lua_getfield(L, -1, "newchannel"); // luaproc, luaproc.newchannel
	lua_pushliteral(L, UPDATE_CHANNEL_NAME);// luaproc, luaproc.newchannel, UPDATE_CHANNEL_NAME
	lua_call(L, 1, 0);	// luaproc

	//
	lua_getglobal(L, "package");// luaproc, package

	if (lua_istable(L, -1))
	{
		int i, n;

		//
		sCPath = GetLuaPath(L, "cpath");
		sPath = GetLuaPath(L, "path");

		//
		lua_getfield(L, -1, "preload");	// luaproc, package, package.preload

		for (i = 0; Preloads[i].name; ++i, lua_pop(L, 1))
		{
			lua_CFunction cur = Preloads[i].func;

			lua_getfield(L, -1, Preloads[i].name);	// luaproc, package, package.preload, mod

			Preloads[i].func = lua_tocfunction(L, -1);

			if (cur == FuncReqM) sReqs[i] = sReqM;
			else if (cur == FuncReqMST) sReqs[i] = sReqMST;
		}

		//
		lua_getfield(L, -2, "loaders");	// luaproc, package, package.preload, package.loaders

		n = (int)lua_objlen(L, -1);

		lua_pushlightuserdata(L, &sLoaders);// luaproc, package, package.preload, package.loaders, loaders_key

		sLoaders = lua_newuserdata(L, sizeof(lua_CFunction) * n + 1);	// luaproc, package, package.preload, package.loaders, loaders_key, loaders_arr

		lua_rawset(L, LUA_REGISTRYINDEX);	// luaproc, package, package.preload, package.loaders; registry = { ..., [loaders_key] = loaders_arr }

		for (i = 1; i <= n; ++i, lua_pop(L, 1))
		{
			lua_rawgeti(L, -1, i);	// luaproc, package, package.preload, package.loaders, loader

			sLoaders[i - 1] = lua_tocfunction(L, -1);
		}

		sLoaders[n] = NULL;

		lua_pop(L, 2);	// luaproc, package

		lua_getfield(L, -1, "loadlib");	// luaproc, package, package.loadlib

		if (!lua_isfunction(L, -1)) printf("Warning: non-function package.loadlib while loading luaproc\n");

		sLoadlibRef = lua_ref(L, 1);// luaproc, package
	}

	lua_pop(L, 1);	// luaproc
// /STEVE CHANGE
	/* initialize scheduler */
	if (sched_init() == LUAPROC_SCHED_PTHREAD_ERROR) {
		luaL_error(L, "failed to create worker");
	}

	return 1;
}

// STEVE CHANGE
const char AlertLua[] =
	"local luaproc = ...\n"
	"local send, wants_to_close = luaproc.send, luaproc.wants_to_close\n"

	"return function(message, payload)\n"
	"	if not wants_to_close() then\n"
	"		send('" UPDATE_CHANNEL_NAME "', 'alert', message, payload)\n"
	"	end\n"
	"end";

const char CallerLua[] =
	"local luaproc, ToString, GetFunction, PushFunction = ...\n"
	"local send, wants_to_close = luaproc.send, luaproc.wants_to_close\n"

	"return function(func)\n"
	"	if not wants_to_close() then\n"
	"		local dummy = {}\n"
	"		local name = ToString(dummy)\n"
	"		local ok, err = newchannel(name)\n"

	"		if ok then\n"
	"			PushFunction(name, func)\n"

	"			ok, err = send('" UPDATE_CHANNEL_NAME "', 'call_func', name)\n"

	"			if ok then\n"
	"				receive(name)\n"
	"			else\n"
	"				GetFunction(name)\n"
	"			end\n"

	"			delchannel(name)\n"
	"		end\n"
	"	end\n"
	"end";

static int PushFunction (lua_State * L)
{
	size_t size = lua_objlen(L, 2), ex = sizeof(lua_CFunction) + sizeof(void *);
	void * payload = NULL;
	lua_CFunction func;

	luaL_argcheck(L, lua_type(L, 2) == LUA_TUSERDATA && (size == sizeof(lua_CFunction) || size == ex), 2, "Argument could not be function");

	memcpy(&func, lua_touserdata(L, 2), sizeof(lua_CFunction));

	if (size == ex) memcpy(&payload, ((lua_CFunction *)lua_touserdata(L, 2)) + 1, sizeof(void *));

	AddFunction(lua_tostring(L, 1), func, payload);

	return 0;
}
// /STEVE CHANGE

static int luaproc_loadlib(lua_State *L) {

	/* register luaproc functions */
	luaL_newlib(L, luaproc_funcs);
// STEVE CHANGE
	if (luaL_loadstring(L, AlertLua) != 0) lua_error(L); // luaproc, AlertLua code?

	lua_pushvalue(L, -2);	// luaproc, AlertLua func, luaproc
	lua_call(L, 1, 1);	// luaproc, Alert
	lua_setfield(L, -2, "alert");	// luaproc

	if (luaL_loadstring(L, CallerLua) != 0) lua_error(L); // luaproc, CallerLua code?

	lua_pushvalue(L, -2);	// luaproc, CallerLua func, luaproc
	lua_pushcfunction(L, ToString);	// luaproc, CallerLua func, luaproc, ToString
	lua_pushcfunction(L, GetFunction);	// luaproc, CallerLua func, luaproc, ToString, GetFunction
	lua_pushcfunction(L, PushFunction);	// luaproc, CallerLua func, luaproc, ToString, GetFunction, PushFunction
	lua_call(L, 4, 1);	// luaproc, Caller
	lua_setfield(L, LUA_REGISTRYINDEX, "LUAPROC_CALLER_FUNC");	// luaproc; registry = { ..., LUAPROC_CALLER_FUNC = Caller }
// /STEVE CHANGE
	return 1;
}

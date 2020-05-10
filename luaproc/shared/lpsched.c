/*
** scheduler module for executing lua processes
** See Copyright Notice in luaproc.h
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// STEVE CHANGE
#include <string.h>
/*
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
*/
#include "CoronaLua.h"
// /STEVE CHANGE

#include "lpsched.h"
#include "luaproc.h"

#define FALSE 0
#define TRUE  !FALSE
#define LUAPROC_SCHED_WORKERS_TABLE "workertb"

#if (LUA_VERSION_NUM >= 502)
#define luaproc_resume( L, from, nargs ) lua_resume( L, from, nargs )
#else
#define luaproc_resume( L, from, nargs ) lua_resume( L, nargs )
#endif

/********************
* global variables *
*******************/

/* ready process list */
list ready_lp_list;

/* ready process queue access mutex */
pthread_mutex_t mutex_sched = PTHREAD_MUTEX_INITIALIZER;

/* active luaproc count access mutex */
pthread_mutex_t mutex_lp_count = PTHREAD_MUTEX_INITIALIZER;

/* wake worker up conditional variable */
pthread_cond_t cond_wakeup_worker = PTHREAD_COND_INITIALIZER;

/* no active luaproc conditional variable */
pthread_cond_t cond_no_active_lp = PTHREAD_COND_INITIALIZER;

/* lua_State used to store workers hash table */
static lua_State *workerls = NULL;

int lpcount = 0;         /* number of active luaprocs */
int workerscount = 0;    /* number of active workers */
int destroyworkers = 0;  /* number of workers to destroy */

//
// STEVE CHANGE
//

static void GlobalsDestructor (void)
{
/*
	pthread_mutex_destroy(&mutex_sched);
	pthread_mutex_destroy(&mutex_lp_count);

	mutex_sched = PTHREAD_MUTEX_INITIALIZER;
	mutex_lp_count = PTHREAD_MUTEX_INITIALIZER;

	pthread_cond_destroy(&cond_wakeup_worker);
	pthread_cond_destroy(&cond_no_active_lp);

	cond_wakeup_worker = PTHREAD_COND_INITIALIZER;
	cond_no_active_lp = PTHREAD_COND_INITIALIZER;
*/
	workerls = NULL;
	lpcount = workerscount = destroyworkers = 0;
}

//
// /STEVE CHANGE
//

/***********************
* register prototypes *
***********************/

static void sched_dec_lpcount(void);

//
// STEVE CHANGE
//

static void add_thread(lua_State * L, pthread_t thread)
{
#if defined(_MSC_VER)
	*(pthread_t *)lua_newuserdata(L, sizeof(pthread_t)) = thread;
#else
	lua_pushlightuserdata(L, (void *)thread);
#endif
}

static pthread_t get_thread(lua_State * L, int index)
{
#if defined(_MSC_VER)
	return *(pthread_t *)lua_touserdata(L, index);
#else
	return (pthread_t)lua_touserdata(L, index);
#endif
}

crash_report * filed_reports = NULL, * processed_reports = NULL;

static void FileReport (lua_State * L, const char * err, size_t length)
{
	crash_report * cr;

	lua_getglobal(L, "CRASH_REPORTS");	// ..., reports?

	if (lua_isnil(L, -1))
	{
		lua_newtable(L);	// ..., nil, reports
		lua_pushvalue(L, -1);// ..., nil, reports, reports
		lua_setglobal(L, "CRASH_REPORTS");	// ..., nil, reports
		lua_remove(L, -2);	// ..., reports
	}

	cr = lua_newuserdata(L, sizeof(crash_report) + length - 1);	// ..., reports, cr

	cr->mIndex = (int)lua_objlen(L, -2) + 1;

	lua_rawseti(L, -2, cr->mIndex);	// ..., reports = { ..., cr, ... }

	cr->mLength = length;

	memcpy(cr->mData, err, length);

	pthread_mutex_lock(&mutex_update);

	cr->mNext = filed_reports;
	filed_reports = cr;

	pthread_mutex_unlock(&mutex_update);
}

static void ReportsDestructor (void)
{
	filed_reports = processed_reports = NULL;
}

//
// /STEVE CHANGE
//

/*******************************
* worker thread main function *
*******************************/

/* worker thread main function */
void *workermain(void *args) {

	luaproc *lp;
	int procstat;

	/* main worker loop */
	while (TRUE) {
		// STEVE CHANGE
		crash_report * processed, * next;

		pthread_mutex_lock(&mutex_update);

		processed = processed_reports;
		processed_reports = NULL;

		pthread_mutex_unlock(&mutex_update);

		if (processed)	// If there are processed reports, the CRASH_REPORTS table exists
		{
			lua_getglobal(workerls, "CRASH_REPORTS");	// ..., reports

			for (; processed; processed = next)
			{
				next = processed->mNext;// Grab before removing from table

				lua_pushnil(workerls);	// ..., reports, nil
				lua_rawseti(workerls, -2, processed->mIndex);	// ..., reports; CRASH_REPORTS = { ..., nil, ... }
			}
		}
		// /STEVE CHANGE
		/*
		wait until instructed to wake up (because there's work to do
		or because workers must be destroyed)
		*/
		pthread_mutex_lock(&mutex_sched);
		while ((list_count(&ready_lp_list) == 0) && (destroyworkers <= 0)) {
			pthread_cond_wait(&cond_wakeup_worker, &mutex_sched);
		}

		if (destroyworkers > 0) {  /* check whether workers should be destroyed */

			destroyworkers--; /* decrease workers to be destroyed count */
			workerscount--; /* decrease active workers count */

			/* remove worker from workers table */
			lua_getglobal(workerls, LUAPROC_SCHED_WORKERS_TABLE);
// STEVE CHANGE
			//	  lua_pushlightuserdata(workerls, (void *)pthread_self());
			add_thread(workerls, pthread_self());
// /STEVE CHANGE
			lua_pushnil(workerls);
			lua_rawset(workerls, -3);
			lua_pop(workerls, 1);

			pthread_cond_signal(&cond_wakeup_worker);  /* wake other workers up */
			pthread_mutex_unlock(&mutex_sched);
			pthread_exit(NULL);  /* destroy itself */
		}

		/* remove lua process from the ready queue */
		lp = list_remove(&ready_lp_list);
		pthread_mutex_unlock(&mutex_sched);

		/* execute the lua code specified in the lua process struct */
		procstat = luaproc_resume(luaproc_get_state(lp), NULL,
			luaproc_get_numargs(lp));
		/* reset the process argument count */
		luaproc_set_numargs(lp, 0);

		/* has the lua process sucessfully finished its execution? */
		if (procstat == 0) {
			luaproc_set_status(lp, LUAPROC_STATUS_FINISHED);
			luaproc_recycle_insert(lp);  /* try to recycle finished lua process */
			sched_dec_lpcount();  /* decrease active lua process count */
		}

		/* has the lua process yielded? */
		else if (procstat == LUA_YIELD) {

			/* yield attempting to send a message */
			if (luaproc_get_status(lp) == LUAPROC_STATUS_BLOCKED_SEND) {
				luaproc_queue_sender(lp);  /* queue lua process on channel */
				/* unlock channel */
				luaproc_unlock_channel(luaproc_get_channel(lp));
			}

			/* yield attempting to receive a message */
			else if (luaproc_get_status(lp) == LUAPROC_STATUS_BLOCKED_RECV) {
				luaproc_queue_receiver(lp);  /* queue lua process on channel */
				/* unlock channel */
				luaproc_unlock_channel(luaproc_get_channel(lp));
			}

			/* yield on explicit coroutine.yield call */
			else {
				/* re-insert the job at the end of the ready process queue */
				pthread_mutex_lock(&mutex_sched);
				list_insert(&ready_lp_list, lp);
				pthread_mutex_unlock(&mutex_sched);
			}
		}

		/* or was there an error executing the lua process? */
		else {
			// STEVE CHANGE
			lua_State * L = luaproc_get_state(lp);
			int report_method = 0, top = lua_gettop(L);

			lua_getglobal(L, "LUAPROC_ERROR");	// ..., err, t?

			if (lua_istable(L, -1))
			{
				lua_getfield(L, -1, "report_method");	// ..., err, t, report_method

				if (lua_type(L, -1) == LUA_TSTRING)
				{
					const char * names[] = { "print", "alert", "ignore", NULL };

					report_method = luaL_checkoption(L, -1, "print", names);
				}

				lua_getfield(L, -2, "cleanup");// ..., err, t, report_method, cleanup

				if (lua_isfunction(L, -1))
				{
					lua_pushvalue(L, -4);	// ..., err, t, report_method, cleanup, err

					if (lua_pcall(L, 1, 1, 0) == 0 && !lua_isnil(L, -1))	// ..., err, t, report_method, message? / new_error
					{
						lua_replace(L, -4);	// ..., new_error, t, report_method
					}
				}
			}

			lua_settop(L, top);	// ..., err

			switch (lua_type(L, -1))
			{
			case LUA_TSTRING:
			case LUA_TNUMBER:	// will be converted to string by luaL_checkstring()
				break;
			case LUA_TBOOLEAN:
				lua_pushstring(L, lua_toboolean(L, -1) ? "true" : "false");	// ..., err, "true" / "false"
				break;
			default:
				lua_pushfstring(L, "%s:%p", luaL_typename(L, -1), lua_topointer(L, -1));// ..., err, "type:address"
				break;
			}

			if (lua_gettop(L) > top) lua_replace(L, -2);// ..., err_str

			if (report_method == 0)
			// /STEVE CHANGE
			/* print error message */
			fprintf(stderr, "close lua_State (error: %s)\n",
				luaL_checkstring(luaproc_get_state(lp), -1));
			// STEVE CHANGE
			else if (report_method == 1) FileReport(workerls, luaL_checkstring(L, -1), lua_objlen(L, -1));
			// /STEVE CHANGE
			lua_close(luaproc_get_state(lp));  /* close lua state */
			sched_dec_lpcount();  /* decrease active lua process count */
		}
	}
}

/***********************
* auxiliary functions *
**********************/

/* decrease active lua process count */
static void sched_dec_lpcount(void) {
	pthread_mutex_lock(&mutex_lp_count);
	lpcount--;
	/* if count reaches zero, signal there are no more active processes */
	if (lpcount == 0) {
		pthread_cond_signal(&cond_no_active_lp);
	}
	pthread_mutex_unlock(&mutex_lp_count);
}

/**********************
* exported functions *
**********************/

/* increase active lua process count */
void sched_inc_lpcount(void) {
	pthread_mutex_lock(&mutex_lp_count);
	lpcount++;
	pthread_mutex_unlock(&mutex_lp_count);
}

/* local scheduler initialization */
int sched_init(void) {

	int i;
	pthread_t worker;

	/* initialize ready process list */
	list_init(&ready_lp_list);

	/* initialize workers table and lua_State used to store it */
	workerls = luaL_newstate();
	lua_newtable(workerls);
	lua_setglobal(workerls, LUAPROC_SCHED_WORKERS_TABLE);

	/* get ready to access worker threads table */
	lua_getglobal(workerls, LUAPROC_SCHED_WORKERS_TABLE);

	/* create default number of initial worker threads */
	for (i = 0; i < LUAPROC_SCHED_DEFAULT_WORKER_THREADS; i++) {

		if (pthread_create(&worker, NULL, workermain, NULL) != 0) {
			lua_pop(workerls, 1); /* pop workers table from stack */
			return LUAPROC_SCHED_PTHREAD_ERROR;
		}

		/* store worker thread id in a table */
// STEVE CHANGE
		//	lua_pushlightuserdata(workerls, (void *)worker);
		add_thread(workerls, worker);
// /STEVE CHANGE
		lua_pushboolean(workerls, TRUE);
		lua_rawset(workerls, -3);

		workerscount++; /* increase active workers count */
	}

	lua_pop(workerls, 1); /* pop workers table from stack */

	return LUAPROC_SCHED_OK;
}

/* set number of active workers */
int sched_set_numworkers(int numworkers) {

	int i, delta;
	pthread_t worker;

	pthread_mutex_lock(&mutex_sched);

	/* calculate delta between existing workers and set number of workers */
	delta = numworkers - workerscount;

	/* create additional workers */
	if (numworkers > workerscount) {

		/* get ready to access worker threads table */
		lua_getglobal(workerls, LUAPROC_SCHED_WORKERS_TABLE);

		/* create additional workers */
		for (i = 0; i < delta; i++) {

			if (pthread_create(&worker, NULL, workermain, NULL) != 0) {
				pthread_mutex_unlock(&mutex_sched);
				lua_pop(workerls, 1); /* pop workers table from stack */
				return LUAPROC_SCHED_PTHREAD_ERROR;
			}

			/* store worker thread id in a table */
// STEVE CHANGE
			//	  lua_pushlightuserdata(workerls, worker);
			add_thread(workerls, worker);
// /STEVE CHANGE
			lua_pushboolean(workerls, TRUE);
			lua_rawset(workerls, -3);

			workerscount++; /* increase active workers count */
		}

		lua_pop(workerls, 1); /* pop workers table from stack */
	}
	/* destroy existing workers */
	else if (numworkers < workerscount) {
		destroyworkers = destroyworkers + numworkers;
	}

	pthread_mutex_unlock(&mutex_sched);

	return LUAPROC_SCHED_OK;
}

/* return the number of active workers */
int sched_get_numworkers(void) {

	int numworkers;

	pthread_mutex_lock(&mutex_sched);
	numworkers = workerscount;
	pthread_mutex_unlock(&mutex_sched);

	return numworkers;
}

/* insert lua process in ready queue */
void sched_queue_proc(luaproc *lp) {
	pthread_mutex_lock(&mutex_sched);
	list_insert(&ready_lp_list, lp);  /* add process to ready queue */
	/* set process status ready */
	luaproc_set_status(lp, LUAPROC_STATUS_READY);
	pthread_cond_signal(&cond_wakeup_worker);  /* wake worker up */
	pthread_mutex_unlock(&mutex_sched);
}

// STEVE CHANGE
static int wants_to_close = FALSE, is_waiting = FALSE;

static void StatesDestructor (void)
{
	wants_to_close = is_waiting = FALSE;
}
// /STEVE CHANGE

/* join worker threads (called when Lua exits). not joining workers causes a
race condition since lua_close unregisters dynamic libs with dlclose and
thus libpthreads can be unloaded while there are workers that are still
alive. */
void sched_join_workers(/*void*/lua_State * main) { // <- STEVE CHANGE

	lua_State *L = luaL_newstate();
	const char *wtb = "workerstbcopy";

// STEVE CHANGE
	pthread_mutex_lock(&mutex_sched);

	wants_to_close = TRUE;

	pthread_mutex_unlock(&mutex_sched);

	Resume(main);
// /STEVE CHANGE

	/* wait for all running lua processes to finish */
	sched_wait();
// STEVE CHANGE
	CommitUnloads(main);
// /STEVE CHANGE
	/* initialize new state and create table to copy worker ids */
	lua_newtable(L);
	lua_setglobal(L, wtb);
	lua_getglobal(L, wtb);

	pthread_mutex_lock(&mutex_sched);

	/* determine remaining active worker threads and copy their ids */
	lua_getglobal(workerls, LUAPROC_SCHED_WORKERS_TABLE);
	lua_pushnil(workerls);
	while (lua_next(workerls, -2) != 0) {
		lua_pushlightuserdata(L, lua_touserdata(workerls, -2));
		lua_pushboolean(L, TRUE);
		lua_rawset(L, -3);
		/* pop value, leave key for next iteration */
		lua_pop(workerls, 1);
	}

	/* pop workers copy table name from stack */
	lua_pop(L, 1);

	/* set all workers to be destroyed */
	destroyworkers = workerscount;

	/* wake workers up */
	pthread_cond_signal(&cond_wakeup_worker);
	pthread_mutex_unlock(&mutex_sched);

	/* join with worker threads (read ids from local table copy ) */
	lua_getglobal(L, wtb);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
// STEVE CHANGE
		//    pthread_join(( pthread_t )lua_touserdata( L, -2 ), NULL );
		pthread_join(get_thread(L, -2), NULL);
// /STEVE CHANGE
		/* pop value, leave key for next iteration */
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_close(workerls);
	lua_close(L);
}

/* wait until there are no more active lua processes and active workers. */
void sched_wait(void) {
// STEVE CHANGE
	pthread_mutex_lock(&mutex_sched);

	is_waiting = TRUE;

	pthread_mutex_unlock(&mutex_sched);
// /STEVE CHANGE
	/* wait until there are not more active lua processes */
	pthread_mutex_lock(&mutex_lp_count);
	/*if*/while (lpcount != 0) {	// <- STEVE CHANGE
		pthread_cond_wait(&cond_no_active_lp, &mutex_lp_count);
	}
	pthread_mutex_unlock(&mutex_lp_count);
// STEVE CHANGE
	pthread_mutex_lock(&mutex_sched);

	is_waiting = FALSE;

	pthread_mutex_unlock(&mutex_sched);
// /STEVE CHANGE
}

// STEVE CHANGE
int sched_is_waiting (int can_suspend)
{
	if (can_suspend) sched_try_to_suspend();

	pthread_mutex_lock(&mutex_sched);

	int waiting = is_waiting;

	pthread_mutex_unlock(&mutex_sched);

	return waiting;
}

int sched_wants_to_close (int can_suspend)
{
	if (can_suspend) sched_try_to_suspend();

	pthread_mutex_lock(&mutex_sched);

	int closing = wants_to_close;

	pthread_mutex_unlock(&mutex_sched);

	return closing;
}

void SchedDestructors (void)
{
	GlobalsDestructor();
	ReportsDestructor();
	StatesDestructor();
}
// /STEVE CHANGE

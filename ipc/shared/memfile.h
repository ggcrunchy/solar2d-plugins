#ifndef MEMFILE_H_
#define MEMFILE_H_

#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"


#define MEMFILE_R  1
#define MEMFILE_W  2
#define MEMFILE_RW (MEMFILE_R|MEMFILE_W)


IPC_LOCAL void* memfile_udata( lua_State* L, int idx,
                               char const* name );

IPC_LOCAL void memfile_new( lua_State* L, void* addr, size_t n,
                            int perms, int oidx, int closeidx,
                            int flushidx );


#endif /* MEMFILE_H_ */


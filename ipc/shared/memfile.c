#include <stddef.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "ipc.h"
#include "memfile.h"


/* used to identify metatable */
#define NAME "ipc.memfile"
#define OBJECT_INDEX 1
#define CLOSE_INDEX 2
#define FLUSH_INDEX 3

typedef struct {
  char* addr;
  size_t n;
  size_t p;
  int flags;
} memfile;


static memfile* tomemfile( lua_State* L, int idx ) {
  memfile* mf = luaL_checkudata( L, idx, NAME );
  if( mf->addr == NULL )
    luaL_error( L, "attempt to use closed in-memory file" );
  return mf;
}


static int read_all( lua_State* L, memfile* mf ) {
  lua_pushlstring( L, mf->addr + mf->p, mf->n - mf->p );
  mf->p = mf->n;
  return LUA_TSTRING;
}


static int read_n( lua_State* L, memfile* mf, size_t n ) {
  if( mf->p == mf->n )
    lua_pushnil( L );
  else {
    if( mf->n - mf->p < n )
      n = mf->n - mf->p;
    lua_pushlstring( L, mf->addr + mf->p, n );
    mf->p += n;
  }
  return lua_type( L, -1 );
}


static int read_line( lua_State* L, memfile* mf,
                             int store_nl ) {
  if( mf->p == mf->n )
    lua_pushnil( L );
  else {
    luaL_Buffer b;
    int oldc = '\0';
    int c = '\0';
    luaL_buffinit( L, &b );
    while( mf->p != mf->n &&
           (c = mf->addr[ mf->p++ ]) != '\n' ) {
      if( oldc == '\r' ) {
        luaL_addchar( &b, oldc );
        oldc = '\0';
      }
      if( c == '\r' ) {
        oldc = '\r';
      } else
        luaL_addchar( &b, c );
    }
    if( oldc == '\r' && (c != '\n' || store_nl) )
      luaL_addchar( &b, oldc );
    if( c == '\n' && store_nl )
      luaL_addchar( &b, c );
    luaL_pushresult( &b );
  }
  return lua_type( L, -1 );
}


static int lines_iter( lua_State* L ) {
  memfile* mf = tomemfile( L, lua_upvalueindex( 1 ) );
  int nupvals = lua_tointeger( L, lua_upvalueindex( 2 ) );
  int i = 3;
  lua_settop( L, 0 );
  luaL_checkstack( L, nupvals-2, "not enough stack space" );
  for( i = 3; i <= nupvals; ++i ) {
    if( lua_type( L, lua_upvalueindex( i ) ) == LUA_TNUMBER ) {
      lua_Number n = lua_tonumber( L, lua_upvalueindex( i ) );
      lua_Integer x = lua_tointeger( L, lua_upvalueindex( i ) );
      if( x < 0 || (lua_Number)x != n )
        luaL_error( L, "invalid format" );
      if( LUA_TNIL == read_n( L, mf, x ) )
        return i-2;
    } else {
      char const* fmt = lua_tostring( L, lua_upvalueindex( i ) );
      if( !fmt )
        luaL_error( L, "invalid format" );
      if( *fmt == '*' ) /* '*' is optional in later Lua versions */
        fmt++;
      switch( *fmt ) {
        case 'a':
          if( LUA_TNIL == read_all( L, mf ) )
            return i-2;
          break;
        case 'l':
          if( LUA_TNIL == read_line( L, mf, 0 ) )
            return i-2;
          break;
        case 'L':
          if( LUA_TNIL == read_line( L, mf, 1 ) )
            return i-2;
          break;
        default:
          luaL_error( L, "invalid format" );
          break;
      }
    }
  }
  return nupvals-2;
}


static int memfile_flush( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  int top;
  if( (mf->flags & MEMFILE_W) == 0 ) {
    lua_pushnil( L );
    lua_pushliteral( L, "permission denied" );
    return 2;
  }
  lua_getuservalue( L, 1 );
  top = lua_gettop( L );
  lua_rawgeti( L, -1, FLUSH_INDEX );
  lua_rawgeti( L, -2, OBJECT_INDEX );
  lua_pushinteger( L, (lua_Integer)mf->p );
  if( lua_type( L, -3 ) == LUA_TFUNCTION )
    lua_call( L, 2, LUA_MULTRET );
  else {
    lua_pop( L, 3 );
    lua_pushboolean( L, 1 );
  }
  return lua_gettop( L )-top;
}


static int memfile_lines( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  int top = lua_gettop( L );
  if( (mf->flags & MEMFILE_R) == 0 )
    luaL_error( L, "permission denied" );
  if( top == 1 ) {
    lua_pushliteral( L, "l" );
    top = 2;
  };
  lua_pushinteger( L, top+1 );
  lua_insert( L, 2 ); /* memfile, nupvals, arg1, ... */
  lua_pushcclosure( L, lines_iter, top+1 );
  return 1;
}


static int memfile_read( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  int top = lua_gettop( L );
  int i = 2;
  if( (mf->flags & MEMFILE_R) == 0 ) {
    lua_pushnil( L );
    lua_pushliteral( L, "permission denied" );
    return 2;
  }
  if( top == 1 ) {
    lua_pushliteral( L, "l" );
    top = 2;
  }
  luaL_checkstack( L, top-1, "not enough stack space" );
  for( i = 2; i <= top; ++i ) {
    if( lua_type( L, i ) == LUA_TNUMBER ) {
      lua_Number n = lua_tonumber( L, i );
      lua_Integer x = lua_tointeger( L, i );
      if( x < 0 || (lua_Number)x != n )
        luaL_argerror( L, i, "invalid format" );
      if( LUA_TNIL == read_n( L, mf, x ) )
        return i-1;
    } else {
      char const* fmt = lua_tostring( L, i );
      if( !fmt )
        luaL_argerror( L, i, "invalid format" );
      if( *fmt == '*' ) /* '*' is optional in later Lua versions */
        fmt++;
      switch( *fmt ) {
        case 'a':
          if( LUA_TNIL == read_all( L, mf ) )
            return i-1;
          break;
        case 'l':
          if( LUA_TNIL == read_line( L, mf, 0 ) )
            return i-1;
          break;
        case 'L':
          if( LUA_TNIL == read_line( L, mf, 1 ) )
            return i-1;
          break;
        default:
          luaL_argerror( L, i, "invalid format" );
          break;
      }
    }
  }
  return top-1;
}


static int memfile_seek( lua_State* L ) {
  static char const* const modenames[] = { "set", "cur", "end", NULL };
  memfile* mf = tomemfile( L, 1 );
  int op = luaL_checkoption( L, 2, "cur", modenames );
  lua_Integer offset = luaL_optinteger( L, 3, 0 );
  int ok = 1;
  switch( op ) {
    case 0: /* SEEK_SET */
      if( offset < 0 || offset > mf->n )
        ok = 0;
      else
        mf->p = offset;
      break;
    case 1: /* SEEK_CUR */
      if( (offset < 0 && -(offset+1) >= mf->p ) ||
          (offset >= 0 && offset > mf->n-mf->p) )
        ok = 0;
      else
        mf->p += offset;
      break;
    default: /* SEEK_END */
      if( offset > 0 ||
          (offset < 0 && -(offset+1) >= mf->n) )
        ok = 0;
      else
        mf->p = mf->n + offset;
      break;
  };
  if( ok ) {
    lua_pushinteger( L, (lua_Integer)mf->p );
    return 1;
  } else {
    lua_pushnil( L );
    lua_pushliteral( L, "invalid offset" );
    return 2;
  }
}


static int memfile_setvbuf( lua_State* L ) {
  static char const* const modenames[] = { "no", "full", "line", NULL };
  (void)tomemfile( L, 1 );
  (void)luaL_checkoption(L, 2, NULL, modenames );
  (void)luaL_optinteger( L, 3, 0 );
  /* there are no caches for memory buffers */
  lua_pushboolean( L, 1 );
  return 1;
}


static int memfile_write( lua_State* L ) {
  memfile* mf = luaL_checkudata( L, 1, NAME );
  int ok = 1;
  int arg = 2;
  int top = lua_gettop( L );
  if( (mf->flags & MEMFILE_W) == 0 ) {
    lua_pushnil( L );
    lua_pushliteral( L, "permission denied" );
    return 2;
  }
  for( arg = 2; arg <= top && ok; ++arg ) {
    size_t len = 0;
    char const* s = NULL;
    if( NULL == (s = lua_tolstring( L, arg, &len ) ) )
      s = luaL_checklstring( L, arg, &len );
    if( mf->p+len > mf->n ) {
      len = mf->n - mf->p;
      ok = 0;
    }
    memcpy( mf->addr + mf->p, s, len );
    mf->p += len;
  }
  lua_settop( L, 1 );
  if( ok )
    return 1;
  else {
    lua_pushnil( L );
    lua_pushliteral( L, "not enough space" );
    return 2;
  }
}


static int memfile_addr( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  lua_pushlightuserdata( L, mf->addr );
  return 1;
}


static int memfile_size( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  lua_pushinteger( L, mf->n );
  return 1;
}


static int memfile_truncate( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  size_t sz = luaL_checkinteger( L, 2 );
  if( sz > mf->n ) {
    lua_pushnil( L );
    lua_pushliteral( L, "new size too large" );
    return 2;
  }
  mf->n = sz;
  if( mf->p > sz )
    mf->p = sz;
  lua_pushboolean( L, 1 );
  return 1;
}


static int memfile_close( lua_State* L ) {
  memfile* mf = tomemfile( L, 1 );
  int top;
  lua_getuservalue( L, 1 );
  top = lua_gettop( L );
  lua_rawgeti( L, -1, CLOSE_INDEX );
  lua_rawgeti( L, -2, OBJECT_INDEX );
  if( lua_type( L, -2 ) == LUA_TFUNCTION )
    lua_call( L, 1, LUA_MULTRET );
  else {
    lua_pop( L, 2 );
    lua_pushboolean( L, 1 );
  }
  mf->addr = NULL;
  luaL_checkstack( L, 1, "not enough stack space" );
#if LUA_VERSION_NUM > 501
  lua_pushnil( L );
#else
  lua_pushvalue( L, LUA_GLOBALSINDEX );
#endif
  lua_setuservalue( L, 1 );
  return lua_gettop( L )-top;
}


static int memfile_gc( lua_State* L ) {
  memfile* mf = lua_touserdata( L, 1 );
  if( mf->addr != NULL ) {
    lua_getuservalue( L, 1 );
    lua_rawgeti( L, -1, CLOSE_INDEX );
    lua_rawgeti( L, -2, OBJECT_INDEX );
    if( lua_type( L, -2 ) == LUA_TFUNCTION )
      lua_call( L, 1, 0 );
  }
  return 0;
}


IPC_LOCAL void* memfile_udata( lua_State* L, int idx,
                               char const* name ) {
  int i = lua_absindex( L, idx );
  (void)tomemfile( L, i );
  lua_getuservalue( L, i );
  lua_rawgeti( L, -1, OBJECT_INDEX );
  lua_replace( L, i ); /* replace memfile object */
  lua_pop( L, 1 ); /* remove uservalue table */
  return luaL_checkudata( L, i, name );
}


IPC_LOCAL void memfile_new( lua_State* L, void* addr, size_t n,
                            int perms, int oidx, int closeidx,
                            int flushidx ) {
  memfile* mf = NULL;
  oidx = lua_absindex( L, oidx );
  closeidx = lua_absindex( L, closeidx );
  flushidx = lua_absindex( L, flushidx );
  mf = lua_newuserdata( L, sizeof( *mf ) );
  mf->addr = addr;
  mf->n = n;
  mf->p = 0;
  mf->flags = perms;
  lua_newtable( L );
  if( oidx ) {
    lua_pushvalue( L, oidx );
    lua_rawseti( L, -2, OBJECT_INDEX );
  }
  if( closeidx ) {
    lua_pushvalue( L, closeidx );
    lua_rawseti( L, -2, CLOSE_INDEX );
  }
  if( flushidx ) {
    lua_pushvalue(L, flushidx );
    lua_rawseti( L, -2, FLUSH_INDEX );
  }
  lua_setuservalue( L, -2 );
  if( luaL_newmetatable( L, NAME ) ) {
    luaL_Reg const methods[] = {
      { "flush", memfile_flush },
      { "lines", memfile_lines },
      { "read", memfile_read },
      { "seek", memfile_seek },
      { "setvbuf", memfile_setvbuf },
      { "write", memfile_write },
      { "addr", memfile_addr },
      { "size", memfile_size },
      { "truncate", memfile_truncate },
      { "close", memfile_close },
      { NULL, NULL }
    };
    luaL_newlib( L, methods );
    lua_setfield( L, -2, "__index" );
    lua_pushcfunction( L, memfile_gc );
    lua_setfield( L, -2, "__gc" );
    lua_pushboolean( L, 0 );
    lua_setfield( L, -2, "__metatable" );
  }
  lua_setmetatable( L, -2 );
}


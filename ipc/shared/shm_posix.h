#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


typedef struct {
  char name[ NAME_MAX ];
  void* addr;
  size_t len;
} ipc_shm_handle;

static void* ipc_shm_addr( ipc_shm_handle* h ) {
  return h->addr;
}

static size_t ipc_shm_size( ipc_shm_handle* h ) {
  return h->len;
}


static void ipc_shm_error( char* buf, size_t len, int code ) {
  if( len > 0 && strerror_r( code, buf, len ) != (int)0 ) {
    strncpy( buf, "unknown error", len-1 );
    buf[ len-1 ] = '\0';
  }
}


/* the format of shm names is restricted (at least if you want to be
 * portable), so this functions checks the input and makes such a
 * portable name. */
static int ipc_shm_make_name_( char const* s, ipc_shm_handle* h ) {
  size_t ilen = strlen( s );
  if( ilen == 0 || ilen+2 > sizeof( h->name ) )
    return IPC_ERR( ENAMETOOLONG );
  if( strchr( s, '/' ) != NULL )
    return IPC_ERR( EINVAL );
  h->name[ 0 ] = '/';
  memcpy( h->name+1, s, ilen+1 );
  return 0;
}


static int ipc_shm_create( ipc_shm_handle* h, char const* name,
                           size_t req ) {
  int fd, rv, flags = O_RDWR|O_CREAT|O_EXCL;
  rv = ipc_shm_make_name_( name, h );
  if( rv != 0 )
    return IPC_ERR( rv );
  if( req == 0 )
    return IPC_ERR( EINVAL );
  /* open(create) shm object */
#ifdef O_CLOEXEC
  flags |= O_CLOEXEC;
#endif
  fd = shm_open( h->name, flags, S_IRUSR|S_IWUSR );
  if( fd < 0 )
    return IPC_ERR( errno );
  /* resize to requested size */
  if( ftruncate( fd, (off_t)req ) < 0 ) {
    int saved_errno = errno;
    close( fd );
    shm_unlink( h->name );
    return IPC_ERR( saved_errno );
  }
  h->len = req;
  /* create mmap */
  h->addr = mmap( NULL, req, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  if( h->addr == MAP_FAILED ) {
    int saved_errno = errno;
    close( fd );
    shm_unlink( h->name );
    return IPC_ERR( saved_errno );
  }
  close( fd ); /* we don't need it anymore! */
  return 0;
}


static int ipc_shm_attach( ipc_shm_handle* h, char const* name ) {
  int fd, rv, flags = O_RDWR;
  struct stat buf;
  rv = ipc_shm_make_name_( name, h );
  if( rv != 0 )
    return IPC_ERR( rv );
  /* open shared memory object */
#ifdef O_CLOEXEC
  flags |= O_CLOEXEC;
#endif
  fd = shm_open( h->name, flags, S_IRUSR|S_IWUSR );
  if( fd < 0 )
    return IPC_ERR( errno );
  /* figure out its size */
  if( fstat( fd, &buf ) < 0 ) {
    int saved_errno = errno;
    close( fd );
    return IPC_ERR( saved_errno );
  }
  if( buf.st_size > (size_t)-1 ) {
    close( fd );
    return IPC_ERR( EFBIG );
  }
  h->len = buf.st_size;
  /* create mmap */
  h->addr = mmap( NULL, h->len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
  if( h->addr == MAP_FAILED ) {
    int saved_errno = errno;
    close( fd );
    return IPC_ERR( saved_errno );
  }
  close( fd ); /* we don't need it anymore! */
  return 0;
}


static int ipc_shm_detach( ipc_shm_handle* h ) {
  if( munmap( h->addr, h->len ) < 0 )
    return IPC_ERR( errno );
  return 0;
}


static int ipc_shm_remove( ipc_shm_handle* h ) {
  if( munmap( h->addr, h->len ) < 0 ) {
    int saved_errno = errno;
    shm_unlink( h->name ); /* try to delete it anyway */
    return IPC_ERR( saved_errno );
  }
  shm_unlink( h->name );
  return 0;
}


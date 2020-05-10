#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


typedef off_t ipc_mmap_off_t;


typedef struct {
  void* addr;
  size_t len;
} ipc_mmap_handle;

static void* ipc_mmap_addr( ipc_mmap_handle* h ) {
  return h->addr;
}

static size_t ipc_mmap_size( ipc_mmap_handle* h ) {
  return h->len;
}


static void ipc_mmap_error( char* buf, size_t len, int code ) {
  if( len > 0 && strerror_r( code, buf, len ) != (int)0 ) {
    strncpy( buf, "unknown error", len-1 );
    buf[ len-1 ] = '\0';
  }
}


static size_t ipc_mmap_pagesize( void ) {
  long result = sysconf( _SC_PAGESIZE );
  if( result < 1 )
    result = 4096;
  return (size_t)result;
}


static int ipc_mmap_open( ipc_mmap_handle* h, char const* name,
                          int mode, off_t offset, size_t size ) {
  int fd, oflags = 0, mmflags = 0;
  if( (mode & MEMFILE_RW) == MEMFILE_RW ) {
    oflags = O_RDWR;
    mmflags = PROT_READ | PROT_WRITE;
  } else if( mode & MEMFILE_R ) {
    oflags = O_RDONLY;
    mmflags = PROT_READ;
  } else if( mode & MEMFILE_W ) {
    oflags = O_RDWR;
    mmflags = PROT_WRITE;
  }
#ifdef O_CLOEXEC
  oflags |= O_CLOEXEC;
#endif
  fd = open( name, oflags );
  if( fd < 0 )
    return IPC_ERR( errno );
  h->len = size;
  if( size == 0 ) { /* figure out its size */
    struct stat buf;
    if( fstat( fd, &buf ) < 0 ) {
      int saved_errno = errno;
      close( fd );
      return IPC_ERR( saved_errno );
    }
    if( buf.st_size < offset ) {
      close( fd );
      return IPC_ERR( EINVAL );
    }
    if( buf.st_size - offset > (size_t)-1 )
      h->len = (size_t)-1;
    else
      h->len = buf.st_size - offset;
  }
  /* create mmap */
  h->addr = mmap( NULL, h->len, mmflags, MAP_SHARED, fd, offset );
  if( h->addr == MAP_FAILED ) {
    int saved_errno = errno;
    close( fd );
    return IPC_ERR( saved_errno );
  }
  close( fd ); /* we don't need it anymore! */
  return 0;
}


static int ipc_mmap_close( ipc_mmap_handle* h ) {
  int rv = munmap( h->addr, h->len );
  if( rv < 0 )
    return IPC_ERR( errno );
  return 0;
}


#if defined( _POSIX_SYNCHRONIZED_IO ) && _POSIX_SYNCHRONIZED_IO > 0
#  define IPC_MMAP_HAVE_FLUSH
static int ipc_mmap_flush( ipc_mmap_handle* h, size_t pos ) {
  int rv = msync( h->addr, pos, MS_ASYNC|MS_INVALIDATE );
  if( rv < 0 )
    return IPC_ERR( errno );
  return 0;
}
#endif


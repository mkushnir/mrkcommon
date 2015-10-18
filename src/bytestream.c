#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include "diag.h"
#include "mrkcommon/util.h"
//#define TRRET_DEBUG
#include "mrkcommon/dumpm.h"
#include "mrkcommon/bytestream.h"

#ifdef DO_MEMDEBUG
#include <mrkcommon/memdebug.h>
MEMDEBUG_DECLARE(bytestream);

#define MEMDEBUG_INIT(self)                                    \
do {                                                           \
    (self)->mdtag = (uint64_t)memdebug_get_runtime_scope();    \
} while (0)                                                    \


#define MEMDEBUG_ENTER(self)                                   \
{                                                              \
    int mdtag;                                                 \
    mdtag = memdebug_set_runtime_scope((int)(self)->mdtag);    \


#define MEMDEBUG_LEAVE(self)                   \
    (void)memdebug_set_runtime_scope(mdtag);   \
}                                              \


#else
#define MEMDEBUG_INIT(self)
#define MEMDEBUG_ENTER(self)
#define MEMDEBUG_LEAVE(self)
#endif

int
bytestream_dump(bytestream_t *stream)
{
    TRACE("stream pos=%ld eod=%ld sz=%ld",
          stream->pos, stream->eod, stream->buf.sz);
    TRACE("stream start");
    D16(stream->buf.data, MIN(stream->eod, 128));
    return 0;
}




int
bytestream_init(bytestream_t *stream, ssize_t growsz)
{
    stream->buf.sz = growsz;
    stream->growsz = growsz;
    MEMDEBUG_INIT(stream);
    MEMDEBUG_ENTER(stream);
    if (growsz > 0) {
        if ((stream->buf.data = malloc(stream->buf.sz)) == NULL) {
            TRRET(BYTESTREAM_INIT + 1);
        }
    } else {
        stream->buf.data = NULL;
    }
    MEMDEBUG_LEAVE(stream);
    stream->eod = 0;
    stream->pos = 0;
    stream->read_more = NULL;
    stream->write = NULL;
    stream->udata = NULL;
    return 0;
}


int
bytestream_grow(bytestream_t *stream, size_t incr)
{
    char *tmp;
    MEMDEBUG_ENTER(stream);
    if ((tmp = realloc(stream->buf.data,
                         stream->buf.sz + incr)) == NULL) {
        TRRET(BYTESTREAM_GROW + 1);
    }
    MEMDEBUG_LEAVE(stream);
    stream->buf.data = tmp;
    stream->buf.sz += incr;
    return 0;
}


ssize_t
bytestream_read_more(bytestream_t *stream, int fd, ssize_t sz)
{
    ssize_t nread;
    ssize_t need;
    need = (stream->eod + sz) - stream->buf.sz;
    if (need > 0) {
        if (bytestream_grow(stream,
                   (need < stream->growsz) ?
                   stream->growsz :
                   need) != 0) {
            return -1;
        }
    }
    if ((nread = read(fd, stream->buf.data + stream->eod, sz)) >= 0) {
        stream->eod += nread;
    }
    return nread;
}


ssize_t
bytestream_recv_more(bytestream_t *stream, int fd, ssize_t sz)
{
    ssize_t nrecv;
    ssize_t need;
    need = (stream->eod + sz) - stream->buf.sz;
    if (need > 0) {
        if (bytestream_grow(stream,
                   (need < stream->growsz) ?
                    stream->growsz :
                    need) != 0) {
            return -1;
        }
    }
    if ((nrecv = recv(fd,
                      stream->buf.data + stream->eod,
                      (size_t)sz,
                      0)) >= 0) {
        stream->eod += nrecv;
    }
    return nrecv;
}


ssize_t
bytestream_write(bytestream_t *stream, int fd, size_t sz)
{
    ssize_t nwritten;

    if ((stream->pos + (ssize_t)sz) > stream->eod) {
        return -1;

    }

    nwritten = write(fd, stream->buf.data + stream->pos, sz);
    stream->pos += nwritten;

    return nwritten;
}

ssize_t
bytestream_stderr_write(bytestream_t *stream, int fd, size_t sz)
{
    ssize_t nwritten;

    fd = 2;

    if ((stream->pos + (ssize_t)sz) > stream->eod) {
        return -1;

    }

    nwritten = write(fd, stream->buf.data + stream->pos, sz);
    stream->pos += nwritten;

    if (write(fd, "\n", 1)) {;}

    return nwritten;
}

ssize_t
bytestream_send(bytestream_t *stream, int fd, size_t sz)
{
    ssize_t nwritten;

    if ((stream->pos + (ssize_t)sz) > stream->eod) {
        return -1;

    }

    nwritten = send(fd, stream->buf.data + stream->pos, sz, 0);
    stream->pos += nwritten;

    return nwritten;
}

int
bytestream_consume_data(bytestream_t *stream, int fd)
{
    ssize_t nread;
    ssize_t need;

    assert(stream->read_more != NULL);

    need = (stream->pos + stream->growsz) - stream->eod;
    nread = stream->read_more(stream, fd, need);

    if (nread == 0) {
        /* eof */
        TRRET(-1);

    } else if (nread < 0) {
        TRRET(BYTESTREAM_CONSUME_DATA + 2);
    }

    return 0;
}

int
bytestream_produce_data(bytestream_t *stream, int fd)
{
    assert(stream->write != NULL);

    if (stream->write(stream, fd, SEOD(stream) - SPOS(stream)) < 0) {
        TRRET(BYTESTREAM_PRODUCE_DATA + 1);
    }

    return 0;
}


int PRINTFLIKE(3, 4)
bytestream_nprintf(bytestream_t *stream,
                   size_t sz,
                   const char *fmt, ...)
{
    int nused;
    ssize_t need;
    va_list ap;
    need = (stream->eod + sz) - stream->buf.sz;
    if (need > 0) {
        if (bytestream_grow(stream,
                   (need < stream->growsz) ?
                   stream->growsz :
                   need) != 0) {
            TRRET(BYTESTREAM_NPRINTF_ERROR);
        }
    }
    va_start(ap, fmt);
    nused = vsnprintf(SDATA(stream, stream->eod), sz, fmt, ap);
    va_end(ap);
    if (nused >= ((ssize_t)sz)) {
        TRRET(BYTESTREAM_NPRINTF_NEEDNORE);
    }
    stream->eod += nused;
    return nused;
}


int
bytestream_cat(bytestream_t *stream, size_t sz, const char *data)
{
    ssize_t need;
    need = (stream->eod + sz) - stream->buf.sz;
    if (need > 0) {
        if (bytestream_grow(stream,
                   (need < stream->growsz) ?
                   stream->growsz :
                   need) != 0) {
            TRRET(-127);
        }
    }
    memcpy(SDATA(stream, stream->eod), data, sz);
    stream->eod += sz;
    return (int)sz;
}


void
bytestream_rewind(bytestream_t *stream)
{
    stream->pos = 0;
    stream->eod = 0;
}

off_t
bytestream_recycle(bytestream_t *stream, int ngrowsz, off_t from)
{
    int pgsz;

#ifdef PAGE_SIZE
    pgsz = PAGE_SIZE;
#else
    pgsz = getpagesize();
#endif
    from -= (from % pgsz);

    if (from > (stream->growsz * ngrowsz)) {
        memmove(stream->buf.data,
                stream->buf.data + from,
                stream->eod - from);
        stream->eod -= from;
        stream->pos -= from;
        return from;
    }

    return 0;
}


void
bytestream_fini(bytestream_t *stream)
{
    MEMDEBUG_ENTER(stream);
    if (stream->buf.data != NULL) {
        free(stream->buf.data);
        stream->buf.data = NULL;
    }
    MEMDEBUG_LEAVE(stream);
    stream->read_more = NULL;
    stream->write = NULL;
    stream->udata = NULL;
}

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <mrkcommon/mpool.h>
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>

#include "diag.h"

//#define MPOOL_USE_STD_MALLOC

void
mpool_ctx_reset(mpool_ctx_t *mpool)
{
    mpool->last_chunk = MAX(mpool->last_chunk, mpool->current_chunk);
    mpool->current_chunk = 0;
    mpool->current_pos = 0;
}


#ifdef MPOOL_USE_STD_MALLOC
UNUSED
#endif
static void **
new_chunk(mpool_ctx_t *mpool, size_t sz)
{
    void **chunk;

    if (mpool->current_chunk >= mpool->last_chunk) {
        ++mpool->current_chunk;
        if (mpool->current_chunk * sizeof(void *) == mpool->arenasz) {
            mpool->arenasz *= 2;
            mpool->arena = realloc(mpool->arena, mpool->arenasz);
            assert(mpool->arena != NULL);
        }

        chunk = mpool->arena + mpool->current_chunk;
        *chunk = malloc(sz);
        assert(*chunk != NULL);
    } else {
        ++mpool->current_chunk;
        chunk = mpool->arena + mpool->current_chunk;
    }
    mpool->current_pos = 0;
    return chunk;
}

void *
mpool_malloc(UNUSED mpool_ctx_t *mpool, size_t sz)
{
#ifdef MPOOL_USE_STD_MALLOC
    return malloc(sz);
#else
    size_t sz1;
    struct _mpool_item *res;

    sz = sz + 8 - (sz % 8);
    sz1 = sz + sizeof(struct _mpool_item);

    if (sz1 > mpool->chunksz) {
        void **chunk;

        /* new "big" chunk */
        chunk = new_chunk(mpool, sz1);
        res = (struct _mpool_item *)*chunk;
        res->sz = sz;
        (void)new_chunk(mpool, mpool->chunksz);

    } else {
        void **chunk;
        size_t nleft;

        nleft = mpool->chunksz - mpool->current_pos;

        if (nleft < sz1) {
            chunk = new_chunk(mpool, mpool->chunksz);
        } else {
            chunk = mpool->arena + mpool->current_chunk;
        }

        res = (struct _mpool_item *)(*chunk + mpool->current_pos);
        res->sz = sz;
        mpool->current_pos += sz1;
    }
//#ifndef NDEBUG
//    memset(res->data, 0xa5, res->sz);
//#endif
    //TRACE("m>>> %p sz=%ld in chunk %d pool %p",
    //      res->data,
    //      res->sz,
    //      mpool->current_chunk,
    //      mpool);
    return res->data;
#endif
}

#define DATA_TO_MPOOL_ITEM(d) ((struct _mpool_item *)(((char *)(d)) - sizeof(size_t)))

void *
mpool_realloc(UNUSED mpool_ctx_t *mpool, void *p, size_t sz)
{
#ifdef MPOOL_USE_STD_MALLOC
    return realloc(p, sz);
#else
    struct _mpool_item *mpi;

    if (p == NULL) {
        p = mpool_malloc(mpool, sz);
    } else {
        mpi = DATA_TO_MPOOL_ITEM(p);
        if (mpi->sz < sz) {
            void *pp;

            pp = mpool_malloc(mpool, sz);
            memcpy(pp, p, mpi->sz);
            p = pp;
        } else {
#ifndef NDEBUG
            memset(mpi->data + sz, 0x5a, mpi->sz - sz);
#endif
        }
    }
    return p;
#endif
}


void
mpool_free(UNUSED mpool_ctx_t *mpool, UNUSED void *o)
{
#ifdef MPOOL_USE_STD_MALLOC
    free(o);
#else
    UNUSED struct _mpool_item *mpi;

    mpi = DATA_TO_MPOOL_ITEM(o);
#ifndef NDEBUG
    memset(mpi->data, 0x5a, mpi->sz);
    //TRACE("f<<< %p", o);
#endif
#endif
}

int
mpool_ctx_init(mpool_ctx_t *mpool, size_t chunksz)
{
    mpool->chunksz = chunksz;
    mpool->current_chunk = 0;
    mpool->last_chunk = 0;
    mpool->current_pos = 0;
    mpool->arenasz = sizeof(void *);
    mpool->arena = malloc(mpool->arenasz);
    assert(mpool->arena != NULL);
    mpool->arena[0] = malloc(mpool->chunksz);
    assert(mpool->arena[0] != NULL);
    return 0;
}


void
mpool_ctx_dump_info(mpool_ctx_t *mpool)
{
    TRACE("%ld chunks of %ld bytes chrrent %d/%ld last %d",
          mpool->arenasz / sizeof(void *),
          mpool->chunksz,
          mpool->current_chunk,
          mpool->current_pos,
          mpool->last_chunk);
}


int
mpool_ctx_fini(mpool_ctx_t *mpool)
{
    int i;

    mpool->last_chunk = MAX(mpool->last_chunk, mpool->current_chunk);

    for (i = mpool->last_chunk; i >= 0; --i) {
        void *chunk;
        chunk = mpool->arena[i];
        assert(chunk != NULL);
        free(chunk);
    }
    free(mpool->arena);
    mpool->arena = NULL;
    return 0;
}

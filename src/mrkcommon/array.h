#ifndef MRKCOMMON_ARRAY_H
#define MRKCOMMON_ARRAY_H

#include <sys/types.h>

typedef int (*array_initializer_t) (void *);
typedef int (*array_finalizer_t) (void *);
typedef int (*array_traverser_t) (void *, void *);
typedef int (*array_compar_t) (const void *, const void *);

/**/
typedef struct _array {
    size_t elsz;
    size_t elnum;
    void *data;
    array_initializer_t init;
    array_finalizer_t fini;
    array_compar_t compar;
} array_t;

typedef struct _array_iter {
    unsigned iter;
    void *data;
} array_iter_t;

int array_init(array_t *, size_t, size_t,
               array_initializer_t,
               array_finalizer_t);

#define ARRAY_FLAG_SAVE 0x01
int array_ensure_len(array_t *, size_t, unsigned int);
void *array_get(array_t *, unsigned);
int array_index(array_t *, void *);
void *array_get_iter(array_t *, array_iter_t *);
int array_clear_item(array_t *, unsigned);
void *array_incr(array_t *);
void *array_incr_iter(array_t *, array_iter_t *);
int array_decr(array_t *);
int array_fini(array_t *);
void *array_first(array_t *, array_iter_t *);
void *array_last(array_t *, array_iter_t *);
void *array_next(array_t *, array_iter_t *);
void *array_prev(array_t *, array_iter_t *);
int array_sort(array_t *);
void *array_find(array_t *, const void *);
int array_traverse(array_t *, array_traverser_t, void *);
int array_cmp(array_t *, array_t *, array_compar_t, ssize_t);

#endif

#ifndef CMEME_C
#define CMEME_C // meme lol

#include "cosmo.h"

#include "cstate.h"

//#define GC_STRESS
//#define GC_DEBUG
// arrays will grow by a factor of 2
#define GROW_FACTOR 2
#define HEAP_GROW_FACTOR 2
#define ARRAY_START 8

#define cosmoM_freearray(state, type, buf, capacity) \
    cosmoM_reallocate(state, buf, sizeof(type)  *capacity, 0)

#define cosmoM_growarray(state, type, buf, count, capacity) \
    if (count >= capacity || buf == NULL) { \
        int old = capacity; \
        capacity = old  *GROW_FACTOR; \
        buf = (type*)cosmoM_reallocate(state, buf, sizeof(type)  *old, sizeof(type)  *capacity); \
    }

#define cosmoM_free(state, type, x) \
    cosmoM_reallocate(state, x, sizeof(type), 0)

#define cosmoM_isFrozen(state) \
    state->freezeGC > 0

#define cosmoM_freezeGC(state) \
    state->freezeGC++

#define  cosmoM_unfreezeGC(state) \
    state->freezeGC--

COSMO_API void *cosmoM_reallocate(CState* state, void *buf, size_t oldSize, size_t newSize);
COSMO_API void cosmoM_collectGarbage(CState* state);

/*
    wrapper for cosmoM_reallocate so we can track our memory usage (it's also safer :P)
*/
static inline void *cosmoM_xmalloc(CState *state, size_t sz) {
    return cosmoM_reallocate(state, NULL, 0, sz);
}

#endif
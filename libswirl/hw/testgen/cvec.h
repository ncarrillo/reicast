//
// Created by RadDad772 on 3/20/24.
//

#ifndef JSMOOCH_EMUS_CVEC_H
#define JSMOOCH_EMUS_CVEC_H

#include "types.h"

struct cvec {
    void *data;
    u32 data_sz;
    u32 len;
    u32 len_allocated;
    u32 realloc_locked;

    // For use by outsiders
    s32 kind;
    void *app_ptr;
};

struct cvec_iterator {
    struct cvec* vec;
    u32 index;
};

void cvec_init(struct cvec* self, u32 data_size, u32 prealloc);
void cvec_delete(struct cvec* self);
u32 cvec_len(struct cvec* self);
void *cvec_push_back(struct cvec* self);
void *cvec_pop_back(struct cvec* self);
void cvec_clear(struct cvec* self);
void *cvec_get(struct cvec* self, u32 index);
void *cvec_get_unsafe(struct cvec* self, u32 index);
void cvec_lock_reallocs(struct cvec* self); // For locking reallocs to preserve pointers
void cvec_push_back_copy(struct cvec* self, void *src);

void cvec_iterator_init(struct cvec_iterator* self);

#endif //JSMOOCH_EMUS_CVEC_H

//
// Created by RadDad772 on 3/20/24.
//

#include <string.h>
#include "stdlib.h"
#include "stdio.h"
#include "assert.h"

#include "cvec.h"

void cvec_lock_reallocs(struct cvec* self)
{
    self->realloc_locked = 1;
}

void cvec_init(struct cvec* self, u32 data_size, u32 prealloc)
{
    self->data = NULL;
    self->data_sz = data_size;
    self->len_allocated = 0;
    self->realloc_locked = 0;
    self->len = 0;

    if (prealloc != 0) {
        self->len_allocated = prealloc * 2;
        self->data = malloc(data_size * self->len_allocated);
    }

}

void cvec_delete(struct cvec* self)
{
    if (self->data != NULL) {
        free(self->data);
    }
    self->realloc_locked = 0;
    self->data = NULL;
    self->len_allocated = 0;
    self->len = 0;
}

u32 cvec_len(struct cvec* self) {
    return self->len;
}

void *cvec_push_back(struct cvec* self)
{
    //printf("\nPUSH BACK! %d", self->len);
    if (self->len >= self->len_allocated) {
        if (self->len_allocated == 0) {
            assert(!self->realloc_locked);
            self->len_allocated = 20;
            self->data = malloc(self->data_sz * self->len_allocated);
        }
        else {
            assert(!self->realloc_locked);
            self->len_allocated = self->len_allocated * 2;
            u32 sz = self->data_sz * self->len_allocated;
            if (sz > (16*1024*1024)) {
                printf("\nWARNING REALLOC ARRAY TO SIZE %d", sz);
            }
            void *mnew = realloc(self->data, sz);
            assert(mnew!=NULL);
            self->data = mnew;
        }
    }
    void *ret = (void *)((u8*)self->data + (self->len * self->data_sz));
    self->len++;
    return ret;
}

void *cvec_pop_back(struct cvec* self)
{
    if (self->len == 0) return NULL;
    return (void *)((u8*)self->data + (self->len-- * self->data_sz));
}

void cvec_clear(struct cvec* self)
{
    self->len = 0;
}

void *cvec_get(struct cvec* self, u32 index)
{
    assert(index < self->len);
    return (void *)((u8*)self->data + (self->data_sz * index));
}

void *cvec_get_unsafe(struct cvec* self, u32 index)
{
    assert(index < self->len_allocated);
    return (void *)((u8 *)self->data + (self->data_sz * index));
}

void cvec_push_back_copy(struct cvec* self, void *src)
{
    void *dst = cvec_push_back(self);
    memcpy(dst, src, self->data_sz);
}

//
// Created by . on 4/22/24.
//
#include "types.h"

#ifndef REICAST_RD_DEBUG_H
#define REICAST_RD_DEBUG_H

struct jsm_string {
    char *ptr; // Pointer
    char *cur; // Current location in string
    u32 len;
};


struct jsm_debug_struct {
    u32 first_flush;

    u32 trace_on;
    struct jsm_string msg;
    char *msg_last_newline;

    u32 var;

    u64 trace_cycles;
};

struct jsm_debug_read_trace {
    void *ptr;
    u32 (*read_trace)(void *,u32);
};

extern struct jsm_debug_struct dbg;

void jsm_string_init(struct jsm_string *str, u32 size);
void jsm_string_sprintf(struct jsm_string *str, const char* format, ...);
void jsm_string_empty(struct jsm_string *str);

void rdbg_printf(const char *format, ...);
void rdbg_init();
void rdbg_seek_in_line(u32 pos);
void rdbg_flush();
void rdbg_clear_msg();

void rdbg_enable_trace();
void rdbg_disable_trace();
void rdbg_cycle();


#endif //REICAST_RD_DEBUG_H

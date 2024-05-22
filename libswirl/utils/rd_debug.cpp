//
// Created by . on 4/22/24.
//

#include <pwd.h>

#include <cassert>
#include <cstdlib>
#include <unistd.h>

#include "rd_debug.h"

struct jsm_debug_struct dbg;

static u32 init_already = 0;

void rdbg_init()
{
    if (init_already) {
        printf("\nINIT ALREADY!");
        return;
    }
    init_already = 1;
    dbg.first_flush = 1;
    printf("\nDBG INIT!");
    dbg.trace_on = 0;
    jsm_string_init(&dbg.msg, 5*1024*1024);
    dbg.msg_last_newline = dbg.msg.ptr;
    dbg.var = 0;
}

void rdbg_printf(const char *format, ...)
{
    if (!dbg.trace_on) return;
    struct jsm_string*self = &dbg.msg;
    // do jsm_sprintf, basically, minus one line
    if (self->ptr == nullptr) {
        assert(1==0);
        return;
    }
    va_list va;
    va_start(va, format);
    self->cur += vsnprintf(self->cur, self->len - (self->cur - self->ptr), format, va);
    va_end(va);

    // Scan for newlines
    /*while(*(self->cur)!=0) {
        if (*(self->cur) == '\n')
            dbg.msg_last_newline = self->cur;
        self->cur++;
    }*/
    if ((self->len - (self->cur - self->ptr)) < 4000) {
        rdbg_flush();
    }
}

static void construct_path(char* w, const char* who)
{
    const char *homeDir = getenv("HOME");

    if (!homeDir) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd)
            homeDir = pwd->pw_dir;
    }

    char *tp = w;
    tp += sprintf(tp, "%s/%s", homeDir, who);
}

char fpath[250];

void rdbg_clear_msg()
{
    jsm_string_empty(&dbg.msg);
    dbg.msg_last_newline = dbg.msg.cur;
}

void rdbg_flush()
{
    if (!dbg.trace_on) return;
    if (dbg.first_flush) {
        construct_path(fpath, "dev/reicast.log");
        dbg.first_flush = 0;
        remove(fpath);
    }
    FILE *w = fopen(fpath, "a");
    fprintf(w, "%s", dbg.msg.ptr);
    fflush(w);
    fclose(w);
    rdbg_clear_msg();
}


void rdbg_enable_trace()
{
    dbg.trace_on = true;
}

void rdbg_disable_trace()
{
    dbg.trace_on = false;
}

void rdbg_cycle()
{
    dbg.trace_cycles++;
}

void jsm_string_init(struct jsm_string *self, u32 sz)
{
    self->ptr = self->cur = nullptr;
    self->len = sz;
    self->ptr = self->cur = (char *)malloc(sz);
    memset(self->ptr, 0, sz);
}
void jsm_string_empty(struct jsm_string *self)
{
    if (self->ptr == nullptr) {
        assert(1==0);
        return;
    }
    self->cur = self->ptr;
    memset(self->ptr, 0, self->len);
}
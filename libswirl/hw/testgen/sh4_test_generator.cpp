//
// Created by RadDad772 on 4/9/24.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <assert.h>
#include "myrandom.h"
//#include "component/cpu/sh4/sh4_interpreter.h"
//#include "component/cpu/sh4/sh4_interpreter_opcodes.h"
#include "cvec.h"
#include "types.h"
#include "multisize_memaccess.c"
#include "../sh4/sh4_if.h"
//#include "../sh4/sh4_core.h"
#include "../sh4/sh4_interpreter.h"


void UpdateFPSCR();
bool UpdateSR();

typedef s64 i64;

static void cpSH4(u32 dest, u32 src) {
    /*memcpy(&SH4_decoded[dest], &SH4_decoded[src], 65536*sizeof(struct SH4_ins_t));
    memcpy(&SH4_disassembled[dest][0], &SH4_disassembled[src][0], 65536*30);
    memcpy(&SH4_mnemonic[dest][0], &SH4_mnemonic[src][0], 65535*30);*/
}

struct sh4_str_ret
{
    u32 n_max, m_max, d_max, i_max;
    u32 n_shift, m_shift, d_shift, i_shift;
    u32 n_mask, m_mask, d_mask, i_mask;
    u32 mask;
};

struct mem_pair {
    u32 addr;
    u32 val;
    u32 rw;
};

struct SH4_tester{
    SH4IInterpreter cpu;
} ;

struct SH4_FV {
    float a, b, c, d;
};

struct SH4_mtx {
    float xf0, xf1, xf2, xf3;
    float xf4, xf5, xf6, xf7;
    float xf8, xf9, xf10, xf11;
    float xf12, xf13, xf14, xf15;
};

struct SH4_test_state {
    u32 PC;   // PC
    u32 GBR;
    u32 SR;
    u32 SSR;
    u32 SPC;
    u32 VBR;
    u32 SGR;
    u32 DBR;
    u32 MACL;
    u32 MACH;
    u32 PR;
    u32 FPSCR;
    u32 FPUL;

    u32 R[16]; // "foreground" registers
    u32 R_[8]; // banked registers
    union {
        u32 U32[16];
        u64 U64[8];
        float FP32[16];
        float FP64[8];
        struct SH4_FV FV[4];
        struct SH4_mtx MTX;
    } fb[3]; // floating-point banked registers, 2 banks + a third for swaps
};

#define NUMTESTS 500

#define TCA_READ 1
#define TCA_WRITE 2
#define TCA_FETCHINS 4

struct test_cycle {
    u32 actions;

    u32 read_addr;
    u64 read_val;

    u32 write_addr;
    u64 write_val;

    u32 fetch_addr;
    u32 fetch_val;
};


static void process_SH4_instruct(struct sh4_str_ret *r, const char* stri)
{
    memset(r, 0, sizeof(struct sh4_str_ret));

    enum ddd{
        nothing,
        in_d,
        in_n,
        in_m,
        in_i
    } doing, new_doing;

    doing = nothing;
    new_doing = nothing;
    u32 val = 0x10000;

    for (u32 i = 0; i < 16; i++) {
        val >>= 1;
        u8 chr = stri[i];
        u32 *max = NULL;
        u32 *msk = NULL;
        if ((chr == '0') || (chr == '1')) {
            if (chr == '1') r->mask |= val;
            switch(doing) {
                case nothing:
                    break;
                case in_d:
                    r->d_shift = 16 - i;
                    break;
                case in_m:
                    r->m_shift = 16 - i;
                    break;
                case in_n:
                    r->n_shift = 16 - i;
                    break;
                case in_i:
                    r->i_shift = 16 - i;
            }
            doing = nothing;
            continue;
        }
        if (chr == 'n') {
            max = &r->n_max;
            msk = &r->n_mask;
            new_doing = in_n;
        }
        else if (chr == 'm') {
            max = &r->m_max;
            msk = &r->m_mask;
            new_doing = in_m;
        }
        else if (chr == 'i') {
            max = &r->i_max;
            msk = &r->i_mask;
            new_doing = in_i;
        }
        else {//if (chr == 'd') {
            max = &r->d_max;
            msk = &r->d_mask;
            new_doing = in_d;
        }
        *msk = ((*msk) << 1) | 1;
        *max = (*msk) + 1;
        if (new_doing != doing) {
            switch(doing) {
                case nothing:
                    break;
                case in_d:
                    r->d_shift = 16 - i;
                    break;
                case in_m:
                    r->m_shift = 16 - i;
                    break;
                case in_n:
                    r->n_shift = 16 - i;
                    break;
                case in_i:
                    r->i_shift = 16 - i;
            }
        }
        doing = new_doing;
    }
}

static void clear_test_cycle(struct test_cycle* self)
{
    self->actions = 0;
    self->read_addr = self->write_addr = self->fetch_addr = -1;
    self->read_val = self->write_val = self->fetch_val = 0;
}

struct sh4test {
    struct SH4_test_state initial;
    struct SH4_test_state final;
    u16 opcodes[5]; // 0-3 are the flow, #5 is for after the jump
    u32 test_base;
    struct test_cycle cycles[4];
};

struct sh4test_array {
    struct SH4_ins_t *ins;
    char fname[40];
    u32 sz, pr;
    char encoding[17];
    struct sh4test tests[NUMTESTS];
};

// 237 encodings with SZ=0 PR=0
//  15 encodings with PR=1 SZ=1
//  10 encodings with PR=1 SZ=0, which are copied into PR=1 SZ=1 also
//=262 encodings
static struct sh4test_array sh4tests[262];

static bool str_is_displaced12(const char* str)
{
    if (strcmp(str, "1010dddddddddddd") == 0) return true;
    return false;
}

static bool str_is_displaced8(const char* str)
{
    if (strcmp(str, "10001111dddddddd") == 0) return true;
    // i1000_1111_iiii_iiii
    return false;
    //if (strcmp(str, ))
}


static u32 encoding_to_mask(const char* encoding_str, u32 m, u32 n, u32 d, u32 i)
{
    struct sh4_str_ret r;
    process_SH4_instruct(&r, encoding_str);
    d &= r.d_mask;
    m &= r.m_mask;
    n &= r.n_mask;
    i &= r.i_mask;
    if (str_is_displaced12(encoding_str)) {
        if (d > 0xFF0) d -= 0x10;
    }
    if (str_is_displaced8(encoding_str)) {
        if (d > 0xF0) d -= 0x10;
    }
    m = (m & r.m_mask) << r.m_shift;
    n = (n & r.n_mask) << r.n_shift;
    d = (d & r.d_mask) << r.d_shift;
    i = (i & r.i_mask) << r.i_shift;
    u32 bit = 1 << 15;
    u32 out = 0;
    for (u32 l = 0; l < 16; l++) {
        if (encoding_str[l] == '1') {
            out |= bit;
        }
        bit >>= 1;
    }

    //if (r.m_max) out |= (m << r.m_shift);

    out |= m | n | d | i;

    return out;
}

static u32 valid_rnd_SR(struct sfc32_state *rstate)
{
    u32 SR = sfc32(rstate) & 0b1110000000000001000001111110011;
    /*
     u32 MD; // bit 30. 1 = privileged modeRB, BL, FD, M, Q, IMASK, S, T;
    u32 RB; //     29. register bank select (privileged only)
*
     */
    if (((SR >> 30) & 1) == 0) { // if MD = 0
        if (((SR >> 29) & 1) == 1) { // if RB = 1
            SR &= 0b1010000000000001000001111110011;

        }
    }
    return SR;
}

static u32 valid_rnd_FPSCR(struct sfc32_state *rstate, u32 sz, u32 pr)
{
    u32 FPSCR = sfc32(rstate) & 0b1001000000000000000001; // 0b1111000000000000000001
    FPSCR |= (sz << 20) | (pr << 19);
    return FPSCR;
}

static void random_initial(struct SH4_test_state *ts, struct sfc32_state *rstate, u32 sz, u32 pr)
{
    for (u32 i = 0; i < 16; i++) {
        ts->R[i] = sfc32(rstate);
        if (i < 8) ts->R_[i] = sfc32(rstate);
        ts->fb[0].U32[i] = sfc32(rstate);
        ts->fb[1].U32[i] = sfc32(rstate);
    }
#define sR(w) ts-> w = sfc32(rstate)
    sR(PC);
    ts->PC &= 0xFFFFFFFE;
    sR(GBR);
    sR(SR);
    sR(SSR);
    sR(SPC);
    ts->SPC &= 0xFFFFFFFE;
    sR(VBR);
    sR(SGR);
    sR(DBR);
    sR(MACL);
    sR(MACH);
    sR(PR);
    ts->PR &= 0xFFFFFFFE;
    ts->SR = valid_rnd_SR(rstate);
    ts->FPSCR = valid_rnd_FPSCR(rstate, sz, pr);
#undef sR
}

static u32 randomize_opcode(const char* encoding_str, struct sfc32_state *rstate)
{
    u32 m = sfc32(rstate), n = sfc32(rstate), d = sfc32(rstate), i = sfc32(rstate);
    return encoding_to_mask(encoding_str, m, n, d, i);
}

static void SH4_SR_set(struct SH4IInterpreter *sh4, u32 val)
{
    //Sh4cntx.sr.T = val;
    p_sh4rcb->cntx.sr.status = val & 0x700083F2;
    p_sh4rcb->cntx.sr.T = val & 1;
    UpdateSR();
}

static void SH4_FPSCR_set(struct SH4IInterpreter *sh4, u32 val)
{
    Sh4cntx.fpscr.full = val;
    UpdateFPSCR();
}

static void copy_state_to_cpu(struct SH4_test_state *st, struct SH4IInterpreter *sh4)
{

    SH4_SR_set(sh4, st->SR);
    SH4_FPSCR_set(sh4, st->FPSCR);

#define CP(rn) st->rn
    for (u32 i = 0; i < 16; i++) {
        Sh4cntx.r[i] = st->R[i];
        if (i < 8) Sh4cntx.r_bank[i] = st->R_[i];
        Sh4cntx.xffr[i+16] = st->fb[0].FP32[i];
        Sh4cntx.xffr[i] = st->fb[1].FP32[i];
    }
    Sh4cntx.pc = CP(PC);
    Sh4cntx.gbr = CP(GBR);
    Sh4cntx.ssr = CP(SSR);
    Sh4cntx.spc = CP(SPC);
    Sh4cntx.vbr = CP(VBR);
    Sh4cntx.sgr = CP(SGR);
    Sh4cntx.dbr = CP(DBR);
    Sh4cntx.mac.l = CP(MACL);
    Sh4cntx.mac.h = CP(MACH);
    Sh4cntx.pr = CP(PR);
    Sh4cntx.fpul = CP(FPUL);
#undef CP
}

static void copy_state_from_cpu(struct SH4_test_state *st, struct SH4IInterpreter *sh4)
{
    st->SR = (p_sh4rcb->cntx.sr.status & 0x700083F2) | p_sh4rcb->cntx.sr.T;
    st->FPSCR = Sh4cntx.fpscr.full;

#define CP(rn, rm) st-> rn = Sh4cntx. rm
    for (u32 i = 0; i < 16; i++) {
        st->R[i] = Sh4cntx.r[i];
        if (i < 8) st->R_[i] = Sh4cntx.r_bank[i];
        st->fb[0].FP32[i] = Sh4cntx.xffr[i+16];
        st->fb[1].FP32[i] = Sh4cntx.xffr[i];
    }
    CP(PC, pc);
    CP(GBR, gbr);
    CP(SSR, ssr);
    CP(SPC, spc);
    CP(VBR, vbr);
    CP(SGR, sgr);
    CP(DBR, dbr);
    CP(MACL, mac.l);
    CP(MACH, mac.h);
    CP(PR, pr);
    CP(FPUL, fpul);
#undef CP
}

struct generating_test_struct {
    struct SH4_tester *tester;
    struct sh4test *test;
    struct sfc32_state *rstate;

    i64 read_addrs[7];
    u64 read_values[7];
    i64 read_sizes[7];
    u64 read_cycles[7];

    i64 write_addr;
    u64 write_value;
    i64 write_size;
    u64 write_cycle;

    i64 ifetch_addr[4];
    u32 ifetch_data[4];
    u32 ifetch_num;

    u32 read_num;
};

static struct generating_test_struct test_struct = {};

u32 test_fetch_ins(u32 addr)
{
    assert(SH4IInterpreter::trace_cycles < 5);
    test_struct.ifetch_addr[SH4IInterpreter::trace_cycles] = addr;
    i64 num = ((i64)addr - (i64)test_struct.test->test_base) / 2;
    u32 v;
    if ((num >= 0) && (num < 4)) v = test_struct.test->opcodes[num];
    else v = test_struct.test->opcodes[4];
    //printf("\n  n: %lld v:%04x c:%d cl:%d", num, v, SH4IInterpreter::trace_cycles, SH4IInterpreter::cycles_left);
    //fflush(stdout);
    test_struct.ifetch_data[SH4IInterpreter::trace_cycles] = v;
    return v;
}

u64 test_read(u32 addr, u32 sz)
{
    assert(test_struct.read_num < 7);
    u64 v = (u64)sfc32(test_struct.rstate) | ((u64)sfc32(test_struct.rstate) << 32);

    switch(sz) {
        case 1:
            v &= 0xFF;
            break;
        case 2:
            v &= 0xFFFF;
            break;
        case 4:
            v &= 0xFFFFFFFF;
            break;
        case 8:
            break;
        default:
            assert(1==0);
            return 0;
    }
    test_struct.read_addrs[test_struct.read_num] = addr;
    test_struct.read_sizes[test_struct.read_num] = sz;
    test_struct.read_values[test_struct.read_num] = v;
    test_struct.read_cycles[test_struct.read_num] = SH4IInterpreter::trace_cycles;
    test_struct.read_num++;
    return v;
}

void test_write(u32 addr, u64 val, u32 sz)
{
    assert(test_struct.write_addr==-1);
    test_struct.write_addr = addr;
    test_struct.write_value = val;
    test_struct.write_size = sz;
    test_struct.write_cycle = SH4IInterpreter::trace_cycles;
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
    tp += sprintf(tp, "%s/dev/%s", homeDir, who);
}

#define TB_NONE 0
#define TB_INITIAL_STATE 1
#define TB_FINAL_STATE 2
#define TB_CYCLES 3
#define TB_OPCODES 4

#define M8 1
#define M16 2
#define M32 4
#define M64 8

static u32 write_cycles(u8 *where, const struct sh4test *test)
{
    cW[M32](where, 4, TB_CYCLES);
    cW[M32](where, 8, 4);
    u32 r = 12;
#define W32(v) cW[M32](where, r, v); r += 4
#define W64(v) cW[M64](where, r, (u64)(v)); r += 8
    for (u32 cycle = 0; cycle < 4; cycle++) {
        const struct test_cycle *c = &test->cycles[cycle];
        W32((u32)c->actions);

        W32((u32)c->fetch_addr);
        W32((u32)c->fetch_val);

        W32((u32)c->write_addr);
        W64(c->write_val);

        W32((u32)c->read_addr);
        W64(c->read_val);
    }
    cW[M32](where, 0, r);
#undef W32
#undef W64
    return r;
}

static u32 write_state(u8* where, struct SH4_test_state *state, u32 is_final)
{
    cW[M32](where, 4, is_final ? TB_FINAL_STATE : TB_INITIAL_STATE);

    u32 r = 8;
    // R0-R15 offset 8
#define W32(val) cW[M32](where, r, state-> val); r += 4
    for (u32 i = 0; i < 16; i++) {
        W32(R[i]);
    }
    // R_0-R_7
    for (u32 i = 0; i < 8; i++) {
        W32(R_[i]);
    }
    // FP bank 0 0-15, and bank 1 0-15
    for (u32 b = 0; b < 2; b++) {
        for (u32 i = 0; i < 16; i++) {
            W32(fb[b].U32[i]);
        }
    }
    W32(PC);
    W32(GBR);
    W32(SR);
    W32(SSR);
    W32(SPC);
    W32(VBR);
    W32(SGR);
    W32(DBR);
    W32(MACL);
    W32(MACH);
    W32(PR);
    W32(FPSCR);
    W32(FPUL);
#undef W32
    // Write size of block
    cW[M32](where, 0, r);
    return r;
}

static u32 write_opcodes(u8* where, struct sh4test *test)
{
    cW[M32](where, 4, TB_OPCODES);
    u32 r = 8;
#define W32(val) cW[M32](where, r, val); r += 4
    W32((u32)test->opcodes[0]);
    W32((u32)test->opcodes[1]);
    W32((u32)test->opcodes[2]);
    W32((u32)test->opcodes[3]);
    W32((u32)test->opcodes[4]);
#undef W32
    cW[M32](where, 0, r);
    return r;
}

static u8 outbuf[3 * 1024 * 1024];

static void write_tests(struct sh4test_array *ta)
{
    char fpath[250];
    char rp[250];
    sprintf(rp, "sh4_json/%s", ta->fname);
    construct_path(fpath, rp);
    remove(fpath);

    FILE *f = fopen(fpath, "wb");

    u32 outbuf_idx = 0;
    for (u32 tnum = 0; tnum < NUMTESTS; tnum++) {
        struct sh4test *t = &ta->tests[tnum];
        // Write out initial state, final state
        u32 outbuf_start = outbuf_idx;
        outbuf_idx += 4;
        outbuf_idx += write_state(&outbuf[outbuf_idx], &t->initial, 0); // 272 bytes
        outbuf_idx += write_state(&outbuf[outbuf_idx], &t->final, 1); // 272 bytes
        outbuf_idx += write_cycles(&outbuf[outbuf_idx], t);
        outbuf_idx += write_opcodes(&outbuf[outbuf_idx], t);
        cW[M32](outbuf, outbuf_start, outbuf_idx - outbuf_start);
    }
    fwrite(outbuf, 1, outbuf_idx, f);

    fclose(f);
}

#define NUM_SKIP 3
static char SKIP_TESTS[NUM_SKIP][17] = {
        "0000nnnn10000011", // PREF (8 writes)
        "0000nnnnmmmm1111", // MACL (2 reads)
        "0100nnnnmmmm1111", // MACW (2 reads)
};

static void generate_test_struct(const char* encoding_str, const char* mnemonic, u32 sz, u32 pr, u32 num, struct SH4_tester *t)
{
    for (u32 i = 0; i < NUM_SKIP; i++) {
        if (strcmp(encoding_str, SKIP_TESTS[i]) == 0) {
            printf("\nSKIPPING TEST!");
            return;
        }
    }
    struct sh4test_array *ta = &sh4tests[num];
    printf("\nGENERATING STRUCT FOR %s", encoding_str);
    snprintf(ta->fname, sizeof(ta->fname), "%s_sz%d_pr%d.json.bin", encoding_str, sz, pr);
    snprintf(ta->encoding, sizeof(ta->encoding), "%s", encoding_str);

    ta->sz = sz;
    ta->pr = pr;
    struct sfc32_state rstate;
    sfc32_seed(ta->fname, &rstate);
    for (u32 i = 0; i < 20; i++) { // Prime the random generator, first few values are low-entropy
        sfc32(&rstate);
    }

    test_struct.rstate = &rstate;
    test_struct.tester = t;

    for (u32 i = 0; i < NUMTESTS; i++) {
        struct sh4test *tst = &ta->tests[i];
        random_initial(&tst->initial, &rstate, sz, pr);
        tst->test_base = tst->initial.PC;
        tst->opcodes[0] = 9; // NOP
        tst->opcodes[1] = randomize_opcode(encoding_str, &rstate); // opcode we're testing

        tst->opcodes[2] = 0b0011000100011100; // add R1, R1. So R1 += R1
        tst->opcodes[3] = 9; // NOP again
        tst->opcodes[4] = 0b0011001000101100; // add R2, R2. so R2 += R2. for after a jump
        copy_state_to_cpu(&tst->initial, &t->cpu);

        // Make sure our CPU isn't interrupted...
        //t->cpu.interrupt_highest_priority = 0;
        SH4IInterpreter::trace_cycles = 0;
        SH4IInterpreter::cycles_left = 0;

        // Zero our test struct
        test_struct.test = tst;
        test_struct.read_num = 0;
        test_struct.write_addr = test_struct.write_size = -1;
        test_struct.write_value = -1;
        test_struct.write_cycle = 50;
        test_struct.ifetch_num = 0;
        for (u32 j = 0; j < 7; j++) {
            if (j < 4) {
                test_struct.ifetch_addr[j] = -1;
                test_struct.ifetch_data[j] = 65536;

            }
            test_struct.read_addrs[j] = -1;
            test_struct.read_sizes[j] = -1;
            test_struct.read_values[j] = 0;
            test_struct.read_cycles[j] = 50;
        }

        // Run CPU 4 cycles
        // Our amazeballs CPU can run 2-for-1 so do it special!
        // ONLY opcode 1 could have a delay slot so all should finish
        t->cpu.Loop();
        assert(SH4IInterpreter::trace_cycles == 4);
        assert(SH4IInterpreter::cycles_left == 0);

        for (u32 cycle = 0; cycle < 4; cycle++) {
            struct test_cycle *c = &tst->cycles[cycle];
            clear_test_cycle(c);
            if ((cycle == 1)  && (test_struct.write_cycle != 50)) {
                c->actions |= TCA_WRITE;
                c->write_addr = test_struct.write_addr;
                c->write_val = test_struct.write_value;
            }
            for (u32 j = 0; j < 7; j++) {
                if ((test_struct.read_cycles[j] != 50) && (cycle == 1)) {
                    assert((c->actions & TCA_READ) == 0);
                    c->actions |= TCA_READ;
                    c->read_addr = test_struct.read_addrs[j];
                    c->read_val = test_struct.read_values[j];
                }
            }
            c->actions |= TCA_FETCHINS;
            c->fetch_addr = test_struct.ifetch_addr[cycle];
            c->fetch_val = test_struct.ifetch_data[cycle];
            assert(c->fetch_addr <= 0xFFFFFFFF);
            assert(c->fetch_addr >= 0);
            assert(c->fetch_val <= 0xFFFF);
        }
        // Now fill out rest of test
        copy_state_from_cpu(&tst->final, &t->cpu);
    }
    write_tests(ta);
}

void generate_sh4_tests()
{
    struct SH4_tester t;
    u32 r = 0;
#define OE(opcstr, func, mn) generate_test_struct(opcstr, mn, 0, 0, r, &t); r++
#define OEo(opcstr, func, mn, sz, pr) generate_test_struct(opcstr, mn, sz, pr, r, & t); r++

#include "sh4_decodings.c"
    //OE("0000000000001001", &SH4_NOP, "nop"); // No operation

#undef OE
#undef OEo

}
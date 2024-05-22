#include "license/bsd"
#include "types.h"
#include "../sh4_interpreter.h"
#include "../sh4_interrupts.h"
#include "../sh4_opcode_list.h"
#include "../sh4_core.h"
#include "hw/sh4/sh4_mem.h"
#include "utils/rd_debug.h"

// Function prototypes
#define CPU_RATIO (1)
void ExecuteOpcode(u16 op)
{
    if (sr.FD == 1 && OpDesc[op]->IsFloatingPoint())
        RaiseFPUDisableException();
    OpPtr[op](op);
}

void ExecuteDelayslot_RTE()
{
#if !defined(NO_MMU)
    try {
#endif
        ExecuteDelayslot();
#if !defined(NO_MMU)
    }
    catch (SH4ThrownException& ex) {
        msgboxf("Exception in RTE delay slot", MBX_ICONERROR);
    }
#endif
}

#include "../sh4_interpreter.h"

SH4IInterpreter::~SH4IInterpreter() {
    //Term();
}
u32 test_fetch_ins(u32 addr);
u64 test_read(u32 addr, u32 sz);
void test_write(u32 addr, u64 val, u32 sz);

void SH4IInterpreter::Loop() {

// Definition of SH4IInterpreter
    l = SH4_TIMESLICE;
    SH4IInterpreter::cycles_left = 4;

#if !defined(NO_MMU)
    try {
#endif
    do {
        u32 addr = next_pc;
        next_pc += 2;
        u32 op = IReadMem16(addr);

        SH4IInterpreter::cycles_left--;
        SH4IInterpreter::trace_cycles++;
        ExecuteOpcode(op);
        //rdbg_cycle();
        l -= 1;
    } while (SH4IInterpreter::cycles_left > 0);

    l += SH4_TIMESLICE;

    if (UpdateSystem()) {
        UpdateINTC();
    }
#if !defined(NO_MMU)
    }
    catch (SH4ThrownException& ex) {
        Do_Exception(ex.epc, ex.expEvn, ex.callVect);
        l -= CPU_RATIO * 5;
    }
#endif
}

bool SH4IInterpreter::Init() { rdbg_init(); return true; }
void SH4IInterpreter::ClearCache() {

}

s64 SH4IInterpreter::til_traces = 0;
s32 SH4IInterpreter::l = 0;
bool SH4IInterpreter::tracing = false;
s64 SH4IInterpreter::tracesLeft = 0;
s32 SH4IInterpreter::cycles_left = 0;
u32 SH4IInterpreter::trace_cycles = 0;

SuperH4Backend* Get_Sh4Interpreter()
{
    return new SH4IInterpreter();
}

void ExecuteDelayslot()
{
#if !defined(NO_MMU)
    try {
#endif
        u32 addr = next_pc;
        next_pc += 2;
        u32 op = IReadMem16(addr);

        if (SH4IInterpreter::tracing && SH4IInterpreter::tracesLeft > 0) {
            // Output next_pc
            rdbg_printf("\n%08x %04x ", addr, ((u16)op & 0xFFFF));

            // Output registers r0 to r15
            for (int i = 0; i < 16; ++i) {
                rdbg_printf("%08x ", r[i]);
            }

            rdbg_printf("%08x", sr.status | sr.T);
            rdbg_printf(" %08x", fpscr.full);
            SH4IInterpreter::tracesLeft--;
        }

        if (op != 0)
            rdbg_cycle();
            ExecuteOpcode(op);
            SH4IInterpreter::cycles_left--;
            SH4IInterpreter::trace_cycles++;
#if !defined(NO_MMU)
    }
    catch (SH4ThrownException& ex) {
        ex.epc -= 2;
        if (ex.expEvn == 0x800)
            ex.expEvn = 0x820;
        else if (ex.expEvn == 0x180)
            ex.expEvn = 0x1A0;
        throw ex;
    }
#endif
}
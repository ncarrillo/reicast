#include "license/bsd"
#include "types.h"
#include "../sh4_interpreter.h"
#include "../sh4_interrupts.h"
#include "../sh4_opcode_list.h"
#include "../sh4_core.h"
#include "hw/sh4/sh4_mem.h"

// Function prototypes
void ExecuteOpcode(u16 op);
void ExecuteDelayslot();
void ExecuteDelayslot_RTE();
SuperH4Backend* Get_Sh4Interpreter();

#define CPU_RATIO (8)

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

// Definition of SH4IInterpreter
struct SH4IInterpreter : SuperH4Backend {
    static s32 l;
    static bool tracing;
    static s64 tracesLeft;
    static u64 totalsCounted;

    ~SH4IInterpreter() { Term(); }
    void Loop() {
        l = SH4_TIMESLICE;

        do {
#if !defined(NO_MMU)
            try {
#endif
                do {
                    u32 addr = next_pc;
                    next_pc += 2;
                    u32 op = IReadMem16(addr);

                    //if (addr == 0xa0000000) {
                    if (addr == 0x8c09b770) {
                        if (!tracing) {
                            SH4IInterpreter::tracing = true;
                            SH4IInterpreter::tracesLeft = 100000;
                        }
                    }

                    if (SH4IInterpreter::tracing && SH4IInterpreter::tracesLeft > 0) {
                        // pc
                        printf("%08x %04x ", addr, ((u16)op & 0xFFFF));

                        // r0-r15
                        for (int i = 0; i < 16; ++i) {
                            printf("%08x ", r[i]);
                        }

                        // sr
                        printf("%08x", sr.status | sr.T);
                        printf(" %08x", fpscr.full);

                        printf("\n");
                        tracesLeft--;

                        if (tracesLeft == 0) {
                            fflush(stdout);
                        }
                    }
                    
                    SH4IInterpreter::totalsCounted += 1;
                    ExecuteOpcode(op);
                    l -= CPU_RATIO;
                } while (l > 0);

                l += SH4_TIMESLICE;

                if (UpdateSystem()) {
                    UpdateINTC();
                    if (!sh4_int_bCpuRun)
                        break;
                }
#if !defined(NO_MMU)
            }
            catch (SH4ThrownException& ex) {
                Do_Exception(ex.epc, ex.expEvn, ex.callVect);
                l -= CPU_RATIO * 5;
            }
#endif
        } while (true);
    }

    bool Init() { return true; }
    void Term() { }
    void ClearCache() { }
};

s32 SH4IInterpreter::l = 0;
bool SH4IInterpreter::tracing = false;
s64 SH4IInterpreter::tracesLeft = 0;
u64 SH4IInterpreter::totalsCounted = 0;

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
            printf("%08x %04x ", addr, ((u16)op & 0xFFFF));

            // Output registers r0 to r15
            for (int i = 0; i < 16; ++i) {
                printf("%08x ", r[i]);
            }

            printf("%08x", sr.status | sr.T);
            printf(" %08x", fpscr.full);
            printf("\n");
            SH4IInterpreter::tracesLeft--;
        }

        SH4IInterpreter::totalsCounted += 1;
        if (op != 0)
            ExecuteOpcode(op);
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
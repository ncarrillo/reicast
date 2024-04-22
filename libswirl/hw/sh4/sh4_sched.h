/*
	This file is part of libswirl
*/
#include "license/bsd"


#pragma once

#include "types.h"

/*
	tag, as passed on sh4_sched_register
	sch_cycles, the cycle duration that the callback requested (sh4_sched_request)
	jitter, the number of cycles that the callback was delayed, [0... 448]
*/
typedef int sh4_sched_callback(void* context, int tag, int sch_cycl, int jitter);

/*
	Registed a callback to the scheduler. The returned id 
	is used for sh4_sched_request and sh4_sched_elapsed calls
*/
int sh4_sched_register(void* context, int tag, sh4_sched_callback* ssc);

/*
	current time in SH4 cycles, referenced to boot.
	Wraps every ~21 secs
*/
u32 sh4_sched_now();

/*
	current time, in SH4 cycles, referenced to boot.
	Does not wrap, 64 bits.
*/
u64 sh4_sched_now64();

/*
	Schedule a callback to be called sh4 *cycles* after the
	invocation of this function. *Cycles* range is (0, 200M].
	
	Passing a value of 0 disables the callback.
	If called multiple times, only the last call is in effect
*/
void sh4_sched_request(int id, int cycles);

/*
	Returns how much time has passed for this callback
*/
int sh4_sched_elapsed(int id);

/*
	Tick for *cycles*
*/
void sh4_sched_tick(int cycles);

void sh4_sched_ffts();

void sh4_sched_cleanup();

void sh4_sched_serialize(void** data, unsigned int* total_size);
void sh4_sched_unserialize(void** data, unsigned int* total_size);

extern u32 sh4_sched_intr;

struct sched_list
{
	sh4_sched_callback* cb;
	void* context;
	int tag;
	int start;
	int end;
};
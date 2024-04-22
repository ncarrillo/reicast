/*
	This file is part of libswirl
*/
#include "license/bsd"



#include "types.h"
#include "sh4_interrupts.h"
#include "sh4_core.h"
#include "sh4_sched.h"
#include "oslib/oslib.h"
#include "serialize.h"


//sh4 scheduler

/*

	register handler
	request callback at time

	single fire events only

	sh4_sched_register(id)
	sh4_sched_request(id, in_cycles)

	sh4_sched_now()

*/
u64 sh4_sched_ffb;
u32 sh4_sched_intr;


vector<sched_list> sch_list;	// using list as external inside a macro confuses clang and msc

int sh4_sched_next_id=-1;

u32 sh4_sched_remaining(int id, u32 reference)
{
	if (sch_list[id].end != -1)
	{
		return sch_list[id].end - reference;
	}
	else
	{
		return -1;
	}
}

u32 sh4_sched_remaining(int id)
{
	return sh4_sched_remaining(id, sh4_sched_now());
}

void sh4_sched_ffts()
{
	u32 diff=-1;
	int slot=-1;

	for (int i=0;i<sch_list.size();i++)
	{
		if (sh4_sched_remaining(i)<diff)
		{
			slot=i;
			diff=sh4_sched_remaining(i);
		}
	}

	sh4_sched_ffb-=Sh4cntx.sh4_sched_next;

	sh4_sched_next_id=slot;
	if (slot!=-1)
	{
		Sh4cntx.sh4_sched_next=diff;
	}
	else
	{
		Sh4cntx.sh4_sched_next=SH4_MAIN_CLOCK;
	}

	sh4_sched_ffb+=Sh4cntx.sh4_sched_next;
}

int sh4_sched_register(void* context, int tag, sh4_sched_callback* ssc)
{
	sched_list t={ssc,context, tag,-1,-1};

	sch_list.push_back(t);

	return (int)(sch_list.size()-1);
}

/*
	Return current cycle count, in 32 bits (wraps after 21 dreamcast seconds)
*/
u32 sh4_sched_now()
{
	return (u32)(sh4_sched_ffb-Sh4cntx.sh4_sched_next);
}

/*
	Return current cycle count, in 64 bits (effectivelly never wraps)
*/
u64 sh4_sched_now64()
{
	return sh4_sched_ffb-Sh4cntx.sh4_sched_next;
}
void sh4_sched_request(int id, int cycles)
{
	verify(cycles== -1 || (cycles >= 0 && cycles <= SH4_MAIN_CLOCK));

	sch_list[id].start=sh4_sched_now();

	if (cycles == -1) {
		sch_list[id].end = -1;
	}
	else
	{
		sch_list[id].end = sch_list[id].start + cycles;
		if (sch_list[id].end == -1)
			sch_list[id].end++;
	}

	sh4_sched_ffts();
}

int sh4_sched_elapsed(int id)
{
	if (sch_list[id].end!=-1)
	{
		int rv=sh4_sched_now()-sch_list[id].start;
		sch_list[id].start=sh4_sched_now();
		return rv;
	}
	else
		return -1;
}

void handle_cb(int id)
{
	int remain=sch_list[id].end-sch_list[id].start;
	int elapsd=sh4_sched_elapsed(id);
	int jitter=elapsd-remain;

	sch_list[id].end=-1;
	int re_sch=sch_list[id].cb(sch_list[id].context, sch_list[id].tag,remain,jitter);

	if (re_sch > 0)
		sh4_sched_request(id, std::max(0, re_sch - jitter));
}

void sh4_sched_tick(int cycles)
{
	/*
	Sh4cntx.sh4_sched_time+=cycles;
	Sh4cntx.sh4_sched_next-=cycles;
	*/

	if (Sh4cntx.sh4_sched_next<0)
	{
		u32 fztime=sh4_sched_now()-cycles;
		sh4_sched_intr++;
		if (sh4_sched_next_id!=-1)
		{
			for (int i=0;i<sch_list.size();i++)
			{
				int remaining = sh4_sched_remaining(i, fztime);
				verify(remaining >= 0 || remaining == -1);
				if (remaining >= 0 && remaining <= (u32)cycles) {
					handle_cb(i);
				}
			}
		}
		sh4_sched_ffts();
	}
}

void sh4_sched_cleanup() {
	sch_list.clear();
	sh4_sched_next_id = -1;
	sh4_sched_ffb = 0;
	sh4_sched_intr = 0;
}

void sh4_sched_serialize(void** data, unsigned int* total_size) {
	REICAST_S(sh4_sched_next_id);
	REICAST_S(sh4_sched_ffb);
	REICAST_S(sh4_sched_intr);

	for (auto& entry : sch_list) {
		REICAST_S(entry.tag);
		REICAST_S(entry.start);
		REICAST_S(entry.end);
	}
}

void sh4_sched_unserialize(void** data, unsigned int* total_size) {
	REICAST_US(sh4_sched_next_id);
	REICAST_US(sh4_sched_ffb);
	REICAST_US(sh4_sched_intr);

	for (auto& entry : sch_list) {
		REICAST_US(entry.tag);
		REICAST_US(entry.start);
		REICAST_US(entry.end);
	}
}

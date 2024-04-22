/*
	This file is part of libswirl
*/
#include "license/bsd"


//gah , ccn emulation
//CCN: Cache and TLB controller

#include "types.h"
#include "hw/sh4/sh4_mmr.h"
#include "types.h"
#include "ccn.h"
#include "../sh4_core.h"
#include "hw/pvr/pvr_mem.h"
#include "hw/mem/_vmem.h"
#include "mmu.h"
#include "../sh4_interpreter.h"
#include "modules.h"

//Types

#define printf_smc(...) // printf



u32 CCN_QACR_TR[2];

struct Sh4ModCcn_impl : Sh4ModCcn {
	SuperH4Mmr* sh4mmr;

	template<u32 idx>
	void CCN_QACR_write(u32 addr, u32 value)
	{
		SH4IO_REGN(CCN, CCN_QACR0_addr + idx * 4, 32) = value;
		//CCN_QACR[idx].reg_data=value;

		u32 area = ((CCN_QACR_type&)value).Area;

		CCN_QACR_TR[idx] = (area << 26) - 0xE0000000; //-0xE0000000 because 0xE0000000 is added on the translation again ...

		switch (area)
		{
		case 3:
			if (_nvmem_enabled())
				do_sqw_nommu = &do_sqw_nommu_area_3;
			else
				do_sqw_nommu = &do_sqw_nommu_area_3_nonvmem;
			break;

		case 4:
			do_sqw_nommu = (sqw_fp*)&TAWriteSQ;
			break;
		default: do_sqw_nommu = &do_sqw_nommu_full;
		}
	}

	void CCN_MMUCR_write(u32 addr, u32 value)
	{
		CCN_MMUCR_type temp;
		temp.reg_data = value;

		bool mmu_changed_state = temp.AT != CCN_MMUCR.AT;

		if (temp.TI != 0)
		{
			for (u32 i = 0; i < 4; i++)
				ITLB[i].Data.V = 0;

			for (u32 i = 0; i < 64; i++)
				UTLB[i].Data.V = 0;

			temp.TI = 0;
		}
		CCN_MMUCR = temp;

		if (mmu_changed_state)
		{
			//printf("<*******>MMU Enabled , ONLY SQ remaps work<*******>\n");
			mmu_set_state();
		}
	}

	void CCN_CCR_write(u32 addr, u32 value)
	{
		CCN_CCR_type temp;
		temp.reg_data = value;


		if (temp.ICI) {
			printf_smc("Sh4: i-cache invalidation %08X\n", curr_pc);

			if (settings.dynarec.SmcCheckLevel != FullCheck) {
				//TODO: Add skip/check vectors for Shikigami No Shiro II (uses ICI frequently)
				//which game is 0xAC13DBF8 from ?
				if (curr_pc != 0xAC13DBF8)
				{
					printf("Sh4: code cache clear (ICI) pc: %08X\n", curr_pc);
					sh4_cpu->ResetCache();
				}
			}
		}

		temp.ICI = 0;
		temp.OCI = 0;

		CCN_CCR = temp;
	}

	u32 CPU_VERSION_read(u32 addr)
	{
		return 0x040205c1;	// this is what a real SH7091 in a Dreamcast returns - the later Naomi BIOSes check and care!
	}

	u32 CCN_PRR_read(u32 addr)
	{
		return 0;
	}

	//Init/Res/Term
	Sh4ModCcn_impl(SuperH4Mmr* sh4mmr) : sh4mmr(sh4mmr)
	{
		//CCN PTEH 0xFF000000 0x1F000000 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_PTEH_addr, RIO_DATA, 32);

		//CCN PTEL 0xFF000004 0x1F000004 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_PTEL_addr, RIO_DATA, 32);

		//CCN TTB 0xFF000008 0x1F000008 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_TTB_addr, RIO_DATA, 32);

		//CCN TEA 0xFF00000C 0x1F00000C 32 Undefined Held Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_TEA_addr, RIO_DATA, 32);

		//CCN MMUCR 0xFF000010 0x1F000010 32 0x00000000 0x00000000 Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_MMUCR_addr, RIO_WF, 32, 0, STATIC_FORWARD(Sh4ModCcn_impl, CCN_MMUCR_write));

		//CCN BASRA 0xFF000014 0x1F000014 8 Undefined Held Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_BASRA_addr, RIO_DATA, 8);

		//CCN BASRB 0xFF000018 0x1F000018 8 Undefined Held Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_BASRB_addr, RIO_DATA, 8);

		//CCN CCR 0xFF00001C 0x1F00001C 32 0x00000000 0x00000000 Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_CCR_addr, RIO_WF, 32, 0, STATIC_FORWARD(Sh4ModCcn_impl, CCN_CCR_write));

		//CCN TRA 0xFF000020 0x1F000020 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_TRA_addr, RIO_DATA, 32);

		//CCN EXPEVT 0xFF000024 0x1F000024 32 0x00000000 0x00000020 Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_EXPEVT_addr, RIO_DATA, 32);

		//CCN INTEVT 0xFF000028 0x1F000028 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_INTEVT_addr, RIO_DATA, 32);

		// CPU VERSION 0xFF000030 0x1F000030 (undocumented)
		sh4_rio_reg(this, CCN, CPU_VERSION_addr, RIO_RO_FUNC, 32, STATIC_FORWARD(Sh4ModCcn_impl, CPU_VERSION_read), 0);

		//CCN PTEA 0xFF000034 0x1F000034 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_PTEA_addr, RIO_DATA, 32);

		//CCN QACR0 0xFF000038 0x1F000038 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_QACR0_addr, RIO_WF, 32, 0, STATIC_FORWARD(Sh4ModCcn_impl, CCN_QACR_write<0>));

		//CCN QACR1 0xFF00003C 0x1F00003C 32 Undefined Undefined Held Held Iclk
		sh4_rio_reg(this, CCN, CCN_QACR1_addr, RIO_WF, 32, 0, STATIC_FORWARD(Sh4ModCcn_impl, CCN_QACR_write<1>));

		// CCN PRR 0xFF000044 0x1F000044 (undocumented)
		sh4_rio_reg(this, CCN, CCN_PRR_addr, RIO_RO_FUNC, 32, STATIC_FORWARD(Sh4ModCcn_impl, CCN_PRR_read), 0);
	}

	void Reset()
	{
		CCN_TRA = 0x0;
		CCN_EXPEVT = 0x0;
		CCN_MMUCR.reg_data = 0x0;
		CCN_CCR.reg_data = 0x0;
	}
};

Sh4ModCcn* Sh4ModCcn::Create(SuperH4Mmr* sh4mmr) {
	return new Sh4ModCcn_impl(sh4mmr);
}
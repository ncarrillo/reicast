/*
	This file is part of libswirl
*/
#include "license/bsd"


/*
	Sh4 internal register routing (P4 & 'area 7')
*/
#include "types.h"
#include <memory>

#include "sh4_mmr.h"
#include "hw/mem/_vmem.h"
#include "modules/mmu.h"
#include "modules/ccn.h"
#include "modules/modules.h"
#include "modules/sh4_mod.h"

#include "serialize.h"
//64bytes of sq // now on context ~



//All registers are 4 byte aligned


//***********
//Store Queue
//***********

//Mem map :)

//AREA 7--Sh4 Regs
_vmem_handler area7_handler;

_vmem_handler area7_orc_handler;


struct SuperH4Mmr_impl final : SuperH4Mmr
{
	Array<u8> OnChipRAM;

	template<u32 sz, typename ARR>
	u32 sh4_rio_read(ARR& sb_regs, u32 addr)
	{
		u32 offset = addr & 255;
#ifdef TRACE
		if (offset & 3/*(size-1)*/) //4 is min align size
		{
			EMUERROR("Unaligned System Bus register read");
		}
#endif

		offset >>= 2;

#ifdef TRACE
		if (sb_regs[offset].flags & sz)
		{
#endif
			if (!(sb_regs[offset].flags & REG_RF))
			{
				if (sz == 4)
					return  sb_regs[offset].data32;
				else if (sz == 2)
					return  sb_regs[offset].data16;
				else
					return  sb_regs[offset].data8;
			}
			else
			{
				return sb_regs[offset].readFunctionAddr(sb_regs[offset].context, addr);
			}
#ifdef TRACE
		}
		else
		{
			if (!(sb_regs[offset].flags & REG_NOT_IMPL))
				EMUERROR("ERROR [wrong size read on register]");
		}
#endif
		//	if ((sb_regs[offset].flags& REG_NOT_IMPL))
		//		EMUERROR2("Read from System Control Regs , not  implemented , addr=%x",addr);
		return 0;
	}

	template<u32 sz, typename ARR>
	void sh4_rio_write(ARR& sb_regs, u32 addr, u32 data)
	{
		u32 offset = addr & 255;
#ifdef TRACE
		if (offset & 3/*(size-1)*/) //4 is min align size
		{
			EMUERROR("Unaligned System bus register write");
		}
#endif
		offset >>= 2;
#ifdef TRACE
		if (sb_regs[offset].flags & sz)
		{
#endif
			if (!(sb_regs[offset].flags & REG_WF))
			{
				if (sz == 4)
					sb_regs[offset].data32 = data;
				else if (sz == 2)
					sb_regs[offset].data16 = (u16)data;
				else
					sb_regs[offset].data8 = (u8)data;
				return;
			}
			else
			{
				//printf("RSW: %08X\n",addr);
				sb_regs[offset].writeFunctionAddr(sb_regs[offset].context, addr, data);
				/*
				if (sb_regs[offset].flags & REG_CONST)
					EMUERROR("Error [Write to read only register , const]");
				else
				{
					if ()
					{
						sb_regs[offset].writeFunction(data);
						return;
					}
					else
					{
						if (!(sb_regs[offset].flags& REG_NOT_IMPL))
							EMUERROR("ERROR [Write to read only register]");
					}
				}*/
				return;
			}
#ifdef TRACE
		}
		else
		{
			if (!(sb_regs[offset].flags & REG_NOT_IMPL))
				EMUERROR4("ERROR: Wrong size write on register - offset=%x, data=%x, size=%d", offset, data, sz);
		}
		if ((sb_regs[offset].flags & REG_NOT_IMPL))
			EMUERROR3("Write to System Control Regs, not implemented - addr=%x, data=%x", addr, data);
#endif

	}

	//Region P4
	//Read P4
	template <u32 sz, class T>
	T ReadMem_P4(u32 addr)
	{
		/*if (((addr>>26)&0x7)==7)
		{
		return ReadMem_area7(addr,sz);
		}*/

		switch ((addr >> 24) & 0xFF)
		{

		case 0xE0:
		case 0xE1:
		case 0xE2:
		case 0xE3:
			EMUERROR("Unhandled p4 read [Store queue] 0x%x", addr);
			return 0;
			break;

		case 0xF0:
			return 0;
			break;

		case 0xF1:
			return 0;
			break;

		case 0xF2:
		{
			u32 entry = (addr >> 8) & 3;
			return ITLB[entry].Address.reg_data | (ITLB[entry].Data.V << 8);
		}
		break;

		case 0xF3:
		{
			u32 entry = (addr >> 8) & 3;
			return ITLB[entry].Data.reg_data;
		}
		break;

		case 0xF4:
		{
			//int W,Set,A;
			//W=(addr>>14)&1;
			//A=(addr>>3)&1;
			//Set=(addr>>5)&0xFF;
			//printf("Unhandled p4 read [Operand cache address array] %d:%d,%d  0x%x\n",Set,W,A,addr);
			return 0;
		}
		break;

		case 0xF5:
			return 0;
			break;

		case 0xF6:
		{
			u32 entry = (addr >> 8) & 63;
			u32 rv = UTLB[entry].Address.reg_data;
			rv |= UTLB[entry].Data.D << 9;
			rv |= UTLB[entry].Data.V << 8;
			return rv;
		}
		break;

		case 0xF7:
		{
			u32 entry = (addr >> 8) & 63;
			return UTLB[entry].Data.reg_data;
		}
		break;

		case 0xFF:
			EMUERROR("Unhandled p4 read [area7] 0x%x", addr);
			break;

		default:
			EMUERROR("Unhandled p4 read [Reserved] 0x%x", addr);
			break;
		}

		EMUERROR("Read from P4 not implemented - addr=%x", addr);
		return 0;

	}

	//Write P4
	template <u32 sz, class T>
	void WriteMem_P4(u32 addr, T data)
	{
		/*if (((addr>>26)&0x7)==7)
		{
		WriteMem_area7(addr,data,sz);
		return;
		}*/

		switch ((addr >> 24) & 0xFF)
		{

		case 0xE0:
		case 0xE1:
		case 0xE2:
		case 0xE3:
			EMUERROR("Unhandled p4 Write [Store queue] 0x%x", addr);
			break;

		case 0xF0:
			return;
			break;

		case 0xF1:
			return;
			break;

		case 0xF2:
		{
			u32 entry = (addr >> 8) & 3;
			ITLB[entry].Address.reg_data = data & 0xFFFFFCFF;
			ITLB[entry].Data.V = (data >> 8) & 1;
			ITLB_Sync(entry);
			return;
		}
		break;

		case 0xF3:
		{
			u32 entry = (addr >> 8) & 3;
			if (addr & 0x800000)
			{
				ITLB[entry].Assistance.reg_data = data & 0xf;
			}
			else
			{
				ITLB[entry].Data.reg_data = data;
			}
			ITLB_Sync(entry);

			return;
		}

		case 0xF4:
		{
			//int W,Set,A;
			//W=(addr>>14)&1;
			//A=(addr>>3)&1;
			//Set=(addr>>5)&0xFF;
			//printf("Unhandled p4 Write [Operand cache address array] %d:%d,%d  0x%x = %x\n",Set,W,A,addr,data);
			return;
		}
		break;

		case 0xF5:
			//printf("Unhandled p4 Write [Operand cache data array] 0x%x = %x\n",addr,data);
			return;
			break;

		case 0xF6:
		{
			if (addr & 0x80)
			{
#ifdef NO_MMU
				EMUERROR("Unhandled p4 Write [Unified TLB address array, Associative Write] 0x%x = %x", addr, data);
#endif

				CCN_PTEH_type t;
				t.reg_data = data;

#ifndef NO_MMU
				u32 va = t.VPN << 10;
#endif
                
				for (int i = 0; i < 64; i++)
				{
#ifndef NO_MMU
					if (mmu_match(va, UTLB[i].Address, UTLB[i].Data))
					{
						UTLB[i].Data.V = ((u32)data >> 8) & 1;
						UTLB[i].Data.D = ((u32)data >> 9) & 1;
						UTLB_Sync(i);
					}
#endif
				}

				for (int i = 0; i < 4; i++)
				{
#ifndef NO_MMU
					if (mmu_match(va, ITLB[i].Address, ITLB[i].Data))
					{
						ITLB[i].Data.V = ((u32)data >> 8) & 1;
						ITLB[i].Data.D = ((u32)data >> 9) & 1;
						ITLB_Sync(i);
					}
#endif
				}
			}
			else
			{
				u32 entry = (addr >> 8) & 63;
				UTLB[entry].Address.reg_data = data & 0xFFFFFCFF;
				UTLB[entry].Data.D = (data >> 9) & 1;
				UTLB[entry].Data.V = (data >> 8) & 1;
				UTLB_Sync(entry);
			}
			return;
		}
		break;

		case 0xF7:
		{
			u32 entry = (addr >> 8) & 63;
			if (addr & 0x800000)
			{
				UTLB[entry].Assistance.reg_data = data & 0xf;
			}
			else
			{
				UTLB[entry].Data.reg_data = data;
			}
			UTLB_Sync(entry);

			return;
		}

		case 0xFF:
			EMUERROR("Unhandled p4 Write [area7] 0x%x = %x", addr, data);
			break;

		default:
			EMUERROR("Unhandled p4 Write [Reserved] 0x%x", addr);
			break;
		}

		EMUERROR3("Write to P4 not implemented - addr=%x, data=%x", addr, data);
	}

	//***********
//**Area  7**
//***********
//Read Area7
	template <u32 sz, class T>
	T ReadMem_area7(u32 addr)
	{
		auto sh4mmr = this;
		/*
		if (likely(addr==0xffd80024))
		{
			return TMU_TCNT(2);
		}
		else if (likely(addr==0xFFD8000C))
		{
			return TMU_TCNT(0);
		}
		else */if (likely(addr == 0xFF000028))
		{
			return CCN_INTEVT;
		}
		else if (likely(addr == 0xFFA0002C))
		{
			return DMAC_CHCR(2).full;
		}
		//else if (addr==)

		//printf("%08X\n",addr);
		addr &= 0x1FFFFFFF;
		u32 map_base = addr >> 16;
		switch (map_base & 0x1FFF)
		{
		case A7_REG_HASH(CCN_BASE_addr):
			if (addr <= 0x1F000044)
			{
				return (T)sh4_rio_read<sz>(CCN, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(UBC_BASE_addr):
			if (addr <= 0x1F200020)
			{
				return (T)sh4_rio_read<sz>(UBC, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(BSC_BASE_addr):
			if (addr <= 0x1F800048)
			{
				return (T)sh4_rio_read<sz>(BSC, addr & 0xFF);
			}
			else if ((addr >= BSC_SDMR2_addr) && (addr <= 0x1F90FFFF))
			{
				//dram settings 2 / write only
				EMUERROR("Read from write-only registers [dram settings 2]");
			}
			else if ((addr >= BSC_SDMR3_addr) && (addr <= 0x1F94FFFF))
			{
				//dram settings 3 / write only
				EMUERROR("Read from write-only registers [dram settings 3]");
			}
			else
			{
				EMUERROR2("Out of range on register index . %x", addr);
			}
			break;



		case A7_REG_HASH(DMAC_BASE_addr):
			if (addr <= 0x1FA00040)
			{
				return (T)sh4_rio_read<sz>(DMAC, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(CPG_BASE_addr):
			if (addr <= 0x1FC00010)
			{
				return (T)sh4_rio_read<sz>(CPG, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(RTC_BASE_addr):
			if (addr <= 0x1FC8003C)
			{
				return (T)sh4_rio_read<sz>(RTC, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(INTC_BASE_addr):
			if (addr <= 0x1FD00010)
			{
				return (T)sh4_rio_read<sz>(INTC, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(TMU_BASE_addr):
			if (addr <= 0x1FD8002C)
			{
				return (T)sh4_rio_read<sz>(TMU, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(SCI_BASE_addr):
			if (addr <= 0x1FE0001C)
			{
				return (T)sh4_rio_read<sz>(SCI, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(SCIF_BASE_addr):
			if (addr <= 0x1FE80024)
			{
				return (T)sh4_rio_read<sz>(SCIF, addr & 0xFF);
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

			// Who really cares about ht-UDI? it's not existent on the Dreamcast IIRC
		case A7_REG_HASH(UDI_BASE_addr):
			switch (addr)
			{
				//UDI SDIR 0x1FF00000 0x1FF00000 16 0xFFFF Held Held Held Pclk
			case UDI_SDIR_addr:
				break;


				//UDI SDDR 0x1FF00008 0x1FF00008 32 Held Held Held Held Pclk
			case UDI_SDDR_addr:
				break;
			}
			break;
		}


		//EMUERROR2("Unknown Read from Area7 - addr=%x",addr);
		return 0;
	}

	//Write Area7
	template <u32 sz, class T>
	void WriteMem_area7(u32 addr, T data)
	{
		//printf("%08X\n",addr);

		addr &= 0x1FFFFFFF;
		u32 map_base = addr >> 16;
		switch (map_base & 0x1FFF)
		{

		case A7_REG_HASH(CCN_BASE_addr):
			if (addr <= 0x1F00003C)
			{
				sh4_rio_write<sz>(CCN, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(UBC_BASE_addr):
			if (addr <= 0x1F200020)
			{
				sh4_rio_write<sz>(UBC, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(BSC_BASE_addr):
			if (addr <= 0x1F800048)
			{
				sh4_rio_write<sz>(BSC, addr & 0xFF, data);
				return;
			}
			else if ((addr >= BSC_SDMR2_addr) && (addr <= 0x1F90FFFF))
			{
				//dram settings 2 / write only
				return;//no need ?
			}
			else if ((addr >= BSC_SDMR3_addr) && (addr <= 0x1F94FFFF))
			{
				//dram settings 3 / write only
				return;//no need ?
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;



		case A7_REG_HASH(DMAC_BASE_addr):
			if (addr <= 0x1FA00040)
			{
				sh4_rio_write<sz>(DMAC, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(CPG_BASE_addr):
			if (addr <= 0x1FC00010)
			{
				sh4_rio_write<sz>(CPG, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(RTC_BASE_addr):
			if (addr <= 0x1FC8003C)
			{
				sh4_rio_write<sz>(RTC, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(INTC_BASE_addr):
			if (addr <= 0x1FD0000C)
			{
				sh4_rio_write<sz>(INTC, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(TMU_BASE_addr):
			if (addr <= 0x1FD8002C)
			{
				sh4_rio_write<sz>(TMU, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(SCI_BASE_addr):
			if (addr <= 0x1FE0001C)
			{
				sh4_rio_write<sz>(SCI, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

		case A7_REG_HASH(SCIF_BASE_addr):
			if (addr <= 0x1FE80024)
			{
				sh4_rio_write<sz>(SCIF, addr & 0xFF, data);
				return;
			}
			else
			{
				EMUERROR2("Out of range on register index %x", addr);
			}
			break;

			//who really cares about ht-udi ? it's not existent on dc iirc ..
		case A7_REG_HASH(UDI_BASE_addr):
			switch (addr)
			{
				//UDI SDIR 0xFFF00000 0x1FF00000 16 0xFFFF Held Held Held Pclk
			case UDI_SDIR_addr:
				break;


				//UDI SDDR 0xFFF00008 0x1FF00008 32 Held Held Held Held Pclk
			case UDI_SDDR_addr:
				break;
			}
			break;
		}

		//EMUERROR3("Write to Area7 not implemented , addr=%x,data=%x",addr,data);
	}


	//***********
	//On Chip Ram
	//***********
	//Read OCR
	template <u32 sz, class T>
	T DYNACALL ReadMem_area7_OCR_T(u32 addr)
	{
		auto sh4mmr = this;

		if (CCN_CCR.ORA)
		{
			if (sz == 1)
				return (T)OnChipRAM[addr & OnChipRAM_MASK];
			else if (sz == 2)
				return (T) * (u16*)&OnChipRAM[addr & OnChipRAM_MASK];
			else if (sz == 4)
				return (T) * (u32*)&OnChipRAM[addr & OnChipRAM_MASK];
			else
			{
				EMUERROR("ReadMem_area7_OCR_T: template SZ is wrong = %d", sz);
				return 0xDE;
			}
		}
		else
		{
			EMUERROR("On Chip Ram Read, but OCR is disabled");
			return 0xDE;
		}
	}

	//Write OCR
	template <u32 sz, class T>
	void DYNACALL WriteMem_area7_OCR_T(u32 addr, T data)
	{
		auto sh4mmr = this;

		if (CCN_CCR.ORA)
		{
			if (sz == 1)
				OnChipRAM[addr & OnChipRAM_MASK] = (u8)data;
			else if (sz == 2)
				*(u16*)&OnChipRAM[addr & OnChipRAM_MASK] = (u16)data;
			else if (sz == 4)
				*(u32*)&OnChipRAM[addr & OnChipRAM_MASK] = data;
			else
			{
				EMUERROR("WriteMem_area7_OCR_T: template SZ is wrong = %d", sz);
			}
		}
		else
		{
			EMUERROR("On Chip Ram Write, but OCR is disabled");
		}
	}
#define DEFAULT_DECL(n) unique_ptr<SuperH4Module> mod##n;

	DEFAULT_DECL(Bsc) 
	DEFAULT_DECL(Ccn)
	DEFAULT_DECL(Cpg)
	DEFAULT_DECL(Dmac)
	DEFAULT_DECL(Intc)
	DEFAULT_DECL(Rtc)
	DEFAULT_DECL(Sci)
	DEFAULT_DECL(Scif)
	DEFAULT_DECL(Tmu)
	DEFAULT_DECL(Ubc)
	
	//Init/Res/Term
	SuperH4Mmr_impl(SuperH4* psh)
	{
		auto sh4mmr = this;

		OnChipRAM.Resize(OnChipRAM_SIZE, false);

		for (u32 i = 0; i < 30; i++)
		{
			if (i < CCN.size())  sh4_rio_reg(psh, CCN, CCN_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(16,true);    //CCN  : 14 registers
			if (i < UBC.size())  sh4_rio_reg(psh, UBC, UBC_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(9,true);     //UBC  : 9 registers
			if (i < BSC.size())  sh4_rio_reg(psh, BSC, BSC_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(19,true);    //BSC  : 18 registers
			if (i < DMAC.size()) sh4_rio_reg(psh, DMAC, DMAC_BASE_addr + i * 4, RIO_NO_ACCESS, 32); //(17,true);    //DMAC : 17 registers
			if (i < CPG.size())  sh4_rio_reg(psh, CPG, CPG_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(5,true);     //CPG  : 5 registers
			if (i < RTC.size())  sh4_rio_reg(psh, RTC, RTC_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(16,true);    //RTC  : 16 registers
			if (i < INTC.size()) sh4_rio_reg(psh, INTC, INTC_BASE_addr + i * 4, RIO_NO_ACCESS, 32); //(4,true);     //INTC : 4 registers
			if (i < TMU.size())  sh4_rio_reg(psh, TMU, TMU_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(12,true);    //TMU  : 12 registers
			if (i < SCI.size())  sh4_rio_reg(psh, SCI, SCI_BASE_addr + i * 4, RIO_NO_ACCESS, 32);   //(8,true);     //SCI  : 8 registers
			if (i < SCIF.size()) sh4_rio_reg(psh, SCIF, SCIF_BASE_addr + i * 4, RIO_NO_ACCESS, 32); //(10,true);    //SCIF : 10 registers
		}

#define DEFAULT_INIT(n) mod##n.reset(Sh4Mod##n::Create(this));
		//initialise Register structs
		
		DEFAULT_INIT(Bsc)
		DEFAULT_INIT(Ccn)
		DEFAULT_INIT(Cpg)
		DEFAULT_INIT(Dmac)
		DEFAULT_INIT(Intc)
		DEFAULT_INIT(Rtc)
		DEFAULT_INIT(Sci)
		DEFAULT_INIT(Scif)
		DEFAULT_INIT(Tmu)
		DEFAULT_INIT(Ubc)

		MMU_init(this);
	}

	void Reset()
	{
#define DEFAULT_RESET(n) mod##n->Reset()

		OnChipRAM.Zero();
		//Reset register values
		DEFAULT_RESET(Bsc);
		DEFAULT_RESET(Ccn);
		DEFAULT_RESET(Cpg);
		DEFAULT_RESET(Dmac);
		DEFAULT_RESET(Intc);
		DEFAULT_RESET(Rtc);
		DEFAULT_RESET(Sci);
		DEFAULT_RESET(Scif);
		DEFAULT_RESET(Tmu);
		DEFAULT_RESET(Ubc);

		MMU_reset();
	}

	~SuperH4Mmr_impl()
	{
#define DEFAULT_TERM(n) mod##n.reset();

		//free any alloc'd resources [if any]
		MMU_term();

		DEFAULT_TERM(Ubc)
		DEFAULT_TERM(Tmu)
		DEFAULT_TERM(Scif)
		DEFAULT_TERM(Sci)
		DEFAULT_TERM(Rtc)
		DEFAULT_TERM(Intc)
		DEFAULT_TERM(Dmac)
		DEFAULT_TERM(Cpg)
		DEFAULT_TERM(Ccn)
		DEFAULT_TERM(Bsc)
		OnChipRAM.Free();
	}

	u32 sh4io_read_noacc(u32 addr)
	{
		EMUERROR("sh4io: Invalid read access @@ %08X", addr);
		return 0;
	}
	void sh4io_write_noacc(u32 addr, u32 data)
	{
		EMUERROR("sh4io: Invalid write access @@ %08X %08X", addr, data);
		//verify(false); 
	}
	void sh4io_write_const(u32 addr, u32 data)
	{
		EMUERROR("sh4io: Const write ignored @@ %08X <- %08X", addr, data);
	}

	virtual void rio_reg(void* context, RegisterStruct* arr, size_t arr_size, u32 addr, RegIO flags, u32 sz, RegReadAddrFP* rf = nullptr, RegWriteAddrFP* wf = nullptr)
	{
		u32 idx = (addr & 255) / 4;

		verify(idx < arr_size);

		arr[idx].flags = flags | REG_ACCESS_32;
		arr[idx].context = context;

		if (flags == RIO_NO_ACCESS)
		{
			arr[idx].readFunctionAddr = STATIC_FORWARD(SuperH4Mmr_impl, sh4io_read_noacc);
			arr[idx].writeFunctionAddr = STATIC_FORWARD(SuperH4Mmr_impl, sh4io_write_noacc);
		}
		else if (flags == RIO_CONST)
		{
			arr[idx].writeFunctionAddr = STATIC_FORWARD(SuperH4Mmr_impl, sh4io_write_const);
		}
		else
		{
			arr[idx].data32 = 0;

			if (flags & REG_RF)
				arr[idx].readFunctionAddr = rf;

			if (flags & REG_WF)
				arr[idx].writeFunctionAddr = wf == 0 ? STATIC_FORWARD(SuperH4Mmr_impl, sh4io_write_noacc) : wf;
		}
	}

	virtual void serialize(void** data, unsigned int* total_size) {
		REICAST_SA(OnChipRAM.data, OnChipRAM_SIZE);

#define register_serialize(mod1, mod2, data, total_size) register_serialize(mod1.data(), mod1.size(), data, total_size); mod##mod2->serialize(data, total_size)

		register_serialize(CCN, Ccn, data, total_size);
		register_serialize(UBC, Ubc, data, total_size);
		register_serialize(BSC, Bsc, data, total_size);
		register_serialize(DMAC, Dmac, data, total_size);
		register_serialize(CPG, Cpg, data, total_size);
		register_serialize(RTC, Rtc, data, total_size);
		register_serialize(INTC, Intc, data, total_size);
		register_serialize(TMU, Tmu, data, total_size);
		register_serialize(SCI, Sci, data, total_size);
		register_serialize(SCIF, Scif, data, total_size);
	}

	virtual void unserialize(void** data, unsigned int* total_size) {
		REICAST_USA(OnChipRAM.data, OnChipRAM_SIZE);

#define register_unserialize(mod1, mod2, data, total_size) register_unserialize(mod1.data(), mod1.size(), data, total_size); mod##mod2->unserialize(data, total_size)

		register_unserialize(CCN, Ccn, data, total_size);
		register_unserialize(UBC, Ubc, data, total_size);
		register_unserialize(BSC, Bsc, data, total_size);
		register_unserialize(DMAC, Dmac, data, total_size);
		register_unserialize(CPG, Cpg, data, total_size);
		register_unserialize(RTC, Rtc, data, total_size);
		register_unserialize(INTC, Intc, data, total_size);
		register_unserialize(TMU, Tmu, data, total_size);
		register_unserialize(SCI, Sci, data, total_size);
		register_unserialize(SCIF, Scif, data, total_size);
	}
};

SuperH4Mmr* SuperH4Mmr::Create(SuperH4* sh4) {
	return new SuperH4Mmr_impl(sh4);
}

VMEM_MAP_FORWARDER(SuperH4Mmr_impl, Mem_P4)
VMEM_MAP_FORWARDER(SuperH4Mmr_impl, Mem_area7)
VMEM_MAP_FORWARDER(SuperH4Mmr_impl, Mem_area7_OCR_T)


void map_area7_init(SuperH4Mmr* mmr)
{
	//=_vmem_register_handler(ReadMem8_area7,ReadMem16_area7,ReadMem32_area7,
	//									WriteMem8_area7,WriteMem16_area7,WriteMem32_area7);

	//default area7 handler
	area7_handler = _vmem_register_handler_Template(mmr, ReadMem_area7, WriteMem_area7);

	area7_orc_handler = _vmem_register_handler_Template(mmr, ReadMem_area7_OCR_T, WriteMem_area7_OCR_T);
}
void map_area7(SuperH4* sh4, u32 base)
{
	//OCR @
	//((addr>=0x7C000000) && (addr<=0x7FFFFFFF))
	if (base == 0x60)
		_vmem_map_handler(area7_orc_handler, 0x1C | base, 0x1F | base);
	else
	{
		_vmem_map_handler(area7_handler, 0x1C | base, 0x1F | base);
	}
}

//P4
void map_p4(SuperH4* sh4, SuperH4Mmr* mmr)
{
	//P4 Region :
	_vmem_handler p4_handler = _vmem_register_handler_Template(mmr, ReadMem_P4, WriteMem_P4);

	//register this before area7 and SQ , so they overwrite it and handle em :)
	//default P4 handler
	//0xE0000000-0xFFFFFFFF
	_vmem_map_handler(p4_handler, 0xE0, 0xFF);

	//Store Queues -- Write only 32bit
	_vmem_map_block(sq_both, 0xE0, 0xE0, 63);
	_vmem_map_block(sq_both, 0xE1, 0xE1, 63);
	_vmem_map_block(sq_both, 0xE2, 0xE2, 63);
	_vmem_map_block(sq_both, 0xE3, 0xE3, 63);

	map_area7(sh4, 0xE0);
}

/*
	This file is part of libswirl
*/
#include "license/bsd"


/*
	Dreamcast 'area 0' emulation
	Pretty much all peripheral registers are mapped here

	Routing is mostly handled here, as well as flash/SRAM emulation
*/
#include "types.h"
#include "hw/sh4/sh4_mem.h"

#include "hw/holly/sh4_mem_area0.h"
#include "hw/holly/sb.h"
#include "hw/pvr/pvr_mem.h"
#include "hw/gdrom/gdrom_if.h"
#include "hw/aica/aica_mmio.h"
#include "hw/aica/aica_mem.h"
#include "hw/naomi/naomi.h"
#include "hw/modem/modem.h"

#include "hw/flashrom/flashrom.h"
#include "reios/reios.h"
#include "hw/sh4/sh4_mmio.h"
#include "hw/sh4/SuperH4_impl.h"

#include <memory>

#if DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
DCFlashChip sys_rom(BIOS_SIZE, BIOS_SIZE / 2);
#else
RomChip sys_rom(BIOS_SIZE);
#endif

#ifdef FLASH_SIZE
DCFlashChip sys_nvmem(FLASH_SIZE);
#endif

#ifdef BBSRAM_SIZE
SRamChip sys_nvmem(BBSRAM_SIZE);
#endif

extern bool bios_loaded;

bool LoadRomFiles(const string& root)
{
#if DC_PLATFORM != DC_PLATFORM_ATOMISWAVE
	if (!sys_rom.Load(root, ROM_PREFIX, "%boot.bin;%boot.bin.bin;%bios.bin;%bios.bin.bin" ROM_NAMES, "bootrom"))
	{
#if DC_PLATFORM == DC_PLATFORM_DREAMCAST
		// Dreamcast absolutely needs a BIOS
		msgboxf("Unable to find bios in \n%s\nExiting...", MBX_ICONERROR, root.c_str());
		return false;
#endif
	}
	else
		bios_loaded = true;
#endif
#if DC_PLATFORM == DC_PLATFORM_DREAMCAST
	if (!sys_nvmem.Load(root, ROM_PREFIX, "%nvmem.bin;%flash_wb.bin;%flash.bin;%flash.bin.bin", "nvram"))
#else
	if (!sys_nvmem.Load(get_game_save_prefix() + ".nvmem"))
#endif
	{
		if (NVR_OPTIONAL)
		{
			printf("flash/nvmem is missing, will create new file...\n");
		}
		else
		{
			msgboxf("Unable to find flash/nvmem in \n%s\nExiting...", MBX_ICONERROR, root.c_str());
			return false;
		}
	}

#if DC_PLATFORM == DC_PLATFORM_DREAMCAST
	struct flash_syscfg_block syscfg;
	int res = sys_nvmem.ReadBlock(FLASH_PT_USER, FLASH_USER_SYSCFG, &syscfg);

	if (!res)
	{
		// write out default settings
		memset(&syscfg, 0xff, sizeof(syscfg));
		syscfg.time_lo = 0;
		syscfg.time_hi = 0;
		syscfg.lang = 0;
		syscfg.mono = 0;
		syscfg.autostart = 1;
	}
	
	u32 time = libAICA_GetRTC_now();
	syscfg.time_lo = time & 0xffff;
	syscfg.time_hi = time >> 16;
	if (settings.dreamcast.language <= 5)
		syscfg.lang = settings.dreamcast.language;

	sys_nvmem.WriteBlock(FLASH_PT_USER, FLASH_USER_SYSCFG, &syscfg);
#endif

#if DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
	sys_rom.Load(get_game_save_prefix() + ".nvmem2");
#endif

	return true;
}

void SaveRomFiles(const string& root)
{
#if DC_PLATFORM == DC_PLATFORM_DREAMCAST
	sys_nvmem.Save(root, ROM_PREFIX, "nvmem.bin", "nvmem");
#else
	sys_nvmem.Save(get_game_save_prefix() + ".nvmem");
#endif
#if DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
	sys_rom.Save(get_game_save_prefix() + ".nvmem2");
#endif
}

bool LoadHle(const string& root) {
	if (!sys_nvmem.Load(root, ROM_PREFIX, "%nvmem.bin;%flash_wb.bin;%flash.bin;%flash.bin.bin", "nvram")) {
		printf("No nvmem loaded\n");
	}

	return reios_init(sys_rom.data, sys_nvmem.data);
}

u32 ReadFlash(u32 addr,u32 sz) { return sys_nvmem.Read(addr,sz); }
void WriteFlash(u32 addr,u32 data,u32 sz) { sys_nvmem.Write(addr,data,sz); }

#if (DC_PLATFORM == DC_PLATFORM_DREAMCAST) || (DC_PLATFORM == DC_PLATFORM_DEV_UNIT) || (DC_PLATFORM == DC_PLATFORM_NAOMI) || (DC_PLATFORM == DC_PLATFORM_NAOMI2)

u32 ReadBios(u32 addr,u32 sz) { return sys_rom.Read(addr,sz); }
void WriteBios(u32 addr,u32 data,u32 sz) { EMUERROR4("Write to [Boot ROM] is not possible, addr=%x,data=%x,size=%d",addr,data,sz); }

#elif (DC_PLATFORM == DC_PLATFORM_ATOMISWAVE)
	u32 ReadBios(u32 addr,u32 sz)
	{
		return sys_rom.Read(addr, sz);
	}

	void WriteBios(u32 addr,u32 data,u32 sz)
	{
		if (sz != 1)
		{
			EMUERROR("Invalid access size @%08x data %x sz %d\n", addr, data, sz);
			return;
		}
		sys_rom.Write(addr, data, sz);
	}

#else
#error unknown flash
#endif

struct BiosDevice : MMIODevice {
    u32 Read(u32 addr, u32 sz) {
        return ReadBios(addr, sz);
    }
    void Write(u32 addr, u32 data, u32 sz) {
        WriteBios(addr, data, sz);
    }
};

struct FlashDevice : MMIODevice {
    u32 Read(u32 addr, u32 sz) {
        return ReadFlash(addr, sz);
    }
    void Write(u32 addr, u32 data, u32 sz) {
        WriteFlash(addr, data, sz);
    }
};

struct ExtDevice_006 : MMIODevice {
    u32 Read(u32 addr, u32 sz) {
#if DC_PLATFORM == DC_PLATFORM_NAOMI || DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
        return libExtDevice_ReadMem_A0_006(addr, sz);
#else
        return 0;
#endif
    }

    void Write(u32 addr, u32 data, u32 sz) {
#if DC_PLATFORM == DC_PLATFORM_NAOMI || DC_PLATFORM == DC_PLATFORM_ATOMISWAVE
		libExtDevice_WriteMem_A0_006(addr, data, sz);
#endif
    }
};

struct ExtDevice_010 : MMIODevice {
    u32 Read(u32 addr, u32 sz) {
        return libExtDevice_ReadMem_A0_010(addr, sz);
    }

    void Write(u32 addr, u32 data, u32 sz) {
	    libExtDevice_WriteMem_A0_010(addr, data, sz);
    }
};


MMIODevice* Create_BiosDevice() {
	return new BiosDevice();
}
MMIODevice* Create_FlashDevice() {
	return new FlashDevice();
}

MMIODevice* Create_ExtDevice_006() {
	return new ExtDevice_006();
}

MMIODevice* Create_ExtDevice_010() {
	return new ExtDevice_010();
}


//Area 0 mem map
//0x00000000- 0x001FFFFF	:MPX	System/Boot ROM
//0x00200000- 0x0021FFFF	:Flash Memory
//0x00400000- 0x005F67FF	:Unassigned
//0x005F6800- 0x005F69FF	:System Control Reg.
//0x005F6C00- 0x005F6CFF	:Maple i/f Control Reg.
//0x005F7000- 0x005F70FF	:GD-ROM / NAOMI BD Reg.
//0x005F7400- 0x005F74FF	:G1 i/f Control Reg.
//0x005F7800- 0x005F78FF	:G2 i/f Control Reg.
//0x005F7C00- 0x005F7CFF	:PVR i/f Control Reg.
//0x005F8000- 0x005F9FFF	:TA / PVR Core Reg.
//0x00600000- 0x006007FF	:MODEM
//0x00600800- 0x006FFFFF	:G2 (Reserved)
//0x00700000- 0x00707FFF	:AICA- Sound Cntr. Reg.
//0x00710000- 0x0071000B	:AICA- RTC Cntr. Reg.
//0x00800000- 0x00FFFFFF	:AICA- Wave Memory
//0x01000000- 0x01FFFFFF	:Ext. Device
//0x02000000- 0x03FFFFFF*	:Image Area*	2MB

//use unified size handler for registers
//it really makes no sense to use different size handlers on em -> especially when we can use templates :p
template<u32 sz, class T>
T DYNACALL ReadMem_area0(void* ctx, u32 addr)
{
	auto sh4 = (SuperH4_impl*)ctx;
	addr &= 0x01FFFFFF;//to get rid of non needed bits
	const u32 base=(addr>>16);
	//map 0x0000 to 0x01FF to Default handler
	//mirror 0x0200 to 0x03FF , from 0x0000 to 0x03FFF
	//map 0x0000 to 0x001F
	if (base<=0x001F)//	:MPX	System/Boot ROM
	{
		return sh4->devices[A0H_BIOS]->Read(addr, sz);
	}
	//map 0x0020 to 0x0021
	else if ((base>= 0x0020) && (base<= 0x0021)) // :Flash Memory
	{
		return sh4->devices[A0H_FLASH]->Read(addr&0x1FFFF,sz);
	}
	//map 0x005F to 0x005F
	else if (likely(base==0x005F))
	{
		if ( /*&& (addr>= 0x00400000)*/ (addr<= 0x005F67FF)) // :Unassigned
		{
			EMUERROR2("Read from area0_32 not implemented [Unassigned], addr=%x",addr);
		}
		else if ((addr>= 0x005F7000) && (addr<= 0x005F70FF)) // GD-ROM
		{
            return sh4->devices[A0H_GDROM]->Read(addr, sz);
		}
		else if (likely((addr>= 0x005F6800) && (addr<=0x005F7CFF))) //	/*:PVR i/f Control Reg.*/ -> ALL SB registers now
		{
            return sh4->devices[A0H_SB]->Read(addr, sz);
		}
		else if (likely((addr>= 0x005F8000) && (addr<=0x005F9FFF))) //	:TA / PVR Core Reg.
		{
			return sh4->devices[A0H_PVR]->Read(addr, sz);
		}
	}
	//map 0x0060 to 0x0060
	else if ((base ==0x0060) /*&& (addr>= 0x00600000)*/ && (addr<= 0x006007FF)) //	:MODEM
	{
        sh4->devices[A0H_EXTDEV_006]->Read(addr, sz);
	}
	//map 0x0060 to 0x006F
	else if ((base >=0x0060) && (base <=0x006F) && (addr>= 0x00600800) && (addr<= 0x006FFFFF)) //	:G2 (Reserved)
	{
		EMUERROR2("Read from area0_32 not implemented [G2 (Reserved)], addr=%x",addr);
	}
	//map 0x0070 to 0x0070
	else if ((base ==0x0070) /*&& (addr>= 0x00700000)*/ && (addr<=0x00707FFF)) //	:AICA- Sound Cntr. Reg.
	{
		return sh4->devices[A0H_AICA]->Read(addr,sz);
	}
	//map 0x0071 to 0x0071
	else if ((base ==0x0071) /*&& (addr>= 0x00710000)*/ && (addr<= 0x0071000B)) //	:AICA- RTC Cntr. Reg.
	{
		return sh4->devices[A0H_RTC]->Read(addr,sz);
	}
	//map 0x0080 to 0x00FF
	else if ((base >=0x0080) && (base <=0x00FF) /*&& (addr>= 0x00800000) && (addr<=0x00FFFFFF)*/) //	:AICA- Wave Memory
	{
		ReadMemArrRet(sh4->aica_ram.data, addr & (sh4->aica_ram.size - 1), sz);
	}
	//map 0x0100 to 0x01FF
	else if ((base >=0x0100) && (base <=0x01FF) /*&& (addr>= 0x01000000) && (addr<= 0x01FFFFFF)*/) //	:Ext. Device
	{
        return sh4->devices[A0H_EXTDEV_010]->Read(addr, sz);
	}
	return 0;
}

template<u32 sz, class T>
void  DYNACALL WriteMem_area0(void* ctx, u32 addr,T data)
{
	auto sh4 = (SuperH4_impl*)ctx;
	addr &= 0x01FFFFFF;//to get rid of non needed bits

	const u32 base=(addr>>16);

	//map 0x0000 to 0x001F
	if ((base <=0x001F) /*&& (addr<=0x001FFFFF)*/)// :MPX System/Boot ROM
	{
        sh4->devices[A0H_BIOS]->Write(addr,data,sz);
	}
	//map 0x0020 to 0x0021
	else if ((base >=0x0020) && (base <=0x0021) /*&& (addr>= 0x00200000) && (addr<= 0x0021FFFF)*/) // Flash Memory
	{
        sh4->devices[A0H_FLASH]->Write(addr,data,sz);
	}
	//map 0x0040 to 0x005F -> actually, I'll only map 0x005F to 0x005F, b/c the rest of it is unspammed (left to default handler)
	//map 0x005F to 0x005F
	else if ( likely(base==0x005F) )
	{
		if (/*&& (addr>= 0x00400000) */ (addr<= 0x005F67FF)) // Unassigned
		{
			EMUERROR4("Write to area0_32 not implemented [Unassigned], addr=%x,data=%x,size=%d",addr,data,sz);
		}
		else if ((addr>= 0x005F7000) && (addr<= 0x005F70FF)) // GD-ROM
		{
            sh4->devices[A0H_GDROM]->Write(addr, data, sz);
		}
		else if ( likely((addr>= 0x005F6800) && (addr<=0x005F7CFF)) ) // /*:PVR i/f Control Reg.*/ -> ALL SB registers
		{
            sh4->devices[A0H_SB]->Write(addr, data, sz);
		}
		else if ( likely((addr>= 0x005F8000) && (addr<=0x005F9FFF)) ) // TA / PVR Core Reg.
		{
            sh4->devices[A0H_PVR]->Write(addr, data, sz);
		}
	}
	//map 0x0060 to 0x0060
	else if ((base ==0x0060) /*&& (addr>= 0x00600000)*/ && (addr<= 0x006007FF)) // MODEM
	{
        sh4->devices[A0H_EXTDEV_006]->Write(addr, data, sz);
	}
	//map 0x0060 to 0x006F
	else if ((base >=0x0060) && (base <=0x006F) && (addr>= 0x00600800) && (addr<= 0x006FFFFF)) // G2 (Reserved)
	{
		EMUERROR4("Write to area0_32 not implemented [G2 (Reserved)], addr=%x,data=%x,size=%d",addr,data,sz);
	}
	//map 0x0070 to 0x0070
	else if ((base >=0x0070) && (base <=0x0070) /*&& (addr>= 0x00700000)*/ && (addr<=0x00707FFF)) // AICA- Sound Cntr. Reg.
	{
        sh4->devices[A0H_AICA]->Write(addr, data, sz);
		return;
	}
	//map 0x0071 to 0x0071
	else if ((base >=0x0071) && (base <=0x0071) /*&& (addr>= 0x00710000)*/ && (addr<= 0x0071000B)) // AICA- RTC Cntr. Reg.
	{
        sh4->devices[A0H_RTC]->Write(addr, data, sz);
		return;
	}
	//map 0x0080 to 0x00FF
	else if ((base >=0x0080) && (base <=0x00FF) /*&& (addr>= 0x00800000) && (addr<=0x00FFFFFF)*/) // AICA- Wave Memory
	{
		WriteMemArrRet(sh4->aica_ram.data, addr & (sh4->aica_ram.size - 1), data, sz);
		return;
	}
	//map 0x0100 to 0x01FF
	else if ((base >=0x0100) && (base <=0x01FF) /*&& (addr>= 0x01000000) && (addr<= 0x01FFFFFF)*/) // Ext. Device
	{
        sh4->devices[A0H_EXTDEV_010]->Write(addr, data, sz);
	}
	return;
}

//AREA 0
_vmem_handler area0_handler;


void map_area0_init(SuperH4* sh4)
{

	area0_handler = _vmem_register_handler_Template(sh4, ReadMem_area0,WriteMem_area0);
}
void map_area0(SuperH4* sh4, u32 base)
{
	verify(base<0xE0);

	_vmem_map_handler(area0_handler,0x00|base,0x01|base);

	//0x0240 to 0x03FF mirrors 0x0040 to 0x01FF (no flashrom or bios)
	//0x0200 to 0x023F are unused
	_vmem_mirror_mapping(0x02|base,0x00|base,0x02);
}

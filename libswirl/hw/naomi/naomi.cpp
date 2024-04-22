/*
	This file is part of libswirl, likely derived from MAME
*/
#include "license/lgpl"


/*
	This file is a mix of my code, Zezu's, and duno wtf-else (most likely ElSemi's ?)
*/
#include "types.h"
#include "cfg/cfg.h"
#include "hw/holly/sb.h"
#include "hw/sh4/sh4_mem.h"
#include "hw/holly/holly_intc.h"
#include "hw/maple/maple_cfg.h"

#include "naomi.h"
#include "naomi_cart.h"
#include "naomi_regs.h"

u32 naomi_updates;

//#define NAOMI_COMM

static const u32 BoardID=0x980055AA;
u32 GSerialBuffer=0,BSerialBuffer=0;
int GBufPos=0,BBufPos=0;
int GState=0,BState=0;
int GOldClk=0,BOldClk=0;
int BControl=0,BCmd=0,BLastCmd=0;
int GControl=0,GCmd=0,GLastCmd=0;
int SerStep=0,SerStep2=0;

#ifdef NAOMI_COMM
	u32 CommOffset;
	u32* CommSharedMem;
	HANDLE CommMapFile=INVALID_HANDLE_VALUE;
#endif

/*
El numero de serie solo puede contener:
0-9		(0x30-0x39)
A-H		(0x41-0x48)
J-N		(0x4A-0x4E)
P-Z		(0x50-0x5A)
*/
unsigned char BSerial[]="\xB7"/*CRC1*/"\x19"/*CRC2*/"0123234437897584372973927387463782196719782697849162342198671923649";
unsigned char GSerial[]="\xB7"/*CRC1*/"\x19"/*CRC2*/"0123234437897584372973927387463782196719782697849162342198671923649";

unsigned int ShiftCRC(unsigned int CRC,unsigned int rounds)
{
	const unsigned int Magic=0x10210000;
	unsigned int i;
	for(i=0;i<rounds;++i)
	{
		if(CRC&0x80000000)
			CRC=(CRC<<1)+Magic;
		else
			CRC=(CRC<<1);
	}
	return CRC;
}

unsigned short CRCSerial(unsigned char *Serial,unsigned int len)
{
	unsigned int CRC=0xDEBDEB00;
	unsigned int i;

	for(i=0;i<len;++i)
	{
		unsigned char c=Serial[i];
		//CRC&=0xFFFFFF00;
		CRC|=c;
		CRC=ShiftCRC(CRC,8);
	}
	CRC=ShiftCRC(CRC,8);
	return (u16)(CRC>>16);
}


void NaomiInit()
{
	u16 CRC;
	CRC=CRCSerial(BSerial+2,0x2E);
	BSerial[0]=(u8)(CRC>>8);
	BSerial[1]=(u8)(CRC);

	CRC=CRCSerial(GSerial+2,0x2E);
	GSerial[0]=(u8)(CRC>>8);
	GSerial[1]=(u8)(CRC);
}




void NaomiBoardIDWrite(const u16 Data)
{
	int Dat=Data&8;
	int Clk=Data&4;
	int Rst=Data&0x20;
	int Sta=Data&0x10;
	

	if(Rst)
	{
		BState=0;
		BBufPos=0;
	}

	
	if(Clk!=BOldClk && !Clk)	//Falling Edge clock
	{
		//State change
		if(BState==0 && Sta) 
			BState=1;		
		if(BState==1 && !Sta)
			BState=2;

		if((BControl&0xfff)==0xFF0)	//Command mode
		{
			BCmd<<=1;
			if(Dat)
				BCmd|=1;
			else
				BCmd&=0xfffffffe;
		}

		//State processing
		if(BState==1)		//LoadBoardID
		{
			BSerialBuffer=BoardID;
			BBufPos=0;		//??
		}
		if(BState==2)		//ShiftBoardID
		{
			BBufPos++;
		}
	}
	BOldClk=Clk;
}

u16 NaomiBoardIDRead()
{
	if((BControl&0xff)==0xFE)
		return 0xffff;
	return (BSerialBuffer&(1<<(31-BBufPos)))?8:0;
}

static u32 AdaptByte(u8 val)
{
	return val<<24;
}

void NaomiBoardIDWriteControl(const u16 Data)
{
	if((Data&0xfff)==0xF30 && BCmd!=BLastCmd)
	{
		if((BCmd&0x81)==0x81)
		{
			SerStep2=(BCmd>>1)&0x3f;

			BSerialBuffer=0x00000000;	//First block contains CRC
			BBufPos=0;
		}
		if((BCmd&0xff)==0x55)	//Load Offset 0
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2])>>1;
		}
		if((BCmd&0xff)==0xAA)	//Load Offset 1
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+1]);
		}
		if((BCmd&0xff)==0x54)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+2]);
		}
		if((BCmd&0xff)==0xA8)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+3]);
		}
		if((BCmd&0xff)==0x50)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+4]);
		}
		if((BCmd&0xff)==0xA0)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+5]);
		}
		if((BCmd&0xff)==0x40)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+6]);
		}
		if((BCmd&0xff)==0x80)
		{
			BState=2;
			BBufPos=0;
			BSerialBuffer=AdaptByte(BSerial[8*SerStep2+7]);
		}
		BLastCmd=BCmd;
	}
	BControl=Data;
}

void NaomiGameIDProcessCmd()
{
	if(GCmd!=GLastCmd)
	{
		if((GCmd&0x81)==0x81)
		{
			SerStep=(GCmd>>1)&0x3f;

			GSerialBuffer=0x00000000;	//First block contains CRC
			GBufPos=0;
		}
		if((GCmd&0xff)==0x55)	//Load Offset 0
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep])>>0;
		}
		if((GCmd&0xff)==0xAA)	//Load Offset 1
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+1]);
		}
		if((GCmd&0xff)==0x54)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+2]);
		}
		if((GCmd&0xff)==0xA8)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+3]);
		}
		if((GCmd&0xff)==0x50)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+4]);
		}
		if((GCmd&0xff)==0xA0)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+5]);
		}
		if((GCmd&0xff)==0x40)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+6]);
		}
		if((GCmd&0xff)==0x80)
		{
			GState=2;
			GBufPos=0;
			GSerialBuffer=AdaptByte(GSerial[8*SerStep+7]);
		}
		GLastCmd=GCmd;
	}
}


void NaomiGameIDWrite(const u16 Data)
{
	int Dat=Data&0x01;
	int Clk=Data&0x02;
	int Rst=Data&0x04;
	int Sta=Data&0x08;
	int Cmd=Data&0x10;
	

	if(Rst)
	{
		GState=0;
		GBufPos=0;
	}

	
	if(Clk!=GOldClk && !Clk)	//Falling Edge clock
	{
		//State change
		if(GState==0 && Sta) 
			GState=1;		
		if(GState==1 && !Sta)
			GState=2;
		
		
		
		

		//State processing
		if(GState==1)		//LoadBoardID
		{
			GSerialBuffer=BoardID;
			GBufPos=0;		//??
		}
		if(GState==2)		//ShiftBoardID
			GBufPos++;

		if(GControl!=Cmd && !Cmd)
		{
			NaomiGameIDProcessCmd();
		}
		GControl=Cmd;
	}
	if(Clk!=GOldClk && Clk)	//Rising Edge clock
	{
		if(Cmd)	//Command mode
		{
			GCmd<<=1;
			if(Dat)
				GCmd|=1;
			else
				GCmd&=0xfffffffe;
			GControl=Cmd;
		}
		
	}

	GOldClk=Clk;

}

u16 NaomiGameIDRead()
{
	return (GSerialBuffer&(1<<(31-GBufPos)))?1:0;
}




//DIMM board
//Uses interrupt ext#3  (holly_EXT_PCI)

//status/flags ? 0x1 is some completion/init flag(?), 0x100 is the interrupt disable flag (?)
//n1 bios rev g (n2/epr-23605b has similar behavior of not same):
//3c=0x1E03
//40=0
//44=0
//48=0
//read 4c
//wait for 4c not 0
//4c=[4c]-1

//Naomi 2 bios epr-23609
//read 3c
//wait 4c to be non 0
//

//SO the writes to 3c/stuff are not relaced with 4c '1'
//If the dimm board has some internal cpu/pic logic 
//4c '1' seems to be the init done bit (?)
//n1/n2 clears it after getting a non 0 value
//n1 bios writes the value -1, meaning it expects the bit 0 to be set
//.//

u32 reg_dimm_3c;	//IO window ! written, 0x1E03 some flag ?
u32 reg_dimm_40;	//parameters
u32 reg_dimm_44;	//parameters
u32 reg_dimm_48;	//parameters

u32 reg_dimm_4c=0x11;	//status/control reg ?

bool NaomiDataRead = false;
static bool aw_ram_test_skipped = false;

void naomi_process(u32 r3c,u32 r40,u32 r44, u32 r48)
{
	printf("Naomi process 0x%04X 0x%04X 0x%04X 0x%04X\n",r3c,r40,r44,r48);
	printf("Possible format 0 %d 0x%02X 0x%04X\n",r3c>>15,(r3c&0x7e00)>>9,r3c&0x1FF);
	printf("Possible format 1 0x%02X 0x%02X\n",(r3c&0xFF00)>>8,r3c&0xFF);

	u32 param=(r3c&0xFF);
	if (param==0xFF)
	{
		printf("invalid opcode or smth ?");
	}
	static int opcd=0;
	//else if (param!=3)
	if (opcd<255)
	{
		reg_dimm_3c=0x8000 | (opcd%12<<9) | (0x0);
		printf("new reg is 0x%X\n",reg_dimm_3c);
		asic_RaiseInterrupt(holly_EXP_PCI);
		printf("Interrupt raised\n");
		opcd++;
	}
}

struct NaomiDevice_impl : MMIODevice {
	SystemBus* sb;
	ASIC* asic;
	SuperH4Mmr* sh4mmr;

	NaomiDevice_impl(SuperH4Mmr* sh4mmr, SystemBus* sb, ASIC* asic) : sh4mmr(sh4mmr), sb(sb), asic(asic) { }

	u32 Read(u32 Addr, u32 sz)
	{
		verify(sz != 1);
		if (unlikely(CurrentCartridge == NULL))
		{
			EMUERROR("called without cartridge\n");
			return 0xFFFF;
		}
		return CurrentCartridge->ReadMem(Addr, sz);
	}

	void Write(u32 Addr, u32 data, u32 sz)
	{
		if (unlikely(CurrentCartridge == NULL))
		{
			EMUERROR("called without cartridge\n");
			return;
		}
		CurrentCartridge->WriteMem(Addr, data, sz);
	}

	//Dma Start
	void DmaStart(u32 addr, u32 data)
	{
		if (SB_GDEN == 0)
		{
			printf("Invalid (NAOMI)GD-DMA start, SB_GDEN=0.Ingoring it.\n");
			return;
		}

		NaomiDataRead = true;
		SB_GDST |= data & 1;

		if (SB_GDST == 1)
		{
			verify(1 == SB_GDDIR);

			SB_GDSTARD = SB_GDSTAR + SB_GDLEN;

			SB_GDLEND = SB_GDLEN;
			SB_GDST = 0;
			if (CurrentCartridge != NULL)
			{
				u32 len = SB_GDLEN;
				u32 offset = 0;
				while (len > 0)
				{
					u32 block_len = len;
					void* ptr = CurrentCartridge->GetDmaPtr(block_len);
					WriteMemBlock_nommu_ptr(SB_GDSTAR + offset, (u32*)ptr, block_len);
					CurrentCartridge->AdvancePtr(block_len);
					len -= block_len;
					offset += block_len;
				}
			}

			asic->RaiseInterrupt(holly_GDROM_DMA);
		}
	}


	void DmaEnable(u32 addr, u32 data)
	{
		SB_GDEN = data & 1;
		if (SB_GDEN == 0 && SB_GDST == 1)
		{
			printf("(NAOMI)GD-DMA aborted\n");
			SB_GDST = 0;
		}
	}

	bool Init()
	{
#ifdef NAOMI_COMM
		CommMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,    // use paging file
			NULL,                    // default security 
			PAGE_READWRITE,          // read/write access
			0,                       // max. object size 
			0x1000 * 4,                // buffer size  
			L"Global\\nullDC_103_naomi_comm");                 // name of mapping object

		if (CommMapFile == NULL || CommMapFile == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("Could not create file mapping object (%d).\nTrying to open existing one\n"), GetLastError());

			CommMapFile = OpenFileMapping(
				FILE_MAP_ALL_ACCESS,   // read/write access
				FALSE,                 // do not inherit the name
				L"Global\\nullDC_103_naomi_comm");               // name of mapping object 
		}

		if (CommMapFile == NULL || CommMapFile == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("Could not open existing file either\n"), GetLastError());
			CommMapFile = INVALID_HANDLE_VALUE;
		}
		else
		{
			printf("NAOMI: Created \"Global\\nullDC_103_naomi_comm\"\n");
			CommSharedMem = (u32*)MapViewOfFile(CommMapFile,   // handle to map object
				FILE_MAP_ALL_ACCESS, // read/write permission
				0,
				0,
				0x1000 * 4);

			if (CommSharedMem == NULL)
			{
				_tprintf(TEXT("Could not map view of file (%d).\n"),
					GetLastError());

				CloseHandle(CommMapFile);
				CommMapFile = INVALID_HANDLE_VALUE;
			}
			else
				printf("NAOMI: Mapped CommSharedMem\n");
		}
#endif
		NaomiInit();

		sb->RegisterRIO(this, SB_GDST_addr, RIO_WF, 0, STATIC_FORWARD(NaomiDevice_impl, DmaStart));

		sb->RegisterRIO(this, SB_GDEN_addr, RIO_WF, 0, STATIC_FORWARD(NaomiDevice_impl, DmaEnable));

		return true;
	}

	void Term()
	{
#ifdef NAOMI_COMM
		if (CommSharedMem)
		{
			UnmapViewOfFile(CommSharedMem);
		}
		if (CommMapFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(CommMapFile);
		}
#endif
	}
	void Reset(bool Manual)
	{
		NaomiDataRead = false;
		aw_ram_test_skipped = false;
		BLastCmd = 0;
	}

	void Update()
	{
		/*
		if (naomi_updates>1)
		{
			naomi_updates--;
		}
		else if (naomi_updates==1)
		{
			naomi_updates=0;
			asic->RaiseInterrupt(holly_EXP_PCI);
		}*/
#if 0
		if (!(SB_GDST & 1) || !(SB_GDEN & 1))
			return;

		//SB_GDST=0;

		//TODO : Fix dmaor
		u32 dmaor = DMAC_DMAOR.full;

		u32	src = SB_GDSTARD,
			len = SB_GDLEN - SB_GDLEND;

		//len=min(len,(u32)32);
		// do we need to do this for gdrom dma ?
		if (0x8201 != (dmaor & DMAOR_MASK)) {
			printf("\n!\tGDROM: DMAOR has invalid settings (%X) !\n", dmaor);
			//return;
		}
		if (len & 0x1F) {
			printf("\n!\tGDROM: SB_GDLEN has invalid size (%X) !\n", len);
			return;
		}

		if (0 == len)
		{
			printf("\n!\tGDROM: Len: %X, Abnormal Termination !\n", len);
		}
		u32 len_backup = len;
		if (1 == SB_GDDIR)
		{
			WriteMemBlock_nommu_ptr(dst, NaomiRom + (DmaOffset & 0x7ffffff), size);

			DmaCount = 0xffff;
		}
		else
			msgboxf(L"GDROM: SB_GDDIR %X (TO AICA WAVE MEM?)", MBX_ICONERROR, SB_GDDIR);

		//SB_GDLEN = 0x00000000; //13/5/2k7 -> acording to docs these regs are not updated by hardware
		//SB_GDSTAR = (src + len_backup);

		SB_GDLEND += len_backup;
		SB_GDSTARD += len_backup;//(src + len_backup)&0x1FFFFFFF;

		if (SB_GDLEND == SB_GDLEN)
		{
			//printf("Streamed GDMA end - %d bytes trasnfered\n",SB_GDLEND);
			SB_GDST = 0;//done
			// The DMA end interrupt flag
			asic->RaiseInterrupt(holly_GDROM_DMA);
		}
		//Readed ALL sectors
		if (read_params.remaining_sectors == 0)
		{
			u32 buff_size = read_buff.cache_size - read_buff.cache_index;
			//And all buffer :p
			if (buff_size == 0)
			{
				verify(!SB_GDST & 1)
					gd_set_state(gds_procpacketdone);
			}
		}
#endif
	}
};

MMIODevice* Create_NaomiDevice(SuperH4Mmr* sh4mmr, SystemBus* sb, ASIC* asic) {
	return new NaomiDevice_impl(sh4mmr, sb, asic);
}

static u8 aw_maple_devs;

u32 libExtDevice_ReadMem_A0_006(u32 addr,u32 size) {
	addr &= 0x7ff;
	//printf("libExtDevice_ReadMem_A0_006 %d@%08x: %x\n", size, addr, mem600[addr]);
	switch (addr)
	{
	case 0x280:
		// 0x00600280 r  0000dcba
		//	a/b - 1P/2P coin inputs (JAMMA), active low
		//	c/d - 3P/4P coin inputs (EX. IO board), active low
		//
		//	(ab == 0) -> BIOS skip RAM test
		if (!aw_ram_test_skipped)
		{
			// Skip RAM test at startup
			aw_ram_test_skipped = true;
			return 0;
		}
		{
			u8 coin_input = 0xF;
			for (int slot = 0; slot < 4; slot++)
				if (maple_atomiswave_coin_chute(slot))
					coin_input &= ~(1 << slot);
			return coin_input;
		}

	case 0x284:		// Atomiswave maple devices
		// ddcc0000 where cc/dd are the types of devices on maple bus 2 and 3:
		// 0: regular AtomisWave controller
		// 1: light gun
		// 2,3: mouse/trackball
		//printf("NAOMI 600284 read %x\n", aw_maple_devs);
		return aw_maple_devs;
	case 0x288:
		// ??? Dolphin Blue
		return 0;

	}
	EMUERROR("Unhandled read @ %x sz %d", addr, size);
	return 0xFF;
}

void libExtDevice_WriteMem_A0_006(u32 addr,u32 data,u32 size) {
	addr &= 0x7ff;
	//printf("libExtDevice_WriteMem_A0_006 %d@%08x: %x\n", size, addr, data);
	switch (addr)
	{
	case 0x284:		// Atomiswave maple devices
		printf("NAOMI 600284 write %x\n", data);
		aw_maple_devs = data & 0xF0;
		return;
	case 0x288:
		// ??? Dolphin Blue
		return;
	//case 0x28C:		// Wheel force feedback?
	default:
		break;
	}
	EMUERROR("Unhandled write @ %x (%d): %x", addr, size, data);
}

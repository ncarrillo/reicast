/*
	This file is part of libswirl
*/
#include "license/bsd"


#pragma once
#include "aica.h"

struct dsp_context_t
{
	//Dynarec
	u8 DynCode[4096 * 8];	//32 kb, 8 pages

	//buffered DSP state
	//24 bit wide
	s32 TEMP[128];
	//24 bit wide
	s32 MEMS[32];
	//20 bit wide
	s32 MIXS[16];

	//RBL/RBP (decoded)
	u32 RBP;
	u32 RBL;

	struct
	{
		bool MAD_OUT;
		bool MEM_ADDR;
		bool MEM_RD_DATA;
		bool MEM_WT_DATA;
		bool FRC_REG;
		bool ADRS_REG;
		bool Y_REG;

		bool MDEC_CT;
		bool MWT_1;
		bool MRD_1;
		//bool MADRS;
		bool MEMS;
		bool NOFL_1;
		bool NOFL_2;

		bool TEMPS;
		bool EFREG;
	}regs_init;

	//s32 -> stored as signed extended to 32 bits
	struct
	{
		s32 MAD_OUT;
		s32 MEM_ADDR;
		s32 MEM_RD_DATA;
		s32 MEM_WT_DATA;
		s32 FRC_REG;
		s32 ADRS_REG;
		s32 Y_REG;

		u32 MDEC_CT;
		u32 MWT_1;
		u32 MRD_1;
		u32 MADRS;
		u32 NOFL_1;
		u32 NOFL_2;
	}regs;
	//DEC counter :)
	//u32 DEC;

	//various dsp regs
	signed int ACC;        //26 bit
	signed int SHIFTED;    //24 bit
	signed int B;          //26 bit
	signed int MEMVAL[4];
	signed int FRC_REG;    //13 bit
	signed int Y_REG;      //24 bit
	unsigned int ADDR;
	unsigned int ADRS_REG; //13 bit

	//Direct Mapped data :
	//COEF  *128
	//MADRS *64
	//MPRO(dsp code) *4 *128
	//EFREG *16
	//EXTS  *2

	// Interpreter flags
	bool Stopped;

	//Dynarec flags
	bool dyndirty;
};

struct _INST
{
	unsigned int TRA;
	unsigned int TWT;
	unsigned int TWA;

	unsigned int XSEL;
	unsigned int YSEL;
	unsigned int IRA;
	unsigned int IWT;
	unsigned int IWA;

	unsigned int EWT;
	unsigned int EWA;
	unsigned int ADRL;
	unsigned int FRCL;
	unsigned int SHIFT;
	unsigned int YRL;
	unsigned int NEGB;
	unsigned int ZERO;
	unsigned int BSEL;

	unsigned int NOFL;  //MRQ set
	unsigned int TABLE; //MRQ set
	unsigned int MWT;   //MRQ set
	unsigned int MRD;   //MRQ set
	unsigned int MASA;  //MRQ set
	unsigned int ADREB; //MRQ set
	unsigned int NXADR; //MRQ set
};


struct DSPData_struct;
struct DSPBackend {
	static u16 DYNACALL PACK(s32 val);
	static s32 DYNACALL UNPACK(u16 val);
	static void DecodeInst(u32* IPtr, _INST* i);
	static void EncodeInst(u32* IPtr, _INST* i);

	virtual void Step() = 0;
	virtual void Recompile() = 0;

	virtual ~DSPBackend() { }

	static DSPBackend* CreateInterpreter(DSPData_struct* DSPData, dsp_context_t* dsp, u8* aica_ram, u32 aram_size);
	static DSPBackend* CreateJIT(DSPData_struct* DSPData, dsp_context_t* dsp, u8* aica_ram, u32 aram_size);
};

/*
	This file is part of libswirl
*/
#include "license/bsd"


#pragma once
#include "aica.h"


struct dsp_context_t;
struct AudioStream;

union fp_22_10
{
	struct
	{
		u32 fp:10;
		u32 ip:22;
	};
	u32 full;
};
union fp_s_22_10
{
	struct
	{
		u32 fp:10;
		s32 ip:22;
	};
	s32 full;
};
union fp_20_12
{
	struct
	{
		u32 fp:12;
		u32 ip:20;
	};
	u32 full;
};

struct DSP_OUT_VOL_REG
{
	//--	EFSDL[3:0]	--	EFPAN[4:0]

	u32 EFPAN:5;
	u32 res_1:3;

	u32 EFSDL:4;
	u32 res_2:4;

	u32 pad:16;
};

//#define SAMPLE_TYPE_SHIFT (8)
typedef s32 SampleType;

#define clip(x,min,max) if ((x)<(min)) (x)=(min); if ((x)>(max)) (x)=(max);
#define clip16(x) clip(x,-32768,32767)

struct SGC {
	virtual void AICA_Sample() = 0;
	virtual void AICA_Sample32() = 0;

	virtual void WriteChannelReg8(u32 channel, u32 reg) = 0;

	static SGC* Create(AudioStream* audio_stream, u8* aica_reg, dsp_context_t* dsp, u8* aica_ram, u32 aram_size);
;
	virtual void ReadCommonReg(u32 reg,bool byte) = 0;
	virtual void WriteCommonReg8(u32 reg,u32 data) = 0;
	virtual bool channel_serialize(void **data, unsigned int *total_size) = 0;
	virtual bool channel_unserialize(void **data, unsigned int *total_size) = 0;

	virtual ~SGC() { }
};
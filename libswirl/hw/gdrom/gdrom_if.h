/*
	This file is part of libswirl
*/
#include "license/bsd"


/*
**	gdrom_if.h
*/
#pragma once
#include "types.h"

//there were at least 4 gdrom implementations, ZGDROM, HLE, gdromv1 (never worked), gdromv2 (first release)
//i removed the #defines to select them as they are now redundant, so this just becomes a part of
//the code's history :)
#include "gdromv3.h"

u32  ReadMem_gdrom(u32 Addr, u32 sz);
void WriteMem_gdrom(u32 Addr, u32 data, u32 sz);
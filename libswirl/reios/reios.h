/*
	This file is part of libswirl
*/
#include "license/bsd"


#ifndef REIOS_H
#define REIOS_H

#include "types.h"

bool reios_init(u8* rom, u8* flash);

void reios_reset();

void reios_term();

void DYNACALL reios_trap(u32 op);

char* reios_disk_id();
extern char reios_software_name[129];
extern char reios_product_number[11];
extern bool reios_windows_ce;

#define REIOS_OPCODE 0x085B

#endif //REIOS_H

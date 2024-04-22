/*
	This file is part of libswirl
*/
#include "license/bsd"


#include "_vmem.h"
#include "hw/aica/aica_mem.h"
#include "hw/sh4/dyna/blockmanager.h"

//top registered handler
_vmem_handler       _vmem_lrp;

//handler tables
_vmem_ReadMem8FP*   _vmem_RF8[HANDLER_COUNT];
_vmem_WriteMem8FP*  _vmem_WF8[HANDLER_COUNT];

_vmem_ReadMem16FP*  _vmem_RF16[HANDLER_COUNT];
_vmem_WriteMem16FP* _vmem_WF16[HANDLER_COUNT];

_vmem_ReadMem32FP*  _vmem_RF32[HANDLER_COUNT];
_vmem_WriteMem32FP* _vmem_WF32[HANDLER_COUNT];

void* _vmem_CTX[HANDLER_COUNT];

//upper 8b of the address
void* _vmem_MemInfo_ptr[0x100];

void _vmem_get_ptrs(u32 sz,bool write,void*** vmap,void*** func)
{
	*vmap=_vmem_MemInfo_ptr;
	switch(sz)
	{
	case 1:
		*func=write?(void**)_vmem_WF8:(void**)_vmem_RF8;
		return;

	case 2:
		*func=write?(void**)_vmem_WF16:(void**)_vmem_RF16;
		return;

	case 4:
	case 8:
		*func=write?(void**)_vmem_WF32:(void**)_vmem_RF32;
		return;

	default:
		die("invalid size");
	}
}

void* _vmem_get_ptr2(u32 addr,u32& mask)
{
	u32   page=addr>>24;
	unat  iirf=(unat)_vmem_MemInfo_ptr[page];
	void* ptr=(void*)(iirf&~HANDLER_MAX);

	if (ptr==0) return 0;

	mask=0xFFFFFFFF>>iirf;
	return ptr;
}

void* _vmem_read_const(u32 addr,bool& ismem,u32 sz)
{
	u32   page=addr>>24;
	unat  iirf=(unat)_vmem_MemInfo_ptr[page];
	void* ptr=(void*)(iirf&~HANDLER_MAX);

	if (ptr==0)
	{
		ismem=false;
		const unat id=iirf;
		if (sz==1)
		{
			return (void*)_vmem_RF8[id/4];
		}
		else if (sz==2)
		{
			return (void*)_vmem_RF16[id/4];
		}
		else if (sz==4)
		{
			return (void*)_vmem_RF32[id/4];
		}
		else
		{
			die("Invalid size");
		}
	}
	else
	{
		ismem=true;
		addr<<=iirf;
		addr>>=iirf;

		return &(((u8*)ptr)[addr]);
	}
	die("Invalid memory size");

	return 0;
}

void* _vmem_page_info(u32 addr,bool& ismem,u32 sz,u32& page_sz,bool rw)
{
	u32   page=addr>>24;
	unat  iirf=(unat)_vmem_MemInfo_ptr[page];
	void* ptr=(void*)(iirf&~HANDLER_MAX);
	
	if (ptr==0)
	{
		ismem=false;
		const unat id=iirf;
		page_sz=24;
		if (sz==1)
		{
			return rw?(void*)_vmem_RF8[id/4]:(void*)_vmem_WF8[id/4];
		}
		else if (sz==2)
		{
			return rw?(void*)_vmem_RF16[id/4]:(void*)_vmem_WF16[id/4];
		}
		else if (sz==4)
		{
			return rw?(void*)_vmem_RF32[id/4]:(void*)_vmem_WF32[id/4];
		}
		else
		{
			die("Invalid size");
		}
	}
	else
	{
		ismem=true;

		page_sz=32-(iirf&0x1F);

		return ptr;
	}
	die("Invalid memory size");

	return 0;
}

//0xDEADC0D3 or 0
#define MEM_ERROR_RETURN_VALUE 0xDEADC0D3

//phew .. that was lota asm code ;) lets go back to C :D
//default mem handlers ;)
//default read handlers
u8 DYNACALL _vmem_ReadMem8_not_mapped(void*, u32 addresss)
{
	//printf("[sh4]Read8 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u8)MEM_ERROR_RETURN_VALUE;
}
u16 DYNACALL _vmem_ReadMem16_not_mapped(void*, u32 addresss)
{
	//printf("[sh4]Read16 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u16)MEM_ERROR_RETURN_VALUE;
}
u32 DYNACALL _vmem_ReadMem32_not_mapped(void*, u32 addresss)
{
	//printf("[sh4]Read32 from 0x%X, not mapped [_vmem default handler]\n",addresss);
	return (u32)MEM_ERROR_RETURN_VALUE;
}
//default write handers
void DYNACALL _vmem_WriteMem8_not_mapped(void*, u32 addresss,u8 data)
{
	//printf("[sh4]Write8 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
void DYNACALL _vmem_WriteMem16_not_mapped(void*, u32 addresss,u16 data)
{
	//printf("[sh4]Write16 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
void DYNACALL _vmem_WriteMem32_not_mapped(void*, u32 addresss,u32 data)
{
	//printf("[sh4]Write32 to 0x%X=0x%X, not mapped [_vmem default handler]\n",addresss,data);
}
//code to register handlers
//0 is considered error :)
_vmem_handler _vmem_register_handler(
									 void* ctx,
									 _vmem_ReadMem8FP* read8,
									 _vmem_ReadMem16FP* read16,
									 _vmem_ReadMem32FP* read32,

									 _vmem_WriteMem8FP* write8,
									 _vmem_WriteMem16FP* write16,
									 _vmem_WriteMem32FP* write32
									 )
{
	_vmem_handler rv=_vmem_lrp++;

	verify(rv<HANDLER_COUNT);
	_vmem_CTX[rv] = ctx;

	_vmem_RF8[rv] =read8==0  ? _vmem_ReadMem8_not_mapped  : read8;
	_vmem_RF16[rv]=read16==0 ? _vmem_ReadMem16_not_mapped : read16;
	_vmem_RF32[rv]=read32==0 ? _vmem_ReadMem32_not_mapped : read32;

	_vmem_WF8[rv] =write8==0 ? _vmem_WriteMem8_not_mapped : write8;
	_vmem_WF16[rv]=write16==0? _vmem_WriteMem16_not_mapped: write16;
	_vmem_WF32[rv]=write32==0? _vmem_WriteMem32_not_mapped: write32;

	return rv;
}
u32 FindMask(u32 msk)
{
	u32 s=-1;
	u32 rv=0;

	while(msk!=s>>rv)
		rv++;

	return rv;
}

//map a registered handler to a mem region
void _vmem_map_handler(_vmem_handler Handler,u32 start,u32 end)
{
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[i]=((u8*)0)+(0x00000000 + Handler*4);
	}
}

//map a memory block to a mem region
void _vmem_map_block(void* base,u32 start,u32 end,u32 mask)
{
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	verify((0xFF & (unat)base)==0);
	verify(base!=0);
	u32 j=0;
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[i]=&(((u8*)base)[j&mask]) + FindMask(mask) - (j & mask);
		j+=0x1000000;
	}
}

void _vmem_mirror_mapping(u32 new_region,u32 start,u32 size)
{
	u32 end=start+size-1;
	verify(start<0x100);
	verify(end<0x100);
	verify(start<=end);
	verify(!((start>=new_region) && (end<=new_region)));

	u32 j=new_region;
	for (u32 i=start;i<=end;i++)
	{
		_vmem_MemInfo_ptr[j&0xFF]=_vmem_MemInfo_ptr[i&0xFF];
		j++;
	}
}

//init/reset/term
void _vmem_init()
{
	_vmem_reset();
}

void _vmem_reset()
{
	//clear read tables
	memset(_vmem_RF8,0,sizeof(_vmem_RF8));
	memset(_vmem_RF16,0,sizeof(_vmem_RF16));
	memset(_vmem_RF32,0,sizeof(_vmem_RF32));
	
	//clear write tables
	memset(_vmem_WF8,0,sizeof(_vmem_WF8));
	memset(_vmem_WF16,0,sizeof(_vmem_WF16));
	memset(_vmem_WF32,0,sizeof(_vmem_WF32));
	
	//clear meminfo table
	memset(_vmem_MemInfo_ptr,0,sizeof(_vmem_MemInfo_ptr));

	//reset registration index
	_vmem_lrp=0;

	//register default functions (0) for slot 0
	verify(_vmem_register_handler(0,0,0,0,0,0,0)==0);
}

void _vmem_term() {}

#include "hw/pvr/pvr_mem.h"
#include "hw/sh4/sh4_mem.h"

u8* virt_ram_base;

void* malloc_pages(size_t size) {
#if HOST_OS == OS_WINDOWS
	return _aligned_malloc(size, PAGE_SIZE);
#elif defined(_ISOC11_SOURCE)
	return aligned_alloc(PAGE_SIZE, size);
#else
	void *data;
	if (posix_memalign(&data, PAGE_SIZE, size) != 0)
		return NULL;
	else
		return data;
#endif
}

void free_pages(void* ptr) {
#if HOST_OS == OS_WINDOWS
	return _aligned_free(ptr);
#elif defined(_ISOC11_SOURCE)
	return free(ptr);
#else
	return free(ptr);;
#endif
}

// Resets the FPCB table (by either clearing it to the default val
// or by flushing it and making it fault on access again.
void _vmem_bm_reset() {
	// If we allocated it via vmem:
	if (virt_ram_base)
		vmem_platform_reset_mem(p_sh4rcb->fpcb, sizeof(p_sh4rcb->fpcb));
	else
		// We allocated it via a regular malloc/new/whatever on the heap
		bm_vmem_pagefill((void**)p_sh4rcb->fpcb, sizeof(p_sh4rcb->fpcb));
}

// This gets called whenever there is a pagefault, it is possible that it lands
// on the fpcb memory range, which is allocated on miss. Returning true tells the
// fault handler this was us, and that the page is resolved and can continue the execution.
bool _vmem_bm_LockedWrite(u8* address) {
	if (!virt_ram_base)
		return false;  // No vmem, therefore not us who caused this.

	uintptr_t ptrint = (uintptr_t)address;
	uintptr_t start  = (uintptr_t)p_sh4rcb->fpcb;
	uintptr_t end    = start + sizeof(p_sh4rcb->fpcb);

	if (ptrint >= start && ptrint < end) {
		// Alloc the page then and initialize it to default values
		void *aligned_addr = (void*)(ptrint & (~PAGE_MASK));
		vmem_platform_ondemand_page(aligned_addr, PAGE_SIZE);
		bm_vmem_pagefill((void**)aligned_addr, PAGE_SIZE);
		return true;
	}
	return false;
}

bool _vmem_reserve(VLockedMemory* mram, VLockedMemory* vram, VLockedMemory* aica_ram, u32 aram_size) {
	// TODO: Static assert?
	verify((sizeof(Sh4RCB)%PAGE_SIZE)==0);

	VMemType vmemstatus = MemTypeError;

	// Use vmem only if settings mandate so, and if we have proper exception handlers.
	#ifndef TARGET_NO_EXCEPTIONS
	if (!settings.dynarec.disable_nvmem)
		vmemstatus = vmem_platform_init((void**)&virt_ram_base, (void**)&p_sh4rcb);
	#endif

	// Fallback to statically allocated buffers, this results in slow-ops being generated.
	if (vmemstatus == MemTypeError) {
		printf("Warning! nvmem is DISABLED (due to failure or not being built-in\n");
		virt_ram_base = 0;

		// Allocate it all and initialize it.
		p_sh4rcb = (Sh4RCB*)malloc_pages(sizeof(Sh4RCB));
		bm_vmem_pagefill((void**)p_sh4rcb->fpcb, sizeof(p_sh4rcb->fpcb));

		mram->size = RAM_SIZE;
		mram->data = (u8*)malloc_pages(RAM_SIZE);

		vram->size = VRAM_SIZE;
		vram->data = (u8*)malloc_pages(VRAM_SIZE);

		aica_ram->size = aram_size;
		aica_ram->data = (u8*)malloc_pages(aram_size);
	}
	else {
		printf("Info: nvmem is enabled, with addr space of size %s\n", vmemstatus == MemType4GB ? "4GB" : "512MB");
		printf("Info: p_sh4rcb: %p virt_ram_base: %p\n", p_sh4rcb, virt_ram_base);
		// Map the different parts of the memory file into the new memory range we got.
		#define MAP_RAM_START_OFFSET  0
		#define MAP_VRAM_START_OFFSET (MAP_RAM_START_OFFSET+RAM_SIZE)
		#define MAP_ARAM_START_OFFSET (MAP_VRAM_START_OFFSET+VRAM_SIZE)
		const vmem_mapping mem_mappings[] = {
			{0x00000000, 0x00800000,                               0,         0, false},  // Area 0 -> unused
			{0x00800000, 0x01000000,           MAP_ARAM_START_OFFSET, aram_size, false},  // Aica, wraps too
			{0x20000000, 0x20000000+aram_size, MAP_ARAM_START_OFFSET, aram_size,  true},
			{0x01000000, 0x04000000,                               0,         0, false},  // More unused
			{0x04000000, 0x05000000,           MAP_VRAM_START_OFFSET, VRAM_SIZE,  true},  // Area 1 (vram, 16MB, wrapped on DC as 2x8MB)
			{0x05000000, 0x06000000,                               0,         0, false},  // 32 bit path (unused)
			{0x06000000, 0x07000000,           MAP_VRAM_START_OFFSET, VRAM_SIZE,  true},  // VRAM mirror
			{0x07000000, 0x08000000,                               0,         0, false},  // 32 bit path (unused) mirror
			{0x08000000, 0x0C000000,                               0,         0, false},  // Area 2
			{0x0C000000, 0x10000000,            MAP_RAM_START_OFFSET,  RAM_SIZE,  true},  // Area 3 (main RAM + 3 mirrors)
			{0x10000000, 0x20000000,                               0,         0, false},  // Area 4-7 (unused)
		};
		vmem_platform_create_mappings(&mem_mappings[0], sizeof(mem_mappings) / sizeof(mem_mappings[0]));

		// Point buffers to actual data pointers
		aica_ram->size = aram_size;
		aica_ram->data = &virt_ram_base[0x20000000];  // Points to the writtable AICA addrspace

		vram->size = VRAM_SIZE;
		vram->data = &virt_ram_base[0x04000000];   // Points to first vram mirror (writtable and lockable)

		mram->size = RAM_SIZE;
		mram->data = &virt_ram_base[0x0C000000];   // Main memory, first mirror
	}

	// Clear out memory
	aica_ram->Zero();
	vram->Zero();
	mram->Zero();

	return true;
}

#define freedefptr(x) \
	if (x) { free_pages(x); x = NULL; }

void _vmem_release(VLockedMemory* mram, VLockedMemory* vram, VLockedMemory* aica_ram) {
	if (virt_ram_base)
		vmem_platform_destroy();
	else {
		freedefptr(p_sh4rcb);
		freedefptr(vram->data);
		freedefptr(aica_ram->data);
		freedefptr(mram->data);
	}
}


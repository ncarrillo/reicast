/*
	This file is part of libswirl
*/
#include "license/bsd"



#include <windows.h>

#include "hw/mem/_vmem.h"

// Implementation of the vmem related function for Windows platforms.
// For now this probably does some assumptions on the CPU/platform.

// This implements the VLockedMemory interface, as defined in _vmem.h
// The implementation allows it to be empty (that is, to not lock memory).

void VLockedMemory::LockRegion(unsigned offset, unsigned size) {
	#ifndef TARGET_NO_EXCEPTIONS
	//verify(offset + size < this->size && size != 0);
	DWORD old;
	VirtualProtect(&data[offset], size, PAGE_READONLY, &old);
	#endif
}

void VLockedMemory::UnLockRegion(unsigned offset, unsigned size) {
	#ifndef TARGET_NO_EXCEPTIONS
	//verify(offset + size <= this->size && size != 0);
	DWORD old;
	VirtualProtect(&data[offset], size, PAGE_READWRITE, &old);
	#endif
}

static HANDLE mem_handle = INVALID_HANDLE_VALUE, mem_handle2 = INVALID_HANDLE_VALUE;
static char * base_alloc = NULL;

// Implement vmem initialization for RAM, ARAM, VRAM and SH4 context, fpcb etc.
// The function supports allocating 512MB or 4GB addr spaces.

// Plase read the POSIX implementation for more information. On Windows this is
// rather straightforward.
VMemType vmem_platform_init(void **vmem_base_addr, void **sh4rcb_addr) {
	// Firt allocate the actual address space (it will be 64KB aligned on windows).
	unsigned memsize = 512 * 1024 * 1024 + sizeof(Sh4RCB) + ARAM_SIZE_MAX;
	base_alloc = (char*)VirtualAlloc(0, memsize, MEM_RESERVE, PAGE_NOACCESS);
	if (base_alloc == NULL) {
		return MemTypeError;
	}

	// Now let's try to allocate the in-memory file
	mem_handle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, RAM_SIZE_MAX + VRAM_SIZE_MAX + ARAM_SIZE_MAX, 0);
	verify(mem_handle != INVALID_HANDLE_VALUE);


	// Calculate pointers now
	*sh4rcb_addr = &base_alloc[0];
	*vmem_base_addr = &base_alloc[sizeof(Sh4RCB)];

	return MemType512MB;
}

// Just tries to wipe as much as possible in the relevant area.
void vmem_platform_destroy() {
	VirtualFree(base_alloc, 0, MEM_RELEASE);
	CloseHandle(mem_handle);
}

// Resets a chunk of memory by deleting its data and setting its protection back.
void vmem_platform_reset_mem(void *ptr, unsigned size_bytes) {
	VirtualFree(ptr, size_bytes, MEM_DECOMMIT);
}

// Allocates a bunch of memory (page aligned and page-sized)
void vmem_platform_ondemand_page(void *address, unsigned size_bytes) {
	verify(NULL != VirtualAlloc(address, size_bytes, MEM_COMMIT, PAGE_READWRITE));
}

/// Creates mappings to the underlying file including mirroring sections
void vmem_platform_create_mappings(const vmem_mapping *vmem_maps, unsigned nummaps) {
	// Since this is tricky to get right in Windows (in posix one can just unmap sections and remap later)
	// we unmap the whole thing only to remap it later.

	// Unmap the whole section
	VirtualFree(base_alloc, 0, MEM_RELEASE);

	// Map the SH4CB block too
	void *base_ptr = VirtualAlloc(base_alloc, sizeof(Sh4RCB), MEM_RESERVE, PAGE_NOACCESS);
	verify(base_ptr == base_alloc);
	void *cntx_ptr = VirtualAlloc((u8*)p_sh4rcb + sizeof(p_sh4rcb->fpcb), sizeof(Sh4RCB) - sizeof(p_sh4rcb->fpcb), MEM_COMMIT, PAGE_READWRITE);
	verify(cntx_ptr == (u8*)p_sh4rcb + sizeof(p_sh4rcb->fpcb));

	for (unsigned i = 0; i < nummaps; i++) {
		unsigned address_range_size = vmem_maps[i].end_address - vmem_maps[i].start_address;
		DWORD protection = vmem_maps[i].allow_writes ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

		if (!vmem_maps[i].memsize) {
			// Unmapped stuff goes with a protected area or memory. Prevent anything from allocating here
			void *ptr = VirtualAlloc(&virt_ram_base[vmem_maps[i].start_address], address_range_size, MEM_RESERVE, PAGE_NOACCESS);
			verify(ptr == &virt_ram_base[vmem_maps[i].start_address]);
		}
		else {
			// Calculate the number of mirrors
			unsigned num_mirrors = (address_range_size) / vmem_maps[i].memsize;
			verify((address_range_size % vmem_maps[i].memsize) == 0 && num_mirrors >= 1);

			// Remap the views one by one
			for (unsigned j = 0; j < num_mirrors; j++) {
				unsigned offset = vmem_maps[i].start_address + j * vmem_maps[i].memsize;

				void *ptr = MapViewOfFileEx(mem_handle, protection, 0, vmem_maps[i].memoffset,
				                    vmem_maps[i].memsize, &virt_ram_base[offset]);
				verify(ptr == &virt_ram_base[offset]);
			}
		}
	}
}

typedef void* (*mapper_fn) (void *addr, unsigned size);

// This is a tempalted function since it's used twice
static void* vmem_platform_prepare_jit_block_template(void *code_area, unsigned size, mapper_fn mapper) {
	// Several issues on Windows: can't protect arbitrary pages due to (I guess) the way
	// kernel tracks mappings, so only stuff that has been allocated with VirtualAlloc can be
	// protected (the entire allocation IIUC).

	// Strategy: ignore code_area and allocate a new one. Protect it properly.
	// More issues: the area should be "close" to the .text stuff so that code gen works.
	// Remember that on x64 we have 4 byte jump/load offset immediates, no issues on x86 :D

	// Take this function addr as reference.
	uintptr_t base_addr = reinterpret_cast<uintptr_t>(&vmem_platform_init) & ~0xFFFFF;

	// Probably safe to assume reicast code is <200MB (today seems to be <16MB on every platform I've seen).
	for (unsigned i = 0; i < 1800*1024*1024; i += 8*1024*1024) {  // Some arbitrary step size.
		uintptr_t try_addr_above = base_addr + i;
		uintptr_t try_addr_below = base_addr - i;

		// We need to make sure there's no address wrap around the end of the addrspace (meaning: int overflow).
		if (try_addr_above > base_addr) {
			void *ptr = mapper((void*)try_addr_above, size);
			if (ptr)
				return ptr;
		}
		if (try_addr_below < base_addr) {
			void *ptr = mapper((void*)try_addr_below, size);
			if (ptr)
				return ptr;
		}
	}
	return NULL;
}

static void* mem_alloc(void *addr, unsigned size) {
	return VirtualAlloc(addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

// Prepares the code region for JIT operations, thus marking it as RWX
bool vmem_platform_prepare_jit_block(void *code_area, unsigned size, void **code_area_rwx) {
	// Get the RWX page close to the code_area
	void *ptr = vmem_platform_prepare_jit_block_template(code_area, size, &mem_alloc);
	if (!ptr)
		return false;

	*code_area_rwx = ptr;
	printf("Found code area at %p, not too far away from %p\n", *code_area_rwx, &vmem_platform_init);

	// We should have found some area in the addrspace, after all size is ~tens of megabytes.
	// Pages are already RWX, all done
	return true;
}


static void* mem_file_map(void *addr, unsigned size) {
	// Maps the entire file at the specified addr.
	void *ptr = VirtualAlloc(addr, size, MEM_RESERVE, PAGE_NOACCESS);
	if (!ptr)
		return NULL;
	VirtualFree(ptr, 0, MEM_RELEASE);
	if (ptr != addr)
		return NULL;

	return MapViewOfFileEx(mem_handle2, FILE_MAP_READ | FILE_MAP_EXECUTE, 0, 0, size, addr);
}

// Use two addr spaces: need to remap something twice, therefore use CreateFileMapping()
bool vmem_platform_prepare_jit_block(void *code_area, unsigned size, void **code_area_rw, uintptr_t *rx_offset) {
	mem_handle2 = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE, 0, size, 0);

	// Get the RX page close to the code_area
	void *ptr_rx = vmem_platform_prepare_jit_block_template(code_area, size, &mem_file_map);
	if (!ptr_rx)
		return false;

	// Ok now we just remap the RW segment at any position (dont' care).
	void *ptr_rw = MapViewOfFileEx(mem_handle2, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size, NULL);

	*code_area_rw = ptr_rw;
	*rx_offset = (char*)ptr_rx - (char*)ptr_rw;
	printf("Info: Using NO_RWX mode, rx ptr: %p, rw ptr: %p, offset: %lu\n", ptr_rx, ptr_rw, (unsigned long)*rx_offset);

	return (ptr_rw != NULL);
}


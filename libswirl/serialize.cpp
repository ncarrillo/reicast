/*
	This file is part of libswirl
*/
#include "license/bsd"


// serialize.cpp : save states
#if 1
#include "types.h"
#include "hw/aica/dsp_backend.h"
#include "hw/aica/aica.h"
#include "hw/aica/sgc_if.h"
#include "hw/arm7/arm7.h"
#include "hw/holly/sh4_mem_area0.h"
#include "hw/flashrom/flashrom.h"
#include "hw/mem/_vmem.h"
#include "hw/gdrom/gdromv3.h"
#include "hw/maple/maple_cfg.h"
#include "hw/pvr/Renderer_if.h"
#include "hw/pvr/ta_structs.h"
#include "hw/sh4/sh4_interrupts.h"
#include "hw/sh4/sh4_sched.h"
#include "hw/sh4/sh4_mmr.h"
#include "hw/sh4/modules/mmu.h"
#include "hw/gdrom/disc_common.h"
#include "reios/reios.h"
#include <map>
#include <set>
#include "rend/gles/gles.h"
#include "hw/sh4/dyna/blockmanager.h"
#include "hw/sh4/dyna/ngen.h"
#include "hw/naomi/naomi_cart.h"

/*
 * search for "maybe" to find items that were left out that may be needed
 */

extern "C" void DYNACALL TAWriteSQ(u32 address,u8* sqb);

enum serialize_version_enum {
	V1,
	V2,
	V3,
	V4,
} ;

//gdrom

//./core/hw/arm7/arm_mem.cpp


//./core/hw/arm7/arm7.cpp

/*
	if AREC dynarec enabled:
	vector<ArmDPOP> ops;
	u8* icPtr;
	u8* ICache;
	u8 ARM7_TCB[ICacheSize+4096];
	void* EntryPoints[8*1024*1024/4];
	map<u32,u32> renamed_regs;
	u32 rename_reg_base;
	u32 nfb,ffb,bfb,mfb;
	static x86_block* x86e;
	x86_Label* end_lbl;
	u8* ARM::emit_opt=0;
	eReg ARM::reg_addr;
	eReg ARM::reg_dst;
	s32 ARM::imma;
*/




//./core/hw/aica/dsp.o

//recheck dsp.cpp if FEAT_DSPREC == DYNAREC_JIT




//./core/hw/aica/aica.o
//these are all just pointers into aica_reg
//extern CommonData_struct* CommonData;
//extern DSPData_struct* DSPData;
//extern InterruptInfo* MCIEB;
//extern InterruptInfo* MCIPD;
//extern InterruptInfo* MCIRE;
//extern InterruptInfo* SCIEB;
//extern InterruptInfo* SCIPD;
//extern InterruptInfo* SCIRE;


//./core/hw/aica/aica_if.o

//extern s32 aica_pending_dma ;

//./core/hw/aica/aica_mem.o


//./core/hw/aica/sgc_if.o
struct ChannelEx;
#define AicaChannel ChannelEx
extern s32 volume_lut[16];
extern s32 tl_lut[256 + 768];	//xx.15 format. >=255 is muted
extern u32 AEG_ATT_SPS[64];
extern u32 AEG_DSR_SPS[64];
extern s16 pl;
extern s16 pr;
//this is just a pointer to aica_reg
//extern DSP_OUT_VOL_REG* dsp_out_vol;
//not needed - one-time init
//void(* 	STREAM_STEP_LUT [5][2][2])(ChannelEx *ch)
//void(* 	STREAM_INITAL_STEP_LUT [5])(ChannelEx *ch)
//void(* 	AEG_STEP_LUT [4])(ChannelEx *ch)
//void(* 	FEG_STEP_LUT [4])(ChannelEx *ch)
//void(* 	ALFOWS_CALC [4])(ChannelEx *ch)
//void(* 	PLFOWS_CALC [4])(ChannelEx *ch)

//special handling




//./core/hw/holly/sb.o


//./core/hw/holly/sb_mem.o
//unused
//static HollyInterruptID dmatmp1;
//static HollyInterruptID dmatmp2;
//static HollyInterruptID OldDmaId;

//this is one-time init, no updates - don't need to serialize
//extern RomChip sys_rom;
#ifdef FLASH_SIZE
extern DCFlashChip sys_nvmem;
#endif

#ifdef BBSRAM_SIZE
extern SRamChip sys_nvmem;
#endif
//this is one-time init, no updates - don't need to serialize
//extern _vmem_handler area0_handler;




//./core/hw/gdrom/gdrom_response.o
extern u16 reply_11[] ;



//./core/hw/maple/maple_devs.o
extern char EEPROM[0x100];
extern bool EEPROM_loaded;

//./core/hw/maple/maple_if.o
//one time set
//incremented but never read
//extern u32 dmacount;
extern bool maple_ddt_pending_reset;


//./core/hw/modem/modem.cpp

//./core/hw/pvr/Renderer_if.o
//only written - not read
//extern u32 VertexCount;
extern u32 FrameCount;
//one-time init
//extern Renderer* renderer;
//these are just mutexes used during rendering
//extern cResetEvent rs;
//extern cResetEvent re;
//these max_?? through ovrn are written and not read
//extern int max_idx;
//extern int max_mvo;
//extern int max_op;
//extern int max_pt;
//extern int max_tr;
//extern int max_vtx;
//extern int max_modt;
//extern int ovrn;
//seems safe to omit this - gets refreshed every frame from a pool
//extern TA_context* _pvrrc;
//just a flag indiating the rendering thread is running
//extern int rend_en ;
//the renderer thread - one time set
//extern cThread rthd;
extern bool pend_rend;

//these will all get cleared out after a few frames - no need to serialize
//static bool render_called = false;
//u32 fb1_watch_addr_start;
//u32 fb1_watch_addr_end;
//u32 fb2_watch_addr_start;
//u32 fb2_watch_addr_end;
//bool fb_dirty;


//maybe
//extern u32 memops_t,memops_l;


//./core/hw/pvr/pvr_mem.o
extern u32 YUV_tempdata[512/4];//512 bytes
extern u32 YUV_dest;
extern u32 YUV_blockcount;
extern u32 YUV_x_curr;
extern u32 YUV_y_curr;
extern u32 YUV_x_size;
extern u32 YUV_y_size;




//./core/hw/pvr/pvr_regs.o
extern bool fog_needs_update;
extern u8 pvr_regs[pvr_RegSize];




//./core/hw/pvr/spg.o
extern u32 in_vblank;
extern u32 clc_pvr_scanline;
extern u32 pvr_numscanlines;
extern u32 prv_cur_scanline;
extern u32 vblk_cnt;
extern u32 Line_Cycles;
extern u32 Frame_Cycles;
extern int vblank_schid;
extern int time_sync;
extern double speed_load_mspdf;
extern int mips_counter;
extern double full_rps;
extern u32 fskip;




//./core/hw/pvr/ta.o
extern u32 ta_type_lut[256];
extern u8 ta_fsm[2049];	//[2048] stores the current state
extern u32 ta_fsm_cl;




//./core/hw/pvr/ta_ctx.o
//these frameskipping don't need to be saved
//extern int frameskip;
//extern bool FrameSkipping;		// global switch to enable/disable frameskip
//maybe need these - but hopefully not
//extern TA_context* ta_ctx;
//extern tad_context ta_tad;
//extern TA_context*  vd_ctx;
//extern rend_context vd_rc;
//extern cMutex mtx_rqueue;
//extern TA_context* rqueue;
//extern cResetEvent frame_finished;
//extern cMutex mtx_pool;
//extern vector<TA_context*> ctx_pool;
//extern vector<TA_context*> ctx_list;
//end maybe



//./core/hw/pvr/ta_vtx.o
extern bool pal_needs_update;
//extern u32 decoded_colors[3][65536];
extern u32 tileclip_val;
extern u8 f32_su8_tbl[65536];
//written but never read
//extern ModTriangle* lmr;
//never changed
//extern PolyParam nullPP;
//maybe
//extern PolyParam* CurrentPP;
//maybe
//extern List<PolyParam>* CurrentPPlist;
//TA state vars
extern DECL_ALIGN(4) u8 FaceBaseColor[4];
extern DECL_ALIGN(4) u8 FaceOffsColor[4];
extern DECL_ALIGN(4) u8 FaceBaseColor1[4];
extern DECL_ALIGN(4) u8 FaceOffsColor1[4];
extern DECL_ALIGN(4) u32 SFaceBaseColor;
extern DECL_ALIGN(4) u32 SFaceOffsColor;
//maybe
//extern TaListFP* TaCmd;
//maybe
//extern u32 CurrentList;
//maybe
//extern TaListFP* VertexDataFP;
//written but never read
//extern bool ListIsFinished[5];
//maybe ; need special handler
//FifoSplitter<0> TAFifo0;
//counter for frameskipping - doesn't need to be saved
//extern int ta_parse_cnt;




//./core/rend/TexCache.o
//maybe
//extern u8* vq_codebook;
//extern u32 palette_index;
//extern bool KillTex;
//extern u32 detwiddle[2][8][1024];
//maybe
//extern vector<vram_block*> VramLocks[/*VRAM_SIZE*/(16*1024*1024)/PAGE_SIZE];
//maybe - probably not - just a locking mechanism
//extern cMutex vramlist_lock;




//./core/hw/sh4/sh4_mmr.o


//./core/hw/sh4/sh4_mem.o
//one-time init
//extern _vmem_handler area1_32b;
//one-time init
//extern _vmem_handler area5_handler;




//./core/hw/sh4/sh4_interrupts.o
extern u16 IRLPriority;
//one-time init
//extern InterptSourceList_Entry InterruptSourceList[28];
extern DECL_ALIGN(64) u16 InterruptEnvId[32];
extern DECL_ALIGN(64) u32 InterruptBit[32];
extern DECL_ALIGN(64) u32 InterruptLevelBit[16];
extern u32 interrupt_vpend; // Vector of pending interrupts
extern u32 interrupt_vmask; // Vector of masked interrupts             (-1 inhibits all interrupts)
extern u32 decoded_srimask; // Vector of interrupts allowed by SR.IMSK (-1 inhibits all interrupts)




//./core/hw/sh4/sh4_core_regs.o
extern Sh4RCB* p_sh4rcb;
//just method pointers
//extern sh4_if  sh4_cpu;
//one-time set
//extern u8* sh4_dyna_rcb;
extern u32 old_rm;
extern u32 old_dn;




//./core/hw/sh4/sh4_sched.o

//extern int sh4_sched_next_id;




//./core/hw/sh4/interpr/sh4_interpreter.o



//./core/hw/sh4/modules/serial.o
extern SCIF_SCFSR2_type SCIF_SCFSR2;
extern u8 SCIF_SCFRDR2;
extern SCIF_SCFDR2_type SCIF_SCFDR2;




//./core/hw/sh4/modules/bsc.o



//./core/hw/sh4/modules/tmu.o
extern u32 tmu_shift[3];
extern u32 tmu_mask[3];
extern u64 tmu_mask64[3];
extern u32 old_mode[3];
extern u32 tmu_ch_base[3];
extern u64 tmu_ch_base64[3];




//./core/hw/sh4/modules/ccn.o
extern u32 CCN_QACR_TR[2];




//./core/hw/sh4/modules/mmu.o
extern TLB_Entry UTLB[64];
extern TLB_Entry ITLB[4];
#if defined(NO_MMU)
extern u32 sq_remap[64];
#else
extern u32 ITLB_LRU_USE[64];
extern u32 mmu_error_TT;
#endif



//./core/imgread/common.o
extern u32 NullDriveDiscType;
//maybe - seems to all be one-time inits ;    needs special handler
//extern Disc* disc;
extern u8 q_subchannel[96];




//./core/nullDC.o
//extern unsigned FLASH_SIZE;
//extern unsigned BBSRAM_SIZE;
//extern unsigned BIOS_SIZE;
//extern unsigned RAM_SIZE;
//extern unsigned ARAM_SIZE;
//extern unsigned VRAM_SIZE;
//extern unsigned RAM_MASK;
//extern unsigned ARAM_MASK;
//extern unsigned VRAM_MASK;
//settings can be dynamic
//extern settings_t settings;




//./core/reios/reios.o
//never used
//extern u8* biosrom;
//one time init
//extern u8* flashrom;
//one time init
//extern u32 base_fad ;
//one time init
//extern bool descrambl;
//one time init
//extern bool bootfile_inited;
//all these reios_?? are one-time inits
//extern char reios_bootfile[32];
//extern char reios_hardware_id[17];
//extern char reios_maker_id[17];
//extern char reios_device_info[17];
//extern char reios_area_symbols[9];
//extern char reios_peripherals[9];
//extern char reios_product_number[11];
//extern char reios_product_version[7];
//extern char reios_releasedate[17];
//extern char reios_boot_filename[17];
//extern char reios_software_company[17];
//extern char reios_software_name[129];
//one time init
//extern map<u32, hook_fp*> hooks;
//one time init
//extern map<hook_fp*, u32> hooks_rev;




//./core/reios/gdrom_hle.o
//never used in any meaningful way
//extern u32 SecMode[4];




//./core/reios/descrambl.o
//random seeds can be...random
//extern unsigned int seed;



//./core/rend/gles/gles.o
//maybe
//extern GLCache glcache;
//maybe
//extern gl_ctx gl;
//maybe
//extern struct ShaderUniforms_t ShaderUniforms;
//maybe
//extern u32 gcflip;
//maybe
//extern float fb_scale_x;
//extern float fb_scale_y;
//extern float scale_x;
//extern float scale_y;
//extern int screen_width;
//extern int screen_height;
//extern GLuint fogTextureId;
//end maybe



//./core/rend/gles/gldraw.o
//maybe
//extern PipelineShader* CurrentShader;
//written, but never used
//extern Vertex* vtx_sort_base;
//maybe
//extern vector<SortTrigDrawParam>	pidx_sort;




//./core/rend/gles/gltex.o
//maybe ; special handler
//extern map<u64,TextureCacheData> TexCache;
//maybe
//extern FBT fb_rtt;
//not used
//static int TexCacheLookups;
//static int TexCacheHits;
//static float LastTexCacheStats;
//maybe should get reset naturally if needed
//GLuint fbTextureId;




//./core/hw/naomi/naomi.o
extern u32 naomi_updates;
extern u32 GSerialBuffer;
extern u32 BSerialBuffer;
extern int GBufPos;
extern int BBufPos;
extern int GState;
extern int BState;
extern int GOldClk;
extern int BOldClk;
extern int BControl;
extern int BCmd;
extern int BLastCmd;
extern int GControl;
extern int GCmd;
extern int GLastCmd;
extern int SerStep;
extern int SerStep2;
extern unsigned char BSerial[];
extern unsigned char GSerial[];
extern u32 reg_dimm_3c;	//IO window ! writen, 0x1E03 some flag ?
extern u32 reg_dimm_40;	//parameters
extern u32 reg_dimm_44;	//parameters
extern u32 reg_dimm_48;	//parameters
extern u32 reg_dimm_4c;	//status/control reg ?
extern bool NaomiDataRead;




//./core/hw/naomi/naomi_cart.o
//all one-time loads
//u8* RomPtr;
//u32 RomSize;
//fd_t*	RomCacheMap;
//u32		RomCacheMapCount;




//./core/rec-x64/rec_x64.o
//maybe need special handler
//extern BlockCompilerx64 *compilerx64_data;




//./core/rec.o



//./core/hw/sh4/dyna/decoder.o
//temp storage only
//extern RuntimeBlockInfo* blk;



//./core/hw/sh4/dyna/driver.o
//extern u8 SH4_TCB[CODE_SIZE+4096];
//one time ptr set
//extern u8* CodeCache



//./core/hw/sh4/dyna/blockmanager.o
//cleared but never read
//extern bm_List blocks_page[/*BLOCKS_IN_PAGE_LIST_COUNT*/(32*1024*1024)/4096];
//maybe - the next three seem to be list of precompiled blocks of code - but if not found will populate
//extern bm_List all_blocks;
//extern bm_List del_blocks;
//extern blkmap_t blkmap;
//these two are never referenced
//extern u32 bm_gc_luc;
//extern u32 bm_gcf_luc;
//data is never written to this
//extern u32 PAGE_STATE[(32*1024*1024)/*RAM_SIZE*//32];
//never read
//extern u32 total_saved;
//counter with no real controlling logic behind it
//extern u32 rebuild_counter;
//just printf output
//extern bool print_stats;




//./core/hw/sh4/dyna/shil.o
extern u32 RegisterWrite[sh4_reg_count];
extern u32 RegisterRead[sh4_reg_count];
extern u32 fallback_blocks;
extern u32 total_blocks;
extern u32 REMOVED_OPS;




//./core/linux-dist/main.cpp, ./core/windows/winmain.cpp , ...
extern u16 kcode[4];
extern u8 rt[4];
extern u8 lt[4];
extern u32 vks[4];
extern s8 joyx[4];
extern s8 joyy[4];


bool rc_serialize(void *src, unsigned int src_size, void **dest, unsigned int *total_size)
{
	if ( *dest != NULL )
	{
		memcpy(*dest, src, src_size) ;
		*dest = ((unsigned char*)*dest) + src_size ;
	}

	*total_size += src_size ;
	return true ;
}

bool rc_unserialize(void *src, unsigned int src_size, void **dest, unsigned int *total_size)
{
	if ( *dest != NULL )
	{
		memcpy(src, *dest, src_size) ;
		*dest = ((unsigned char*)*dest) + src_size ;
	}

	*total_size += src_size ;
	return true ;
}

bool register_serialize(RegisterStruct* regs, size_t size, void **data, unsigned int *total_size )
{
	int i = 0 ;

	for ( i = 0 ; i < size ; i++ )
	{
		REICAST_S(regs[i].flags) ;
		REICAST_S(regs[i].data32) ;
	}

	return true ;
}

bool register_unserialize(RegisterStruct* regs, size_t size,void **data, unsigned int *total_size )
{
	int i = 0 ;
	u32 dummy = 0 ;

	for ( i = 0 ; i < size; i++ )
	{
		REICAST_US(regs[i].flags) ;
		if ( ! (regs[i].flags & REG_RF) )
			REICAST_US(regs[i].data32) ;
		else
			REICAST_US(dummy) ;
	}
	return true ;
}

bool dc_serialize(void **data, unsigned int *total_size)
{
	int i = 0;
	serialize_version_enum version = V4 ;

	*total_size = 0 ;

	//dc not initialized yet
	if ( p_sh4rcb == NULL )
		return false ;

	REICAST_S(version) ;

	sh4_cpu->serialize(data, total_size);

	REICAST_SA(sh4_cpu->aica_ram.data, sh4_cpu->aica_ram.size);



	REICAST_SA(volume_lut,16);
	REICAST_SA(tl_lut,256 + 768);
	REICAST_SA(AEG_ATT_SPS,64);
	REICAST_SA(AEG_DSR_SPS,64);
	REICAST_S(pl);
	REICAST_S(pr);

	//this is one-time init, no updates - don't need to serialize
	//extern RomChip sys_rom;
	REICAST_S(sys_nvmem.size);
	REICAST_S(sys_nvmem.mask);
#ifdef FLASH_SIZE
	REICAST_S(sys_nvmem.state);
#endif
	REICAST_SA(sys_nvmem.data,sys_nvmem.size);

	//this is one-time init, no updates - don't need to serialize
	//extern _vmem_handler area0_handler;




	REICAST_SA(reply_11,16) ;


	REICAST_SA(EEPROM,0x100);
	REICAST_S(EEPROM_loaded);


	REICAST_S(maple_ddt_pending_reset);

	mcfg_SerializeDevices(data, total_size);

	REICAST_S(FrameCount);
	

	REICAST_SA(YUV_tempdata,512/4);
	REICAST_S(YUV_dest);
	REICAST_S(YUV_blockcount);
	REICAST_S(YUV_x_curr);
	REICAST_S(YUV_y_curr);
	REICAST_S(YUV_x_size);
	REICAST_S(YUV_y_size);

	REICAST_SA(pvr_regs,pvr_RegSize);

	REICAST_S(in_vblank);
	REICAST_S(clc_pvr_scanline);
	REICAST_S(pvr_numscanlines);
	REICAST_S(prv_cur_scanline);
	REICAST_S(vblk_cnt);
	REICAST_S(Line_Cycles);
	REICAST_S(Frame_Cycles);
	REICAST_S(speed_load_mspdf);
	REICAST_S(mips_counter);
	REICAST_S(full_rps);
	REICAST_S(fskip);



	REICAST_SA(ta_type_lut,256);
	REICAST_SA(ta_fsm,2049);
	REICAST_S(ta_fsm_cl);

	REICAST_S(tileclip_val);
	REICAST_SA(f32_su8_tbl,65536);
	REICAST_SA(FaceBaseColor,4);
	REICAST_SA(FaceOffsColor,4);
	REICAST_S(SFaceBaseColor);
	REICAST_S(SFaceOffsColor);

	REICAST_SA(sh4_cpu->vram.data, sh4_cpu->vram.size);

	REICAST_SA(sh4_cpu->mram.data, sh4_cpu->mram.size);



	REICAST_S(IRLPriority);
	REICAST_SA(InterruptEnvId,32);
	REICAST_SA(InterruptBit,32);
	REICAST_SA(InterruptLevelBit,16);
	REICAST_S(interrupt_vpend);
	REICAST_S(interrupt_vmask);
	REICAST_S(decoded_srimask);


	//default to nommu_full
	i = 3 ;
	if ( do_sqw_nommu == &do_sqw_nommu_area_3)
		i = 0 ;
	else if (do_sqw_nommu == &do_sqw_nommu_area_3_nonvmem)
		i = 1 ;
	else if (do_sqw_nommu==(sqw_fp*)&TAWriteSQ)
		i = 2 ;
	else if (do_sqw_nommu==&do_sqw_nommu_full)
		i = 3 ;

	REICAST_S(i) ;

	REICAST_SA((*p_sh4rcb).sq_buffer,64/8);

	//store these before unserializing and then restore after
	//void *getptr = &((*p_sh4rcb).cntx.sr.GetFull) ;
	//void *setptr = &((*p_sh4rcb).cntx.sr.SetFull) ;
	REICAST_S((*p_sh4rcb).cntx);

	REICAST_S(old_rm);
	REICAST_S(old_dn);


	sh4_sched_serialize(data, total_size);

	REICAST_S(SCIF_SCFSR2);
	REICAST_S(SCIF_SCFRDR2);
	REICAST_S(SCIF_SCFDR2);

	REICAST_SA(tmu_shift,3);
	REICAST_SA(tmu_mask,3);
	REICAST_SA(tmu_mask64,3);
	REICAST_SA(old_mode,3);
	REICAST_SA(tmu_ch_base,3);
	REICAST_SA(tmu_ch_base64,3);

	REICAST_SA(CCN_QACR_TR,2);

	REICAST_SA(UTLB,64);
	REICAST_SA(ITLB,4);
#if defined(NO_MMU)
	REICAST_SA(sq_remap,64);
#else
	REICAST_SA(ITLB_LRU_USE,64);
	REICAST_S(mmu_error_TT);
#endif

	REICAST_S(NullDriveDiscType);
	REICAST_SA(q_subchannel,96);

	REICAST_S(naomi_updates);
	REICAST_S(i);	//BoardID
	REICAST_S(GSerialBuffer);
	REICAST_S(BSerialBuffer);
	REICAST_S(GBufPos);
	REICAST_S(BBufPos);
	REICAST_S(GState);
	REICAST_S(BState);
	REICAST_S(GOldClk);
	REICAST_S(BOldClk);
	REICAST_S(BControl);
	REICAST_S(BCmd);
	REICAST_S(BLastCmd);
	REICAST_S(GControl);
	REICAST_S(GCmd);
	REICAST_S(GLastCmd);
	REICAST_S(SerStep);
	REICAST_S(SerStep2);
	REICAST_SA(BSerial,69);
	REICAST_SA(GSerial,69);
	REICAST_S(reg_dimm_3c);
	REICAST_S(reg_dimm_40);
	REICAST_S(reg_dimm_44);
	REICAST_S(reg_dimm_48);
	REICAST_S(reg_dimm_4c);
	REICAST_S(NaomiDataRead);

	REICAST_SA(RegisterWrite,sh4_reg_count);
	REICAST_SA(RegisterRead,sh4_reg_count);
	REICAST_S(fallback_blocks);
	REICAST_S(total_blocks);
	REICAST_S(REMOVED_OPS);

	REICAST_SA(kcode,4);
	REICAST_SA(rt,4);
	REICAST_SA(lt,4);
	REICAST_SA(vks,4);
	REICAST_SA(joyx,4);
	REICAST_SA(joyy,4);

	REICAST_S(settings.dreamcast.broadcast);
	REICAST_S(settings.dreamcast.cable);
	REICAST_S(settings.dreamcast.region);

	if (CurrentCartridge != NULL)
	   CurrentCartridge->Serialize(data, total_size);

	return true ;
}

bool dc_unserialize(void **data, unsigned int *total_size)
{
	int i = 0;
	serialize_version_enum version = V1 ;

	*total_size = 0 ;

	REICAST_US(version) ;

	if (version != V4)
	{
		fprintf(stderr, "Save State version not supported: %d\n", version);
		return false;
	}

	sh4_cpu->unserialize(data, total_size);

	REICAST_USA(sh4_cpu->aica_ram.data, sh4_cpu->aica_ram.size);
	
	REICAST_USA(volume_lut,16);
	REICAST_USA(tl_lut,256 + 768);
	REICAST_USA(AEG_ATT_SPS,64);
	REICAST_USA(AEG_DSR_SPS,64);
	REICAST_US(pl);
	REICAST_US(pr);

	//this is one-time init, no updates - don't need to serialize
	//extern RomChip sys_rom;
	REICAST_US(sys_nvmem.size);
	REICAST_US(sys_nvmem.mask);
#ifdef FLASH_SIZE
	REICAST_US(sys_nvmem.state);
#endif
	REICAST_USA(sys_nvmem.data,sys_nvmem.size);


	//this is one-time init, no updates - don't need to serialize
	//extern _vmem_handler area0_handler;




	REICAST_USA(reply_11,16) ;


	REICAST_USA(EEPROM,0x100);
	REICAST_US(EEPROM_loaded);


	REICAST_US(maple_ddt_pending_reset);

	mcfg_UnserializeDevices(data, total_size);

	REICAST_US(FrameCount);

	REICAST_USA(YUV_tempdata,512/4);
	REICAST_US(YUV_dest);
	REICAST_US(YUV_blockcount);
	REICAST_US(YUV_x_curr);
	REICAST_US(YUV_y_curr);
	REICAST_US(YUV_x_size);
	REICAST_US(YUV_y_size);

	REICAST_USA(pvr_regs,pvr_RegSize);
	fog_needs_update = true ;

	REICAST_US(in_vblank);
	REICAST_US(clc_pvr_scanline);
	REICAST_US(pvr_numscanlines);
	REICAST_US(prv_cur_scanline);
	REICAST_US(vblk_cnt);
	REICAST_US(Line_Cycles);
	REICAST_US(Frame_Cycles);
	REICAST_US(speed_load_mspdf);
	REICAST_US(mips_counter);
	REICAST_US(full_rps);
	REICAST_US(fskip);

	REICAST_USA(ta_type_lut,256);
	REICAST_USA(ta_fsm,2049);
	REICAST_US(ta_fsm_cl);

	REICAST_US(tileclip_val);
	REICAST_USA(f32_su8_tbl,65536);
	REICAST_USA(FaceBaseColor,4);
	REICAST_USA(FaceOffsColor,4);
	REICAST_US(SFaceBaseColor);
	REICAST_US(SFaceOffsColor);

	pal_needs_update = true;

	REICAST_USA(sh4_cpu->vram.data, sh4_cpu->vram.size);

	REICAST_USA(sh4_cpu->mram.data, sh4_cpu->mram.size);

	REICAST_US(IRLPriority);
	REICAST_USA(InterruptEnvId,32);
	REICAST_USA(InterruptBit,32);
	REICAST_USA(InterruptLevelBit,16);
	REICAST_US(interrupt_vpend);
	REICAST_US(interrupt_vmask);
	REICAST_US(decoded_srimask);

	REICAST_US(i) ;
	if ( i == 0 )
		do_sqw_nommu = &do_sqw_nommu_area_3 ;
	else if ( i == 1 )
		do_sqw_nommu = &do_sqw_nommu_area_3_nonvmem ;
	else if ( i == 2 )
		do_sqw_nommu = (sqw_fp*)&TAWriteSQ ;
	else if ( i == 3 )
		do_sqw_nommu = &do_sqw_nommu_full ;



	REICAST_USA((*p_sh4rcb).sq_buffer,64/8);

	//store these before unserializing and then restore after
	//void *getptr = &((*p_sh4rcb).cntx.sr.GetFull) ;
	//void *setptr = &((*p_sh4rcb).cntx.sr.SetFull) ;
	REICAST_US((*p_sh4rcb).cntx);
	//(*p_sh4rcb).cntx.sr.GetFull = getptr ;
	//(*p_sh4rcb).cntx.sr.SetFull = setptr ;

	REICAST_US(old_rm);
	REICAST_US(old_dn);

	sh4_sched_unserialize(data, total_size);
	
	REICAST_US(SCIF_SCFSR2);
	REICAST_US(SCIF_SCFRDR2);
	REICAST_US(SCIF_SCFDR2);


	REICAST_USA(tmu_shift,3);
	REICAST_USA(tmu_mask,3);
	REICAST_USA(tmu_mask64,3);
	REICAST_USA(old_mode,3);
	REICAST_USA(tmu_ch_base,3);
	REICAST_USA(tmu_ch_base64,3);




	REICAST_USA(CCN_QACR_TR,2);




	REICAST_USA(UTLB,64);
	REICAST_USA(ITLB,4);
#if defined(NO_MMU)
	REICAST_USA(sq_remap,64);
#else
	REICAST_USA(ITLB_LRU_USE,64);
	REICAST_US(mmu_error_TT);
#endif


	REICAST_US(NullDriveDiscType);
	REICAST_USA(q_subchannel,96);

// FIXME
//	REICAST_US(i);	// FLASH_SIZE
//	REICAST_US(i);	// BBSRAM_SIZE
//	REICAST_US(i);	// BIOS_SIZE
//	REICAST_US(i);	// RAM_SIZE
//	REICAST_US(i);	// ARAM_SIZE
//	REICAST_US(i);	// VRAM_SIZE
//	REICAST_US(i);	// RAM_MASK
//	REICAST_US(i);	// ARAM_MASK
//	REICAST_US(i);	// VRAM_MASK


	REICAST_US(naomi_updates);
	REICAST_US(i); // BoardID
	REICAST_US(GSerialBuffer);
	REICAST_US(BSerialBuffer);
	REICAST_US(GBufPos);
	REICAST_US(BBufPos);
	REICAST_US(GState);
	REICAST_US(BState);
	REICAST_US(GOldClk);
	REICAST_US(BOldClk);
	REICAST_US(BControl);
	REICAST_US(BCmd);
	REICAST_US(BLastCmd);
	REICAST_US(GControl);
	REICAST_US(GCmd);
	REICAST_US(GLastCmd);
	REICAST_US(SerStep);
	REICAST_US(SerStep2);
	REICAST_USA(BSerial,69);
	REICAST_USA(GSerial,69);
	REICAST_US(reg_dimm_3c);
	REICAST_US(reg_dimm_40);
	REICAST_US(reg_dimm_44);
	REICAST_US(reg_dimm_48);
	REICAST_US(reg_dimm_4c);
	REICAST_US(NaomiDataRead);

	//REICAST_USA(CodeCache,CODE_SIZE) ;
	//REICAST_USA(SH4_TCB,CODE_SIZE+4096);

	REICAST_USA(RegisterWrite,sh4_reg_count);
	REICAST_USA(RegisterRead,sh4_reg_count);
	REICAST_US(fallback_blocks);
	REICAST_US(total_blocks);
	REICAST_US(REMOVED_OPS);

	REICAST_USA(kcode,4);
	REICAST_USA(rt,4);
	REICAST_USA(lt,4);
	REICAST_USA(vks,4);
	REICAST_USA(joyx,4);
	REICAST_USA(joyy,4);


	REICAST_US(settings.dreamcast.broadcast);
	REICAST_US(settings.dreamcast.cable);
	REICAST_US(settings.dreamcast.region);

	if (CurrentCartridge != NULL)
		CurrentCartridge->Unserialize(data, total_size);

	return true ;
}
#endif

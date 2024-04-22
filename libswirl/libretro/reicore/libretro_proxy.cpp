/*
	This file is part of libswirl
*/
#include "license/bsd"

#include "types.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "rend/gles/glcache.h"
#include "libretro-common/include/libretro.h"
#include "utils/glwrap/gl3w.h"
#include "gui/gui.h"
#include "gui/gui_renderer.h"
#include <chrono>
#include <libswirl/gui/gui.h>
#include <libswirl/libswirl.h>
#include <libswirl/input/gamepad.h>
#include <libswirl/input/gamepad_device.h>
#include <utils/bit_utils.hpp>
#include <utils/string_utils.hpp>
#include "version.h"

using namespace bit_utils;
using namespace string_utils;

#if HOST_OS == OS_WINDOWS
#include <Windows.h>
#endif

#define RARCH_GL_FRAMEBUFFER GL_FRAMEBUFFER
#define RARCH_GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE
#define RARCH_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0

extern "C" {
#ifdef _WIN32
#if BUILD_RETROARCH_CORE==1
#define LIBRETRO_PROXY_STUB_TYPE RETRO_API
#define trace_plugin(__s__) //MessageBoxA(nullptr,__s__,"Trace",MB_OK)
#else
#define LIBRETRO_PROXY_STUB_TYPE
#define trace_plugin(__s__)  //printf("RAW_TRACE:%s\n",__s__)
#endif
#else
#define LIBRETRO_PROXY_STUB_TYPE
#define trace_plugin(__s__) //printf("RAW_TRACE:%s\n",__s__)
#endif
}

//Private bridge module contexts
struct hw_libretro_accel_ctx_t {
    const char* alias;
    retro_hw_context_type val;
};

struct spg_mapping_info_t {
    const char* str;
    double fps;
};

static hw_libretro_accel_ctx_t k_hw_accel_contexts[] = {
    {"HW_CTX_GL",RETRO_HW_CONTEXT_OPENGL},
    {"HW_CTX_GL_CORE",RETRO_HW_CONTEXT_OPENGL_CORE},
    {"HW_CTX_GL_ES2",RETRO_HW_CONTEXT_OPENGLES2},
    {"HW_CTX_GL_ES3",RETRO_HW_CONTEXT_OPENGLES3},
    {"HW_CTX_GL_ES",RETRO_HW_CONTEXT_OPENGLES_VERSION},
    {"HW_CTX_D3D",RETRO_HW_CONTEXT_DIRECT3D},
    {"HW_CTX_GL_FORCE_ANY",RETRO_HW_CONTEXT_DUMMY}
};

static const struct retro_controller_description k_ctl_ports_default[] = {
      { "Keyboard",		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)  },
      { "Controller",	RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)  },
      { "Controller",	RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)  },
      { "Controller",	RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 2)  },
      { "Controller",	RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 3)  },
      { "Mouse",		RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE ,1)  },
      { "Light Gun",	RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)  },
      { nullptr },
};

static const size_t k_ctl_port_count = (sizeof(k_ctl_ports_default) / sizeof(k_ctl_ports_default[0])) - 1U; // Minus the null

static const struct retro_controller_info k_static_ctl_ports[] = {  
        { k_ctl_ports_default,  k_ctl_port_count },
        { nullptr,0}
};

static struct retro_variable config_options_variables[] = {
      {
         "libreicast_resolution",
         "Internal resolution; 640x480|720x576|800x600|960x720|1024x768|1024x1024|1280x720|1280x960|1600x1200|1920x1080|1920x1440|1920x1600|2048x2048",
      },
      { "libreicast_multisample", "Multisampling; 1x|2x|4x" },
      { "libreicast_aspectratio", "Aspect Ratio; 4:3|16:9" },
      { "libreicast_mouse_enabled", "Mouse enabled; true|false" },
      { "libreicast_pixel_clock","Pixel Clock; VGA|NTSC|PAL"},
      { nullptr, nullptr },
};

static const spg_mapping_info_t k_spg_mapping_info[] = {
    {"VGA:60fps",60.0},
    {"NTSC:59.94fps",59.94},
    {"PAL:50fps",50.0},
    {"Undocumented:50fps",50.0}
};
 
static const size_t k_libretro_hw_accellerated_count = sizeof(k_hw_accel_contexts) / sizeof(k_hw_accel_contexts[0]);
static const size_t k_spg_num_mappings = sizeof(k_spg_mapping_info) / sizeof(k_spg_mapping_info[0]);
static uint32_t input_states[k_ctl_port_count] = {0}; 

//Private bridge module variables
static retro_log_printf_t log_cb;
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
retro_audio_sample_batch_t audio_batch_cb = nullptr;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static struct retro_rumble_interface rumble;
static struct retro_log_callback logging;
static bool g_b_init_done = false;
static struct retro_hw_render_callback hw_render;
static cResetEvent retro_run_alive;
std::atomic<bool> monitor_thread_running;
std::atomic<bool> monitor_thread_stopping;

#if BUILD_RETROARCH_CORE==1
extern void retro_reicast_entry_point();
#endif

static int g_previous_bound_rfb = -99;
//extern int g_gl_bound_render_frame_buffer;
extern int screen_width;
extern int screen_height;
extern u16 kcode[4];
extern u32 vks[4];
extern s8 joyx[4], joyy[4];
extern u8 rt[4], lt[4];
static bool mouse_enabled = false;
static bool retro_init_gl_hw_ctx();
static void update_vars() ;
  

bool os_gl_init(void* hwnd, void* hdc) { return true; }
bool os_gl_swap() { return true; }
void os_DoEvents() {

}
//Private bridge module functions
static void __emu_log_provider(enum retro_log_level level, const char* fmt, ...) {
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static inline void display_popup_msg(const char* text,const int32_t frame_cnt) {
   	struct retro_message msg;
				
	msg.msg = text ;
	msg.frames = 200;
	environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

static inline void display_popup_msg(const std::string& s,const int32_t frame_cnt) {
    display_popup_msg(s.c_str(),frame_cnt);
}

static inline double get_spg_fps_value()   {
    return k_spg_mapping_info[ (SPG_CONTROL.full >> 6U) & 3U ].fps;
}

static inline const char* get_spg_fps_mode_string()  {
    return k_spg_mapping_info[ (SPG_CONTROL.full >> 6U) & 3U ].str;
}

static inline const int get_spg_to_libretro_mode() {
    const int64_t v = (int64_t)get_spg_fps_value() ;
    return (v == 60) ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

static cThread monitor_thread([](void*) { 
    cResetEvent stop_finished;
    
    verify(sh4_cpu->IsRunning());

    retro_run_alive.Reset();

    for (;;) {
        if (!retro_run_alive.Wait(100)) {
            printf("retro_pause detected\n");
            break;
        }
    }

    monitor_thread_stopping = true;
    virtualDreamcast->Stop([&]() {
        stop_finished.Set();
    });

    while (!stop_finished.Wait(10)) {
        g_GUIRenderer->UIFrame();
    }

    monitor_thread_stopping = false;
    monitor_thread_running = false;
}, nullptr);


static inline const std::string get_var_data(const char* key) {
    struct retro_variable var = { .key = key,nullptr};

    if ( ! (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE,&var) )  )
        return "";
    else if (!var.value)
        return "";

    return std::string(var.value);
}

static void update_vars() {
    bool updated = false;

    environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) ;
    if ( !updated)
        return;

    display_popup_msg("Config UPDATED.Restart emu!",200);
 
    const std::string val = get_var_data("libreicast_resolution");
    if (!val.empty()) {
        std::vector<int> res;

        if (split_string(val,"x",res,true)) {
            if (res.size() == 2) {
                screen_width = res[0];
                screen_height = res[1];

                display_popup_msg("Display set: " + std::to_string(screen_width) + "x" + std::to_string(screen_height),200);
            }
        } else {
            display_popup_msg("Wrong input : " + val,200);
        }
   }
}

LIBRETRO_PROXY_STUB_TYPE void  retro_init(void) {
    trace_plugin("retro_init");

    screen_width = 640  ;
    screen_height = 480 ;  
    if (!g_b_init_done) {
        //TODO
        g_b_init_done = true;
    }
}

LIBRETRO_PROXY_STUB_TYPE void  retro_deinit(void) {
    trace_plugin("retro_deinit");
}

LIBRETRO_PROXY_STUB_TYPE unsigned  retro_api_version(void) {
    trace_plugin("retro_api_version");
    return RETRO_API_VERSION;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_controller_port_device(unsigned port, unsigned device) {
    trace_plugin("retro_set_controller_port_device");
}

LIBRETRO_PROXY_STUB_TYPE void  retro_get_system_info(struct retro_system_info* info) {
    trace_plugin("retro_get_system_info");

    memset(info, 0, sizeof(*info));
    info->library_name = "Reicast";
    info->library_version =  REICAST_VERSION ;
    info->need_fullpath = true;
    info->block_extract = true;
    info->valid_extensions =   "bin|gdi|chd|cue|cdi";
}

LIBRETRO_PROXY_STUB_TYPE void  retro_get_system_av_info(struct retro_system_av_info* info) {
    trace_plugin("retro_get_system_av_info");
 
    const int gt = (screen_width > screen_height) ? screen_width : screen_height;
    info->timing.fps = get_spg_fps_value(); 
    info->timing.sample_rate = 44100;
    info->geometry.base_height = screen_width,
    info->geometry.base_height = screen_height,
    info->geometry.max_width = gt,
    info->geometry.max_height = gt,
    info->geometry.aspect_ratio =   4.0 / 3.0;  //0 == auto
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_environment(retro_environment_t cb) {
    trace_plugin("retro_set_environment");

    environ_cb = cb;

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = __emu_log_provider;

    bool save_state_in_background = false ;
    bool clear_all_threads_wait_cb = true;
    bool no_rom = false;
    unsigned int poll_type_early      = 1; /* POLL_TYPE_EARLY */
        
    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, config_options_variables);
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)k_static_ctl_ports);
	environ_cb(2 | 0x800000, &save_state_in_background);
    //environ_cb(3 | 0x800000, &clear_all_threads_wait_cb);
    environ_cb(4 | 0x800000, &poll_type_early);
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_audio_sample(retro_audio_sample_t cb) {
    trace_plugin("retro_set_audio_sample");
    audio_cb = cb;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    trace_plugin("retro_set_audio_sample_batch");
    audio_batch_cb = cb;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_input_poll(retro_input_poll_t cb) {
    trace_plugin("retro_set_input_poll");   
    input_poll_cb = cb;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_input_state(retro_input_state_t cb) {
    trace_plugin("retro_set_input_state");
    input_state_cb = cb;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_set_video_refresh(retro_video_refresh_t cb) {
    trace_plugin("retro_set_video_refresh");
    video_cb = cb;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_reset(void) {
    trace_plugin("retro_reset");
}

LIBRETRO_PROXY_STUB_TYPE void  retro_run(void) {
    trace_plugin("retro_run");
    if (!g_b_init_done)
        return;

    while(monitor_thread_stopping) {}

    if (sh4_cpu) {
        if (!sh4_cpu->IsRunning()) {
            // start monitoring thread
            monitor_thread.WaitToEnd();
            verify(!monitor_thread_running);
            virtualDreamcast->Resume();
            monitor_thread_running = true;
            monitor_thread.Start();
        } else if (!monitor_thread_running) {
            monitor_thread_running = true;
            monitor_thread.Start();
        }
    }

    retro_run_alive.Set();

    input_poll_cb();

    s32 analog_x = 0,analog_y =0;
    s16 s0,s1;

    if (!mouse_enabled) {
        s0 = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
        s1 = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) ;
    } else {
        s0 = input_state_cb(0, RETRO_DEVICE_MOUSE, RETRO_DEVICE_ID_MOUSE_X, RETRO_DEVICE_ID_ANALOG_X);
        s1 = input_state_cb(0, RETRO_DEVICE_MOUSE, RETRO_DEVICE_ID_MOUSE_Y, RETRO_DEVICE_ID_ANALOG_Y) ;
    }

    if (s0 != 0)
        analog_x = (s32)((128.0f * (float)s0)  / 32767.0f);

    if (s1 != 0)
        analog_y = (s32)(128.0f * (float)s1  / 32767.0f);

    kcode[0]  = (uint16_t)-1U;
    rt[0] = (s8)analog_x;
    lt[0] = (s8)analog_y;
    
    bit_msk_set<u16>(kcode[0],(u16)DC_BTN_START, !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_BTN_A, !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A ));
    bit_msk_set<u16>(kcode[0],(u16)DC_BTN_B , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_BTN_X , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_BTN_Y , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)   );
    bit_msk_set<u16>(kcode[0],(u16)DC_DPAD_UP , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_DPAD_LEFT, !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_DPAD_RIGHT, !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_DPAD_DOWN , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ) ;
    bit_msk_set<u16>(kcode[0],(u16)DC_AXIS_LT, !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L)  );
    bit_msk_set<u16>(kcode[0],(u16)DC_AXIS_RT , !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)  );
 
    update_vars() ;

    gl.screen_fb = hw_render.get_current_framebuffer();
    glBindFramebuffer(RARCH_GL_FRAMEBUFFER,  hw_render.get_current_framebuffer());

    glcache.Reset();
    g_GUIRenderer->UIFrame();

    video_cb(RETRO_HW_FRAME_BUFFER_VALID, screen_width, screen_height, 0);
}


static void context_reset(void) {
    trace_plugin("GOT Context");

    extern int gl3wInit2(GL3WGetProcAddressProc proc);
    int r = gl3wInit2(hw_render.get_proc_address);    
    GLint major,minor;

    glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
    hw_render.version_major = (unsigned int)major;
    hw_render.version_minor = (unsigned int)minor;
    g_GUIRenderer.reset(GUIRenderer::Create(g_GUI.get()));
    g_GUIRenderer->CreateContext();
    g_GUIRenderer->Start();
    g_previous_bound_rfb = -9999;
}

static void context_destroy(void) {    
    trace_plugin("DROPPED Context");
    if (virtualDreamcast && sh4_cpu->IsRunning()) {
        virtualDreamcast->Stop([] { g_GUIRenderer->Stop();  });
    } else {
        g_GUIRenderer->Stop();
    }
}

static bool retro_init_gl_hw_ctx() {
    trace_plugin("init GL CTX???");
     
    for (size_t i = 0; i < k_libretro_hw_accellerated_count; ++i) {
        hw_render.context_type = k_hw_accel_contexts[i].val;// RETRO_HW_CONTEXT_OPENGLES2; //force gles2 for now
        //hw_render.version_major
        hw_render.context_reset = context_reset;
        hw_render.context_destroy = context_destroy;
        hw_render.depth = true;
        hw_render.bottom_left_origin = true;
        hw_render.depth = true;
        hw_render.stencil = true;
        hw_render.bottom_left_origin = true;
        hw_render.cache_context      = false;
        if (environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render)) {
            trace_plugin("SET HW RENDER:");
            trace_plugin(k_hw_accel_contexts[i].alias);
            return true;
        }
     }
    return false;
}

LIBRETRO_PROXY_STUB_TYPE bool  retro_load_game(const struct retro_game_info* game) {
    trace_plugin("retro_load_game");

    enum retro_pixel_format fmt = retro_pixel_format::RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
        trace_plugin("XRGB8888 is not supported.\n");
    else 
        trace_plugin("XRGB8888 is supported.\n");

    g_previous_bound_rfb = -999;
    //TODO
    if (retro_init_gl_hw_ctx())
        trace_plugin("SUCCEED retro_init_gl_hw_ctx");
	else 
        trace_plugin("FAILED retro_init_gl_hw_ctx");

    extern int libretro_prologue(int argc, wchar* argv[]) ;
    
    wchar* e[] = {"",(char*)game->path};
    libretro_prologue(2,e);

    display_popup_msg("Starting...",150);

    return true;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_unload_game(void) {
    trace_plugin("retro_unload_game");
    //printf("UNLOADING\n");
    if (g_b_init_done) {
        // TODO
        g_b_init_done = false;
        SleepMs(1500);
    }
    audio_batch_cb = nullptr;

    extern void libretro_epilogue() ;
    libretro_epilogue() ;
}

LIBRETRO_PROXY_STUB_TYPE unsigned  retro_get_region(void) {
    trace_plugin("retro_get_region");
    return RETRO_REGION_NTSC;
}

LIBRETRO_PROXY_STUB_TYPE bool  retro_load_game_special(unsigned type, const struct retro_game_info* info, size_t num) {
    trace_plugin("retro_load_game_special");
 

    return false;
}

LIBRETRO_PROXY_STUB_TYPE size_t  retro_serialize_size(void) {
    trace_plugin("retro_serialize_size");
    unsigned int total_size = 0;
    void *data = nullptr;
 
    dc_serialize(&data, &total_size) ;

    printf("Serialize set block sz %u\n",total_size);
   return total_size;
}


LIBRETRO_PROXY_STUB_TYPE bool  retro_serialize(void* data_, size_t size) {
    trace_plugin("retro_serialize");
    
    unsigned int total_size = 0;

    // wait for cpu to stop
    while(sh4_cpu->IsRunning())
        ;

    bool res = dc_serialize(&data_, &total_size) ;
   
    return res;
}

LIBRETRO_PROXY_STUB_TYPE bool   retro_unserialize(const void* data_, size_t size) {
    trace_plugin("retro_unserialize");
    unsigned int total_size = 0;
    void* pa = (void*)(data_);
 

    // wait for cpu to stop
    while(sh4_cpu->IsRunning())
        ;

    bool res = dc_unserialize(&pa, &total_size) ;

    return res;
}


LIBRETRO_PROXY_STUB_TYPE void*  retro_get_memory_data(unsigned id) {
    trace_plugin("retro_get_memory_data");
    return NULL;
}

LIBRETRO_PROXY_STUB_TYPE size_t  retro_get_memory_size(unsigned id) {
    trace_plugin("retro_get_memory_size");
    return 0;
}

LIBRETRO_PROXY_STUB_TYPE void  retro_cheat_reset(void) {
    trace_plugin("retro_cheat_reset");
}

LIBRETRO_PROXY_STUB_TYPE void  retro_cheat_set(unsigned index, bool enabled, const char* code) {
    trace_plugin("retro_cheat_set");
}


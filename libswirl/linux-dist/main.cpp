/*
	This file is part of libswirl
*/
#include "license/bsd"


#include "types.h"
#include "cfg/cfg.h"

#if HOST_OS==OS_LINUX
#include <poll.h>
#include <termios.h>
//#include <curses.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "hw/sh4/dyna/blockmanager.h"
#include "hw/maple/maple_cfg.h"
#include <unistd.h>
#include <pty.h>

#include "libswirl.h"
#include "hw/pvr/Renderer_if.h"

#if defined(TARGET_EMSCRIPTEN)
	#include "emscripten.h"
#endif

#if defined(SUPPORT_DISPMANX)
	#include "linux-dist/dispmanx.h"
#endif

#if defined(SUPPORT_X11)
	#include "linux-dist/x11.h"
#endif

#if defined(USE_SDL)
	#include "sdl/sdl.h"
#endif

#if defined(USE_EVDEV)
	#include "linux-dist/evdev.h"
#endif

#if defined(USE_JOYSTICK)
	#include "linux-dist/joystick.h"
#endif

#ifdef TARGET_PANDORA
	#include <signal.h>
	#include <execinfo.h>
	#include <sys/soundcard.h>
#endif

#if FEAT_HAS_NIXPROF
#include "profiler/profiler.h"
#endif

#if defined(SUPPORT_GLX)
#include "utils/glinit/glx/glx.h"
#endif

#if defined(SUPPORT_EGL)
#include "utils/glinit/egl/egl.h"
#endif

#if defined(SUPPORT_SDL)
#include "utils/glinit/sdl/sdl.h"
#endif

void* x11_win = 0;
void* x11_disp = 0;


void* sdl_win = 0;
void* sdl_disp = 0;

void* libPvr_GetRenderTarget()
{
	return x11_win;
}

void* libPvr_GetRenderSurface()
{
	return x11_disp;
}

u16 kcode[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
u8 rt[4] = {0, 0, 0, 0};
u8 lt[4] = {0, 0, 0, 0};
u32 vks[4];
s8 joyx[4], joyy[4];

void emit_WriteCodeCache();

#if defined(USE_JOYSTICK)
	/* legacy joystick input */
	static int joystick_fd = -1; // Joystick file descriptor
#endif

void os_SetupInput()
{
#if defined(USE_EVDEV)
	input_evdev_init();
#endif

#if defined(USE_JOYSTICK)
	int joystick_device_id = cfgLoadInt("input", "joystick_device_id", JOYSTICK_DEFAULT_DEVICE_ID);
	if (joystick_device_id < 0) {
		puts("Legacy Joystick input disabled by config.\n");
	}
	else
	{
		int joystick_device_length = snprintf(NULL, 0, JOYSTICK_DEVICE_STRING, joystick_device_id);
		char* joystick_device = (char*)malloc(joystick_device_length + 1);
		sprintf(joystick_device, JOYSTICK_DEVICE_STRING, joystick_device_id);
		joystick_fd = input_joystick_init(joystick_device);
		free(joystick_device);
	}
#endif

#if defined(SUPPORT_X11)
	input_x11_init();
#endif

#if defined(USE_SDL)
	input_sdl_init();
#endif

#if defined(TARGET_EMSCRIPTEN)
	input_emscripten_init();
#endif
}

void UpdateInputState(u32 port)
{
	#if defined(TARGET_EMSCRIPTEN)
		return;
	#endif

	#if defined(USE_JOYSTICK)
		input_joystick_handle(joystick_fd, port);
	#endif

	#if defined(USE_EVDEV)
		input_evdev_handle(port);
	#endif

	#if defined(USE_SDL)
		input_sdl_handle(port);
	#endif
}

#if BUILD_RETROARCH_CORE==0
void os_DoEvents()
{
	#if defined(SUPPORT_X11)
		input_x11_handle();
		event_x11_handle();
	#endif
}
#endif

void os_SetWindowText(const char * text)
{
	#if defined(SUPPORT_X11)
		x11_window_set_text(text);
	#elif defined(USE_SDL)
		sdl_window_set_text(text);
	#else
		printf("Status: %s\n", text);
	#endif
}

void os_LaunchFromURL(const string& url)
{
	auto cmd = "xdg-open " + url; 
	auto rv = system(cmd.c_str());
}

void os_CreateWindow()
{
	#if defined(SUPPORT_DISPMANX)
		dispmanx_window_create();
	#endif
	#if defined(SUPPORT_X11)
		x11_window_create();
	#endif
	#if defined(USE_SDL)
		sdl_window_create(&sdl_win, &sdl_disp);
	#endif
}

#ifdef TARGET_PANDORA

	void clean_exit(int sig_num)
	{
		void* array[10];
		size_t size;

		if (joystick_fd >= 0) { close(joystick_fd); }
		for (int port = 0; port < 4 ; port++)
		{
			if (evdev_controllers[port]->fd >= 0)
			{
				close(evdev_controllers[port]->fd);
			}
		}

		// Close EGL context ???
		if (sig_num!=0)
		{
			os_gl_term();
		}

		x11_window_destroy():

		// finish cleaning
		if (sig_num!=0)
		{
			write(2, "\nSignal received\n", sizeof("\nSignal received\n"));

			size = backtrace(array, 10);
			backtrace_symbols_fd(array, size, STDERR_FILENO);
			exit(1);
		}
	}
#endif

string find_user_config_dir()
{
	struct stat info;
	string home = "";
	if(getenv("HOME") != NULL)
	{
		// Support for the legacy config dir at "$HOME/.reicast"
		string legacy_home = (string)getenv("HOME") + "/.reicast";
		if((stat(legacy_home.c_str(), &info) == 0) && (info.st_mode & S_IFDIR))
		{
			// "$HOME/.reicast" already exists, let's use it!
			return legacy_home;
		}

		/* If $XDG_CONFIG_HOME is not set, we're supposed to use "$HOME/.config" instead.
		 * Consult the XDG Base Directory Specification for details:
		 *   http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables
		 */
		home = (string)getenv("HOME") + "/.config/reicast";
	}
	if(getenv("XDG_CONFIG_HOME") != NULL)
	{
		// If XDG_CONFIG_HOME is set explicitly, we'll use that instead of $HOME/.config
		home = (string)getenv("XDG_CONFIG_HOME") + "/reicast";
	}

	if(!home.empty())
	{
		if((stat(home.c_str(), &info) != 0) || !(info.st_mode & S_IFDIR))
		{
			// If the directory doesn't exist yet, create it!
			mkdir(home.c_str(), 0755);
		}
		return home;
	}

	// Unable to detect config dir, use the current folder
	return ".";
}

string find_user_data_dir()
{
	struct stat info;
	string data = "";
	if(getenv("HOME") != NULL)
	{
		// Support for the legacy config dir at "$HOME/.reicast"
		string legacy_data = (string)getenv("HOME") + "/.reicast";
		if((stat(legacy_data.c_str(), &info) == 0) && (info.st_mode & S_IFDIR))
		{
			// "$HOME/.reicast" already exists, let's use it!
			return legacy_data;
		}

		/* If $XDG_DATA_HOME is not set, we're supposed to use "$HOME/.local/share" instead.
		 * Consult the XDG Base Directory Specification for details:
		 *   http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables
		 */
		data = (string)getenv("HOME") + "/.local/share/reicast";
	}
	if(getenv("XDG_DATA_HOME") != NULL)
	{
		// If XDG_DATA_HOME is set explicitly, we'll use that instead of $HOME/.config
		data = (string)getenv("XDG_DATA_HOME") + "/reicast";
	}

	if(!data.empty())
	{
		if((stat(data.c_str(), &info) != 0) || !(info.st_mode & S_IFDIR))
		{
			// If the directory doesn't exist yet, create it!
			mkdir(data.c_str(), 0755);
		}
		return data;
	}

	// Unable to detect config dir, use the current folder
	return ".";
}

std::vector<string> find_system_config_dirs()
{
	std::vector<string> dirs;
	if (getenv("XDG_CONFIG_DIRS") != NULL)
	{
		string s = (string)getenv("XDG_CONFIG_DIRS");

		string::size_type pos = 0;
		string::size_type n = s.find(":", pos);
		while(n != std::string::npos)
		{
			dirs.push_back(s.substr(pos, n-pos) + "/reicast");
			pos = n + 1;
			n = s.find(":", pos);
		}
		// Separator not found
		dirs.push_back(s.substr(pos) + "/reicast");
	}
	else
	{
		dirs.push_back("/etc/reicast"); // This isn't part of the XDG spec, but much more common than /etc/xdg/
		dirs.push_back("/etc/xdg/reicast");
	}
	return dirs;
}

std::vector<string> find_system_data_dirs()
{
	std::vector<string> dirs;
	if (getenv("XDG_DATA_DIRS") != NULL)
	{
		string s = (string)getenv("XDG_DATA_DIRS");

		string::size_type pos = 0;
		string::size_type n = s.find(":", pos);
		while(n != std::string::npos)
		{
			dirs.push_back(s.substr(pos, n-pos) + "/reicast");
			pos = n + 1;
			n = s.find(":", pos);
		}
		// Separator not found
		dirs.push_back(s.substr(pos) + "/reicast");
	}
	else
	{
		dirs.push_back("/usr/local/share/reicast");
		dirs.push_back("/usr/share/reicast");
	}
	return dirs;
}

void common_linux_setup();
#if FEAT_HAS_SERIAL_TTY
bool common_serial_pty_setup();
#endif

#if BUILD_RETROARCH_CORE==1

int libretro_prologue(int argc, char* argv[])
{
	#ifdef TARGET_PANDORA
		signal(SIGSEGV, clean_exit);
		signal(SIGKILL, clean_exit);
	#endif

	/* Set directories */
	set_user_config_dir(find_user_config_dir());
	set_user_data_dir(find_user_data_dir());
	std::vector<string> dirs;
	dirs = find_system_config_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	dirs = find_system_data_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	add_system_data_dir(find_user_data_dir());
 

	common_linux_setup();

	settings.profile.run_counts=0;

	if (reicast_init(argc, argv))
		die("Reicast initialization failed\n");
    
    #if FEAT_HAS_SERIAL_TTY
    //bool cleanup_pty_symlink = common_serial_pty_setup();
    #endif

	#if FEAT_HAS_NIXPROF
	install_prof_handler(1);
	#endif

	return 0;
}

void libretro_body()
{

}

void libretro_epilogue()
{
	reicast_term();
}
#else
bool os_gl_init(void* hwnd, void* hdc)
{
	#if defined(SUPPORT_GLX)
		return glx_Init(x11_win, x11_disp);
	#elif defined(SUPPORT_EGL)
		return egl_Init(x11_win, x11_disp);
	#elif defined(SUPPORT_SDL)
		return sdlgl_Init(sdl_win, sdl_win);
	#else
		#error "only glx/egl/sdl supported"
		return true;
	#endif
}

bool os_gl_swap()
{
	#if defined(SUPPORT_GLX)
		return glx_Swap();
	#elif defined(SUPPORT_EGL)
		return egl_Swap();
	#elif defined(SUPPORT_SDL)
		return sdlgl_Swap();
	#else
		#error "only glx/egl/sdl supported"
		return true;
	#endif
}

void os_gl_term()
{
	#if defined(SUPPORT_GLX)
		glx_Term();
	#elif defined(SUPPORT_EGL)
		egl_Term();
	#elif defined(SUPPORT_SDL)
		return sdlgl_Term();
	#else
		#error "only glx/egl/sdl supported"
	#endif
}

int main(int argc, wchar* argv[])
{


	#ifdef TARGET_PANDORA
		signal(SIGSEGV, clean_exit);
		signal(SIGKILL, clean_exit);
	#endif

	/* Set directories */
	set_user_config_dir(find_user_config_dir());
	set_user_data_dir(find_user_data_dir());
	std::vector<string> dirs;
	dirs = find_system_config_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	dirs = find_system_data_dirs();
	for(unsigned int i = 0; i < dirs.size(); i++)
	{
		add_system_data_dir(dirs[i]);
	}
	add_system_data_dir(find_user_data_dir());

	#if defined(USE_SDL)
		if (SDL_Init(0) != 0)
		{
			die("SDL: Initialization failed!");
		}
	#endif

	common_linux_setup();

	settings.profile.run_counts=0;

	if (reicast_init(argc, argv))
		die("Reicast initialization failed\n");
    
    #if FEAT_HAS_SERIAL_TTY
    bool cleanup_pty_symlink = common_serial_pty_setup();
    #endif

	#if FEAT_HAS_NIXPROF
	install_prof_handler(1);
	#endif
	
	reicast_ui_loop();
#if FEAT_HAS_SERIAL_TTY
	if (cleanup_pty_symlink)
	{
		unlink(settings.debug.VirtualSerialPortFile.c_str());
	}
#endif
	reicast_term();

	#ifdef TARGET_PANDORA
		clean_exit(0);
	#endif

	#if defined(USE_EVDEV)
		input_evdev_close();
	#endif

	#if defined(SUPPORT_X11)
		x11_window_destroy();
	#endif

	return 0;
}
#endif
#endif
 
int get_mic_data(u8* buffer) { return 0; }

void os_DebugBreak()
{
	#if !defined(TARGET_EMSCRIPTEN)
		raise(SIGTRAP);
	#else
		printf("DEBUGBREAK!\n");
		exit(-1);
	#endif
}




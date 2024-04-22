/*
	This file is part of libswirl
*/
#include "license/bsd"



#if defined(USE_SDL)
#include <map>
#include "types.h"
#include "cfg/cfg.h"
#include "linux-dist/main.h"
#include "sdl/sdl.h"
#include "gui/gui.h"
#include "gui/gui_renderer.h"

// for screenDPI
#include "deps/imgui/imgui.h"
#include "gui/gui_partials.h"

#include "libswirl.h"
#include "utils/glinit/sdl/sdl.h"

#endif
#include "hw/maple/maple_devs.h"
#include "sdl_gamepad.h"
#include "sdl_keyboard.h"

static SDL_Window* window = NULL;

#ifdef TARGET_PANDORA
	#define WINDOW_WIDTH  800
#else
	#define WINDOW_WIDTH  640
#endif
#define WINDOW_HEIGHT  480

static std::shared_ptr<SDLMouseGamepadDevice> sdl_mouse_gamepad;
static std::shared_ptr<SDLKbGamepadDevice> sdl_kb_gamepad;
static SDLKeyboardDevice* sdl_keyboard = NULL;

#ifdef TARGET_PANDORA
	extern char OSD_Info[128];
	extern int OSD_Delay;
	extern char OSD_Counters[256];
	extern int OSD_Counter;
#endif

extern u32 mo_buttons;
extern s32 mo_x_abs;
extern s32 mo_y_abs;
extern f32 mo_x_delta;
extern f32 mo_y_delta;
extern f32 mo_wheel_delta;

extern int screen_width, screen_height;

static void sdl_open_joystick(int index)
{
	SDL_Joystick *pJoystick = SDL_JoystickOpen(index);

	if (pJoystick == NULL)
	{
		printf("SDL: Cannot open joystick %d\n", index + 1);
		return;
	}
	std::shared_ptr<SDLGamepadDevice> gamepad = std::make_shared<SDLGamepadDevice>(index < MAPLE_PORTS ? index : -1, pJoystick);
	SDLGamepadDevice::AddSDLGamepad(gamepad);
}

static void sdl_close_joystick(SDL_JoystickID instance)
{
	std::shared_ptr<SDLGamepadDevice> gamepad = SDLGamepadDevice::GetSDLGamepad(instance);
	if (gamepad != NULL)
		gamepad->close();
}

void input_sdl_init()
{
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			die("SDL: error initializing Joystick subsystem");
		}
	}
	if (SDL_WasInit(SDL_INIT_HAPTIC) == 0)
		SDL_InitSubSystem(SDL_INIT_HAPTIC);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	sdl_keyboard = new SDLKeyboardDevice(0);
	sdl_kb_gamepad = std::make_shared<SDLKbGamepadDevice>(0);
	GamepadDevice::Register(sdl_kb_gamepad);
	sdl_mouse_gamepad = std::make_shared<SDLMouseGamepadDevice>(0);
	GamepadDevice::Register(sdl_mouse_gamepad);
}

static int mouse_prev_x = -1;
static int mouse_prev_y = -1;

static void set_mouse_position(int x, int y)
{
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	if (width != 0 && height != 0)
	{
		float scale = 480.f / height;
		mo_x_abs = (x - (width - 640.f / scale) / 2.f) * scale;
		mo_y_abs = y * scale;
		if (mouse_prev_x != -1)
		{
			mo_x_delta += (f32)(x - mouse_prev_x) * settings.input.MouseSensitivity / 100.f;
			mo_y_delta += (f32)(y - mouse_prev_y) * settings.input.MouseSensitivity / 100.f;
		}
		mouse_prev_x = x;
		mouse_prev_y = y;
	}
}

// FIXME this shouldn't be done by port. Need something like: handle_events() then get_port(0), get_port(2), ...
void input_sdl_handle(u32 port)
{
	if (port == 0)	// FIXME hack
		SDLGamepadDevice::UpdateRumble();

	#define SET_FLAG(field, mask, expr) field =((expr) ? (field & ~mask) : (field | mask))
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				if (virtualDreamcast && sh4_cpu->IsRunning()) {
					virtualDreamcast->Stop([] { 
						g_GUIRenderer->Stop();
					});
				}
				else
				{
					g_GUIRenderer->Stop();
				}
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				sdl_kb_gamepad->gamepad_btn_input(event.key.keysym.sym, event.type == SDL_KEYDOWN);
				{
					int modifier_keys = 0;
					if (event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
						SET_FLAG(modifier_keys, (0x02 | 0x20), event.type == SDL_KEYUP);
					if (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
						SET_FLAG(modifier_keys, (0x01 | 0x10), event.type == SDL_KEYUP);
					sdl_keyboard->keyboard_input(event.key.keysym.sym, event.type == SDL_KEYDOWN, modifier_keys);
				}
				break;
			case SDL_TEXTINPUT:
				for (int i = 0; event.text.text[i] != '\0'; i++)
					sdl_keyboard->keyboard_character(event.text.text[i]);
				break;

			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				{
					std::shared_ptr<SDLGamepadDevice> device = SDLGamepadDevice::GetSDLGamepad((SDL_JoystickID)event.jbutton.which);
					if (device != NULL)
						device->gamepad_btn_input(event.jbutton.button, event.type == SDL_JOYBUTTONDOWN);
				}
				break;
			case SDL_JOYAXISMOTION:
				{
					std::shared_ptr<SDLGamepadDevice> device = SDLGamepadDevice::GetSDLGamepad((SDL_JoystickID)event.jaxis.which);
					if (device != NULL)
						device->gamepad_axis_input(event.jaxis.axis, event.jaxis.value);
				}
				break;

			case SDL_MOUSEMOTION:
				set_mouse_position(event.motion.x, event.motion.y);
				SET_FLAG(mo_buttons, 1 << 2, event.motion.state & SDL_BUTTON_LMASK);
				SET_FLAG(mo_buttons, 1 << 1, event.motion.state & SDL_BUTTON_RMASK);
				SET_FLAG(mo_buttons, 1 << 3, event.motion.state & SDL_BUTTON_MMASK);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				set_mouse_position(event.button.x, event.button.y);
				switch (event.button.button)
				{
				case SDL_BUTTON_LEFT:
					SET_FLAG(mo_buttons, 1 << 2, event.button.state == SDL_PRESSED);
					break;
				case SDL_BUTTON_RIGHT:
					SET_FLAG(mo_buttons, 1 << 1, event.button.state == SDL_PRESSED);
					break;
				case SDL_BUTTON_MIDDLE:
					SET_FLAG(mo_buttons, 1 << 3, event.button.state == SDL_PRESSED);
					break;
				}
				sdl_mouse_gamepad->gamepad_btn_input(event.button.button, event.button.state == SDL_PRESSED);
				break;

			case SDL_MOUSEWHEEL:
				mo_wheel_delta -= event.wheel.y * 35;
				break;

			case SDL_JOYDEVICEADDED:
				sdl_open_joystick(event.jdevice.which);
				break;

			case SDL_JOYDEVICEREMOVED:
				sdl_close_joystick((SDL_JoystickID)event.jdevice.which);
				break;
		}
	}
}

void sdl_window_set_text(const char* text)
{
	#ifdef TARGET_PANDORA
		strncpy(OSD_Counters, text, 256);
	#else
		if(window)
		{
			SDL_SetWindowTitle(window, text);    // *TODO*  Set Icon also...
		}
	#endif
}

bool sdl_window_create(void** wind, void** disp) {
	
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		{
			die("error initializing SDL Joystick subsystem");
		}
	}


	int window_width  = cfgLoadInt("x11","width", WINDOW_WIDTH);
	int window_height = cfgLoadInt("x11","height", WINDOW_HEIGHT);

	int flags = 0;
	#ifdef TARGET_PANDORA
		flags |= SDL_FULLSCREEN;
	#else
		flags |= SDL_SWSURFACE | SDL_WINDOW_RESIZABLE;
	#endif

	// try full GL first
	if (!sdlgl_CreateWindow(false, flags, window_width, window_height, wind, disp)) {
		if (!sdlgl_CreateWindow(true, flags, window_width, window_height, wind, disp)) {
			return false;
		}
	}

	window = reinterpret_cast<SDL_Window*>(*wind);

	SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

	float ddpi, hdpi, vdpi;
	if (!SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), &ddpi, &hdpi, &vdpi))
		screen_dpi = (int)roundf(max(hdpi, vdpi));

	printf("Created SDL Window (%ix%i) and GL Context successfully\n", window_width, window_height);
}
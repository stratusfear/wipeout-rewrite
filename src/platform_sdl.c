#include "SDL.h"
#if defined(RENDERER_VULKAN)
#include <SDL_vulkan.h>
#endif

#include "platform.h"
#include "input.h"
#include "system.h"
#include "utils.h"
#include "mem.h"

static uint64_t perf_freq = 0;
static bool wants_to_exit = false;
static SDL_Window *window;
static SDL_AudioDeviceID audio_device;
static SDL_GameController *gamepad;
static void (*audio_callback)(float *buffer, uint32_t len) = NULL;
static char *path_assets = "";
static char *path_userdata = "";
static char *temp_path = NULL;

#ifdef __APPLE__
	// macOS behaves best with desktop fullscreen (Space-based, non-exclusive).
	#define USE_FULLSCREEN_DESKTOP 1
#else
	#define USE_FULLSCREEN_DESKTOP 0
#endif


uint8_t platform_sdl_gamepad_map[] = {
	[SDL_CONTROLLER_BUTTON_A] = INPUT_GAMEPAD_A,
	[SDL_CONTROLLER_BUTTON_B] = INPUT_GAMEPAD_B,
	[SDL_CONTROLLER_BUTTON_X] = INPUT_GAMEPAD_X,
	[SDL_CONTROLLER_BUTTON_Y] = INPUT_GAMEPAD_Y,
	[SDL_CONTROLLER_BUTTON_BACK] = INPUT_GAMEPAD_SELECT,
	[SDL_CONTROLLER_BUTTON_GUIDE] = INPUT_GAMEPAD_HOME,
	[SDL_CONTROLLER_BUTTON_START] = INPUT_GAMEPAD_START,
	[SDL_CONTROLLER_BUTTON_LEFTSTICK] = INPUT_GAMEPAD_L_STICK_PRESS,
	[SDL_CONTROLLER_BUTTON_RIGHTSTICK] = INPUT_GAMEPAD_R_STICK_PRESS,
	[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = INPUT_GAMEPAD_L_SHOULDER,
	[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = INPUT_GAMEPAD_R_SHOULDER,
	[SDL_CONTROLLER_BUTTON_DPAD_UP] = INPUT_GAMEPAD_DPAD_UP,
	[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = INPUT_GAMEPAD_DPAD_DOWN,
	[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = INPUT_GAMEPAD_DPAD_LEFT,
	[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = INPUT_GAMEPAD_DPAD_RIGHT,
	[SDL_CONTROLLER_BUTTON_MAX] = INPUT_INVALID
};


uint8_t platform_sdl_axis_map[] = {
	[SDL_CONTROLLER_AXIS_LEFTX] = INPUT_GAMEPAD_L_STICK_LEFT,
	[SDL_CONTROLLER_AXIS_LEFTY] = INPUT_GAMEPAD_L_STICK_UP,
	[SDL_CONTROLLER_AXIS_RIGHTX] = INPUT_GAMEPAD_R_STICK_LEFT,
	[SDL_CONTROLLER_AXIS_RIGHTY] = INPUT_GAMEPAD_R_STICK_UP,
	[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = INPUT_GAMEPAD_L_TRIGGER,
	[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = INPUT_GAMEPAD_R_TRIGGER,
	[SDL_CONTROLLER_AXIS_MAX] = INPUT_INVALID
};


void platform_exit(void) {
	wants_to_exit = true;
}

#if defined(RENDERER_VULKAN)
void *platform_get_window_handle(void) {
	return (void *)window;
}
#endif

SDL_GameController *platform_find_gamepad(void) {
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			return SDL_GameControllerOpen(i);
		}
	}

	return NULL;
}

// ---------------------------------------------------------------------------
// Pacing state shared with the frame limiter below: the current vsync
// preference (render_set_vsync forwards here from both GL and Vulkan) and
// a cached display refresh rate. The refresh is queried once at startup
// and again on the window events that can change it (moved to another
// display, display change, fullscreen enter/leave) — not every frame.
static int vsync_enabled = 1;      // save.vsync default is 1; game boot pushes the real value
static int display_refresh_hz = 0; // 0 = unknown; the limiter's vsync skip rule then cannot apply

static void platform_refresh_display_rate(void) {
	SDL_DisplayMode mode;
	int display = SDL_GetWindowDisplayIndex(window);
	if (display >= 0 && SDL_GetCurrentDisplayMode(display, &mode) == 0 && mode.refresh_rate > 0) {
		display_refresh_hz = mode.refresh_rate;
	}
	else {
		display_refresh_hz = 0;
	}
}


void platform_pump_events(void) {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		// Detect ALT+Enter press to toggle fullscreen
		if (
			ev.type == SDL_KEYDOWN && 
			ev.key.keysym.scancode == SDL_SCANCODE_RETURN &&
			(ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
		) {
			platform_set_fullscreen(!platform_get_fullscreen());
		}

		// Input Keyboard
		else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			int code = ev.key.keysym.scancode;
			float state = ev.type == SDL_KEYDOWN ? 1.0 : 0.0;
			if (code >= SDL_SCANCODE_LCTRL && code <= SDL_SCANCODE_RALT) {
				int code_internal = code - SDL_SCANCODE_LCTRL + INPUT_KEY_LCTRL;
				input_set_button_state(code_internal, state);
			}
			else if (code > 0 && code < INPUT_KEY_MAX) {
				input_set_button_state(code, state);
			}
		}

		else if (ev.type == SDL_TEXTINPUT) {
			input_textinput(ev.text.text[0]);
		}

		// Gamepads connect/disconnect
		else if (ev.type == SDL_CONTROLLERDEVICEADDED) {
			gamepad = SDL_GameControllerOpen(ev.cdevice.which);
		}
		else if (ev.type == SDL_CONTROLLERDEVICEREMOVED) {
			if (gamepad && ev.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad))) {
				SDL_GameControllerClose(gamepad);
				gamepad = platform_find_gamepad();
			}
		}

		// Input Gamepad Buttons
		else if (
			ev.type == SDL_CONTROLLERBUTTONDOWN || 
			ev.type == SDL_CONTROLLERBUTTONUP
		) {
			if (ev.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
				button_t button = platform_sdl_gamepad_map[ev.cbutton.button];
				if (button != INPUT_INVALID) {
					float state = ev.type == SDL_CONTROLLERBUTTONDOWN ? 1.0 : 0.0;
					input_set_button_state(button, state);
				}
			}
		}

		// Input Gamepad Axis
		else if (ev.type == SDL_CONTROLLERAXISMOTION) {
			float state = (float)ev.caxis.value / 32767.0;

			if (ev.caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
				int code = platform_sdl_axis_map[ev.caxis.axis];
				if (
					code == INPUT_GAMEPAD_L_TRIGGER || 
					code == INPUT_GAMEPAD_R_TRIGGER
				) {
					input_set_button_state(code, state);
				}
				else if (state > 0) {
					input_set_button_state(code, 0.0);
					input_set_button_state(code+1, state);
				}
				else {
					input_set_button_state(code, -state);
					input_set_button_state(code+1, 0.0);
				}
			}
		}

		// Mouse buttons
		else if (
			ev.type == SDL_MOUSEBUTTONDOWN ||
			ev.type == SDL_MOUSEBUTTONUP
		) {
			button_t button = INPUT_BUTTON_NONE;
			switch (ev.button.button) {
				case SDL_BUTTON_LEFT: button = INPUT_MOUSE_LEFT; break;
				case SDL_BUTTON_MIDDLE: button = INPUT_MOUSE_MIDDLE; break;
				case SDL_BUTTON_RIGHT: button = INPUT_MOUSE_RIGHT; break;
				default: break;
			}
			if (button != INPUT_BUTTON_NONE) {
				float state = ev.type == SDL_MOUSEBUTTONDOWN ? 1.0 : 0.0;
				input_set_button_state(button, state);
			}
		}

		// Mouse wheel
		else if (ev.type == SDL_MOUSEWHEEL) {
			button_t button = ev.wheel.y > 0 
				? INPUT_MOUSE_WHEEL_UP
				: INPUT_MOUSE_WHEEL_DOWN;
			input_set_button_state(button, 1.0);
			input_set_button_state(button, 0.0);
		}

		// Mouse move
		else if (ev.type == SDL_MOUSEMOTION) {
			input_set_mouse_pos(ev.motion.x, ev.motion.y);
		}

		// Window Events
		if (ev.type == SDL_QUIT) {
			wants_to_exit = true;
		}
		else if (
			ev.type == SDL_WINDOWEVENT &&
			(
				ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
				ev.window.event == SDL_WINDOWEVENT_RESIZED
			)
		) {
			// resize (and DPI changes) can move the window to a mode with a
			// different refresh on some backends; update the cache, then resize
			platform_refresh_display_rate();
			system_resize(platform_screen_size());
		}
		// Events that can change which display the window is on, and with
		// it the cached refresh rate the frame limiter consults. (OS-level
		// fullscreen changes surface as size/display events; the game's own
		// toggle refreshes the cache inside platform_set_fullscreen.)
		else if (
			ev.type == SDL_WINDOWEVENT &&
			(
				ev.window.event == SDL_WINDOWEVENT_MOVED ||
				ev.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED
			)
		) {
			platform_refresh_display_rate();
		}
	}
}

double platform_now(void) {
	uint64_t perf_counter = SDL_GetPerformanceCounter();
	return (double)perf_counter / (double)perf_freq;
}

bool platform_get_fullscreen(void) {
	return SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
}

void platform_set_fullscreen(bool fullscreen) {
	if (fullscreen) {
		#if !USE_FULLSCREEN_DESKTOP
			int32_t display = SDL_GetWindowDisplayIndex(window);

			SDL_DisplayMode mode;
			SDL_GetDesktopDisplayMode(display, &mode);
			SDL_SetWindowDisplayMode(window, &mode);
		#endif
		SDL_SetWindowFullscreen(window, USE_FULLSCREEN_DESKTOP ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN);
		SDL_ShowCursor(SDL_DISABLE);
	}
	else {
		SDL_SetWindowFullscreen(window, 0);
		SDL_ShowCursor(SDL_ENABLE);
	}
	// The display mode (and with it the refresh rate) can change with the
	// fullscreen state; SDL does not always emit a usable window event for
	// it under every backend, so update the cache directly here as well.
	platform_refresh_display_rate();
}

void platform_set_vsync(bool enabled) {
	// Recorded for the frame limiter: with vsync on, the present path paces
	// the loop, so the limiter stands down for caps at/above the refresh
	// instead of fighting it (see platform_limit_frame).
	vsync_enabled = enabled ? 1 : 0;
	#if defined(RENDERER_GL)
		// GL pacing lives where the GL context lives (platform_video_init
		// created it). Other renderers pace in their own backend (Vulkan:
		// present mode) or not at all (software/null).
		SDL_GL_SetSwapInterval(enabled ? 1 : 0);
	#endif
	(void)enabled;
}

// ---------------------------------------------------------------------------
// Frame limiter (renderer-independent, so it caps GL/SW/VK alike).
// The menu's FRAME LIMIT maps to frame_limit_fps; 0 means "UNLIMITED" and
// installs no limiter at all. (The old behaviour — silently capping 0 at
// 0.99x the display refresh — made the menu label a lie and paced the loop
// a hair slower than the display: with vsync on, roughly every 100th
// vblank passed with no new frame, a duplicated-frame judder about every
// 1.7s at 60Hz.)
// With vsync on the present path (GL swap interval, Vulkan FIFO) paces the
// loop to the refresh rate, so an explicit cap at or above refresh cannot
// improve on it and only risks fighting it; the limiter stands down for
// those caps (1Hz tolerance). Caps clearly below refresh (30 on a 60Hz
// panel) are still enforced.
// Sleep with SDL_Delay(1) for most of the remaining frame time, then spin
// the last ~1ms: SDL_Delay granularity is ~1ms (and coarser on some
// platforms), so pure sleeping drifts. At high caps (budget under ~4ms)
// the spin tail is shortened proportionally so the wait does not become
// mostly busy-waiting.

static int frame_limit_fps = 0;

void platform_set_fps_limit(int fps) {
	frame_limit_fps = fps > 0 ? fps : 0;
}

static void platform_limit_frame(double frame_start) {
	int fps = frame_limit_fps;
	if (fps == 0) {
		return; // "UNLIMITED": raw loop speed, or whatever the present mode paces
	}
	if (vsync_enabled && display_refresh_hz > 0 && fps >= display_refresh_hz - 1) {
		return; // vsync already paces at the refresh rate; do not fight it
	}

	double budget = 1.0 / (double)fps;
	double deadline = frame_start + budget;
	double spin_tail = budget < 0.004 ? budget * 0.25 : 0.001;
	double now;
	while ((now = platform_now()) < deadline - spin_tail) {
		SDL_Delay(1);
	}
	while ((now = platform_now()) < deadline) {
		// busy-wait the last short interval for frame-time precision
	}
}

void platform_audio_callback(void* userdata, uint8_t* stream, int len) {
	if (audio_callback) {
		audio_callback((float *)stream, len/sizeof(float));
	}
	else {
		memset(stream, 0, len);
	}
}

void platform_set_audio_mix_cb(void (*cb)(float *buffer, uint32_t len)) {
	audio_callback = cb;
	SDL_PauseAudioDevice(audio_device, 0);
}


bool platform_asset_exists(const char *name) {
	char *path = strcat(strcpy(temp_path, path_assets), name);
	return file_exists(path);
}

FILE *platform_open_asset(const char *name, const char *mode) {
	char *path = strcat(strcpy(temp_path, path_assets), name);
	return fopen(path, mode);
}

uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read) {
	char *path = strcat(strcpy(temp_path, path_assets), name);
	return file_load(path, bytes_read);
}

uint8_t *platform_load_userdata(const char *name, uint32_t *bytes_read) {
	char *path = strcat(strcpy(temp_path, path_userdata), name);
	if (!file_exists(path)) {
		*bytes_read = 0;
		return NULL;
	}
	return file_load(path, bytes_read);
}

uint32_t platform_store_userdata(const char *name, void *bytes, int32_t len) {
	char *path = strcat(strcpy(temp_path, path_userdata), name);
	return file_store(path, bytes, len);
}

#if defined(RENDERER_GL) // ----------------------------------------------------
	#define PLATFORM_WINDOW_FLAGS SDL_WINDOW_OPENGL
	SDL_GLContext platform_gl;

	void platform_video_init(void) {
		#if defined(USE_GLES2)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		#endif

		platform_gl = SDL_GL_CreateContext(window);
		SDL_GL_SetSwapInterval(1);
	}

	void platform_prepare_frame(void) {
		// nothing
	}

	void platform_video_cleanup(void) {
		SDL_GL_DeleteContext(platform_gl);
	}

	void platform_end_frame(void) {
		SDL_GL_SwapWindow(window);
	}

	vec2i_t platform_screen_size(void) {
		int width, height;
		SDL_GL_GetDrawableSize(window, &width, &height);
		return vec2i(width, height);
	}


#elif defined(RENDERER_VULKAN) // ---------------------------------------------
	#define PLATFORM_WINDOW_FLAGS SDL_WINDOW_VULKAN

	void platform_video_init(void) {
		// nothing
	}

	void platform_prepare_frame(void) {
		// nothing
	}

	void platform_video_cleanup(void) {
		// nothing
	}

	void platform_end_frame(void) {
		// Present handled by render_frame_end() via vkQueuePresentKHR
	}

	vec2i_t platform_screen_size(void) {
		int width, height;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);
		return vec2i(width, height);
	}


#elif defined(RENDERER_SOFTWARE) || defined(RENDERER_NULL)// -------------------
	#define PLATFORM_WINDOW_FLAGS 0
	static SDL_Renderer *renderer;
	static SDL_Texture *screenbuffer = NULL;
	static void *screenbuffer_pixels = NULL;
	static int screenbuffer_pitch;
	static vec2i_t screenbuffer_size = vec2i(0, 0);
	static vec2i_t screen_size = vec2i(0, 0);


	void platform_video_init(void) {
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	}

	void platform_video_cleanup(void) {
		if (screenbuffer) {
			SDL_DestroyTexture(screenbuffer);
		}
		SDL_DestroyRenderer(renderer);
	}

	void platform_prepare_frame(void) {
		if (screen_size.x != screenbuffer_size.x || screen_size.y != screenbuffer_size.y) {
			if (screenbuffer) {
				SDL_DestroyTexture(screenbuffer);
			}
			screenbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_size.x, screen_size.y);
			screenbuffer_size = screen_size;
		}
		SDL_LockTexture(screenbuffer, NULL, &screenbuffer_pixels, &screenbuffer_pitch);
	}

	void platform_end_frame(void) {
		screenbuffer_pixels = NULL;
		SDL_UnlockTexture(screenbuffer);
		SDL_RenderCopy(renderer, screenbuffer, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	rgba_t *platform_get_screenbuffer(int32_t *pitch) {
		*pitch = screenbuffer_pitch;
		return screenbuffer_pixels;
	}

	vec2i_t platform_screen_size(void) {
		int width, height;
		SDL_GetWindowSize(window, &width, &height);

		// float aspect = (float)width / (float)height;
		// screen_size = vec2i(240 * aspect, 240);
		screen_size = vec2i(width, height);
		return screen_size;
	}

#else
	#error "Unsupported renderer for platform SDL"
#endif

int main(int argc, char *argv[]) {
	// Video is required for startup. Everything else is best-effort, so a
	// missing audio device or gamepad does not stop the game from running.
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Failed to initialize video: %s\n", SDL_GetError());
		return 1;
	}
	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
		printf("Some subsystems failed to initialize: %s\n", SDL_GetError());
	}

#if defined(RENDERER_VULKAN)
	/* Load Vulkan library explicitly (required on Linux before SDL_Vulkan_CreateSurface) */
	SDL_Vulkan_LoadLibrary(NULL);
#endif

	// Figure out the absolute asset and userdata paths. These may either be
	// supplied at build time through -DPATH_ASSETS=.. and -DPATH_USERDATA=..
	// or received at runtime from SDL. Note that SDL may return NULL for these.
	// We fall back to the current directory (i.e. just "") in this case.

	char *sdl_path_assets = NULL;
	#ifdef PATH_ASSETS
		path_assets = TOSTRING(PATH_ASSETS);
	#else
		sdl_path_assets = SDL_GetBasePath();
		if (sdl_path_assets) {
			path_assets = sdl_path_assets;
		}
	#endif

	char *sdl_path_userdata = NULL;
	#ifdef PATH_USERDATA
		path_userdata = TOSTRING(PATH_USERDATA);
	#else
		sdl_path_userdata = SDL_GetPrefPath("phoboslab", "wipeout");
		if (sdl_path_userdata) {
			path_userdata = sdl_path_userdata;
		}
	#endif

	// Reserve some space for concatenating the asset and userdata paths with
	// local filenames.
	temp_path = mem_bump(max(strlen(path_assets), strlen(path_userdata)) + 64);

	// Load gamecontrollerdb.txt if present.
	// FIXME: Should this load from userdata instead?
	char *gcdb_path = strcat(strcpy(temp_path, path_assets), "gamecontrollerdb.txt");
	int gcdb_res = SDL_GameControllerAddMappingsFromFile(gcdb_path);
	if (gcdb_res < 0) {
		printf("Failed to load gamecontrollerdb.txt\n");
	}
	else {
		printf("load gamecontrollerdb.txt\n");
	}



	gamepad = platform_find_gamepad();

	perf_freq = SDL_GetPerformanceFrequency();

	audio_device = SDL_OpenAudioDevice(NULL, 0, &(SDL_AudioSpec){
		.freq = 44100,
		.format = AUDIO_F32SYS,
		.channels = 2,
		.samples = 1024,
		.callback = platform_audio_callback
	}, NULL, 0);

	window = SDL_CreateWindow(
		SYSTEM_WINDOW_NAME,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SYSTEM_WINDOW_WIDTH, SYSTEM_WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | PLATFORM_WINDOW_FLAGS | SDL_WINDOW_ALLOW_HIGHDPI
	);

	if (!window) {
		printf("Failed to create window: %s\n", SDL_GetError());
		return 1;
	}

	platform_video_init();
	platform_refresh_display_rate(); // seed the limiter's refresh cache
	system_init();

	while (!wants_to_exit) {
		double frame_start = platform_now();
		platform_pump_events();
		platform_prepare_frame();
		system_update();
		platform_end_frame();
		platform_limit_frame(frame_start);
	}

	system_cleanup();
	platform_video_cleanup();

	SDL_DestroyWindow(window);

	if (gamepad) {
		SDL_GameControllerClose(gamepad);
	}

	if (sdl_path_assets) {
		SDL_free(sdl_path_assets);
	}
	if (sdl_path_userdata) {
		SDL_free(sdl_path_userdata);
	}

	SDL_CloseAudioDevice(audio_device);
	SDL_Quit();
	return 0;
}

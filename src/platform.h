#ifndef PLATFORM_H
#define PLATFORM_H

#include "types.h"

void platform_exit(void);
vec2i_t platform_screen_size(void);
double platform_now(void);
bool platform_get_fullscreen(void);
void platform_set_fullscreen(bool fullscreen);
// GL only: SDL_GL_SetSwapInterval; no-op for other renderers/platforms.
void platform_set_vsync(bool enabled);
// Main-loop frame cap (renderer-independent): fps > 0 caps at that rate;
// 0 caps at just under the display refresh (vsync pacing the plan's
// "target just under it"). Negative or implausible rates are treated as 0.
void platform_set_fps_limit(int fps);
void platform_set_audio_mix_cb(void (*cb)(float *buffer, uint32_t len));

bool platform_asset_exists(const char *name);
FILE *platform_open_asset(const char *name, const char *mode);
uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read);
uint8_t *platform_load_userdata(const char *name, uint32_t *bytes_read);
uint32_t platform_store_userdata(const char *name, void *bytes, int32_t len);

#if defined(RENDERER_SOFTWARE)
	rgba_t *platform_get_screenbuffer(int32_t *pitch);
#endif

#if defined(RENDERER_VULKAN)
	void *platform_get_window_handle(void);
#endif

#endif

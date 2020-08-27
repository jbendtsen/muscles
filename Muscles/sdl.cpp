#include <SDL.h>
#include "muscles.h"

int main(int argc, char **argv) {
	return run();
}

#define SW_WIDTH   2048
#define SW_HEIGHT  2048

struct {
	u8 r, g, b;
} back_color;

SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;

static SDL_Surface *sf_cache = nullptr;
static SDL_Renderer *soft_renderer = nullptr;

std::vector<SDL_Cursor*> cursors;
Cursor_Type cursor = CursorDefault;

bool capture = false;
int dpi_w = 0, dpi_h = 0;

void *sdl_get_hw_renderer() {
	return renderer;
}

void sdl_set_cursor(Cursor_Type type) {
	if (type == cursor)
		return;

	cursor = type;
	SDL_SetCursor(cursors[cursor]);
}

void sdl_get_dpi(int& w, int& h) {
	w = dpi_w;
	h = dpi_h;
}

void sdl_log_string(const char *msg) {
	SDL_Log(msg);
}

void sdl_log_last_error() {
	const char *msg = SDL_GetError();
	if (msg && strlen(msg))
		SDL_Log(msg);
}

Renderer sdl_acquire_sw_renderer(int w, int h) {
	sf_cache->w = w;
	sf_cache->h = h;
	sf_cache->pitch = w * 4;
	return soft_renderer;
}

Texture sdl_bake_sw_render() {
	return SDL_CreateTextureFromSurface(renderer, sf_cache);
}

bool sdl_init(const char *title, int width, int height) {
	int res = SDL_Init(SDL_INIT_VIDEO);
	if (res != 0) {
		SDL_Log("SDL_Init() failed (%d)", res);
		return false;
	}

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
	if (!window) {
		SDL_Log("SDL_CreateWindow() failed");
		return false;
	}

	SDL_StartTextInput();

	cursors.resize(NumCursors);
	cursors[CursorDefault]          = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	cursors[CursorClick]            = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	cursors[CursorEdit]             = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	cursors[CursorPan]              = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	cursors[CursorResizeNorthSouth] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	cursors[CursorResizeWestEast]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	cursors[CursorResizeNESW]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	cursors[CursorResizeNWSE]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);

	sf_cache = (SDL_Surface*)sdl_create_surface(nullptr, SW_WIDTH, SW_HEIGHT);
	soft_renderer = SDL_CreateSoftwareRenderer(sf_cache);

	float hdpi, vdpi;
	SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), nullptr, &hdpi, &vdpi);

	dpi_w = (int)hdpi;
	dpi_h = (int)vdpi;

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		SDL_Log("SDL_CreateRenderer() failed");
		return false;
	}

	back_color.r = back_color.g = back_color.b = 0;
	SDL_SetRenderDrawColor(renderer, back_color.r, back_color.g, back_color.b, 255);
	return true;
}

bool sdl_poll_input(Input& input) {
	input.ch = 0;
	input.last_key = 0;
	input.action = false;
	input.lclick = false;
	input.rclick = false;
	input.scroll_x = 0;
	input.scroll_y = 0;

	input.prev_x = input.mouse_x;
	input.prev_y = input.mouse_y;
	SDL_GetMouseState(&input.mouse_x, &input.mouse_y);

	if (input.lmouse || input.rmouse)
		input.held++;
	else
		input.held = 0;

	if (input.del) input.del++;
	if (input.back) input.back++;
	if (input.esc) input.esc++;
	if (input.enter) input.enter++;
	if (input.tab) input.tab++;
	if (input.home) input.home++;
	if (input.end) input.end++;
	if (input.left) input.left++;
	if (input.down) input.down++;
	if (input.right) input.right++;
	if (input.up) input.up++;

	if (input.action_timer > 0) input.action_timer--;

	bool quit = false;
	SDL_Event event;
	while (!quit && SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_TEXTINPUT:
				input.ch = event.text.text[0];
				break;
			case SDL_KEYDOWN:
			{
				auto key = event.key.keysym.sym;
				input.last_key = key;

				if (key == SDLK_LCTRL) input.lctrl = true;
				if (key == SDLK_RCTRL) input.rctrl = true;
				if (key == SDLK_LSHIFT) input.lshift = true;
				if (key == SDLK_RSHIFT) input.rshift = true;

				switch (key) {
					case SDLK_DELETE:
						input.del = 1;
						break;
					case SDLK_BACKSPACE:
						input.back = 1;
						break;
					case SDLK_ESCAPE:
						input.esc = 1;
						break;
					case SDLK_RETURN:
						input.enter = 1;
						break;
					case SDLK_TAB:
						input.tab = 1;
						break;
					case SDLK_HOME:
						input.home = 1;
						break;
					case SDLK_END:
						input.end = 1;
						break;
					case SDLK_LEFT:
						input.left = 1;
						break;
					case SDLK_DOWN:
						input.down = 1;
						break;
					case SDLK_RIGHT:
						input.right = 1;
						break;
					case SDLK_UP:
						input.up = 1;
						break;
				}

				break;
			}
			case SDL_KEYUP:
			{
				auto key = event.key.keysym.sym;
				if (key == SDLK_LCTRL) input.lctrl = false;
				if (key == SDLK_RCTRL) input.rctrl = false;
				if (key == SDLK_LSHIFT) input.lshift = false;
				if (key == SDLK_RSHIFT) input.rshift = false;

				switch (key) {
					case SDLK_DELETE:
						input.del = 0;
						break;
					case SDLK_BACKSPACE:
						input.back = 0;
						break;
					case SDLK_ESCAPE:
						input.esc = 0;
						break;
					case SDLK_RETURN:
						input.enter = 0;
						break;
					case SDLK_TAB:
						input.tab = 0;
						break;
					case SDLK_HOME:
						input.home = 0;
						break;
					case SDLK_END:
						input.end = 0;
						break;
					case SDLK_LEFT:
						input.left = 0;
						break;
					case SDLK_DOWN:
						input.down = 0;
						break;
					case SDLK_RIGHT:
						input.right = 0;
						break;
					case SDLK_UP:
						input.up = 0;
						break;
				}

				break;
			}
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT)  input.lclick = true;
				if (event.button.button == SDL_BUTTON_RIGHT) input.rclick = true;
				if (input.lclick) input.lmouse = true;
				if (input.rclick) input.rmouse = true;

				input.click_x = input.mouse_x;
				input.click_y = input.mouse_y;
				break;
			case SDL_MOUSEBUTTONUP:
			{
				if (event.button.button == SDL_BUTTON_LEFT)  input.lmouse = false;
				if (event.button.button == SDL_BUTTON_RIGHT) input.rmouse = false;

				if (!input.prevent_action) {
					double dx = input.mouse_x - input.click_x;
					double dy = input.mouse_y - input.click_y;

					if (sqrt(dx*dx + dy*dy) < input.action_circle) {
						dx = input.mouse_x - input.action_x;
						dy = input.mouse_y - input.action_y;

						if (sqrt(dx*dx + dy*dy) < input.action_circle)
							input.double_click = input.action_timer > 0;
						else
							input.double_click = false;

						input.action = true;
						input.action_x = input.mouse_x;
						input.action_y = input.mouse_y;
						input.action_timer = input.action_timeout;
					}
				}
				input.prevent_action = false;
				break;
			}
			case SDL_MOUSEWHEEL:
				input.scroll_x = event.wheel.x;
				input.scroll_y = event.wheel.y;
				break;
		}
	}

	return quit;
}

void sdl_update_camera(Camera& camera) {
	SDL_GetRendererOutputSize(renderer, &camera.center_x, &camera.center_y);
	camera.center_x /= 2;
	camera.center_y /= 2;
}

void sdl_acquire_mouse() {
	if (!capture) {
		SDL_CaptureMouse((SDL_bool)true);
		capture = true;
	}
}
void sdl_release_mouse() {
	if (capture) {
		SDL_CaptureMouse((SDL_bool)false);
		capture = false;
	}
}

void sdl_copy(std::string& string) {
	SDL_SetClipboardText(string.c_str());
}

int sdl_paste_into(std::string& string, int offset) {
	if (!SDL_HasClipboardText())
		return 0;

	char *text = SDL_GetClipboardText();
	string.insert(offset, text);
	int len = strlen(text);
	SDL_free(text);

	return len;
}

Surface sdl_create_surface(u32 *data, int w, int h) {
	return (Surface)(data ?
		SDL_CreateRGBSurfaceFrom(data, w, h, 32, w * 4, 0xff000000, 0xff0000, 0xff00, 0xff) :
		SDL_CreateRGBSurface(0, w, h, 32, 0xff000000, 0xff0000, 0xff00, 0xff)
	);
}

Texture sdl_create_texture(u32 *data, int w, int h) {
	SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	if (data) {
		void *bits = nullptr;
		int pitch;
		SDL_LockTexture(tex, nullptr, &bits, &pitch);
		memcpy(bits, data, w * h * 4);
		SDL_UnlockTexture(tex);
	}

	return (Texture)tex;
}

void sdl_get_texture_size(Texture tex, int *w, int *h) {
	SDL_QueryTexture((SDL_Texture*)tex, nullptr, nullptr, w, h);
}

// The 'renderer' parameter is ignored, it is there so that this function can be used as a drop-in replacement for SDL_RenderCopy() if necessary.
void sdl_sw_apply_texture(SDL_Renderer *r, SDL_Texture *tex, SDL_Rect *src, SDL_Rect *dst) {
	int canv_w = sf_cache->w;
	int canv_h = sf_cache->h;
	if (dst->x < 0 || dst->y < 0 || dst->w <= 0 || dst->h <= 0 || dst->x >= canv_w || dst->y >= canv_h)
		return;

	int in_w, in_h, pitch;
	SDL_QueryTexture(tex, nullptr, nullptr, &in_w, &in_h);

	int src_w, src_h;
	if (src) {
		src_w = src->w;
		src_h = src->h;
	}
	else {
		src_w = in_w;
		src_h = in_h;
	}

	u32 *data = nullptr;
	SDL_LockTexture(tex, src, (void**)&data, &pitch);

	if (!data)
		return;

	int out_w = dst->w;
	int out_h = dst->h;
	if (dst->x + out_w > canv_w)
		out_w = canv_w - dst->x;
	if (dst->y + out_h > canv_h)
		out_h = canv_h - dst->y;

	double x_ratio = (double)dst->w / (double)src_w;
	double y_ratio = (double)dst->h / (double)src_h;

	u8 *out = &((u8*)sf_cache->pixels)[4 * (dst->y * canv_w + dst->x)];
	for (int i = 0; i < out_h; i++) {
		u8 *temp = out;
		for (int j = 0; j < out_w; j++, out += 4) {
			int x = (double)j / x_ratio;
			int y = (double)i / y_ratio;
			u8 *in = &((u8*)data)[4 * (y * in_w + x)];

			int a = in[0];
			int b = 255 - a;
			out[0] = (u8)((a * (int)in[0] + b * (int)out[0]) / 255);
			out[1] = (u8)((a * (int)in[1] + b * (int)out[1]) / 255);
			out[2] = (u8)((a * (int)in[2] + b * (int)out[2]) / 255);
			out[3] = (u8)((a * (int)in[3] + b * (int)out[3]) / 255);
		}
		out = &temp[canv_w * 4];
	}

	SDL_UnlockTexture(tex);
}

void sdl_apply_texture(Texture tex, Rect_Int& dst, Rect_Int *src, Renderer rdr) {
	Renderer r = rdr ? (SDL_Renderer*)rdr : renderer;
	if (r != renderer)
		sdl_sw_apply_texture(nullptr, (SDL_Texture*)tex, (SDL_Rect*)src, (SDL_Rect*)&dst);
	else
		SDL_RenderCopy(renderer, (SDL_Texture*)tex, (SDL_Rect*)src, (SDL_Rect*)&dst);
}

void sdl_apply_texture(Texture tex, Rect& dst, Rect_Int *src, Renderer rdr) {
	SDL_Rect dst_fixed = {(int)(dst.x + 0.5), (int)(dst.y + 0.5), (int)(dst.w + 0.5), (int)(dst.h + 0.5)};
	Renderer r = rdr ? (SDL_Renderer*)rdr : renderer;
	if (r != renderer)
		sdl_sw_apply_texture(nullptr, (SDL_Texture*)tex, (SDL_Rect*)src, &dst_fixed);
	else
		SDL_RenderCopy(renderer, (SDL_Texture*)tex, (SDL_Rect*)src, &dst_fixed);
}

void sdl_draw_rect(Rect_Int& rect, RGBA& color, Renderer rdr) {
	SDL_Renderer *r = rdr ? (SDL_Renderer*)rdr : renderer;
	SDL_SetRenderDrawColor(r, (u8)(color.r * 255.0), (u8)(color.g * 255.0), (u8)(color.b * 255.0), (u8)(color.a * 255.0));
	SDL_RenderFillRect(r, (SDL_Rect*)&rect);
}

void sdl_draw_rect(Rect& rect, RGBA& color, Renderer rdr) {
	SDL_Rect rect_fixed = {(int)(rect.x + 0.5), (int)(rect.y + 0.5), (int)(rect.w + 0.5), (int)(rect.h + 0.5)};
	SDL_Renderer *r = rdr ? (SDL_Renderer*)rdr : renderer;
	SDL_SetRenderDrawColor(r, (u8)(color.r * 255.0), (u8)(color.g * 255.0), (u8)(color.b * 255.0), (u8)(color.a * 255.0));
	SDL_RenderFillRect(r, &rect_fixed);
}

void sdl_clear() {
	SDL_SetRenderDrawColor(renderer, back_color.r, back_color.g, back_color.b, 255);
	SDL_RenderClear(renderer);
}

void sdl_render() {
	SDL_RenderPresent(renderer);
}

void sdl_close() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	for (auto& c : cursors) {
		SDL_FreeCursor(c);
		c = nullptr;
	}

	SDL_Quit();
}

void *sdl_lock_texture(Texture tex) {
	int pitch;
	void *data = nullptr;
	SDL_LockTexture((SDL_Texture*)tex, nullptr, &data, &pitch);
	return data;
}

void sdl_unlock_texture(Texture tex) {
	SDL_UnlockTexture((SDL_Texture*)tex);
}

void sdl_destroy_texture(Texture *tex) {
	if (*tex) {
		SDL_DestroyTexture((SDL_Texture*)*tex);
		*tex = nullptr;
	}
}
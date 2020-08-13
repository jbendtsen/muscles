#include <SDL.h>
#include "muscles.h"

int main(int argc, char **argv) {
	return run();
}

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

std::unordered_map<CursorType, SDL_Cursor*> cursors;
CursorType cursor = CursorDefault;

bool capture = false;
int dpi_w = 0, dpi_h = 0;

void sdl_set_cursor(CursorType type) {
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

	cursors[CursorDefault]          = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	cursors[CursorClick]            = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	cursors[CursorEdit]             = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	cursors[CursorPan]              = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	cursors[CursorResizeNorthSouth] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	cursors[CursorResizeWestEast]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	cursors[CursorResizeNESW]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	cursors[CursorResizeNWSE]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);

	float hdpi, vdpi;
	SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), nullptr, &hdpi, &vdpi);

	dpi_w = (int)hdpi;
	dpi_h = (int)vdpi;

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		SDL_Log("SDL_CreateRenderer() failed");
		return false;
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	return true;
}

bool sdl_poll_input(Input& input) {
	input.ch = 0;
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

Texture sdl_create_texture(u32 *data, int w, int h) {
	SDL_Texture *tex = nullptr;
	if (data) {
		SDL_Surface *s = SDL_CreateRGBSurfaceFrom(data, w, h, 32, w * 4, 0xff000000, 0xff0000, 0xff00, 0xff);
		tex = SDL_CreateTextureFromSurface(renderer, s);
		SDL_FreeSurface(s);
	}
	else
		tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	return (Texture)tex;
}

void sdl_get_texture_size(Texture tex, int *w, int *h) {
	SDL_QueryTexture((SDL_Texture*)tex, nullptr, nullptr, w, h);
}

void sdl_apply_texture(Texture tex, Rect_Fixed& dst, Rect_Fixed *src) {
	SDL_RenderCopy(renderer, (SDL_Texture*)tex, (SDL_Rect*)src, (SDL_Rect*)&dst);
}

void sdl_apply_texture(Texture tex, Rect& dst, Rect_Fixed *src) {
	SDL_Rect dst_fixed = {(int)(dst.x + 0.5), (int)(dst.y + 0.5), (int)(dst.w + 0.5), (int)(dst.h + 0.5)};
	SDL_RenderCopy(renderer, (SDL_Texture*)tex, (SDL_Rect*)src, &dst_fixed);
}

void sdl_draw_rect(Rect_Fixed& rect, RGBA& color) {
	SDL_SetRenderDrawColor(renderer, (u8)(color.r * 255.0), (u8)(color.g * 255.0), (u8)(color.b * 255.0), (u8)(color.a * 255.0));
	SDL_RenderFillRect(renderer, (SDL_Rect*)&rect);
}

void sdl_draw_rect(Rect& rect, RGBA& color) {
	SDL_Rect rect_fixed = {(int)(rect.x + 0.5), (int)(rect.y + 0.5), (int)(rect.w + 0.5), (int)(rect.h + 0.5)};
	SDL_SetRenderDrawColor(renderer, (u8)(color.r * 255.0), (u8)(color.g * 255.0), (u8)(color.b * 255.0), (u8)(color.a * 255.0));
	SDL_RenderFillRect(renderer, &rect_fixed);
}

void sdl_clear() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
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
		if (c.second) {
			SDL_FreeCursor(c.second);
			c.second = nullptr;
		}
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
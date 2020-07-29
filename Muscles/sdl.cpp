#include <SDL.h>
#include "muscles.h"

int main(int argc, char **argv) {
	return run();
}

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

bool capture = false;
int dpi_w = 0, dpi_h = 0;

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

inline u32 rgba_to_u32(RGBA color) {
	return ((u32)((u8)(color.r * 255.0)) << 24) |
		((u32)((u8)(color.g * 255.0)) << 16) |
		((u32)((u8)(color.b * 255.0)) <<  8) |
		(u8)(color.a * 255.0);
}

inline float clamp(float f, float min, float max) {
	if (f < min)
		return min;
	if (f > max)
		return max;
	return f;
}

Texture sdl_make_circle(int diameter, RGBA& color) {
	auto pixels = std::make_unique<u32[]>(diameter * diameter);
	float radius = (diameter / 2) - 2;

	RGBA shade = color;

	// circle outline maybe?
	float y = -diameter / 2;
	for (int i = 0; i < diameter; i++) {
		float x = -diameter / 2;
		for (int j = 0; j < diameter; j++) {
			float dist = sqrt((double)(x * x + y * y));
			float lum = 0.5 + clamp(radius - dist, -1, 1) / 2;

			shade.a = color.a * lum;
			pixels[i * diameter + j] = rgba_to_u32(shade);

			x += 1;
		}
		y += 1;
	}

	return sdl_create_texture(pixels.get(), diameter, diameter);
}

inline int abs(int x) {
	return x < 0 ? -x : x;
}

Texture sdl_make_cross(int length, RGBA& color) {
	auto pixels = std::make_unique<u32[]>(length * length);

	float l = (float)length + 0.5;
	int strong = l / 24;
	int weak = l / 16;

	RGBA shade = color;
	
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < length; j++) {
			int d1 = abs(i - j);
			int d2 = abs(i - ((length - 1) - j));

			float lum = 0.0;
			if (d1 == weak || d2 == weak)
				lum = 0.5;
			if (d1 <= strong || d2 <= strong)
				lum = 1.0;

			shade.a = color.a * lum;
			pixels[i * length + j] = rgba_to_u32(shade);
		}
	}

	return sdl_create_texture(pixels.get(), length, length);
}

Texture sdl_make_folder_icon(RGBA& dark, RGBA& light, int w, int h) {
	auto pixels = std::make_unique<u32[]>(w * h);

	float width = (float)w + 0.5;
	float height = (float)h + 0.5;

	int pad_x = width * 0.07;
	int pad_y = height * 0.14;
	int tab_w = width * 0.4;
	int tab_h = height * 0.2;
	int lid_y = height * 0.4;

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			RGBA *color = i < lid_y ? &light : &dark;
			u32 pixel = 0;
			if (i >= pad_y && i < h - pad_y && j >= pad_x && j < w - pad_x && (i > tab_h || j <= tab_w))
				pixel = rgba_to_u32(*color);

			pixels[i * w + j] = pixel;
		}
	}

	return sdl_create_texture(pixels.get(), w, h);
}

Texture sdl_make_file_icon(RGBA& back, RGBA& fold_color, RGBA& line_color, int w, int h) {
	auto pixels = std::make_unique<u32[]>(w * h);

	const float fold_length = 0.17;

	const float line_x = 0.27;
	const float line_start = 0.28;
	const float line_height_frac = 0.4;
	const float line_gap = 0.16;
	const float lines_end = 0.8;

	const float pad_x = 0.2;
	const float pad_y = 0.1;

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			float x = ((float)j + 0.5) / (float)w;
			float y = ((float)i + 0.5) / (float)h;

			int col = 0;
			if (x >= pad_x && x < 1.0 - pad_x && y >= pad_y && y < 1.0 - pad_y)
				col = 1;

			float fold_x = (1.0 - pad_x) - x;
			float fold_y = y - pad_y;

			if (fold_x + fold_y < fold_length)
				col = 0;
			if (col > 0 && fold_x < fold_length && fold_y < fold_length)
				col = 2;

			float start = 0.1 + line_start;
			if (x >= line_x && x < 1.0 - line_x &&
				y >= start && y < lines_end
			) {
				double whole = 0.0;
				float frac = modf((y - start) / line_gap, &whole);
				if (frac <= line_height_frac)
					col = 3;
			}

			RGBA *color = nullptr;
			if (col == 1)
				color = &back;
			else if (col == 2)
				color = &fold_color;
			else if (col == 3)
				color = &line_color;

			pixels[i*w+j] = color ? rgba_to_u32(*color) : 0;
		}
	}

	return sdl_create_texture(pixels.get(), w, h);
}

Texture sdl_make_process_icon(RGBA& back, RGBA& outline, int length) {
	auto pixels = std::make_unique<u32[]>(length * length);

	const double outer_rim = 0.175;

	const double circle_outer = 0.12;
	const double circle_outer_rim = 0.1;

	const double circle_inner = 0.042;
	const double circle_inner_rim = 0.03;

	const double bar_inner = 0.06;
	const double bar_outer = 0.09;

	double root_2 = sqrt(2);
	const double diag_inner = bar_inner * root_2;
	const double diag_outer = bar_outer * root_2;

	for (int i = 0; i < length; i++) {
		for (int j = 0; j < length; j++) {
			int col = 0;
			double x = ((double)j + 0.5) / (double)length;
			double y = ((double)i + 0.5) / (double)length;

			double cx = x - 0.5;
			double cy = y - 0.5;
			double dist = cx * cx + cy * cy;

			if (dist < 0.2) {
				float d1 = x - y;
				float d2 = (1.0 - x) - y;

				if (
					(d1 >= -diag_inner && d1 < diag_inner) ||
					(d2 >= -diag_inner && d2 < diag_inner) ||
					(cx >= -bar_inner && cx < bar_inner) ||
					(cy >= -bar_inner && cy < bar_inner)
				) {
					col = 1;
				}
				else if (
					(d1 >= -diag_outer && d1 < diag_outer) ||
					(d2 >= -diag_outer && d2 < diag_outer) ||
					(cx >= -bar_outer && cx < bar_outer) ||
					(cy >= -bar_outer && cy < bar_outer)
				) {
					col = 2;
				}

				if (col > 0 && dist >= outer_rim)
					col = 2;
			}

			if (dist < circle_outer_rim)
				col = 1;
			else if (col != 1 && dist < circle_outer)
				col = 2;

			if (dist < circle_inner_rim)
				col = 0;
			else if (dist < circle_inner)
				col = 2;

			RGBA *color = nullptr;
			if (col == 1)
				color = &back;
   			else if (col == 2)
				color = &outline;

			pixels[i*length+j] = color ? rgba_to_u32(*color) : 0;
		}
	}

	return sdl_create_texture(pixels.get(), length, length);
}

void sdl_apply_texture(Texture tex, Rect_Fixed *dst, Rect_Fixed *src) {
	SDL_RenderCopy(renderer, (SDL_Texture*)tex, (SDL_Rect*)src, (SDL_Rect*)dst);
}

void sdl_apply_texture(Texture tex, Rect *dst, Rect_Fixed *src) {
	SDL_Rect dst_fixed = {(int)(dst->x + 0.5), (int)(dst->y + 0.5), (int)(dst->w + 0.5), (int)(dst->h + 0.5)};
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
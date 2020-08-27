#include "muscles.h"
#include "ui.h"

#define PROJECT_PATH "C:/Users/Jack/source/repos/Muscles"

#define FONT_MONO "RobotoMono-Regular.ttf"
#define FONT_SANS "OpenSans-Regular.ttf"

int run() {
	if (!sdl_init("Muscles", 960, 540))
		return 1;

	std::string path = PROJECT_PATH;
	path += "/Fonts/";
	path += FONT_MONO;
	Font_Face face = load_font_face(path.c_str());
	if (!face)
		return 2;

	Workspace ctx(face);
	Camera camera;
	Input input;

	while (true) {
		if (sdl_poll_input(input))
			break;

		bool zoom = false;
		float old_scale = camera.scale;
		Point cursor = camera.to_world(input.mouse_x, input.mouse_y);

		if (input.scroll_y != 0 && (input.lctrl || input.rctrl)) {
			float coeff = 1.0 + camera.zoom_coeff;
			if (input.scroll_y < 0)
				coeff = 1.0 / coeff;

			camera.scale *= coeff;
			zoom = true;
		}

		if (camera.scale > camera.max_scale)
			camera.scale = camera.max_scale;
		if (camera.scale < camera.min_scale)
			camera.scale = camera.min_scale;

		sdl_update_camera(camera);

		if (zoom) {
			float dx = (float)(input.mouse_x - camera.center_x) / camera.scale;
			float dy = (float)(input.mouse_y - camera.center_y) / camera.scale;
			camera.x = cursor.x - dx;
			camera.y = cursor.y - dy;

			ctx.adjust_scale(old_scale, camera.scale);
		}

		if (!ctx.box_moving && input.lmouse && (input.lctrl || input.rctrl)) {
			camera.moving = true;
			sdl_acquire_mouse();
		}
		if (!input.lmouse) {
			ctx.box_moving = false;
			for (auto& b : ctx.boxes)
				b->moving = false;

			camera.moving = false;
			sdl_release_mouse();
		}

		if (camera.moving) {
			camera.x -= (float)(input.mouse_x - input.prev_x) / camera.scale;
			camera.y -= (float)(input.mouse_y - input.prev_y) / camera.scale;
		}

		ctx.update(camera, input, cursor);
	}

	destroy_font_face(face);

	sdl_close();

	return 0;
}

std::pair<int, int> next_power_of_2(int num) {
	int power = 2;
	int exp = 1;
	while (power < num) {
		power *= 2;
		exp++;
	}

	return std::make_pair(power, exp);
}

int count_digits(u64 num) {
	if (num == 0)
		return 1;

	u64 n = num;
	int n_digits = 0;
	while (n) {
		n >>= 4;
		n_digits++;
	}

	return n_digits;
}

void print_hex(const char *hex, char *out, u64 n, int n_digits) {
	int shift = (n_digits-1) * 4;
	char *p = out;
	for (int i = 0; i < n_digits; i++) {
		*p++ = hex[(n >> shift) & 0xf];
		shift -= 4;
	}
	*p = 0;
}
#include "muscles.h"

static inline u32 rgba_to_u32(RGBA color) {
	return ((u32)((u8)(color.r * 255.0)) << 24) |
		((u32)((u8)(color.g * 255.0)) << 16) |
		((u32)((u8)(color.b * 255.0)) <<  8) |
		(u8)(color.a * 255.0);
}

static inline int abs(int x) {
	return x < 0 ? -x : x;
}

static inline float clamp(float f, float min, float max) {
	if (f < min)
		return min;
	if (f > max)
		return max;
	return f;
}

Texture make_circle(RGBA& color, int diameter) {
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

Texture make_triangle(RGBA& color, int width, int height) {
	auto pixels = std::make_unique<u32[]>(width * height);
	RGBA shade = color;

	const double y_begin = -0.15;
	const double y_inter = 0.25;
	const double x_dil = 1.2;
	const double intensity = 20.0;

	double w = (double)width;
	double h = (double)height;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			double x = -0.5 + ((double)j + 0.5) / w;
			double y = -0.5 + ((double)i + 0.5) / h;

			float lum = 0.0;
			if (y >= y_begin) {
				double l = y_inter - (abs(x * x_dil) + y);
				lum = clamp(l * intensity, 0.0, 1.0);
			}

			shade.a = color.a * lum;
			pixels[i*width+j] = rgba_to_u32(shade);
		}
	}

	return sdl_create_texture(pixels.get(), width, height);
}

Texture make_cross_icon(RGBA& color, int length) {
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

Texture make_folder_icon(RGBA& dark, RGBA& light, int w, int h) {
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

Texture make_file_icon(RGBA& back, RGBA& fold_color, RGBA& line_color, int w, int h) {
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

Texture make_process_icon(RGBA& back, RGBA& outline, int length) {
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
				double d1 = x - y;
				double d2 = (1.0 - x) - y;

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

Texture make_vertical_divider_icon(RGBA& color, int height, double squish, double gap, double thicc, double sharpness, int *width) {
	RGBA clr = color;
	int g = (int)((double)height * gap + 0.5);
	gap = (double)g / (double)height;

	int w = (double)height * (1.0 + gap + thicc) + 0.5;
	auto pixels = std::make_unique<u32[]>(w * height);

	double mirror = (gap + thicc) / 2.0;
	double t_half = thicc / 2.0;
	double start = 0.5 - t_half;
	double end = 0.5 + t_half;
	double lum = 0.0;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < w; j++) {
			double x = ((double)j + 0.5) / (double)height - 0.5;
			double y = ((double)i + 0.5) / (double)height - 0.5;

			x -= t_half;
			x = x < 0.0 ? x : gap - x;
			y *= squish;

			double d1 = x - y;
			double d2 = -x - y;
			double lum = 0.0;

			if (d1 >= -end && d2 < end && (d1 < -start || d2 > start) && d1 < start && d2 >= -start) {
				double bar = y > 0.0 ? d1 + 0.5 : d2 - 0.5;
				lum = 1.0 - abs(bar / (thicc / 2.0));
       			lum = clamp(lum * sharpness, 0.0, 1.0);
			}

			clr.a = color.a * lum;
			pixels[i*w+j] = rgba_to_u32(clr);
		}
	}

	if (width)
		*width = w;

	return sdl_create_texture(pixels.get(), w, height);
}

Texture make_goto_icon(RGBA& color, int length) {
	auto pixels = std::make_unique<u32[]>(length * length);
	RGBA shade = color;

	const double fade_distance = 0.02;
	const double inner_edge = 0.3;
	const double outer_edge = 0.45;

	const double arc_dilation = 1.1;
	const double arc_x_offset = 0.1;
	const double arc_y_offset = -0.1;

	const double arrow_size = 0.3;
	const double arrow_squish = 1.15;
	const double arrow_fade = 0.08;

	double len = (double)length;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < length; j++) {
			double x = ((double)j + 0.5) / len - 0.5;
			double y = ((double)i + 0.5) / len - 0.5;
    
			double lum = 0.0;
    
			double cx = x * arc_dilation + arc_x_offset;
			double cy = y + arc_y_offset;
			double ix = x + outer_edge - inner_edge;

			double arrow_x = (((outer_edge - arc_x_offset) / arc_dilation) + (inner_edge - (ix - x))) / 2.0;

			if (cy < 0.0 && cy-cx < 0.0) {
				double outer = sqrt(cx * cx + cy * cy);
				double inner = sqrt(ix * ix + cy * cy);
        
				if (inner >= inner_edge && outer < outer_edge) {
					inner -= inner_edge;
					outer = outer_edge - outer;

					if (inner < fade_distance)
						lum = inner / fade_distance;
					else if (outer < fade_distance)
						lum = outer / fade_distance;
					else
						lum = 1.0;
				}
			}

			double head = 1.0 - (y + abs(x - arrow_x) * arrow_squish) / arrow_size;
			if (cy >= 0.0 && head > 0.0)
				lum = clamp(head / arrow_fade, 0.0, 1.0);

			shade.a = color.a * lum;
			pixels[i*length+j] = lum == 0 ? 0 : rgba_to_u32(shade);
		}
	}

	return sdl_create_texture(pixels.get(), length, length);
}

double smooth_edges(double pos, double end, double fade) {
    double outer = end - pos;

    double lum = 1.0;
    if (pos < fade)
        lum = pos / fade;
    else if (outer < fade)
        lum = outer / fade;

    return lum;
}

Texture make_glass_icon(RGBA& color, int length) {
	auto pixels = std::make_unique<u32[]>(length * length);
	RGBA shade = color;

	const double circle_fade = 0.025;
	const double handle_fade = 0.02;

	const double circle_x = 0.4;
	const double circle_y = 0.4;

	const double inner_edge = 0.18;
	const double outer_edge = 0.3;

	const double handle_height = 0.15;
	const double handle_start = 0.1;
	const double handle_width = 0.7;

	double len = (double)length;
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < length; j++) {
			double x = ((double)j + 0.5) / len - 0.5;
			double y = ((double)i + 0.5) / len - 0.5;

			double lum = 0.0;

			double cx = x + 0.5 - circle_x;
			double cy = y + 0.5 - circle_y;

			double dist = sqrt(cx*cx + cy*cy);
			if (dist >= inner_edge && dist < outer_edge)
				lum = smooth_edges(dist - inner_edge, outer_edge - inner_edge, circle_fade);

			double w = outer_edge - inner_edge;
			double handle_x = x + y - handle_start;
			double handle_y = x - y + handle_height / 2.0;

			if (handle_x >= 0.0 && handle_x < handle_width &&
				handle_y >= 0.0 && handle_y < handle_height
			) {
				lum += smooth_edges(handle_x, handle_width, handle_fade) *
				       smooth_edges(handle_y, handle_height, handle_fade);
			}

			shade.a = color.a * clamp(lum, 0.0, 1.0);
			pixels[i*length+j] = lum > 0.0 ? rgba_to_u32(shade) : 0;
		}
	}

	return sdl_create_texture(pixels.get(), length, length);
}
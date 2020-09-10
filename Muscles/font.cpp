#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "muscles.h"

#define FLOAT_FROM_16_16(n) ((float)((n) >> 16) + (float)((n) & 0xffff) / 65536.0)

FT_Library library = nullptr;

Font_Face load_font_face(const char *path) {
	if (!library) {
		if (FT_Init_FreeType(&library) != 0) {
			fprintf(stderr, "Failed to load freetype\n");
			return nullptr;
		}
	}

	FT_Face face;
	if (FT_New_Face(library, path, 0, &face) != 0) {
		fprintf(stderr, "Error loading font \"%s\"\n", path);
		return nullptr;
	}

	return (FT_Face)face;
}

int next_pow2(int x) {
	x--;
	int n = 2;
	while (x >>= 1)
		n <<= 1;

	return n;
}

void make_font_render(FT_Face face, float size, RGBA& color, int dpi_w, int dpi_h, Font_Render& render) {
	sdl_destroy_texture(&render.tex);

	float req_size = size * 64.0;
	int pts = (int)(req_size + 0.5);

	render.pts = pts;
	FT_Set_Char_Size(face, 0, render.pts, dpi_w, dpi_h);

	float pt_w = (render.pts * dpi_w) / (72.0 * 64.0);
	float pt_h = (render.pts * dpi_h) / (72.0 * 64.0);

	float em_units = (float)face->units_per_EM;
	int max_w = (int)(1.5 + pt_w * (float)face->max_advance_width / em_units);
	int max_h = (int)(1.5 + pt_h * (float)face->max_advance_height / em_units);

	if (max_w <= 0)
		return;

	int n_cols = 16;
	int w = max_w * n_cols + 1;
	int tex_w = next_pow2(w);

	n_cols = tex_w / max_w;
	int n_rows = (1 + (N_CHARS-1) / n_cols);

	int h = max_h * n_rows + 1;
	int tex_h = next_pow2(h);

	Texture tex = sdl_create_texture(nullptr, tex_w, tex_h);
	void *data = sdl_lock_texture(tex);

	for (int c = MIN_CHAR; c <= MAX_CHAR; c++) {
		FT_Load_Char(face, c, FT_LOAD_RENDER);

		FT_Bitmap bmp = face->glyph->bitmap;
		int left = face->glyph->bitmap_left;
		int top = face->glyph->bitmap_top;

		int idx = c - MIN_CHAR;
		Glyph *gl = &render.glyphs[idx];
		*gl = {
			/*atlas_x*/ (idx % n_cols) * max_w,
			/*atlas_y*/ (idx / n_cols) * max_h,
			/*img_w*/   (int)bmp.width,
			/*img_h*/   (int)bmp.rows,
			/*box_w*/   (float)FLOAT_FROM_16_16(face->glyph->linearHoriAdvance),
			/*box_h*/   (float)FLOAT_FROM_16_16(face->glyph->linearVertAdvance),
			/*left*/    left,
			/*top*/     top
		};

		int off = 0;
		for (int i = 0; i < gl->img_h; i++) {
			u32 *p = &((u32*)data)[((gl->atlas_y + i) * tex_w) + gl->atlas_x];
			for (int j = 0; j < gl->img_w; j++, off++) {
				float lum = (float)bmp.buffer[off];
				*p++ =
					((u32)(color.r * 255.0) << 24) |
					((u32)(color.g * 255.0) << 16) |
					((u32)(color.b * 255.0) << 8) |
					(u32)(color.a * lum);
			}
		}
	}

	sdl_unlock_texture(tex);
	render.tex = tex;
}

void destroy_font_face(Font_Face face) {
	FT_Done_Face((FT_Face)face);
}

void ft_quit() {
	FT_Done_FreeType(library);
}

void Font_Render::draw_text_simple(void *renderer, const char *text, float x, float y) {
	Rect_Int src, dst;
	y += text_height();

	int len = strlen(text);
	for (int i = 0; i < len; i++) {
		if (text[i] < MIN_CHAR || text[i] > MAX_CHAR)
			continue;

		Glyph *gl = &glyphs[text[i] - MIN_CHAR];

		src.x = gl->atlas_x;
		src.y = gl->atlas_y;
		src.w = gl->img_w;
		src.h = gl->img_h;

		dst.x = x + gl->left;
		dst.y = y - gl->top;
		dst.w = src.w;
		dst.h = src.h;

		sdl_apply_texture(tex, dst, &src, renderer);

		x += gl->box_w;
	}
}

void Font_Render::draw_text(void *renderer, const char *text, float x, float y, Render_Clip& clip, int tab_cols, bool multiline) {
	bool clip_top = (clip.flags & CLIP_TOP) != 0;
	bool clip_bottom = (clip.flags & CLIP_BOTTOM) != 0;
	bool clip_left = (clip.flags & CLIP_LEFT) != 0;
	bool clip_right = (clip.flags & CLIP_RIGHT) != 0;

	if ((clip_right && x > clip.x_upper) || (clip_bottom && y > clip.y_upper))
		return;

	float x_start = x;
	Rect_Int src, dst;

	bool draw = true;
	int col = 0;
	int offset = 0;
	int len = strlen(text);

	float height = text_height();
	y += height;

	if (clip_top && y < clip.y_lower) {
		for (offset = 0; offset < len && y < clip.y_lower; offset++) {
			if (text[offset] == '\n')
				y += height;
		}

		if (offset >= len)
			return;
	}

	for (int i = offset; i < len; i++, col++) {
		if (text[i] == '\t') {
			int add = tab_cols - (col % tab_cols);
			x += (float)add * glyph_for(' ')->box_w;
			col += add - 1;
			continue;
		}

		if (multiline && text[i] == '\n') {
			x = x_start;
			y += height;
			col = -1;
			draw = true;
			continue;
		}

		if (!draw || text[i] < MIN_CHAR || text[i] > MAX_CHAR)
			continue;

		Glyph *gl = &glyphs[text[i] - MIN_CHAR];
		float advance = gl->box_w;

		src.x = gl->atlas_x;
		src.y = gl->atlas_y;
		src.w = gl->img_w;
		src.h = gl->img_h;

		float y_off = 0;
		
		if (clip_left) {
			if (x + gl->img_w < clip.x_lower) {
				x += advance;
				continue;
			}
			float delta = (float)x - clip.x_lower;
			if (delta < 0) {
				src.x = ((float)src.x - delta + 0.5);
				src.w = ((float)src.w + delta + 0.5);
				x -= delta;
				advance += delta;
			}
		}
		if (clip_bottom) {
			if (y + gl->img_h < clip.y_lower) {
				x += advance;
				continue;
			}
			float delta = (float)y - gl->top - clip.y_lower;
			if (delta < 0) {
				src.y = ((float)src.y - delta + 0.5);
				src.h = ((float)src.h + delta + 0.5);
				y_off = -delta;
			}
		}
		dst.x = x + gl->left;
		dst.y = y + y_off - gl->top;

		if (clip_right) {
			int w = clip.x_upper - dst.x;
			src.w = w < src.w ? w : src.w;
		}
		if (clip_bottom) {
			int h = clip.y_upper - dst.y;
			src.h = h < src.h ? h : src.h;
		}

		dst.w = src.w;
		dst.h = src.h;

		sdl_apply_texture(tex, dst, &src, renderer);

		x += advance;
		if (clip_right && x > clip.x_upper)
			draw = false;
	}
}

Font::Font(Font_Face f, float sz, RGBA& color, int dpi_w, int dpi_h) {
	face = f;
	size = sz;
	this->color = color;
	prev_scale = 1.0;
	make_font_render((FT_Face)face, size, color, dpi_w, dpi_h, render);
}

void Font::adjust_scale(float scale, int dpi_w, int dpi_h) {
	if (scale != prev_scale) {
		make_font_render((FT_Face)face, size * scale, color, dpi_w, dpi_h, render);
		prev_scale = scale;
	}
}
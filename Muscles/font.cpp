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
	int max_w = (int)(0.5 + pt_w * (float)face->max_advance_width / em_units);
	int max_h = (int)(0.5 + pt_h * (float)face->max_advance_height / em_units);

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
			/*box_w*/   (int)FLOAT_FROM_16_16(face->glyph->linearHoriAdvance),
			/*box_h*/   (int)FLOAT_FROM_16_16(face->glyph->linearVertAdvance),
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

void Font_Render::draw_text(const char *text, int x, int y, int w, int max_y, int offset) {
	int max_x = -1;
	if (w >= 0)
		max_x = offset + x + w;

	int start = x;
	int len = strlen(text);

	Rect_Fixed src, dst;

	y += glyph_for('k')->box_h;

	for (int i = 0; i < len; i++) {
		Glyph *gl = glyph_for(text[i]);
		if (!gl)
			continue;

		src.x = gl->atlas_x;
		src.y = gl->atlas_y;
		src.w = gl->img_w;
		src.h = gl->img_h;

		if (max_x >= 0) {
			w = max_x - x;
			src.w = w < src.w ? w : src.w;
		}
		if (max_y >= 0) {
			int h = max_y - (y - gl->top);
			src.h = h < src.h ? h : src.h;
		}

		dst.x = x + gl->left - offset;
		dst.y = y - gl->top;
		dst.w = src.w;
		dst.h = src.h;

		if (dst.x >= start + gl->left)
			sdl_apply_texture(tex, &dst, &src);

		x += gl->box_w;
		if (max_x >= 0 && x > max_x)
			break;
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
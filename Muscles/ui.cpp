#include "muscles.h"
#include "ui.h"

static inline float clamp(float f, float min, float max) {
	if (f < min)
		return min;
	if (f > max && max >= min)
		return max;
	return f;
}

Rect make_ui_box(Rect_Fixed& box, Rect& elem, float scale) {
	return {
		.x = (float)box.x + elem.x * scale,
		.y = (float)box.y + elem.y * scale,
		.w = elem.w * scale,
		.h = elem.h * scale
	};
}

float center_align_title(Label *title, Box& b, float scale, float y_offset) {
	float font_height = title->font->render.text_height();
	float padding = title->padding * font_height;

	if (title->text.size() > 0) {
		title->width = title->font->render.text_width(title->text.c_str());
		title->x = (b.box.w * scale - (float)title->width) / 2;
		float line_off = title->font->line_offset * font_height;
		title->y = y_offset * scale + padding - line_off;
	}

	float line_h = font_height + (title->font->line_spacing * font_height);
	return (line_h + padding * 2) / scale;
}

void Image::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);
	sdl_apply_texture(img, &box);
}

void Label::update_position(Camera& view) {
	float font_height = font->render.text_height();
	float pad = padding * font_height;

	x = pos.x * view.scale + pad;
	y = pos.y * view.scale - pad;
	width = pos.w * view.scale - pad;
}

void Label::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	clip.x_upper = rect.x + x + width;
	font->render.draw_text(text.c_str(), rect.x + x, rect.y + y, clip);
}

void Button::update_size(float scale) {
	Theme *theme = active ? &active_theme : &inactive_theme;

	float font_unit = theme->font->render.text_height() / scale;
	width = theme->font->render.text_width(text.c_str()) / scale;
	if (icon)
		width += font_unit * 1.2f;

	float hack = 0.15;
	width += (hack + 2 * x_offset) * font_unit;

	height = 1.2f * font_unit;
}

bool Button::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	return pos.contains(cursor);
}

void Button::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect r = make_ui_box(rect, pos, view.scale);

	float pad = padding * view.scale;
	r.x += pad;
	r.y += pad;
	r.w -= pad * 2;
	r.h -= pad * 2;

	Theme *theme = active ? &active_theme : &inactive_theme;
	float font_height = theme->font->render.text_height();

	RGBA *color = &theme->back;
	if (elem_hovered)
		color = &theme->hover;
	if (parent && parent->parent->held_element == this)
		color = &theme->held;

	sdl_draw_rect(r, *color);

	float x = x_offset * font_height;
	float y = y_offset * font_height;

	if (icon) {
		Rect box = {r.x, r.y, font_height, font_height};

		if (icon_right)
			box.x += r.w - x - font_height;
		else {
			box.x += x;
			x += font_height * 1.2f;
		}

		sdl_apply_texture(icon, &box);
	}

	theme->font->render.draw_text_simple(text.c_str(), r.x + x, r.y + y_offset * font_height);
}

void Divider::make_icon(float scale) {
	bool was_hl = icon == icon_hl;
	sdl_destroy_texture(&icon_default);
	sdl_destroy_texture(&icon_hl);

	RGBA fade_color = default_color;
	fade_color.a = 0.5;

	icon_h = padding * 1.25 * scale + 0.5;
	int w = 0;

	double squish = 1.0;
	double gap = breadth * 2 / padding;
	double thickness = 0.3;
	double sharpness = 3.0;

	icon_default = make_vertical_divider_icon(fade_color, icon_h, squish, gap, thickness, sharpness, &w);
	icon_hl = make_vertical_divider_icon(default_color, icon_h, squish, gap, thickness, sharpness, &w);

	icon_w = w;
	icon = was_hl ? icon_hl : icon_default;
}

bool Divider::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!moveable)
		return false;

	Rect box = pos;
	if (vertical) {
		box.x -= padding;
		box.w += 2 * padding;
	}
	else {
		box.y -= padding;
		box.h += 2 * padding;
	}

	if (!held && !box.contains(cursor)) {
		icon = icon_default;
		return false;
	}
	icon = icon_hl;

	if (!input.lmouse) {
		held = false;
		return false;
	}

	if (input.lclick) {
		hold_pos = vertical ? cursor.x - pos.x : cursor.y - pos.y;
		held = true;
		parent->ui_held = true;
	}

	if (held) {
		float cur = vertical ? cursor.x : cursor.y;
		position = clamp(cur - hold_pos, minimum, maximum);
	}
}

void Divider::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect r = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(r, default_color);

	if (icon) {
		Rect box = {
			r.x + (r.w - (float)icon_w) / 2,
			r.y + (r.h - (float)icon_h) / 2,
			icon_w,
			icon_h
		};
		sdl_apply_texture(icon, &box);
	}
}

void Scroll::set_maximum(double max, double span) {
	maximum = max;

	show_thumb = span < max;
	if (!show_thumb)
		span = max;

	view_span = span;
}

void Scroll::engage(Point& p) {
	held = true;
	float thumb_pos = length * (1 - thumb_frac) * position / (maximum - view_span);

	if (vertical)
		hold_region = p.y - (pos.y + thumb_pos);
	else
		hold_region = p.x - (pos.x + thumb_pos);

	if (hold_region < 0 || hold_region > thumb_frac * length)
		hold_region = 0;

	parent->ui_held = true;
}

void Scroll::scroll(double delta) {
	position = clamp(position + delta, 0, maximum - view_span);
}

bool Scroll::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (input.lclick && pos.contains(cursor))
		engage(cursor);

	if (!input.lmouse)
		held = false;

	double old_pos = position;
	if (held) {
		double dist = 0;
		if (vertical)
			dist = cursor.y - (pos.y + hold_region);
		else
			dist = cursor.x - (pos.x + hold_region);

		double span = (1 - thumb_frac) * length;
		position = dist * (maximum - view_span) / span;
	}

	// apply bounds-checking
	scroll(0);
	return false;
}

void Scroll::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	if (vertical)
		length = pos.h - 2*padding;
	else
		length = pos.w - 2*padding;

	Rect bar = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(bar, back);

	if (!show_thumb)
		return;

	thumb_frac = view_span / maximum;
	if (thumb_frac < thumb_min) thumb_frac = thumb_min;

	float thumb_pos = length * (1 - thumb_frac) * position / (maximum - view_span);
	float thumb_len = thumb_frac * length * view.scale;

	float gap = 2*padding * view.scale;
	float x = padding;
	float y = padding;

	float w = 0, h = 0;
	if (vertical) {
		y += thumb_pos;
		w = bar.w - gap;
		h = thumb_len;
	}
	else {
		x += thumb_pos;
		w = thumb_len;
		h = bar.h - gap;
	}

	Rect_Fixed scroll_rect = {
		bar.x + (float)(x * view.scale + 0.5),
		bar.y + (float)(y * view.scale + 0.5),
		w,
		h
	};

	RGBA *color = &default_color;
	if (held)
		color = &sel_color;
	else if (hl)
		color = &hl_color;

	sdl_draw_rect(scroll_rect, *color);
}

bool Scroll::highlight(Camera& view, Point& inside) {
	hl = pos.contains(inside);
	return hl;
}

void Scroll::deselect() {
	hl = false;
}

bool Data_View::highlight(Camera& view, Point& inside) {
	if (!font) {
		hl_row = -1;
		return false;
	}

	int top = vscroll ? vscroll->position : 0;

	Rect table = pos;
	if (show_column_names) {
		table.y += header_height;
		table.h -= header_height;
	}

	float font_height = (float)font->render.text_height();
	float font_units = font_height / view.scale;

	int n_rows = data.visible >= 0 ? data.visible : data.row_count();
	int n_cols = data.column_count();
	float total_width = table.w - font_units * ((n_cols - 1) * column_spacing);

	float space = font->line_spacing * font_units;

	float y = 0;
	float item_w = table.w - 4;

	hl_row = -1;
	for (int i = top; i < n_rows && y < table.h; i++) {
		float h = y > table.h - item_height ? table.h - y : item_height;
		if (inside.x >= table.x && inside.x < table.x + table.w && inside.y >= table.y + y && inside.y < table.y + y + h) {
			hl_row = i;
			return true;
		}
		y += item_height;
	}

	return false;
}

float Data_View::column_width(float total_width, float font_height, float scale, int idx) {
	float w = data.headers[idx].width * total_width * scale;
	if (data.headers[idx].max_size > 0) {
		float max = data.headers[idx].max_size * font_height;
		w = w < max ? w : max;
	}

	return w;
}

void Data_View::draw_item_backing(RGBA& color, Rect& back, float scale, int idx) {
	int top = vscroll ? vscroll->position : 0;
	if (idx < 0 || idx < top)
		return;

	float y = (idx - top) * item_height * scale;
	float h = item_height * scale;
	if (y + h > back.h)
		h = back.h - y;

	if (h > 0) {
		Rect_Fixed r = {
			back.x,
			back.y + y,
			back.w,
			h
		};
		sdl_draw_rect(r, color);
	}
}

void Data_View::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	int n_rows = data.row_count();
	int n_cols = data.column_count();

	float font_height = (float)font->render.text_height();
	float font_units = font_height / view.scale;

	item_height = (font_height + (font->line_spacing * font_height)) / view.scale;

	Rect table = pos;
	if (show_column_names) {
		table.y += header_height;
		table.h -= header_height;
	}

	int top = 0;
	int n_visible = table.h / item_height;

	int n_items = data.visible;
	if (n_items < 0)
		n_items = n_rows;

	if (vscroll) {
		top = vscroll->position;
		vscroll->set_maximum(n_items, n_visible);
	}

	float scroll_x = 0;
	float table_w = pos.w;
	if (hscroll) {
		scroll_x = hscroll->position;
		hscroll->set_maximum(hscroll->maximum, pos.w);
		table_w = hscroll->maximum;
	}
	scroll_x *= view.scale;

	float total_width = table_w - font_units * ((n_cols - 1) * column_spacing);

	Rect back = make_ui_box(rect, table, view.scale);

	if (show_column_names) {
		float x = back.x;
		float y = back.y - (header_height * view.scale + font->line_offset * font_height);

		for (int i = 0; i < n_cols; i++) {
			font->render.draw_text_simple(data.headers[i].name, x, y);

			float w = column_width(total_width, font_height, view.scale, i);
			x += w + column_spacing * font_height;
		}
	}

	sdl_draw_rect(back, default_color);

	draw_item_backing(sel_color, back, view.scale, sel_row);
	if (hl_row != sel_row && (box_hovered || focussed))
		draw_item_backing(hl_color, back, view.scale, hl_row);

	auto draw_tex = [this](Texture t, Rect_Fixed& dst, Rect_Fixed& src, float x, float tex_y, float y_max) {
		sdl_get_texture_size(t, &src.w, &src.h);

		dst.x = x;
		dst.y = tex_y;

		if (tex_y + dst.h > y_max) {
			int h = y_max - tex_y;
			src.h = (float)(src.h * h) / (float)dst.h;
			dst.h = h;
		}

		sdl_apply_texture(t, &dst, &src);
	};

	float pad = padding * view.scale;
	float x = back.x + pad - scroll_x;
	float y = back.y;
	float y_max = y + table.h * view.scale;

	float line_off = font->line_offset * font_height;
	float line_h = font_height + (font->line_spacing * font_height);
	float sp_half = font->line_spacing * font_height / 2;

	Rect_Fixed dst = {0, 0, font_height, font_height};
	Rect_Fixed src = {0};

	clip.x_lower = x + scroll_x;
	clip.x_upper = back.x + back.w - pad;
	clip.y_upper = y_max;

	float table_off_y = y;

	for (int i = 0; i < n_cols && x < back.x + back.w; i++) {
		float w = column_width(total_width, font_height, view.scale, i);

		Type type = data.headers[i].type;

		if (data.visible >= 0) {
			int n = -1;
			for (auto& idx : data.list) {
				if (y >= y_max)
					break;
				n++;
				if (n < top)
					continue;

				auto thing = data.columns[i][idx];
				if (thing) {
					if (type == String)
						font->render.draw_text((const char*)thing, x, y - line_off, clip);
					else if (type == File) {
						auto name = (const char*)((File_Entry*)thing)->name;
						font->render.draw_text(name, x, y - line_off, clip);
					}
					else if (type == Tex)
						draw_tex(thing, dst, src, x, y + sp_half, y_max);
				}

				y += line_h;
			}
		}
		else {
			for (int j = top; j < n_rows && y < y_max; j++) {
				auto thing = data.columns[i][j];
				if (thing) {
					if (type == String)
						font->render.draw_text((const char*)thing, x, y - line_off, clip);
					else if (type == File) {
						auto name = (const char*)((File_Entry*)thing)->name;
						font->render.draw_text(name, x, y - line_off, clip);
					}
					else if (type == Tex)
						draw_tex(thing, dst, src, x, y + sp_half, y_max);
				}

				y += line_h;
			}
		}

		x += w + column_spacing * font_height;
		y = table_off_y;
	}
}

bool Data_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (hovered) {
		if (consume_box_scroll || pos.contains(cursor)) {
			Scroll *scroll = input.lshift || input.rshift ? hscroll : vscroll;
			if (scroll && input.scroll_y > 0)
				scroll->scroll(-1);
			if (scroll && input.scroll_y < 0)
				scroll->scroll(1);
		}
	}

	return false;
}

void Data_View::deselect() {
	hl_row = -1;
}

void Data_View::release() {
	data.release();
}

bool Edit_Box::remove(bool is_back) {
	if (is_back) {
		if (cursor <= 0)
			return false;

		cursor--;
	}
	else if (cursor >= line.size())
		return false;

	line.erase(cursor, 1);
	return true;
}

void Edit_Box::move_cursor(int delta) {
	cursor += delta;
	if (cursor <= 0)
		cursor = 0;
	else if (cursor > line.size())
		cursor = line.size();
}

void Edit_Box::clear() {
	line.clear();
	cursor = 0;
	offset = 0;
}

void Edit_Box::key_handler(Camera& view, Input& input) {
	update = false;

	int delta = 0;
	if (input.strike(input.back)) {
		update = remove(true);
		delta = update ? -1 : 0;
	}
	else if (input.strike(input.del))
		update = remove(false);
	else if (input.strike(input.left)) {
		move_cursor(-1);
		delta = -1;
	}
	else if (input.strike(input.right)) {
		move_cursor(1);
		delta = 1;
	}
	else if (input.ch) {
		line.insert(line.begin() + cursor, input.ch);
		cursor++;
		delta = 1;
		update = true;
	}

	if (update && table)
		table->update_filter(line);

	if (delta) {
		int x = 0;
		const char *str = get_str();
		font->render.text_width(str, cursor, &x);

		float front = 0;
		if (cursor > 0)
			front = font->render.glyph_for(str[cursor-1])->box_w;

		if (delta > 0 && x > offset + window_w)
			offset = x - window_w;
		else if (delta < 0 && x - front < offset)
			offset = x - front;

		if (offset < 0)
			offset = 0;
	}
}

void Edit_Box::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect bar = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(bar, default_color);

	const char *str = get_str();
	int cur_x;
	font->render.text_width(str, cursor, &cur_x);

	float height = font->render.text_height();
	float gap_x = text_off_x * height;
	float gap_y = text_off_y * height;

	float text_x = pos.x * view.scale + gap_x;
	float text_y = pos.y * view.scale + gap_y;
	window_w = (pos.w * view.scale) - gap_x * 2;

	clip.x_lower = rect.x + text_x;
	clip.x_upper = clip.x_lower + window_w;
	font->render.draw_text(str, rect.x + text_x - offset, rect.y + text_y, clip);

	float caret_y = (float)caret_off_y * height;
	float caret_w = (float)caret_width * height;

	Rect_Fixed r = {
		rect.x + text_x + cur_x - offset,
		rect.y + text_y + (int)caret_y + gap_y,
		(int)caret_w,
		(int)height
	};

	sdl_draw_rect(r, caret);
}

bool Drop_Down::mouse_handler(Camera& view, Input& input, Point& inside, bool hovered) {
	if (pos.contains(inside)) {
		if (input.lclick || parent->current_dd)
			parent->set_dropdown(this);
	}

	return false;
}

bool Drop_Down::highlight(Camera& view, Point& inside) {
	sel = -1;
	if (!dropped || inside.x < pos.x || inside.x >= pos.x + width)
		return false;

	float font_unit = font->render.text_height() / view.scale;
	float y = pos.y + pos.h + title_off_y * font_unit;
	float h = line_height * font_unit;

	for (int i = 0; i < lines.size(); i++) {
		if (inside.y >= y && inside.y < y + h) {
			sel = i;
			return true;
		}
		y += h;
	}

	return false;
}

void Drop_Down::draw_menu(Camera& view, Rect_Fixed& rect, bool held) {
	float font_height = font->render.text_height();

	Rect box_wsp = {
		pos.x,
		pos.y + pos.h,
		width,
		0
	};
	Rect box = make_ui_box(rect, box_wsp, view.scale);
	box.h = (title_off_y + line_height * lines.size()) * font_height;

	sdl_draw_rect(box, hl_color);

	if (sel >= 0) {
		Rect hl = box;
		hl.y += line_height * sel * font_height;
		hl.h = (title_off_y + line_height) * font_height + 1;
		sdl_draw_rect(hl, sel_color);
	}

	float hack = 3;
	float item_x = item_off_x * font_height;
	float item_w = (width - hack) * view.scale - item_x;
	item_clip.x_upper = box.x + item_x + item_w;

	float y = font_height * ((line_height - 1) / 2 - font->line_offset);

	for (auto& l : lines) {
		if (l)
			font->render.draw_text(l, box.x + item_x, box.y + y, item_clip);

		y += font_height * line_height;
	}
}

void Drop_Down::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);

	if (elem_hovered || dropped)
		sdl_draw_rect(box, hl_color);
	else
		sdl_draw_rect(box, default_color);

	float gap_y = title_off_y * font->render.text_height();

	float text_w = font->render.text_width(title);
	float text_x = pos.x * view.scale + (pos.w * view.scale - text_w) / 2;
	float text_y = pos.y * view.scale + gap_y;

	font->render.draw_text_simple(title, rect.x + text_x, rect.y + text_y);
}

void Hex_View::set_region(u64 address, u64 size) {
	region_address = address;
	region_size = size;
	alive = true;
}

void Hex_View::update(float scale) {
	float font_height = font->render.text_height();
	rows = pos.h * scale / font_height;

	if (vscroll) {
		offset = (int)vscroll->position;
		offset -= offset % columns;
		vscroll->set_maximum(region_size, rows * columns);
	}

	if (alive)
		span_idx = source->reset_span(span_idx, region_address + offset, rows * columns);
}

void Hex_View::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(box, default_color);

	if (!alive || span_idx < 0)
		return;

	Rect_Fixed src, dst;

	float font_height = font->render.text_height();
	float x_start = 4 * view.scale;
	float x = x_start;
	float y = font_height;
	float padding = font_height / 4;

	float row_height = rows > 0 ? (box.h - padding) / rows : font_height;

	int row_digits = columns * 2;
	auto& span = source->spans[span_idx];

	bool clip = false;
	for (int i = 0; i < span.size * 2; i++) {
		Glyph *gl = nullptr;
		int digit = 0;
		if (i < span.retrieved * 2) {
			digit = span.cache[i / 2];
			if (i % 2 == 0)
				digit >>= 4;
			digit &= 0xf;

			char c = digit <= 9 ? '0' + digit : 'a' - 10 + digit;
			gl = font->render.glyph_for(c);
		}
		else
			gl = font->render.glyph_for('?');

		if (!clip) {
			src.x = gl->atlas_x;
			src.y = gl->atlas_y;
			src.w = gl->img_w;
			src.h = gl->img_h;

			float clip_x = x + padding + gl->left;
			if (clip_x + src.w > box.w)
				src.w = box.w - clip_x;

			dst.x = box.x + x + gl->left;
			dst.y = box.y + y - gl->top;
			dst.w = src.w;
			dst.h = src.h;

			sdl_apply_texture(font->render.tex, &dst, &src);
		}

		if (i % row_digits == row_digits-1) {
			x = x_start;
			y += row_height;
			clip = false;
		}
		else {
			x += gl->box_w;
			if (i % 2 == 1)
				x += padding;
		}

		if (x >= box.w)
			clip = true;
	}
}

bool Hex_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (vscroll && hovered && pos.contains(cursor)) {
		int delta = columns;
		if (input.lshift || input.rshift)
			delta *= rows;

		if (input.scroll_y > 0)
			vscroll->scroll(-delta);
		if (input.scroll_y < 0)
			vscroll->scroll(delta);
	}

	return false;
}
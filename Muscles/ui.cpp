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

void set_active_edit(UI_Element *elem, bool dbl_click) {
	elem->parent->active_edit = elem;
}
void (*get_set_active_edit(void))(UI_Element*, bool) {
	return set_active_edit;
}

void delete_box(UI_Element *elem, bool dbl_click) {
	elem->parent->parent->delete_box(elem->parent);
}
void (*get_delete_box(void))(UI_Element*, bool) {
	return delete_box;
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

void Button::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	held = input.lmouse && pos.contains(cursor);
}

void Button::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect r = make_ui_box(rect, pos, view.scale);

	float pad = padding * view.scale;
	r.x += pad;
	r.y += pad;
	r.w -= pad * 2;
	r.h -= pad * 2;

	Theme *theme = active ? &active_theme : &inactive_theme;

	RGBA *color = &theme->back;
	if (held)
		color = &theme->held;
	else if (elem_hovered)
		color = &theme->hover;

	sdl_draw_rect(r, *color);

	if (!theme->font) {
		if (icon)
			sdl_apply_texture(icon, r);
		return;
	}

	float font_height = theme->font->render.text_height();

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

		sdl_apply_texture(icon, box);
	}

	theme->font->render.draw_text_simple(text.c_str(), r.x + x, r.y + y_offset * font_height);
}

void Checkbox::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	if (elem_hovered) {
		Rect back = make_ui_box(rect, pos, view.scale);
		sdl_draw_rect(back, hl_color);
	}

	Rect box = make_ui_box(rect, pos, view.scale);
	box.w = box.h;
	sdl_draw_rect(box, default_color);

	if (checked) {
		int w = 0.5 + box.w * border_frac;
		int h = 0.5 + box.h * border_frac;
		Rect check = {
			box.x + w,
			box.y + h,
			box.w - 2*w,
			box.h - 2*h
		};
		sdl_draw_rect(check, sel_color);
	}

	float font_height = font->render.text_height();
	float gap_x = text_off_x * font_height;
	float gap_y = text_off_y * font_height;

	float text_x = box.x + box.w + gap_x;
	float text_y = box.y + gap_y;

	font->render.draw_text_simple(text.c_str(), text_x, text_y);
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

	int n_rows = data.filtered >= 0 ? data.filtered : data.row_count();
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

float Data_View::column_width(float total_width, float min_width, float font_height, float scale, int idx) {
	float w = data.headers[idx].width;
	if (w == 0)
		return (total_width - min_width) * scale;

	w *= total_width * scale;

	if (data.headers[idx].max_size > 0) {
		float max = data.headers[idx].max_size * font_height;
		w = w < max ? w : max;
	}

	if (data.headers[idx].min_size > 0) {
		float min = data.headers[idx].min_size * font_height;
		w = w > min ? w : min;
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

	int n_items = data.filtered;
	if (n_items < 0)
		n_items = n_rows;

	if (vscroll) {
		top = vscroll->position;
		vscroll->set_maximum(n_items, n_visible);
	}

	float scroll_x = 0;
	float scroll_total = column_spacing;
	float table_w = pos.w;

	if (hscroll) {
		for (int i = 0; i < n_cols; i++)
			scroll_total += data.headers[i].min_size + column_spacing;
		scroll_total *= font_height / view.scale;

		hscroll->set_maximum(scroll_total, pos.w);

		scroll_x = scroll_total - pos.w;
		if (scroll_x < 0) scroll_x = 0;

		scroll_x = hscroll->position - scroll_x;
		table_w = hscroll->maximum;
	}
	scroll_x *= view.scale;

	float col_space = column_spacing * font_height;

	Rect back = make_ui_box(rect, table, view.scale);

	if (show_column_names) {
		float x = back.x;
		float y = back.y - (header_height * view.scale + font->line_offset * font_height);

		for (int i = 0; i < n_cols; i++) {
			font->render.draw_text_simple(data.headers[i].name, x, y);

			float w = column_width(pos.w, scroll_total, font_height, view.scale, i);
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

		sdl_apply_texture(t, dst, &src);
	};

	float pad = padding * view.scale;
	float x = back.x + col_space - scroll_x;
	float y = back.y;
	float x_max = back.x + back.w - col_space;
	float y_max = y + table.h * view.scale;

	float line_off = font->line_offset * font_height;
	float line_h = font_height + (font->line_spacing * font_height);
	float sp_half = font->line_spacing * font_height / 2;

	Rect_Fixed dst = {0, 0, font_height, font_height};
	Rect_Fixed src = {0};

	clip.x_lower = x + scroll_x;
	clip.y_upper = y_max;

	float table_off_y = y;

	for (int i = 0; i < n_cols && x < back.x + back.w; i++) {
		float w = column_width(pos.w, scroll_total, font_height, view.scale, i);
		clip.x_upper = x+w < x_max ? x+w : x_max;

		Type type = data.headers[i].type;

		if (data.filtered >= 0) {
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

void Data_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (hovered) {
		if (consume_box_scroll || pos.contains(cursor)) {
			Scroll *scroll = input.lshift || input.rshift ? hscroll : vscroll;
			if (scroll && input.scroll_y > 0)
				scroll->scroll(-1);
			if (scroll && input.scroll_y < 0)
				scroll->scroll(1);
		}
	}
}

void Data_View::deselect() {
	hl_row = -1;
}

void Data_View::release() {
	data.release();
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

void Divider::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!moveable)
		return;

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
		return;
	}
	icon = icon_hl;

	sdl_set_cursor(cursor_type);
	parent->parent->cursor_set = true;

	if (!input.lmouse) {
		held = false;
		return;
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
		sdl_apply_texture(icon, box);
	}
}

void Drop_Down::mouse_handler(Camera& view, Input& input, Point& inside, bool hovered) {
	if (hovered) {
		if (pos.contains(inside)) {
			if (input.lclick) {
				if (this == parent->current_dd)
					parent->set_dropdown(nullptr);
				else if (!parent->current_dd)
					parent->set_dropdown(this);
			}
			else if (parent->current_dd)
				parent->set_dropdown(this);
		}
		else if (sel >= 0)
			parent->set_dropdown(this);
	}
}

bool Drop_Down::highlight(Camera& view, Point& inside) {
	sel = -1;
	if (!dropped || inside.x < pos.x || inside.x >= pos.x + width)
		return false;

	float font_unit = font->render.text_height() / view.scale;
	float y = pos.y + pos.h + title_off_y * font_unit;
	float h = line_height * font_unit;

	auto lines = get_content();
	for (int i = 0; i < lines->size(); i++) {
		if (inside.y >= y && inside.y < y + h) {
			sel = i;
			return true;
		}
		y += h;
	}

	return false;
}

void Drop_Down::draw_menu(Camera& view, Rect_Fixed& rect) {
	float font_height = font->render.text_height();
	auto lines = get_content();

	Rect box_wsp = {
		pos.x,
		pos.y + pos.h,
		width,
		0
	};
	Rect box = make_ui_box(rect, box_wsp, view.scale);
	box.h = (title_off_y + line_height * lines->size()) * font_height;

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

	for (auto& l : *lines) {
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

bool Edit_Box::disengage(bool try_select) {
	float cancel_lclick = false;
	if (dropdown) {
		if (try_select && dropdown->sel >= 0) {
			line = (*dropdown->get_content())[dropdown->sel];
			cursor = 0;
			offset = 0;
			cancel_lclick = true;
		}
		dropdown->dropped = false;
	}
	return cancel_lclick;
}

void Edit_Box::key_handler(Camera& view, Input& input) {
	update = false;
	if (this != parent->active_edit)
		return;

	int delta = 0;
	if (input.strike(input.back)) {
		update = remove(true);
		delta = update ? -1 : 0;
	}
	else if (input.strike(input.del))
		update = remove(false);
	else if (input.strike(input.esc)) {
		clear();
		disengage(false);
		parent->active_edit = nullptr;
		update = true;
	}
	else if (input.strike(input.enter))
		update = true;
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

	if (update && key_action)
		key_action(this, input);

	if (delta) {
		float x = 0;
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

void Edit_Box::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered)
		return;

	float w = pos.h;
	float x = icon_right ? pos.x + pos.w - w : pos.x;
	bool on_icon = icon && cursor.y >= pos.y && cursor.y < pos.y + pos.h && cursor.x >= x && cursor.x - x < w;
	use_default_cursor = on_icon;

	if (dropdown) {
		dropdown->highlight(view, cursor);
		if (input.lclick && on_icon)
			dropdown->dropped = !dropdown->dropped;
	}
}

void Edit_Box::update_icon(IconType type, float height, float scale) {
	pos.h = height;
	icon_length = 0.5 + height * scale - 2 * text_off_y * font->render.text_height();

	switch (type) {
		case IconGoto:
			icon = make_goto_icon(icon_color, icon_length);
			break;
		case IconGlass:
			icon = make_glass_icon(icon_color, icon_length);
			break;
		case IconTriangle:
			icon = make_triangle(icon_color, icon_length, icon_length);
			break;
	}
}

void Edit_Box::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect back = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(back, default_color);

	const char *str = nullptr;
	Font *fnt = font;

	if (ph_font && line.size() == 0) {
		fnt = ph_font;
		str = placeholder.c_str();
	}
	else
		str = get_str();

	float cur_x;
	fnt->render.text_width(str, cursor, &cur_x);

	float height = fnt->render.text_height();
	float gap_x = text_off_x * height;
	float gap_y = text_off_y * height;

	float x = gap_x;
	float icon_w = 0;
	if (icon) {
		Rect dst = {back.x + gap_y, back.y + gap_y, (float)icon_length, (float)icon_length};
		if (icon_right) {
			icon_w = dst.w;
			dst.x = back.x + back.w - gap_y - icon_w;
		}
		else {
			x += icon_length;
		}
		sdl_apply_texture(icon, dst);
	}

	float text_x = pos.x * view.scale + x;
	float text_y = pos.y * view.scale + gap_y;
	window_w = (pos.w * view.scale) - x - icon_w - gap_x;

	clip.x_lower = rect.x + text_x;
	clip.x_upper = clip.x_lower + window_w;
	fnt->render.draw_text(str, rect.x + text_x - offset, rect.y + text_y, clip);

	if (this == parent->active_edit) {
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

	if (!dropdown)
		return;

	dropdown->pos = pos;
	if (dropdown->dropped)
		dropdown->draw_menu(view, rect);
}

void Hex_View::set_region(u64 address, u64 size) {
	region_address = address;
	region_size = size;
	sel = -1;
	alive = true;
}

void Hex_View::update(float scale) {
	float font_height = font->render.text_height();
	rows = pos.h * scale / font_height;

	if (scroll) {
		offset = (int)scroll->position;
		offset -= offset % columns;
		u64 size = region_size;
		if (size % columns > 0)
			size += columns - (size % columns);

		scroll->set_maximum(size, rows * columns);
	}

	addr_digits = count_digits(region_address + region_size - 1);

	if (alive)
		span_idx = source->reset_span(span_idx, region_address + offset, rows * columns);
}

float Hex_View::print_address(u64 address, float x, float y, float digit_w, Render_Clip& clip, Rect& box, float pad) {
	int n_digits = count_digits(address);
	float text_x = x + (addr_digits - n_digits) * digit_w;

	char buf[20];
	print_hex("0123456789abcdef", buf, address, n_digits);
	font->render.draw_text(buf, box.x + text_x, box.y + y, clip);

	return x + addr_digits * digit_w + 2*pad;
}

float Hex_View::print_hex_row(Span& span, int idx, float x, float y, Rect& box, float pad) {
	Rect_Fixed src, dst;

	for (int i = 0; i < columns * 2; i++) {
		const Glyph *gl = nullptr;
		int digit = 0;
		if (i < span.retrieved * 2) {
			digit = span.cache[idx + i/2];
			if (i % 2 == 0)
				digit >>= 4;
			digit &= 0xf;

			char c = digit <= 9 ? '0' + digit : 'a' - 10 + digit;
			gl = font->render.glyph_for(c);
		}
		else
			gl = font->render.glyph_for('?');

		src.x = gl->atlas_x;
		src.y = gl->atlas_y;
		src.w = gl->img_w;
		src.h = gl->img_h;

		float clip_x = x + pad + gl->left;
		if (clip_x + src.w > box.w)
			src.w = box.w - clip_x;

		dst.x = box.x + x + gl->left;
		dst.y = box.y + y - gl->top;
		dst.w = src.w;
		dst.h = src.h;

		sdl_apply_texture(font->render.tex, dst, &src);

		x += gl->box_w;
		if (i % 2 == 1)
			x += pad;

		if (x >= box.w)
			break;
	}

	return x + pad;
}

void Hex_View::print_ascii_row(Span& span, int idx, float x, float y, Render_Clip& clip, Rect& box) {
	char *ascii = (char*)alloca(columns + 1);
	for (int i = 0; i < columns; i++) {
		char c = '?';
		if (idx+i < span.retrieved) {
			c = (char)span.cache[idx+i];
			if (c < ' ' || c > '~')
				c = '.';
		}
		ascii[i] = c;
	}
	ascii[columns] = 0;

	font->render.draw_text(ascii, box.x + x, box.y + y, clip);
}

void Hex_View::draw_cursors(int idx, Rect& back, float x_start, float pad, float scale) {
	int row = idx / columns;
	int col = idx % columns;

	float cur_w = cursor_width * scale;
	float digit_w = font->render.digit_width();
	float byte_w = 2 * digit_w + pad;
	float hex_w = (float)columns * byte_w;

	float x = back.x + x_start;
	float hex_x = x + (float)col * byte_w;
	float ascii_x = x + (float)col * digit_w;
	if (show_hex)
		ascii_x += hex_w + pad;

	float x_max = back.x + back.w - cur_w;
	float y = back.y + (float)row * row_height + (pad / 2);

	if (show_hex && hex_x < x_max) {
		Rect cursor = {
			hex_x,
			y,
			cur_w,
			row_height
		};
		sdl_draw_rect(cursor, caret);
	}
	if (show_ascii && ascii_x < x_max) {
		Rect cursor = {
			ascii_x,
			y,
			cur_w,
			row_height
		};
		sdl_draw_rect(cursor, caret);
	}
}

void Hex_View::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);

	if (!alive || span_idx < 0 || rows <= 0 || columns <= 0) {
		sdl_draw_rect(box, default_color);
		return;
	}

	Rect_Fixed src, dst;

	font_height = font->render.text_height();

	float x_start = x_offset * view.scale;
	float x = x_start;
	float y = font_height;
	float pad = padding * font_height;

	float digit_w = font->render.digit_width();
	float addr_w = addr_digits * digit_w + 2*pad;

	Rect back = box;
	if (show_addrs) {
		back.x += addr_w;
		back.w -= addr_w;
	}
	sdl_draw_rect(back, default_color);

	Render_Clip addr_clip = {
		CLIP_RIGHT, 0, 0, box.x + addr_w - pad, 0
	};
	Render_Clip ascii_clip = {
		CLIP_RIGHT, 0, 0, box.x + box.w - pad, 0
	};

	row_height = rows > 0 ? (box.h - pad) / (float)rows : font_height;

	auto& span = source->spans[span_idx];
	int left = span.size;

	while (left > 0) {
		int idx = span.size - left;

		if (show_addrs)
			x = print_address(region_address + offset + idx, x, y - font_height, digit_w, addr_clip, box, pad);
		if (show_hex && x < box.w)
			x = print_hex_row(span, idx, x, y, box, pad);
		if (show_ascii && x < box.w)
			print_ascii_row(span, idx, x, y - font_height, ascii_clip, box);

		x = x_start;
		y += row_height;
		left -= columns;
	}

	if (sel >= 0 && sel >= offset && sel < offset + span.size)
		draw_cursors(sel - offset, back, x_start, pad, view.scale);
}

void Hex_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered || !pos.contains(cursor))
		return;

	if (scroll) {
		int delta = columns;
		if (input.lshift || input.rshift)
			delta *= rows;

		if (input.scroll_y > 0)
			scroll->scroll(-delta);
		if (input.scroll_y < 0)
			scroll->scroll(delta);
	}

	if (input.lclick) {
		sel = -1;

		float font_h = font_height / view.scale;
		float pad = padding * font_h;
		float row_height = rows > 0 ? (pos.h - pad) / (float)rows : font_h;

		float digit_w = font->render.digit_width() / view.scale;
		float byte_w = 2*digit_w + pad;
		float hex_w = (float)columns * byte_w;

		float x_start = x_offset;
		if (show_addrs)
			x_start += addr_digits * digit_w + 2*pad;

		float x = cursor.x - pos.x - x_start + (pad / 2);
		float y = cursor.y - pos.y - (pad / 2);

		if (x >= 0 && x < hex_w) {
			int row = y / row_height;
			int col = x / byte_w;
			sel = offset + (row * columns) + col;
		}
	}
}

void Image::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);
	sdl_apply_texture(img, box);
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

void Scroll::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!show_thumb)
		return;

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

void Text_Editor::erase(Cursor& cursor, bool is_back) {
	if (is_back) {
		if (cursor.cursor <= 0)
			return;

		set_cursor(cursor, cursor.cursor - 1);
	}
	else if (cursor.cursor >= text.size())
		return;

	text.erase(cursor.cursor, 1);
}

// Used for left/right arrow movement and when adding/removing characters
void Text_Editor::set_cursor(Cursor& cursor, int cur) {
	int c = 0;
	int l = 0;
	int i, len = text.size();

	for (i = 0; i < cur && i < len; i++) {
		if (text[i] == '\t')
			c += tab_width;
		else
			c++;
			
		if (text[i] == '\n') {
			c = 0;
			l++;
		}
	}

	cursor.line = l;
	cursor.column = c;
	cursor.target_column = c;
	cursor.cursor = i;
}

// Used for up/down arrow movement and mouse selection on y-axis
void Text_Editor::set_line(Cursor& cursor, int line_idx) {
	if (cursor.line == line_idx)
		return;

	int l = 0;
	int c = 0;
	int len = text.size();
	int prev_col_end = 0;
	int last_nl = 0;

	bool reached_target = false;
	int idx = 0;
	for (int i = 0; i < len; i++) {
		if (l == line_idx && c >= cursor.target_column) {
			cursor.column = c;
			reached_target = true;
			idx = i;
			break;
		}
		if (l > line_idx && line_idx >= 0) {
			cursor.column = prev_col_end;
			l--;
			idx = i-1;
			reached_target = true;
			break;
		}

		if (text[i] == '\t')
			c += tab_width;
		else
			c++;

		if (text[i] == '\n') {
			l++;
			last_nl = i+1;
			prev_col_end = c-1;
			c = 0;
		}
	}

	if (!reached_target) {
		int end = 0;
		int chars = 0;

		for (int i = last_nl; i < len; i++) {
			if (end < cursor.column)
				chars++;

			end += text[i] == '\t' ? tab_width : 1;
		}

		if (cursor.column > end || cursor.target_column >= end)
			cursor.column = end;

		cursor.cursor = last_nl + chars;
	}
	else
		cursor.cursor = idx;

	cursor.line = l;
}

// Used for home/end movement and mouse selection on x-axis
void Text_Editor::set_column(Cursor& cursor, int col_idx) {
	int len = text.size();
	int start = 0, end = 0;
	bool past_end = true;
	int l = 0;

	for (int i = 0; i < len; i++) {
		if (text[i] != '\n')
			continue;

		if (end) start = end+1;
		end = i;

		if (l >= cursor.line) {
			past_end = false;
			break;
		}

		l++;
	}
	if (past_end) {
		if (end) start = end+1;
		end = len;
	}

	int max_cols = 0;
	int cols = 0;
	int chars = 0;
	int line_len = end - start;

	bool found_column = false;
	for (int i = 0; i < line_len; i++) {
		if (!found_column && max_cols >= col_idx) {
			cols = max_cols;
			chars = i;
			found_column = true;
		}

		max_cols += text[start+i] == '\t' ? tab_width : 1;
	}
	if (!found_column) {
		cols = max_cols;
		chars = line_len;
	}

	if (col_idx < 0 || col_idx > max_cols) {
		col_idx = max_cols;
		chars = line_len;
	}
	else
		col_idx = cols;

	cursor.column = cursor.target_column = col_idx;
	cursor.line = l;
	cursor.cursor = start + chars;
}

bool is_word_char(char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool is_space(char c) {
	return c == ' ' || c == '\t';
}

int previous_word(std::string& text, int cur) {
	if (cur > 0) cur--;
	int old = cur;

	while (cur > 0 && is_space(text[cur]))
		cur--;
	while (cur > 0 && is_word_char(text[cur]))
		cur--;
	if (old > cur && cur < text.size() && (text[cur] == ' ' || text[cur] == '\t'))
		cur++;

	return cur;
}

int next_word(std::string& text, int cur, bool selecting) {
	bool skip_word = true;
	if (is_space(text[cur])) {
		skip_word = selecting;
		while (cur < text.size() && is_space(text[cur]))
			cur++;
	}
	if (skip_word) {
		cur++;
		while (cur < text.size() && is_word_char(text[cur]))
			cur++;
	}

	if (!selecting) {
		while (cur < text.size() && (text[cur] == ' ' || text[cur] == '\t'))
			cur++;
	}

	return cur;
}

void Text_Editor::key_handler(Camera& view, Input& input) {
	if (this != parent->active_edit)
		return;

	char ch = 0;
	bool ctrl = input.lctrl || input.rctrl;
	bool shift = input.lshift || input.rshift;
	bool end_selection = true;
	bool erased = false;

	if (input.strike(input.back)) {
		if (selected && primary.cursor == secondary.cursor)
			selected = false;

		if (!selected) {
			if (ctrl) {
				int end = primary.cursor;
				int cur = primary.cursor - 1;
				if (shift) {
					shift = false;
					while (cur > 0 && text[cur] != '\n')
						cur--;
					if (text[cur] == '\n')
						cur++;
				}
				else
					cur = previous_word(text, primary.cursor);

				if (cur < 0) cur = 0;
				text.erase(cur, end - cur);
				set_cursor(primary, cur);
			}
			else
				erase(primary, true);
		}
		erased = true;
	}
	else if (input.strike(input.del)) {
		if (selected && primary.cursor == secondary.cursor)
			selected = false;

		if (!selected) {
			if (ctrl) {
				int start = primary.cursor;
				int cur = primary.cursor + 1;
				if (shift) {
					shift = false;
					while (cur <= text.size() && text[cur] != '\n')
						cur++;
				}
				else
					cur = next_word(text, primary.cursor, false);

				text.erase(start, cur - start);
				set_cursor(primary, start);
			}
			else
				erase(primary, false);
		}
		erased = true;
	}
	else if (input.strike(input.esc))
		parent->active_edit = nullptr;
	else if (input.strike(input.enter))
		ch = '\n';
	else if (input.strike(input.tab))
		ch = '\t';
	else if (input.strike(input.home)) {
		if (ctrl)
			set_line(primary, 0);
		set_column(primary, 0);
	}
	else if (input.strike(input.end)) {
		if (ctrl)
			set_line(primary, -1);
		set_column(primary, -1);
	}
	else if (input.strike(input.left)) {
		int cur = primary.cursor - 1;
		if (ctrl)
			cur = previous_word(text, primary.cursor);

		set_cursor(primary, cur);
	}
	else if (input.strike(input.right)) {
		int cur = primary.cursor + 1;
		if (ctrl)
			cur = next_word(text, primary.cursor, shift);

		set_cursor(primary, cur);
	}
	else if (input.strike(input.up) && !ctrl) {
		int l = primary.line - 1;
		if (l < 0) l = 0;
		set_line(primary, l);
	}
	else if (input.strike(input.down) && !ctrl) {
		set_line(primary, primary.line + 1);
	}
	else if (input.ch && !ctrl)
		ch = input.ch;
	else
		end_selection = false;

	if (end_selection) {
		bool expunge = selected && (erased || ch);
		if (expunge && secondary.cursor != primary.cursor) {
			int start, len;
			if (primary.cursor < secondary.cursor) {
				start = primary.cursor;
				len = secondary.cursor - primary.cursor;
			}
			else {
				start = secondary.cursor;
				len = primary.cursor - secondary.cursor;
				primary = secondary;
			}

			text.erase(start, len);
		}
		selected = false;
		ticks = 0;
	}

	if (ch) {
		text.insert(text.begin() + primary.cursor, ch);
		set_cursor(primary, primary.cursor + 1);
	}
	else if (input.lmouse || shift)
		selected = true;

	if (!selected)
		secondary = primary;
}

void Text_Editor::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (hovered && input.lclick && pos.contains(cursor)) {
		parent->ui_held = true;
		parent->active_edit = this;
		mouse_held = true;
	}
	if (!input.lmouse)
		mouse_held = false;

	if (mouse_held) {
		float x = (cursor.x - pos.x) * view.scale;
		float y = (cursor.y - pos.y) * view.scale;

		float edge = border * view.scale;
		float gl_w = font->render.digit_width();
		float gl_h = font->render.text_height();

		int line = (y - edge) / gl_h;
		int col = (x - edge) / gl_w + 0.5;
		if (line < 0) line = 0;
		if (col < 0) col = 0;

		set_line(primary, line);
		set_column(primary, col);

		if (input.lclick)
			secondary = primary;
	}
}

void Text_Editor::draw_selection_box(Render_Clip& clip, float digit_w, float font_h, float line_pad) {
	Cursor *first  = primary.cursor < secondary.cursor ? &primary : &secondary;
	Cursor *second = primary.cursor < secondary.cursor ? &secondary : &primary;

	float y = clip.y_lower + line_pad;
	Rect r = {0};
	if (first->line == second->line) {
		r.x = clip.x_lower + first->column * digit_w;
		r.y = y + first->line * font_h;
		r.w = (second->column - first->column) * digit_w;
		r.h = font_h + 1;
		sdl_draw_rect(r, sel_color);
	}
	else {
		float line_w = clip.x_upper - clip.x_lower;

		float x = first->column * digit_w;
		r.x = clip.x_lower + x;
		r.y = y + first->line * font_h;
		r.w = line_w - x;
		r.h = font_h + 1;
		sdl_draw_rect(r, sel_color);

		int gulf = second->line - first->line - 1;
		if (gulf > 0) {
			r.x = clip.x_lower;
			r.y += font_h;
			r.w = line_w;
			r.h = gulf * font_h + 1;
			sdl_draw_rect(r, sel_color);
		}

		if (second->column > 0) {
			r.x = clip.x_lower;
			r.y = y + second->line * font_h;
			r.w = second->column * digit_w;
			r.h = font_h + 1;
			sdl_draw_rect(r, sel_color);
		}
	}
}

void Text_Editor::draw_cursor(Cursor& cursor, Rect& back, float digit_w, float font_h, float line_pad, float edge, float scale) {
	float caret_x = back.x + edge + ((float)cursor.column * digit_w);
	float caret_y = back.y + edge + line_pad + ((float)cursor.line * font_h) + (line_pad / 2);

	Rect caret = { caret_x, caret_y, cursor_width * scale, font_h - line_pad };
	sdl_draw_rect(caret, caret_color);
}

void Text_Editor::draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect back = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(back, default_color);

	float scroll_x = hscroll ? hscroll->position : 0;
	float scroll_y = vscroll ? vscroll->position : 0;

	float edge = border * view.scale;

	clip.x_lower = back.x + edge;
	clip.x_upper = back.x + back.w - edge;
	clip.y_lower = back.y + edge;
	clip.y_upper = back.y + back.h - edge;

	float digit_w = font->render.digit_width();
	float font_h = font->render.text_height();
	float line_pad = font_h * 0.15f;

	if (secondary.cursor != primary.cursor)
		draw_selection_box(clip, digit_w, font_h, line_pad);

	font->render.draw_text(text.c_str(), back.x + edge - scroll_x, back.y + edge - scroll_y, clip, tab_width, true);

	if (this != parent->active_edit)
		return;

	ticks++;
	if (secondary.cursor != primary.cursor || ticks % (caret_on_time + caret_off_time) < caret_on_time) {
		draw_cursor(primary, back, digit_w, font_h, line_pad, edge, view.scale);
		if (secondary.cursor != primary.cursor)
			draw_cursor(secondary, back, digit_w, font_h, line_pad, edge, view.scale);
	}
}
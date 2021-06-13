#include "muscles.h"
#include "ui.h"

void *allocate_ui_element(Arena& arena, int i_type) {
	auto type = static_cast<Element_Type>(i_type);

	// if there's a better way to do this i'm all ears
	#define TRY_UI_TYPE(type) \
		case Elem_##type:\
			return arena.allocate_object<type>();

	switch (type) {
		TRY_UI_TYPE(Image)
		TRY_UI_TYPE(Label)
		TRY_UI_TYPE(Button)
		TRY_UI_TYPE(Divider)
		TRY_UI_TYPE(Data_View)
		TRY_UI_TYPE(Scroll)
		TRY_UI_TYPE(Edit_Box)
		TRY_UI_TYPE(Drop_Down)
		TRY_UI_TYPE(Checkbox)
		TRY_UI_TYPE(Hex_View)
		TRY_UI_TYPE(Text_Editor)
		TRY_UI_TYPE(Tabs)
		TRY_UI_TYPE(Number_Edit)
	}

	return nullptr;
}

static inline float clamp_f(float f, float min, float max) {
	if (f < min)
		return min;
	if (f > max && max >= min)
		return max;
	return f;
}

Rect_Int make_int_ui_box(Rect_Int& box, Rect& elem, float scale) {
	return {
		.x = (int)(0.5 + (float)box.x + elem.x * scale),
		.y = (int)(0.5 + (float)box.y + elem.y * scale),
		.w = (int)(0.5 + elem.w * scale),
		.h = (int)(0.5 + elem.h * scale)
	};
}

Rect make_ui_box(Rect_Int& box, Rect& elem, float scale) {
	return {
		.x = (float)box.x + elem.x * scale,
		.y = (float)box.y + elem.y * scale,
		.w = elem.w * scale,
		.h = elem.h * scale
	};
}

void reposition_box_buttons(Image& cross, Image& maxm, float box_w, float size) {
	cross.pos = {
		box_w - size * 1.5f,
		size * 0.5f,
		size,
		size
	};
	maxm.pos = {
		cross.pos.x - size * 1.75f,
		size * 0.5f,
		size,
		size
	};
}

void delete_box(UI_Element *elem, Camera& view, bool dbl_click) {
	elem->parent->parent->delete_box(elem->parent);
}
void (*get_delete_box(void))(UI_Element*, Camera&, bool) { return delete_box; }

void maximize_box(UI_Element *elem, Camera& view, bool dbl_click) {
	Box *box = elem->parent;

	float center_x = view.center_x;
	float center_y = view.center_y;

	center_x *= 0.9;
	center_y *= 0.9;

	box->box = {
		view.x - center_x / view.scale,
		view.y - center_y / view.scale,
		2 * center_x / view.scale,
		2 * center_y / view.scale,
	};

	box->update_ui(view);
}
void (*get_maximize_box(void))(UI_Element*, Camera&, bool) { return maximize_box; }

void UI_Element::draw(Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	ticks++;
	update(view, rect);

	if (!use_sf_cache) {
		Rect_Int back = make_int_ui_box(rect, pos, view.scale);
		screen = back;
		draw_element(nullptr, view, back, elem_hovered, box_hovered, focussed);
		return;
	}

	int w = 0.5 + pos.w * view.scale;
	int h = 0.5 + pos.h * view.scale;
	Rect_Int back = {0, 0, w, h};

	bool draw_existing = false;
	Renderer renderer = nullptr;
	Renderer hw = sdl_get_hw_renderer();
	bool soft_draw = false;

	if (needs_redraw || w != old_width || h != old_height) {
		reify_timer = 0;
		needs_redraw = false;
		old_width = w;
		old_height = h;

		if (tex_cache)
			sdl_destroy_texture(&tex_cache);

		renderer = hw;
	}
	else {
		reify_timer++;
		if (reify_timer < hw_draw_timeout) {
			renderer = hw;
		}
		else if (!tex_cache) {
			renderer = sdl_acquire_sw_renderer(w, h);
			soft_draw = renderer != nullptr;
		}
		else {
			// this means that we don't need to render a new frame and can simply use the existing surface instead
			draw_existing = true;
		}
	}

	if (!draw_existing) {
		if (!soft_draw) {
			back = make_int_ui_box(rect, pos, view.scale);
			screen = back;
		}

		draw_element(renderer, view, back, elem_hovered, box_hovered, focussed);

		if (soft_draw) {
			if (tex_cache)
				sdl_destroy_texture(&tex_cache);

			tex_cache = sdl_bake_sw_render();
			draw_existing = true;
		}

		if (use_sf_cache)
			needs_redraw = false;
	}

	Rect_Int r = make_int_ui_box(rect, pos, view.scale);

	if (draw_existing)
		sdl_apply_texture(tex_cache, r, nullptr, nullptr);
}

void UI_Element::set_active(bool state) {
	needs_redraw = active != state;
	active = state;
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

int Button::get_icon_length(float scale) {
	Theme *theme = active ? &active_theme : &inactive_theme;
	if (theme->font)
		return (int)theme->font->render.text_height();

	int w = pos.w * scale + 0.5;
	w -= 2 * padding * scale;
	return w;
}

void Button::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	bool was_held = held;
	held = input.lmouse && pos.contains(cursor);
	needs_redraw = was_held != held;
}

void Button::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	float pad = padding * view.scale;
	back.x += pad;
	back.y += pad;
	back.w -= pad * 2;
	back.h -= pad * 2;

	Theme *theme = active ? &active_theme : &inactive_theme;

	RGBA *color = &theme->back;
	if (held)
		color = &theme->held;
	else if (elem_hovered)
		color = &theme->hover;

	sdl_draw_rect(back, *color, renderer);

	int icon_length = get_icon_length(view.scale);

	if (!theme->font) {
		back.w = icon_length;
		back.h = icon_length;
		if (icon)
			sdl_apply_texture(icon, back, nullptr, renderer);

		return;
	}

	float font_height = theme->font->render.text_height();

	float x = x_offset * font_height;
	float y = y_offset * font_height;

	if (icon) {
		Rect box = {(float)back.x, (float)back.y, (float)icon_length, (float)icon_length};

		if (icon_right)
			box.x += (float)back.w - x - font_height;
		else {
			box.x += x;
			x += font_height * 1.2f;
		}

		sdl_apply_texture(icon, box, nullptr, renderer);
	}

	theme->font->render.draw_text_simple(renderer, text.c_str(), back.x + x, back.y + y);
}

void Checkbox::toggle() {
	checked = !checked;
	needs_redraw = true;
}

void Checkbox::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (elem_hovered)
		sdl_draw_rect(back, hl_color, renderer);

	float font_height = font->render.text_height();
	float gap_x = text_off_x * font_height;
	float gap_y = text_off_y * font_height;

	float text_x = back.x + gap_x;
	float text_y = back.y + gap_y;

	if (leaning < 0)
		text_x += back.h;

	font->render.draw_text_simple(renderer, text.c_str(), text_x, text_y);

	float frac = clamp_f((leaning + 1) / 2, 0, 1);
	float cb_x = frac * (back.w - back.h);
	if (cb_x < 0) cb_x = 0;

	back.x += cb_x;
	back.w = back.h;
	sdl_draw_rect(back, default_color, renderer);

	if (checked) {
		int w = 0.5 + (float)back.w * border_frac;
		int h = 0.5 + (float)back.h * border_frac;
		Rect_Int check = {
			back.x + w,
			back.y + h,
			back.w - 2*w,
			back.h - 2*h
		};
		sdl_draw_rect(check, sel_color, renderer);
	}
}

void Checkbox::base_action(Camera& view, Point& cursor, Input& input) {
	checked = !checked;
}

void Data_View::highlight(Camera& view, Point& inside) {
	if (!font || !data) {
		hl_row = hl_col = -1;
		return;
	}

	int top = vscroll ? vscroll->position : 0;

	Rect table = pos;
	if (show_column_names) {
		table.y += header_height;
		table.h -= header_height;
	}

	float font_height = (float)font->render.text_height();
	float font_units = font_height / view.scale;

	int n_rows = data->tree.size() > 0 ? data->tree.size() : data->row_count();
	n_rows = data->filtered >= 0 ? data->filtered : n_rows;
	int n_cols = data->column_count();

	float space = font->line_spacing * font_units;

	float y = 0;
	float item_w = table.w - 4;

	int old_hl_row = hl_row;
	int old_hl_col = hl_col;
	hl_row = hl_col = -1;
	bool hl = false;

	for (int i = top; i < n_rows && y < table.h; i++) {
		float h = y > table.h - item_height ? table.h - y : item_height;
		if (inside.x >= table.x && inside.x < table.x + table.w && inside.y >= table.y + y && inside.y < table.y + y + h) {
			hl_row = i;

			float x = table.x;
			for (int j = 0; j < n_cols; j++) {
				float w = column_width(pos.w, scroll_total, font_height, view.scale, j) / view.scale;

				if (inside.x >= x && inside.x < x + w) {
					hl_col = j;

					/*
					if (data->headers[j].type == ColumnElement) {
						int row = data->get_table_index(hl_row);
						UI_Element *elem = row >= 0 ? (UI_Element*)data->columns[j][row] : nullptr;
						if (elem) {
							Point yes = {0};
							elem->highlight(view, yes); // just hope that elem isn't another Data_View lol
						}
					}
					*/

					break;
				}

				x += w + column_spacing * font_units;
			}

			hl = true;
			break;
		}
		y += item_height;
	}

	needs_redraw = needs_redraw || hl_row != old_hl_row || hl_col != old_hl_col;
}

float Data_View::column_width(float total_width, float min_width, float font_height, float scale, int idx) {
	float w = data->headers[idx].width;
	if (w == 0)
		return (total_width - min_width) * scale;

	w *= (total_width - total_spacing) * scale;

	if (data->headers[idx].max_size > 0) {
		float max = data->headers[idx].max_size * font_height;
		w = w < max ? w : max;
	}

	if (data->headers[idx].min_size > 0) {
		float min = data->headers[idx].min_size * font_height;
		w = w > min ? w : min;
	}

	return w;
}

void Data_View::draw_item_backing(Renderer renderer, RGBA& color, Rect_Int& back, float scale, int idx) {
	int top = vscroll ? vscroll->position : 0;
	if (idx < 0 || idx < top)
		return;

	float y = (idx - top) * item_height * scale;
	float h = item_height * scale;
	if (y + h > back.h)
		h = back.h - y;

	if (h > 0) {
		Rect_Int r = {
			back.x,
			(int)((float)back.y + y),
			back.w,
			(int)h
		};
		sdl_draw_rect(r, color, renderer);
	}
}

void Data_View::draw_cell(Draw_Cell_Info& info, void *cell, Column& header, Rect& r) {
	auto type = header.type;
	if (type == ColumnString || type == ColumnDec || type == ColumnHex || type == ColumnFile || type == ColumnStdString) {
		char *str = nullptr;
		char buf[24];

		if (type == ColumnString)
			str = (char*)cell;
		else if (type == ColumnDec) {
			write_dec(buf, (s64)cell);
			str = buf;
		}
		else if (type == ColumnHex) {
			write_hex(buf, (u64)cell, header.count_per_cell);
			str = buf;
		}
		else if (type == ColumnFile && cell)
			str = ((File_Entry*)cell)->name;
		else if (type == ColumnStdString && cell)
			str = (char*)((std::string*)cell)->c_str();

		if (str)
			font->render.draw_text(info.renderer, (const char*)str, r.x, r.y - info.line_off, clip);
	}
	else if (type == ColumnImage && cell) {
		sdl_get_texture_size(cell, &info.src.w, &info.src.h);
		info.dst.x = r.x;
		info.dst.y = r.y + info.sp_half;

		if (info.dst.y + info.dst.h > info.y_max) {
			int img_h = info.y_max - info.dst.y;
			info.src.h = (float)(info.src.h * img_h) / (float)info.dst.h;
			info.dst.h = img_h;
		}

		sdl_apply_texture(cell, info.dst, &info.src, info.renderer);
	}
	else if (type == ColumnCheckbox) {
		float indent = info.font_height * 0.1;
		info.dst.x = r.x + 0.5 + indent;
		info.dst.y = r.y + info.sp_half + 0.5 + indent;
		info.dst.w = 0.5 + info.font_height - 2*indent;
		info.dst.h = info.dst.w;

		if (info.dst.y + info.dst.h > info.y_max)
			info.dst.h = info.y_max - info.dst.y;

		sdl_draw_rect(info.dst, parent->parent->colors.table_cb_back, info.renderer);

		if (*(u8*)&cell == 2) {
			indent = info.font_height * 0.15;
			info.src.x = info.dst.x + 0.5 + indent;
			info.src.y = info.dst.y + 0.5 + indent;
			info.src.w = 0.5 + info.dst.w - 2*indent;
			info.src.h = info.src.w;

			if (info.src.y + info.src.h > info.y_max)
				info.src.h = info.y_max - info.src.y;

			sdl_draw_rect(info.src, parent->parent->colors.cb, info.renderer);
			info.src.x = info.src.y = 0;
		}

		info.dst.w = info.dst.h = info.font_height;
	}
	else if (type == ColumnElement && cell) {
		auto elem = (UI_Element*)cell;
		elem->font = font;
		elem->parent = parent;
		elem->table_vis = true;

		if (elem->visible) {
			Rect_Int back = r.to_rect_int();
			back.y += info.elem_pad;
			back.h -= 2 * info.elem_pad;

			float scale = info.camera->scale;
			elem->pos = {0, 0, back.w / scale, back.h / scale};
			elem->screen = back;

			elem->draw_element(info.renderer, *info.camera, back, false, false, false);
		}
	}
}

void Data_View::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (!data) {
		sdl_draw_rect(back, default_color, renderer);
		return;
	}

	int n_rows = data->row_count();
	int n_cols = data->column_count();
	int tree_size = data->tree.size();

	float font_height = (float)font->render.text_height();
	float font_units = font_height / view.scale;

	item_height = (font_height + (font->line_spacing * font_height)) / view.scale;

	Rect table = pos;
	if (show_column_names) {
		table.y += header_height;
		table.h -= header_height;

		int y = back.y;
		back.y += header_height * view.scale + 0.5;
		back.h -= back.y - y;
	}

	int top = 0;
	int n_visible = table.h / item_height;

	int n_items = data->filtered;
	if (n_items < 0)
		n_items = tree_size > 0 ? tree_size : n_rows;

	if (vscroll) {
		top = vscroll->position;
		vscroll->set_maximum(n_items, n_visible);
	}

	float scroll_x = 0;
	float table_w = pos.w;

	scroll_total = column_spacing;
	total_spacing = column_spacing;

	for (int i = 0; i < n_cols; i++) {
		scroll_total += data->headers[i].min_size + column_spacing;
		total_spacing += column_spacing;
	}

	scroll_total *= font_units;
	total_spacing *= font_units;

	if (hscroll) {
		hscroll->set_maximum(scroll_total, pos.w);

		scroll_x = scroll_total - pos.w;
		if (scroll_x < 0) scroll_x = 0;

		scroll_x = hscroll->position - scroll_x;
		table_w = hscroll->maximum;
	}
	scroll_x *= view.scale;

	float col_space = column_spacing * font_height;

	float x_start = back.x + col_space - scroll_x;

	if (show_column_names) {
		float scaled_head_h = header_height * view.scale;
		Rect header = {
			(float)back.x,
			(float)back.y - scaled_head_h,
			(float)back.w,
			scaled_head_h
		};
		sdl_draw_rect(header, back_color, renderer);

		float x = x_start;
		float y = back.y - (header.h + font->line_offset * font_height);

		for (int i = 0; i < n_cols; i++) {
			font->render.draw_text_simple(renderer, data->headers[i].name, x, y);

			float w = column_width(pos.w, scroll_total, font_height, view.scale, i);
			x += w + column_spacing * font_height;
		}
	}

	sdl_draw_rect(back, default_color, renderer);

	draw_item_backing(renderer, sel_color, back, view.scale, sel_row);
	if (hl_row != sel_row && (box_hovered || focussed))
		draw_item_backing(renderer, hl_color, back, view.scale, hl_row);

	float pad = padding * view.scale;
	float x = x_start;
	float y = back.y;
	float x_max = back.x + back.w - col_space;
	float y_max = y + table.h * view.scale;

	float line_off = font->line_offset * font_height;
	float line_sp = (font->line_spacing + extra_line_spacing) * font_height;
	float line_h = font_height + line_sp;

	Rect_Int dst = {0, 0, (int)font_height, (int)font_height};
	Rect_Int src = {0};

	clip.x_lower = x + scroll_x;
	clip.y_upper = y_max;

	int elem_pad = view.scale + 1.0;
	Draw_Cell_Info dci = {
		this, &view, renderer, dst, src, y_max, line_off, line_sp / 2, font_height, elem_pad
	};

	float table_off_y = y;

	for (int i = 0; i < n_cols && x < back.x + back.w; i++) {
		float w = column_width(pos.w, scroll_total, font_height, view.scale, i);
		clip.x_upper = x+w < x_max ? x+w : x_max;

		Rect cell_rect = {x, y, w, line_h};

		if (data->headers[i].type == ColumnElement) {
			for (auto& cell : data->columns[i]) {
				auto elem = (UI_Element*)cell;
				if (elem)
					elem->table_vis = false;
			}
		}

		if (data->filtered >= 0) {
			int n = -1;
			for (auto& idx : data->list) {
				if (y >= y_max)
					break;
				n++;
				if (n < top)
					continue;

				bool skip_draw = false;
				if (condition_col >= 0) {
					if (!TABLE_CHECKBOX_CHECKED(data, condition_col, idx))
						continue;

					skip_draw = condition_col == i;
				}

				auto thing = data->columns[i][idx];
				if (!skip_draw)
					draw_cell(dci, thing, data->headers[i], cell_rect);

				y += line_h;
				cell_rect.y = y;
			}
		}
		else {
			for (int j = top; y < y_max; j++) {
				int idx = j;
				if (tree_size) {
					if (j >= tree_size)
						break;
					idx = data->tree[j];
				}
				else if (j >= n_rows)
					break;

				if (idx < 0 && i == 0) {
					int branch = -idx - 1;
					Texture icon = data->branches[branch].closed ? icon_plus : icon_minus;
					Column temp;
					temp.type = ColumnImage;
					cell_rect.y += font_height * 0.025;
					draw_cell(dci, icon, temp, cell_rect);

					int name_idx = data->branches[branch].name_idx;
					if (name_idx >= 0)
						font->render.draw_text(renderer, data->branch_name_vector.at(name_idx), x + font_height * 1.2, y - line_off, clip);
				}
				else if (idx >= 0) {
					bool skip_draw = false;
					if (condition_col >= 0) {
						if (!TABLE_CHECKBOX_CHECKED(data, condition_col, idx))
							continue;

						skip_draw = condition_col == i;
					}

					auto thing = data->columns[i][idx];
					if (!skip_draw)
						draw_cell(dci, thing, data->headers[i], cell_rect);
				}

				y += line_h;
				cell_rect.y = y;
			}
		}

		if (data->headers[i].type == ColumnElement) {
			for (auto& cell : data->columns[i]) {
				auto elem = (UI_Element*)cell;
				if (elem && elem->table_vis && elem->visible && elem->use_post_draw) {
					Rect_Int r = view.to_screen_rect(parent->box);
					Rect_Int b = make_int_ui_box(r, elem->pos, view.scale);
					elem->post_draw(view, b, false, false, false);
				}
			}
		}

		x += w + column_spacing * font_height;
		y = table_off_y;
	}
}

void Data_View::base_action(Camera& view, Point& cursor, Input& input) {
	if (hl_row >= 0)
		sel_row = hl_row;

	if (active_elem) {
		active_elem->base_action(view, cursor, input);
		if (active_elem->action)
			active_elem->action(active_elem, view, input.double_click);

		active_elem = nullptr;
	}
}

void Data_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!data)
		return;

	if (data->has_ui_elements) {
		Rect_Int box = view.to_screen_rect(parent->box);
		int n_cols = data->column_count();

		for (int i = 0; i < n_cols; i++) {
			if (data->headers[i].type != ColumnElement)
				continue;

			for (auto& cell : data->columns[i]) {
				if (!cell)
					continue;

				auto elem = (UI_Element*)cell;
				Point inside = {
					cursor.x - (elem->screen.x - box.x) / view.scale,
					cursor.y - (elem->screen.y - box.y) / view.scale
				};

				elem->mouse_handler(view, input, inside, hovered);
				elem->key_handler(view, input);

				if (input.lclick && elem->pos.contains(inside))
					active_elem = elem;
			}

			needs_redraw = true;
		}
	}

	if (!hovered || (!input.lclick && !input.rclick))
		return;

	if (input.lclick) {
		int row = data->get_table_index(hl_row);
		if (hl_col >= 0 && row >= 0 && data->headers[hl_col].type == ColumnCheckbox) {
			TOGGLE_TABLE_CHECKBOX(data, hl_col, row);
			if (checkbox_toggle_handler)
				checkbox_toggle_handler(this, hl_col, row);
		}
		else if (hl_row >= 0 && row < 0 && data->tree.size() > 0) {
			int b = -row - 1;
			data->branches[b].closed = !data->branches[b].closed;
			data->update_tree();
		}
	}
	if (input.rclick && hl_row >= 0)
		sel_row = hl_row;

	needs_redraw = true;
}

bool Data_View::scroll_handler(Camera& view, Input& input, Point& inside) {
	bool scrolled = false;

	if (data && data->has_ui_elements) {
		Rect_Int box = view.to_screen_rect(parent->box);
		int n_cols = data->column_count();

		for (int i = 0; i < n_cols; i++) {
			if (data->headers[i].type != ColumnElement)
				continue;

			for (auto& cell : data->columns[i]) {
				if (!cell)
					continue;

				auto elem = (UI_Element*)cell;
				Point inside_elem = {
					inside.x - (elem->screen.x - screen.x) / view.scale,
					inside.y - (elem->screen.y - screen.y) / view.scale
				};

				if (elem->pos.contains(inside_elem)) {
					scrolled = elem->scroll_handler(view, input, inside_elem);
					if (scrolled) {
						needs_redraw = true;
						return true;
					}
				}
			}
		}
	}

	Scroll *scroll = input.lshift || input.rshift ? hscroll : vscroll;
	if (scroll && input.scroll_y > 0) {
		scroll->scroll(-1);
		scrolled = true;
	}
	if (scroll && input.scroll_y < 0) {
		scroll->scroll(1);
		scrolled = true;
	}

	if (scrolled)
		needs_redraw = true;

	return scrolled;
}

void Data_View::deselect() {
	hl_row = hl_col = -1;
}

void Data_View::release() {
	data->release();
}

void Divider::make_icon(float scale) {
	bool was_hl = icon == icon_hl;
	sdl_destroy_texture(&icon_default);
	sdl_destroy_texture(&icon_hl);

	RGBA fade_color = default_color;
	fade_color.a = 0.5;

	float pad = padding * 1.5;
	float h = pad * scale;
	float w = h * 1.375;
	icon_w = w + 0.5;
	icon_h = h + 0.5;

	double gap = breadth * 2 / pad;
	double thickness = 0.15;
	double sharpness = 2.0;

	icon_default = make_divider_icon(fade_color, icon_w, icon_h, gap, thickness, sharpness, !vertical);
	icon_hl = make_divider_icon(default_color, icon_w, icon_h, gap, thickness, sharpness, !vertical);

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

	bool use_default = !held && !box.contains(cursor);

	Texture old_icon = icon;
	icon = use_default ? icon_default : icon_hl;
	needs_redraw = icon != old_icon;

	if (use_default)
		return;

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
		position = clamp_f(cur - hold_pos, minimum, maximum);
	}

	needs_redraw = true;
}

void Divider::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	sdl_draw_rect(back, default_color, renderer);

	if (icon) {
		Rect_Int box = {
			back.x + (back.w - icon_w) / 2,
			back.y + (back.h - icon_h) / 2,
			icon_w,
			icon_h
		};
		sdl_apply_texture(icon, box, nullptr, renderer);
	}
}

void Drop_Down::mouse_handler(Camera& view, Input& input, Point& inside, bool hovered) {
	if (edit_elem)
		return;

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
		else if (hl >= 0)
			parent->set_dropdown(this);
	}
}

void Drop_Down::highlight(Camera& view, Point& inside) {
	hl = -1;
	if (!dropped || inside.x < pos.x || inside.x >= pos.x + width)
		return;

	float font_unit = font->render.text_height() / view.scale;
	float y = pos.y + pos.h + title_off_y * font_unit;
	float h = line_height * font_unit;

	auto lines = get_content();
	for (int i = 0; i < lines->size(); i++) {
		if (inside.y >= y && inside.y < y + h) {
			hl = i;
			break;
		}
		y += h;
	}
}

void Drop_Down::cancel() {
	dropped = false;
	hl = -1;
}

void Drop_Down::base_action(Camera& view, Point& cursor, Input& input) {
	if (hl >= 0) {
		if (keep_selected)
			sel = hl;
		if (edit_elem) {
			auto edit = dynamic_cast<Edit_Box*>(edit_elem);
			if (edit)
				edit->dropdown_item_selected(input);
		}
	}
}

void Drop_Down::draw_menu(Renderer renderer, Camera& view, float menu_x, float menu_y) {
	float font_height = font->render.text_height();
	auto lines = get_content();

	Rect box = {
		menu_x,
		menu_y,
		width * view.scale,
		(title_off_y + line_height * lines->size()) * font_height
	};

	sdl_draw_rect(box, hl_color, renderer);

	if (hl >= 0) {
		Rect hl_rect = box;
		hl_rect.y += line_height * hl * font_height;
		hl_rect.h = (title_off_y + line_height) * font_height + 1;
		sdl_draw_rect(hl_rect, sel_color, renderer);
	}

	float hack = 3;
	float item_x = item_off_x * font_height;
	float item_w = (width - hack) * view.scale - item_x;
	item_clip.x_upper = box.x + item_x + item_w;

	float y = font_height * ((line_height - 1) / 2 - font->line_offset);

	for (auto& l : *lines) {
		if (l)
			font->render.draw_text(renderer, l, box.x + item_x, box.y + y, item_clip);

		y += font_height * line_height;
	}
}

void Drop_Down::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (edit_elem)
		return;

	if (elem_hovered || dropped)
		sdl_draw_rect(back, hl_color, renderer);
	else
		sdl_draw_rect(back, default_color, renderer);

	auto lines = get_content();

	const char *text = title;
	if (!title && lines && lines->size() > 0 && sel >= 0 && (*lines)[sel])
		text = (*lines)[sel];

	float x = 0;
	float w = pos.w * view.scale;

	if (icon) {
		Rect_Int r = { back.x, back.y, icon_length, icon_length };

		w -= icon_length;
		if (!icon_right)
			x = icon_length;
		else
			r.x += back.w - icon_length;

		sdl_apply_texture(icon, r, nullptr, renderer);
	}

	if (text) {
		float text_w = font->render.text_width(text);
		float font_h = font->render.text_height();

		float pad_x = title_pad_x * font_h;
		float gap_y = title_off_y * font_h;

		float text_x = x + pad_x + leaning * (w - text_w - 2*pad_x);
		font->render.draw_text_simple(renderer, text, back.x + text_x, back.y + gap_y);
	}
}

bool Edit_Box::is_above_icon(Point& p) {
	const float w = pos.h;
	float icon_x = icon_right ? pos.x + pos.w - w : pos.x;
	return icon && p.y >= pos.y && p.y < pos.y + pos.h && p.x >= icon_x && p.x - icon_x < w;
}

void Edit_Box::dropdown_item_selected(Input& input) {
	editor.set_cursor(editor.primary, 0);
	editor.set_cursor(editor.secondary, 0);
	editor.text = (*dropdown->get_content())[dropdown->hl];
	if (key_action)
		key_action(this, input);
}

void Edit_Box::key_handler(Camera& view, Input& input) {
	text_changed = false;

	if (parent->active_edit != &editor)
		return;

	int res = editor.handle_input(input);
	editor.apply_scroll();

	if ((res & 1) && key_action) {
		text_changed = true;
		key_action(this, input);
	}

	if (res & 2)
		needs_redraw = true;

	if (res & 8) {
		editor.clear();
		//disengage(input, false);
		parent->active_edit = nullptr;
	}
}

void Edit_Box::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered)
		return;

	const float w = pos.h;
	float text_x = icon_right ? pos.x : pos.x + w;

	bool on_icon = is_above_icon(cursor);
	use_default_cursor = on_icon;

	if (hovered && input.lclick && pos.contains(cursor)) {
		float x = (cursor.x - text_x) * view.scale;
		float y = (cursor.y - pos.y) * view.scale;

		editor.update_cursor(x, y, font, view.scale, ticks, input.lclick);
	}
}

void Edit_Box::base_action(Camera& view, Point& cursor, Input& input) {
	if (dropdown && is_above_icon(cursor))
		parent->set_dropdown(dropdown);
	else
		parent->active_edit = &editor;
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

	needs_redraw = true;
}

void Edit_Box::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (dropdown) {
		dropdown->pos = pos;
		dropdown->screen = screen;
		dropdown->width = pos.w;
		/*
		if (dropdown->dropped)
			dropdown->draw_menu(nullptr, view, screen.x, screen.y + pos.h * view.scale);
		*/
	}

	sdl_draw_rect(back, default_color, renderer);

	const char *str = nullptr;
	Font *fnt = font;

	if (ph_font && editor.text.size() == 0) {
		fnt = ph_font;
		str = placeholder.c_str();
	}
	else
		str = editor.text.c_str();

	float cur_x;
	fnt->render.text_width(str, editor.primary.cursor, &cur_x);

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
		sdl_apply_texture(icon, dst, nullptr, renderer);
	}

	float window_w = (pos.w * view.scale) - x - icon_w - gap_x;

	clip.x_lower = back.x + x;
	clip.x_upper = clip.x_lower + window_w;
	clip.y_lower = back.y;
	clip.y_upper = back.y + back.h;

	editor.refresh(clip, font);
	float offset = editor.x_offset;

	editor.draw_selection_box(renderer, sel_color);

	fnt->render.draw_text(renderer, str, back.x + x - offset, back.y + gap_y, clip);

	if (parent->active_edit == &editor) {
		float caret_w = (float)caret_width * height;
		float caret_y = (float)caret_off_y * height;

		Rect_Int r = {
			(int)((float)back.x + x + cur_x - offset),
			(int)((float)back.y + 2.0f*gap_y + caret_y),
			(int)caret_w,
			(int)height
		};

		sdl_draw_rect(r, caret, renderer);
	}
}

void Hex_View::set_region(u64 address, u64 size) {
	region_address = address;
	region_size = size;
	col_offset = 0;
	offset = 0;
	sel = -1;
	alive = true;

	if (scroll)
		scroll->maximum = (double)size;
}

void Hex_View::set_offset(u64 off) {
	if (scroll)
		scroll->position = (double)off;

	col_offset = off % columns;
	offset = off - col_offset;
}

void Hex_View::update_view(float scale) {
	float font_height = font->render.text_height();
	vis_rows = pos.h * scale / font_height;

	if (scroll) {
		u64 size = region_size;
		if (size % columns > 0)
			size += columns - (size % columns);

		scroll->set_maximum(size, vis_rows * columns);

		offset = (int)scroll->position;
		offset -= offset % columns;
		offset += col_offset;
	}

	addr_digits = count_hex_digits(region_address + region_size - 1);

	if (alive) {
		if (span_idx < 0)
			span_idx = source->request_span();

		Span& s = source->spans[span_idx];
		s.address = region_address + offset;
		s.size = vis_rows * columns;
	}
}

float Hex_View::print_address(Renderer renderer, u64 address, float x, float y, float digit_w, Render_Clip& clip, Rect_Int& box, float pad) {
	int n_digits = count_hex_digits(address);
	float text_x = x + (addr_digits - n_digits) * digit_w;

	char buf[20];
	write_hex(buf, address, n_digits);
	font->render.draw_text(renderer, buf, box.x + text_x, box.y + y, clip);

	return x + addr_digits * digit_w + 2*pad;
}

float Hex_View::print_hex_row(Renderer renderer, Span& span, int idx, float x, float y, Rect_Int& box, float pad) {
	Rect_Int src, dst;
	int n_chars = columns * 2;
	char *hex_row = &hex_vec.pool[idx * 2];

	for (int i = 0; i < n_chars && x < box.w; i++) {
		const Glyph *gl = &font->render.glyphs[hex_row[i]];

		src.x = gl->atlas_x;
		src.y = gl->atlas_y;
		src.w = gl->img_w;
		src.h = gl->img_h;
/*
		float clip_x = x + pad + gl->left;
		if (clip_x + src.w > box.w)
			src.w = box.w - clip_x;
*/
		dst.x = box.x + x + gl->left;
		dst.y = box.y + y - gl->top;
		dst.w = src.w;
		dst.h = src.h;

		sdl_apply_texture(font->render.tex, dst, &src, renderer);

		x += gl->box_w;
		x += pad * (i % 2 == 1);
	}

	return x + pad;
}

void Hex_View::print_ascii_row(Renderer renderer, Span& span, int idx, float x, float y, Render_Clip& clip, Rect_Int& box) {
	char *ascii = (char*)alloca(columns + 1);
	for (int i = 0; i < columns; i++) {
		char c = '?';
		if (idx+i < span.retrieved) {
			c = (char)span.data[idx+i];
			if (c < ' ' || c > '~')
				c = '.';
		}
		ascii[i] = c;
	}
	ascii[columns] = 0;

	font->render.draw_text(renderer, ascii, box.x + x, box.y + y, clip);
}

void Hex_View::draw_cursors(Renderer renderer, int idx, Rect_Int& back, float x_start, float pad, float scale) {
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
		sdl_draw_rect(cursor, caret, renderer);
	}
	if (show_ascii && ascii_x < x_max) {
		Rect cursor = {
			ascii_x,
			y,
			cur_w,
			row_height
		};
		sdl_draw_rect(cursor, caret, renderer);
	}
}

void Hex_View::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (!alive || span_idx < 0 || vis_rows <= 0 || columns <= 0) {
		sdl_draw_rect(back, default_color, renderer);
		return;
	}

	Rect_Int src, dst;

	font_height = font->render.text_height();

	float x_start = x_offset * view.scale;
	float x = x_start;
	float y = font_height;
	float pad = padding * font_height;

	float digit_w = font->render.digit_width();
	int addr_w = addr_digits * digit_w + 2*pad + 0.5;

	Rect_Int hex_back = back;
	if (show_addrs) {
		Rect_Int addr_back = back;
		addr_back.w = addr_w;
		sdl_draw_rect(addr_back, back_color, renderer);

		hex_back.x += addr_w;
		hex_back.w -= addr_w;
	}
	sdl_draw_rect(hex_back, default_color, renderer);

	Render_Clip addr_clip = {
		CLIP_RIGHT, 0, 0, back.x + addr_w - pad, 0
	};
	Render_Clip ascii_clip = {
		CLIP_RIGHT, 0, 0, back.x + back.w - pad, 0
	};

	row_height = vis_rows > 0 ? (back.h - pad) / (float)vis_rows : font_height;

	auto& span = source->spans[span_idx];
	int left = span.size;

	int n_chars = span.size * 2;
	int end = span.retrieved * 2;
	hex_vec.try_expand(n_chars);

	for (int i = 0; i < end; i++) {
		int digit = span.data[i/2];
		digit >>= 4 * (i % 2 == 0);
		digit &= 0xf;
		hex_vec.pool[i] = (digit <= 9 ? '0' + digit : 'a' - 10 + digit) - MIN_CHAR;
	}
	if (n_chars > end)
		memset(hex_vec.pool + end, '?' - MIN_CHAR, n_chars - end);

	while (left > 0) {
		int idx = span.size - left;

		if (show_addrs)
			x = print_address(renderer, region_address + offset + idx, x, y - font_height, digit_w, addr_clip, back, pad);
		if (show_hex && x < back.w)
			x = print_hex_row(renderer, span, idx, x, y, back, pad);
		if (show_ascii && x < back.w)
			print_ascii_row(renderer, span, idx, x, y - font_height, ascii_clip, back);

		x = x_start;
		y += row_height;
		left -= columns;
	}

	if (sel >= 0 && sel >= offset && sel < offset + span.size)
		draw_cursors(renderer, sel - offset, hex_back, x_start, pad, view.scale);
}

void Hex_View::key_handler(Camera& view, Input& input) {
	bool pg_down = input.strike(input.pgdown);
	bool pg_up = input.strike(input.pgup);

	if ((pg_up || pg_down) && scroll) {
		int delta = pg_up ? -1 : 1;
		delta *= columns * vis_rows;
		scroll->scroll(delta);
		needs_redraw = true;
	}
}

bool Hex_View::scroll_handler(Camera& view, Input& input, Point& inside) {
	if (!scroll)
		return false;

	int delta = columns;
	if (input.lshift || input.rshift)
		delta *= vis_rows;

	bool scrolled = false;
	if (input.scroll_y > 0) {
		scroll->scroll(-delta);
		scrolled = true;
	}
	if (input.scroll_y < 0) {
		scroll->scroll(delta);
		scrolled = true;
	}

	if (scrolled)
		needs_redraw = true;

	return scrolled;
}

void Hex_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered || !input.lclick || !pos.contains(cursor))
		return;

	sel = -1;

	float font_h = font_height / view.scale;
	float pad = padding * font_h;
	float row_height = vis_rows > 0 ? (pos.h - pad) / (float)vis_rows : font_h;

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

	needs_redraw = true;
}

void Image::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	sdl_apply_texture(img, back, nullptr, renderer);
}

void Label::update(Camera& view, Rect_Int& rect) {
	if (outer_box.w <= 0)
		return;

	float text_w = font->render.text_width(text.c_str()) / view.scale;
	float font_h = font->render.text_height() / view.scale;

	pos.x = outer_box.x + (outer_box.w - text_w) / 2;
	pos.y = outer_box.y + (outer_box.h - font_h) / 2;
	pos.w = text_w;
	pos.h = font_h;
}

void Label::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	clip.x_upper = back.x + back.w;
	font->render.draw_text(renderer, text.c_str(), back.x, back.y, clip);
}

void Number_Edit::make_icon(float scale) {
	sdl_destroy_texture(&icon);

	float w = font->render.text_height() * end_width * box_width;
	float h = pos.h * scale * box_height;

	icon_w = w;
	icon_h = h;

	double gap = 0.15;
	double thickness = 0.15;
	double sharpness = 2.0;
	icon = make_divider_icon(arrow_color, icon_w, icon_h, gap, thickness, sharpness, true);
}

void Number_Edit::key_handler(Camera& view, Input& input) {
	if (parent->active_edit == &editor) {
		int res = editor.handle_input(input);

		number = strtol(editor.text.c_str(), nullptr, 10);
		if ((res & 1) && key_action)
			key_action(this, input);
		if (res & 2)
			needs_redraw = true;
		if (res & 8)
			parent->active_edit = nullptr;
	}
}

void Number_Edit::base_action(Camera& view, Point& cursor, Input& input) {
	parent->active_edit = &editor;
}

void Number_Edit::affect_number(int delta) {
	number = strtol(editor.text.c_str(), nullptr, 10) + delta;

	editor.text = std::to_string(number);
	editor.set_cursor(editor.primary, 0);
	editor.secondary = editor.primary;
	editor.selected = false;

	if (key_action) {
		Input blank = {};
		key_action(this, blank);
	}
}

bool Number_Edit::scroll_handler(Camera& view, Input& input, Point& inside) {
	int delta = 0;
	if (input.scroll_y > 0)
		delta = 1;
	if (input.scroll_y < 0)
		delta = -1;

	affect_number(delta);
	return delta != 0;
}

void Number_Edit::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered || !input.lclick || !pos.contains(cursor))
		return;

	float x = (cursor.x - pos.x) * view.scale;
	float y = (cursor.y - pos.y) * view.scale;

	editor.update_cursor(x, y, font, view.scale, ticks, input.lclick);

	if (x >= (pos.w * view.scale) - (end_width * font->render.text_height())) {
		int delta = y < pos.h * view.scale / 2 ? 1 : -1;
		affect_number(delta);
	}
}

void Number_Edit::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	float font_h = font->render.text_height();
	float end_w = font_h * end_width;
	if (back.w <= end_w)
		return;

	Rect r = back.to_rect();
	r.w -= end_w - 1.0;
	sdl_draw_rect(r, default_color, renderer);

	float text_x = back.x + font_h * text_off_x;
	float text_y = back.y + font_h * text_off_y;

	clip.x_upper = back.x + back.w - end_w;

	editor.refresh(clip, font, 0, 0);

	font->render.draw_text(renderer, editor.text.c_str(), text_x, text_y, clip);

	float digit_w = font->render.digit_width();
	if (parent->active_edit == &editor) {
		Point p = {text_x, text_y};
		editor.draw_cursor(renderer, arrow_color, editor.primary, p, clip, view.scale);
	}

	r.x = clip.x_upper;
	r.w = end_w;
	sdl_draw_rect(r, sel_color, renderer);

	r.x += (r.w - (float)icon_w) / 2.0;
	r.y += (r.h - (float)icon_h) / 2.0;
	r.w = icon_w;
	r.h = icon_h;
	sdl_apply_texture(icon, r, nullptr, renderer);
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
	double old_pos = position;
	position = clamp_f(position + delta, 0, maximum - view_span);

	double epsilon = 0.01;
	needs_redraw = abs(position - old_pos) > epsilon;
	if (content)
		content->needs_redraw = content->needs_redraw || needs_redraw;
}

void Scroll::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!show_thumb)
		return;

	if (input.lclick && pos.contains(cursor))
		engage(cursor);

	if (!input.lmouse)
		held = false;

	double new_pos = position;
	if (held) {
		double dist = 0;
		if (vertical)
			dist = cursor.y - (pos.y + hold_region);
		else
			dist = cursor.x - (pos.x + hold_region);

		double span = (1 - thumb_frac) * length;
		new_pos = dist * (maximum - view_span) / span;
	}

	// apply bounds-checking
	scroll(new_pos - position);
}

void Scroll::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (vertical)
		length = pos.h - 2*padding;
	else
		length = pos.w - 2*padding;

	sdl_draw_rect(back, back_color, renderer);

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
		w = back.w - gap;
		h = thumb_len;
	}
	else {
		x += thumb_pos;
		w = thumb_len;
		h = back.h - gap;
	}

	Rect_Int scroll_rect = {
		back.x + (int)(x * view.scale + 0.5),
		back.y + (int)(y * view.scale + 0.5),
		(int)(w + 0.5),
		(int)(h + 0.5)
	};

	RGBA *color = &default_color;
	if (held)
		color = &sel_color;
	else if (hl)
		color = &hl_color;

	sdl_draw_rect(scroll_rect, *color, renderer);
}

void Scroll::highlight(Camera& view, Point& inside) {
	hl = pos.contains(inside);
}

void Scroll::deselect() {
	hl = false;
}

void Tabs::add(const char **names, int count) {
	for (int i = 0; i < count; i++) {
		Tab t = {(char*)names[i], 0};
		tabs.push_back(t);
	}
}

void Tabs::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	hl = -1;
	if (!hovered || !pos.contains(cursor))
		return;

	float total = 0;
	for (auto& t : tabs)
		total += t.width;

	float border = tab_border * view.scale;
	float spacing = (pos.w * view.scale) / (total + border - 1.0f);
	float cur_x = cursor.x - pos.x;
	int n_tabs = tabs.size();

	float x = 0;
	for (int i = 0; i < n_tabs; i++) {
		x += tabs[i].width * spacing / view.scale;
		if (cur_x < x) {
			hl = i;
			break;
		}
	}

	if (input.lclick && hl >= 0) {
		sel = hl;
		event(this);
	}
}

void Tabs::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	back.h += 1.0f;
	sdl_draw_rect(back, default_color, renderer);

	float font_h = font->render.text_height();
	float pad = x_offset * font_h;
	float border = tab_border * view.scale;

	float total = 0;
	for (auto& t : tabs) {
		t.width = font->render.digit_width() * (float)strlen(t.name);
		total += t.width + border;
	}
	float spacing = back.w / (total + border - 1.0f);

	int n_tabs = tabs.size();
	float x = back.x + border;

	Rect r = {0, back.y + border, 0, back.h - border};
	for (int i = 0; i < n_tabs; i++) {
		float width = (tabs[i].width + border) * spacing;

		if (i == sel || i == hl) {
			r.x = x;
			r.w = width - border;
			RGBA *color = i == hl ? &hl_color : &sel_color;
			sdl_draw_rect(r, *color, renderer);
		}

		font->render.draw_text_simple(renderer, tabs[i].name, x + pad, back.y);
		x += width;
	}
}

void Text_Editor::key_handler(Camera& view, Input& input) {
	if (parent->active_edit == &editor) {
		int res = editor.handle_input(input);
		editor.apply_scroll(hscroll, vscroll);

		if ((res & 1) && key_action)
			key_action(this, input);
		if (res & 2)
			needs_redraw = true;
		if (res & 4)
			ticks = 0;
		if (res & 8)
			parent->active_edit = nullptr;

		needs_redraw = needs_redraw || res != 0;
	}
}

void Text_Editor::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (hovered && input.lclick && pos.contains(cursor)) {
		parent->ui_held = true;
		parent->active_edit = &editor;
		mouse_held = true;
	}
	else if (!input.lmouse)
		mouse_held = false;

	if (mouse_held) {
		float x = (cursor.x - pos.x) * view.scale;
		float y = (cursor.y - pos.y) * view.scale;

		editor.update_cursor(x, y, font, view.scale, ticks, input.lclick);
	}

	needs_redraw = needs_redraw || mouse_held;
}

void Text_Editor::base_action(Camera& view, Point& cursor, Input& input) {
	parent->active_edit = &editor;
}

bool Text_Editor::scroll_handler(Camera& view, Input& input, Point& inside) {
	Scroll *scroll = input.lshift || input.rshift ? hscroll : vscroll;
	bool scrolled = false;

	if (scroll && input.scroll_y > 0) {
		scroll->scroll(-font->render.text_height());
		scrolled = true;
	}
	if (scroll && input.scroll_y < 0) {
		scroll->scroll(font->render.text_height());
		scrolled = true;
	}

	if (scrolled)
		needs_redraw = true;

	return scrolled;
}

void Text_Editor::update(Camera& view, Rect_Int& rect) {
	show_caret = ticks % (caret_on_time + caret_off_time) < caret_on_time;
}

Render_Clip Text_Editor::make_clip(Rect_Int& r, float edge) {
	return {
		CLIP_ALL,
		r.x + edge,
		r.y + edge,
		r.x + r.w - edge,
		r.y + r.h - edge
	};
}

void Text_Editor::draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	sdl_draw_rect(back, default_color, renderer);

	float digit_w = font->render.digit_width();
	float font_h = font->render.text_height();
	float edge = border * view.scale;

	clip = make_clip(back, edge);

	float scroll_x = 0, scroll_y = 0;
	if (hscroll) {
		float view_w = back.w - 2*edge;
		hscroll->set_maximum((float)editor.columns * digit_w, view_w);
		scroll_x = hscroll->position;
	}
	if (vscroll) {
		float view_h = back.h - 2*edge;
		vscroll->set_maximum((float)editor.lines * font_h, view_h);
		scroll_y = vscroll->position;
	}

	editor.refresh(clip, font, scroll_x, scroll_y);

	if (editor.secondary.cursor != editor.primary.cursor)
		editor.draw_selection_box(renderer, sel_color);

	font->render.draw_text(renderer, editor.text.c_str(), back.x + edge - scroll_x, back.y + edge - scroll_y, clip, editor.tab_width, true);
}

void Text_Editor::post_draw(Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {
	if (parent->active_edit != &editor)
		return;

	float edge = border * view.scale;
	Render_Clip cur_clip = make_clip(back, edge);

	float digit_w = font->render.digit_width();
	float font_h = font->render.text_height();

	if (editor.secondary.cursor != editor.primary.cursor || show_caret) {
		Point origin = {back.x + edge, back.y + edge};
		editor.draw_cursor(nullptr, caret_color, editor.primary, origin, cur_clip, view.scale);
		if (editor.secondary.cursor != editor.primary.cursor)
			editor.draw_cursor(nullptr, caret_color, editor.secondary, origin, cur_clip, view.scale);
	}
}

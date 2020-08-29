#include "muscles.h"
#include "ui.h"

static inline float clamp(float f, float min, float max) {
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

Rect UI_Element::make_ui_box(Rect_Int& box, Rect& elem, float scale) {
	if (soft_draw) {
		return {
			.x = (elem.x - pos.x) * scale,
			.y = (elem.y - pos.y) * scale,
			.w = elem.w * scale,
			.h = elem.h * scale
		};
	}

	return {
		.x = (float)box.x + elem.x * scale,
		.y = (float)box.y + elem.y * scale,
		.w = elem.w * scale,
		.h = elem.h * scale
	};
}

void UI_Element::draw(Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	update();

	// Some UI elements are more expensive to draw than others.
	// For the cheaper ones, we don't bother with surface caching, we just draw them and return.
	bool drawn = true;
	switch (elem_type) {
		case ElemButton:
			dynamic_cast<Button*>(this)->draw_button(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemCheckbox:
			dynamic_cast<Checkbox*>(this)->draw_checkbox(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemDivider:
			dynamic_cast<Divider*>(this)->draw_divider(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemDropDown:
			dynamic_cast<Drop_Down*>(this)->draw_dropdown(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemEditBox:
			dynamic_cast<Edit_Box*>(this)->draw_editbox(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemHexView:
			dynamic_cast<Hex_View*>(this)->draw_hexview(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemImage:
			dynamic_cast<Image*>(this)->draw_image(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemLabel:
			dynamic_cast<Label*>(this)->draw_label(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		case ElemScroll:
			dynamic_cast<Scroll*>(this)->draw_scroll(nullptr, view, rect, elem_hovered, box_hovered, focussed);
			break;
		default:
			drawn = false;
	}
	if (drawn) {
		post_draw(view, rect, elem_hovered, box_hovered, focussed);
		return;
	}

	int w = 0.5 + pos.w * view.scale;
	int h = 0.5 + pos.h * view.scale;

	bool draw_existing = false;
	Renderer renderer = nullptr;
	Renderer hw = sdl_get_hw_renderer();
	soft_draw = false;

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
		if (reify_timer < hw_draw_timeout)
			renderer = hw;

		else if (!tex_cache) {
			renderer = sdl_acquire_sw_renderer(w, h);
			soft_draw = true;
		}
		else {
			// this means that we don't need to render a new frame and can simply use the existing surface instead
			draw_existing = true;
		}
	}

	if (!draw_existing) {
		Rect_Int space = rect;
		if (soft_draw)
			space = {0, 0, w, h};

		switch (elem_type) {
			case ElemDataView:
				dynamic_cast<Data_View*>(this)->draw_dataview(renderer, view, space, elem_hovered, box_hovered, focussed);
				break;
			case ElemTextEditor:
				dynamic_cast<Text_Editor*>(this)->draw_texteditor(renderer, view, space, elem_hovered, box_hovered, focussed);
				break;
		}

		if (soft_draw) {
			if (tex_cache)
				sdl_destroy_texture(&tex_cache);

			tex_cache = sdl_bake_sw_render();
			draw_existing = true;
		}
	}

	soft_draw = false;

	if (draw_existing) {
		Rect_Int back = make_int_ui_box(rect, pos, view.scale);
		sdl_apply_texture(tex_cache, back, nullptr, nullptr);
		//sdl_log_last_error();
	}

	post_draw(view, rect, elem_hovered, box_hovered, focussed);
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

void Button::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	bool was_held = held;
	held = input.lmouse && pos.contains(cursor);
	needs_redraw = was_held != held;
}

void Button::draw_button(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
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

	sdl_draw_rect(r, *color, renderer);

	if (!theme->font) {
		if (icon)
			sdl_apply_texture(icon, r, nullptr, renderer);
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

		sdl_apply_texture(icon, box, nullptr, renderer);
	}

	theme->font->render.draw_text_simple(renderer, text.c_str(), r.x + x, r.y + y_offset * font_height);
}

void Checkbox::toggle() {
	checked = !checked;
	needs_redraw = true;
}

void Checkbox::draw_checkbox(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	if (elem_hovered) {
		Rect back = make_ui_box(rect, pos, view.scale);
		sdl_draw_rect(back, hl_color, renderer);
	}

	Rect box = make_ui_box(rect, pos, view.scale);
	box.w = box.h;
	sdl_draw_rect(box, default_color, renderer);

	if (checked) {
		int w = 0.5 + box.w * border_frac;
		int h = 0.5 + box.h * border_frac;
		Rect check = {
			box.x + w,
			box.y + h,
			box.w - 2*w,
			box.h - 2*h
		};
		sdl_draw_rect(check, sel_color, renderer);
	}

	float font_height = font->render.text_height();
	float gap_x = text_off_x * font_height;
	float gap_y = text_off_y * font_height;

	float text_x = box.x + box.w + gap_x;
	float text_y = box.y + gap_y;

	font->render.draw_text_simple(renderer, text.c_str(), text_x, text_y);
}

void Data_View::update_tree(std::vector<Branch> *new_branches) {
	if (new_branches) {
		branches.resize(new_branches->size());
		std::copy(new_branches->begin(), new_branches->end(), branches.begin());
	}

	int n_rows = data->row_count();
	int n_branches = branches.size();
	tree.clear();
	tree.reserve(n_rows);

	int b = 0;
	for (int i = 0; i < n_rows; i++) {
		if (b < n_branches && branches[b].row_idx == i) {
			if (branches[b].closed)
				i = branches[b].row_idx + branches[b].length;

			tree.push_back(-b - 1);
			b++;
			i--;
			continue;
		}

		tree.push_back(i);
	}
}

int Data_View::get_table_index(int view_idx) {
	if (view_idx < 0)
		return view_idx;

	int idx = -1;
	int tree_size = tree.size();

	if (tree_size > 0) {
		if (view_idx < tree_size)
			idx = tree[view_idx];
	}
	else if (view_idx < data->row_count())
		idx = view_idx;

	return idx;
}

bool Data_View::highlight(Camera& view, Point& inside) {
	if (!font) {
		hl_row = hl_col = -1;
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

	int n_rows = tree.size() > 0 ? tree.size() : data->row_count();
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
					break;
				}

				x += w + column_spacing * font_units;
			}

			hl = true;
			break;
		}
		y += item_height;
	}

	needs_redraw = hl_row != old_hl_row || hl_col != old_hl_col;
	return hl;
}

float Data_View::column_width(float total_width, float min_width, float font_height, float scale, int idx) {
	float w = data->headers[idx].width;
	if (w == 0)
		return (total_width - min_width) * scale;

	w *= total_width * scale;

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

void Data_View::draw_item_backing(Renderer renderer, RGBA& color, Rect& back, float scale, int idx) {
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
			back.y + y,
			back.w,
			h
		};
		sdl_draw_rect(r, color, renderer);
	}
}

void Data_View::draw_dataview(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	int n_rows = data->row_count();
	int n_cols = data->column_count();
	int tree_size = tree.size();

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

	if (hscroll) {
		for (int i = 0; i < n_cols; i++)
			scroll_total += data->headers[i].min_size + column_spacing;
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
	float x_start = back.x + col_space - scroll_x;

	if (show_column_names) {
		Rect header = {back.x, 0, back.w, header_height * view.scale};
		header.y = back.y - header.h;
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
	float line_h = font_height + (font->line_spacing * font_height);
	float sp_half = font->line_spacing * font_height / 2;

	Rect_Int dst = {0, 0, font_height, font_height};
	Rect_Int src = {0};

	clip.x_lower = x + scroll_x;
	clip.y_upper = y_max;

	auto draw_cell =
	[this, &renderer, &dst, &src, y_max, line_off, sp_half, font_height]
	(void *cell, Column_Type type, float x, float y) {
		if (type == ColumnString)
			font->render.draw_text(renderer, (const char*)cell, x, y - line_off, clip);
		else if (type == ColumnFile) {
			auto name = (const char*)((File_Entry*)cell)->name;
			font->render.draw_text(renderer, name, x, y - line_off, clip);
		}
		else if (type == ColumnImage) {
			sdl_get_texture_size(cell, &src.w, &src.h);
			dst.x = x;
			dst.y = y + sp_half;

			if (dst.y + dst.h > y_max) {
				int h = y_max - dst.y;
				src.h = (float)(src.h * h) / (float)dst.h;
				dst.h = h;
			}

			sdl_apply_texture(cell, dst, &src, renderer);
		}
		else if (type == ColumnCheckbox) {
			float indent = font_height * 0.1;
			dst.x = x + 0.5 + indent;
			dst.y = y + sp_half + 0.5 + indent;
			dst.w = 0.5 + font_height - 2*indent;
			dst.h = dst.w;

			if (dst.y + dst.h > y_max)
				dst.h = y_max - dst.y;

			sdl_draw_rect(dst, parent->parent->table_cb_back, renderer);

			if (*(u8*)&cell == 2) {
				indent = font_height * 0.15;
				src.x = dst.x + 0.5 + indent;
				src.y = dst.y + 0.5 + indent;
				src.w = 0.5 + dst.w - 2*indent;
				src.h = src.w;

				if (src.y + src.h > y_max)
					src.h = y_max - src.y;

				sdl_draw_rect(src, parent->parent->cb_color, renderer);
				src.x = src.y = 0;
			}

			dst.w = dst.h = font_height;
		}
	};

	float table_off_y = y;

	for (int i = 0; i < n_cols && x < back.x + back.w; i++) {
		float w = column_width(pos.w, scroll_total, font_height, view.scale, i);
		clip.x_upper = x+w < x_max ? x+w : x_max;

		Column_Type type = data->headers[i].type;

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
				if (thing && !skip_draw)
					draw_cell(thing, type, x, y);

				y += line_h;
			}
		}
		else {
			for (int j = top; y < y_max; j++) {
				int idx = j;
				if (tree_size) {
					if (j >= tree_size)
						break;
					idx = tree[j];
				}
				else if (j >= n_rows)
					break;

				if (idx < 0 && i == 0) {
					int branch = -idx - 1;
					Texture icon = branches[branch].closed ? icon_plus : icon_minus;
					draw_cell(icon, ColumnImage, x, y + font_height * 0.025);

					int name_idx = branches[branch].name_idx;
					if (name_idx >= 0)
						font->render.draw_text(renderer, branch_name_vector.at(name_idx), x + font_height * 1.2, y - line_off, clip);
				}
				else if (idx >= 0) {
					bool skip_draw = false;
					if (condition_col >= 0) {
						if (!TABLE_CHECKBOX_CHECKED(data, condition_col, idx))
							continue;

						skip_draw = condition_col == i;
					}

					auto thing = data->columns[i][idx];
					if (thing && !skip_draw)
						draw_cell(thing, type, x, y);
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
			if (scroll && input.scroll_y > 0) {
				scroll->scroll(-1);
				needs_redraw = true;
			}
			if (scroll && input.scroll_y < 0) {
				scroll->scroll(1);
				needs_redraw = true;
			}
		}
		int row = get_table_index(hl_row);
		if (input.lclick) {
			if (hl_col >= 0 && row >= 0 && data->headers[hl_col].type == ColumnCheckbox) {
				TOGGLE_TABLE_CHECKBOX(data, hl_col, row);
			}
			else if (hl_row >= 0 && row < 0 && tree.size() > 0) {
				int b = -row - 1;
				branches[b].closed = !branches[b].closed;
				update_tree();
			}
			needs_redraw = true;
		}
	}
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
		position = clamp(cur - hold_pos, minimum, maximum);
	}

	needs_redraw = true;
}

void Divider::draw_divider(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect r = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(r, default_color, renderer);

	if (icon) {
		Rect box = {
			r.x + (r.w - (float)icon_w) / 2,
			r.y + (r.h - (float)icon_h) / 2,
			icon_w,
			icon_h
		};
		sdl_apply_texture(icon, box, nullptr, renderer);
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

void Drop_Down::draw_menu(Renderer renderer, Camera& view, Rect_Int& rect) {
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

	sdl_draw_rect(box, hl_color, renderer);

	if (sel >= 0) {
		Rect hl = box;
		hl.y += line_height * sel * font_height;
		hl.h = (title_off_y + line_height) * font_height + 1;
		sdl_draw_rect(hl, sel_color, renderer);
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

void Drop_Down::draw_dropdown(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);

	if (elem_hovered || dropped)
		sdl_draw_rect(box, hl_color, renderer);
	else
		sdl_draw_rect(box, default_color, renderer);

	float gap_y = title_off_y * font->render.text_height();

	float text_w = font->render.text_width(title);
	float text_x = pos.x * view.scale + (pos.w * view.scale - text_w) / 2;
	float text_y = pos.y * view.scale + gap_y;

	font->render.draw_text_simple(renderer, title, rect.x + text_x, rect.y + text_y);
}

void Edit_Box::clear() {
	editor.set_cursor(editor.primary, 0);
	editor.text.clear();
	//offset = 0;
}

bool Edit_Box::disengage(Input& input, bool try_select) {
	float cancel_lclick = false;
	if (dropdown) {
		if (try_select && dropdown->sel >= 0) {
			editor.set_cursor(editor.primary, 0);
			editor.text = (*dropdown->get_content())[dropdown->sel];
			//offset = 0;
			cancel_lclick = true;

			if (key_action)
				key_action(this, input);
		}
		dropdown->dropped = false;
	}
	return cancel_lclick;
}

void Edit_Box::key_handler(Camera& view, Input& input) {
	/*
	if (delta) {
		editor.set_cursor(editor.primary, editor.primary.cursor + delta);

		float x = 0;
		const char *str = line.c_str();
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
	*/

	text_changed = false;

	if (this != parent->active_edit)
		return;

	int res = editor.handle_input(input);

	if ((res & 1) && key_action) {
		text_changed = true;
		key_action(this, input);
	}

	if (res & 2)
		needs_redraw = true;

	if (res & 8) {
		clear();
		disengage(input, false);
		parent->active_edit = nullptr;
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

void Edit_Box::update_icon(Icon_Type type, float height, float scale) {
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

void Edit_Box::draw_editbox(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect back = make_ui_box(rect, pos, view.scale);
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

	float text_x = pos.x * view.scale + x;
	float text_y = pos.y * view.scale + gap_y;
	float caret_y = (float)caret_off_y * height;
	float window_w = (pos.w * view.scale) - x - icon_w - gap_x;

	clip.x_lower = rect.x + text_x;
	clip.x_upper = clip.x_lower + window_w;
	float offset = 0;

	int first_col = editor.primary.column;
	int second_col = editor.secondary.column;
	if (first_col != second_col) {
		if (first_col > second_col) {
			int temp = first_col;
			first_col = second_col;
			second_col = temp;
		}
		float digit_w = fnt->render.digit_width();
		Rect r = {
			clip.x_lower + first_col * digit_w,
			rect.y + text_y + caret_y,
			(second_col - first_col) * digit_w,
			height + 1
		};
		sdl_draw_rect(r, sel_color, renderer);
	}

	fnt->render.draw_text(renderer, str, rect.x + text_x - offset, rect.y + text_y, clip);

	if (this == parent->active_edit) {
		float caret_w = (float)caret_width * height;

		Rect_Int r = {
			rect.x + text_x + cur_x - offset,
			rect.y + text_y + (int)caret_y + gap_y,
			(int)caret_w,
			(int)height
		};

		sdl_draw_rect(r, caret, renderer);
	}

	if (!dropdown)
		return;

	dropdown->pos = pos;
	if (dropdown->dropped)
		dropdown->draw_menu(renderer, view, rect);
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

	if (alive) {
		if (span_idx < 0)
			span_idx = source->request_span();

		Span& s = source->spans[span_idx];
		s.address = region_address + offset;
		s.size = rows * columns;
	}
}

float Hex_View::print_address(Renderer renderer, u64 address, float x, float y, float digit_w, Render_Clip& clip, Rect& box, float pad) {
	int n_digits = count_digits(address);
	float text_x = x + (addr_digits - n_digits) * digit_w;

	char buf[20];
	print_hex("0123456789abcdef", buf, address, n_digits);
	font->render.draw_text(renderer, buf, box.x + text_x, box.y + y, clip);

	return x + addr_digits * digit_w + 2*pad;
}

float Hex_View::print_hex_row(Renderer renderer, Span& span, int idx, float x, float y, Rect& box, float pad) {
	Rect_Int src, dst;

	for (int i = 0; i < columns * 2; i++) {
		const Glyph *gl = nullptr;
		int digit = 0;
		if (i < span.retrieved * 2) {
			digit = span.data[idx + i/2];
			if (i % 2 == 0)
				digit >>= 4;
			digit &= 0xf;

			char c = digit <= 9 ? '0' + digit : 'a' - 10 + digit;
			gl = &font->render.glyphs[c - MIN_CHAR];
		}
		else
			gl = &font->render.glyphs['?' - MIN_CHAR];

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

		sdl_apply_texture(font->render.tex, dst, &src, renderer);

		x += gl->box_w;
		if (i % 2 == 1)
			x += pad;

		if (x >= box.w)
			break;
	}

	return x + pad;
}

void Hex_View::print_ascii_row(Renderer renderer, Span& span, int idx, float x, float y, Render_Clip& clip, Rect& box) {
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

void Hex_View::draw_cursors(Renderer renderer, int idx, Rect& back, float x_start, float pad, float scale) {
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

void Hex_View::draw_hexview(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);

	if (!alive || span_idx < 0 || rows <= 0 || columns <= 0) {
		sdl_draw_rect(box, default_color, renderer);
		return;
	}

	Rect_Int src, dst;

	font_height = font->render.text_height();

	float x_start = x_offset * view.scale;
	float x = x_start;
	float y = font_height;
	float pad = padding * font_height;

	float digit_w = font->render.digit_width();
	float addr_w = addr_digits * digit_w + 2*pad;

	Rect back = box;
	if (show_addrs) {
		Rect addr_back = back;
		addr_back.w = addr_w + 1;
		sdl_draw_rect(addr_back, back_color, renderer);

		back.x += addr_w;
		back.w -= addr_w;
	}
	sdl_draw_rect(back, default_color, renderer);

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
			x = print_address(renderer, region_address + offset + idx, x, y - font_height, digit_w, addr_clip, box, pad);
		if (show_hex && x < box.w)
			x = print_hex_row(renderer, span, idx, x, y, box, pad);
		if (show_ascii && x < box.w)
			print_ascii_row(renderer, span, idx, x, y - font_height, ascii_clip, box);

		x = x_start;
		y += row_height;
		left -= columns;
	}

	if (sel >= 0 && sel >= offset && sel < offset + span.size)
		draw_cursors(renderer, sel - offset, back, x_start, pad, view.scale);
}

void Hex_View::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (!hovered || !pos.contains(cursor))
		return;

	if (scroll) {
		int delta = columns;
		if (input.lshift || input.rshift)
			delta *= rows;

		if (input.scroll_y > 0) {
			scroll->scroll(-delta);
			needs_redraw = true;
		}
		if (input.scroll_y < 0) {
			scroll->scroll(delta);
			needs_redraw = true;
		}
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

		needs_redraw = true;
	}
}

void Image::draw_image(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect box = make_ui_box(rect, pos, view.scale);
	sdl_apply_texture(img, box, nullptr, renderer);
}

void Label::update_position(float scale) {
	float font_height = font->render.text_height();
	float pad = padding * font_height;

	x = pos.x * scale + pad;
	y = pos.y * scale - pad;
	width = pos.w * scale - pad;
}

void Label::draw_label(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	clip.x_upper = rect.x + x + width;
	font->render.draw_text(renderer, text.c_str(), rect.x + x, rect.y + y, clip);
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
	int old_pos = position;
	position = clamp(position + delta, 0, maximum - view_span);
	needs_redraw = position != old_pos;
	if (content)
		content->needs_redraw = needs_redraw;
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

void Scroll::draw_scroll(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	if (vertical)
		length = pos.h - 2*padding;
	else
		length = pos.w - 2*padding;

	Rect bar = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(bar, back, renderer);

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

	Rect_Int scroll_rect = {
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

	sdl_draw_rect(scroll_rect, *color, renderer);
}

bool Scroll::highlight(Camera& view, Point& inside) {
	hl = pos.contains(inside);
	return hl;
}

void Scroll::deselect() {
	hl = false;
}

void Text_Editor::key_handler(Camera& view, Input& input) {
	if (this == parent->active_edit) {
		int res = editor.handle_input(input);
		if ((res & 1) && key_action)
			key_action(this, input);
		if (res & 2)
			needs_redraw = true;
		if (res & 4)
			ticks = 0;
		if (res & 8)
			parent->active_edit = nullptr;
	}
}

void Text_Editor::mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {
	if (hovered && input.lclick && pos.contains(cursor)) {
		parent->ui_held = true;
		parent->active_edit = this;
		mouse_held = true;
		needs_redraw = true;
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

		editor.set_line(editor.primary, line);
		editor.set_column(editor.primary, col);

		if (input.lclick)
			editor.secondary = editor.primary;
	}
}

void Text_Editor::draw_selection_box(Renderer renderer, Render_Clip& clip, float digit_w, float font_h, float line_pad) {
	Editor::Cursor *first  = editor.primary.cursor < editor.secondary.cursor ? &editor.primary : &editor.secondary;
	Editor::Cursor *second = editor.primary.cursor < editor.secondary.cursor ? &editor.secondary : &editor.primary;

	float y = clip.y_lower + line_pad;
	Rect r = {0};
	if (first->line == second->line) {
		r.x = clip.x_lower + first->column * digit_w;
		r.y = y + first->line * font_h;
		r.w = (second->column - first->column) * digit_w;
		r.h = font_h + 1;
		sdl_draw_rect(r, sel_color, renderer);
	}
	else {
		float line_w = clip.x_upper - clip.x_lower;

		float x = first->column * digit_w;
		r.x = clip.x_lower + x;
		r.y = y + first->line * font_h;
		r.w = line_w - x;
		r.h = font_h + 1;
		sdl_draw_rect(r, sel_color, renderer);

		int gulf = second->line - first->line - 1;
		if (gulf > 0) {
			r.x = clip.x_lower;
			r.y += font_h;
			r.w = line_w;
			r.h = gulf * font_h + 1;
			sdl_draw_rect(r, sel_color, renderer);
		}

		if (second->column > 0) {
			r.x = clip.x_lower;
			r.y = y + second->line * font_h;
			r.w = second->column * digit_w;
			r.h = font_h + 1;
			sdl_draw_rect(r, sel_color, renderer);
		}
	}
}

void Text_Editor::draw_cursor(Renderer renderer, Editor::Cursor& cursor, Rect& back, float digit_w, float font_h, float line_pad, float edge, float scale) {
	float caret_x = back.x + edge + ((float)cursor.column * digit_w);
	float caret_y = back.y + edge + line_pad + ((float)cursor.line * font_h) + (line_pad / 2);

	Rect caret = { caret_x, caret_y, cursor_width * scale, font_h - line_pad };
	sdl_draw_rect(caret, caret_color, renderer);
}

void Text_Editor::update() {
	ticks++;
	show_caret = ticks % (caret_on_time + caret_off_time) < caret_on_time;
}

void Text_Editor::draw_texteditor(Renderer renderer, Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	Rect back = make_ui_box(rect, pos, view.scale);
	sdl_draw_rect(back, default_color, renderer);

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

	if (editor.secondary.cursor != editor.primary.cursor)
		draw_selection_box(renderer, clip, digit_w, font_h, line_pad);

	font->render.draw_text(renderer, editor.text.c_str(), back.x + edge - scroll_x, back.y + edge - scroll_y, clip, editor.tab_width, true);
}

void Text_Editor::post_draw(Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed) {
	if (this != parent->active_edit)
		return;

	Rect back = make_ui_box(rect, pos, view.scale);

	float edge = border * view.scale;
	float digit_w = font->render.digit_width();
	float font_h = font->render.text_height();
	float line_pad = font_h * 0.15f;

	if (editor.secondary.cursor != editor.primary.cursor || show_caret) {
		draw_cursor(nullptr, editor.primary, back, digit_w, font_h, line_pad, edge, view.scale);
		if (editor.secondary.cursor != editor.primary.cursor)
			draw_cursor(nullptr, editor.secondary, back, digit_w, font_h, line_pad, edge, view.scale);
	}
}
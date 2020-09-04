#include "muscles.h"
#include "ui.h"
#include "dialog.h"

void View_Object::update_ui(Camera& camera) {
	cross.pos.y = cross_size * 0.5;
	cross.pos.x = box.w - cross_size * 1.5;
	cross.pos.w = cross_size;
	cross.pos.h = cross_size;

	float title_w = 200;
	float title_x = (cross.pos.x - title_w) / 2;
	if (title_x < border) {
		title_x = border;
		title_w = cross.pos.x - title_x - 2*border;
	}

	title_edit.pos = {
		title_x,
		2,
		title_w,
		title_edit.font->render.text_height() * 1.4f / camera.scale
	};

	float meta_y = title_edit.pos.h + 4*border;
	hide_meta.pos = {
		cross.pos.x + (cross.pos.w - meta_btn_width),
		meta_y,
		meta_btn_width,
		meta_btn_height
	};

	float label_x = 2*border;
	float y = meta_y;

	if (!meta_hidden) {
		float label_h = struct_label.font->render.text_height() * 1.2f / camera.scale;

		float edit_h = get_edit_height(camera.scale);
		float edit_gap = edit_h + border;
		float edit_y = meta_y - (edit_h - label_h) / 2;

		struct_label.pos = {
			label_x,
			y,
			80,
			label_h
		};
		y += edit_gap;

		source_label.pos = {
			label_x,
			y,
			80,
			label_h
		};
		y += edit_gap;

		addr_label.pos = {
			label_x,
			y,
			100,
			label_h
		};

		struct_label.update_position(camera.scale);
		source_label.update_position(camera.scale);
		addr_label.update_position(camera.scale);

		y = edit_y;
		struct_edit.pos = {
			label_x + struct_label.pos.w + border,
			y,
			160,
			edit_h
		};
		y += edit_gap;

		source_edit.pos = {
			label_x + source_label.pos.w + border,
			y,
			160,
			edit_h
		};
		y += edit_gap;

		addr_edit.pos = {
			label_x + addr_label.pos.w + border,
			y,
			140,
			edit_h
		};

		y += edit_gap + border;
	}
	else
		y += meta_btn_height + 2*border;

	float scroll_w = 14;
	float view_w = box.w - 2*label_x - scroll_w - border;
	float view_h = box.h - y - scroll_w - all_btn.height - 4*border;

	vscroll.pos = {
		label_x + view_w + border,
		y,
		scroll_w,
		view_h
	};

	hscroll.pos = {
		label_x,
		y + view_h + border,
		view_w,
		scroll_w
	};

	view.pos = {
		label_x,
		y,
		view_w,
		view_h
	};

	all_btn.pos = {
		label_x,
		box.h - 2*border - all_btn.height,
		all_btn.width,
		all_btn.height
	};

	sel_btn.pos = {
		all_btn.pos.x + all_btn.pos.w,
		all_btn.pos.y,
		sel_btn.width,
		sel_btn.height
	};
}

float View_Object::get_edit_height(float scale) {
	return struct_edit.font->render.text_height() * 1.4f / scale;
}

void View_Object::update_hide_meta_button(float scale) {
	float pad = hide_meta.padding;
	int meta_w = 0.5 + (meta_btn_width - 2*pad) * scale;
	int meta_h = 0.5 + (meta_btn_height - 2*pad) * scale;

	sdl_destroy_texture(&hide_meta.icon);
	hide_meta.icon = make_triangle(hide_meta.default_color, meta_w, meta_h, !meta_hidden);
}

void View_Object::select_view_type(bool all) {
	all = all;

	auto theme_all = all ? &theme_on : &theme_off;
	auto theme_sel = all ? &theme_off : &theme_on;

	all_btn.active_theme = *theme_all;
	all_btn.inactive_theme = *theme_all;
	sel_btn.active_theme = *theme_sel;
	sel_btn.inactive_theme = *theme_sel;

	view.condition_col = all ? -1 : 0;
	view.needs_redraw = true;
}

void struct_edit_handler(Edit_Box *edit, Input& input) {
	//Box *structs_box = edit->parent->parent->first_box_of_type(BoxStructs);
	/*if (structs_box)*/ {
		auto ui = dynamic_cast<View_Object*>(edit->parent);
		Workspace *ws = edit->parent->parent;
		populate_object_table(ui, ws->structs, ws->name_vector);
	}
}

void source_edit_handler(Edit_Box *edit, Input& input) {
	auto ui = dynamic_cast<View_Object*>(edit->parent);

	Source *src = nullptr;

	if (edit->editor.text.size() > 0) {
		Workspace& ws = *edit->parent->parent;
		for (auto& s : ws.sources) {
			if (s->name == edit->editor.text) {
				src = s;
				break;
			}
		}
	}

	if (src != ui->source || !src) {
		if (ui->source && ui->span_idx >= 0)
			ui->source->deactivate_span(ui->span_idx);

		ui->source = src;
		ui->span_idx = src->request_span();
	}

	ui->view.needs_redraw = true;
}

void View_Object::refresh(Point& cursor) {
	Workspace *ws = parent;
	Box *structs_box = ws->first_box_of_type(BoxStructs);

	struct_dd.external = &ws->struct_names;
	if (!record || !source)
		return;

	Span& span = source->spans[span_idx];

	span.address = strtoull(addr_edit.editor.text.c_str(), nullptr, 16);
	span.size = (record->total_size + 7) / 8;

	int idx = 0;
	for (auto& row : view.data->columns[1]) {
		auto name = (const char*)row;
		if (!name) {
			idx++;
			continue;
		}

		for (int i = 0; i < record->fields.n_fields; i++) {
			Field& field = record->fields.data[i];
			char *field_name = ws->name_vector.at(field.field_name_idx);

			if (!field_name || strcmp(field_name, name))
				continue;

			if (field.array_len > 0 || field.flags & FLAG_COMPOSITE || field.bit_size > 64 || field.bit_size <= 0)
				break;

			auto& cell = (char*&)view.data->columns[2][idx];

			int offset = (field.bit_offset + 7) / 8;
			if (offset >= span.retrieved) {
				strcpy(cell, "???");
				break;
			}

			u64 n = 0;
			int off = field.bit_offset;
			int out_pos = field.bit_size - 1;
			for (int j = 0; j < field.bit_size; j++, off++, out_pos--) {
				u8 byte = span.data[off / 8];
				int bit = 7 - (off % 8);
				n |= (u64)((byte >> bit) & 1) << out_pos;
			}

			strcpy(cell, "0x");

			int digits = count_hex_digits(n);
			write_hex(&cell[2], n, digits);

			break;
		}
		idx++;
	}
}

void View_Object::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;

	update_hide_meta_button(new_scale);

	float height = get_edit_height(new_scale);
	struct_edit.update_icon(IconTriangle, height, new_scale);
	source_edit.update_icon(IconTriangle, height, new_scale);
}

View_Object::View_Object(Workspace& ws) {
	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	title_edit.placeholder = "<instance>";
	title_edit.ph_font = ws.make_font(12, ws.ph_text_color);
	title_edit.font = ws.default_font;
	title_edit.caret = ws.caret_color;
	title_edit.caret.a = 0.6;
	title_edit.default_color = ws.dark_color;
	ui.push_back(&title_edit);

	hide_meta.padding = 0;
	hide_meta.action = [](UI_Element *elem, bool dbl_click) {
		auto ui = dynamic_cast<View_Object*>(elem->parent);
		ui->meta_hidden = !ui->meta_hidden;
		ui->struct_label.visible = ui->struct_edit.visible = !ui->meta_hidden;
		ui->source_label.visible = ui->source_edit.visible = !ui->meta_hidden;
		ui->addr_label.visible = ui->addr_edit.visible = !ui->meta_hidden;
		ui->update_hide_meta_button(elem->parent->parent->temp_scale);
	};
	hide_meta.active_theme = {
		ws.light_color,
		ws.light_hl_color,
		ws.light_hl_color,
		nullptr
	};
	hide_meta.inactive_theme = hide_meta.active_theme;
	hide_meta.default_color = ws.text_color;
	hide_meta.default_color.a = 0.8;
	ui.push_back(&hide_meta);

	hscroll.back = ws.scroll_back;
	hscroll.default_color = ws.scroll_color;
	hscroll.hl_color = ws.scroll_hl_color;
	hscroll.sel_color = ws.scroll_sel_color;
	hscroll.vertical = false;
	ui.push_back(&hscroll);

	vscroll.back = ws.scroll_back;
	vscroll.default_color = ws.scroll_color;
	vscroll.hl_color = ws.scroll_hl_color;
	vscroll.sel_color = ws.scroll_sel_color;
	ui.push_back(&vscroll);

	view.font = ws.default_font;
	view.default_color = ws.dark_color;
	view.hl_color = ws.dark_color;
	view.hscroll = &hscroll;
	view.hscroll->content = &view;
	view.vscroll = &vscroll;
	view.vscroll->content = &view;

	Column cols[] = {
		{ColumnCheckbox, 0, 0.05, 1.0, 1.0, ""},
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 20, 0.45, 0, 0, "Value"}
	};

	Arena *arena = &ws.object_arena;
	arena->set_rewind_point();
	table.init(cols, arena, 3, 0);
	view.data = &table;

	ui.push_back(&view);

	struct_label.font = ws.make_font(11, ws.text_color);
	struct_label.text = "Struct:";
	struct_label.padding = 0;
	ui.push_back(&struct_label);

	source_label.font = struct_label.font;
	source_label.text = "Source:";
	source_label.padding = 0;
	ui.push_back(&source_label);

	addr_label.font = struct_label.font;
	addr_label.text = "Address:";
	addr_label.padding = 0;
	ui.push_back(&addr_label);

	auto mm = dynamic_cast<Main_Menu*>(ws.first_box_of_type(BoxMain));
	RGBA icon_color = ws.text_color;
	icon_color.a = 0.7;

	addr_edit.font = addr_label.font;
	addr_edit.caret = ws.caret_color;
	addr_edit.default_color = ws.dark_color;
	ui.push_back(&addr_edit);

	source_dd.visible = false;
	source_dd.font = source_label.font;
	source_dd.default_color = ws.back_color;
	source_dd.hl_color = ws.hl_color;
	source_dd.sel_color = ws.active_color;
	source_dd.width = 250;
	source_dd.external = (std::vector<char*>*)&mm->sources_view.data->columns[2];
	ui.push_back(&source_dd);

	source_edit.font = source_label.font;
	source_edit.caret = ws.caret_color;
	source_edit.default_color = ws.dark_color;
	source_edit.icon_color = icon_color;
	source_edit.icon_right = true;
	source_edit.dropdown = &source_dd;
	source_edit.key_action = source_edit_handler;
	ui.push_back(&source_edit);

	struct_dd.visible = false;
	struct_dd.font = struct_label.font;
	struct_dd.default_color = ws.back_color;
	struct_dd.hl_color = ws.hl_color;
	struct_dd.sel_color = ws.active_color;
	struct_dd.width = 250;
	ui.push_back(&struct_dd);

	struct_edit.font = struct_label.font;
	struct_edit.caret = ws.caret_color;
	struct_edit.default_color = ws.dark_color;
	struct_edit.icon_color = icon_color;
	struct_edit.icon_right = true;
	struct_edit.dropdown = &struct_dd;
	struct_edit.key_action = struct_edit_handler;
	ui.push_back(&struct_edit);

	theme_on = {
		ws.dark_color,
		ws.light_color,
		ws.light_color,
		ws.make_font(10, ws.text_color)
	};
	theme_off = {
		ws.back_color,
		ws.light_color,
		ws.light_color,
		theme_on.font
	};

	all_btn.padding = 0;
	all_btn.text = "All";
	all_btn.active_theme = theme_on;
	all_btn.inactive_theme = theme_on;
	all_btn.update_size(ws.temp_scale);
	all_btn.action = [](UI_Element *elem, bool dbl_click) { dynamic_cast<View_Object*>(elem->parent)->select_view_type(true); };
	ui.push_back(&all_btn);

	sel_btn.padding = 0;
	sel_btn.text = "Selected";
	sel_btn.active_theme = theme_off;
	sel_btn.inactive_theme = theme_off;
	sel_btn.update_size(ws.temp_scale);
	sel_btn.action = [](UI_Element *elem, bool dbl_click) { dynamic_cast<View_Object*>(elem->parent)->select_view_type(false); };
	ui.push_back(&sel_btn);

	refresh_every = 1;
	back = ws.back_color;
	edge_color = ws.dark_color;
	box = {-200, -225, 400, 450};
	min_width = 300;
	min_height = 200;
	expungable = true;
}

void populate_object_table(View_Object *ui, std::vector<Struct*>& structs, String_Vector& name_vector) {
	ui->view.data->clear_data();
	ui->view.data->arena->rewind();

	ui->record = nullptr;

	if (!ui->struct_edit.editor.text.size())
		return;

	const char *name_str = ui->struct_edit.editor.text.c_str();
	Struct *record = nullptr;
	for (auto& s : structs) {
		if (!s)
			continue;

		char *name = name_vector.at(s->name_idx);
		if (name && !strcmp(name, name_str)) {
			record = s;
			break;
		}
	}

	if (!record || (record->flags & (FLAG_UNUSABLE | FLAG_AVAILABLE)) != 0 || record->fields.n_fields <= 0)
		return;

	ui->record = record;
	int n_rows = ui->record->fields.n_fields;
	ui->view.data->resize(n_rows);

	for (int i = 0; i < n_rows; i++) {
		SET_TABLE_CHECKBOX(ui->view.data, 0, i, false);
		ui->view.data->columns[1][i] = (void*)name_vector.at(ui->record->fields.data[i].field_name_idx);
	}

	ui->view.needs_redraw = true;
}
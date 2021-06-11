#include "muscles.h"
#include "ui.h"
#include "dialog.h"

void View_Object::update_ui(Camera& camera) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

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

	object.pos = {
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

	object.condition_col = all ? -1 : 0;
	object.needs_redraw = true;
}

void struct_edit_handler(Edit_Box *edit, Input& input) {
	auto ui = dynamic_cast<View_Object*>(edit->parent);
	Workspace *ws = edit->parent->parent;
	populate_object_table<View_Object>(ui, ws->structs, ws->name_vector);
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
		ui->span_idx = src ? src->request_span() : -1;
	}

	ui->object.needs_redraw = true;
}

void view_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);
	table->sel_row = table->hl_row;
}

void View_Object::refresh(Point *cursor) {
	Workspace& ws = *parent;

	int n_sources = ws.sources.size();
	source_dd.content.resize(n_sources);
	for (int i = 0; i < n_sources; i++)
		source_dd.content[i] = (char*)ws.sources[i]->name.c_str();

	struct_dd.external = &ws.struct_names;
	if (!record || !source)
		return;

	Span& span = source->spans[span_idx];

	span.address = strtoull(addr_edit.editor.text.c_str(), nullptr, 16);
	span.size = (record->total_size + 7) / 8;

	field_vec.clear();

	int idx = -1;
	for (auto& row : object.data->columns[1]) {
		idx++;
		if (idx >= record->fields.n_fields)
			break;

		Field& field = record->fields.data[idx];
		int name_idx = get_full_field_name(field, ws.name_vector, field_vec);
		char *name = &field_vec.pool[name_idx];

		Bucket& buck = field_fmt.insert(name);
		Value_Format fmt = {};

		if (buck.flags & FLAG_NEW) {
			if (field.flags & FLAG_FLOAT)
				fmt.floatfmt = FloatAuto;

			buck.value = format.size();
			format.push_back(fmt);
		}
		else
			fmt = format[buck.value];

		format_field_value(field, fmt, span, (char*&)object.data->columns[2][idx], object.data->headers[2].count_per_cell);
	}
}

void View_Object::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	update_hide_meta_button(new_scale);

	float height = get_edit_height(new_scale);
	struct_edit.update_icon(IconTriangle, height, new_scale);
	source_edit.update_icon(IconTriangle, height, new_scale);
}

void View_Object::on_close() {
	source->deactivate_span(span_idx);
}

View_Object::View_Object(Workspace& ws, MenuType mtype) {
	float scale = get_default_camera().scale;

	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;
	ui.push_back(&maxm);

	title_edit.placeholder = "Instance";
	title_edit.ph_font = ws.make_font(12, ws.colors.ph_text, scale);
	title_edit.font = ws.default_font;
	title_edit.caret = ws.colors.caret;
	title_edit.caret.a = 0.6;
	title_edit.default_color = ws.colors.dark;
	ui.push_back(&title_edit);

	hide_meta.padding = 0;
	hide_meta.action = [](UI_Element *elem, Camera& view, bool dbl_click) {
		auto ui = dynamic_cast<View_Object*>(elem->parent);
		ui->meta_hidden = !ui->meta_hidden;
		ui->struct_label.visible = ui->struct_edit.visible = !ui->meta_hidden;
		ui->source_label.visible = ui->source_edit.visible = !ui->meta_hidden;
		ui->addr_label.visible = ui->addr_edit.visible = !ui->meta_hidden;
		ui->update_hide_meta_button(view.scale);
	};
	hide_meta.active_theme = {
		ws.colors.light,
		ws.colors.light_hl,
		ws.colors.light_hl,
		nullptr
	};
	hide_meta.inactive_theme = hide_meta.active_theme;
	hide_meta.default_color = ws.colors.text;
	hide_meta.default_color.a = 0.8;
	ui.push_back(&hide_meta);

	hscroll.back_color = ws.colors.scroll_back;
	hscroll.default_color = ws.colors.scroll;
	hscroll.hl_color = ws.colors.scroll_hl;
	hscroll.sel_color = ws.colors.scroll_sel;
	hscroll.vertical = false;
	ui.push_back(&hscroll);

	vscroll.back_color = ws.colors.scroll_back;
	vscroll.default_color = ws.colors.scroll;
	vscroll.hl_color = ws.colors.scroll_hl;
	vscroll.sel_color = ws.colors.scroll_sel;
	ui.push_back(&vscroll);

	object.font = ws.default_font;
	object.default_color = ws.colors.dark;
	object.hl_color = ws.colors.hl;
	object.sel_color = ws.colors.light;
	object.action = view_handler;
	object.use_sf_cache = false;
	object.hscroll = &hscroll;
	object.hscroll->content = &object;
	object.vscroll = &vscroll;
	object.vscroll->content = &object;

	Column cols[] = {
		{ColumnCheckbox, 0, 0.05, 1.0, 1.0, ""},
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 20, 0.45, 0, 0, "Value"}
	};

	Arena *arena = &ws.object_arena;
	arena->set_rewind_point();
	table.init(cols, arena, 3, 0);
	object.data = &table;

	ui.push_back(&object);

	struct_label.font = ws.make_font(11, ws.colors.text, scale);
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

	auto mm = ws.first_box_of_type<Main_Menu>();
	RGBA icon_color = ws.colors.text;
	icon_color.a = 0.7;

	addr_edit.font = addr_label.font;
	addr_edit.caret = ws.colors.caret;
	addr_edit.default_color = ws.colors.dark;
	ui.push_back(&addr_edit);

	source_dd.visible = false;
	source_dd.font = source_label.font;
	source_dd.default_color = ws.colors.back;
	source_dd.hl_color = ws.colors.hl;
	source_dd.sel_color = ws.colors.active;
	source_dd.width = 250;
	ui.push_back(&source_dd);

	source_edit.font = source_label.font;
	source_edit.caret = ws.colors.caret;
	source_edit.default_color = ws.colors.dark;
	source_edit.icon_color = icon_color;
	source_edit.icon_right = true;
	source_edit.dropdown = &source_dd;
	source_edit.key_action = source_edit_handler;
	ui.push_back(&source_edit);

	struct_dd.visible = false;
	struct_dd.font = struct_label.font;
	struct_dd.default_color = ws.colors.back;
	struct_dd.hl_color = ws.colors.hl;
	struct_dd.sel_color = ws.colors.active;
	struct_dd.width = 250;
	ui.push_back(&struct_dd);

	struct_edit.font = struct_label.font;
	struct_edit.caret = ws.colors.caret;
	struct_edit.default_color = ws.colors.dark;
	struct_edit.icon_color = icon_color;
	struct_edit.icon_right = true;
	struct_edit.dropdown = &struct_dd;
	struct_edit.key_action = struct_edit_handler;
	ui.push_back(&struct_edit);

	theme_on = {
		ws.colors.dark,
		ws.colors.light,
		ws.colors.light,
		ws.make_font(10, ws.colors.text, scale)
	};
	theme_off = {
		ws.colors.back,
		ws.colors.light,
		ws.colors.light,
		theme_on.font
	};

	all_btn.padding = 0;
	all_btn.text = "All";
	all_btn.active_theme = theme_on;
	all_btn.inactive_theme = theme_on;
	all_btn.update_size(scale);
	all_btn.action = [](UI_Element *elem, Camera&, bool dbl_click) { dynamic_cast<View_Object*>(elem->parent)->select_view_type(true); };
	ui.push_back(&all_btn);

	sel_btn.padding = 0;
	sel_btn.text = "Selected";
	sel_btn.active_theme = theme_off;
	sel_btn.inactive_theme = theme_off;
	sel_btn.update_size(scale);
	sel_btn.action = [](UI_Element *elem, Camera&, bool dbl_click) { dynamic_cast<View_Object*>(elem->parent)->select_view_type(false); };
	ui.push_back(&sel_btn);

	refresh_every = 1;
	initial_width = 400;
	initial_height = 450;
	min_width = 300;
	min_height = 200;
	expungeable = true;
}

#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void Edit_Structs::update_ui(Camera& view) {
	cross.pos = {
		box.w - cross_size * 1.5f,
		cross_size * 0.5f,
		cross_size,
		cross_size
	};

	float w = title.font->render.text_width(title.text.c_str()) / view.scale;
	float max_title_w = box.w - 5 * cross_size;
	w = w < max_title_w ? w : max_title_w;

	title.pos = {
		(box.w - w) / 2,
		2,
		w,
		title.font->render.text_height() * 1.1f / view.scale
	};

	show_cb.pos.x = border;
	show_cb.pos.y = box.h - show_cb.pos.h - border;

	float scroll_w = 14;
	float edit_y = 2*cross_size;
	float end_x = box.w - border;
	float cb_y = show_cb.pos.y;
	float win_h = cb_y - 2*border - scroll_w - edit_y;

	if (show_cb.checked) {
		div.pos = {
			div.position,
			edit_y,
			div.breadth,
			cb_y - border - edit_y
		};
		div.maximum = box.w - div.minimum;

		float pad = div.padding;
		float data_x = div.pos.x + div.pos.w + pad;

		output.pos = {
			data_x,
			edit_y,
			box.w - data_x - 2*border - scroll_w,
			win_h
		};

		out_vscroll.pos = {
			box.w - border - scroll_w,
			edit_y,
			scroll_w,
			output.pos.h
		};

		out_hscroll.pos = {
			data_x,
			show_cb.pos.y - border - scroll_w,
			output.pos.w,
			scroll_w
		};

		end_x = div.pos.x - pad;
		min_width = div.position + box.w - div.maximum;
	}
	else
		min_width = min_width;

	edit.pos = {
		border,
		edit_y,
		end_x - 2*border - scroll_w,
		win_h
	};

	edit_vscroll.pos = {
		end_x - scroll_w,
		edit_y,
		scroll_w,
		edit.pos.h
	};

	edit_hscroll.pos = {
		edit.pos.x,
		show_cb.pos.y - border - scroll_w,
		edit.pos.w,
		scroll_w
	};
}

void structs_edit_handler(Text_Editor *edit, Input& input) {
	auto ui = (Edit_Structs*)edit->parent;
	auto ws = (Workspace*)edit->parent->parent;

	if (ws->first_struct_run) {
		int reserve = edit->editor.text.size() + 2;
		ws->tokens.try_expand(reserve);
		ws->name_vector.try_expand(reserve);
		ws->first_struct_run = false;
	}
	else {
		ws->tokens.head = 0;
		ws->name_vector.head = 0;
		ws->struct_names.clear();

		for (auto& s : ws->structs) {
			s->flags |= FLAG_AVAILABLE;
			s->fields.zero_out();
		}
	}

	tokenize(ws->tokens, edit->editor.text.c_str(), edit->editor.text.size());

	//parse_definitions(ws->definitions, ws->tokens);

	char *tokens_alias = ws->tokens.pool;
	parse_c_struct(ws->structs, &tokens_alias, ws->name_vector, ws->definitions);

	for (auto& s : ws->structs) {
		char *name = ws->name_vector.at(s->name_idx);
		if (name)
			ws->struct_names.push_back(name);
	}

	for (auto& b : ws->boxes) {
		if (b->box_type == BoxObject)
			populate_object_table((View_Object*)b, ws->structs, ws->name_vector);
	}

	int n_rows = 0;
	for (auto& s : ws->structs)
		n_rows += s->fields.n_fields;

	ui->output.branches.clear();
	ui->output.branch_name_vector.head = 0;

	ui->output.data->resize(n_rows);
	ui->output.data->arena->rewind();

	int idx = 0;
	for (auto& s : ws->structs) {
		if (s->flags & FLAG_AVAILABLE)
			continue;

		int name_idx = -1;
		if (s->name_idx >= 0) {
			char *str = ws->name_vector.at(s->name_idx);
			name_idx = ui->output.branch_name_vector.add_string(str);

			ui->output.branches.push_back({
				idx, s->fields.n_fields, name_idx, false
			});
		}

		for (int i = 0; i < s->fields.n_fields; i++, idx++) {
			auto& f = s->fields.data[i];
			auto& cols = ui->output.data->columns;

			cols[0][idx] = format_field_name(*ui->output.data->arena, ws->name_vector, f);
			cols[1][idx] = format_type_name(*ui->output.data->arena, ws->name_vector, f);
		}
	}

	ui->output.update_tree();
	ui->output.needs_redraw = true;
}

void show_cb_handler(UI_Element *elem, bool dbl_click) {
	auto ui = (Edit_Structs*)elem->parent;
	auto cb = dynamic_cast<Checkbox*>(elem);

	cb->toggle();

	ui->div.visible = cb->checked;
	ui->output.visible = cb->checked;
	ui->out_hscroll.visible = cb->checked;
	ui->out_vscroll.visible = cb->checked;

	if (cb->checked)
		ui->min_width = ui->div.minimum * 2;
	else
		ui->min_width = ui->min_width;

	if (ui->box.w < ui->min_width)
		ui->box.w = ui->min_width;

	Camera just_scale = {0};
	just_scale.scale = ui->parent->temp_scale;
	ui->update_ui(just_scale);
}

void Edit_Structs::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;

	div.make_icon(new_scale);

	sdl_destroy_texture(&output.icon_plus);
	sdl_destroy_texture(&output.icon_minus);

	int len = output.font->render.text_height() + 0.5;
	RGBA color = {1, 1, 1, 0.8};
	output.icon_plus = make_plus_minus_icon(color, len, true);
	output.icon_minus = make_plus_minus_icon(color, len, false);
}

void Edit_Structs::wake_up() {
	Input blank = {0};
	edit.parent = this;
	structs_edit_handler(&edit, blank);
}

Edit_Structs::Edit_Structs(Workspace& ws) {
	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	title.font = ws.default_font;
	title.text = "Structs";
	title.padding = 0;
	ui.push_back(&title);

	edit.default_color = ws.scroll_back;
	edit.sel_color = ws.inactive_outline_color;
	edit.caret_color = ws.caret_color;
	edit.font = ws.make_font(10, ws.text_color);
	edit.editor.text = "struct Test {\n\tint a;\n\tint b;\n};";
	edit.key_action = structs_edit_handler;
	edit.hscroll = &edit_hscroll;
	edit.hscroll->content = &edit;
	edit.vscroll = &edit_vscroll;
	edit.vscroll->content = &edit;
	ui.push_back(&edit);

	edit_hscroll.back_color = ws.scroll_back;
	edit_hscroll.default_color = ws.scroll_color;
	edit_hscroll.hl_color = ws.scroll_hl_color;
	edit_hscroll.sel_color = ws.scroll_sel_color;
	edit_hscroll.vertical = false;
	ui.push_back(&edit_hscroll);

	edit_vscroll.back_color = ws.scroll_back;
	edit_vscroll.default_color = ws.scroll_color;
	edit_vscroll.hl_color = ws.scroll_hl_color;
	edit_vscroll.sel_color = ws.scroll_sel_color;
	ui.push_back(&edit_vscroll);

	show_cb.font = ws.make_font(10, ws.text_color);
	show_cb.text = "Show Output";
	show_cb.default_color = ws.scroll_back;
	show_cb.hl_color = ws.light_color;
	show_cb.sel_color = ws.cb_color;
	show_cb.pos.h = show_cb.font->render.text_height() * 1.1f / ws.temp_scale;
	show_cb.pos.w = 14 * show_cb.font->render.digit_width();
	show_cb.action = show_cb_handler;
	ui.push_back(&show_cb);

	div.visible = false;
	div.default_color = ws.div_color;
	div.breadth = 2;
	div.vertical = true;
	div.moveable = true;
	div.minimum = 8 * title.font->render.text_height();
	div.position = div.minimum + 50;
	div.cursor_type = CursorResizeWestEast;
	ui.push_back(&div);

	out_hscroll.visible = false;
	out_hscroll.back_color = ws.scroll_back;
	out_hscroll.default_color = ws.scroll_color;
	out_hscroll.hl_color = ws.scroll_hl_color;
	out_hscroll.sel_color = ws.scroll_sel_color;
	out_hscroll.vertical = false;
	ui.push_back(&out_hscroll);

	out_vscroll.visible = false;
	out_vscroll.back_color = ws.scroll_back;
	out_vscroll.default_color = ws.scroll_color;
	out_vscroll.hl_color = ws.scroll_hl_color;
	out_vscroll.sel_color = ws.scroll_sel_color;
	ui.push_back(&out_vscroll);

	output.visible = false;
	output.show_column_names = true;
	output.font = edit.font;
	output.default_color = ws.dark_color;
	output.hl_color = ws.hl_color;
	output.back_color = ws.back_color;
	output.hscroll = &out_hscroll;
	output.hscroll->content = &output;
	output.vscroll = &out_vscroll;
	output.vscroll->content = &output;

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 0, 0.5, 0, 0, "Type"}
	};

	arena.set_rewind_point();
	table.init(cols, &arena, 2, 0);
	output.data = &table;
	ui.push_back(&output);

	box_type = BoxStructs;
	//refresh_every = 1;
	back = ws.back_color;
	edge_color = ws.dark_color;
	box = {-200, -225, 400, 450};
	min_width = min_width;
	min_height = 200;
}
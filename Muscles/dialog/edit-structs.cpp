#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void reposition_struct_box(Box& b, float scale) {
	auto ui = (Edit_Structs*)b.markup;
	ui->cross.pos = {
		b.box.w - b.cross_size * 1.5f,
		b.cross_size * 0.5f,
		b.cross_size,
		b.cross_size
	};

	float w = ui->title.font->render.text_width(ui->title.text.c_str()) / scale;
	float max_title_w = b.box.w - 5 * b.cross_size;
	w = w < max_title_w ? w : max_title_w;

	ui->title.pos = {
		(b.box.w - w) / 2,
		2,
		w,
		ui->title.font->render.text_height() * 1.1f / scale
	};
	ui->title.update_position(scale);

	ui->show_cb.pos.x = b.border;
	ui->show_cb.pos.y = b.box.h - ui->show_cb.pos.h - b.border;

	float scroll_w = 14;
	float edit_y = 2*b.cross_size;
	float end_x = b.box.w - b.border;
	float cb_y = ui->show_cb.pos.y;
	float win_h = cb_y - 2*b.border - scroll_w - edit_y;

	if (ui->show_cb.checked) {
		ui->div.pos = {
			ui->div.position,
			edit_y,
			ui->div.breadth,
			cb_y - b.border - edit_y
		};
		ui->div.maximum = b.box.w - ui->div.minimum;

		float pad = ui->div.padding;
		float data_x = ui->div.pos.x + ui->div.pos.w + pad;

		ui->output.pos = {
			data_x,
			edit_y,
			b.box.w - data_x - 2*b.border - scroll_w,
			win_h
		};

		ui->out_vscroll.pos = {
			b.box.w - b.border - scroll_w,
			edit_y,
			scroll_w,
			ui->output.pos.h
		};

		ui->out_hscroll.pos = {
			data_x,
			ui->show_cb.pos.y - b.border - scroll_w,
			ui->output.pos.w,
			scroll_w
		};

		end_x = ui->div.pos.x - pad;
		b.min_width = ui->div.position + b.box.w - ui->div.maximum;
	}
	else
		b.min_width = ui->min_width;

	ui->edit.pos = {
		b.border,
		edit_y,
		end_x - 2*b.border - scroll_w,
		win_h
	};

	ui->edit_vscroll.pos = {
		end_x - scroll_w,
		edit_y,
		scroll_w,
		ui->edit.pos.h
	};

	ui->edit_hscroll.pos = {
		ui->edit.pos.x,
		ui->show_cb.pos.y - b.border - scroll_w,
		ui->edit.pos.w,
		scroll_w
	};
}

void update_struct_box(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	reposition_struct_box(b, view.scale);

	b.post_update_elements(view, input, inside, hover, focussed);
}

void structs_edit_handler(Text_Editor *edit, Input& input) {
	auto ui = (Edit_Structs*)edit->parent->markup;
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

	char *tokens_alias = ws->tokens.pool;
	parse_c_struct(ws->structs, &tokens_alias, ws->name_vector);

	for (auto& s : ws->structs) {
		char *name = ws->name_vector.at(s->name_idx);
		if (name)
			ws->struct_names.push_back(name);
	}

	for (auto& b : ws->boxes) {
		if (b->type == BoxObject)
			populate_object_table((View_Object*)b->markup, ws->structs, ws->name_vector);
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
	Box& b = *elem->parent;
	auto ui = (Edit_Structs*)b.markup;
	auto cb = dynamic_cast<Checkbox*>(elem);

	cb->toggle();

	ui->div.visible = cb->checked;
	ui->output.visible = cb->checked;
	ui->out_hscroll.visible = cb->checked;
	ui->out_vscroll.visible = cb->checked;

	if (cb->checked)
		b.min_width = ui->div.minimum * 2;
	else
		b.min_width = ui->min_width;

	if (b.box.w < b.min_width)
		b.box.w = b.min_width;

	reposition_struct_box(b, b.parent->temp_scale);
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Edit_Structs*)b.markup;
	ui->cross.img = ws.cross;

	ui->div.make_icon(new_scale);

	sdl_destroy_texture(&ui->output.icon_plus);
	sdl_destroy_texture(&ui->output.icon_minus);

	int len = ui->output.font->render.text_height() + 0.5;
	RGBA color = {1, 1, 1, 0.8};
	ui->output.icon_plus = make_plus_minus_icon(color, len, true);
	ui->output.icon_minus = make_plus_minus_icon(color, len, false);
}

static void delete_markup(Box *b) {
	delete (Edit_Structs*)b->markup;
	b->markup = nullptr;
}

void make_struct_box(Workspace& ws, Box& b) {
	auto ui = new Edit_Structs();

	ui->cross.action = get_delete_box();
	ui->cross.img = ws.cross;
	b.ui.push_back(&ui->cross);

	ui->title.font = ws.default_font;
	ui->title.text = "Structs";
	ui->title.padding = 0;
	b.ui.push_back(&ui->title);

	ui->edit.default_color = ws.scroll_back;
	ui->edit.sel_color = ws.inactive_outline_color;
	ui->edit.caret_color = ws.caret_color;
	ui->edit.font = ws.make_font(10, ws.text_color);
	ui->edit.editor.text = "struct Test {\n\tint a;\n\tint b;\n};";
	ui->edit.key_action = structs_edit_handler;
	ui->edit.hscroll = &ui->edit_hscroll;
	ui->edit.hscroll->content = &ui->edit;
	ui->edit.vscroll = &ui->edit_vscroll;
	ui->edit.vscroll->content = &ui->edit;
	b.ui.push_back(&ui->edit);

	ui->edit_hscroll.back = ws.scroll_back;
	ui->edit_hscroll.default_color = ws.scroll_color;
	ui->edit_hscroll.hl_color = ws.scroll_hl_color;
	ui->edit_hscroll.sel_color = ws.scroll_sel_color;
	ui->edit_hscroll.vertical = false;
	b.ui.push_back(&ui->edit_hscroll);

	ui->edit_vscroll.back = ws.scroll_back;
	ui->edit_vscroll.default_color = ws.scroll_color;
	ui->edit_vscroll.hl_color = ws.scroll_hl_color;
	ui->edit_vscroll.sel_color = ws.scroll_sel_color;
	b.ui.push_back(&ui->edit_vscroll);

	ui->show_cb.font = ws.make_font(10, ws.text_color);
	ui->show_cb.text = "Show Output";
	ui->show_cb.default_color = ws.scroll_back;
	ui->show_cb.hl_color = ws.light_color;
	ui->show_cb.sel_color = ws.cb_color;
	ui->show_cb.pos.h = ui->show_cb.font->render.text_height() * 1.1f / ws.temp_scale;
	ui->show_cb.pos.w = 14 * ui->show_cb.font->render.digit_width();
	ui->show_cb.action = show_cb_handler;
	b.ui.push_back(&ui->show_cb);

	ui->div.visible = false;
	ui->div.default_color = ws.div_color;
	ui->div.breadth = 2;
	ui->div.vertical = true;
	ui->div.moveable = true;
	ui->div.minimum = 8 * ui->title.font->render.text_height();
	ui->div.position = ui->div.minimum + 50;
	ui->div.cursor_type = CursorResizeWestEast;
	b.ui.push_back(&ui->div);

	ui->out_hscroll.visible = false;
	ui->out_hscroll.back = ws.scroll_back;
	ui->out_hscroll.default_color = ws.scroll_color;
	ui->out_hscroll.hl_color = ws.scroll_hl_color;
	ui->out_hscroll.sel_color = ws.scroll_sel_color;
	ui->out_hscroll.vertical = false;
	b.ui.push_back(&ui->out_hscroll);

	ui->out_vscroll.visible = false;
	ui->out_vscroll.back = ws.scroll_back;
	ui->out_vscroll.default_color = ws.scroll_color;
	ui->out_vscroll.hl_color = ws.scroll_hl_color;
	ui->out_vscroll.sel_color = ws.scroll_sel_color;
	b.ui.push_back(&ui->out_vscroll);

	ui->output.visible = false;
	ui->output.show_column_names = true;
	ui->output.font = ui->edit.font;
	ui->output.default_color = ws.dark_color;
	ui->output.hl_color = ws.hl_color;
	ui->output.back_color = ws.back_color;
	ui->output.consume_box_scroll = true;
	ui->output.hscroll = &ui->out_hscroll;
	ui->output.hscroll->content = &ui->output;
	ui->output.vscroll = &ui->out_vscroll;
	ui->output.vscroll->content = &ui->output;

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 0, 0.5, 0, 0, "Type"}
	};

	ui->arena.set_rewind_point();
	ui->table.init(cols, &ui->arena, 2, 0);
	ui->output.data = &ui->table;
	b.ui.push_back(&ui->output);

	b.type = BoxStructs;
	b.markup = ui;
	b.delete_markup_handler = delete_markup;
	b.update_handler = update_struct_box;
	b.scale_change_handler = scale_change_handler;
	//b.refresh_handler = refresh_handler;
	b.refresh_every = 1;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.min_width = ui->min_width;
	b.min_height = 200;
	b.visible = true;
}

void open_edit_structs(Workspace& ws) {
	Box *box = ws.first_box_of_type(BoxStructs);
	if (!box) {
		box = new Box();
		make_struct_box(ws, *box);
		ws.add_box(box);
	}
	else {
		box->visible = true;
		ws.bring_to_front(box);
	}

	Input blank = {0};
	Text_Editor *edit = &((Edit_Structs*)box->markup)->edit;
	edit->parent = box;
	edit->parent->parent = &ws;
	structs_edit_handler(edit, blank);
}
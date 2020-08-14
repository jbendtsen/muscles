#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void update_struct_box(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	auto ui = (Edit_Structs*)b.markup;
	ui->cross->pos = {
		b.box.w - b.cross_size * 1.5f,
		b.cross_size * 0.5f,
		b.cross_size,
		b.cross_size
	};

	float w = ui->title->font->render.text_width(ui->title->text.c_str()) / view.scale;
	float max_title_w = b.box.w - 5 * b.cross_size;
	w = w < max_title_w ? w : max_title_w;

	ui->title->pos = {
		(b.box.w - w) / 2,
		2,
		w,
		ui->title->font->render.text_height() * 1.1f / view.scale
	};
	ui->title->update_position(view);

	float scroll_w = 14;

	float edit_y = 2*b.cross_size;
	ui->edit->pos = {
		b.border,
		edit_y,
		b.box.w - 3*b.border - scroll_w,
		b.box.h - 2*b.border - scroll_w - edit_y
	};

	ui->vscroll->pos = {
		b.box.w - b.border - scroll_w,
		edit_y,
		scroll_w,
		ui->edit->pos.h
	};

	ui->hscroll->pos = {
		ui->edit->pos.x,
		b.box.h - b.border - scroll_w,
		ui->edit->pos.w,
		scroll_w
	};

	b.post_update_elements(view, input, inside, hover, focussed);
}

void structs_edit_handler(Text_Editor *edit, Input& input) {
	auto ui = (Edit_Structs*)edit->parent->markup;

	if (ui->first_run)
		ui->tokens.reserve(edit->text.size());
	else {
		ui->tokens.clear();
		ui->struct_names.clear();

		for (auto& s : ui->structs)
			s->flags |= FLAG_AVAILABLE;
	}

	tokenize(ui->tokens, edit->text.c_str(), edit->text.size());

	if (ui->name_pool)
		delete[] ui->name_pool;

	ui->name_pool = new char[ui->tokens.size()];

	char *tokens = ui->tokens.data();
	char *pool_alias = ui->name_pool;
	parse_c_struct(ui->structs, &tokens, &pool_alias);

	for (auto& s : ui->structs)
		ui->struct_names.push_back(s->name);

	Workspace& ws = *edit->parent->parent;
	for (auto& b : ws.boxes) {
		if (b->type == BoxObject)
			populate_object_table((View_Object*)b->markup, ui->structs);
	}

	ui->first_run = false;
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Edit_Structs*)b.markup;
	ui->cross->img = ws.cross;
}

void make_struct_box(Workspace& ws, Box& b) {
	auto ui = new Edit_Structs();

	ui->cross = new Image();
	ui->cross->action = get_delete_box();
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title = new Label();
	ui->title->font = ws.default_font;
	ui->title->text = "Structs";
	ui->title->padding = 0;
	b.ui.push_back(ui->title);

	ui->edit = new Text_Editor();
	ui->edit->default_color = ws.scroll_back;
	ui->edit->sel_color = ws.inactive_outline_color;
	ui->edit->caret_color = ws.caret_color;
	ui->edit->font = ws.make_font(10, ws.text_color);
	ui->edit->text = "struct Test {\n\tint a;\n\tint b;\n};";
	ui->edit->action = get_set_active_edit();
	ui->edit->key_action = structs_edit_handler;
	b.ui.push_back(ui->edit);

	ui->hscroll = new Scroll();
	ui->hscroll->back = ws.scroll_back;
	ui->hscroll->default_color = ws.scroll_color;
	ui->hscroll->hl_color = ws.scroll_hl_color;
	ui->hscroll->sel_color = ws.scroll_sel_color;
	ui->hscroll->vertical = false;
	b.ui.push_back(ui->hscroll);

	ui->vscroll = new Scroll();
	ui->vscroll->back = ws.scroll_back;
	ui->vscroll->default_color = ws.scroll_color;
	ui->vscroll->hl_color = ws.scroll_hl_color;
	ui->vscroll->sel_color = ws.scroll_sel_color;
	b.ui.push_back(ui->vscroll);

	b.type = BoxStructs;
	b.markup = ui;
	b.update_handler = update_struct_box;
	b.scale_change_handler = scale_change_handler;
	//b.refresh_handler = refresh_handler;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.min_width = 200;
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

	ws.focus = ws.boxes.back();

	Input blank = {0};
	Text_Editor *edit = ((Edit_Structs*)box->markup)->edit;
	edit->parent = box;
	edit->parent->parent = &ws;
	structs_edit_handler(edit, blank);
}
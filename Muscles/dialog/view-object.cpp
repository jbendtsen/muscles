#include "muscles.h"
#include "ui.h"
#include "dialog.h"

void update_view_object(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	auto ui = (View_Object*)b.markup;

	ui->cross->pos.y = b.cross_size * 0.5;
	ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
	ui->cross->pos.w = b.cross_size;
	ui->cross->pos.h = b.cross_size;

	float title_w = 200;
	float title_x = (ui->cross->pos.x - title_w) / 2;
	if (title_x < b.border) {
		title_x = b.border;
		title_w = ui->cross->pos.x - title_x - 2*b.border;
	}

	ui->title_edit->pos = {
		title_x,
		2,
		title_w,
		ui->title_edit->font->render.text_height() * 1.4f / view.scale
	};

	float meta_y = ui->title_edit->pos.h + 4*b.border;
	float y = meta_y;
	float label_x = 2*b.border;
	float label_h = ui->struct_label->font->render.text_height() * 1.2f / view.scale;

	float edit_h = ui->struct_edit->font->render.text_height() * 1.4f / view.scale;
	float edit_gap = edit_h + b.border;
	float edit_y = meta_y - (edit_gap - label_h) / 2;

	ui->struct_label->pos = {
		label_x,
		y,
		80,
		label_h
	};
	y += edit_gap;

	ui->source_label->pos = {
		label_x,
		y,
		80,
		label_h
	};
	y += edit_gap;

	ui->addr_label->pos = {
		label_x,
		y,
		100,
		label_h
	};

	ui->struct_label->update_position(view);
	ui->source_label->update_position(view);
	ui->addr_label->update_position(view);

	y = edit_y;
	ui->struct_edit->pos = {
		label_x + ui->struct_label->pos.w + b.border,
		y,
		150,
		edit_h
	};
	y += edit_gap;

	b.post_update_elements(view, input, inside, hover, focussed);
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (View_Object*)b.markup;
	ui->cross->img = ws.cross;
}

void make_view_object(Workspace& ws, Box& b) {
	/*
		Edit_Box *struct_edit = nullptr;
		Drop_Down *struct_dd = nullptr;
		Edit_Box *source_edit = nullptr;
		Drop_Down *source_dd = nullptr;
		Edit_Box *addr_edit = nullptr;
		Button *hide_meta = nullptr;
		Data_View *view = nullptr;
		Scroll *hscroll = nullptr;
		Scroll *vscroll = nullptr;
		Button *all_btn = nullptr;
		Button *sel_btn = nullptr;
	*/

	auto ui = new View_Object();

	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title_edit = new Edit_Box();
	ui->title_edit->placeholder = "<instance>";
	ui->title_edit->ph_font = ws.make_font(12, ws.ph_text_color);
	ui->title_edit->font = ws.default_font;
	ui->title_edit->caret = ws.caret_color;
	ui->title_edit->caret.a = 0.6;
	ui->title_edit->default_color = ws.dark_color;
	ui->title_edit->action = [](UI_Element *elem, bool dbl_click) {elem->parent->active_edit = elem;};
	b.ui.push_back(ui->title_edit);

	ui->struct_label = new Label();
	ui->struct_label->font = ws.make_font(11, ws.text_color);
	ui->struct_label->text = "Struct:";
	ui->struct_label->padding = 0;
	b.ui.push_back(ui->struct_label);

	ui->struct_edit = new Edit_Box();
	ui->struct_edit->font = ui->struct_label->font;
	ui->struct_edit->caret = ws.caret_color;
	ui->struct_edit->default_color = ws.dark_color;
	b.ui.push_back(ui->struct_edit);

	ui->source_label = new Label();
	ui->source_label->font = ui->struct_label->font;
	ui->source_label->text = "Source:";
	ui->source_label->padding = 0;
	b.ui.push_back(ui->source_label);

	ui->addr_label = new Label();
	ui->addr_label->font = ui->struct_label->font;
	ui->addr_label->text = "Address:";
	ui->addr_label->padding = 0;
	b.ui.push_back(ui->addr_label);

	b.markup = ui;
	b.update_handler = update_view_object;
	b.scale_change_handler = scale_change_handler;
	//b.refresh_handler = refresh_handler;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.min_width = 200;
	b.min_height = 200;
	b.visible = true;
}
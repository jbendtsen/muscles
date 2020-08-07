#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void update_struct_box(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	b.update_elements(view, input, inside, hovered, focussed);

	auto ui = (Struct_Box*)b.markup;
	ui->cross->pos = {
		b.box.w - b.cross_size * 1.5f,
		b.cross_size * 0.5f,
		b.cross_size,
		b.cross_size
	};

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

	b.post_update_elements(view, input, inside, hovered, focussed);
}

static void refresh_handler(Box& b, Point& cursor) {

}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Struct_Box*)b.markup;
	ui->cross->img = ws.cross;
}

void make_struct_box(Workspace& ws, Box& b) {
	auto ui = new Struct_Box();

	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->edit = new Text_Editor();
	ui->edit->default_color = ws.scroll_back;
	ui->edit->sel_color = ws.inactive_outline_color;
	ui->edit->caret_color = ws.caret_color;
	ui->edit->font = ws.make_font(10, ws.text_color);
	ui->edit->text = "struct Test {\n\tint a;\n\tint b;\n};";
	ui->edit->action = [](UI_Element *elem, bool dbl_click) {elem->parent->active_edit = elem;};
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

	b.markup = ui;
	b.update_handler = update_struct_box;
	b.scale_change_handler = scale_change_handler;
	b.refresh_handler = refresh_handler;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.min_width = 200;
	b.min_height = 200;
	b.visible = true;
}
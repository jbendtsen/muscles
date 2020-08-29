#include "muscles.h"
#include "ui.h"
#include "dialog.h"

void update_defs_box(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	auto ui = (View_Definitions*)b.markup;
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
	ui->title->update_position(view.scale);

	float y = ui->title->pos.y + ui->title->pos.h;

	ui->view->pos = {
		b.border,
		y,
		b.box.w - 2*b.border,
		b.box.h - b.border - y
	};

	b.post_update_elements(view, input, inside, hover, focussed);
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Edit_Structs*)b.markup;
	ui->cross->img = ws.cross;
}

void make_view_definitions(Workspace& ws, Box& b) {
	auto ui = new View_Definitions();

	ui->cross = new Image();
	ui->cross->action = get_delete_box();
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title = new Label();
	ui->title->font = ws.default_font;
	ui->title->text = "Definitions";
	ui->title->padding = 0;
	b.ui.push_back(ui->title);

	ui->vscroll = new Scroll();
	ui->vscroll->back = ws.scroll_back;
	ui->vscroll->default_color = ws.scroll_color;
	ui->vscroll->hl_color = ws.scroll_hl_color;
	ui->vscroll->sel_color = ws.scroll_sel_color;

	ui->hscroll = new Scroll();
	ui->hscroll->back = ws.scroll_back;
	ui->hscroll->default_color = ws.scroll_color;
	ui->hscroll->hl_color = ws.scroll_hl_color;
	ui->hscroll->sel_color = ws.scroll_sel_color;
	ui->hscroll->vertical = false;

	ui->view = new Data_View();
	ui->view->show_column_names = true;
	ui->view->font = ws.make_font(11, ws.text_color);
	ui->view->default_color = ws.dark_color;
	ui->view->hl_color = ws.hl_color;
	ui->view->back_color = ws.back_color;
	ui->view->consume_box_scroll = true;
	ui->view->hscroll = ui->hscroll;
	ui->view->hscroll->content = ui->view;
	ui->view->vscroll = ui->vscroll;
	ui->view->vscroll->content = ui->view;

	ui->arena.set_rewind_point();

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 0, 0.5, 0, 0, "Value"}
	};

	ui->constants.init(cols, &ui->arena, 0, 2);
	ui->view->data = &ui->constants;

	b.ui.push_back(ui->view);
	b.ui.push_back(ui->vscroll);
	b.ui.push_back(ui->hscroll);

	b.type = BoxDefinitions;
	b.markup = ui;
	b.update_handler = update_defs_box;
	b.scale_change_handler = scale_change_handler;
	//b.refresh_handler = refresh_handler;
	//b.refresh_every = 1;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.visible = true;
}

void open_view_definitions(Workspace& ws) {
	Box *b = ws.first_box_of_type(BoxDefinitions);
	if (b) {
		b->visible = true;
		ws.bring_to_front(b);
	}
	else {
		b = new Box();
		make_view_definitions(ws, *b);
		ws.add_box(b);
	}

	ws.focus = ws.boxes.back();
}
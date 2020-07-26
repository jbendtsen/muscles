#include "../muscles.h"
#include "../ui.h"
#include "../io.h"
#include "dialog.h"

void update_view_source(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	b.update_elements(view, input, inside, hovered, focussed);
	if (hovered && input.lclick) {
		b.moving = true;
		b.select_edge(view, inside);
	}

	auto ui = (View_Source *)b.markup;

	if (ui->cross) {
		ui->cross->pos.y = b.cross_size * 0.5;
		ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
		ui->cross->pos.w = b.cross_size;
		ui->cross->pos.h = b.cross_size;
	}

	b.post_update_elements(view, input, inside, hovered, focussed);
}

void make_view_source_menu(Workspace& ws, Source& s, Box& b) {
	auto ui = new View_Source();
	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;

	b.ui.push_back(ui->cross);

	b.markup = ui;
	b.update_handler = update_view_source;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-80, -50, 160, 100};
	b.visible = true;
}
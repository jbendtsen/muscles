#include "../muscles.h"
#include "../ui.h"
#include "../io.h"
#include "dialog.h"

void update_view_source(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	b.update_elements(view, input, inside, hovered, focussed);

	auto ui = (View_Source*)b.markup;

	if (ui->cross) {
		ui->cross->pos.y = b.cross_size * 0.5;
		ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
		ui->cross->pos.w = b.cross_size;
		ui->cross->pos.h = b.cross_size;
	}

	float hack = 6;

	auto& render = ui->title->font->render;
	float w = render.text_width(ui->title->text.c_str()) / view.scale;
	ui->title->pos = {
		(b.box.w - w) / 2,
		hack,
		w * 1.1f,
		render.text_height() * 1.2f / view.scale
	};
	ui->title->update_position(view, ui->title->pos);

	float y = ui->title->pos.y + ui->title->pos.h + hack;

	ui->div->pos = {
		b.box.w * 0.4f,
		y,
		ui->div->pos.w,
		b.box.h - y - b.border
	};
	float pad = ui->div->padding;

	y += hack;

	ui->reg_title->pos = {
		b.border,
		y,
		ui->div->pos.x - b.border - pad,
		ui->reg_title->font->render.text_height() * 1.1f / view.scale
	};
	ui->reg_title->update_position(view, ui->reg_title->pos);

	ui->hex_title->pos.x = ui->div->pos.x + ui->div->pos.w + b.border;
	ui->hex_title->pos.y = y;
	ui->hex_title->pos.w = b.box.w - ui->hex_title->pos.x - b.border;
	ui->hex_title->pos.h = ui->reg_title->pos.h;

	y += ui->reg_title->pos.h;

	float scroll_w = 12;
	ui->reg_scroll->pos = {
		ui->reg_title->pos.x + ui->reg_title->pos.w - scroll_w,
		y,
		scroll_w,
		b.box.h - y - scroll_w - 3*b.border
	};

	ui->reg_lat_scroll->pos = {
		b.border,
		b.box.h - scroll_w - 2*b.border,
		ui->reg_scroll->pos.x - 2*b.border,
		scroll_w
	};

	ui->regions->pos = {
		ui->reg_lat_scroll->pos.x,
		ui->reg_scroll->pos.y,
		ui->reg_lat_scroll->pos.w,
		ui->reg_scroll->pos.h
	};

	float n_heights = 40;
	ui->reg_lat_scroll->maximum = ui->regions->font->render.text_height() * n_heights / view.scale;

	//Label *hex_title = nullptr;
	//Hex_View *hex = nullptr;
	//Scroll *hex_scroll = nullptr;

	b.post_update_elements(view, input, inside, hovered, focussed);
}

static void refresh_handler(Box& b) {
	auto ui = (View_Source*)b.markup;

	int n_sources = ui->source->regions.size();
	ui->regions->data.resize(n_sources);

	int idx = 0;
	for (auto& r : ui->source->regions) {
		snprintf((char*)ui->regions->data.columns[0][idx], ui->regions->data.headers[0].count_per_cell, "%016llx", r.base);
		snprintf((char*)ui->regions->data.columns[1][idx], ui->regions->data.headers[1].count_per_cell, "%08llx", r.size);
		ui->regions->data.columns[2][idx] = r.name;
		idx++;
	}
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	((View_Source*)b.markup)->cross->img = ws.cross;
}

void make_view_source_menu(Workspace& ws, Source *s, Box& b) {
	auto ui = new View_Source();
	ui->source = s;

	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title = new Label();
	ui->title->text = "View Source - ";
	ui->title->text += s->name;
	ui->title->font = ws.default_font;
	b.ui.push_back(ui->title);

	ui->div = new Divider();
	ui->div->default_color = {0.5, 0.6, 0.8, 1.0};
	ui->div->pos.w = 1.5;
	ui->div->vertical = true;
	ui->div->moveable = true;
	b.ui.push_back(ui->div);

	ui->reg_title = new Label();
	ui->reg_title->text = "Regions";
	ui->reg_title->font = ws.make_font(10, ws.text_color);
	b.ui.push_back(ui->reg_title);

	ui->hex_title = new Label();
	ui->hex_title->text = "Data";
	ui->hex_title->font = ui->reg_title->font;
	b.ui.push_back(ui->hex_title);

	ui->reg_scroll = new Scroll();
	ui->reg_scroll->back = {0, 0.1, 0.4, 1.0};
	ui->reg_scroll->default_color = {0.4, 0.45, 0.55, 1.0};
	ui->reg_scroll->hl_color = {0.6, 0.63, 0.7, 1.0};
	ui->reg_scroll->sel_color = {0.8, 0.8, 0.8, 1.0};
	ui->reg_scroll->breadth = 12;
	b.ui.push_back(ui->reg_scroll);

	ui->reg_lat_scroll = new Scroll();
	ui->reg_lat_scroll->back = ui->reg_scroll->back;
	ui->reg_lat_scroll->default_color = ui->reg_scroll->default_color;
	ui->reg_lat_scroll->hl_color = ui->reg_scroll->hl_color;
	ui->reg_lat_scroll->sel_color = ui->reg_scroll->sel_color;
	ui->reg_lat_scroll->vertical = false;
	ui->reg_lat_scroll->breadth = 12;
	b.ui.push_back(ui->reg_lat_scroll);

	ui->regions = new Data_View();
	ui->regions->font = ws.make_font(9, ws.text_color);
	ui->regions->default_color = ws.dark_color;
	ui->regions->hl_color = ws.hl_color;
	ui->regions->vscroll = ui->reg_scroll;
	ui->regions->hscroll = ui->reg_lat_scroll;

	Column cols[] = {
		{String, 20, 0.2, 10, "Address"},
		{String, 20, 0.1, 5, "Size"},
		{String, 0, 0.7, 0, "Name"}
	};

	ui->regions->data.init(cols, 3, 0);
	b.ui.push_back(ui->regions);

	ui->hex = new Hex_View();
	ui->hex->font = ws.make_font(8, ws.text_color);
	ui->hex->default_color = ws.dark_color;
	b.ui.push_back(ui->hex);
	b.ui.push_back(&ui->hex->hscroll);

	ui->hex_scroll = new Scroll();
	ui->hex_scroll->back = ui->reg_scroll->back;
	ui->hex_scroll->default_color = ui->reg_scroll->default_color;
	ui->hex_scroll->hl_color = ui->reg_scroll->hl_color;
	ui->hex_scroll->sel_color = ui->reg_scroll->sel_color;
	b.ui.push_back(ui->hex_scroll);

	b.markup = ui;
	b.update_handler = update_view_source;
	b.refresh_handler = refresh_handler;
	b.scale_change_handler = scale_change_handler;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-250, -200, 500, 400};
	b.visible = true;
}
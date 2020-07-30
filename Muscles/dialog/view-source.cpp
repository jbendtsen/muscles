#include <numeric>
#include "../muscles.h"
#include "../ui.h"
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
	ui->title->update_position(view);

	float scroll_w = 12;
	float data_x = b.border;
	float data_y = ui->title->pos.y + ui->title->pos.h + hack;

	if (ui->multiple_regions) {
		float y = data_y;

		ui->div->pos = {
			ui->div->position,
			y,
			ui->div->breadth,
			b.box.h - y - b.border
		};
		ui->div->maximum = b.box.w - ui->div->minimum;

		float pad = ui->div->padding;
		data_x = ui->div->pos.x + ui->div->pos.w + pad;

		ui->reg_title->pos = {
			b.border,
			y,
			ui->div->pos.x - b.border - pad,
			ui->reg_title->font->render.text_height() * 1.1f / view.scale
		};
		ui->reg_title->update_position(view);

		y += ui->reg_title->pos.h;

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

		ui->reg_table->pos = {
			ui->reg_lat_scroll->pos.x,
			ui->reg_scroll->pos.y,
			ui->reg_lat_scroll->pos.w,
			ui->reg_scroll->pos.h
		};

		float n_heights = 40;
		ui->reg_lat_scroll->maximum = ui->reg_table->font->render.text_height() * n_heights / view.scale;
	}

	ui->hex_title->pos.x = data_x;
	ui->hex_title->pos.y = data_y;
	ui->hex_title->pos.w = b.box.w - ui->hex_title->pos.x - b.border;
	ui->hex_title->pos.h = ui->hex_title->font->render.text_height() * 1.1f / view.scale;
	ui->hex_title->update_position(view);

	float data_w = ui->hex_scroll->pos.x - ui->hex_title->pos.x - b.border;

	ui->hex_info->pos = {
		data_x,
		b.box.h - b.border - ui->hex_title->pos.h,
		data_w,
		ui->hex_title->pos.h
	};
	ui->hex_info->update_position(view);

	data_y += ui->hex_title->pos.h;
	float data_h = ui->hex_info->pos.y - data_y - b.border;

	ui->hex_scroll->pos = {
		b.box.w - b.border - scroll_w,
		data_y,
		scroll_w,
		data_h
	};

	ui->hex->pos = {
		ui->hex_title->pos.x,
		data_y,
		data_w,
		data_h
	};

	char info[32];
	sprintf(info, "0x%08x/0x%08x", ui->hex->offset, ui->hex->region_size);
	ui->hex_info->text = info;

	b.post_update_elements(view, input, inside, hovered, focussed);
}

void regions_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);
	table->sel_row = table->hl_row;

	if (table->sel_row < 0)
		return;

	auto ui = (View_Source*)table->parent->markup;

	u64 address = strtoull((char*)table->data.columns[0][table->sel_row], nullptr, 16);
	int size = strtol((char*)table->data.columns[1][table->sel_row], nullptr, 16);
	ui->hex->set_region(address, size);

	ui->hex_scroll->position = 0;
}

void print_hex(char *hex, char *out, u64 n, int n_digits) {
	int shift = (n_digits-1) * 4;
	char *p = out;
	for (int i = 0; i < n_digits; i++) {
		*p++ = hex[(n >> shift) & 0xf];
		shift -= 4;
	}
	*p = 0;
}

static void refresh_handler(Box& b, Point& cursor) {
	auto ui = (View_Source*)b.markup;
	if (ui->multiple_regions) {
		int n_rows = ui->reg_table->data.row_count();
		bool hovered = ui->reg_table->pos.contains(cursor);

		if (!n_rows || (ui->source->region_refreshed && !hovered)) {
			int n_sources = ui->source->regions.size();
			if (n_sources != ui->region_list.size()) {
				u64 sel_addr = 0;
				int sel = ui->reg_table->sel_row;
				if (sel >= 0)
					sel_addr = ui->source->regions[ui->region_list[sel]].base;

				ui->reg_table->data.resize(n_sources);
				ui->region_list.resize(n_sources);
				std::iota(ui->region_list.begin(), ui->region_list.end(), 0);

				if (sel >= 0) {
					sel = -1;
					for (int i = 0; i < n_sources; i++) {
						if (ui->source->regions[i].base == sel_addr) {
							sel = i;
							break;
						}
					}
					ui->reg_table->sel_row = sel;
				}
			}
		}

		ui->source->block_region_refresh = hovered;

		// place a hex digit LUT on the stack
		char hex[16];
		memcpy(hex, "0123456789abcdef", 16);

		auto& addrs = (std::vector<char*>&)ui->reg_table->data.columns[0];
		auto& sizes = (std::vector<char*>&)ui->reg_table->data.columns[1];
		auto& names = (std::vector<char*>&)ui->reg_table->data.columns[2];

		int idx = 0;
		for (auto& r : ui->region_list) {
			/*
			if (it == ui->source->regions.end()) {
				strcpy(addrs[idx], "???");
				strcpy(sizes[idx], "???");
				strcpy(names[idx], "???");
			}
			*/
			auto& reg = ui->source->regions[r];
			print_hex(hex, addrs[idx], reg.base, 16);
			print_hex(hex, sizes[idx], reg.size, 8);
			names[idx] = reg.name;
			idx++;
		}
	}
	else {
		ui->hex->set_region(0, ui->hex->source->regions[0].size);
	}
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (View_Source*)b.markup;
	ui->cross->img = ws.cross;

	if (ui->div)
		ui->div->make_icon(new_scale);
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

	ui->multiple_regions = s->regions.size() > 1;
	if (ui->multiple_regions) {
		ui->div = new Divider();
		ui->div->default_color = {0.5, 0.6, 0.8, 1.0};
		ui->div->breadth = 2;
		ui->div->vertical = true;
		ui->div->moveable = true;
		ui->div->minimum = 8 * ui->title->font->render.text_height();
		ui->div->position = ui->div->minimum;
		ui->div->make_icon(ws.temp_scale);
		b.ui.push_back(ui->div);

		ui->reg_title = new Label();
		ui->reg_title->text = "Regions";
		ui->reg_title->font = ws.make_font(10, ws.text_color);
		b.ui.push_back(ui->reg_title);

		ui->reg_scroll = new Scroll();
		ui->reg_scroll->back = ws.scroll_back;
		ui->reg_scroll->default_color = ws.scroll_color;
		ui->reg_scroll->hl_color = ws.scroll_hl_color;
		ui->reg_scroll->sel_color = ws.scroll_sel_color;
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

		ui->reg_table = new Data_View();
		ui->reg_table->action = regions_handler;
		ui->reg_table->font = ws.make_font(9, ws.text_color);
		ui->reg_table->default_color = ws.dark_color;
		ui->reg_table->hl_color = ws.hl_color;
		ui->reg_table->sel_color = ws.light_color;
		ui->reg_table->vscroll = ui->reg_scroll;
		ui->reg_table->hscroll = ui->reg_lat_scroll;

		Column cols[] = {
			{String, 20, 0.2, 10, "Address"},
			{String, 20, 0.1, 5, "Size"},
			{String, 0, 0.7, 0, "Name"}
		};

		ui->reg_table->data.init(cols, 3, 0);
		b.ui.push_back(ui->reg_table);
	}

	ui->hex_title = new Label();
	ui->hex_title->text = "Data";
	ui->hex_title->font = ws.make_font(10, ws.text_color);
	b.ui.push_back(ui->hex_title);

	ui->hex_scroll = new Scroll();
	ui->hex_scroll->back = ws.scroll_back;
	ui->hex_scroll->default_color = ws.scroll_color;
	ui->hex_scroll->hl_color = ws.scroll_hl_color;
	ui->hex_scroll->sel_color = ws.scroll_sel_color;
	b.ui.push_back(ui->hex_scroll);

	ui->hex = new Hex_View();
	ui->hex->font = ws.make_font(8, ws.text_color);
	ui->hex->default_color = ws.dark_color;
	ui->hex->source = ui->source;
	ui->hex->vscroll = ui->hex_scroll;
	b.ui.push_back(ui->hex);

	ui->hex_info = new Label();
	ui->hex_info->font = ui->hex_title->font;
	b.ui.push_back(ui->hex_info);

	b.markup = ui;
	b.update_handler = update_view_source;
	b.scale_change_handler = scale_change_handler;
	b.refresh_handler = refresh_handler;
	b.refresh_every = 1;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-250, -200, 500, 400};
	b.min_width = 400;
	b.min_height = 300;
	b.visible = true;
}
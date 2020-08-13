#include <numeric>
#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void update_view_source(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	auto ui = (View_Source*)b.markup;

	ui->cross->pos.y = b.cross_size * 0.5;
	ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
	ui->cross->pos.w = b.cross_size;
	ui->cross->pos.h = b.cross_size;

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

	float data_x = b.border;
	float data_y = ui->title->pos.y + ui->title->pos.h + 8;

	float goto_w = (float)ui->goto_digits + 5.0f;
	goto_w *= ui->goto_box->font->render.glyph_for('0')->box_w / view.scale;
	float goto_x = data_x;
	float goto_h = ui->goto_box->pos.h;
	float goto_y = b.box.h - b.border - goto_h;

	if (!ui->multiple_regions) {
		ui->goto_box->pos = {
			goto_x,
			goto_y,
			goto_w,
			goto_h
		};
	}
	else {
		goto_x = ui->div->position - goto_w / 2;
		ui->goto_box->pos = {
			goto_x,
			goto_y,
			goto_w,
			goto_h
		};

		float y = data_y;

		ui->div->pos = {
			ui->div->position,
			y,
			ui->div->breadth,
			goto_y - y - b.border
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
			goto_y - y - scroll_w - 3*b.border
		};

		ui->reg_lat_scroll->pos = {
			b.border,
			goto_y - scroll_w - 2*b.border,
			ui->reg_scroll->pos.x - 2*b.border,
			scroll_w
		};

		ui->reg_table->pos = {
			ui->reg_lat_scroll->pos.x,
			ui->reg_scroll->pos.y,
			ui->reg_lat_scroll->pos.w,
			ui->reg_scroll->pos.h
		};

		ui->reg_search->pos = {
			b.border,
			goto_y,
			120,
			goto_h
		};
	}

	ui->hex_title->pos = {
		data_x,
		data_y,
		b.box.w - data_x - b.border,
		ui->hex_title->font->render.text_height() * 1.1f / view.scale
	};
	ui->hex_title->update_position(view);

	float data_w = ui->hex_scroll->pos.x - ui->hex_title->pos.x - b.border;

	float size_w = ui->size_label->font->render.text_width(ui->size_label->text.c_str()) * 1.1f / view.scale;
	float size_font_h = ui->size_label->font->render.text_height();
	float size_h = size_font_h * 1.1f / view.scale;

	float size_x, size_y;
	if (ui->multiple_regions) {
		size_x = data_x + data_w - size_w;
		size_y = goto_y - size_h - b.border;
	}
	else {
		size_x = goto_x + goto_w + b.border;
		size_y = goto_y + ui->goto_box->text_off_y * size_font_h; //b.box.h - goto_h + (goto_h - size_h) - b.border;
	}

	ui->size_label->pos = {
		size_x,
		size_y,
		size_w,
		size_h
	};
	ui->size_label->update_position(view);

	if (ui->multiple_regions) {
		ui->reg_name->pos = {
			data_x,
			goto_y - b.border - ui->hex_title->pos.h,
			data_w - size_w,
			ui->hex_title->pos.h
		};
		ui->reg_name->update_position(view);
	}

	data_y += ui->hex_title->pos.h;

	float data_h = ui->multiple_regions ? size_y : goto_y;
	data_h -= data_y + b.border;

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

	ui->ascii_box->pos.x = b.box.w - b.border - ui->ascii_box->pos.w;
	ui->ascii_box->pos.y = b.box.h - b.border - ui->ascii_box->pos.h;

	ui->hex_box->pos.x = ui->ascii_box->pos.x - ui->hex_box->pos.w;
	ui->hex_box->pos.y = ui->ascii_box->pos.y;

	ui->addr_box->pos.x = ui->hex_box->pos.x - ui->addr_box->pos.w;
	ui->addr_box->pos.y = ui->hex_box->pos.y;

	ui->hex->show_addrs = ui->addr_box->checked;
	ui->hex->show_hex = ui->hex_box->checked;
	ui->hex->show_ascii = ui->ascii_box->checked;

	ui->hex->update(view.scale);

	if (ui->div)
		b.min_width = ui->div->position + b.box.w - ui->div->maximum;

	char buf[20];
	snprintf(buf, 19, "%#llx", ui->hex->region_address + ui->hex->offset);
	ui->goto_box->placeholder = buf;

	b.post_update_elements(view, input, inside, hover, focussed);
}

void update_regions_table(View_Source *ui) {
	int sel = ui->reg_table->sel_row;
	if (sel < 0) {
		ui->reg_name->text = "";
		return;
	}

	sel = ui->reg_table->data.index_from_filtered(sel);

	char *name = (char*)ui->reg_table->data.columns[0][sel];
	ui->reg_name->text = name ? name : "";

	u64 address = strtoull((char*)ui->reg_table->data.columns[1][sel], nullptr, 16);
	u64 size = strtoull((char*)ui->reg_table->data.columns[2][sel], nullptr, 16);

	ui->selected_region = address;
	ui->hex->set_region(address, size);
	ui->hex_scroll->position = 0;
}

void regions_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);
	table->sel_row = table->hl_row;

	auto ui = (View_Source*)table->parent->markup;
	ui->selected_region = 0;
	update_regions_table(ui);
}

void refresh_region_list(View_Source *ui, Point& cursor) {
	int n_rows = ui->reg_table->data.row_count();
	bool hovered = ui->reg_table->pos.contains(cursor);
	ui->source->block_region_refresh = hovered;

	if (!n_rows)
		ui->needs_region_update = true;

	if (!ui->needs_region_update && !ui->source->region_refreshed)
		return;

	int n_sources = ui->source->regions.size();
	if (n_sources != ui->region_list.size()) {
		u64 sel_addr = 0;

		ui->reg_table->data.resize(n_sources);
		ui->region_list.resize(n_sources);
		std::iota(ui->region_list.begin(), ui->region_list.end(), 0);
	}

	// place a hex digit LUT on the stack
	char hex[16];
	memcpy(hex, "0123456789abcdef", 16);

	auto& names = (std::vector<char*>&)ui->reg_table->data.columns[0];
	auto& addrs = (std::vector<char*>&)ui->reg_table->data.columns[1];
	auto& sizes = (std::vector<char*>&)ui->reg_table->data.columns[2];

	int idx = 0, sel = -1;
	for (auto& r : ui->region_list) {
		auto& reg = ui->source->regions[r];
		if (ui->selected_region == reg.base)
			sel = idx;

		print_hex(hex, addrs[idx], reg.base, 16);
		print_hex(hex, sizes[idx], reg.size, 8);
		names[idx] = reg.name;
		idx++;
	}

	ui->goto_digits = idx <= 0 ? 4 : count_digits(ui->source->regions[idx-1].base) + 2;

	if (ui->reg_table->data.filtered > 0) {
		ui->reg_table->data.update_filter(ui->reg_search->line);
		sel = ui->reg_table->data.filtered_from_index(sel);
	}

	ui->reg_table->sel_row = sel;

	ui->needs_region_update = false;
}

void goto_handler(Edit_Box *edit, Input& input) {
	if (input.enter != 1 || !edit->line.size())
		return;

	auto ui = (View_Source*)edit->parent->markup;

	u64 base = 0;
	u64 address = strtoull(edit->line.c_str(), nullptr, 16);

	if (ui->multiple_regions) {
		int sel_row = -1;
		for (auto& idx : ui->region_list) {
			auto& reg = ui->source->regions[idx];
			if (address >= reg.base && address < reg.base + reg.size) {
				sel_row = idx;
				base = reg.base;
				break;
			}
		}

		if (sel_row > 0)
			ui->reg_table->data.clear_filter();

		ui->reg_table->sel_row = sel_row;
		update_regions_table(ui);

		if (sel_row < 0)
			return;
	}

	u64 offset = address - base;
	offset -= offset % ui->hex->columns;
	ui->hex_scroll->position = (double)offset;

	edit->clear();
}

void region_search_handler(Edit_Box *edit, Input& input) {
	auto ui = (View_Source*)edit->parent->markup;
	ui->reg_table->data.update_filter(edit->line);
	ui->reg_table->hl_row = ui->reg_table->sel_row = -1;
	ui->reg_scroll->position = 0;
	ui->selected_region = 0;
}

static void refresh_handler(Box& b, Point& cursor) {
	auto ui = (View_Source*)b.markup;
	if (ui->multiple_regions) {
		refresh_region_list(ui, cursor);
	}
	else {
		ui->hex->set_region(0, ui->hex->source->regions[0].size);
		ui->goto_digits = count_digits(ui->hex->region_size) + 2;
	}

	u64 n = ui->hex->region_size;
	if (n > 0) {
		int n_digits = count_digits(n);

		char buf[40];
		if (ui->multiple_regions)
			snprintf(buf, 40, "0x%0*llx/0x%0*llx", n_digits, (u64)ui->hex->offset, n_digits, ui->hex->region_size);
		else
			snprintf(buf, 40, "/ 0x%0*llx", n_digits, ui->hex->region_size);

		ui->size_label->text = buf;
	}
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (View_Source*)b.markup;
	ui->cross->img = ws.cross;

	if (ui->div)
		ui->div->make_icon(new_scale);

	float goto_h = ui->goto_box->font->render.text_height() * 1.4f / new_scale;
	ui->goto_box->update_icon(IconGoto, goto_h, new_scale);

	if (ui->reg_search)
		ui->reg_search->update_icon(IconGlass, goto_h, new_scale);
}

void make_view_source_menu(Workspace& ws, Source *s, Box& b) {
	auto ui = new View_Source();
	ui->source = s;

	ui->cross = new Image();
	ui->cross->action = get_delete_box();
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title = new Label();
	ui->title->text = "View Source - ";
	ui->title->text += s->name;
	ui->title->font = ws.default_font;
	ui->title->padding = 0;
	b.ui.push_back(ui->title);

	ui->multiple_regions = s->regions.size() > 1;

	if (ui->multiple_regions) {
		ui->div = new Divider();
		ui->div->default_color = {0.5, 0.6, 0.8, 1.0};
		ui->div->breadth = 2;
		ui->div->vertical = true;
		ui->div->moveable = true;
		ui->div->minimum = 8 * ui->title->font->render.text_height();
		ui->div->position = ui->div->minimum + 100;
		ui->div->cursor_type = CursorResizeWestEast;
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
		ui->reg_table->column_spacing = 0.5;
		ui->reg_table->vscroll = ui->reg_scroll;
		ui->reg_table->hscroll = ui->reg_lat_scroll;

		float digit_units = (float)ui->reg_table->font->render.digit_width() / (float)ui->reg_table->font->render.text_height();
		float addr_w = 16 * digit_units;
		float size_w = 8 * digit_units;

		Column cols[] = {
			{String, 0, 0, 0, 0, "Name"},
			{String, 20, 0.2, addr_w, 0, "Address"},
			{String, 20, 0.1, size_w, 0, "Size"}
		};

		ui->reg_table->data.init(cols, 3, 0);
		b.ui.push_back(ui->reg_table);

		ui->reg_search = new Edit_Box();
		ui->reg_search->font = ui->reg_title->font;
		ui->reg_search->caret = ws.caret_color;
		ui->reg_search->default_color = ws.dark_color;
		ui->reg_search->icon_color = ws.text_color;
		ui->reg_search->icon_color.a = 0.6;
		ui->reg_search->key_action = region_search_handler;
		b.ui.push_back(ui->reg_search);

		ui->reg_name = new Label();
		ui->reg_name->font = ui->reg_title->font;
		b.ui.push_back(ui->reg_name);
	}

	float goto_font_size = 10;
	ui->hex_title = new Label();
	ui->hex_title->text = "Data";
	ui->hex_title->font = ws.make_font(goto_font_size, ws.text_color);
	b.ui.push_back(ui->hex_title);

	ui->size_label = new Label();
	ui->size_label->font = ui->hex_title->font;

	if (!ui->multiple_regions)
		ui->size_label->padding = 0;

	b.ui.push_back(ui->size_label);

	ui->hex_scroll = new Scroll();
	ui->hex_scroll->back = ws.scroll_back;
	ui->hex_scroll->default_color = ws.scroll_color;
	ui->hex_scroll->hl_color = ws.scroll_hl_color;
	ui->hex_scroll->sel_color = ws.scroll_sel_color;
	b.ui.push_back(ui->hex_scroll);

	ui->hex = new Hex_View();
	ui->hex->font = ws.make_font(8, ws.text_color);
	ui->hex->default_color = ws.dark_color;
	ui->hex->caret = ws.caret_color;
	ui->hex->source = ui->source;
	ui->hex->scroll = ui->hex_scroll;
	b.ui.push_back(ui->hex);

	ui->goto_box = new Edit_Box();
	ui->goto_box->caret = ws.caret_color;
	ui->goto_box->default_color = ws.dark_color;
	ui->goto_box->font = ui->hex_title->font;
	ui->goto_box->key_action = goto_handler;

	ui->goto_box->icon_color = ws.text_color;
	ui->goto_box->icon_color.a = 0.6;
	ui->goto_box->ph_font = ws.make_font(goto_font_size, ui->goto_box->icon_color);
	b.ui.push_back(ui->goto_box);

	ui->addr_box = new Checkbox();
	ui->addr_box->font = ui->size_label->font;
	ui->addr_box->text = "O";
	ui->addr_box->default_color = ws.scroll_back;
	ui->addr_box->hl_color = ws.light_color;
	ui->addr_box->sel_color = ws.cb_color;
	ui->addr_box->pos.h = ui->addr_box->font->render.text_height() * 1.1f / ws.temp_scale;
	ui->addr_box->pos.w = 2 * ui->addr_box->pos.h;
	ui->addr_box->action = [](UI_Element *elem, bool dbl_click) {auto c = dynamic_cast<Checkbox*>(elem); c->checked = !c->checked;};
	b.ui.push_back(ui->addr_box);

	ui->hex_box = new Checkbox();
	ui->hex_box->font = ui->size_label->font;
	ui->hex_box->text = "H";
	ui->hex_box->default_color = ui->addr_box->default_color;
	ui->hex_box->hl_color = ui->addr_box->hl_color;
	ui->hex_box->sel_color = ui->addr_box->sel_color;
	ui->hex_box->pos = ui->addr_box->pos;
	ui->hex_box->checked = true;
	ui->hex_box->action = ui->addr_box->action;
	b.ui.push_back(ui->hex_box);

	ui->ascii_box = new Checkbox();
	ui->ascii_box->font = ui->size_label->font;
	ui->ascii_box->text = "A";
	ui->ascii_box->default_color = ui->hex_box->default_color;
	ui->ascii_box->hl_color = ui->addr_box->hl_color;
	ui->ascii_box->sel_color = ui->addr_box->sel_color;
	ui->ascii_box->pos = ui->addr_box->pos;
	ui->ascii_box->action = ui->addr_box->action;
	b.ui.push_back(ui->ascii_box);

	b.markup = ui;
	b.update_handler = update_view_source;
	b.scale_change_handler = scale_change_handler;
	b.refresh_handler = refresh_handler;
	b.refresh_every = 1;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-300, -200, 600, 400};
	b.min_width = 400;
	b.min_height = 300;
	b.visible = true;
}
#include <numeric>
#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void View_Source::update_ui(Camera& view) {
	cross.pos.y = cross_size * 0.5;
	cross.pos.x = box.w - cross_size * 1.5;
	cross.pos.w = cross_size;
	cross.pos.h = cross_size;

	float w = title.font->render.text_width(title.text.c_str()) / view.scale;
	float max_title_w = box.w - 5 * cross_size;
	w = w < max_title_w ? w : max_title_w;

	title.pos = {
		(box.w - w) / 2,
		2,
		w,
		title.font->render.text_height() * 1.1f / view.scale
	};
	title.update_position(view.scale);

	float scroll_w = 14;

	float data_x = border;
	float data_y = title.pos.y + title.pos.h + 8;

	float goto_w = (float)goto_digits + 5.0f;
	goto_w *= goto_box.font->render.glyph_for('0')->box_w / view.scale;
	float goto_x = data_x;
	float goto_h = goto_box.pos.h;
	float goto_y = box.h - border - goto_h;

	if (menu_type == MenuFile) {
		goto_box.pos = {
			goto_x,
			goto_y,
			goto_w,
			goto_h
		};
	}
	else {
		goto_x = div.position - goto_w / 2;
		goto_box.pos = {
			goto_x,
			goto_y,
			goto_w,
			goto_h
		};

		float y = data_y;

		div.pos = {
			div.position,
			y,
			div.breadth,
			goto_y - y - border
		};
		div.maximum = box.w - div.minimum;

		float pad = div.padding;
		data_x = div.pos.x + div.pos.w + pad;

		reg_title.pos = {
			border,
			y,
			div.pos.x - border - pad,
			reg_title.font->render.text_height() * 1.1f / view.scale
		};
		reg_title.update_position(view.scale);

		y += reg_title.pos.h;

		reg_scroll.pos = {
			reg_title.pos.x + reg_title.pos.w - scroll_w,
			y,
			scroll_w,
			goto_y - y - scroll_w - 3*border
		};

		reg_lat_scroll.pos = {
			border,
			goto_y - scroll_w - 2*border,
			reg_scroll.pos.x - 2*border,
			scroll_w
		};

		reg_table.pos = {
			reg_lat_scroll.pos.x,
			reg_scroll.pos.y,
			reg_lat_scroll.pos.w,
			reg_scroll.pos.h
		};

		reg_search.pos = {
			border,
			goto_y,
			120,
			goto_h
		};
	}

	hex_title.pos = {
		data_x,
		data_y,
		box.w - data_x - border,
		hex_title.font->render.text_height() * 1.1f / view.scale
	};
	hex_title.update_position(view.scale);

	float data_w = hex_scroll.pos.x - hex_title.pos.x - border;

	float size_w = size_label.font->render.text_width(size_label.text.c_str()) * 1.1f / view.scale;
	float size_font_h = size_label.font->render.text_height();
	float size_h = size_font_h * 1.1f / view.scale;

	float size_x, size_y;
	if (menu_type == MenuProcess) {
		size_x = data_x + data_w - size_w;
		size_y = goto_y - size_h - border;
	}
	else {
		size_x = goto_x + goto_w + border;
		size_y = goto_y + goto_box.text_off_y * size_font_h; //box.h - goto_h + (goto_h - size_h) - border;
	}

	size_label.pos = {
		size_x,
		size_y,
		size_w,
		size_h
	};
	size_label.update_position(view.scale);

	if (menu_type == MenuProcess) {
		reg_name.pos = {
			data_x,
			goto_y - border - hex_title.pos.h,
			data_w - size_w,
			hex_title.pos.h
		};
		reg_name.update_position(view.scale);
	}

	data_y += hex_title.pos.h;

	float data_h = menu_type == MenuProcess ? size_y : goto_y;
	data_h -= data_y + border;

	hex_scroll.pos = {
		box.w - border - scroll_w,
		data_y,
		scroll_w,
		data_h
	};

	hex.pos = {
		hex_title.pos.x,
		data_y,
		data_w,
		data_h
	};

	ascii_box.pos.x = box.w - border - ascii_box.pos.w;
	ascii_box.pos.y = box.h - border - ascii_box.pos.h;

	hex_box.pos.x = ascii_box.pos.x - hex_box.pos.w;
	hex_box.pos.y = ascii_box.pos.y;

	addr_box.pos.x = hex_box.pos.x - addr_box.pos.w;
	addr_box.pos.y = hex_box.pos.y;

	columns.pos.x = addr_box.pos.x - columns.pos.w - 2*border;
	columns.pos.y = addr_box.pos.y;

	hex.show_addrs = addr_box.checked;
	hex.show_hex = hex_box.checked;
	hex.show_ascii = ascii_box.checked;

	hex.update(view.scale);

	if (menu_type == MenuProcess)
		min_width = div.position + box.w - div.maximum;

	char buf[20];
	snprintf(buf, 19, "%#llx", hex.region_address + hex.offset);
	goto_box.placeholder = buf;
}

void View_Source::update_regions_table() {
	int sel = reg_table.sel_row;
	if (sel < 0) {
		reg_name.text = "";
		return;
	}

	sel = reg_table.data->index_from_filtered(sel);

	char *name = (char*)reg_table.data->columns[0][sel];
	reg_name.text = name ? name : "";

	u64 address = strtoull((char*)reg_table.data->columns[1][sel], nullptr, 16);
	u64 size = strtoull((char*)reg_table.data->columns[2][sel], nullptr, 16);

	selected_region = address;
	hex.set_region(address, size);
	hex_scroll.position = 0;
}

void View_Source::refresh_region_list(Point& cursor) {
	int n_rows = table.row_count();
	bool hovered = reg_table.pos.contains(cursor);
	source->block_region_refresh = hovered;

	if (!n_rows)
		needs_region_update = true;

	if (!needs_region_update && !source->region_refreshed)
		return;

	int n_sources = source->regions.size();
	if (n_sources != region_list.size()) {
		u64 sel_addr = 0;

		reg_table.data->resize(n_sources);
		region_list.resize(n_sources);
		std::iota(region_list.begin(), region_list.end(), 0);
	}

	auto& names = (std::vector<char*>&)table.columns[0];
	auto& addrs = (std::vector<char*>&)table.columns[1];
	auto& sizes = (std::vector<char*>&)table.columns[2];

	int idx = 0, sel = -1;
	for (auto& r : region_list) {
		auto& reg = source->regions[r];
		if (selected_region == reg.base)
			sel = idx;

		print_hex(addrs[idx], reg.base, 16);
		print_hex(sizes[idx], reg.size, 8);
		names[idx] = reg.name;
		idx++;
	}

	goto_digits = idx <= 0 ? 4 : count_digits(source->regions[idx-1].base) + 2;

	if (table.filtered > 0) {
		table.update_filter(reg_search.editor.text);
		sel = table.filtered_from_index(sel);
	}

	reg_table.sel_row = sel;

	needs_region_update = false;
}

void regions_handler(UI_Element *elem, bool dbl_click) {
	auto ui = dynamic_cast<View_Source*>(elem->parent);
	ui->reg_table.sel_row = ui->reg_table.hl_row;
	ui->selected_region = 0;
	ui->update_regions_table();
}

void goto_handler(Edit_Box *edit, Input& input) {
	if (input.enter != 1 || !edit->editor.text.size())
		return;

	auto ui = dynamic_cast<View_Source*>(edit->parent);

	u64 base = 0;
	u64 address = strtoull(edit->editor.text.c_str(), nullptr, 16);

	if (ui->menu_type == MenuProcess) {
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
			ui->reg_table.data->clear_filter();

		ui->reg_table.sel_row = sel_row;
		ui->update_regions_table();

		if (sel_row < 0)
			return;
	}

	u64 offset = address - base;
	//offset -= offset % ui->hex.columns;
	ui->hex_scroll.position = (double)offset;

	edit->clear();
}

void region_search_handler(Edit_Box *edit, Input& input) {
	auto ui = dynamic_cast<View_Source*>(edit->parent);
	ui->reg_table.data->update_filter(edit->editor.text);
	ui->reg_table.hl_row = ui->reg_table.sel_row = -1;
	ui->reg_scroll.position = 0;
	ui->selected_region = 0;
}

void set_hex_columns(Number_Edit *edit, Input& input) {
	auto ui = dynamic_cast<View_Source*>(edit->parent);
	ui->hex.columns = edit->number > 0 ? edit->number : 16;
}

void View_Source::open_source(Source *s) {
	title.text = "View Source - ";
	title.text += s->name;
	hex.source = s;
	source = s;
}

void View_Source::refresh(Point& cursor) {
	if (menu_type == MenuProcess) {
		refresh_region_list(cursor);
	}
	else {
		hex.set_region(0, hex.source->regions[0].size);
		goto_digits = count_digits(hex.region_size) + 2;
	}

	u64 n = hex.region_size;
	if (n > 0) {
		int n_digits = count_digits(n);

		char buf[40];
		if (menu_type == MenuProcess)
			snprintf(buf, 40, "0x%0*llx/0x%0*llx", n_digits, (u64)hex.offset, n_digits, hex.region_size);
		else
			snprintf(buf, 40, "/ 0x%0*llx", n_digits, hex.region_size);

		size_label.text = buf;
	}
}

void View_Source::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;

	if (menu_type == MenuProcess)
		div.make_icon(new_scale);

	columns.make_icon(new_scale);

	float goto_h = goto_box.font->render.text_height() * 1.4f / new_scale;
	goto_box.update_icon(IconGoto, goto_h, new_scale);

	if (menu_type == MenuProcess)
		reg_search.update_icon(IconGlass, goto_h, new_scale);
}

View_Source::View_Source(Workspace& ws, MenuType mtype)
	: menu_type(mtype)
{
	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	title.font = ws.default_font;
	title.padding = 0;
	ui.push_back(&title);

	if (menu_type == MenuProcess) {
		div.default_color = ws.div_color;
		div.breadth = 2;
		div.vertical = true;
		div.moveable = true;
		div.minimum = 8 * title.font->render.text_height();
		div.position = div.minimum + 100;
		div.cursor_type = CursorResizeWestEast;
		ui.push_back(&div);

		reg_title.text = "Regions";
		reg_title.font = ws.make_font(10, ws.text_color);
		ui.push_back(&reg_title);

		reg_scroll.back = ws.scroll_back;
		reg_scroll.default_color = ws.scroll_color;
		reg_scroll.hl_color = ws.scroll_hl_color;
		reg_scroll.sel_color = ws.scroll_sel_color;
		reg_scroll.breadth = 12;
		ui.push_back(&reg_scroll);

		reg_lat_scroll.back = reg_scroll.back;
		reg_lat_scroll.default_color = reg_scroll.default_color;
		reg_lat_scroll.hl_color = reg_scroll.hl_color;
		reg_lat_scroll.sel_color = reg_scroll.sel_color;
		reg_lat_scroll.vertical = false;
		reg_lat_scroll.breadth = 12;
		ui.push_back(&reg_lat_scroll);

		reg_table.action = regions_handler;
		reg_table.font = ws.make_font(9, ws.text_color);
		reg_table.default_color = ws.dark_color;
		reg_table.hl_color = ws.hl_color;
		reg_table.sel_color = ws.light_color;
		reg_table.column_spacing = 0.5;
		reg_table.vscroll = &reg_scroll;
		reg_table.vscroll->content = &reg_table;
		reg_table.hscroll = &reg_lat_scroll;
		reg_table.hscroll->content = &reg_table;

		float digit_units = (float)reg_table.font->render.digit_width() / (float)reg_table.font->render.text_height();
		float addr_w = 16 * digit_units;
		float size_w = 8 * digit_units;

		Column cols[] = {
			{ColumnString, 0, 0, 0, 0, "Name"},
			{ColumnString, 20, 0.2, addr_w, 0, "Address"},
			{ColumnString, 20, 0.1, size_w, 0, "Size"}
		};

		table.init(cols, nullptr, 3, 0);
		reg_table.data = &table;
		ui.push_back(&reg_table);

		reg_search.font = reg_title.font;
		reg_search.caret = ws.caret_color;
		reg_search.default_color = ws.dark_color;
		reg_search.icon_color = ws.text_color;
		reg_search.icon_color.a = 0.6;
		reg_search.key_action = region_search_handler;
		ui.push_back(&reg_search);

		reg_name.font = reg_title.font;
		ui.push_back(&reg_name);
	}
	else {
		div.visible = false;
		reg_title.visible = false;
		reg_scroll.visible = false;
		reg_lat_scroll.visible = false;
		reg_table.visible = false;
		reg_search.visible = false;
		reg_name.visible = false;
	}

	float goto_font_size = 10;
	hex_title.text = "Data";
	hex_title.font = ws.make_font(goto_font_size, ws.text_color);
	ui.push_back(&hex_title);

	size_label.font = hex_title.font;

	if (menu_type == MenuFile)
		size_label.padding = 0;

	ui.push_back(&size_label);

	hex_scroll.back = ws.scroll_back;
	hex_scroll.default_color = ws.scroll_color;
	hex_scroll.hl_color = ws.scroll_hl_color;
	hex_scroll.sel_color = ws.scroll_sel_color;
	ui.push_back(&hex_scroll);

	hex.font = ws.make_font(8, ws.text_color);
	hex.default_color = ws.dark_color;
	hex.back_color = ws.back_color;
	hex.caret = ws.caret_color;
	hex.scroll = &hex_scroll;
	ui.push_back(&hex);

	goto_box.caret = ws.caret_color;
	goto_box.default_color = ws.dark_color;
	goto_box.font = hex_title.font;
	goto_box.key_action = goto_handler;

	goto_box.icon_color = ws.text_color;
	goto_box.icon_color.a = 0.6;
	goto_box.ph_font = ws.make_font(goto_font_size, goto_box.icon_color);
	ui.push_back(&goto_box);

	addr_box.font = size_label.font;
	addr_box.text = "O";
	addr_box.default_color = ws.scroll_back;
	addr_box.hl_color = ws.light_color;
	addr_box.sel_color = ws.cb_color;
	addr_box.pos.h = addr_box.font->render.text_height() * 1.1f / ws.temp_scale;
	addr_box.pos.w = 2 * addr_box.pos.h;
	addr_box.action = [](UI_Element *elem, bool dbl_click) {auto c = dynamic_cast<Checkbox*>(elem); c->toggle();};
	ui.push_back(&addr_box);

	hex_box.font = size_label.font;
	hex_box.text = "H";
	hex_box.default_color = addr_box.default_color;
	hex_box.hl_color = addr_box.hl_color;
	hex_box.sel_color = addr_box.sel_color;
	hex_box.pos = addr_box.pos;
	hex_box.checked = true;
	hex_box.action = addr_box.action;
	ui.push_back(&hex_box);

	ascii_box.font = size_label.font;
	ascii_box.text = "A";
	ascii_box.default_color = hex_box.default_color;
	ascii_box.hl_color = addr_box.hl_color;
	ascii_box.sel_color = addr_box.sel_color;
	ascii_box.pos = addr_box.pos;
	ascii_box.action = addr_box.action;
	ui.push_back(&ascii_box);

	columns.font = size_label.font;
	columns.default_color = ws.dark_color;
	columns.sel_color = ws.dark_color;
	columns.editor.text = "16";
	columns.pos.w = 40;
	columns.pos.h = addr_box.pos.h;
	columns.key_action = set_hex_columns;
	ui.push_back(&columns);

	refresh_every = 1;
	back = ws.back_color;
	edge_color = ws.dark_color;
	box = {-300, -200, 600, 400};
	min_width = 400;
	min_height = 300;
	expungable = true;
}
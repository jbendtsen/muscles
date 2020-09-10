#include "muscles.h"
#include "ui.h"
#include "dialog.h"

#include <algorithm>

void View_Definitions::update_ui(Camera& camera) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

	float w = title.font->render.text_width(title.text.c_str()) / camera.scale;
	float max_title_w = box.w - 5 * cross_size;
	w = w < max_title_w ? w : max_title_w;

	title.pos = {
		(box.w - w) / 2,
		2,
		w,
		title.font->render.text_height() * 1.1f / camera.scale
	};

	float y = 2*cross.pos.y + cross.pos.h;

	tabs.pos = {
		border,
		y,
		box.w - 2*border,
		tabs.font->render.text_height() * 1.4f / camera.scale
	};
	y += tabs.pos.h;

	view.pos = {
		tabs.pos.x,
		y,
		tabs.pos.w,
		box.h - border - y
	};
}

void View_Definitions::update_tables(Workspace *ws) {
	types.clear_data();
	enums.clear_data();

	auto& types_vec = (std::vector<s64>&)types.columns[0];
	auto& enums_vec = (std::vector<s64>&)enums.columns[0];

	if (!ws) ws = parent;
	auto& defs = ws->definitions;
	int n_buckets = defs.size();

	for (int i = 0; i < n_buckets; i++) {
		u32 flags = defs.data[i].flags;
		if ((flags & FLAG_OCCUPIED) == 0)
			continue;

		int name_idx = defs.data[i].name_idx;
		if (flags & FLAG_PRIMITIVE)
			types_vec.push_back(name_idx);
		else if (flags & FLAG_ENUM)
			enums_vec.push_back(name_idx);
	}

	String_Vector *sv = defs.sv;
	auto sort_names = [sv](s64 a, s64 b) {
		return strcmp(sv->at(a), sv->at(b)) < 0;
	};

	std::sort(types_vec.begin(), types_vec.end(), sort_names);
	std::sort(enums_vec.begin(), enums_vec.end(), sort_names);

	int n_types = types_vec.size();
	types.resize(n_types);

	for (int i = 0; i < n_types; i++) {
		char *name = sv->at(types_vec[i]);
		types.columns[0][i] = (void*)name;

		Bucket& buck = defs[name];
		if (buck.flags & FLAG_EXTERNAL) {
			const u32 mask = FLAG_OCCUPIED | FLAG_EXTERNAL | FLAG_PRIMITIVE;
			while ((buck.flags & mask) == mask)
				buck = defs[sv->at(buck.value)];

			if ((buck.flags & (FLAG_EXTERNAL | FLAG_PRIMITIVE)) != FLAG_PRIMITIVE)
				continue;
		}

		// Size
		types.columns[1][i] = (void*)buck.value;
		// Big Endian, Float, Signed
		SET_TABLE_CHECKBOX((&types), 2, i, buck.flags & FLAG_BIG_ENDIAN);
		SET_TABLE_CHECKBOX((&types), 3, i, buck.flags & FLAG_FLOAT);
		SET_TABLE_CHECKBOX((&types), 4, i, buck.flags & FLAG_SIGNED);
	}

	int n_enums = enums_vec.size();
	enums.resize(n_enums);

	for (int i = 0; i < n_enums; i++) {
		char *name = sv->at(types_vec[i]);
		types.columns[0][i] = (void*)name;
		types.columns[1][i] = (void*)defs[name].value;
	}

	require_redraw();
}

void View_Definitions::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;
}

View_Definitions::View_Definitions(Workspace& ws) {
	float scale = get_default_camera().scale;

	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;
	ui.push_back(&maxm);

	title.font = ws.default_font;
	title.text = "Definitions";
	title.padding = 0;
	ui.push_back(&title);

	const char *tab_names[] = {
		"Types", "Enums", "Constants", "All"
	};

	Column type_cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnDec, 0, 0.2, 3, 5, "Size"},
		{ColumnCheckbox, 0, 0.1, 1.5, 1.5, "BE"},
		{ColumnCheckbox, 0, 0.1, 1.5, 1.5, "FP"},
		{ColumnCheckbox, 0, 0.1, 1.5, 1.5, "S"}
	};
	types.init(type_cols, nullptr, 5, 0);

	Column enum_cols[] = {
		{ColumnString, 0, 0.7, 0, 0, "Name"},
		{ColumnDec, 0, 0.3, 0, 0, "Value"}
	};
	enums.init(enum_cols, nullptr, 2, 0);

	tables.push_back(&types);
	tables.push_back(&enums);
	tables.push_back(&constants);
	tables.push_back(nullptr);

	update_tables(&ws);

	tabs.add(tab_names, 4);
	tabs.font = ws.default_font;
	tabs.default_color = ws.dark_color;
	tabs.sel_color = ws.scroll_back;
	tabs.hl_color = ws.hl_color;
	tabs.event = [](Tabs *tabs) {
		if (tabs->sel >= 0) {
			auto ui = dynamic_cast<View_Definitions*>(tabs->parent);
			ui->view.data = ui->tables[tabs->sel];
		}
	};
	ui.push_back(&tabs);

	vscroll.back_color = ws.scroll_back;
	vscroll.default_color = ws.scroll_color;
	vscroll.hl_color = ws.scroll_hl_color;
	vscroll.sel_color = ws.scroll_sel_color;

	hscroll.back_color = ws.scroll_back;
	hscroll.default_color = ws.scroll_color;
	hscroll.hl_color = ws.scroll_hl_color;
	hscroll.sel_color = ws.scroll_sel_color;
	hscroll.vertical = false;

	view.show_column_names = true;
	view.font = ws.make_font(11, ws.text_color, scale);
	view.default_color = ws.scroll_back;
	view.hl_color = ws.hl_color;
	view.back_color = ws.scroll_back;
	view.hscroll = &hscroll;
	view.hscroll->content = &view;
	view.vscroll = &vscroll;
	view.vscroll->content = &view;

	arena.set_rewind_point();

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 0, 0.5, 0, 0, "Value"}
	};

	constants.init(cols, &arena, 0, 2);
	view.data = &constants;
	tabs.sel = 2;

	ui.push_back(&view);
	ui.push_back(&vscroll);
	ui.push_back(&hscroll);

	back = ws.back_color;
	edge_color = ws.dark_color;

	initial_width = 400;
	initial_height = 450;
}
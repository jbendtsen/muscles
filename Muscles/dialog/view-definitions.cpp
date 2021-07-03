#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

#include <algorithm>
#include <numeric>

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
	std::vector<s64> enum_name_idxs;

	if (!ws) ws = parent;
	auto& defs = ws->definitions;
	int n_buckets = defs.size();

	for (int i = 0; i < n_buckets; i++) {
		u32 flags = defs.data[i].flags;
		if ((flags & FLAG_OCCUPIED) == 0)
			continue;

		int name_idx = defs.data[i].name_idx;
		char *name = defs.sv->at(name_idx);
		if (flags & FLAG_PRIMITIVE)
			types_vec.push_back(name_idx);
		else if (flags & FLAG_ENUM)
			enum_name_idxs.push_back(name_idx);
	}

	String_Vector *sv = defs.sv;
	auto sort_names = [sv](s64 a, s64 b) {
		return strcmp(sv->at(a), sv->at(b)) < 0;
	};

	std::sort(types_vec.begin(), types_vec.end(), sort_names);
	std::sort(enum_name_idxs.begin(), enum_name_idxs.end(), sort_names);

	int n_types = types_vec.size();
	types.resize(n_types);

	for (int i = 0; i < n_types; i++) {
		char *name = defs.sv->at(types_vec[i]);
		types.columns[0][i] = (void*)name;

		Bucket& buck = defs[name];
		if (buck.flags & FLAG_EXTERNAL) {
			const u32 mask = FLAG_OCCUPIED | FLAG_EXTERNAL | FLAG_PRIMITIVE;
			while ((buck.flags & mask) == mask)
				buck = defs[defs.sv->at(buck.value)];

			if ((buck.flags & (FLAG_EXTERNAL | FLAG_PRIMITIVE)) != FLAG_PRIMITIVE)
				continue;
		}

		// Size
		types.columns[1][i] = (void*)buck.value;
		// Big Endian, Float, Signed
		types.set_checkbox(2, i, buck.flags & FLAG_BIG_ENDIAN);
		types.set_checkbox(3, i, buck.flags & FLAG_FLOAT);
		types.set_checkbox(4, i, buck.flags & FLAG_SIGNED);
	}

	enums.branches.clear();
	enums.branch_name_vector.head = 0;

	int n_elems = 0;
	int n_enums = enum_name_idxs.size();
	for (int i = 0; i < n_enums; i++) {
		char *name = defs.sv->at(enum_name_idxs[i]);
		if (!name)
			continue;

		Bucket& buck = defs[name];
		const u32 mask = FLAG_OCCUPIED | FLAG_ENUM;
		if ((buck.flags & mask) != mask || !buck.pointer)
			continue;

		int *data = (int*)buck.pointer;
		int size = *data++ / sizeof(int);
		enums.resize(enums.row_count() + size);

		int name_idx = enums.branch_name_vector.add_string(name);
		enums.branches.push_back({
			n_elems,
			size,
			name_idx,
			false
		});

		for (int j = 0; j < size; j++) {
			char *elem = defs.sv->at(data[j]);
			if (elem) {
				enums.columns[0][n_elems] = elem;
				enums.columns[1][n_elems] = (void*)defs[elem].value;
			}
			n_elems++;
		}
	}

	enums.update_tree();

	require_redraw();
}

void View_Definitions::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	sdl_destroy_texture(&view.icon_plus);
	sdl_destroy_texture(&view.icon_minus);

	int len = view.font->render.text_height() + 0.5;
	RGBA color = {1, 1, 1, 0.8};
	view.icon_plus = make_plus_minus_icon(color, len, true);
	view.icon_minus = make_plus_minus_icon(color, len, false);
}

View_Definitions::View_Definitions(Workspace& ws, MenuType mtype) {
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
	types.init(type_cols, nullptr, nullptr, nullptr, 5, 0);

	Column enum_cols[] = {
		{ColumnString, 0, 0.7, 0, 0, "Name"},
		{ColumnDec, 0, 0.3, 0, 0, "Value"}
	};
	enums.init(enum_cols, nullptr, nullptr, nullptr, 2, 0);

	tables.push_back(&types);
	tables.push_back(&enums);
	tables.push_back(&constants);
	tables.push_back(nullptr);

	update_tables(&ws);

	tabs.add(tab_names, 4);
	tabs.font = ws.default_font;
	tabs.default_color = ws.colors.dark;
	tabs.sel_color = ws.colors.scroll_back;
	tabs.hl_color = ws.colors.hl;
	tabs.event = [](Tabs *tabs) {
		if (tabs->sel >= 0) {
			auto ui = dynamic_cast<View_Definitions*>(tabs->parent);
			ui->view.data = ui->tables[tabs->sel];
		}
	};
	ui.push_back(&tabs);

	vscroll.back_color = ws.colors.scroll_back;
	vscroll.default_color = ws.colors.scroll;
	vscroll.hl_color = ws.colors.scroll_hl;
	vscroll.sel_color = ws.colors.scroll_sel;

	hscroll.back_color = ws.colors.scroll_back;
	hscroll.default_color = ws.colors.scroll;
	hscroll.hl_color = ws.colors.scroll_hl;
	hscroll.sel_color = ws.colors.scroll_sel;
	hscroll.vertical = false;

	view.show_column_names = true;
	view.font = ws.make_font(11, ws.colors.text, scale);
	view.default_color = ws.colors.scroll_back;
	view.hl_color = ws.colors.hl;
	view.sel_color = ws.colors.hl;
	view.back_color = ws.colors.scroll_back;
	view.hscroll = &hscroll;
	view.hscroll->content = &view;
	view.vscroll = &vscroll;
	view.vscroll->content = &view;

	arena.set_rewind_point();

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, "Name"},
		{ColumnString, 0, 0.5, 0, 0, "Value"}
	};

	constants.init(cols, &arena, nullptr, nullptr, 0, 2);
	view.data = &constants;
	tabs.sel = 2;

	ui.push_back(&view);
	ui.push_back(&vscroll);
	ui.push_back(&hscroll);

	initial_width = 400;
	initial_height = 450;
}

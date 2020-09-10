#include "muscles.h"
#include "ui.h"
#include "dialog.h"

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

	tables.push_back(&types);
	tables.push_back(&enums);
	tables.push_back(&constants);
	tables.push_back(nullptr);

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

	//view.show_column_names = true;
	view.font = ws.make_font(11, ws.text_color, scale);
	view.default_color = ws.scroll_back;
	view.hl_color = ws.hl_color;
	view.back_color = ws.back_color;
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
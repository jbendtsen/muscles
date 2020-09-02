#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void Main_Menu::update_ui(Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	if (sources_view.sel_row >= 0) {
		button.set_active(true);
		view_dd.content[0] = (char*)sources_view.data->columns[2][sources_view.sel_row];
	}
	else {
		button.set_active(false);
		view_dd.content[0] = (char*)"<source>";
	}

	update_elements(view, input, inside, hover, focussed);

	float x = border;
	float y = border;
	float dd_x = x;

	sources_dd.pos = { dd_x, y, 80, 25 };
	dd_x += sources_dd.pos.w;

	edit_dd.pos = { dd_x, y, 56, 25 };
	dd_x += edit_dd.pos.w;

	view_dd.pos = { dd_x, y, 56, 25 };

	y += sources_dd.pos.h + border;

	float view_w = button.width;
	float view_h = button.height;

	button.pos = {
		box.w - border - view_w,
		box.h - border - view_h,
		view_w,
		view_h
	};

	y += 10;

	sources_view.pos = {
		x,
		y,
		box.w - 2*x,
		box.h - y - 2*border - view_h
	};

	post_update_elements(view, input, inside, hover, focussed);
}

void Main_Menu::refresh(Point& cursor) {
	auto& sources = (std::vector<Source*>&)parent->sources;

	int n_sources = sources.size();
	sources_view.data->resize(n_sources);
	if (!n_sources)
		return;

	auto& icons = sources_view.data->columns[0];
	auto& pids = (std::vector<char*>&)sources_view.data->columns[1];
	auto& names = (std::vector<char*>&)sources_view.data->columns[2];

	Arena *arena = get_default_arena();
	for (int i = 0; i < n_sources; i++) {
		if (!pids[i])
			pids[i] = (char*)arena->allocate(8);

		if (sources[i]->type == SourceProcess) {
			icons[i] = process_icon;
			snprintf(pids[i], 7, "%d", sources[i]->pid);
		}
		else if (sources[i]->type == SourceFile) {
			icons[i] = file_icon;
			strcpy(pids[i], "---");
		}
		else {
			icons[i] = nullptr;
			pids[i][0] = 0;
		}
	}

	int idx = 0;
	for (auto& n : names) {
		int len = sources[idx]->name.size();
		if (!n || strlen(n) != len)
			n = (char*)arena->allocate(len + 1);

		strcpy(n, sources[idx]->name.c_str());
		idx++;
	}
}

void open_view_source(Workspace& ws, int idx) {
	if (idx < 0)
		return;

	MenuType type = ws.sources[idx]->type == SourceFile ? MenuFile : MenuProcess;
	auto vs = dynamic_cast<View_Source*>(ws.make_box(BoxViewSource, type));
	vs->open_source(ws.sources[idx]);
}

void sources_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto dd = dynamic_cast<Drop_Down*>(elem);
	if (dd->sel < 0)
		return;

	if (dd->sel == 0 || dd->sel == 1) {
		Workspace *ws = dd->parent->parent;
		MenuType type = dd->sel == 0 ? MenuFile : MenuProcess;
		ws->make_box(BoxOpenSource, type);
	}

	dd->parent->set_dropdown(nullptr);
}

void edit_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto dd = dynamic_cast<Drop_Down*>(elem);
	if (dd->sel < 0 || dd->sel > 2)
		return;

	auto ui = (Main_Menu*)dd->parent;
	Workspace *ws = dd->parent->parent;

	BoxType types[] = {BoxObject, BoxStructs, BoxDefinitions};
	ws->make_box(types[dd->sel]);

	dd->parent->set_dropdown(nullptr);
}

void view_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto dd = dynamic_cast<Drop_Down*>(elem);
	if (dd->sel < 0)
		return;

	auto ui = (Main_Menu*)dd->parent;
	Workspace *ws = dd->parent->parent;

	if (dd->sel == 0)
		open_view_source(*ws, ui->sources_view.sel_row);

	dd->parent->set_dropdown(nullptr);
}

void table_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);

	if (dbl_click)
		open_view_source(*elem->parent->parent, table->sel_row);
	else
		table->sel_row = table->hl_row;
}

void Main_Menu::handle_zoom(Workspace& ws, float new_scale) {
	sdl_destroy_texture(&file_icon);
	sdl_destroy_texture(&process_icon);

	float h = sources_view.font->render.text_height();
	file_icon = make_file_icon(file_back, file_fold, file_line, h, h);
	process_icon = make_process_icon(process_back, process_outline, h);

	int n_rows = sources_view.data->row_count();
	auto& icons = (std::vector<Texture>&)sources_view.data->columns[0];
	for (int i = 0; i < n_rows; i++) {
		Source_Type type = ((Source*)ws.sources[i])->type;
		if (type == SourceFile)
			icons[i] = file_icon;
		else if (type == SourceProcess)
			icons[i] = process_icon;
	}
}

Main_Menu::Main_Menu(Workspace& ws) {
	process_back = {0.8, 0.8, 0.8, 1.0};
	process_outline = {0.6, 0.6, 0.6, 1.0};

	file_back = {0.8, 0.8, 0.8, 1.0};
	file_fold = {0.9, 0.9, 0.9, 1.0};
	file_line = {0.5, 0.5, 0.5, 1.0};

	sources_dd.title = "Sources";
	sources_dd.font = ws.make_font(11, ws.text_color);
	sources_dd.action = sources_main_menu_handler;
	sources_dd.default_color = ws.back_color;
	sources_dd.hl_color = ws.hl_color;
	sources_dd.sel_color = ws.active_color;
	sources_dd.width = 150;
	sources_dd.content = {
		(char*)"Add File",
		(char*)"Add Process"
	};

	edit_dd.title = "Edit";
	edit_dd.font = sources_dd.font;
	edit_dd.action = edit_main_menu_handler;
	edit_dd.default_color = ws.back_color;
	edit_dd.hl_color = ws.hl_color;
	edit_dd.sel_color = ws.active_color;
	edit_dd.width = 150;
	edit_dd.content = {
		(char*)"New Object",
		(char*)"Structs",
		(char*)"Definitions",
		(char*)"Mappings"
	};

	view_dd.title = "View";
	view_dd.font = sources_dd.font;
	view_dd.action = view_main_menu_handler;
	view_dd.default_color = ws.back_color;
	view_dd.hl_color = ws.hl_color;
	view_dd.sel_color = ws.active_color;
	view_dd.width = 140;
	view_dd.content = {
		(char*)"<source>",
		(char*)"Search"
	};

	sources_view.show_column_names = true;
	sources_view.action = table_main_menu_handler;
	sources_view.font = ws.default_font;
	sources_view.default_color = ws.dark_color;
	sources_view.hl_color = ws.hl_color;
	sources_view.sel_color = ws.light_color;
	sources_view.back_color = ws.back_color;

	Column sources_cols[] = {
		{ColumnImage, 0, 0.1, 0, 1.2, ""},
		{ColumnString, 0, 0.2, 0, 3, "PID"},
		{ColumnString, 0, 0.7, 0, 0, "Name"},
	};
	table.init(sources_cols, nullptr, 3, 0);
	sources_view.data = &table;

	button.action = [](UI_Element* elem, bool dbl_click) {
		Data_View *view = &dynamic_cast<Main_Menu*>(elem->parent)->sources_view;
		open_view_source(*elem->parent->parent, view->sel_row);
	};
	button.active_theme = {
		ws.light_color,
		ws.light_color,
		ws.hl_color,
		ws.make_font(11, ws.text_color)
	};
	button.inactive_theme = {
		ws.inactive_color,
		ws.inactive_color,
		ws.inactive_color,
		ws.make_font(11, ws.inactive_text_color)
	};
	button.text = "View";
	button.active = false;
	button.update_size(ws.temp_scale);

	ui.push_back(&sources_dd);
	ui.push_back(&edit_dd);
	ui.push_back(&view_dd);
	ui.push_back(&sources_view);
	ui.push_back(&button);

	refresh_every = 1;
	box = { -200, -100, 400, 200 };
	back = ws.back_color;
	edge_color = ws.dark_color;
}

void opening_menu_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);

	switch (table->hl_row) {
		case 0: // New workspace
		{
			Workspace *ws = table->parent->parent;
			ws->make_box(BoxMain);
			break;
		}
	}

	if (table->hl_row == 0)
		table->parent->visible = false;
}

void Opening_Menu::update_ui(Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	update_elements(view, input, inside, hover, focussed);

	float y = border;
	y += center_align_title(&title, *this, view.scale, y);

	menu.pos.x = border;
	menu.pos.y = y;
	menu.pos.w = box.w - border * 2;
	menu.pos.h = box.h - y - border;

	post_update_elements(view, input, inside, hover, focussed);
}

Opening_Menu::Opening_Menu(Workspace& ws) {
	title.font = ws.default_font;
	title.text = "Muscles";

	menu.font = ws.default_font;
	menu.hl_color = ws.hl_color;
	menu.default_color = ws.back_color;

	Column menu_column = { ColumnString, 0, 1, 0, 0, "" };
	table.init(&menu_column, nullptr, 1, 2);
	table.columns[0][0] = (void*)"New";
	table.columns[0][1] = (void*)"Open Workspace";

	menu.data = &table;
	menu.action = opening_menu_handler;

	ui.push_back(&title);
	ui.push_back(&menu);

	back = ws.back_color;
	box = {-80, -50, 160, 100};
	visible = true;
}
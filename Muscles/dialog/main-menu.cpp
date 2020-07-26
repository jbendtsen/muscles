#include "../muscles.h"
#include "../ui.h"
#include "../io.h"
#include "dialog.h"

void update_main_menu(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	Main_Menu *ui = (Main_Menu*)b.markup;
	if (ui->table->sel_row >= 0) {
		ui->button->active = true;
		ui->view->lines[0] = (char*)ui->table->data.columns[2][ui->table->sel_row];
	}
	else {
		ui->button->active = false;
		ui->view->lines[0] = "<source>";
	}

	b.update_elements(view, input, inside, hovered, focussed);

	float x = b.border;
	float y = b.border;

	ui->sources->pos = { x, y, 75, 25 };

	ui->view->pos = { x + ui->sources->pos.w, y, 50, 25 };

	y += ui->sources->pos.h + b.border;

	float view_w = ui->button->width;
	float view_h = ui->button->height;

	ui->button->pos = {
		b.box.w - b.border - view_w,
		b.box.h - b.border - view_h,
		view_w,
		view_h
	};

	y += 10;

	ui->table->pos = {
		x,
		y,
		b.box.w - 2*x,
		b.box.h - y - 2*b.border - view_h
	};

	if (hovered && input.lclick) {
		b.moving = true;
		b.select_edge(view, inside);
	}

	b.post_update_elements(view, input, inside, hovered, focussed);
}

void refresh_main_menu(Box& b) {
	auto ui = (Main_Menu*)b.markup;
	auto& sources = (std::vector<Source*>&)b.parent->sources;

	int n_sources = sources.size();
	ui->table->data.resize(n_sources);
	if (!n_sources)
		return;

	auto& icons = ui->table->data.columns[0];
	auto& pids = (std::vector<char*>&)ui->table->data.columns[1];
	auto& names = (std::vector<char*>&)ui->table->data.columns[2];

	Arena *arena = get_arena();
	for (int i = 0; i < n_sources; i++) {
		if (!pids[i])
			pids[i] = (char*)arena->allocate(8);

		if (sources[i]->type == TypeProcess) {
			icons[i] = ui->process_icon;
			snprintf(pids[i], 7, "%d", sources[i]->pid);
		}
		else if (sources[i]->type == TypeFile) {
			icons[i] = ui->file_icon;
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

	Box *b = new Box();
	make_view_source_menu(ws, (Source&)ws.sources[idx], *b);
	ws.add_box(b);
	ws.focus = ws.boxes.back();
}

void sources_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto dd = dynamic_cast<Drop_Down*>(elem);
	if (dd->sel < 0)
		return;

	if (dd->sel == 0 || dd->sel == 1) {
		Box *box = new Box();
		Workspace *ws = dd->parent->parent;

		if (dd->sel == 0)
			make_file_menu(*ws, *box);
		else
			make_process_menu(*ws, *box);

		ws->add_box(box);
		ws->focus = ws->boxes.back();
	}

	dd->parent->set_dropdown(nullptr);
}

void view_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto dd = dynamic_cast<Drop_Down*>(elem);
	if (dd->sel < 0)
		return;

	if (dd->sel == 0) {
		auto ui = (Main_Menu*)dd->parent->markup;
		open_view_source(*elem->parent->parent, ui->table->sel_row);
	}

	dd->parent->set_dropdown(nullptr);
}

void table_main_menu_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);

	if (dbl_click)
		open_view_source(*elem->parent->parent, table->sel_row);
	else
		table->sel_row = table->hl_row;
}

void main_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Main_Menu*)b.markup;

	sdl_destroy_texture(&ui->file_icon);
	sdl_destroy_texture(&ui->process_icon);

	float h = ui->table->font->render.text_height();
	ui->file_icon = sdl_make_file_icon(ui->file_back, ui->file_fold, ui->file_line, h, h);
	ui->process_icon = sdl_make_process_icon(ui->process_back, ui->process_outline, h);

	int n_rows = ui->table->data.row_count();
	auto& icons = (std::vector<Texture>&)ui->table->data.columns[0];
	for (int i = 0; i < n_rows; i++) {
		SourceType type = ((Source*)ws.sources[i])->type;
		if (type == TypeFile)
			icons[i] = ui->file_icon;
		else if (type == TypeProcess)
			icons[i] = ui->process_icon;
	}
}

void make_main_menu(Workspace& ws, Box& b) {
	Main_Menu *ui = new Main_Menu();

	ui->process_back = {0.8, 0.8, 0.8, 1.0};
	ui->process_outline = {0.6, 0.6, 0.6, 1.0};

	ui->file_back = {0.8, 0.8, 0.8, 1.0};
	ui->file_fold = {0.9, 0.9, 0.9, 1.0};
	ui->file_line = {0.5, 0.5, 0.5, 1.0};

	ui->sources = new Drop_Down();
	ui->sources->title = "Sources";
	ui->sources->font = ws.make_font(11, ws.text_color);
	ui->sources->action = sources_main_menu_handler;
	ui->sources->default_color = ws.back_color;
	ui->sources->hl_color = ws.hl_color;
	ui->sources->sel_color = ws.sel_color;
	ui->sources->lines = {
		"Add File",
		"Add Process"
	};

	ui->view = new Drop_Down();
	ui->view->title = "View";
	ui->view->font = ui->sources->font;
	ui->view->action = view_main_menu_handler;
	ui->view->default_color = ws.back_color;
	ui->view->hl_color = ws.hl_color;
	ui->view->sel_color = ws.sel_color;
	ui->view->lines = {
		"<source>",
		"Mappings",
		"Types",
		"Structs",
		"Instances"
	};

	ui->table = new Data_View();
	ui->table->show_column_names = true;
	ui->table->action = table_main_menu_handler;
	ui->table->font = ws.default_font;
	ui->table->default_color = ws.dark_color;
	ui->table->hl_color = ws.hl_color;
	ui->table->sel_color = ws.light_color;

	Column sources_cols[] = {
		{Tex, 0, 0.1, 1.2, ""},
		{String, 0, 0.2, 3, "PID"},
		{String, 0, 0.7, 0, "Name"},
	};
	ui->table->data.init(sources_cols, 3, 0);

	ui->button = new Button();

	ui->button->action = [](UI_Element* elem, bool dbl_click) {
		auto ui = (Main_Menu*)elem->parent->markup;
		open_view_source(*elem->parent->parent, ui->table->sel_row);
	};

	ui->button->active_theme = {
		ws.light_color,
		ws.light_color,
		ws.hl_color,
		ws.make_font(11, ws.text_color)
	};
	ui->button->inactive_theme = {
		ws.inactive_color,
		ws.inactive_color,
		ws.inactive_color,
		ws.make_font(11, ws.inactive_text_color)
	};
	ui->button->text = "View";
	ui->button->active = false;
	ui->button->update_size(ws.temp_scale);

	b.ui.push_back(ui->sources);
	b.ui.push_back(ui->view);
	b.ui.push_back(ui->table);
	b.ui.push_back(ui->button);

	b.type = TypeMain;
	b.visible = true;
	b.markup = ui;
	b.update_handler = update_main_menu;
	b.scale_change_handler = main_scale_change_handler;
	b.refresh_handler = refresh_main_menu;
	b.refresh_every = 1;
	b.box = { -200, -100, 400, 200 };
	b.back = ws.back_color;
}

void opening_menu_handler(UI_Element *elem, bool dbl_click) {
	auto table = dynamic_cast<Data_View*>(elem);

	switch (table->hl_row) {
		case 0: // New workspace
		{
			Box *b = new Box();
			Workspace *ws = table->parent->parent;
			make_main_menu(*ws, *b);
			ws->add_box(b);
			ws->focus = ws->boxes.back();
			break;
		}
	}

	if (table->hl_row == 0)
		table->parent->visible = false;
}

void update_opening_menu(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	b.update_elements(view, input, inside, hovered, focussed);

	auto title = dynamic_cast<Label*>(b.ui[0]);
	float y = b.border;
	y += center_align_title(title, b, view.scale, y);

	auto table = dynamic_cast<Data_View*>(b.ui[1]);
	table->pos.x = b.border;
	table->pos.y = y;
	table->pos.w = b.box.w - b.border * 2;
	table->pos.h = b.box.h - y - b.border;

	b.post_update_elements(view, input, inside, hovered, focussed);
}

void make_opening_menu(Workspace& ws, Box& b) {
	Label *title = new Label();
	title->font = ws.default_font;
	title->text = "Muscles";

	Data_View *table = new Data_View();
	table->font = ws.default_font;
	table->hl_color = ws.hl_color;
	table->default_color = ws.back_color;

	Column menu_column = { String, 0, 1, 0, "" };
	table->data.init(&menu_column, 1, 2);
	table->data.columns[0][0] = (void*)"New";
	table->data.columns[0][1] = (void*)"Open Workspace";
	table->action = opening_menu_handler;

	b.ui.push_back(dynamic_cast<UI_Element*>(title));
	b.ui.push_back(dynamic_cast<UI_Element*>(table));

	b.update_handler = update_opening_menu;
	b.back = ws.back_color;

	b.box = {-80, -50, 160, 100};
	b.visible = true;
}
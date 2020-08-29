#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void update_source_menu(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	Source_Menu *ui = (Source_Menu*)b.markup;

	float y = b.border;

	if (ui->title) {
		y += center_align_title(ui->title, b, view.scale, y);
	}

	if (ui->search && ui->scroll) {
		if (focussed && ui->menu && ui->search->text_changed)
			ui->scroll->position = 0;

		float x2 = b.box.w - ui->scroll->padding;
		float x1 = x2 - ui->search->max_width;
		if (x1 < ui->scroll->padding) x1 = ui->scroll->padding;

		ui->search->pos.x = x1;
		ui->search->pos.y = y;
		ui->search->pos.w = x2 - x1;

		float hack = 4;
		y += ui->search->pos.h + hack;
	}

	float end_x = b.box.w - b.border;

	if (ui->scroll) {
		ui->scroll->pos.w = ui->scroll->breadth;
		ui->scroll->pos.x = b.box.w - ui->scroll->pos.w - ui->scroll->padding;
		ui->scroll->pos.y = y;
		ui->scroll->pos.h = b.box.h - b.border - y;

		end_x = ui->scroll->pos.x;
	}

	if (ui->path && ui->div) {
		float path_h = ui->path->font->render.text_height() * 1.4f / view.scale;

		ui->div->pos.y = b.box.h - 3*b.border - path_h;
		ui->div->pos.x = 5;
		ui->div->pos.w = b.box.w - 2 * ui->div->pos.x;

		ui->path->pos.x = 2*b.border;
		ui->path->pos.y = ui->div->pos.y + 2*b.border;
		ui->path->pos.w = b.box.w - 4*b.border;
		ui->path->pos.h = path_h;
	}

	if (ui->menu) {
		ui->menu->pos.x = b.border;
		ui->menu->pos.y = y;
		ui->menu->pos.w = end_x - ui->menu->pos.x;

		float hack = 3;
		float end = ui->div ? ui->div->pos.y - hack : b.box.h;
		ui->menu->pos.h = end - b.border - y;

		if (ui->scroll)
			ui->scroll->pos.h = ui->menu->pos.h;
	}

	if (ui->cross) {
		ui->cross->pos.y = b.cross_size * 0.5;
		ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
		ui->cross->pos.w = b.cross_size;
		ui->cross->pos.h = b.cross_size;
	}

	if (ui->up && ui->menu && ui->search) {
		ui->up->pos = {
			ui->menu->pos.x,
			ui->search->pos.y,
			ui->up->width,
			ui->up->height
		};
	}

	b.post_update_elements(view, input, inside, hover, focussed);
}

void scale_search_bar(Source_Menu *ui, float new_scale) {
	float height = ui->search->font->render.text_height() * 1.5 / new_scale;
	ui->search->update_icon(IconGlass, height, new_scale);
}

Source_Menu *make_source_menu(Workspace& ws, Box& b, const char *title_str, void (*table_handler)(UI_Element*, bool)) {
	Source_Menu *ui = new Source_Menu();

	ui->cross = new Image();
	ui->cross->action = get_delete_box();
	ui->cross->img = ws.cross;

	ui->scroll = new Scroll();
	ui->scroll->back = ws.scroll_back;
	ui->scroll->default_color = ws.scroll_color;
	ui->scroll->hl_color = ws.scroll_hl_color;
	ui->scroll->sel_color = ws.scroll_sel_color;

	ui->title = new Label();
	ui->title->text = title_str;
	ui->title->font = ws.default_font;

	ui->menu = new Data_View();
	ui->menu->data = &ui->table;
	ui->menu->action = table_handler;
	ui->menu->default_color = ws.back_color;
	ui->menu->hl_color = ws.hl_color;
	ui->menu->font = ws.default_font;
	ui->menu->consume_box_scroll = true;
	ui->menu->vscroll = ui->scroll;
	ui->menu->vscroll->content = ui->menu;

	ui->search = new Edit_Box();
	ui->search->caret = ws.caret_color;
	ui->search->default_color = ws.dark_color;
	ui->search->sel_color = ws.inactive_outline_color;
	ui->search->font = ws.make_font(9, ws.text_color);
	ui->search->icon_color = ws.text_color;
	ui->search->icon_color.a = 0.7;
	ui->search->key_action = [](Edit_Box* edit, Input& input) {
		Data_View *menu = ((Source_Menu*)edit->parent->markup)->menu;
		menu->data->update_filter(edit->editor.text);
		menu->needs_redraw = true;
	};

	b.ui.push_back(ui->scroll);
	b.ui.push_back(ui->menu);
	b.ui.push_back(ui->search);
	b.ui.push_back(ui->title);
	b.ui.push_back(ui->cross);

	b.visible = true;
	b.markup = ui;
	b.update_handler = update_source_menu;
	b.back = ws.back_color;

	return ui;
}

void notify_main_box(Workspace *ws) {
	Box *main_box = ws->first_box_of_type(BoxMain);
	auto main_ui = (Main_Menu*)main_box->markup;
	main_ui->sources->sel_row = ws->sources.size() - 1;
	main_box->require_redraw();
}

void process_menu_handler(UI_Element *elem, bool dbl_click) {
	if (dbl_click)
		return;

	auto menu = dynamic_cast<Data_View*>(elem);
	if (menu->hl_row < 0)
		return;

	int idx = menu->data->index_from_filtered(menu->hl_row);
	char *process = (char*)menu->data->columns[1][idx];

	Workspace *ws = elem->parent->parent;
	ws->delete_box(elem->parent);

	char *name_ptr = nullptr;
	int pid = strtol(process, &name_ptr, 0);
	auto& sources = (std::vector<Source*>&)ws->sources;

	for (auto& s : sources) {
		if (s->type == SourceProcess && s->pid == pid)
			return;
	}

	Source *s = new Source();
	s->type = SourceProcess;
	s->pid = pid;
	s->name = name_ptr+1;
	s->refresh_span_rate = 1;

	sources.push_back(s);
	notify_main_box(ws);
}

void refresh_file_menu(Box& b, Point& cursor);

void file_menu_handler(UI_Element *elem, bool dbl_click) {
	if (dbl_click)
		return;

	auto menu = dynamic_cast<Data_View*>(elem);
	if (menu->hl_row < 0)
		return;

	auto ui = (Source_Menu*)menu->parent->markup;

	int idx = menu->data->index_from_filtered(menu->hl_row);
	auto file = (File_Entry*)menu->data->columns[1][idx];

	if (file->flags & 8) {
		ui->path->placeholder += file->name;
		ui->path->placeholder += get_folder_separator();

		Point p = {0, 0};
		refresh_file_menu(*menu->parent, p);

		ui->scroll->position = 0;
		ui->search->clear();
		menu->data->clear_filter();

		return;
	}

	Workspace *ws = elem->parent->parent;
	ws->delete_box(elem->parent);

	std::string path = ui->path->placeholder;
	path += get_folder_separator();
	path += file->name;

	auto& sources = (std::vector<Source*>&)ws->sources;

	for (auto& s : sources) {
		if (s->type == SourceFile && path == (const char*)s->identifier)
			return;
	}

	Source *s = new Source();
	s->type = SourceFile;
	s->identifier = (void*)get_default_arena()->alloc_string((char*)path.c_str());
	s->name = file->name;
	s->refresh_span_rate = 1;

	sources.push_back(s);
	notify_main_box(ws);
}

void refresh_process_menu(Box& b, Point& cursor) {
	auto ui = (Source_Menu*)b.markup;
	if (ui->menu->pos.contains(cursor))
		return;

	std::vector<int> pids;
	get_process_id_list(pids);

	ui->menu->data->resize(pids.size());

	int n_procs = get_process_names(pids, ui->icon_map, ui->menu->data->columns[0], ui->menu->data->columns[1], 64);
	ui->menu->data->resize(n_procs);
}

void process_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Source_Menu*)b.markup;
	ui->cross->img = ws.cross;
	scale_search_bar(ui, new_scale);

	ui->menu->data->release();
	ui->icon_map.release();
}

void refresh_file_menu(Box& b, Point& cursor) {
	auto ui = (Source_Menu*)b.markup;
	if (ui->menu->pos.contains(cursor))
		return;

	auto& files = (std::vector<File_Entry*>&)ui->menu->data->columns[1];
	enumerate_files((char*)ui->path->placeholder.c_str(), files, *get_default_arena());

	if (files.size() > 0) {
		ui->menu->data->columns[0].resize(files.size(), nullptr);

		for (int i = 0; i < files.size(); i++)
			ui->menu->data->columns[0][i] = (files[i]->flags & 8) ? ui->folder_icon : nullptr;
	}
	else
		ui->menu->data->clear_data();
}

void file_up_handler(UI_Element *elem, bool dbl_click) {
	auto ui = (Source_Menu*)elem->parent->markup;

	char sep = get_folder_separator()[0];
	int last = ui->path->placeholder.size() - 1;

	while (last >= 0 && ui->path->placeholder[last] == sep)
		last--;

	size_t pos = last > 0 ? ui->path->placeholder.find_last_of(sep, last) : std::string::npos;
	if (pos != std::string::npos) {
		ui->path->placeholder.erase(pos + 1);
		ui->scroll->position = 0;
		ui->search->clear();
		ui->menu->data->clear_filter();
	}

	Point p = {0};
	refresh_file_menu(*elem->parent, p);
	ui->menu->needs_redraw = true;
}

void file_path_handler(Edit_Box *edit, Input& input) {
	if (input.enter != 1 || !edit->editor.text.size())
		return;

	std::string str = edit->editor.text;
	char sep = get_folder_separator()[0];
	if (str.back() == sep)
		str.pop_back();

	if (!is_folder(str.c_str()))
		return;

	str += sep;
	edit->placeholder = str;
	edit->parent->active_edit = nullptr;
	edit->clear();

	auto ui = (Source_Menu*)edit->parent->markup;
	ui->scroll->position = 0;
	ui->search->clear();
	ui->menu->data->clear_filter();

	Point p = {0};
	refresh_file_menu(*edit->parent, p);
	ui->menu->needs_redraw = true;
}

void file_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Source_Menu*)b.markup;

	sdl_destroy_texture(&ui->up->icon);
	sdl_destroy_texture(&ui->folder_icon);

	float h = ui->up->active_theme.font->render.text_height();
	ui->up->icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	h = ui->menu->font->render.text_height();
	ui->folder_icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	int n_rows = ui->menu->data->row_count();
	auto& icons = (std::vector<Texture>&)ui->menu->data->columns[0];
	for (int i = 0; i < n_rows; i++) {
		if (icons[i])
			icons[i] = ui->folder_icon;
	}

	ui->cross->img = ws.cross;
	scale_search_bar(ui, new_scale);
}

void make_process_menu(Workspace& ws, Box& b) {
	auto ui = make_source_menu(ws, b, "Open Process", process_menu_handler);
	b.refresh_handler = refresh_process_menu;
	b.scale_change_handler = process_scale_change_handler;

	b.box = { -100, -100, 300, 200 };

	Column col[] = {
		{ColumnImage, 0, 0.1, 0, 1.5, ""},
		{ColumnString, 64, 0.9, 0, 0, ""}
	};
	ui->menu->data->init(col, nullptr, 2, 0);
}

void make_file_menu(Workspace& ws, Box& b) {
	auto ui = make_source_menu(ws, b, "Open File", file_menu_handler);
	b.refresh_handler = refresh_file_menu;
	b.scale_change_handler = file_scale_change_handler;

	b.box = { -150, -150, 400, 300 };

	Column col[] = {
		{ColumnImage, 0, 0.1, 0, 1.5, ""},
		{ColumnFile, 0, 0.9, 0, 0, ""}
	};
	ui->menu->data->init(col, nullptr, 2, 1);

	float h = ui->menu->font->render.text_height();
	ui->folder_icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	ui->up = new Button();
	ui->up->text = "Up";
	ui->up->action = file_up_handler;
	ui->up->icon_right = true;

	float up_font_size = 10;
	Font *up_font = ws.make_font(up_font_size, ws.text_color);

	h = up_font->render.text_height();
	ui->up->icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	ui->up->active_theme = {
		ws.light_color,
		ws.light_color,
		ws.light_color,
		up_font
	};
	ui->up->inactive_theme = ui->up->active_theme;
	ui->up->update_size(ws.temp_scale);

	ui->div = new Divider();
	ui->div->default_color = {0.5, 0.6, 0.8, 1.0};
	ui->div->pos.h = 1.5;

	ui->path = new Edit_Box();
	ui->path->font = up_font;
	ui->path->default_color = ws.dark_color;
	ui->path->sel_color = ws.inactive_outline_color;
	ui->path->caret = ws.caret_color;
	ui->path->placeholder = get_root_folder();
	ui->path->key_action = file_path_handler;

	RGBA ph_color = ws.text_color;
	ph_color.a = 0.75;
	ui->path->ph_font = ws.make_font(up_font_size, ph_color);

	b.ui.push_back(ui->up);
	b.ui.push_back(ui->div);
	b.ui.push_back(ui->path);
}
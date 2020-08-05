#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void update_source_menu(Box& b, Camera& view, Input& input, Point& inside, bool hovered, bool focussed) {
	b.update_elements(view, input, inside, hovered, focussed);

	Source_Menu *ui = (Source_Menu*)b.markup;

	float y = b.border;

	if (ui->title) {
		y += center_align_title(ui->title, b, view.scale, y);
	}

	if (ui->search && ui->scroll) {
		if (focussed && ui->table && ui->search->update)
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

	if (ui->table) {
		ui->table->pos.x = b.border;
		ui->table->pos.y = y;
		ui->table->pos.w = end_x - ui->table->pos.x;

		float hack = 3;
		float end = ui->div ? ui->div->pos.y - hack : b.box.h;
		ui->table->pos.h = end - b.border - y;

		if (ui->scroll)
			ui->scroll->pos.h = ui->table->pos.h;
	}

	if (ui->cross) {
		ui->cross->pos.y = b.cross_size * 0.5;
		ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
		ui->cross->pos.w = b.cross_size;
		ui->cross->pos.h = b.cross_size;
	}

	if (ui->up && ui->table && ui->search) {
		ui->up->pos = {
			ui->table->pos.x,
			ui->search->pos.y,
			ui->up->width,
			ui->up->height
		};
	}

	b.post_update_elements(view, input, inside, hovered, focussed);
}

void scale_search_bar(Source_Menu *ui, float new_scale) {
	float height = ui->search->font->render.text_height() * 1.5 / new_scale;
	ui->search->update_icon(IconGlass, height, new_scale);
}

Source_Menu *make_source_menu(Workspace& ws, Box& b, const char *title_str, void (*table_handler)(UI_Element*, bool)) {
	Source_Menu *ui = new Source_Menu();

	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;

	ui->scroll = new Scroll();
	ui->scroll->back = ws.scroll_back;
	ui->scroll->default_color = ws.scroll_color;
	ui->scroll->hl_color = ws.scroll_hl_color;
	ui->scroll->sel_color = ws.scroll_sel_color;

	ui->title = new Label();
	ui->title->text = title_str;
	ui->title->font = ws.default_font;

	ui->table = new Data_View();
	ui->table->action = table_handler;
	ui->table->default_color = ws.back_color;
	ui->table->hl_color = ws.hl_color;
	ui->table->font = ws.default_font;
	ui->table->consume_box_scroll = true;
	ui->table->vscroll = ui->scroll;

	ui->search = new Edit_Box();
	ui->search->caret = ws.caret_color;
	ui->search->default_color = ws.dark_color;
	ui->search->font = ws.make_font(9, ws.text_color);
	ui->search->icon_color = ws.text_color;
	ui->search->icon_color.a = 0.7;
	ui->search->key_action = [](Edit_Box* edit, Input& input) {((Source_Menu*)edit->parent->markup)->table->data.update_filter(edit->line);};
	ui->search->action = [](UI_Element *elem, bool dbl_click) {elem->parent->active_edit = elem;};

	b.ui.push_back(ui->scroll);
	b.ui.push_back(ui->table);
	b.ui.push_back(ui->search);
	b.ui.push_back(ui->title);
	b.ui.push_back(ui->cross);

	b.visible = true;
	b.markup = ui;
	b.update_handler = update_source_menu;
	b.back = ws.back_color;

	return ui;
}

void process_menu_handler(UI_Element *elem, bool dbl_click) {
	if (dbl_click)
		return;

	auto table = dynamic_cast<Data_View*>(elem);
	if (table->hl_row < 0)
		return;

	int idx = table->data.index_from_filtered(table->hl_row);
	char *process = (char*)table->data.columns[1][idx];

	elem->parent->parent->delete_box(elem->parent);

	char *name_ptr = nullptr;
	int pid = strtol(process, &name_ptr, 0);
	auto& sources = (std::vector<Source*>&)elem->parent->parent->sources;

	for (auto& s : sources) {
		if (s->type == TypeProcess && s->pid == pid)
			return;
	}

	Source *s = new Source();
	s->type = TypeProcess;
	s->pid = pid;
	s->name = name_ptr+1;
	s->refresh_span_rate = 1;

	sources.push_back(s);

	auto main_ui = (Main_Menu*)elem->parent->parent->first_box_of_type(TypeMain)->markup;
	main_ui->table->sel_row = sources.size() - 1;
}

void refresh_file_menu(Box& b, Point& cursor);

void file_menu_handler(UI_Element *elem, bool dbl_click) {
	if (dbl_click)
		return;

	auto table = dynamic_cast<Data_View*>(elem);
	if (table->hl_row < 0)
		return;

	auto ui = (Source_Menu*)table->parent->markup;

	int idx = table->data.index_from_filtered(table->hl_row);
	auto file = (File_Entry*)table->data.columns[1][idx];

	if (file->flags & 8) {
		ui->path->placeholder += file->name;
		ui->path->placeholder += get_folder_separator();

		Point p = {0, 0};
		refresh_file_menu(*table->parent, p);

		ui->scroll->position = 0;
		ui->search->clear();
		table->data.clear_filter();

		return;
	}

	elem->parent->parent->delete_box(elem->parent);

	std::string path = ui->path->placeholder;
	path += get_folder_separator();
	path += file->name;

	auto& sources = (std::vector<Source*>&)elem->parent->parent->sources;

	for (auto& s : sources) {
		if (s->type == TypeFile && path == (const char*)s->identifier)
			return;
	}

	Source *s = new Source();
	s->type = TypeFile;
	s->identifier = (void*)get_arena()->alloc_string((char*)path.c_str());
	s->name = file->name;
	s->refresh_span_rate = 1;

	sources.push_back(s);

	auto main_ui = (Main_Menu*)elem->parent->parent->first_box_of_type(TypeMain)->markup;
	main_ui->table->sel_row = sources.size() - 1;
}

void refresh_process_menu(Box& b, Point& cursor) {
	auto ui = (Source_Menu*)b.markup;
	if (ui->table->pos.contains(cursor))
		return;

	std::vector<int> pids;
	get_process_id_list(pids);

	ui->table->data.release();
	ui->table->data.resize(pids.size());

	int n_procs = get_process_names(pids, (std::vector<Texture>&)ui->table->data.columns[0], (std::vector<char*>&)ui->table->data.columns[1], 64);
	ui->table->data.resize(n_procs);
}

void process_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Source_Menu*)b.markup;
	ui->cross->img = ws.cross;
	scale_search_bar(ui, new_scale);
}

void refresh_file_menu(Box& b, Point& cursor) {
	auto ui = (Source_Menu*)b.markup;
	if (ui->table->pos.contains(cursor))
		return;

	auto& files = (std::vector<File_Entry*>&)ui->table->data.columns[1];
	enumerate_files((char*)ui->path->placeholder.c_str(), files, *get_arena());

	if (files.size() > 0) {
		ui->table->data.columns[0].resize(files.size(), nullptr);

		for (int i = 0; i < files.size(); i++)
			ui->table->data.columns[0][i] = (files[i]->flags & 8) ? ui->folder_icon : nullptr;
	}
	else
		ui->table->data.clear_data();
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
		ui->table->data.clear_filter();
	}

	Point p = {0};
	refresh_file_menu(*elem->parent, p);
}

void file_path_handler(Edit_Box *edit, Input& input) {
	if (input.enter != 1 || !edit->line.size())
		return;

	std::string str = edit->line;
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
	ui->table->data.clear_filter();

	Point p = {0};
	refresh_file_menu(*edit->parent, p);
}

void file_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (Source_Menu*)b.markup;

	sdl_destroy_texture(&ui->up->icon);
	sdl_destroy_texture(&ui->folder_icon);

	float h = ui->up->active_theme.font->render.text_height();
	ui->up->icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	h = ui->table->font->render.text_height();
	ui->folder_icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	int n_rows = ui->table->data.row_count();
	auto& icons = (std::vector<Texture>&)ui->table->data.columns[0];
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
		{Tex, 0, 0.1, 0, 1.5, ""},
		{String, 64, 0.9, 0, 0, ""}
	};
	ui->table->data.init(col, 2, 0);
}

void make_file_menu(Workspace& ws, Box& b) {
	auto ui = make_source_menu(ws, b, "Open File", file_menu_handler);
	b.refresh_handler = refresh_file_menu;
	b.scale_change_handler = file_scale_change_handler;

	b.box = { -150, -150, 400, 300 };

	Column col[] = {
		{Tex, 0, 0.1, 0, 1.5, ""},
		{File, 0, 0.9, 0, 0, ""}
	};
	ui->table->data.init(col, 2, 1);

	float h = ui->table->font->render.text_height();
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
	ui->path->caret = ws.caret_color;
	ui->path->placeholder = get_root_folder();
	ui->path->key_action = file_path_handler;
	ui->path->action = [](UI_Element *elem, bool dbl_click) {elem->parent->active_edit = elem;};

	RGBA ph_color = ws.text_color;
	ph_color.a = 0.75;
	ui->path->ph_font = ws.make_font(up_font_size, ph_color);

	b.ui.push_back(ui->up);
	b.ui.push_back(ui->div);
	b.ui.push_back(ui->path);
}
#include "../muscles.h"
#include "../ui.h"
#include "../io.h"
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
		ui->search->pos.h = ui->search->font->render.text_height() * 1.5 / view.scale;

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
		float h = ui->path->font->render.text_height();

		float hack = 4;
		ui->div->pos.y = b.box.h - b.border - h / view.scale - hack;
		ui->div->pos.x = 5;
		ui->div->pos.w = b.box.w - 2 * ui->div->pos.x;

		ui->path->pos.x = b.border * 2;
		ui->path->pos.y = ui->div->pos.y + hack;
		ui->path->pos.w = ui->div->pos.w;
		ui->path->update_position(view);
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

Source_Menu *make_source_menu(Workspace& ws, Box& b, const char *title_str, void (*table_handler)(UI_Element*, bool)) {
	Source_Menu *ui = new Source_Menu();

	ui->cross = new Image();
	ui->cross->action = [](UI_Element *elem, bool dbl_click) {elem->parent->parent->delete_box(elem->parent);};
	ui->cross->img = ws.cross;

	ui->scroll = new Scroll();
	ui->scroll->back = {0, 0.1, 0.4, 1.0};
	ui->scroll->default_color = {0.4, 0.45, 0.55, 1.0};
	ui->scroll->hl_color = {0.6, 0.63, 0.7, 1.0};
	ui->scroll->sel_color = {0.8, 0.8, 0.8, 1.0};

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
	ui->search->caret = {0.9, 0.9, 0.9, 1.0};
	ui->search->default_color = ws.dark_color;
	ui->search->font = ws.make_font(9, ws.text_color);
	ui->search->table = &ui->table->data;

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

	int idx = table->data.visible == -1 ? table->hl_row : *std::next(table->data.list.begin(), table->hl_row);
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

	sources.push_back(s);

	auto main_ui = (Main_Menu*)elem->parent->parent->first_box_of_type(TypeMain)->markup;
	main_ui->table->sel_row = sources.size() - 1;
}

void refresh_file_menu(Box& b);

void file_menu_handler(UI_Element *elem, bool dbl_click) {
	if (dbl_click)
		return;

	auto table = dynamic_cast<Data_View*>(elem);
	if (table->hl_row < 0)
		return;

	auto ui = (Source_Menu*)table->parent->markup;

	int idx = table->data.visible == -1 ? table->hl_row : *std::next(table->data.list.begin(), table->hl_row);
	auto file = (File_Entry*)table->data.columns[1][idx];

	if (file->flags & 8) {
		ui->path->text += file->name;
		ui->path->text += get_folder_separator();
		refresh_file_menu(*table->parent);

		ui->scroll->position = 0;
		ui->search->clear();
		table->data.clear_filter();

		return;
	}

	elem->parent->parent->delete_box(elem->parent);

	std::string path = ui->path->text;
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

	sources.push_back(s);

	auto main_ui = (Main_Menu*)elem->parent->parent->first_box_of_type(TypeMain)->markup;
	main_ui->table->sel_row = sources.size() - 1;
}

void refresh_process_menu(Box& b) {
	auto ui = (Source_Menu*)b.markup;

	std::vector<int> pids;
	get_process_id_list(pids);

	ui->table->data.release();
	ui->table->data.resize(pids.size());

	int n_procs = get_process_names(pids, (std::vector<Texture>&)ui->table->data.columns[0], (std::vector<char*>&)ui->table->data.columns[1], 64);
	ui->table->data.resize(n_procs);
}

void process_scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	((Source_Menu*)b.markup)->cross->img = ws.cross;
}

void refresh_file_menu(Box& b) {
	auto ui = (Source_Menu*)b.markup;

	auto& files = (std::vector<File_Entry*>&)ui->table->data.columns[1];
	enumerate_files((char*)ui->path->text.c_str(), files, *get_arena());

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
	int last = ui->path->text.size() - 1;

	while (last >= 0 && ui->path->text[last] == sep)
		last--;

	size_t pos = last > 0 ? ui->path->text.find_last_of(sep, last) : std::string::npos;
	if (pos != std::string::npos) {
		ui->path->text.erase(pos + 1);
		ui->scroll->position = 0;
		ui->search->clear();
		ui->table->data.clear_filter();
	}

	refresh_file_menu(*elem->parent);
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
}

void make_process_menu(Workspace& ws, Box& b) {
	auto ui = make_source_menu(ws, b, "Open Process", process_menu_handler);
	b.refresh_handler = refresh_process_menu;
	b.scale_change_handler = process_scale_change_handler;

	b.box = { -100, -100, 300, 200 };

	Column col[] = {
		{Tex, 0, 0.1, 1.5, ""},
		{String, 64, 0.9, 0, ""}
	};
	ui->table->data.init(col, 2, 0);
}

void make_file_menu(Workspace& ws, Box& b) {
	auto ui = make_source_menu(ws, b, "Open File", file_menu_handler);
	b.refresh_handler = refresh_file_menu;
	b.scale_change_handler = file_scale_change_handler;

	b.box = { -150, -150, 400, 300 };

	Column col[] = {
		{Tex, 0, 0.1, 1.5, ""},
		{File, 0, 0.9, 0, ""}
	};
	ui->table->data.init(col, 2, 1);

	float h = ui->table->font->render.text_height();
	ui->folder_icon = make_folder_icon(ui->folder_dark, ui->folder_light, h, h);

	ui->up = new Button();
	ui->up->text = "Up";
	ui->up->action = file_up_handler;
	ui->up->icon_right = true;

	Font *up_font = ws.make_font(10, ws.text_color);

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

	ui->path = new Label();
	ui->path->font = up_font;
	ui->path->text = get_root_folder();

	b.ui.push_back(ui->up);
	b.ui.push_back(ui->div);
	b.ui.push_back(ui->path);
}
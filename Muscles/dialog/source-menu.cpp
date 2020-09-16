#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void Source_Menu::update_ui(Camera& view) {
	float title_h = title.font->render.text_height() / view.scale;
	title.outer_box = {
		border,
		-border,
		box.w - 2*border,
		title_h + 4*border
	};

	if (search.text_changed)
		scroll.position = 0;

	float x2 = box.w - scroll.padding;
	float x1 = x2 - search.max_width;
	if (x1 < scroll.padding) x1 = scroll.padding;

	search.pos.x = x1;
	search.pos.y = title.outer_box.y + title.outer_box.h;
	search.pos.w = x2 - x1;

	float y = search.pos.y + search.pos.h + border;

	float end_x = box.w - border;

	scroll.pos.w = scroll.breadth;
	scroll.pos.x = box.w - scroll.pos.w - scroll.padding;
	scroll.pos.y = y;
	scroll.pos.h = box.h - border - y;

	end_x = scroll.pos.x;

	if (path.visible && div.visible) {
		float path_h = path.font->render.text_height() * 1.4f / view.scale;

		div.pos.y = box.h - 3*border - path_h;
		div.pos.x = 5;
		div.pos.w = box.w - 2 * div.pos.x;

		path.pos.x = 2*border;
		path.pos.y = div.pos.y + 2*border;
		path.pos.w = box.w - 4*border;
		path.pos.h = path_h;
	}

	menu.pos.x = border;
	menu.pos.y = y;
	menu.pos.w = end_x - menu.pos.x;

	float end = div.visible ? div.pos.y - 3 : box.h;
	menu.pos.h = end - border - y;

	scroll.pos.h = menu.pos.h;

	reposition_box_buttons(cross, maxm, box.w, cross_size);

	if (up.visible) {
		up.pos = {
			menu.pos.x,
			search.pos.y,
			up.width,
			up.height
		};
	}
}

static void process_menu_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	if (dbl_click)
		return;

	auto menu = dynamic_cast<Data_View*>(elem);
	if (menu->hl_row < 0)
		return;

	// Copy the information we need
	int idx = menu->data->index_from_filtered(menu->hl_row);

	s64 pid_long = (s64)menu->data->columns[1][idx];
	int pid = pid_long;

	std::string name = (char*)menu->data->columns[2][idx];

	auto box = (Source_Menu*)elem->parent;
	Box *caller = box->caller;
	auto callback = box->open_process_handler;

	// THEN delete this box
	Workspace *ws = box->parent;
	ws->delete_box(box);

	if (callback && caller)
		callback(caller, pid, name);
}

static void file_menu_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	if (dbl_click)
		return;

	auto menu = dynamic_cast<Data_View*>(elem);
	if (menu->hl_row < 0)
		return;

	auto ui = dynamic_cast<Source_Menu*>(elem->parent);

	int idx = menu->data->index_from_filtered(menu->hl_row);
	auto file = (File_Entry*)menu->data->columns[1][idx];

	if (file->flags & 8) {
		ui->path.placeholder += file->name;
		ui->path.placeholder += get_folder_separator();

		ui->refresh(nullptr);

		ui->scroll.position = 0;
		ui->search.editor.clear();
		menu->data->clear_filter();
		menu->needs_redraw = true;

		return;
	}

	std::string path = ui->path.placeholder;
	path += get_folder_separator();
	path += file->name;

	auto callback = ui->open_file_handler;
	Box *caller = ui->caller;

	Workspace *ws = elem->parent->parent;
	ws->delete_box(elem->parent);

	if (callback && caller)
		callback(caller, path, file);
}

void file_up_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto ui = dynamic_cast<Source_Menu*>(elem->parent);

	char sep = get_folder_separator()[0];
	int last = ui->path.placeholder.size() - 1;

	while (last >= 0 && ui->path.placeholder[last] == sep)
		last--;

	size_t pos = last > 0 ? ui->path.placeholder.find_last_of(sep, last) : std::string::npos;
	if (pos != std::string::npos) {
		ui->path.placeholder.erase(pos + 1);
		ui->scroll.position = 0;
		ui->search.editor.clear();
		ui->menu.data->clear_filter();
	}

	ui->refresh(nullptr);
	ui->menu.needs_redraw = true;
}

void file_path_handler(Edit_Box *edit, Input& input) {
	if (input.enter != 1 || !edit->editor.text.size())
		return;

	auto ui = dynamic_cast<Source_Menu*>(edit->parent);
	std::string str = edit->editor.text;

	char sep = get_folder_separator()[0];
	while (str.back() == sep)
		str.pop_back();

	if (!is_folder(str.c_str())) {
		auto callback = ui->open_file_handler;
		Box *caller = ui->caller;

		Workspace *ws = ui->parent;
		ws->delete_box(ui);

		if (callback && caller) {
			auto last = str.find_last_of(sep);
			int off = last == std::string::npos ? 0 : last + 1;
			int len = str.size() - off;

			if (len < 111) {
				File_Entry file = {0};
				strcpy(file.name, str.c_str() + off);
				callback(caller, str, nullptr);
			}
		}

		return;
	}

	str += sep;
	edit->placeholder = str;
	edit->parent->active_edit = nullptr;
	edit->editor.clear();

	ui->scroll.position = 0;
	ui->search.editor.clear();
	ui->menu.data->clear_filter();

	ui->refresh(nullptr);
	ui->menu.needs_redraw = true;
}

void Source_Menu::refresh(Point *cursor) {
	if (cursor && menu.pos.contains(*cursor))
		return;

	if (menu_type == MenuProcess) {
		std::vector<s64> pids;
		get_process_id_list(pids);

		menu.data->resize(pids.size());

		int n_procs = get_process_names(&pids, icon_map, menu.data->columns[0], menu.data->columns[1], menu.data->columns[2], 64);
		menu.data->resize(n_procs);
	}
	else {
		auto& files = (std::vector<File_Entry*>&)menu.data->columns[1];
		enumerate_files((char*)path.placeholder.c_str(), files, get_default_arena());

		if (files.size() > 0) {
			menu.data->columns[0].resize(files.size(), nullptr);

			for (int i = 0; i < files.size(); i++)
				menu.data->columns[0][i] = (files[i]->flags & 8) ? folder_icon : nullptr;
		}
		else
			menu.data->clear_data();
	}
}

void Source_Menu::handle_zoom(Workspace& ws, float new_scale) {
	if (menu_type == MenuFile) {
		sdl_destroy_texture(&up.icon);
		sdl_destroy_texture(&folder_icon);

		float h = up.active_theme.font->render.text_height();
		up.icon = make_folder_icon(folder_dark, folder_light, h, h);

		h = menu.font->render.text_height();
		folder_icon = make_folder_icon(folder_dark, folder_light, h, h);

		int n_rows = menu.data->row_count();
		auto& icons = (std::vector<Texture>&)menu.data->columns[0];
		for (int i = 0; i < n_rows; i++) {
			if (icons[i])
				icons[i] = folder_icon;
		}
	}

	cross.img = ws.cross;
	maxm.img = ws.maxm;

	float height = search.font->render.text_height() * 1.5 / new_scale;
	search.update_icon(IconGlass, height, new_scale);
}

Source_Menu::Source_Menu(Workspace& ws, MenuType mtype)
	: menu_type(mtype)
{
	up.visible = false;
	div.visible = false;
	path.visible = false;

	float scale = get_default_camera().scale;

	cross.action = get_delete_box();
	cross.img = ws.cross;

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;

	scroll.back_color = ws.scroll_back;
	scroll.default_color = ws.scroll_color;
	scroll.hl_color = ws.scroll_hl_color;
	scroll.sel_color = ws.scroll_sel_color;

	title.font = ws.default_font;

	menu.data = &table;
	menu.default_color = ws.back_color;
	menu.hl_color = ws.hl_color;
	menu.sel_color = ws.hl_color;
	menu.font = ws.default_font;
	menu.vscroll = &scroll;
	menu.vscroll->content = &menu;

	search.caret = ws.caret_color;
	search.default_color = ws.dark_color;
	search.sel_color = ws.inactive_outline_color;
	search.font = ws.make_font(9, ws.text_color, scale);
	search.icon_color = ws.text_color;
	search.icon_color.a = 0.7;
	search.key_action = [](Edit_Box* edit, Input& input) {
		Data_View *menu = &dynamic_cast<Source_Menu*>(edit->parent)->menu;
		menu->data->update_filter(edit->editor.text);
		menu->needs_redraw = true;
	};

	ui.push_back(&scroll);
	ui.push_back(&menu);
	ui.push_back(&search);
	ui.push_back(&title);
	ui.push_back(&cross);
	ui.push_back(&maxm);

	if (menu_type == MenuProcess) {
		title.text = "Open Process";
		menu.action = process_menu_handler;

		Column col[] = {
			{ColumnImage, 0, 0.1, 0, 1.5, ""},
			{ColumnDec, 0, 0.2, 0, 3, ""},
			{ColumnString, 64, 0.7, 0, 0, ""}
		};
		menu.data->init(col, nullptr, 3, 0);

		initial_width = 300;
		initial_height = 200;
	}
	else {
		title.text = "Open File";
		menu.action = file_menu_handler;

		Column col[] = {
			{ColumnImage, 0, 0.1, 0, 1.5, ""},
			{ColumnFile, 0, 0.9, 0, 0, ""}
		};
		menu.data->init(col, nullptr, 2, 1);

		float h = menu.font->render.text_height();
		folder_icon = make_folder_icon(folder_dark, folder_light, h, h);

		up.visible = true;
		up.text = "Up";
		up.action = file_up_handler;
		up.icon_right = true;

		float up_font_size = 10;
		Font *up_font = ws.make_font(up_font_size, ws.text_color, scale);

		h = up_font->render.text_height();
		up.icon = make_folder_icon(folder_dark, folder_light, h, h);

		up.active_theme = {
			ws.light_color,
			ws.light_color,
			ws.light_color,
			up_font
		};
		up.inactive_theme = up.active_theme;
		up.update_size(scale);

		div.visible = true;
		div.default_color = {0.5, 0.6, 0.8, 1.0};
		div.pos.h = 1.5;

		path.visible = true;
		path.font = up_font;
		path.default_color = ws.dark_color;
		path.sel_color = ws.inactive_outline_color;
		path.caret = ws.caret_color;
		path.placeholder = get_root_folder();
		path.key_action = file_path_handler;

		RGBA ph_color = ws.text_color;
		ph_color.a = 0.75;
		path.ph_font = ws.make_font(up_font_size, ph_color, scale);

		ui.push_back(&up);
		ui.push_back(&div);
		ui.push_back(&path);

		initial_width = 400;
		initial_height = 300;
	}

	back = ws.back_color;
	edge_color = ws.dark_color;
	expungable = true;
}
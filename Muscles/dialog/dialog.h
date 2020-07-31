#pragma once

struct Main_Menu {
	Drop_Down *sources = nullptr;
	Drop_Down *view = nullptr;
	Data_View *table = nullptr;
	Button *button = nullptr;

	Texture process_icon = nullptr;
	RGBA process_back = {};
	RGBA process_outline = {};

	Texture file_icon = nullptr;
	RGBA file_back = {};
	RGBA file_fold = {};
	RGBA file_line = {};
};

struct Source_Menu {
	Data_View *table = nullptr;
	Image *cross = nullptr;
	Label *title = nullptr;
	Scroll *scroll = nullptr;
	Edit_Box *search = nullptr;

	Button *up = nullptr;
	Divider *div = nullptr;
	Edit_Box *path = nullptr;

	RGBA folder_dark = {0.65, 0.64, 0.15, 1.0};
	RGBA folder_light = {0.88, 0.84, 0.26, 1.0};

	Texture folder_icon = nullptr;
};

struct View_Source {
	Image *cross = nullptr;
	Label *title = nullptr;
	Divider *div = nullptr;
	Label *reg_title = nullptr;
	Data_View *reg_table = nullptr;
	Scroll *reg_scroll = nullptr;
	Scroll *reg_lat_scroll = nullptr;
	Label *hex_title = nullptr;
	Hex_View *hex = nullptr;
	Scroll *hex_scroll = nullptr;
	Label *reg_name = nullptr;
	Label *size_label = nullptr;
	Edit_Box *goto_box = nullptr;

	Source *source = nullptr;
	std::vector<u64> region_list;
	bool multiple_regions = false;
};

void make_file_menu(Workspace& ws, Box& b);
void make_process_menu(Workspace& ws, Box& b);
void make_view_source_menu(Workspace& ws, Source *s, Box& b);
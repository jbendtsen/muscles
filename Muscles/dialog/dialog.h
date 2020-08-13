#pragma once

struct Main_Menu {
	Drop_Down *sources = nullptr;
	Drop_Down *edit = nullptr;
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
	Edit_Box *reg_search = nullptr;
	Label *hex_title = nullptr;
	Hex_View *hex = nullptr;
	Scroll *hex_scroll = nullptr;
	Label *reg_name = nullptr;
	Label *size_label = nullptr;
	Edit_Box *goto_box = nullptr;
	Checkbox *addr_box = nullptr;
	Checkbox *hex_box = nullptr;
	Checkbox *ascii_box = nullptr;

	Source *source = nullptr;
	std::vector<u64> region_list;
	u64 selected_region = 0;
	int goto_digits = 2;
	bool multiple_regions = false;
	bool needs_region_update = true;
};

struct Edit_Structs {
	Image *cross = nullptr;
	Label *title = nullptr;
	Text_Editor *edit = nullptr;
	Scroll *hscroll = nullptr;
	Scroll *vscroll = nullptr;
};

struct View_Object {
	Image *cross = nullptr;
	Edit_Box *title_edit = nullptr;
	Label *struct_label = nullptr;
	Label *source_label = nullptr;
	Label *addr_label = nullptr;
	Edit_Box *struct_edit = nullptr;
	Drop_Down *struct_dd = nullptr;
	Edit_Box *source_edit = nullptr;
	Drop_Down *source_dd = nullptr;
	Edit_Box *addr_edit = nullptr;
	Button *hide_meta = nullptr;
	Data_View *view = nullptr;
	Scroll *hscroll = nullptr;
	Scroll *vscroll = nullptr;
	Button *all_btn = nullptr;
	Button *sel_btn = nullptr;

	Button::Theme theme_on = {};
	Button::Theme theme_off = {};

	float meta_btn_width = 20;
	float meta_btn_height = 20;
	bool meta_hidden = false;

	bool all = true;
};

void make_file_menu(Workspace& ws, Box& b);
void make_process_menu(Workspace& ws, Box& b);
void make_view_source_menu(Workspace& ws, Source *s, Box& b);
void make_view_object(Workspace& ws, Box& b);
void open_edit_structs(Workspace& ws);
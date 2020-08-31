#pragma once

struct Main_Menu {
	Drop_Down sources_dd;
	Drop_Down edit_dd;
	Drop_Down view_dd;
	Data_View sources;
	Button button;

	Table table;

	Texture process_icon = nullptr;
	RGBA process_back = {};
	RGBA process_outline = {};

	Texture file_icon = nullptr;
	RGBA file_back = {};
	RGBA file_fold = {};
	RGBA file_line = {};
};

struct Source_Menu {
	Data_View menu;
	Image cross;
	Label title;
	Scroll scroll;
	Edit_Box search;

	Button up;
	Divider div;
	Edit_Box path;

	Table table;

	RGBA folder_dark = {0.65, 0.64, 0.15, 1.0};
	RGBA folder_light = {0.88, 0.84, 0.26, 1.0};

	Texture folder_icon = nullptr;
	Map icon_map;
};

struct View_Source {
	Image cross;
	Label title;
	Divider div;
	Label reg_title;
	Data_View reg_table;
	Scroll reg_scroll;
	Scroll reg_lat_scroll;
	Edit_Box reg_search;
	Label hex_title;
	Hex_View hex;
	Scroll hex_scroll;
	Label reg_name;
	Label size_label;
	Edit_Box goto_box;
	Checkbox addr_box;
	Checkbox hex_box;
	Checkbox ascii_box;
	Number_Edit columns;

	Table table;
	Source *source = nullptr;
	std::vector<u64> region_list;
	u64 selected_region = 0;
	int goto_digits = 2;
	bool multiple_regions = false;
	bool needs_region_update = true;
};

struct Edit_Structs {
	Image cross;
	Label title;
	Text_Editor edit;
	Scroll edit_hscroll;
	Scroll edit_vscroll;
	Checkbox show_cb;
	Divider div;
	Data_View output;
	Scroll out_hscroll;
	Scroll out_vscroll;

	Arena arena;
	Table table;

	String_Vector tokens;
	String_Vector name_vector;

	std::vector<Struct*> structs;
	std::vector<char*> struct_names;

	float min_width = 200;
	bool first_run = true;
};

struct View_Object {
	Image cross;
	Edit_Box title_edit;
	Label struct_label;
	Label source_label;
	Label addr_label;
	Edit_Box struct_edit;
	Drop_Down struct_dd;
	Edit_Box source_edit;
	Drop_Down source_dd;
	Edit_Box addr_edit;
	Button hide_meta;
	Data_View view;
	Scroll hscroll;
	Scroll vscroll;
	Button all_btn;
	Button sel_btn;

	Button::Theme theme_on = {};
	Button::Theme theme_off = {};

	Arena arena;
	Table table;

	Struct *record = nullptr;
	Source *source = nullptr;
	int span_idx = -1;

	float meta_btn_width = 20;
	float meta_btn_height = 20;
	bool meta_hidden = false;

	bool all = true;
};

struct View_Definitions {
	Image cross;
	Label title;
	Tabs tabs;
	Data_View view;
	Scroll vscroll;
	Scroll hscroll;

	Table types;
	Table enums;
	Table typedefs;
	Table constants;
	std::vector<Table*> tables;

	Arena arena;
};

void make_file_menu(Workspace& ws, Box& b);
void make_process_menu(Workspace& ws, Box& b);
void make_view_source_menu(Workspace& ws, Source *s, Box& b);
void make_view_object(Workspace& ws, Box& b);
void open_edit_structs(Workspace& ws);
void open_view_definitions(Workspace& ws);

void populate_object_table(View_Object *ui, std::vector<Struct*>& structs, String_Vector& name_vector);
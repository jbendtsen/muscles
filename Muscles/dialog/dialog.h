#pragma once

struct Opening_Menu : Box {
	static const BoxType box_type_meta = BoxOpening;
	Opening_Menu(Workspace& ws, MenuType mtype);

	void update_ui(Camera& view) override;

	Label title;
	Data_View menu;
	Table table;
};

struct Main_Menu : Box {
	static const BoxType box_type_meta = BoxMain;
	Main_Menu(Workspace& ws, MenuType mtype);

	void update_ui(Camera& view) override;
	void refresh(Point *cursor) override;
	void handle_zoom(Workspace& ws, float new_scale) override;

	Drop_Down sources_dd;
	Drop_Down edit_dd;
	Drop_Down view_dd;
	Drop_Down search_dd;
	Data_View sources_view;
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

struct Source_Menu : Box {
	static const BoxType box_type_meta = BoxOpenSource;
	Source_Menu(Workspace& ws, MenuType mtype);

	void update_ui(Camera& view) override;
	void refresh(Point *cursor) override;
	void handle_zoom(Workspace& ws, float new_scale) override;

	Box *caller = nullptr;
	void (*open_process_handler)(Box *caller, int pid, std::string& name) = nullptr;
	void (*open_file_handler)(Box *caller, std::string& path, File_Entry *file) = nullptr;

	Data_View menu;
	Image cross;
	Image maxm;
	Label title;
	Scroll scroll;
	Edit_Box search;

	Button up;
	Divider div;
	Edit_Box path;

	Table table;

	Texture folder_icon = nullptr;
	Map icon_map;
};

struct View_Source : Box {
	static const BoxType box_type_meta = BoxViewSource;
	View_Source(Workspace& ws, MenuType mtype);

	void update_ui(Camera& view) override;
	void refresh(Point *cursor) override;
	void handle_zoom(Workspace& ws, float new_scale) override;
	void on_close() override;

	void refresh_region_list(Point *cursor);
	void update_regions_table();

	void open_source(Source *s);

	Image cross;
	Image maxm;
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
	std::vector<u64> region_list;
	u64 selected_region = 0;
	int goto_digits = 2;
	bool needs_region_update = true;
};

struct Edit_Structs : Box {
	static const BoxType box_type_meta = BoxStructs;
	Edit_Structs(Workspace& ws, MenuType mtype);
	
	void update_ui(Camera& view) override;
	void handle_zoom(Workspace& ws, float new_scale) override;
	void wake_up() override;

	Image cross;
	Image maxm;
	Label title;
	Drop_Down file_menu;
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

	float flex_min_width = 200;
};

struct View_Object : Box {
	static const BoxType box_type_meta = BoxViewObject;
	View_Object(Workspace& ws, MenuType mtype);
	
	void update_ui(Camera& view) override;
	void refresh(Point *cursor) override;
	void handle_zoom(Workspace& ws, float new_scale) override;
	void on_close() override;

	float get_edit_height(float scale);
	void update_hide_meta_button(float scale);
	void select_view_type(bool all);

	Image cross;
	Image maxm;
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

	Table table;
	Map field_fmt;
	String_Vector field_vec;
	std::vector<Value_Format> format;

	Struct *record = nullptr;
	Source *source = nullptr;
	int span_idx = -1;

	float meta_btn_width = 20;
	float meta_btn_height = 20;
	bool meta_hidden = false;

	bool all = true;
};

struct View_Definitions : Box {
	static const BoxType box_type_meta = BoxDefinitions;
	View_Definitions(Workspace& ws, MenuType mtype);
	
	void update_ui(Camera& view) override;
	void handle_zoom(Workspace& ws, float new_scale) override;

	void update_tables(Workspace *ws = nullptr);

	Image cross;
	Image maxm;
	Label title;
	Tabs tabs;
	Data_View view;
	Scroll vscroll;
	Scroll hscroll;

	Table types;
	Table enums;
	Table constants;
	std::vector<Table*> tables;

	Arena arena;
};

struct Search_Menu : Box {
	static const BoxType box_type_meta = BoxSearch;
	Search_Menu(Workspace& ws, MenuType mtype);

	void update_ui(Camera& view) override;
	void refresh(Point *cursor) override;
	void handle_zoom(Workspace& ws, float new_scale) override;

	void update_reveal_button(float scale);

	Image cross;
	Image maxm;
	Label title;

	Label source_lbl;
	Edit_Box source_edit;
	Drop_Down source_dd;

	Label addr_lbl;
	Edit_Box start_addr_edit;
	Edit_Box end_addr_edit;

	Label type_lbl;
	Edit_Box type_edit;
	Drop_Down type_dd;

	Label method_lbl;
	Drop_Down method_dd;

	Label value_lbl;
	Edit_Box value1_edit;
	Edit_Box value2_edit;

	//Progress_Bar progress_bar;

	Button search_btn;
	Button cancel_btn;
	Button reveal_btn;

	Label results_lbl;
	Label results_count_lbl;

	Data_View results;
	Scroll vscroll;

	float reveal_btn_length = 20;
	bool params_revealed = true;

	Search search;

	Table table;
};

void populate_object_table(View_Object *ui, std::vector<Struct*>& structs, String_Vector& name_vector);

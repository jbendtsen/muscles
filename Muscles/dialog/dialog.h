#pragma once

struct Search_Menu;

template<class UI>
void populate_object_table(UI *ui, std::vector<Struct*>& structs, String_Vector& name_vector) {
	ui->object.data->clear_data();
	ui->object.data->arena->rewind();

	ui->record = nullptr;

	if (!ui->struct_edit.editor.text.size())
		return;

	const char *name_str = ui->struct_edit.editor.text.c_str();
	Struct *record = nullptr;
	for (auto& s : structs) {
		if (!s)
			continue;

		char *name = name_vector.at(s->name_idx);
		if (name && !strcmp(name, name_str)) {
			record = s;
			break;
		}
	}

	if (!is_struct_usable(record))
		return;

	ui->record = record;
	int n_rows = ui->record->fields.n_fields;
	ui->object.data->resize(n_rows);

	const bool is_search = std::is_same_v<UI, Search_Menu>;

	for (int i = 0; i < n_rows; i++) {
		ui->object.data->set_checkbox(0, i, false);

		Field& f = ui->record->fields.data[i];
		char *name = name_vector.at(f.field_name_idx);
		ui->object.data->columns[1][i] = name;

		if constexpr (is_search)
			ui->object.data->columns[2][i] = name_vector.at(f.type_name_idx);
	}

	ui->object.needs_redraw = true;
}

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
	void goto_address(u64 address);

	void open_source(Source *s);

	Source *get_source() const { return hex.source; }

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
	Data_View object;
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
	void on_close() override;

	void set_object_row_visibility(int col, int row);
	void update_reveal_button(float scale);

	bool prepare_object_params();
	bool prepare_value_param();

	Image cross;
	Image maxm;
	Label title;

	Label struct_lbl;
	Edit_Box struct_edit;
	Drop_Down struct_dd;

	Label source_lbl;
	Edit_Box source_edit;
	Drop_Down source_dd;

	Label addr_lbl;
	Edit_Box start_addr_edit;
	Edit_Box end_addr_edit;

	Label align_lbl;
	Edit_Box align_edit;

	Label type_lbl;
	Edit_Box type_edit;
	Drop_Down type_dd;

	Label method_lbl;
	Drop_Down method_dd;

	Label value_lbl;
	Edit_Box value1_edit;
	Edit_Box value2_edit;

	Label object_lbl;
	Data_View object;
	Scroll object_scroll;
	Divider object_div;

	Table object_table;

	//Progress_Bar progress_bar;

	Button search_btn;
	Button cancel_btn;
	Button reset_btn;
	Button reveal_btn;

	Label results_lbl;
	Label results_count_lbl;

	Table results_table;
	Data_View results;
	Scroll results_scroll;

	Font *label_font = nullptr;
	Font *dd_font = nullptr;
	Font *table_font = nullptr;

	const float min_revealed_height = 500;
	const float min_hidden_height = 250;

	float reveal_btn_length = 20;
	bool params_revealed = true;

	std::vector<char*> method_options = {
		(char*)"Equals",
		(char*)"Range"
	};

	Search_Parameter *params_pool = nullptr;

	Search search;
	Source *source = nullptr;
	Struct *record = nullptr;
};

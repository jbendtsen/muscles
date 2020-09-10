#pragma once

enum Element_Type {
	ElemNone = 0,
	ElemImage,
	ElemLabel,
	ElemButton,
	ElemDivider,
	ElemDataView,
	ElemScroll,
	ElemEditBox,
	ElemDropDown,
	ElemCheckbox,
	ElemHexView,
	ElemTextEditor,
	ElemTabs,
	ElemNumEdit
};

struct UI_Element;
struct Scroll;
struct Box;

struct Editor {
	Editor(UI_Element *elem) {
		parent = elem;
	}

	bool multiline = true;
	bool use_clipboard = true;
	bool numeric_only = false;

	int last_tick = 0;
	int click_run = 0;
	int click_window = 30;

	bool selected = false;
	int tab_width = 4;
	UI_Element *parent = nullptr;

	struct Cursor {
		int cursor;
		int line;
		int column;
		int target_column;
	}
	primary = {0},
	secondary = {0};

	std::string text;
	int columns = 0;
	int lines = 0;

	Render_Clip clip = {0};
	Font *font = nullptr;

	float digit_w = 0;
	float font_h = 0;

	float x_offset = 0;
	float y_offset = 0;
	float x_scroll_delta = 0;
	float y_scroll_delta = 0;

	int vis_lines = 0;
	int vis_columns = 0;

	float cursor_width = 1;
	float border = 4;

	void clear();
	void measure_text();
	void set_text_from_buffer(std::pair<int, std::unique_ptr<u8[]>>&& buffer);

	void erase(Cursor& cursor, bool is_back);
	void set_cursor(Cursor& cursor, int cur);
	void set_line(Cursor& cursor, int line_idx);
	void set_column(Cursor& cursor, int col_idx);

	int handle_input(Input& input);
	void update_cursor(float x, float y, Font *font, float scale, int ticks, bool click);

	void refresh(Render_Clip& clip, Font *font, float scroll_x = -1.0, float scroll_y = -1.0);
	void apply_scroll(Scroll *hscroll = nullptr, Scroll *vscroll = nullptr);

	void draw_selection_box(Renderer renderer, RGBA& color);
	void draw_cursor(Renderer renderer, RGBA& color, Editor::Cursor& cursor, Point& pos, Render_Clip& cur_clip, float scale);
};

struct UI_Element {
	bool visible = true;
	bool active = true;
	bool use_sf_cache = false;
	bool use_default_cursor = false;

	enum Element_Type elem_type;
	enum CursorType cursor_type = CursorDefault;

	Box *parent = nullptr;

	Rect pos = {};
	int old_width = 0;
	int old_height = 0;

	Texture tex_cache = nullptr;
	int reify_timer = 0;
	int hw_draw_timeout = 15;
	bool needs_redraw = true;

	int ticks = 0;

	RGBA default_color = {};
	RGBA inactive_color = {};
	RGBA hl_color = {};
	RGBA sel_color = {};

	Font *font = nullptr;

	void (*action)(UI_Element*, bool) = nullptr;

	void draw(Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed);
	void set_active(bool state);

	virtual void update(Camera& view, Rect_Int& back) {}
	virtual void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {}
	virtual void post_draw(Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {}

	virtual void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {}
	virtual void key_handler(Camera& view, Input& input) {}

	virtual void scroll_handler(Camera& view, Input& input, Point& inside) {}
	virtual bool highlight(Camera& view, Point& inside) { return false; }
	virtual void deselect() {}
	virtual bool disengage(Input& input, bool try_select) { return false; }
	virtual void release() {}

	UI_Element() = default;
	UI_Element(Element_Type t, CursorType ct = CursorDefault) : elem_type(t), cursor_type(ct) {}

	virtual ~UI_Element() {
		sdl_destroy_texture(&tex_cache);
	}
};

void (*get_delete_box(void))(UI_Element*, bool);
void (*get_maximize_box(void))(UI_Element*, bool);
void (*get_text_editor_action(void))(UI_Element*, bool);
void (*get_edit_box_action(void))(UI_Element*, bool);
void (*get_number_edit_action(void))(UI_Element*, bool);

struct Scroll;

struct Button : UI_Element {
	Button() : UI_Element(ElemButton) {}
	~Button() {
		sdl_destroy_texture(&icon);
	}

	struct Theme {
		RGBA back;
		RGBA hover;
		RGBA held;
		Font *font;
	};
	Theme active_theme = {};
	Theme inactive_theme = {};

	Texture icon = nullptr;
	float icon_right = false;

	float width = 0;
	float height = 0;

	float padding = 1;
	float x_offset = 0.25;
	float y_offset = -0.2;

	bool held = false;
	std::string text;

	void update_size(float scale);
	
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Checkbox : UI_Element {
	Checkbox() : UI_Element(ElemCheckbox) {}

	bool checked = false;
	std::string text;

	float border_frac = 0.2;
	float text_off_x = 0.3;
	float text_off_y = -0.15;

	void toggle();
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Data_View : UI_Element {
	Data_View() : UI_Element(ElemDataView) {
		use_sf_cache = true;
	}
	~Data_View() {
		sdl_destroy_texture(&icon_plus);
		sdl_destroy_texture(&icon_minus);
	}

	RGBA back_color = {};

	Table *data = nullptr;
	std::vector<Table*> *tables = nullptr;

	struct Branch {
		int row_idx;
		int length;
		int name_idx;
		bool closed;
	};
	String_Vector branch_name_vector;
	std::vector<Branch> branches;
	std::vector<int> tree;
	Texture icon_plus = nullptr;
	Texture icon_minus = nullptr;

	Scroll *hscroll = nullptr;
	Scroll *vscroll = nullptr;

	int hl_row = -1;
	int hl_col = -1;
	int sel_row = -1;
	int condition_col = -1;

	float item_height = 0;
	float header_height = 25;
	float padding = 2;
	float column_spacing = 0.2;
	float scroll_total = 0.2;

	bool show_column_names = false;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT | CLIP_BOTTOM,
		0, 0, 0, 0
	};

	void update_tree(std::vector<Branch> *new_branches = nullptr);
	int get_table_index(int view_idx);

	float column_width(float total_width, float min_width, float font_height, float scale, int idx);
	void draw_item_backing(Renderer renderer, RGBA& color, Rect_Int& back, float scale, int idx);

	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;

	void scroll_handler(Camera& view, Input& input, Point& inside) override;
	bool highlight(Camera& view, Point& inside) override;
	void deselect() override;
	void release() override;
};

struct Divider : UI_Element {
	Divider() : UI_Element(ElemDivider) {}
	~Divider() {
		sdl_destroy_texture(&icon_default);
		sdl_destroy_texture(&icon_hl);
	}

	bool vertical = false;
	bool moveable = false;

	bool held = false;
	float minimum = 0;
	float maximum = 0;
	float position = 0;
	float hold_pos = 0;

	float breadth = 0;
	float padding = 8;

	Texture icon_default = nullptr;
	Texture icon_hl = nullptr;
	Texture icon = nullptr;
	int icon_w = 0;
	int icon_h = 0;

	void make_icon(float scale);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Drop_Down : UI_Element {
	Drop_Down() : UI_Element(ElemDropDown) {}

	bool dropped = false;
	int sel = -1;

	float title_off_x = 0.3;
	float item_off_x = 0.8;
	float title_off_y = 0.03;
	float line_height = 1.2f;
	float width = 150;

	const char *title = nullptr;
	std::vector<char*> content;
	std::vector<char*> *external = nullptr;

	Render_Clip item_clip = {
		CLIP_RIGHT,
		0, 0, 0, 0
	};

	std::vector<char*> *get_content() {
		return external ? external : &content;
	}

	void draw_menu(Renderer renderer, Camera& view, float menu_x, float menu_y);
	void cancel();

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	bool highlight(Camera& view, Point& inside) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Edit_Box : UI_Element {
	Edit_Box() : UI_Element(ElemEditBox, CursorEdit), editor(this) {
		action = get_edit_box_action();
		editor.multiline = false;
	}
	~Edit_Box() {
		sdl_destroy_texture(&icon);
	}

	void (*key_action)(Edit_Box*, Input&) = nullptr;

	bool text_changed = false;

	Editor editor;

	RGBA caret = {};

	float text_off_x = 0.3;
	float text_off_y = 0.03;
	float max_width = 200;

	float caret_off_y = 0.2;
	float caret_width = 0.08;

	bool icon_right = false;
	int icon_length = 0;
	RGBA icon_color = {};
	Texture icon = nullptr;

	Drop_Down *dropdown = nullptr;

	Font *ph_font = nullptr;

	std::string placeholder;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT,
		0, 0, 0, 0
	};

	void update_icon(IconType type, float height, float scale);

	bool disengage(Input& input, bool try_select) override;
	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	//void update(Rect_Int& rect) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Hex_View : UI_Element {
	Hex_View() : UI_Element(ElemHexView) {}

	bool alive = false;
	int offset = 0;
	int col_offset = 0;
	int sel = -1;
	int vis_rows = 0;
	int columns = 16;
	int addr_digits = 0;

	bool show_addrs = false;
	bool show_hex = false;
	bool show_ascii = false;

	RGBA caret = {};
	RGBA back_color = {};

	u64 region_address = 0;
	u64 region_size = 0;

	Source *source = nullptr;
	int span_idx = -1;

	float font_height = 0;
	float row_height = 0;
	float x_offset = 4;
	float padding = 0.25;
	float cursor_width = 1.5;

	Scroll *scroll = nullptr;

	void set_region(u64 address, u64 size);
	void set_offset(u64 offset);
	void update_view(float scale);

	float print_address(Renderer renderer, u64 address, float x, float y, float digit_w, Render_Clip& clip, Rect_Int& box, float padding);
	float print_hex_row(Renderer renderer, Span& span, int idx, float x, float y, Rect_Int& box, float padding);
	void print_ascii_row(Renderer renderer, Span& span, int idx, float x, float y, Render_Clip& clip, Rect_Int& box);
	void draw_cursors(Renderer renderer, int idx, Rect_Int& back, float x_start, float pad, float scale);

	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void scroll_handler(Camera& view, Input& input, Point& inside) override;
};

struct Image : UI_Element {
	Image() : UI_Element(ElemImage) {}
	~Image() {
		//sdl_destroy_texture(&img);
	}

	Texture img = nullptr;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Label : UI_Element {
	Label() : UI_Element(ElemLabel) {}

	std::string text;

	Rect outer_box = {0};

	Render_Clip clip = {
		CLIP_RIGHT,
		0, 0, 0, 0
	};

	float padding = 0.2;

	void update(Camera& view, Rect_Int& rect) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Number_Edit : UI_Element {
	Number_Edit() : UI_Element(ElemNumEdit), editor(this) {
		action = get_number_edit_action();
		editor.use_clipboard = false;
		editor.numeric_only = true;
	}
	~Number_Edit() {
		sdl_destroy_texture(&icon);
	}

	int number = 0;
	Editor editor;

	void (*key_action)(Number_Edit*, Input&) = nullptr;

	Render_Clip clip = {
		CLIP_RIGHT,
		0, 0, 0, 0
	};

	float end_width = 0.6;
	float box_width = 0.6;
	float box_height = 0.8;
	RGBA arrow_color = {0.8, 0.87, 1.0, 0.9};

	Texture icon = nullptr;
	int icon_w = 0;
	int icon_h = 0;

	void make_icon(float scale);
	void affect_number(int delta);

	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void scroll_handler(Camera& view, Input& input, Point& inside) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Scroll : UI_Element {
	Scroll() : UI_Element(ElemScroll) {}

	bool vertical = true;

	RGBA back_color = {};

	float breadth = 16;
	float length = 0;
	float padding = 2;
	float thumb_min = 0.2;

	double thumb_frac = 0;
	double view_span = 0;
	double position = 0;
	double maximum = 0;

	UI_Element *content = nullptr;

	bool show_thumb = true;
	bool hl = false;
	bool held = false;
	int hold_region = 0;

	void set_maximum(double max, double span);
	void engage(Point& p);
	void scroll(double delta);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;

	bool highlight(Camera& view, Point& inside) override;
	void deselect() override;
};

struct Tabs : UI_Element {
	Tabs() : UI_Element(ElemTabs) {}

	void (*event)(Tabs *tabs) = nullptr;

	struct Tab {
		char *name;
		float width;
	};

	std::vector<Tab> tabs;
	int sel = 0;
	int hl = -1;

	float x_offset = 0.2;
	float tab_border = 4;

	void add(const char **names, int count);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Text_Editor : UI_Element {
	Text_Editor() : UI_Element(ElemTextEditor, CursorEdit), editor(this) {
		action = get_text_editor_action();
		editor.parent = this;
		use_sf_cache = true;
	}

	bool mouse_held = false;
	bool show_caret = false;
	float border = 4;

	int caret_on_time = 35;
	int caret_off_time = 30;
	RGBA caret_color = {};

	Scroll *vscroll = nullptr;
	Scroll *hscroll = nullptr;

	Editor editor;

	Render_Clip clip = {
		CLIP_ALL,
		0, 0, 0, 0
	};

	void (*key_action)(Text_Editor*, Input&) = nullptr;

	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void scroll_handler(Camera& view, Input& input, Point& inside) override;
	void update(Camera& view, Rect_Int& rect) override;

	Render_Clip make_clip(Rect_Int& r, float edge);
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
	void post_draw(Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

enum BoxType {
	BoxDefault = 0,
	BoxOpening,
	BoxMain,
	BoxOpenSource,
	BoxViewSource,
	BoxStructs,
	BoxObject,
	BoxDefinitions
};

enum MenuType {
	MenuDefault = 0,
	MenuProcess,
	MenuFile
};

struct Workspace;

struct Box {
	BoxType box_type;
	bool expungable = false; // the expungables
	bool visible = false;
	bool moving = false;
	bool ui_held = false;
	bool dropdown_set = false;

	int ticks = 0;
	int refresh_every = 60;
	int move_start = 5;

	Workspace *parent = nullptr;
	Drop_Down *current_dd = nullptr;
	Editor *active_edit = nullptr;

	virtual void update_ui(Camera& view) {}
	virtual void refresh(Point *cursor) {}
	virtual void handle_zoom(Workspace& ws, float new_scale) {}
	virtual void wake_up() {}
	virtual void on_close() {}

	int edge = 0;

	Rect box = {};
	RGBA back = {};
	RGBA edge_color = {};

	std::vector<UI_Element*> ui;

	float cross_size = 16;
	float border = 4;
	float edge_size = 2;

	float min_width = 300;
	float min_height = 200;

	void draw(Workspace& ws, Camera& view, bool held, Point *inside, bool hovered, bool focussed);
	void require_redraw();

	void select_edge(Camera& view, Point& p);
	void move(float dx, float dy, Camera& view, Input& input);

	void set_dropdown(Drop_Down *dd);
	void update_hovered(Camera& view, Input& input, Point& inside);
	void update_focussed(Camera& view, Input& input, Point& inside, Box *hover);
	void update(Workspace& ws, Camera& view, Input& input, Box *hover, bool focussed);
};

struct Workspace {
	int dpi_w = 0, dpi_h = 0;

	Texture cross = nullptr;
	Texture maxm = nullptr;
	float cross_size = 16;

	RGBA text_color = { 1, 1, 1, 1 };
	RGBA ph_text_color = { 1, 1, 1, 0.7 };
	RGBA outline_color = { 0.4, 0.6, 0.8, 1.0 };
	RGBA back_color = { 0.1, 0.2, 0.7, 1.0 };
	RGBA dark_color = { 0.05, 0.2, 0.5, 1.0 };
	RGBA light_color = { 0.15, 0.35, 0.85, 1.0 };
	RGBA light_hl_color = { 0.2, 0.5, 1.0, 1.0 };
	RGBA table_cb_back = { 0, 0.05, 0.2, 1.0 };
	RGBA hl_color = { 0.2, 0.4, 0.7, 1.0 };
	RGBA active_color = { 0.1, 0.25, 0.8, 1.0 };
	RGBA inactive_color = { 0.2, 0.3, 0.5, 1.0 };
	RGBA inactive_text_color = { 0.7, 0.7, 0.7, 1.0 };
	RGBA inactive_outline_color = { 0.55, 0.6, 0.65, 1.0 };
	RGBA scroll_back = {0, 0.15, 0.4, 1.0};
	RGBA scroll_color = {0.4, 0.45, 0.55, 1.0};
	RGBA scroll_hl_color = {0.6, 0.63, 0.7, 1.0};
	RGBA scroll_sel_color = {0.8, 0.8, 0.8, 1.0};
	RGBA div_color = {0.5, 0.6, 0.8, 1.0};
	RGBA caret_color = {0.9, 0.9, 0.9, 1.0};
	RGBA cb_color = {0.55, 0.7, 0.9, 1.0};
	RGBA sel_color = {0.45, 0.5, 0.6, 1.0};

	Font_Face face = nullptr;
	std::vector<Font*> fonts;
	Font *default_font = nullptr;
	Font *inactive_font = nullptr;

	std::vector<Box*> boxes;
	Box *selected = nullptr;
	Box *new_box = nullptr;
	bool box_moving = false;
	bool box_expunged = false;

	bool cursor_set = false;

	std::vector<Source*> sources;

	Map definitions;

	Arena object_arena;

	String_Vector tokens;
	String_Vector name_vector;

	std::vector<Struct*> structs;
	std::vector<char*> struct_names;
	bool first_struct_run = true;

	Workspace(Font_Face face);
	~Workspace();

	Box *make_box(BoxType btype, MenuType mtype = MenuDefault);

	Box *box_under_cursor(Camera& view, Point& cur, Point& inside);
	void add_box(Box *b);
	void delete_box(int idx);
	void delete_box(Box *b);
	void bring_to_front(Box *b);

	Box *first_box_of_type(BoxType type);
	Font *make_font(float size, RGBA& color, float scale);

	void adjust_scale(float old_scale, float new_scale);
	void refresh_sources();
	void update(Camera& view, Input& input, Point& cursor);
};

Rect make_ui_box(Rect_Int& box, Rect& elem, float scale);
void reposition_box_buttons(Image& cross, Image& maxm, float box_w, float size);
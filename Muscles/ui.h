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
	ElemTextEditor
};

struct Box;

struct UI_Element {
	bool visible = true;
	bool active = true;
	bool use_default_cursor = false;

	enum Element_Type type;
	enum Cursor_Type cursor_type = CursorDefault;

	Box *parent = nullptr;

	Rect pos = {};

	RGBA default_color = {};
	RGBA inactive_color = {};
	RGBA hl_color = {};
	RGBA sel_color = {};

	Font *font = nullptr;

	void (*action)(UI_Element*, bool) = nullptr;

	virtual void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {}
	virtual void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {}
	virtual void key_handler(Camera& view, Input& input) {}

	virtual bool highlight(Camera& view, Point& inside) { return false; }
	virtual void deselect() {}
	virtual bool disengage(Input& input, bool try_select) { return false; }
	virtual void release() {}

	UI_Element() = default;
	UI_Element(Element_Type t, Cursor_Type ct = CursorDefault) : type(t), cursor_type(ct) {}
};

void (*get_set_active_edit(void))(UI_Element*, bool);
void (*get_delete_box(void))(UI_Element*, bool);

struct Scroll;

struct Button : UI_Element {
	Button() : UI_Element(ElemButton) {}

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
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Checkbox : UI_Element {
	Checkbox() : UI_Element(ElemCheckbox) {}

	bool checked = false;
	std::string text;

	float border_frac = 0.2;
	float text_off_x = 0.3;
	float text_off_y = -0.15;

	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Data_View : UI_Element {
	Data_View() : UI_Element(ElemDataView) {}

	Table data = {};

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
	bool consume_box_scroll = false;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT | CLIP_BOTTOM,
		0, 0, 0, 0
	};

	float column_width(float total_width, float min_width, float font_height, float scale, int idx);
	void draw_item_backing(RGBA& color, Rect& back, float scale, int idx);

	bool highlight(Camera& view, Point& inside) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;

	void deselect() override;
	void release() override;
};

struct Divider : UI_Element {
	Divider() : UI_Element(ElemDivider) {}

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
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
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

	void draw_menu(Camera& view, Rect_Fixed& rect);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	bool highlight(Camera& view, Point& inside) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Edit_Box : UI_Element {
	Edit_Box() : UI_Element(ElemEditBox, CursorEdit) {
		action = get_set_active_edit();
	}

	void (*key_action)(Edit_Box*, Input&) = nullptr;
	bool update = false;

	int cursor = 0;
	float offset = 0;
	float window_w = 0;
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

	std::string line;
	std::string placeholder;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT,
		0, 0, 0, 0
	};

	bool remove(bool is_back);
	void move_cursor(int delta);
	void clear();

	void update_icon(Icon_Type type, float height, float scale);

	bool disengage(Input& input, bool try_select) override;
	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Hex_View : UI_Element {
	Hex_View() : UI_Element(ElemHexView) {}

	bool alive = false;
	int offset = 0;
	int sel = -1;
	int rows = 0;
	int columns = 16;
	int addr_digits = 0;

	bool show_addrs = false;
	bool show_hex = false;
	bool show_ascii = false;

	RGBA caret = {};

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
	void update(float scale);

	float print_address(u64 address, float x, float y, float digit_w, Render_Clip& clip, Rect& box, float padding);
	float print_hex_row(Span& span, int idx, float x, float y, Rect& box, float padding);
	void print_ascii_row(Span& span, int idx, float x, float y, Render_Clip& clip, Rect& box);
	void draw_cursors(int idx, Rect& back, float x_start, float pad, float scale);

	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
};

struct Image : UI_Element {
	Image() : UI_Element(ElemImage) {}

	Texture img = nullptr;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Label : UI_Element {
	Label() : UI_Element(ElemLabel) {}

	std::string text;
	float x = 0;
	float y = 0;
	float width = 0;

	Render_Clip clip = {
		CLIP_RIGHT,
		0, 0, 0, 0
	};

	float padding = 0.2;

	void update_position(Camera& view);
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Scroll : UI_Element {
	Scroll() : UI_Element(ElemScroll) {}

	bool vertical = true;

	RGBA back = {};

	float breadth = 16;
	float length = 0;
	float padding = 2;
	float thumb_min = 0.2;

	double thumb_frac = 0;
	double view_span = 0;
	double position = 0;
	double maximum = 0;

	bool show_thumb = true;
	bool hl = false;
	bool held = false;
	int hold_region = 0;

	void set_maximum(double max, double span);
	void engage(Point& p);
	void scroll(double delta);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;

	bool highlight(Camera& view, Point& inside) override;
	void deselect() override;
};

struct Text_Editor : UI_Element {
	Text_Editor() : UI_Element(ElemTextEditor, CursorEdit) {}

	bool selected = false;
	bool mouse_held = false;

	int tab_width = 4;
	float border = 4;
	float cursor_width = 1;

	int ticks = 0;
	int caret_on_time = 35;
	int caret_off_time = 30;
	RGBA caret_color = {};

	Scroll *vscroll = nullptr;
	Scroll *hscroll = nullptr;

	struct Cursor {
		int cursor;
		int line;
		int column;
		int target_column;
	}
	primary = {0},
	secondary = {0};

	Render_Clip clip = {
		CLIP_TOP | CLIP_BOTTOM | CLIP_LEFT | CLIP_RIGHT,
		0, 0, 0, 0
	};
	std::string text;

	void (*key_action)(Text_Editor*, Input&) = nullptr;

	void erase(Cursor& cursor, bool is_back);
	void set_cursor(Cursor& cursor, int cur);
	void set_line(Cursor& cursor, int line_idx);
	void set_column(Cursor& cursor, int col_idx);

	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;

	void draw_cursor(Cursor& cursor, Rect& back, float digit_w, float font_h, float line_pad, float edge, float scale);
	void draw_selection_box(Render_Clip& clip, float digit_w, float font_h, float line_pad);
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

enum BoxType {
	BoxDefault = 0,
	BoxMain,
	BoxStructs,
	BoxObject
};

struct Workspace;

struct Box {
	BoxType type = BoxDefault;
	bool visible = false;
	bool moving = false;
	bool ui_held = false;
	bool dropdown_set = false;

	int ticks = 0;
	int refresh_every = 60;
	int move_start = 5;

	Workspace *parent = nullptr;
	Drop_Down *current_dd = nullptr;
	UI_Element *active_edit = nullptr;
	void *markup = nullptr;

	void (*update_handler)(Box&, Camera&, Input&, Point&, Box*, bool) = nullptr;
	void (*refresh_handler)(Box&, Point&) = nullptr;
	void (*scale_change_handler)(Workspace& ws, Box&, float) = nullptr;

	int edge = 0;

	Rect box = {};
	RGBA back = {};
	RGBA edge_color = {};

	std::vector<UI_Element*> ui;

	float cross_size = 16;
	float border = 4;
	float edge_size = 2;

	Box() = default;
	Box(const Box& b) = default;
	Box(Box&& b) = default;

	float min_width = 300;
	float min_height = 200;

	void draw(Workspace& ws, Camera& view, bool held, Point *inside, bool hovered, bool focussed);

	void select_edge(Camera& view, Point& p);
	void move(float dx, float dy, Camera& view, Input& input);

	void set_dropdown(Drop_Down *dd);
	void update_elements(Camera& view, Input& input, Point& inside, Box *hover, bool focussed);
	void post_update_elements(Camera& view, Input& input, Point& inside, Box *hover, bool focussed);
	void update(Workspace& ws, Camera& view, Input& input, Box *hover, bool focussed);
};

struct Workspace {
	int dpi_w = 0, dpi_h = 0;
	float temp_scale = 1.0;

	Texture cross = nullptr;
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
	RGBA caret_color = {0.9, 0.9, 0.9, 1.0};
	RGBA cb_color = {0.55, 0.7, 0.9, 1.0};
	RGBA sel_color = {0.45, 0.5, 0.6, 1.0};

	Font_Face face = nullptr;
	std::vector<Font*> fonts;
	Font *default_font = nullptr;
	Font *inactive_font = nullptr;

	std::vector<Box*> boxes;
	Box *focus = nullptr;
	Box *selected = nullptr;
	Box *new_box = nullptr;
	bool box_moving = false;

	bool cursor_set = false;

	std::vector<Source*> sources;

	Workspace(Font_Face face);
	~Workspace();

	Box *box_under_cursor(Camera& view, Point& cur, Point& inside);
	void add_box(Box *b);
	void delete_box(int idx);
	void delete_box(Box *b);
	void bring_to_front(Box *b);

	Box *first_box_of_type(BoxType type);
	Font *make_font(float size, RGBA& color);

	void adjust_scale(float old_scale, float new_scale);
	void refresh_sources();
	void update(Camera& view, Input& input, Point& cursor);
};

Rect make_ui_box(Rect_Fixed& box, Rect& elem, float scale);
float center_align_title(Label *title, Box& b, float scale, float y_offset);

void make_opening_menu(Workspace& ws, Box& b);
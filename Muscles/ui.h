#pragma once

enum ElementType {
	TypeNone = 0,
	TypeImage,
	TypeLabel,
	TypeButton,
	TypeDivider,
	TypeDataView,
	TypeScroll,
	TypeEditBox,
	TypeDropDown,
	TypeHexView
};

struct Box;

struct UI_Element {
	bool visible = true;
	bool active = true;

	enum ElementType type;
	int id = 0;

	Box *parent = nullptr;

	Rect pos = {};

	RGBA default_color = {};
	RGBA inactive_color = {};
	RGBA hl_color = {};
	RGBA sel_color = {};

	Font *font = nullptr;

	void (*action)(UI_Element*, bool) = nullptr;

	virtual void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) {}
	virtual bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) { return false; }
	virtual void key_handler(Camera& view, Input& input) {}

	virtual bool highlight(Camera& view, Point& inside) { return false; }
	virtual void deselect() {}
	virtual void release() {}

	UI_Element() = default;
	UI_Element(ElementType t) : type(t) {}
};

struct Image : UI_Element {
	Image() : UI_Element(TypeImage) {}

	Texture img = nullptr;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Label : UI_Element {
	Label() : UI_Element(TypeLabel) {}

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

struct Button : UI_Element {
	Button() : UI_Element(TypeButton) {}

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
	
	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Divider : UI_Element {
	Divider() : UI_Element(TypeDivider) {}

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

	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Scroll : UI_Element {
	Scroll() : UI_Element(TypeScroll) {}

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

	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;

	bool highlight(Camera& view, Point& inside) override;
	void deselect() override;
};

struct Data_View : UI_Element {
	Data_View() : UI_Element(TypeDataView) {}

	Table data = {};

	Scroll *hscroll = nullptr;
	Scroll *vscroll = nullptr;

	int hl_row = -1;
	int sel_row = -1;

	float item_height = 0;
	float header_height = 25;
	float column_spacing = 0.2;
	float padding = 2;

	bool show_column_names = false;
	bool consume_box_scroll = false;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT | CLIP_BOTTOM,
		0, 0, 0, 0
	};

	//void find_column(Camera& view, float total_width, int& index, Point& span, bool get_index);
	float column_width(float total_width, float font_height, float scale, int idx);
	void draw_item_backing(RGBA& color, Rect& back, float scale, int idx);

	bool highlight(Camera& view, Point& inside) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;

	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;

	void deselect() override;
	void release() override;
};

struct Edit_Box : UI_Element {
	Edit_Box() : UI_Element(TypeEditBox) {}

	bool update = false;
	Table *table = nullptr;

	int cursor = 0;
	float offset = 0;
	float window_w = 0;
	RGBA caret = {};

	float text_off_x = 0.3;
	float text_off_y = 0.03;
	float max_width = 200;

	float caret_off_y = 0.2;
	float caret_width = 0.08;

	std::string line;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT,
		0, 0, 0, 0
	};

	const char *get_str() const {
		return line.c_str();
	}

	bool remove(bool is_back);
	void move_cursor(int delta);
	void clear();

	void key_handler(Camera& view, Input& input) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Drop_Down : UI_Element {
	Drop_Down() : UI_Element(TypeDropDown) {}

	bool dropped = false;
	int sel = -1;

	float title_off_x = 0.3;
	float item_off_x = 0.8;
	float title_off_y = 0.03;
	float line_height = 1.2f;
	float width = 150;

	const char *title = nullptr;
	std::vector<const char*> lines;

	Render_Clip item_clip = {
		CLIP_RIGHT,
		0, 0, 0, 0
	};

	void draw_menu(Camera& view, Rect_Fixed& rect, bool held);

	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	bool highlight(Camera& view, Point& inside) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Hex_View : UI_Element {
	Hex_View() : UI_Element(TypeHexView) {}

	bool alive = false;
	int offset = 0;
	int size = 0;
	int rows = 0;
	int columns = 16;

	u64 region_address = 0;
	u64 region_size = 0;

	Source *source = nullptr;
	int span_idx = -1;

	Scroll *vscroll = nullptr;
	Scroll hscroll = {};

	void set_region(u64 address, u64 size);
	void update(float scale);

	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
	bool mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
};

enum BoxType {
	NoBoxType = 0,
	TypeMain
};

struct Workspace;

struct Box {
	BoxType type = NoBoxType;
	bool visible = false;
	bool moving = false;
	bool ui_held = false;

	int ticks = 0;
	int refresh_every = 60;
	int move_start = 5;

	Workspace *parent = nullptr;
	Drop_Down *current_dd = nullptr;
	void *markup = nullptr;

	void (*update_handler)(Box&, Camera&, Input&, Point&, bool, bool) = nullptr;
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
	void update_elements(Camera& view, Input& input, Point& inside, bool hovered, bool focussed);
	void post_update_elements(Camera& view, Input& input, Point& inside, bool hovered, bool focussed);
	void update(Workspace& ws, Camera& view, Input& input, bool hovered, bool focussed);
};

struct Workspace {
	int dpi_w = 0, dpi_h = 0;
	float temp_scale = 1.0;

	Texture cross = nullptr;
	float cross_size = 16;

	RGBA text_color = { 1, 1, 1, 1 };
	RGBA outline_color = { 0.4, 0.6, 0.8, 1.0 };
	RGBA back_color = { 0.1, 0.2, 0.7, 1.0 };
	RGBA dark_color = { 0.05, 0.2, 0.5, 1.0 };
	RGBA light_color = { 0.15, 0.35, 0.85, 1.0 };
	RGBA default_color = {};
	RGBA hl_color = { 0.2, 0.4, 0.7, 1.0 };
	RGBA sel_color = { 0.1, 0.25, 0.8, 1.0 };
	RGBA inactive_color = { 0.2, 0.3, 0.5, 1.0 };
	RGBA inactive_text_color = { 0.7, 0.7, 0.7, 1.0 };
	RGBA inactive_outline_color = { 0.55, 0.6, 0.65, 1.0 };
	RGBA scroll_back = {0, 0.1, 0.4, 1.0};
	RGBA scroll_color = {0.4, 0.45, 0.55, 1.0};
	RGBA scroll_hl_color = {0.6, 0.63, 0.7, 1.0};
	RGBA scroll_sel_color = {0.8, 0.8, 0.8, 1.0};
	RGBA caret_color = {0.9, 0.9, 0.9, 1.0};

	Font_Face face = nullptr;
	std::vector<Font*> fonts;
	Font *default_font = nullptr;
	Font *inactive_font = nullptr;

	std::vector<Box*> boxes;
	Box *focus = nullptr;
	Box *selected = nullptr;
	Box *new_box = nullptr;
	UI_Element *held_element = nullptr;

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
#ifndef UI_H
#define UI_H

enum ElementType {
	TypeNone = 0,
	TypeImage,
	TypeLabel,
	TypeButton,
	TypeDivider,
	TypeDataView,
	TypeScroll,
	TypeEditBox,
	TypeDropDown
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
	virtual void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {}
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
	bool auto_width = false;

	float padding = 0.2;

	void update_position(Camera& view, Rect& pos);
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

	std::string text;

	void update_size();
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Divider : UI_Element {
	Divider() : UI_Element(TypeDivider) {}

	bool vertical = false;
	bool moveable = false;

	float min = 0;
	float max = 0;

	float distance = 0;

	Texture hint = nullptr;

	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Data_View : UI_Element {
	Data_View() : UI_Element(TypeDataView) {}

	Table data = {};

	int n_items = 0;
	int n_visible = 0;
	int top = 0;
	int hl_row = -1;
	int sel_row = -1;

	float item_height = 0;
	float column_spacing = 0.2;
	float border = 10;

	//void find_column(Camera& view, float total_width, int& index, Point& span, bool get_index);
	void draw_item_backing(RGBA& color, Rect_Fixed& rect, float scale, int idx);

	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
	bool highlight(Camera& view, Point& inside) override;

	void deselect() override;
	void release() override;
};

struct Scroll : UI_Element {
	Scroll() : UI_Element(TypeScroll) {}

	Table *source = nullptr;

	RGBA back = {};

	float min_height = 0.2;
	float width = 16;
	float padding = 2;

	float thumb_frac = 0;
	float thumb_pos = 0;

	int top = 0;
	int n_items = 0;
	int n_visible = 0;
	int max = 0;

	bool hl = false;
	bool held = false;
	int hold_region = 0;

	void engage(Point& p);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;

	bool highlight(Camera& view, Point& inside) override;
	void deselect() override;
};

struct Edit_Box : UI_Element {
	Edit_Box() : UI_Element(TypeEditBox) {}

	std::string line;

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

	void draw_menu(Camera& view, Rect_Fixed& rect, bool held);

	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	bool highlight(Camera& view, Point& inside) override;
	void draw(Camera& view, Rect_Fixed& rect, bool elem_hovered, bool box_hovered, bool focussed) override;
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
	int ticks = 0;
	int refresh_every = 60;

	Workspace *parent = nullptr;
	Drop_Down *current_dd = nullptr;
	void *markup = nullptr;

	void (*update_handler)(Box&, Camera&, Input&, Point&, bool, bool) = nullptr;
	void (*refresh_handler)(Box&) = nullptr;
	void (*scale_change_handler)(Workspace& ws, Box&, float) = nullptr;

	int edge = 0;

	Rect box = {};
	RGBA back = {};

	std::vector<UI_Element*> ui;

	float cross_size = 16;
	float border = 4;

	Box() = default;
	Box(const Box& b) = default;
	Box(Box&& b) = default;

	float minimum_width() const {
		return 300;
	}
	float minimum_height() const {
		return 200;
	}

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

	Texture corner = nullptr;
	float corner_size = 8;
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
	RGBA inactive_color = { 0.4, 0.45, 0.5, 1.0 };
	RGBA inactive_text_color = { 0.9, 0.9, 0.9, 1.0 };
	RGBA inactive_outline_color = { 0.55, 0.6, 0.65, 1.0 };

	Font_Face face = nullptr;
	std::vector<Font*> fonts;
	Font *default_font = nullptr;
	Font *inactive_font = nullptr;

	std::vector<Box*> boxes;
	Box *focus = nullptr;
	Box *new_box = nullptr;

	std::vector<void*> sources;

	Workspace(Font_Face face);
	~Workspace();

	Box *box_under_cursor(Camera& view, Point& cur, Point& inside);
	void add_box(Box *b);
	void delete_box(int idx);
	void delete_box(Box *b);
	void bring_to_front(Box *b);

	Font *make_font(float size, RGBA& color);

	void adjust_scale(float old_scale, float new_scale);
	void refresh_sources();
	void update(Camera& view, Input& input, Point& cursor);
};

Rect make_ui_box(Rect_Fixed& box, Rect& elem, float scale);
float center_align_title(Label *title, Box& b, float scale, float y_offset);

void make_opening_menu(Workspace& ws, Box& b);

void make_file_menu(Workspace& ws, Box& b);
void make_process_menu(Workspace& ws, Box& b);

#endif
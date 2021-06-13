#pragma once

enum Element_Type {
	Elem_None = 0,
	Elem_Image,
	Elem_Label,
	Elem_Button,
	Elem_Divider,
	Elem_Data_View,
	Elem_Scroll,
	Elem_Edit_Box,
	Elem_Drop_Down,
	Elem_Checkbox,
	Elem_Hex_View,
	Elem_Text_Editor,
	Elem_Tabs,
	Elem_Number_Edit
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
	bool table_vis = true;
	bool active = true;
	bool use_sf_cache = false;
	bool use_default_cursor = false;
	bool use_post_draw = false;

	enum Element_Type elem_type;
	enum CursorType cursor_type = CursorDefault;

	Box *parent = nullptr;

	Rect pos = {};
	int old_width = 0;
	int old_height = 0;

	Rect_Int screen = {};

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

	void (*action)(UI_Element*, Camera&, bool) = nullptr;

	void draw(Camera& view, Rect_Int& rect, bool elem_hovered, bool box_hovered, bool focussed);
	void set_active(bool state);

	virtual void update(Camera& view, Rect_Int& back) {}
	virtual void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {}
	virtual void post_draw(Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) {}

	// 'cursor' is relative to the upper-left corner of the box
	virtual void base_action(Camera& view, Point& cursor, Input& input) {}
	virtual void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) {}
	virtual void key_handler(Camera& view, Input& input) {}

	// 'inside' is relative to the upper-left corner of this element
	virtual bool scroll_handler(Camera& view, Input& input, Point& inside) { return false; }
	virtual void highlight(Camera& view, Point& inside) {}
	virtual void deselect() {}
	virtual void release() {}

	UI_Element() = default;
	UI_Element(Element_Type t, CursorType ct = CursorDefault) : elem_type(t), cursor_type(ct) {}

	virtual ~UI_Element() {
		sdl_destroy_texture(&tex_cache);
	}
};

struct Data_View;

struct Draw_Cell_Info {
	Data_View *dv;
	Camera *camera;
	Renderer renderer;
	Rect_Int dst;
	Rect_Int src;
	float y_max;
	float line_off;
	float sp_half;
	float font_height;
	int elem_pad;
};

void (*get_delete_box(void))(UI_Element*, Camera&, bool);
void (*get_maximize_box(void))(UI_Element*, Camera&, bool);

struct Scroll;

struct Button : UI_Element {
	Button() : UI_Element(Elem_Button) {}
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
	int get_icon_length(float scale);
	
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Checkbox : UI_Element {
	Checkbox() : UI_Element(Elem_Checkbox) {}

	bool checked = false;
	std::string text;

	float border_frac = 0.2;
	float text_off_x = 0.3;
	float text_off_y = -0.15;

	float leaning = -1.0; // -1 = left, 0 = centre, 1 = right

	void toggle();
	void base_action(Camera& view, Point& cursor, Input& input) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Data_View : UI_Element {
	Data_View() : UI_Element(Elem_Data_View) {
		use_sf_cache = true;
	}
	~Data_View() {
		sdl_destroy_texture(&icon_plus);
		sdl_destroy_texture(&icon_minus);
	}

	RGBA back_color = {};

	Table *data = nullptr;
	std::vector<Table*> *tables = nullptr;

	Texture icon_plus = nullptr;
	Texture icon_minus = nullptr;

	Scroll *hscroll = nullptr;
	Scroll *vscroll = nullptr;

	UI_Element *active_elem = nullptr;

	int hl_row = -1;
	int hl_col = -1;
	int sel_row = -1;
	int condition_col = -1;

	float header_height = 25;
	float padding = 2;
	float extra_line_spacing = 0;
	float column_spacing = 0.2;
	float scroll_total = 0.2;

	float item_height = 0;
	float total_spacing = 0;

	bool show_column_names = false;

	void (*checkbox_toggle_handler)(Data_View*, int, int) = nullptr;

	Render_Clip clip = {
		CLIP_LEFT | CLIP_RIGHT | CLIP_BOTTOM,
		0, 0, 0, 0
	};

	float column_width(float total_width, float min_width, float font_height, float scale, int idx);
	void draw_item_backing(Renderer renderer, RGBA& color, Rect_Int& back, float scale, int idx);
	void draw_cell(Draw_Cell_Info& dci, void *cell, Column& header, Rect& r);

	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;

	void base_action(Camera& view, Point& cursor, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;

	bool scroll_handler(Camera& view, Input& input, Point& inside) override;
	void highlight(Camera& view, Point& inside) override;
	void deselect() override;
	void release() override;
};

struct Divider : UI_Element {
	Divider() : UI_Element(Elem_Divider) {}
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
	Drop_Down() : UI_Element(Elem_Drop_Down) {}

	bool dropped = false;
	int hl = -1;
	int sel = -1;
	bool keep_selected = false;

	UI_Element *edit_elem = nullptr;

	float item_off_x = 0.8;
	float title_pad_x = 0.3;
	float title_off_y = 0.03;

	float leaning = 0.5;

	float line_height = 1.2f;
	float width = 150;

	bool icon_right = true;
	int icon_length = 0;
	RGBA icon_color = {0};
	Texture icon = nullptr;

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

	void base_action(Camera& view, Point& cursor, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void highlight(Camera& view, Point& inside) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Edit_Box : UI_Element {
	Edit_Box() : UI_Element(Elem_Edit_Box, CursorEdit), editor(this) {
		editor.multiline = false;
	}
	~Edit_Box() {
		if (manage_icon)
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

	bool manage_icon = true;
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
	bool is_above_icon(Point& p);
	void dropdown_item_selected(Input& input);

	void key_handler(Camera& view, Input& input) override;
	void mouse_handler(Camera& view, Input& input, Point& cursor, bool hovered) override;
	void base_action(Camera& view, Point& cursor, Input& input) override;
	//void update(Rect_Int& rect) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Hex_View : UI_Element {
	Hex_View() : UI_Element(Elem_Hex_View) {}

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

	String_Vector hex_vec;

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
	bool scroll_handler(Camera& view, Input& input, Point& inside) override;
};

struct Image : UI_Element {
	Image() : UI_Element(Elem_Image) {}
	~Image() {
		//sdl_destroy_texture(&img);
	}

	Texture img = nullptr;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Label : UI_Element {
	Label() : UI_Element(Elem_Label) {}

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
	Number_Edit() : UI_Element(Elem_Number_Edit), editor(this) {
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

	float text_off_x = 0.15;
	float text_off_y = -0.15;

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
	void base_action(Camera& view, Point& cursor, Input& input) override;
	bool scroll_handler(Camera& view, Input& input, Point& inside) override;
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Scroll : UI_Element {
	Scroll() : UI_Element(Elem_Scroll) {}

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

	void highlight(Camera& view, Point& inside) override;
	void deselect() override;
};

struct Tabs : UI_Element {
	Tabs() : UI_Element(Elem_Tabs) {}

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
	Text_Editor() : UI_Element(Elem_Text_Editor, CursorEdit), editor(this) {
		editor.parent = this;
		use_sf_cache = true;
		use_post_draw = true;
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
	void base_action(Camera& view, Point& cursor, Input& input) override;
	bool scroll_handler(Camera& view, Input& input, Point& inside) override;
	void update(Camera& view, Rect_Int& rect) override;

	Render_Clip make_clip(Rect_Int& r, float edge);
	void draw_element(Renderer renderer, Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
	void post_draw(Camera& view, Rect_Int& back, bool elem_hovered, bool box_hovered, bool focussed) override;
};

struct Colors {
	RGBA background;
	RGBA text;
	RGBA ph_text;
	RGBA outline;
	RGBA back;
	RGBA dark;
	RGBA light;
	RGBA light_hl;
	RGBA table_cb_back;
	RGBA hl;
	RGBA active;
	RGBA inactive;
	RGBA inactive_text;
	RGBA inactive_outline;
	RGBA scroll_back;
	RGBA scroll;
	RGBA scroll_hl;
	RGBA scroll_sel;
	RGBA div;
	RGBA caret;
	RGBA cb;
	RGBA sel;
	RGBA editor;

	RGBA folder_dark;
	RGBA folder_light;
	RGBA process_back;
	RGBA process_outline;
	RGBA file_back;
	RGBA file_fold;
	RGBA file_line;
	RGBA cancel;
};

enum BoxType {
	BoxDefault = 0,
	BoxOpening,
	BoxMain,
	BoxOpenSource,
	BoxViewSource,
	BoxStructs,
	BoxViewObject,
	BoxDefinitions,
	BoxSearch
};

enum MenuType {
	MenuDefault = 0,
	MenuProcess,
	MenuFile,
	MenuValue,
	MenuObject
};

struct Workspace;

#define FLAG_INVISIBLE  1
#define FLAG_INACTIVE   2

struct Menu_Item {
	u32 flags;
	char *name;
	void (*action)(Workspace& ws, Box *box);
};

struct Context_Menu {
	std::vector<Menu_Item> *list = nullptr;
	Rect_Int rect = {0};
	Font *font = nullptr;
	Font *inactive_font = nullptr;
	int hl = -1;

	int get_item_height() {
		return font ? 0.5 + font->render.text_height() * 1.2 : 0;
	}
};

struct Box {
	static const BoxType box_type_meta = BoxDefault;
	BoxType box_type = BoxDefault;
	enum MenuType menu_type = MenuDefault;

	bool expungeable = false; // the expungeables
	bool visible = false;
	bool moving = false;
	bool ui_held = false;
	bool dd_menu_hovered = false;

	int ticks = 0;
	int refresh_every = 60;
	int move_start = 5;

	Workspace *parent = nullptr;
	Drop_Down *current_dd = nullptr;
	Editor *active_edit = nullptr;

	int edge = 0;

	int dd_hl_count = 0;

	Rect box = {};
	RGBA back_color = {};
	RGBA edge_color = {};

	std::vector<Menu_Item> rclick_menu_items;

	std::vector<UI_Element*> ui;

	float cross_size = 16;
	float border = 4;
	float edge_size = 2;

	float min_width = 300;
	float min_height = 200;
	float initial_width = 450;
	float initial_height = 300;

	virtual void update_ui(Camera& view) {}
	virtual void refresh(Point *cursor) {}
	virtual void handle_zoom(Workspace& ws, float new_scale) {}
	virtual void prepare_rclick_menu(Context_Menu& menu, Camera& view, Point& cursor) {}
	virtual void wake_up() {}
	virtual void on_close() {}

	void draw(Workspace& ws, Camera& view, bool held, Point *inside, bool hovered, bool focussed);
	void require_redraw();

	void select_edge(Camera& view, Point& p);
	void move(float dx, float dy, Camera& view, Input& input);

	void set_dropdown(Drop_Down *dd);
	void update_hovered(Camera& view, Input& input, Point& inside);
	void update_active_elements(Camera& view, Input& input, Point& inside, Box *hover);
	void update_element_actions(Camera& view, Input& input, Point& inside, Box *hover);
	void update(Workspace& ws, Camera& view, Input& input, Box *hover, bool focussed);
};

struct Workspace {
	Colors colors;

	int dpi_w = 0, dpi_h = 0;

	Texture cross = nullptr;
	Texture maxm = nullptr;
	float cross_size = 16;

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
	bool show_rclick_menu = false;

	std::vector<Menu_Item> default_rclick_menu;
	Box *rclick_box = nullptr;
	Context_Menu rclick_menu;

	std::vector<Source*> sources;

	Map definitions;

	Arena object_arena;

	String_Vector tokens;
	String_Vector name_vector;

	std::vector<Struct*> structs;
	std::vector<char*> struct_names;
	bool first_struct_run = true;

	void init(Font_Face face);
	void close();

	Box *box_under_cursor(Camera& view, Point& cur, Point& inside);
	void add_box(Box *b);
	void delete_box(int idx);
	void delete_box(Box *b);
	void bring_to_front(Box *b);

	Font *make_font(float size, RGBA& color, float scale);

	void refresh_sources();

	void view_source_at(Source *source, u64 address = -1);

	void adjust_scale(float old_scale, float new_scale);
	void prepare_rclick_menu(Camera& view, Point& cursor);
	void update(Camera& view, Input& input, Point& cursor);

	void update_structs(std::string& text);

	template<typename B>
	B *first_box_of_type(MenuType mtype = MenuDefault) {
		static_assert(std::is_base_of_v<Box, B>);

		if (mtype != MenuDefault) {
			for (auto& b : boxes) {
				if (b->box_type == B::box_type_meta && b->menu_type == mtype)
					return dynamic_cast<B*>(b);
			}
		}
		else {
			for (auto& b : boxes) {
				if (b->box_type == B::box_type_meta)
					return dynamic_cast<B*>(b);
			}
		}
		return nullptr;
	}

	template<typename B>
	B *make_box(MenuType mtype = MenuDefault) {
		static_assert(std::is_base_of_v<Box, B>);

		B *box = first_box_of_type<B>(mtype);
		if (box && !box->expungeable) {
			box->visible = true;
			bring_to_front(box);
		}
		else {
			B *b = new B(*this, mtype);
			b->box_type = B::box_type_meta;
			b->menu_type = mtype;
			b->parent = this;
			b->visible = true;

			for (auto& elem : b->ui)
				elem->parent = b;

			Camera& view = get_default_camera();
			float x = view.center_x - b->initial_width * view.scale / 2;
			float y = view.center_y - b->initial_height * view.scale / 2;
			Point p = view.to_world(x, y);

			b->box = { p.x, p.y, b->initial_width, b->initial_height };

			b->edge_color = colors.outline;
			b->back_color = colors.back;

			add_box(b);
			box = b;
		}

		if (box)
			box->wake_up();

		return box;
	}
};

Rect make_ui_box(Rect_Int& box, Rect& elem, float scale);
Rect_Int make_int_ui_box(Rect_Int& box, Rect& elem, float scale);

void reposition_box_buttons(Image& cross, Image& maxm, float box_w, float size);

void load_config(Workspace& ws, std::string& path);

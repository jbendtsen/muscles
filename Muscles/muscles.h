#pragma once

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <cstring>

#include "containers.h"

#define MIN_CHAR ' '
#define MAX_CHAR '~'
#define N_CHARS  (MAX_CHAR - MIN_CHAR + 1)

#ifdef _WIN32
	typedef unsigned long THREAD_RETURN_TYPE; // DWORD
	typedef void* SOURCE_HANDLE; // HANDLE
#else
	typedef void* THREAD_RETURN_TYPE;
	typedef int SOURCE_HANDLE;
#endif

typedef void* Texture;
typedef void* Surface;
typedef void* Renderer;
typedef void* Font_Face;

struct SDL;

Font_Face load_font_face(const char *path);
void destroy_font_face(Font_Face face);
void ft_quit();

int run();

int count_hex_digits(u64 num);
void write_hex(char *out, u64 n, int n_digits);
void write_dec(char *out, s64 n);

struct Glyph {
	int atlas_x;
	int atlas_y;
	int img_w;
	int img_h;
	float box_w;
	float box_h;
	int left;
	int top;
};

struct RGBA {
	float r;
	float g;
	float b;
	float a;
};

struct Point {
	float x, y;
};

struct Rect_Int;

struct Rect {
	float x, y;
	float w, h;

	Rect_Int to_rect_int() const;

	bool contains(Point& p) const {
		return p.x >= x && p.x < x+w && p.y >= y && p.y < y+h;
	}
	bool contains(float px, float py) const {
		return px >= x && px < x+w && py >= y && py < y+h;
	}
};

struct Rect_Int {
	int x, y;
	int w, h;

	Rect to_rect() const;

	bool contains(Point& p) const {
		return p.x >= x && p.x < x+w && p.y >= y && p.y < y+h;
	}
	bool contains(float px, float py) const {
		return px >= x && px < x+w && py >= y && py < y+h;
	}
};

#define CLIP_TOP     1
#define CLIP_BOTTOM  2
#define CLIP_LEFT    4
#define CLIP_RIGHT   8
#define CLIP_ALL    15

struct Render_Clip {
	u32 flags = 0;
	float x_lower = 0, y_lower = 0;
	float x_upper = 0, y_upper = 0;
};

struct Input;
struct Camera;

enum CursorType {
	CursorDefault = 0,
	CursorClick,
	CursorEdit,
	CursorPan,
	CursorResizeNorthSouth,
	CursorResizeWestEast,
	CursorResizeNESW,
	CursorResizeNWSE,
	NumCursors
};

void *sdl_get_hw_renderer();
void sdl_set_cursor(CursorType type);
void sdl_get_dpi(int& w, int& h);

void sdl_log_string(const char *msg);
void sdl_log_last_error();

Renderer sdl_acquire_sw_renderer(int w, int h);
Texture sdl_bake_sw_render();

bool sdl_init(const char *title, int width, int height, RGBA bg_color);
void sdl_close();

bool sdl_poll_input(Input& input);
void sdl_update_camera(Camera& camera);

void sdl_acquire_mouse();
void sdl_release_mouse();

void sdl_copy(std::string& string);
int sdl_paste_into(std::string& string, int offset, int (*filterer)(char*, int));

void sdl_apply_texture(Texture tex, Rect_Int& dst, Rect_Int *src, Renderer rdr);
void sdl_apply_texture(Texture tex, Rect& dst, Rect_Int *src, Renderer rdr);

void sdl_draw_rect(Rect_Int& rect, RGBA& color, Renderer rdr);
void sdl_draw_rect(Rect& rect, RGBA& color, Renderer rdr);
void sdl_draw_rect_clipped(Rect_Int& rect, Render_Clip& clip, RGBA& color, Renderer rdr);
void sdl_draw_rect_clipped(Rect& rect, Render_Clip& clip, RGBA& color, Renderer rdr);

void sdl_clear();
void sdl_render();

Surface sdl_create_surface(u32 *data, int w, int h);
Texture sdl_create_texture(u32 *data, int w, int h);
void sdl_get_texture_size(Texture tex, int *w, int *h);

void *sdl_lock_texture(Texture tex);
void sdl_unlock_texture(Texture tex);
void sdl_destroy_texture(Texture *tex);

enum IconType {
	IconNone = 0,
	IconGoto,
	IconGlass,
	IconTriangle
};

Texture make_circle(RGBA& color, int diameter);
Texture make_triangle(RGBA& color, int width, int height, bool up = false);
Texture make_rectangle(RGBA& color, int width, int height, float left, float top, float thickness);
Texture make_cross_icon(RGBA& color, int length, float inner, float outer);
Texture make_folder_icon(RGBA& dark, RGBA& light, int w, int h);
Texture make_file_icon(RGBA& back, RGBA& fold_color, RGBA& line_color, int w, int h);
Texture make_process_icon(RGBA& back, RGBA& outline, int length);
Texture make_divider_icon(RGBA& color, int width, int height, double gap, double thicc, double sharpness, bool up_down);
Texture make_goto_icon(RGBA& color, int length);
Texture make_glass_icon(RGBA& color, int length);
Texture make_plus_minus_icon(RGBA& color, int length, bool plus);

void *allocate_ui_element(Arena& arena, int i_type);

#define MAX_FNAME 112

struct File_Entry {
	u32 flags;
	char *path;
	char name[MAX_FNAME];
};

enum ColumnType {
	ColumnNone = 0,
	ColumnString,
	ColumnFile,
	ColumnDec,
	ColumnHex,
	ColumnStdString,
	ColumnImage,
	ColumnCheckbox,
	ColumnElement
};

// Column Type, Count Per Cell, Fractional Width, Minimum Heights, Maximum Heights, Name
struct Column {
	ColumnType type = ColumnNone;
	int count_per_cell = 0;        // Number of elements (eg. chars or digits) to allocate or print per cell
	float width = 0;               // Fraction of the width of the whole table
	float min_size = 0;            // Minimum column width in font height units (ie. 1.0 == the height of the current font in pixels)
	float max_size = 0;
	const char *name;              // Used as the column header text
};

// row_idx, length, name_idx, closed
struct Branch {
	int row_idx;
	int length;
	int name_idx;
	bool closed;
};

struct Table {
	Arena *arena = nullptr;
	std::vector<Column> headers;
	std::unique_ptr<std::vector<void*>[]> columns;
	std::set<int> list;
	int filtered = -1;
	bool has_ui_elements = false;

	void *element_tag = nullptr;
	void (*element_init_func)(Table*, int, int) = nullptr;

	String_Vector branch_name_vector;
	std::vector<Branch> branches;
	std::vector<int> tree;

	Table();
	Table(const Table& t);
	Table(Table&& t);
	~Table();

	int column_count() const {
		return headers.size();
	}
	int row_count() const {
		return headers.size() > 0 ? columns[0].size() : 0;
	}

	int index_from_filtered(int sel) {
		return filtered < 0 ? sel : *std::next(list.begin(), sel);
	}
	int filtered_from_index(int idx) {
		auto it = list.find(idx);
		return it == list.end() ? -1 : std::distance(list.begin(), it);
	}

	void set_checkbox(int col, int row, bool state) {
		*(u8*)&columns[col][row] = state ? 2 : 1;
	}
	void toggle_checkbox(int col, int row) {
		*(u8*)&columns[col][row] = *(u8*)&columns[col][row] == 2 ? 1 : 2;
	}
	bool checkbox_checked(int col, int row) const {
		return *(u8*)&columns[col][row] == 2;
	}

	void init(Column *headers, Arena *a, void *m, void (*elem_init)(Table*, int, int), int n_cols, int n_rows);
	void resize(int n_rows);
	void clear_filter();
	void clear_data();
	void update_filter(std::string& line);

	void update_tree(std::vector<Branch> *new_branches = nullptr);
	int get_table_index(int view_idx);

	void release();
};

struct Font_Render {
	Texture tex = nullptr;
	Glyph glyphs[N_CHARS] = {};
	int pts = 0;

	const Glyph *glyph_for(char c) const {
		if (c < MIN_CHAR || c > MAX_CHAR)
			return nullptr;
		return &glyphs[c - MIN_CHAR];
	}

	float text_width(const char *text, int cursor = 0, float *x_out = nullptr) const {
		float w = 0;
		int len = strlen(text);

		for (int i = 0; i < len; i++) {
			if (x_out && cursor == i)
				*x_out = w;

			const Glyph *gl = glyph_for(text[i]);
			if (gl)
				w += gl->box_w;
		}

		if (x_out && cursor == len)
			*x_out = w;

		return w;
	}

	float text_height() const {
		return glyph_for('j')->box_h;
	}

	float digit_width() const {
		return glyph_for('0')->box_w;
	}

	void draw_text_simple(void *renderer, const char *text, float x, float y);
	void draw_text(void *renderer, const char *text, float x, float y, Render_Clip& clip, int tab_cols = 4, bool multiline = false);

	Font_Render() = default;
	~Font_Render() {
		sdl_destroy_texture(&tex);
	}
};

struct Font {
	Font_Face face = nullptr;
	float size = 0;
	RGBA color = {1, 1, 1, 1};
	float prev_scale = 1.0;
	Font_Render render = {};

	const float line_spacing = 0.1;
	const float line_offset = 0.15;

	Font(Font_Face f, float sz, RGBA& color, int dpi_w, int dpi_h);

	void adjust_scale(float scale, int dpi_w, int dpi_h);
};

struct Camera {
	float x = 0, y = 0;
	float scale = 1.0;
	float max_scale = 3.0;
	float min_scale = 0.1;
	float zoom_coeff = 0.1;
	float scroll_delta = 20;
	int center_x = 0, center_y = 0;
	bool moving = false;

	Point to_world(Point p) const {
		return {
			x + (p.x - center_x) / scale,
			y + (p.y - center_y) / scale
		};
	}
	Point to_world(int px, int py) const {
		return {
			x + (float)(px - center_x) / scale,
			y + (float)(py - center_y) / scale
		};
	}

	float to_world_x(int p) const {
		return x + (float)(p - center_x) / scale;
	}
	float to_world_y(int p) const {
		return y + (float)(p - center_y) / scale;
	}
	
	Point to_screen(Point p) const {
		return {
			center_x + (p.x - x) * scale,
			center_y + (p.y - y) * scale
		};
	}
	Point to_screen(float px, float py) const {
		return {
			center_x + (px - x) * scale,
			center_y + (py - y) * scale
		};
	}

	Rect_Int to_screen_rect(Rect r) const {
		return {
			(int)(center_x + (r.x - x) * scale + 0.5),
			(int)(center_y + (r.y - y) * scale + 0.5),
			(int)(r.w * scale + 0.5),
			(int)(r.h * scale + 0.5)
		};
	}
};

Camera& get_default_camera();

struct Input {
	bool lmouse = false, rmouse = false;
	bool lclick = false, rclick = false;
	int scroll_x = 0, scroll_y = 0;

	int mouse_x = 0, mouse_y = 0;
	int prev_x = 0, prev_y = 0;
	int click_x = 0, click_y = 0;
	int action_x = 0, action_y = 0;
	int held = 0;

	bool action = false;
	bool prevent_action = false;
	bool double_click = false;
	float action_circle = 2.5;
	int action_timer = 0;
	int action_timeout = 30;

	bool lctrl = false, rctrl = false;
	bool lshift = false, rshift = false;

	int del = 0;
	int back = 0;
	int esc = 0;
	int enter = 0;
	int tab = 0;
	int home = 0;
	int end = 0;
	int pgup = 0;
	int pgdown = 0;

	int left = 0;
	int down = 0;
	int right = 0;
	int up = 0;

	int last_key = 0;
	char ch = 0;

	bool strike(int t) {
		return t == 1 || (t >= 20 && (t % 5 == 1));
	}
};

#define REG_PM_EXEC   0
#define REG_PM_WRITE  1
#define REG_PM_READ   2

struct Span {
	u8 *data = nullptr;
	u64 address = 0;
	int size = 0;
	int retrieved = 0;
	int offset = 0;
	int tag = 0;
	u32 flags = 0;
};

struct Region {
	char *name = nullptr;
	u64 base = 0;
	u64 size = 0;
	//u32 offset = 0;
	u32 flags = 0;
};

enum SourceType {
	SourceNone = 0,
	SourceFile,
	SourceProcess
};

struct Source {
	SourceType type = SourceNone;
	std::string name;

	Arena arena;

	int pid = 0;
	void *identifier = nullptr;

	union {
		int fd = 0;
		void *handle;
	};

	u8 *buffer = nullptr;
	int buf_size = 0;

	// list of pages, where each page is 4k of text loaded from /proc/<pid>/maps
	std::vector<char*> proc_map_pages;

	Map address_to_region;

	std::vector<Region> regions;
	std::vector<Span> spans;

	bool region_refreshed = false;
	bool block_region_refresh = false;

	int refresh_region_rate = 60;
	int refresh_span_rate = 60;
	int timer = 0;

	int request_span();
	void deactivate_span(int idx);
	void gather_data();
};

#define PAGE_SIZE 0x1000

const char *get_folder_separator();
const char *get_root_folder();
const char *get_project_path();

bool is_folder(const char *name);

std::pair<int, std::unique_ptr<u8[]>> read_file(std::string& path);

void get_process_id_list(std::vector<s64>& list);
int get_process_names(std::vector<s64> *list, Map& icon_map, std::vector<void*>& icons, std::vector<void*>& pids, std::vector<void*>& names, int count_per_cell);

void enumerate_files(char *path, std::vector<File_Entry*>& files, Arena& arena);

void refresh_file_region(Source& source);
void refresh_file_spans(Source& source, std::vector<Span>& input);

void refresh_process_regions(Source& source);
void refresh_process_spans(Source& source, std::vector<Span>& input);

void close_source(Source& source);

SOURCE_HANDLE get_readonly_file_handle(void *identifier);
SOURCE_HANDLE get_readonly_process_handle(int pid);

int read_page(SOURCE_HANDLE handle, SourceType type, u64 address, char *buf);

void wait_ms(int ms);

bool start_thread(void** thread_ptr, void *data, THREAD_RETURN_TYPE (*function)(void*));


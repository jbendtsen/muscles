#pragma once

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>

#define MIN_CHAR ' '
#define MAX_CHAR '~'
#define N_CHARS  (MAX_CHAR - MIN_CHAR + 1)

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef void* Texture;
typedef void* Font_Face;

struct SDL;

Font_Face load_font_face(const char *path);
void destroy_font_face(Font_Face face);

int run();

struct Glyph {
	int atlas_x;
	int atlas_y;
	int img_w;
	int img_h;
	int box_w;
	int box_h;
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

struct Rect {
	float x, y;
	float w, h;

	bool contains(Point& p) const {
		return p.x >= x && p.x < x+w && p.y >= y && p.y < y+h;
	}
	bool contains(float px, float py) const {
		return px >= x && px < x+w && py >= y && py < y+h;
	}
};

struct Rect_Fixed {
	int x, y;
	int w, h;
};

struct Input;
struct Camera;

void sdl_get_dpi(int& w, int& h);

void sdl_log_string(const char *msg);

bool sdl_init(const char *title, int width, int height);
void sdl_close();

bool sdl_poll_input(Input& input);
void sdl_update_camera(Camera& camera);

void sdl_acquire_mouse();
void sdl_release_mouse();

void sdl_apply_texture(Texture tex, Rect_Fixed *dst, Rect_Fixed *src = nullptr);
void sdl_apply_texture(Texture tex, Rect *dst, Rect_Fixed *src = nullptr);

void sdl_draw_rect(Rect_Fixed& rect, RGBA& color);
void sdl_draw_rect(Rect& rect, RGBA& color);

void sdl_clear();
void sdl_render();

Texture sdl_create_texture(u32 *data, int w, int h);
void sdl_get_texture_size(Texture tex, int *w, int *h);

void *sdl_lock_texture(Texture tex);
void sdl_unlock_texture(Texture tex);
void sdl_destroy_texture(Texture *tex);

Texture make_circle(int diameter, RGBA& color);
Texture make_cross_icon(int length, RGBA& color);
Texture make_folder_icon(RGBA& dark, RGBA& light, int w, int h);
Texture make_file_icon(RGBA& back, RGBA& fold_color, RGBA& line_color, int w, int h);
Texture make_process_icon(RGBA& back, RGBA& outline, int length);
Texture make_vertical_divider_icon(RGBA& color, int height, double squish, double gap, double thicc, double sharpness, int *width = nullptr);

#define MAX_FNAME 112

struct File_Entry {
	int flags;
	char *path;
	char name[MAX_FNAME];
};

enum Type {
	None = 0,
	String,
	Tex,
	File
};

struct Column {
	Type type;
	int count_per_cell;
	float width;
	float min_size;
	float max_size;
	const char *name;
};

struct Table {
	std::vector<Column> headers;
	std::unique_ptr<std::vector<void*>[]> columns;
	std::set<int> list;
	int visible = -1;

	Table() = default;
	Table(const Table& t);
	Table(Table&& t);

	int column_count() const {
		return headers.size();
	}
	int row_count() const {
		return headers.size() > 0 ? columns[0].size() : 0;
	}

	void init(Column *headers, int n_cols, int n_rows);
	void resize(int n_rows);
	void clear_filter();
	void clear_data();
	void update_filter(std::string& line);

	void release();
};

struct Arena {
	std::vector<char*> pools;
	int pool_size = 1024 * 1024;
	int idx = 0;

	void add_pool() {
		pools.push_back(new char[pool_size]);
		idx = 0;
	}

	Arena() {
		add_pool();
	}
	~Arena() {
		for (auto& p : pools)
			delete[] p;
	}

	void *allocate(int size);
	void *alloc_cell(Column& info);
	char *alloc_string(char *str);
};

Arena *get_arena();

#define CLIP_TOP     1
#define CLIP_BOTTOM  2
#define CLIP_LEFT    4
#define CLIP_RIGHT   8

struct Render_Clip {
	int flags = 0;
	float x_lower = 0, y_lower = 0;
	float x_upper = 0, y_upper = 0;
};

struct Font_Render {
	Texture tex = nullptr;
	Glyph glyphs[N_CHARS] = {};
	int pts = 0;

	Glyph *glyph_for(char c) {
		if (c < MIN_CHAR || c > MAX_CHAR)
			return nullptr;
		return &glyphs[c - MIN_CHAR];
	}

	int text_width(const char *text, int cursor = 0, int *x_out = nullptr) {
		int w = 0;
		int len = strlen(text);

		for (int i = 0; i < len; i++) {
			if (x_out && cursor == i)
				*x_out = w;

			Glyph *gl = glyph_for(text[i]);
			if (gl)
				w += gl->box_w;
		}

		if (x_out && cursor == len)
			*x_out = w;

		return w;
	}

	int text_height() {
		return glyph_for('j')->box_h;
	}

	void draw_text_simple(const char *text, int x, int y);
	void draw_text(const char *text, int x, int y, Render_Clip& clip);

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
};

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

	int left = 0;
	int down = 0;
	int right = 0;
	int up = 0;

	char ch = 0;

	bool strike(int t) {
		return t == 1 || (t >= 20 /* && (t % 10 == 0)*/);
	}
};

#define REG_PM_EXEC   0
#define REG_PM_WRITE  1
#define REG_PM_READ   2

struct Span {
	u64 offset = 0;
	int size = 0;
	int retrieved = 0;
	int clients = 0;
	u8 *cache = nullptr;
};

struct Region {
	u64 base = 0;
	u64 size = 0;
	int flags = 0;
	char *name = nullptr;
};

enum SourceType {
	TypeNoSource = 0,
	TypeFile,
	TypeProcess
};

struct Source {
	SourceType type = TypeNoSource;
	std::string name;

	int pid = 0;
	void *identifier = nullptr;
	void *handle = nullptr;

	std::vector<Region> regions;
	std::vector<Span> spans;

	std::map<u64, int> region_index;
	std::map<u64, char*> sections;

	bool region_refreshed = false;
	bool block_region_refresh = false;

	int refresh_region_rate = 60;
	int refresh_span_rate = 60;
	int timer = 0;

	int reset_span(int old_idx, u64 offset, int size);
	int set_offset(int idx, u64 offset);
	void delete_span(int idx);
};

void get_process_id_list(std::vector<int>& list);
int get_process_names(std::vector<int>& list, std::vector<Texture>& icons, std::vector<char*>& names, int count_per_cell);

const char *get_folder_separator();
const char *get_root_folder();
bool is_folder(const char *name);

void enumerate_files(char *path, std::vector<File_Entry*>& files, Arena& arena);

void refresh_file_region(Source& source, Arena& arena);
void refresh_process_regions(Source& source, Arena& arena);

void refresh_file_spans(Source& source, Arena& arena);
void refresh_process_spans(Source& source, Arena& arena);

void close_source(Source& source);
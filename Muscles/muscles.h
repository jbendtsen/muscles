#pragma once

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <cstring>

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

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed long long s64;

typedef void* Texture;
typedef void* Surface;
typedef void* Renderer;
typedef void* Font_Face;

struct SDL;

Font_Face load_font_face(const char *path);
void destroy_font_face(Font_Face face);
void ft_quit();

int run();

std::pair<int, int> next_power_of_2(int num);
int count_hex_digits(u64 num);
void write_hex(char *out, u64 n, int n_digits);
void write_dec(char *out, s64 n);

union Value64 {
	u64 i;
	double d;
};

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

void clear_word(char* word);
char *advance_word(char *p);

struct Arena {
	std::vector<char*> pools;
	std::vector<char*> big_pools;

	int pool_size = 1024 * 1024;
	int idx = 0;

	int rewind_pool = -1;
	int rewind_idx = -1;

	void set_rewind_point() {
		rewind_pool = pools.size() - 1;
		if (rewind_pool < 0) rewind_pool = 0;
		rewind_idx = idx;
	}

	void add_pool() {
		pools.push_back(new char[pool_size]);
		idx = 0;
	}
	void *add_big_pool(int size) {
		big_pools.push_back(new char[size]);
		return (void*)big_pools.back();
	}

	Arena() = default;
	~Arena() {
		for (auto& p : pools) {
			delete[] p;
			p = nullptr;
		}
		for (auto& p : big_pools) {
			delete[] p;
			p = nullptr;
		}
	}

	void *allocate(int size, int align = 0);
	char *alloc_string(char *str);

	void *store_buffer(void *buf, int size, u64 align = 4);

	void rewind();

	template<class T>
	T *allocate_object() {
		T *t = (T*)allocate(sizeof(T), sizeof(void*));
		new(t) T();
		return t;
	}
};

Arena& get_default_arena();

void *allocate_ui_element(Arena& arena, int i_type);

struct String_Vector {
	char *pool = nullptr;
	int pool_size = 32;
	int head = 0;

	~String_Vector() {
		if (pool) delete[] pool;
	}

	char *at(int idx);

	void try_expand(int new_size = 0);

	int add_string(const char *str);
	int add_buffer(const char *buf, int size);

	char *allocate(int size);
	void append_extra_zero();

	void clear();
};

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

struct Bucket {
	u32 flags = 0;
	int name_idx = 0;
	int parent_name_idx = 0;
	union {
		u64 value = 0;
		void *pointer;
	};
};

struct Map {
	String_Vector default_sv;
	String_Vector *sv = nullptr;

	Bucket *data = nullptr;
	int log2_slots = 7;
	int n_entries = 0;
	float max_load = 0.75;
	const u32 seed = 331;

	Map() {
		data = new Bucket[1 << log2_slots]();
		sv = &default_sv;
	}
	Map(int n_slots) {
		log2_slots = next_power_of_2(n_slots).second;
		data = new Bucket[1 << log2_slots]();
		sv = &default_sv;
	}
	~Map() {
		delete[] data;
	}

	Bucket& operator[](const char *str) {
		return data[get(str, 0)];
	}

	int size() const {
		return 1 << log2_slots;
	}

	char *key_from_index(int idx) {
		return sv->at(data[idx].name_idx);
	}

	int get(const char *str, int len = 0);
	Bucket& insert(const char *str, int len = 0);
	void remove(const char *str, int len = 0);

	void erase_all_of_type(u32 flags);
	void erase_all_of_exact_type(u32 flags);
	void erase_all_similar_to(u32 flags);

	void next_level();
	void next_level_maybe();
	void release();
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

struct Region_Map {
	Region *data = nullptr;
	float max_load = 0.75;
	int log2_slots = 3;
	int n_entries = 0;

	const u32 seed = 2357;

	Region_Map() {
		data = new Region[1 << log2_slots]();
	}
	Region_Map(int n_slots) {
		log2_slots = next_power_of_2(n_slots - 1).second;
		data = new Region[1 << log2_slots]();
	}
	~Region_Map() {
		delete[] data;
	}

	Region& operator[](u64 key) {
		return data[get(key)];
	}

	int get(u64 key);
	void insert(u64 key, Region& reg);
	void clear(int new_size);

	void ensure(int min_size) {
		if ((1 << log2_slots) < min_size)
			clear(min_size);
	}

	int size() { return 1 << log2_slots; }

	void place_at(int idx, Region& reg);
	void place_at(int idx, Region&& reg);
	void next_level();
	void next_level_maybe();
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

	Region_Map regions_map;
	Region_Map sections;

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

#define MAX_SEARCH_RESULTS 10000
#define MAX_SEARCH_PARAMS 100

#define METHOD_EQUALS 0
#define METHOD_RANGE  1

struct Search_Parameter {
	u32 flags;
	int method;
	int offset;
	int size;
	u64 value1;
	u64 value2;
};

struct Search {
	Search_Parameter single_value = {0};
	Search_Parameter *params = nullptr;
	int n_params = 0;

	int byte_align = 0;
	u64 start_addr = 0;
	u64 end_addr = 0;

	SourceType source_type = SourceNone;
	int pid = 0;
	void *identifier = nullptr;
};

void start_search(Search& s, std::vector<Region> const& regions);
bool check_search_running();
bool check_search_finished();
void get_search_results(std::vector<u64>& results_vec);
void reset_search();
void exit_search();

#define FLAG_POINTER       0x0001
#define FLAG_BITFIELD      0x0002
#define FLAG_UNNAMED       0x0004
#define FLAG_COMPOSITE     0x0008
#define FLAG_UNION         0x0010
#define FLAG_SIGNED        0x0020
#define FLAG_FLOAT         0x0040
#define FLAG_BIG_ENDIAN    0x0080
#define FLAG_PRIMITIVE     0x0100
#define FLAG_ENUM          0x0200
#define FLAG_ENUM_ELEMENT  0x0400
#define FLAG_TYPEDEF       0x0800
#define FLAG_VALUE_INITED  0x1000

#define FIELD_FLAGS        0x1fff
#define PRIMITIVE_FLAGS    0x00ff

#define FLAG_UNUSABLE      0x4000
#define FLAG_UNRECOGNISED  0x8000

#define FLAG_AVAILABLE  0x10000

#define FLAG_OCCUPIED  0x80000000
#define FLAG_NEW       0x40000000
#define FLAG_EXTERNAL  0x20000000

struct Struct;

struct Field {
	int field_name_idx;  // These would be strings, except that they point into a String_Vector
	int type_name_idx;
	Struct *st;          // The struct/union that the field refers to, if it's a composite field
	Struct *this_st;     // The struct that the field originally belongs to
	Struct *paste_st;    // The struct that this field currently lives in
	char *parent_tag;    // Used for the accumulated name of the struct that owns this field
	int paste_array_idx; // In case this field is embedded inside an array of structs, this keeps track of the struct index
	int paste_field;     // Refers to the field where the this field's original struct has been embedded/instantiated/pasted
	int index;           // The index of this field within its original struct
	int bit_offset;
	int bit_size;
	int default_bit_size;
	int array_len;
	int pointer_levels;
	u32 flags;

	Value64 value;

	void reset() {
		memset(this, 0, sizeof(Field));
		field_name_idx = type_name_idx = paste_field = paste_array_idx = index = -1;
	}
};

struct Field_Vector {
	Field *data = nullptr;
	int pool_size = 8;
	int n_fields = 0;

	Field_Vector();
	~Field_Vector();

	void expand();

	Field& back();
	Field& add(Field& f);
	Field& add_blank();

	void cancel_latest();
	void zero_out();
};

struct Struct {
	int name_idx;
	int offset;
	int total_size;
	int longest_primitive;
	u32 flags;
	Field_Vector fields;
};

bool is_struct_usable(Struct *s);
void set_primitives(Map& definitions);
Value64 evaluate_number(const char *token, bool as_float = false);
int get_full_field_name(Field& field, String_Vector& name_vector, String_Vector& out_vec);

void tokenize(String_Vector& tokens, const char *text, int sz);
void parse_typedefs_and_enums(Map& definitions, String_Vector& tokens);
void parse_c_struct(std::vector<Struct*>& structs, char **tokens, String_Vector& name_vector, Map& definitions, Struct *st = nullptr);

enum StringStyle {
	StringAuto = 0,
	StringNone,
	StringBA,
	StringAscii
};

enum BracketsStyle {
	BracketsAuto = 0,
	BracketsNone,
	BracketsSquare,
	BracketsParen,
	BracketsCurly
};

enum PrecisionStyle {
	PrecisionAuto = -2,
	PrecisionField
};

enum FloatStyle {
	FloatNone = 0,
	FloatAuto,
	FloatFixed,
	FloatScientific
};

enum SignStyle {
	SignAuto = 0,
	SignSigned,
	SignUnsigned
};

#define SEPARATOR_LEN 4
#define PREFIX_LEN    4

struct Value_Format {
	enum StringStyle string = StringAuto;
	enum BracketsStyle brackets = BracketsAuto;
	char separator[4] = {0};
	char prefix[4] = {0};
	int base = 10;
	union {
		enum PrecisionStyle precision = PrecisionAuto;
		int digits;
	};
	enum FloatStyle floatfmt = FloatNone;
	bool uppercase = false;
	enum SignStyle sign = SignAuto;
	bool big_endian = false;
};

char *format_field_name(Arena& arena, String_Vector& in_vec, Field& field);
char *format_type_name(Arena& arena, String_Vector& in_vec, Field& field);

void format_field_value(Field& field, Value_Format& format, Span& span, char*& cell, int cell_len);

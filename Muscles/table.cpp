#include <numeric>
#include "muscles.h"

Table::Table() : arena(&get_default_arena()) {}

Table::Table(const Table& t) :
	headers(t.headers),
	arena(t.arena ? t.arena : &get_default_arena())
{
	int n_cols = t.column_count();
	int n_rows = t.row_count();
	if (!n_cols || !n_rows)
		return;

	columns = std::make_unique<std::vector<void*>[]>(n_cols);

	for (int i = 0; i < n_cols; i++) {
		columns[i].resize(n_rows, nullptr);
		std::copy(t.columns[i].begin(), t.columns[i].end(), columns[i].begin());
	}
}

Table::Table(Table&& t) :
	headers(std::move(t.headers)),
	columns(std::move(t.columns)),
	arena(t.arena ? t.arena : &get_default_arena())
{}

Table::~Table() {
	clear_data();
}

void Table::resize(int n_rows) {
	int n_cols = column_count();

	for (int i = 0; i < n_cols; i++) {
		columns[i].resize(n_rows, nullptr);

		if (headers[i].count_per_cell > 0) {
			if (headers[i].type == ColumnString) {
				for (int j = 0; j < n_rows; j++) {
					if (columns[i][j])
						continue;

					columns[i][j] = arena->allocate(headers[i].count_per_cell);
					((char*)(columns[i][j]))[0] = 0;
				}
			}
			else if (headers[i].type == ColumnElement) {
				for (int j = 0; j < n_rows; j++) {
					if (columns[i][j])
						continue;

					columns[i][j] = allocate_ui_element(*arena, headers[i].count_per_cell);
					if (element_init_func)
						element_init_func(this, i, j);
				}
			}
		}
	}
}

void Table::init(Column *headers, Arena *a, void *tag, void (*elem_init)(Table*, int, int), int n_cols, int n_rows) {
	element_tag = tag;
	element_init_func = elem_init;
	if (a)
		arena = a;

	this->headers.resize(n_cols);
	memcpy(this->headers.data(), headers, n_cols * sizeof(Column));

	for (auto& h : this->headers) {
		if (h.type == ColumnElement)
			has_ui_elements = true;
	}

	columns = std::make_unique<std::vector<void*>[]>(n_cols);
	if (n_rows > 0)
		resize(n_rows);
}

void Table::clear_filter() {
	list.clear();
	filtered = -1;
}

void Table::clear_data() {
	clear_filter();
	int n_cols = column_count();
	for (int i = 0; i < n_cols; i++)
		columns[i].clear();
}

inline void lowify(char *str) {
	for (int i = 0; str[i]; i++)
		str[i] = str[i] >= 'a' && str[i] <= 'z' ? str[i] - 0x20 : str[i];
}

void Table::update_filter(std::string& filter) {
	list.clear();
	if (!filter.size()) {
		filtered = -1;
		return;
	}

	char sub[100] = {0};
	strncpy(sub, filter.c_str(), 99);
	lowify(sub);

	char cell[200];

	int n_rows = row_count();
	int n_cols = column_count();

	filtered = 0;
	for (int i = 0; i < n_cols; i++) {
		ColumnType type = headers[i].type;
		if (headers[i].type == ColumnString || headers[i].type == ColumnFile) {
			for (int j = 0; j < n_rows; j++) {
				char *name = type == ColumnFile ? ((File_Entry*)columns[i][j])->name : (char*)columns[i][j];
				if (!name)
					continue;

				strncpy(cell, name, 199);
				lowify(cell);

				if (strstr(cell, sub)) {
					auto res = list.insert(j);
					filtered += res.second ? 1 : 0;
				}
			}
		}
	}
}

void Table::update_tree(std::vector<Branch> *new_branches) {
	if (new_branches) {
		branches.resize(new_branches->size());
		std::copy(new_branches->begin(), new_branches->end(), branches.begin());
	}

	int n_rows = row_count();
	int n_branches = branches.size();
	tree.clear();
	tree.reserve(n_rows);

	int b = 0;
	for (int i = 0; i < n_rows; i++) {
		if (b < n_branches && branches[b].row_idx == i) {
			if (branches[b].closed)
				i = branches[b].row_idx + branches[b].length;

			tree.push_back(-b - 1);
			b++;
			i--;
			continue;
		}

		tree.push_back(i);
	}
}

int Table::get_table_index(int view_idx) {
	if (view_idx < 0)
		return view_idx;

	int idx = -1;
	int tree_size = tree.size();

	if (tree_size > 0) {
		if (view_idx < tree_size)
			idx = tree[view_idx];
	}
	else if (view_idx < row_count())
		idx = view_idx;

	return idx;
}

void Table::release() {
	int n_cols = column_count();
	for (int i = 0; i < n_cols; i++) {
		if (headers[i].type != ColumnImage)
			continue;

		for (auto& t : columns[i])
			sdl_destroy_texture(&t);
	}
}

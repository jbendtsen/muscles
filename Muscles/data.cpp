#include <numeric>
#include "muscles.h"

void *Arena::allocate(int size) {
	if (size <= 0 || size > pool_size)
		return nullptr;

	if (idx + size > pool_size)
		add_pool();

	void *ptr = (void*)&pools.back()[idx];
	idx += size;
	return ptr;
}

void *Arena::alloc_cell(Column& info) {
	int size = 0;
	if (info.type == String)
		size = 1;

	size *= info.count_per_cell;
	return allocate(size);
}

char *Arena::alloc_string(char *str) {
	if (!str)
		return nullptr;

	int len = strlen(str) + 1;
	char *p = (char*)allocate(len);
	if (p)
		strcpy(p, str);

	return p;
}

Arena arena;

Arena *get_arena() {
	return &arena;
}

Table::Table(const Table& t) :
	headers(t.headers)
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
	columns(std::move(t.columns))
{}

void Table::resize(int n_rows) {
	int n_cols = column_count();
	for (int i = 0; i < n_cols; i++) {
		columns[i].resize(n_rows, nullptr);

		if (headers[i].type == String && headers[i].count_per_cell > 0) {
			for (int j = 0; j < n_rows; j++) {
				if (columns[i][j])
					continue;

				columns[i][j] = arena.allocate(headers[i].count_per_cell);
				((char*)(columns[i][j]))[0] = 0;
			}
		}
	}
}

void Table::init(Column *headers, int n_cols, int n_rows) {
	this->headers.resize(n_cols);
	memcpy(this->headers.data(), headers, n_cols * sizeof(Column));

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
	for (int i = 0; i < str[i]; i++)
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
		Type type = headers[i].type;
		if (headers[i].type == String || headers[i].type == File) {
			for (int j = 0; j < n_rows; j++) {
				char *name = type == File ? ((File_Entry*)columns[i][j])->name : (char*)columns[i][j];
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

void Table::release() {
	int n_cols = column_count();
	for (int i = 0; i < n_cols; i++) {
		if (headers[i].type != Tex)
			continue;

		for (auto& t : columns[i])
			sdl_destroy_texture(&t);
	}
}
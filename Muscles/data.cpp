#include <numeric>
#include "muscles.h"

Arena default_arena;

Arena *get_default_arena() {
	return &default_arena;
}

Field_Vector::Field_Vector() {
	data = new Field[pool_size];
}
Field_Vector::~Field_Vector() {
	delete[] data;
}

Field& Field_Vector::back() {
	return data[n_fields-1];
}

// increase pool size by a power of two each time a pool expansion is necessary
void Field_Vector::expand() {
	int old_size = pool_size;
	pool_size *= 2;
	Field *next = new Field[pool_size];
	memcpy(next, data, old_size * sizeof(Field));
	delete[] data;
	data = next;
}

Field& Field_Vector::add(Field& f) {
	if (n_fields >= pool_size)
		expand();
	data[n_fields++] = f;
	return back();
}

Field& Field_Vector::add_blank() {
	if (n_fields >= pool_size)
		expand();

	Field& f = data[n_fields];
	f.field_name_idx = f.type_name_idx = -1;
	n_fields++;
	return f;
}

void Field_Vector::cancel_latest() {
	if (n_fields <= 0)
		return;

	data[n_fields] = {0};
	n_fields--;
}

void Field_Vector::zero_out() {
	memset(data, 0, pool_size * sizeof(Field));
	n_fields = 0;
}

void String_Vector::try_expand(int new_size) {
	if (!new_size)
		new_size = pool_size * 2;
	else {
		if (new_size <= pool_size)
			return;

		new_size = next_power_of_2(new_size);
	}

	char *new_pool = new char[new_size];

	if (pool) {
		memcpy(new_pool, pool, pool_size);
		delete[] pool;
	}

	pool = new_pool;
	pool_size = new_size;
}

int String_Vector::add_string(const char *str) {
	if (!str)
		return -1;

	int cur = head;
	int len = strlen(str);
	if (len > 0) {
		head += len + 1;
		try_expand(head);
		strcpy(&pool[cur], str);
	}
	return cur;
}

int String_Vector::add_buffer(const char *buf, int size) {
	if (!buf)
		return -1;

	int cur = head;
	if (size > 0) {
		head += size + 1;
		try_expand(head);
		memcpy(&pool[cur], buf, size);
		pool[cur+size] = 0;
	}
	return cur;
}

char *String_Vector::allocate(int size) {
	int cur = head;
	head += size + 1;
	try_expand(head);
	return &pool[cur];
}

void String_Vector::append_extra_zero() {
	try_expand(++head);
	pool[head-1] = 0;
}

void *Arena::allocate(int size) {
	if (size <= 0 || size > pool_size)
		return nullptr;

	if (idx + size > pool_size || pools.size() == 0)
		add_pool();

	void *ptr = (void*)&pools.back()[idx];
	idx += size;
	return ptr;
}

char *Arena::alloc_string(char *str) {
	if (!str)
		return nullptr;

	int len = strlen(str) + 1;
	char *p = (char*)allocate(len);
	if (p) {
		if (len > 1)
			strcpy(p, str);
		else
			*p = 0;
	}

	return p;
}

void Arena::rewind() {
	if (rewind_pool < 0 || rewind_idx < 0)
		return;

	int last = pools.size() - 1;
	if (last < 0)
		return;

	if (rewind_pool != last) {
		rewind_pool = last;
		rewind_idx = 0;
	}

	int delta = idx - rewind_idx;
	if (delta <= 0)
		return;

	idx = rewind_idx;
	memset(&pools.back()[idx], 0, delta);
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
	Arena *arena = use_default_arena ? get_default_arena() : &this->arena;
	int n_cols = column_count();

	for (int i = 0; i < n_cols; i++) {
		columns[i].resize(n_rows, nullptr);

		if (headers[i].type == ColumnString && headers[i].count_per_cell > 0) {
			for (int j = 0; j < n_rows; j++) {
				if (columns[i][j])
					continue;

				columns[i][j] = arena->allocate(headers[i].count_per_cell);
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
		Column_Type type = headers[i].type;
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

void Table::release() {
	int n_cols = column_count();
	for (int i = 0; i < n_cols; i++) {
		if (headers[i].type != ColumnImage)
			continue;

		for (auto& t : columns[i])
			sdl_destroy_texture(&t);
	}
}
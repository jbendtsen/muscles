#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed long long s64;

union Value64 {
	s64 i;
	double d;
};

struct Pair_Int {
	int first;
	int second;
};

void clear_word(char* word);
char *advance_word(char *p);

Pair_Int next_power_of_2(int num);

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

Arena& get_default_arena();

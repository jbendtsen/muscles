#include <vector>
#include <cstring>

#include "containers.h"
#include "structs.h"

Arena default_arena;

Arena& get_default_arena() {
	return default_arena;
}

void clear_word(char* word) {
	int len = strlen(word);
	memset(word, 0xff, len);
}

char *advance_word(char *p) {
	do {
		p += strlen(p) + 1;
	} while (*(u8*)p == 0xff);
	return p;
}

Pair_Int next_power_of_2(int num) {
	int power = 2;
	int exp = 1;
	while (power < num) {
		power *= 2;
		exp++;
	}

	return {
		.first = power,
		.second = exp
	};
}

char *String_Vector::at(int idx) {
	return idx < 0 || idx >= pool_size ? nullptr : &pool[idx];
}

void String_Vector::try_expand(int new_size) {
	if (!new_size)
		new_size = pool_size * 2;
	else {
		if (pool && new_size <= pool_size)
			return;

		new_size = next_power_of_2(new_size).first;
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

void String_Vector::clear() {
	if (pool) memset(pool, 0, pool_size);
	head = 0;
}

void *Arena::allocate(int size, int align) {
	if (size <= 0)
		return nullptr;

	if (size > pool_size)
		return add_big_pool(size);

	if (align > 1)
		idx += (align - (idx % align)) % align;

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

void *Arena::store_buffer(void *buf, int size, u64 align) {
	if (align < 1) align = 1;

	void *ptr = nullptr;
	int max_size = size + sizeof(int) + align - 1;

	if (max_size > pool_size)
		ptr = add_big_pool(max_size);
	else {
		if (idx + max_size > pool_size || pools.size() == 0)
			add_pool();

		ptr = &pools.back()[idx];
		idx += max_size;
	}

	// We add sizeof(int) here so that we have room to store the size of the buffer (just before the actual buffer).
	// The contents of the buffer are what should be aligned, not necessarily the size word that precedes it.
	u64 addr = (u64)ptr + sizeof(int);
	int diff = ((align - (int)(addr % (u64)align)) % align);
	ptr = (void*)((char*)ptr + diff); // ptr += diff;

	*(int*)ptr = size;
	memcpy(&((int*)ptr)[1], buf, size); // memcpy(ptr + sizeof(int), buf, size);
	return ptr;
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

#define ROTL32(x, y) ((x) << (y)) | ((x) >> (32 - (y)))

u32 murmur3_32(const char* key, int len, u32 seed) {
	u32 c1 = 0xcc9e2d51;
	u32 c2 = 0x1b873593;
	u32 h = seed;
	const u32 *p = (const u32*)key;
	const u8 *tail = (const u8*)&p[len / sizeof(u32)];

	for (int i = 0; i < (len & ~3); i += sizeof(u32)) {
		u32 k = *p++;
		k *= c1;
		k = ROTL32(k, 15);
		k *= c2;

		h ^= k;
		h = ROTL32(h, 13);
		h = h * 5 + 0xe6546b64;
	}

	u32 k = 0;
	for (int i = len & 3; i > 0; i--) {
		k <<= 8;
		k |= tail[i - 1];
	}
	if (k) {
		k *= c1;
		k = ROTL32(k, 15);
		k *= c2;
		h ^= k;
	}

	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

// Returns the corresponding bucket for a key, where the bucket is either empty or occupied (with collision resolution if occupied).
int Map::get(const char *str, int len) {
	if (len <= 0) len = strlen(str);
	u32 hash = murmur3_32(str, len, seed);

	int mask = (1 << log2_slots) - 1;
	int idx = hash & mask;

	// If the bucket is not occupied, return it immediately.
	// Don't assume that the caller wants anything to happen to the bucket if it's not in use.
	Bucket *buck = &data[idx];
	if ((buck->flags & FLAG_OCCUPIED) == 0)
		return idx;

	// If the bucket is occupied, check that it's not a collision by comparing the keys, starting from this bucket onwards.
	// If at any point we find a bucket that is not occupied, we immediately return that bucket.
	// Until then, given that we are looking at occupied buckets, if we find a matching key we return that bucket.
	int end = (idx + mask) & mask;
	while (idx != end && (data[idx].flags & FLAG_OCCUPIED)) {
		if (!memcmp(str, sv->at(data[idx].name_idx), len))
			break;

		idx = (idx + 1) & mask;
	}

	data[idx].flags &= ~FLAG_NEW;
	return idx;
}

Bucket& Map::insert(const char *str, int len) {
	next_level_maybe();

	if (len <= 0) len = strlen(str);
	int idx = get(str, len);

	Bucket& buck = data[idx];
	if ((buck.flags & FLAG_OCCUPIED) == 0) {
		buck.name_idx = sv->add_buffer(str, len);
		buck.flags |= FLAG_NEW;
		n_entries++;
	}

	buck.flags |= FLAG_OCCUPIED;
	return buck;
}

void Map::remove(const char *str, int len) {
	Bucket& buck = data[get(str, len)];
	if (buck.flags & FLAG_OCCUPIED)
		n_entries--;

	buck = {0};
}

void Map::erase_all_of_type(u32 flags) {
	int size = 1 << log2_slots;
	for (int i = 0; i < size; i++) {
		if ((data[i].flags & flags) == flags) {
			if (data[i].flags & FLAG_OCCUPIED)
				n_entries--;

			data[i] = {0};
		}
	}
}
void Map::erase_all_of_exact_type(u32 flags) {
	int size = 1 << log2_slots;
	for (int i = 0; i < size; i++) {
		if (data[i].flags == flags) {
			if (data[i].flags & FLAG_OCCUPIED)
				n_entries--;

			data[i] = {0};
		}
	}
}
void Map::erase_all_similar_to(u32 flags) {
	int size = 1 << log2_slots;
	for (int i = 0; i < size; i++) {
		if (data[i].flags & flags) {
			if (data[i].flags & FLAG_OCCUPIED)
				n_entries--;

			data[i] = {0};
		}
	}
}

void Map::next_level_maybe() {
	int n_slots = 1 << log2_slots;
	float load = (float)n_entries / (float)n_slots;
	if (load >= max_load)
		next_level();
}

void Map::next_level() {
	int old_n_slots = 1 << log2_slots;
	log2_slots++;

	int new_n_slots = 1 << log2_slots;
	int new_mask = new_n_slots - 1;
	Bucket *new_buf = new Bucket[new_n_slots]();

	for (int i = 0; i < old_n_slots; i++) {
		if ((data[i].flags & FLAG_OCCUPIED) == 0)
			continue;

		const char *str = sv->at(data[i].name_idx);
		int len = strlen(str);
		u32 hash = murmur3_32(str, len, seed);
		int idx = hash & new_mask;

		int end = (idx + new_mask) & new_mask;
		while (idx != end && (new_buf[idx].flags & FLAG_OCCUPIED))
			idx = (idx + 1) & new_mask;

		new_buf[idx] = data[i];
	}

	delete[] data;
	data = new_buf;
}

void Map::release() {
	int n_slots = 1 << log2_slots;
	for (int i = 0; i < n_slots; i++)
		data[i].flags = 0;

	sv->head = 0;
	n_entries = 0;
}

#include "muscles.h"

#define ROTL32(x, y) ((x) << (y)) | ((x) >> (32 - (y)))

u32 murmur3_32(const char* key, int len, u32 seed) {
	u32 c1 = 0xcc9e2d51;
	u32 c2 = 0x1b873593;
	u32 h = seed;
	const u32 *p = (const u32*)key;

	for (int i = 0; i < (len & ~3); i += sizeof(u32)) {
		u32 k = *p++;
		k *= c1;
		k = ROTL32(k, 15);
		k *= c2;

		h ^= k;
		h = ROTL32(h, 13);
		h = h * 5 + 0xe6546b64;
	}

	p--;
	const u8 *tail = (const u8*)p;
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

int Region_Map::get(u64 key) {
	int mask = (1 << log2_slots) - 1;
	u64 *ptr = &key;
	u32 hash = murmur3_32((const char*)ptr, sizeof(u64), seed);
	int idx = hash & mask;

	Region *reg = &data[idx];
	if ((reg->flags & FLAG_OCCUPIED) == 0)
		return idx;
		
	int end = (idx + mask) & mask;
	while (idx != end && (data[idx].flags & FLAG_OCCUPIED) && data[idx].base != key)
		idx = (idx + 1) & mask;

	return idx;
}

void Region_Map::insert(u64 key, Region& reg) {
	next_level_maybe();

	int n_slots = 1 << log2_slots;
	int idx = get(key);
/*
	if (data[idx].flags & FLAG_OCCUPIED) {
		int mask = n_slots - 1;
		int end = (idx + mask) & mask;
		while (idx != end && (data[idx].flags & FLAG_OCCUPIED))
			idx = (idx + 1) & mask;
	}
*/
	place_at(idx, reg);
}

void Region_Map::clear(int new_size) {
	delete[] data;
	n_entries = 0;

	auto size = next_power_of_2(new_size - 1);
	data = new Region[size.first]();
	log2_slots = size.second;
}

// This function assumes that you've already found an available (non-occupied) slot in the map
void Region_Map::place_at(int idx, Region& reg) {
	data[idx] = reg;
	data[idx].flags |= FLAG_OCCUPIED;
	n_entries++;
}
void Region_Map::place_at(int idx, Region&& reg) {
	data[idx] = reg;
	data[idx].flags |= FLAG_OCCUPIED;
	n_entries++;
}

void Region_Map::next_level_maybe() {
	int n_slots = 1 << log2_slots;
	float load = (float)n_entries / (float)n_slots;
	if (load >= max_load)
		next_level();
}

void Region_Map::next_level() {
	int old_n_slots = 1 << log2_slots;
	log2_slots++;

	int new_n_slots = 1 << log2_slots;
	int new_mask = new_n_slots - 1;
	Region *new_regions = new Region[new_n_slots]();

	for (int i = 0; i < old_n_slots; i++) {
		if ((data[i].flags & FLAG_OCCUPIED) == 0)
			continue;

		u64 *ptr = &data[i].base;
		u32 hash = murmur3_32((const char*)ptr, sizeof(u64), seed);
		int idx = hash & new_mask;
		int end = (idx + new_n_slots - 1) & new_mask;

		while (idx != end && (new_regions[idx].flags & FLAG_OCCUPIED))
			idx = (idx + 1) & new_mask;

		new_regions[idx] = data[i];
		//new_regions[idx].flags |= FLAG_OCCUPIED;
	}

	delete[] data;
	data = new_regions;
}

int Map::get(const char *str, int len) {
	if (len <= 0) len = strlen(str);
	u32 hash = murmur3_32(str, len, seed);

	int mask = (1 << log2_slots) - 1;
	int idx = hash & mask;

	Bucket *buck = &data[idx];
	if ((buck->flags & FLAG_OCCUPIED) == 0)
		return idx;
		
	int end = (idx + mask) & mask;
	while (idx != end && (data[idx].flags & FLAG_OCCUPIED)) {
		if (!memcmp(str, sv->at(data[idx].name_idx), len))
			break;

		idx = (idx + 1) & mask;
	}

	return idx;
}

void Map::insert(const char *str, int len, u64 value) {
	next_level_maybe();

	if (len <= 0) len = strlen(str);
	place_at(get(str, len), str, len, value);
}
void Map::insert(const char *str, int len, void *pointer) {
	next_level_maybe();

	if (len <= 0) len = strlen(str);
	place_at(get(str, len), str, len, pointer);
}

void Map::assign(Bucket& buck, u64 value) {
	buck.value = value;
	buck.flags &= ~FLAG_POINTER;

	if ((buck.flags & FLAG_OCCUPIED) == 0) {
		buck.flags |= FLAG_OCCUPIED;
		n_entries++;
	}
}
void Map::assign(Bucket& buck, void *pointer) {
	buck.pointer = pointer;
	buck.flags |= FLAG_POINTER;

	if ((buck.flags & FLAG_OCCUPIED) == 0) {
		buck.flags |= FLAG_OCCUPIED;
		n_entries++;
	}
}

void Map::place_at(int idx, const char *str, int len, u64 value) {
	int real_len = strlen(str);
	if (len <= 0)
		len = real_len;
	else
		len = real_len < len ? real_len : len;

	data[idx].flags |= FLAG_OCCUPIED;
	data[idx].flags &= ~FLAG_POINTER;
	data[idx].name_idx = sv->add_buffer(str, len);
	data[idx].value = value;
	n_entries++;
}
void Map::place_at(int idx, const char *str, int len, void *pointer) {
	int real_len = strlen(str);
	if (len <= 0)
		len = real_len;
	else
		len = real_len < len ? real_len : len;

	data[idx].flags |= FLAG_OCCUPIED | FLAG_POINTER;
	data[idx].name_idx = sv->add_buffer(str, len);
	data[idx].pointer = pointer;
	n_entries++;
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
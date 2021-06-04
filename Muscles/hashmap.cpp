#include "muscles.h"

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

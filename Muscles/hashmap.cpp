#include "muscles.h"

#define ROTL32(x, y) ((x) << (y)) | ((x) >> (32 - (y)))

u32 murmur3_32(const char* key, int len, u32 seed) {
	u32 c1 = 0xcc9e2d51;
	u32 c2 = 0x1b873593;
	u32 h = seed;
	const u32 *p = (const u32*)key;

	for (int i = 0; i < len; i += sizeof(u32)) {
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

	if (data[idx].flags & FLAG_OCCUPIED) {
		int mask = n_slots - 1;
		int end = (idx + mask) & mask;
		while (idx != end && (data[idx].flags & FLAG_OCCUPIED))
			idx = (idx + 1) & mask;
	}

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
		new_regions[idx].flags |= FLAG_OCCUPIED;
	}

	delete[] data;
	data = new_regions;
}
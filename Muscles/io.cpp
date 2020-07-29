#include "muscles.h"

int Source::reset_span(int old_idx, u64 offset, int size) {
	if (old_idx >= 0 && spans[old_idx].offset == offset && spans[old_idx].size == size)
		return old_idx;

	int idx = -1;
	for (int i = 0; i < spans.size(); i++) {
		if (spans[i].offset != offset)
			continue;

		if (spans[i].cache && spans[i].size < size) {
			u8 *cache = new u8[size]();
			memcpy(cache, spans[i].cache, spans[i].size);
			delete[] spans[i].cache;
			spans[i].cache = cache;
		}

		idx = i;
		break;
	}
	if (idx < 0) {
		idx = spans.size();
		spans.push_back({});
	}

	if (!spans[idx].cache && size > 0)
		spans[idx].cache = new u8[size]();

	spans[idx].offset = offset;
	spans[idx].size = size;
	if (idx != old_idx)
		spans[idx].clients++;

	return idx;
}

int Source::set_offset(int idx, u64 offset) {
	if (spans[idx].clients <= 1) {
		spans[idx].offset = offset;
		return idx;
	}

	spans[idx].clients--;
	int size = spans[idx].size;

	idx = spans.size();
	spans.push_back({});

	spans[idx].offset = offset;
	spans[idx].size = size;
	spans[idx].clients = 1;
	spans[idx].cache = new u8[size]();

	return idx;
}

void Source::delete_span(int idx) {
	spans[idx].clients--;
	if (spans[idx].clients <= 0) {
		delete[] spans[idx].cache;
		spans[idx].cache = nullptr;
	}
}
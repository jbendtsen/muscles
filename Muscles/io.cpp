#include <algorithm>
#include <numeric>
#include "muscles.h"

int Source::request_span(void) {
	int idx = -1;
	for (int i = 0; i < spans.size(); i++) {
		if (spans[i].flags & FLAG_AVAILABLE) {
			spans[i] = {0};
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		idx = spans.size();
		spans.push_back({0});
	}

	return idx;
}

void Source::deactivate_span(int idx) {
	if (idx >= 0 && idx < spans.size()) {
		spans[idx].flags |= FLAG_AVAILABLE;
		spans[idx].size = 0;
	}
}

void Source::gather_data(Arena& arena) {
	if (spans.size() < 1)
		return;

	std::vector<int> index(spans.size());
	std::iota(index.begin(), index.end(), 0);
	std::sort(index.begin(), index.end(), [this](int a, int b) {
		return this->spans[a].address < this->spans[b].address;
	});

	std::vector<Span> io = {{0}};
	io[0].address = spans[index[0]].address;
	io[0].size = spans[index[0]].size;
	spans[index[0]].offset = 0;

	for (int i = 1; i < spans.size(); i++) {
		Span& virt = spans[index[i]];
		if ((virt.flags & FLAG_AVAILABLE) || virt.size <= 0)
			continue;

		Span& real = io.back();

		if (virt.address <= real.address + (u64)real.size) {
			virt.offset = virt.address - real.address;
			int size = virt.offset + virt.size;
			real.size = size > real.size ? size : real.size;
		}
		else {
			int offset = real.offset + real.size;
			virt.offset = offset;

			io.push_back({
				virt.address,
				virt.size,
				0,
				virt.offset,
				0,
				nullptr
			});
		}
	}

	auto& end = io.back();
	int size = end.offset + end.size;

	if (!size) {
		if (buffer)
			delete[] buffer;

		buffer = nullptr;
		buf_size = 0;
		return;
	}
	else if (size != buf_size) {
		if (!buffer)
			buffer = new u8[size];
		else if (size > buf_size) {
			if (buffer)
				delete[] buffer;

			buffer = new u8[size];
		}
	}

	buf_size = size;

	if (type == SourceFile)
		refresh_file_spans(*this, io, arena);
	else if (type == SourceProcess)
		refresh_process_spans(*this, io, arena);

	int real_idx = 0;
	int n_spans = io.size();
	for (auto& idx : index) {
		auto& s = spans[idx];
		s.data = &buffer[s.offset];

		while (real_idx < n_spans && !(s.offset >= io[real_idx].offset && s.offset < io[real_idx].offset + io[real_idx].size))
			real_idx++;
		if (real_idx >= n_spans)
			real_idx = n_spans-1;

		s.retrieved = io[real_idx].retrieved - s.offset;
		if (s.retrieved < 0) s.retrieved = 0;
	}
}
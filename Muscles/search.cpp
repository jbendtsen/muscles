#include "muscles.h"
#include "structs.h"
#include "search.h"

#include <cstdint>
#include <algorithm>

static void *thread = nullptr;
static bool started = false;
static bool running = false;

static u64 *results = nullptr;
static int n_results = 0;

static u64 *prev_results = nullptr;
static int n_prev_results = 0;

static Search search;
static std::vector<std::pair<u64, u64>> ranges;

void perform_search();

void start_search(Search& s, std::vector<Region> const& regions) {
	if (running)
		return;

	ranges.resize(0);
	for (auto& reg : regions)
		ranges.push_back(std::make_pair<u64, u64>((u64)reg.base, (u64)reg.size));

	std::sort(ranges.begin(), ranges.end(), [](std::pair<u64, u64>& a, std::pair<u64, u64>& b) {
		return a.first < b.first;
	});

	search.params = s.params;
	if (search.params) {
		search.n_params = s.n_params;
		search.record = s.record;
	}
	else
		search.single_value = s.single_value;

	search.byte_align = s.byte_align;

	search.start_addr = s.start_addr;
	search.end_addr = s.end_addr;

	search.source_type = s.source_type;
	search.pid = s.pid;
	search.identifier = s.identifier;

	auto func = [](void *data) {
		perform_search();
		return (THREAD_RETURN_TYPE)0;
	};
	start_thread(&thread, nullptr, func);
}

bool check_search_running() {
	return running;
}

bool check_search_finished() {
	bool finished = started && !running;
	if (finished)
		started = false;

	return finished;
}

void get_search_results(std::vector<u64>& results_vec) {
	if (!results)
		return;

	results_vec.resize(n_results);
	if (n_results)
		memcpy(results_vec.data(), results, n_results * sizeof(u64));
}

void reset_search() {
	n_results = 0;
}

void exit_search() {
	if (results) {
		delete[] results;
		results = nullptr;
	}
	if (prev_results) {
		delete[] prev_results;
		prev_results = nullptr;
	}
}

struct Scan_Range {
	u64 start;
	int first_range;
	int last_range;
};

Scan_Range isolate_scan_ranges() {
	Scan_Range scan = {
		.start = search.start_addr,
		.first_range = -1,
		.last_range = -1
	};

	for (auto& r : ranges) {
		scan.first_range++;
		if (scan.start < r.first + r.second) {
			if (scan.start < r.first)
				scan.start = r.first;
			break;
		}
	}
	for (auto& r : ranges) {
		if (search.end_addr < r.first)
			break;

		scan.last_range++;
	}

	return scan;
}

template <int method, typename T>
void single_value_search(SOURCE_HANDLE handle, T v1, T v2) {
	int byte_align = search.byte_align;
	if (byte_align <= 0)
		byte_align = sizeof(T);

	char *buf = new char[PAGE_SIZE]();

	if (n_results == 0) {
		auto scan = isolate_scan_ranges();

		u64 addr = scan.start;
		for (int i = scan.first_range; i <= scan.last_range && addr <= search.end_addr; i++) {
			u64 range_end = ranges[i].first + ranges[i].second;
			if (search.end_addr < range_end)
				range_end = search.end_addr;

			u64 page = addr & ~(PAGE_SIZE - 1);
			int offset = (int)(addr - page) & ~(sizeof(T) - 1);

			for (; page < range_end; page += PAGE_SIZE) {
				int retrieved = read_page(handle, search.source_type, page, buf);
				if (retrieved <= 0)
					continue;

				for (int j = offset; j <= PAGE_SIZE - sizeof(T); j += byte_align) {
					T value = *(T*)(&buf[j]);
					if constexpr (method == METHOD_EQUALS) {
						if (value == v1) {
							results[n_results++] = page + (u64)j;
							if (n_results >= MAX_SEARCH_RESULTS)
								goto done_single;
						}
					}
					else {
						if (value >= v1 && value <= v2) {
							results[n_results++] = page + (u64)j;
							if (n_results >= MAX_SEARCH_RESULTS)
								goto done_single;
						}
					}
				}
			}

			if (i < scan.last_range)
				addr = ranges[i + 1].first;
		}
	}
	else {
		n_prev_results = n_results;
		if (!prev_results)
			prev_results = new u64[MAX_SEARCH_RESULTS];

		memcpy(prev_results, results, n_prev_results * sizeof(u64));
		n_results = 0;

		u64 page = 0;
		bool fail = false;

		for (int i = 0; i < n_prev_results && n_results < MAX_SEARCH_RESULTS; i++) {
			u64 addr = prev_results[i];
			u64 p = addr & ~(PAGE_SIZE - 1);
			int offset = (int)(addr - p);//(int)(addr & (u64)(PAGE_SIZE - 1));

			if (i == 0 || p > page) {
				page = p;
				int retrieved = read_page(handle, search.source_type, page, buf);
				fail = retrieved <= 0;
			}
			if (!fail) {
				T value = *(T*)(&buf[offset]);
				if constexpr (method == METHOD_EQUALS) {
					if (value == v1)
						results[n_results++] = addr;
				}
				else {
					if (value >= v1 && value <= v2)
						results[n_results++] = addr;
				}
			}
		}
	}

done_single:
	delete[] buf;
}

// DRY: Do Repeat Yourself
void do_single_value_search(SOURCE_HANDLE handle) {
	Search_Parameter sv = search.single_value;
	u32 flags = sv.flags & FIELD_FLAGS;

	if (sv.method == METHOD_EQUALS) {
		if (flags & FLAG_FLOAT) {
			if (sv.size == 32)
				single_value_search<METHOD_EQUALS, float>(handle, *(float*)&sv.value1, 0);
			else if (sv.size == 64)
				single_value_search<METHOD_EQUALS, double>(handle, *(double*)&sv.value1, 0);
		}
		else if (flags & FLAG_SIGNED) {
			if (sv.size == 8)
				single_value_search<METHOD_EQUALS, std::int8_t>(handle, *(std::int8_t*)&sv.value1, 0);
			else if (sv.size == 16)
				single_value_search<METHOD_EQUALS, std::int16_t>(handle, *(std::int16_t*)&sv.value1, 0);
			else if (sv.size == 32)
				single_value_search<METHOD_EQUALS, std::int32_t>(handle, *(std::int32_t*)&sv.value1, 0);
			else
				single_value_search<METHOD_EQUALS, std::int64_t>(handle, *(std::int64_t*)&sv.value1, 0);
		}
		else {
			if (sv.size == 8)
				single_value_search<METHOD_EQUALS, std::uint8_t>(handle, *(std::uint8_t*)&sv.value1, 0);
			else if (sv.size == 16)
				single_value_search<METHOD_EQUALS, std::uint16_t>(handle, *(std::uint16_t*)&sv.value1, 0);
			else if (sv.size == 32)
				single_value_search<METHOD_EQUALS, std::uint32_t>(handle, *(std::uint32_t*)&sv.value1, 0);
			else
				single_value_search<METHOD_EQUALS, std::uint64_t>(handle, *(std::uint64_t*)&sv.value1, 0);
		}
	}
	else if (sv.method == METHOD_RANGE) {
		if (flags & FLAG_FLOAT) {
			if (sv.size == 32)
				single_value_search<METHOD_RANGE, float>(handle, *(float*)&sv.value1, *(float*)&sv.value2);
			else if (sv.size == 64)
				single_value_search<METHOD_RANGE, double>(handle, *(double*)&sv.value1, *(double*)&sv.value2);
		}
		else if (flags & FLAG_SIGNED) {
			if (sv.size == 8)
				single_value_search<METHOD_RANGE, std::int8_t>(handle, *(std::int8_t*)&sv.value1, *(std::int8_t*)&sv.value2);
			else if (sv.size == 16)
				single_value_search<METHOD_RANGE, std::int16_t>(handle, *(std::int16_t*)&sv.value1, *(std::int16_t*)&sv.value2);
			else if (sv.size == 32)
				single_value_search<METHOD_RANGE, std::int32_t>(handle, *(std::int32_t*)&sv.value1, *(std::int32_t*)&sv.value2);
			else
				single_value_search<METHOD_RANGE, std::int64_t>(handle, *(std::int64_t*)&sv.value1, *(std::int64_t*)&sv.value2);
		}
		else {
			if (sv.size == 8)
				single_value_search<METHOD_RANGE, std::uint8_t>(handle, *(std::uint8_t*)&sv.value1, *(std::uint8_t*)&sv.value2);
			else if (sv.size == 16)
				single_value_search<METHOD_RANGE, std::uint16_t>(handle, *(std::uint16_t*)&sv.value1, *(std::uint16_t*)&sv.value2);
			else if (sv.size == 32)
				single_value_search<METHOD_RANGE, std::uint32_t>(handle, *(std::uint32_t*)&sv.value1, *(std::uint32_t*)&sv.value2);
			else
				single_value_search<METHOD_RANGE, std::uint64_t>(handle, *(std::uint64_t*)&sv.value1, *(std::uint64_t*)&sv.value2);
		}
	}
}

template<typename T>
s64 roundtrip_cast(s64 in) {
	T value = (T)in;
	return (s64)value;
}

s64 cast(u32 flags, int size, s64 in) {
	s64 out = 0;

	if (flags & FLAG_FLOAT) {
		out = in;
	}
	else if (flags & FLAG_SIGNED) {
		if (size == 8)
			out = roundtrip_cast<std::int8_t>(in);
		else if (size == 16)
			out = roundtrip_cast<std::int16_t>(in);
		else if (size == 32)
			out = roundtrip_cast<std::int32_t>(in);
		else if (size == 64)
			out = roundtrip_cast<std::int64_t>(in);
		else
			out = in;
	}
	else {
		if (size == 8)
			out = roundtrip_cast<std::uint8_t>(in);
		else if (size == 16)
			out = roundtrip_cast<std::uint16_t>(in);
		else if (size == 32)
			out = roundtrip_cast<std::uint32_t>(in);
		else if (size == 64)
			out = roundtrip_cast<std::uint64_t>(in);
		else
			out = in;
	}

	return out;
}

// TODO: Support both endians, bitfields, arrays (including string literals)
void do_object_search(SOURCE_HANDLE handle) {
	int byte_inc = search.byte_align;
	if (byte_inc <= 0)
		byte_inc = search.record->total_size / 8;

	int n_params = search.n_params;

	char *page_attrs_buf = new char[n_params * (2 * sizeof(s64) + sizeof(u64) + sizeof(int))]();

	s64 *values      = reinterpret_cast<s64*>(page_attrs_buf);
	u64 *page_addrs  = reinterpret_cast<u64*>(page_attrs_buf + n_params * 2 * sizeof(s64));
	int *page_idxs   = reinterpret_cast<int*>(page_attrs_buf + n_params * (2 * sizeof(s64) + sizeof(u64)));

	char *page_mem = new char[(n_params + 1) * PAGE_SIZE];

	// totally unnecessary but totally swag
	char *pages = reinterpret_cast<char*>((reinterpret_cast<u64>(page_mem) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

	for (int i = 0; i < n_params; i++) {
		page_addrs[i] = -1;

		Search_Parameter& p = search.params[i];
		values[i*2]   = cast(p.flags, p.size, p.value1);
		values[i*2+1] = cast(p.flags, p.size, p.value2);
	}

	auto search_all_parameters = [&](u64 head_address) {
		int matches = 0;

		for (int j = 0; j < n_params; j++) {
			u64 byte_offset = (u64)(search.params[j].offset / 8);
			u64 addr = head_address + byte_offset;
			u64 page = addr & ~(PAGE_SIZE - 1);
			int page_offset = (int)(addr - page);

			if (page != page_addrs[j]) {
				int p = 0;
				for (; p < n_params; p++) {
					if (page_addrs[p] == page)
						break;
				}

				if (p >= n_params) {
					int retrieved = read_page(handle, search.source_type, page, &pages[j * PAGE_SIZE]);
					if (retrieved <= 0)
						return -1;

					p = j;
				}

				page_addrs[j] = page;
				page_idxs[j] = p;
			}

			char *buf = &pages[page_idxs[j] * PAGE_SIZE];
			int byte_size = search.params[j].size / 8;

			s64 value = 0;
			for (int k = 0; k < byte_size; k++)
				value |= (s64)(buf[page_offset + k] & 0xff) << (s64)(8*k);

			value = cast(search.params[j].flags, search.params[j].size, value);

			if (
				(search.params[j].method == METHOD_EQUALS && value == values[2*j]) || 
				(search.params[j].method == METHOD_RANGE  && value >= values[2*j] && value <= values[2*j+1])
			)
				matches++;
		}

		return matches;
	};

	if (n_results == 0) {
		auto scan = isolate_scan_ranges();

		u64 head = scan.start;
		for (int i = scan.first_range; i <= scan.last_range && head <= search.end_addr; i++) {
			u64 range_end = ranges[i].first + ranges[i].second;
			if (search.end_addr < range_end)
				range_end = search.end_addr;

			for (; head < range_end; head += byte_inc) {
				int matches = search_all_parameters(head);

				if (matches == n_params) {
					results[n_results++] = head;
					if (n_results >= MAX_SEARCH_RESULTS)
						goto done_object;
				}
			}

			if (i < scan.last_range)
				head = ranges[i + 1].first;
		}
	}
	else {
		n_prev_results = n_results;
		if (!prev_results)
			prev_results = new u64[MAX_SEARCH_RESULTS];

		memcpy(prev_results, results, n_prev_results * sizeof(u64));
		n_results = 0;

		for (int i = 0; i < n_prev_results; i++) {
			u64 head = prev_results[i];
			int matches = search_all_parameters(head);

			if (matches == n_params) {
				results[n_results++] = head;
				if (n_results >= MAX_SEARCH_RESULTS)
					goto done_object;
			}
		}
	}

done_object:
	delete[] page_attrs_buf;
	delete[] page_mem;
}

// We know the thread has ended if started == true and running == false
void perform_search() {
	started = true;
	running = true;

	if (!results)
		results = new u64[MAX_SEARCH_RESULTS];

	auto handle = (SOURCE_HANDLE)0;
	if (search.source_type == SourceFile)
		handle = get_readonly_file_handle(search.identifier);
	else if (search.source_type == SourceProcess)
		handle = get_readonly_process_handle(search.pid);

	if (!handle) {
		running = false;
		return;
	}

	if (!search.params)
		do_single_value_search(handle);
	else
		do_object_search(handle);

	running = false;
}

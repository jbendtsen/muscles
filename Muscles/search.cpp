#include "muscles.h"
#include <cstdint>
#include <algorithm>

// TODO try this on the Windows backend: https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory
// Linux (probably wisely) doesn't seem to allow mmap into /proc/<pid>/mem

void *thread = nullptr;
bool started = false;
bool running = false;

u64 *results = nullptr;
int n_results = 0;

Search search;
std::vector<std::pair<u64, u64>> ranges;

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

	search.params = nullptr;
	search.single_value = s.single_value;

	search.start_addr = s.start_addr & ~PAGE_SIZE;
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

bool check_search_finished() {
	bool finished = started && !running;
	if (finished)
		started = false;

	return finished;
}

void get_search_results(std::vector<u64>& results_vec) {
	results_vec.resize(n_results);
	if (n_results)
		memcpy(results_vec.data(), results, n_results * sizeof(u64));
}

template <int method, typename T>
void single_value_search(SOURCE_HANDLE handle, T v1, T v2) {
	char *buf = new char[PAGE_SIZE]();

	u64 start = search.start_addr;
	int first_range = -1;
	for (auto& r : ranges) {
		first_range++;
		if (start < r.first + r.second) {
			if (start < r.first)
				start = r.first;
			break;
		}
	}

	u64 end = search.end_addr;
	int last_range = -1;
	for (auto& r : ranges) {
		if (search.end_addr < r.first)
			break;

		end = r.first + r.second;
		last_range++;
	}

	/*
	char msg[200];
	snprintf(msg, 200, "start=%#llx, end=%#llx, first_range=%d, last_range=%d", start, end, first_range, last_range);
	sdl_log_string(msg);
	*/

	u64 addr = start;
	for (int i = first_range; i <= last_range; i++) {
		u64 range_end = ranges[i].first + ranges[i].second;

		for (; addr < range_end; addr += PAGE_SIZE) {
			int retrieved = read_page(handle, search.source_type, addr, buf);
			if (retrieved <= 0)
				continue;

			for (int j = 0; j < PAGE_SIZE; j += sizeof(T)) {
				T value = *(T*)(&buf[j]);
				if constexpr (method == METHOD_EQUALS) {
					if (value == v1) {
						results[n_results++] = addr + (u64)j;
						if (n_results >= MAX_SEARCH_RESULTS)
							break;
					}
				}
				else {
					if (value >= v1 && value <= v2) {
						results[n_results++] = addr + (u64)j;
						if (n_results >= MAX_SEARCH_RESULTS)
							break;
					}
				}
			}

			if (n_results >= MAX_SEARCH_RESULTS)
				break;
		}

		if (i < last_range)
			addr = ranges[i + 1].first;
	}

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

// We know the thread has ended if started == true and running == false
void perform_search() {
	started = true;
	running = true;

	n_results = 0;
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
		wait_ms(1000);

	running = false;
}
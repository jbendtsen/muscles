#include "muscles.h"
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
	if (search.params)
		search.n_params = s.n_params;
	else
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

template <int method, typename T>
void single_value_search(SOURCE_HANDLE handle, T v1, T v2) {
	char *buf = new char[PAGE_SIZE]();

	if (n_results == 0) {
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
								goto done;
						}
					}
					else {
						if (value >= v1 && value <= v2) {
							results[n_results++] = addr + (u64)j;
							if (n_results >= MAX_SEARCH_RESULTS)
								goto done;
						}
					}
				}
			}

			if (i < last_range)
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
			u64 p = addr & ~PAGE_SIZE;
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

done:
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

void do_object_search(SOURCE_HANDLE handle) {
	wait_ms(1000);
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

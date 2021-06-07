#include "muscles.h"
#include <cstdint>

// TODO try this on the Windows backend: https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory
// Linux (probably wisely) doesn't seem to allow mmap into /proc/<pid>/mem

void Search::start() {
	if (running)
		return;

	start_addr &= ~PAGE_SIZE;

	auto func = [](void *data) {
		((Search*)data)->perform_search();
		return (THREAD_RETURN_TYPE)0;
	};
	start_thread(&thread, (void*)this, func);
}

template <typename T>
void single_value_equals_search(std::vector<u64>& results, SOURCE_HANDLE handle, u64 start, u64 end, T value) {
	char *buf = new char[PAGE_SIZE]();

	for (u64 addr = start; addr < end; addr += PAGE_SIZE) {
		int retrieved = read_page(handle, addr, buf);
		if (retrieved <= 0)
			continue;

		for (int i = 0; i < PAGE_SIZE; i += sizeof(T)) {
			if (*(T*)(&buf[i]) == value)
				results.push_back(addr + (u64)i);
		}
	}

	delete[] buf;
}

template <typename T>
void single_value_range_search(std::vector<u64>& results, SOURCE_HANDLE handle, u64 start, u64 end, T first, T last) {
	char *buf = new char[PAGE_SIZE]();

	for (u64 addr = start; addr < end; addr += PAGE_SIZE) {
		int retrieved = read_page(handle, addr, buf);
		if (retrieved <= 0)
			continue;

		for (int i = 0; i < retrieved; i += sizeof(T)) {
			T value = *(T*)(&buf[i]);
			if (value >= first && value <= last)
				results.push_back(addr + (u64)i);
		}
	}

	delete[] buf;
}

// DRY: Do Repeat Yourself
void Search::single_value_search(SOURCE_HANDLE handle) {
	Search_Parameter sv = single_value;
	u32 flags = sv.flags & FIELD_FLAGS;

	if (sv.method == METHOD_EQUALS) {
		if (flags & FLAG_FLOAT) {
			if (sv.size == 32)
				single_value_equals_search<float>(results, handle, start_addr, end_addr, *(float*)sv.value1);
			else if (sv.size == 64)
				single_value_equals_search<double>(results, handle, start_addr, end_addr, *(double*)sv.value1);
		}
		else if (flags & FLAG_SIGNED) {
			if (sv.size == 8)
				single_value_equals_search<std::int8_t>(results, handle, start_addr, end_addr, *(std::int8_t*)sv.value1);
			else if (sv.size == 16)
				single_value_equals_search<std::int16_t>(results, handle, start_addr, end_addr, *(std::int16_t*)sv.value1);
			else if (sv.size == 32)
				single_value_equals_search<std::int32_t>(results, handle, start_addr, end_addr, *(std::int32_t*)sv.value1);
			else
				single_value_equals_search<std::int64_t>(results, handle, start_addr, end_addr, *(std::int64_t*)sv.value1);
		}
		else {
			if (sv.size == 8)
				single_value_equals_search<std::uint8_t>(results, handle, start_addr, end_addr, *(std::uint8_t*)sv.value1);
			else if (sv.size == 16)
				single_value_equals_search<std::uint16_t>(results, handle, start_addr, end_addr, *(std::uint16_t*)sv.value1);
			else if (sv.size == 32)
				single_value_equals_search<std::uint32_t>(results, handle, start_addr, end_addr, *(std::uint32_t*)sv.value1);
			else
				single_value_equals_search<std::uint64_t>(results, handle, start_addr, end_addr, *(std::uint64_t*)sv.value1);
		}
	}
	else if (sv.method == METHOD_RANGE) {
		if (flags & FLAG_FLOAT) {
			if (sv.size == 32)
				single_value_range_search<float>(results, handle, start_addr, end_addr, *(float*)sv.value1, *(float*)sv.value2);
			else if (sv.size == 64)
				single_value_range_search<double>(results, handle, start_addr, end_addr, *(double*)sv.value1, *(double*)sv.value2);
		}
		else if (flags & FLAG_SIGNED) {
			if (sv.size == 8)
				single_value_range_search<std::int8_t>(results, handle, start_addr, end_addr, *(std::int8_t*)sv.value1, *(std::int8_t*)sv.value2);
			else if (sv.size == 16)
				single_value_range_search<std::int16_t>(results, handle, start_addr, end_addr, *(std::int16_t*)sv.value1, *(std::int16_t*)sv.value2);
			else if (sv.size == 32)
				single_value_range_search<std::int32_t>(results, handle, start_addr, end_addr, *(std::int32_t*)sv.value1, *(std::int32_t*)sv.value2);
			else
				single_value_range_search<std::int64_t>(results, handle, start_addr, end_addr, *(std::int64_t*)sv.value1, *(std::int64_t*)sv.value2);
		}
		else {
			if (sv.size == 8)
				single_value_range_search<std::uint8_t>(results, handle, start_addr, end_addr, *(std::uint8_t*)sv.value1, *(std::uint8_t*)sv.value2);
			else if (sv.size == 16)
				single_value_range_search<std::uint16_t>(results, handle, start_addr, end_addr, *(std::uint16_t*)sv.value1, *(std::uint16_t*)sv.value2);
			else if (sv.size == 32)
				single_value_range_search<std::uint32_t>(results, handle, start_addr, end_addr, *(std::uint32_t*)sv.value1, *(std::uint32_t*)sv.value2);
			else
				single_value_range_search<std::uint64_t>(results, handle, start_addr, end_addr, *(std::uint64_t*)sv.value1, *(std::uint64_t*)sv.value2);
		}
	}
}

// We know the thread has ended if started == true and running == false
void Search::perform_search() {
	started = true;
	running = true;

	results.resize(0);

	auto handle = (SOURCE_HANDLE)0;
	if (source->type == SourceFile)
		handle = get_readonly_file_handle(source->identifier);
	else if (source->type == SourceProcess)
		handle = get_readonly_process_handle(source->pid);

	if (!handle) {
		running = false;
		return;
	}

	if (!params)
		single_value_search(handle);
	else
		wait_ms(1000);

	running = false;
}

void Search::finalize() {
	
}

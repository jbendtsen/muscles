#include "muscles.h"
#include "structs.h"

#include <algorithm>

#include <windows.h>
#include <Psapi.h>
#include <DbgHelp.h>

const char *get_folder_separator() {
	return "\\";
}
const char *get_root_folder() {
	return "C:\\";
}
const char *get_project_path() {
	return "C:/Users/Jack/source/repos/Muscles";
}

bool is_folder(const char *name) {
	WIN32_FIND_DATAA info = {0};
	HANDLE h = FindFirstFileA(name, &info);
	if (h == INVALID_HANDLE_VALUE)
		return false;

	bool folder = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	FindClose(h);
	return folder;
}

HANDLE open_file(LPCSTR name) {
	return CreateFileA(
		name,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
}

std::pair<int, std::unique_ptr<u8[]>> read_file(std::string& path) {
	std::pair<int, std::unique_ptr<u8[]>> buf = {0, nullptr};
	HANDLE h = open_file(path.c_str());
	if (!h)
		return buf;

	BY_HANDLE_FILE_INFORMATION info = {0};
	GetFileInformationByHandle(h, &info);
	buf.first = info.nFileSizeLow;
	buf.second = std::make_unique<u8[]>(buf.first);

	DWORD retrieved = 0;
	ReadFile(h, buf.second.get(), buf.first, &retrieved, nullptr);
	return buf;
}

Texture load_icon(const char *path);

void get_process_id_list(std::vector<s64>& list) {
	const int max_pids = 1024;
	auto pids = std::make_unique<DWORD[]>(max_pids);
	DWORD size = 0;
	EnumProcesses(pids.get(), max_pids * sizeof(DWORD), &size);

	int n_pids = size / sizeof(DWORD);
	list.resize(n_pids);

	for (int i = 0; i < n_pids; i++)
		list[i] = (s64)pids[i];
}

int get_process_names(std::vector<s64> *full_list, Map& icon_map, std::vector<void*>& icons, std::vector<void*>& pids, std::vector<void*>& names, int count_per_cell) {
	const int proc_path_len = 1024;
	auto proc_path = std::make_unique<char[]>(proc_path_len);

	std::vector<s64> *list = full_list ? full_list : (std::vector<s64>*)&pids;
	int size = list->size() < names.size() ? list->size() : names.size();

	int idx = 0;
	for (int i = 0; i < size; i++) {
		// Many of the PIDs provided by EnumProcesses can't be opened, so we filter them out here
		int pid = (*list)[i];
		HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, pid);
		if (!proc)
			continue;

		char *path = proc_path.get();
		// gets us the full name of the current process
		//int res = GetProcessImageFileNameA(proc, path, proc_path_len);
		int len = proc_path_len;
		int res = QueryFullProcessImageNameA(proc, 0, path, (PDWORD)&len);
		CloseHandle(proc);

		if (!res)
			continue;

		// The name in 'proc_path' is currently represented as a path to the file,
		//  so we find the last backslash and use the string from that point
		char *p = strrchr(path, '\\');
		if (!p)
			p = path;
		else
			p++;

		Bucket& buck = icon_map.insert(p);
		if (buck.flags & FLAG_NEW) {
			buck.pointer = load_icon(path);
			buck.flags |= FLAG_EXTERNAL;
		}

		icons[idx] = buck.pointer;

		s64 pid_long = (s64)pid;
		pids[idx] = (void*)pid_long;

		char *name = (char*)names[idx];
		strncpy(name, p, count_per_cell-1);
		name[strlen(p)] = 0;

		idx++;
	}

	return idx;
}

Texture load_icon(const char *path) {
	HICON icon = ExtractIconA(GetModuleHandle(nullptr), path, 0);
	if (!icon)
		return nullptr;

	ICONINFO info = {0};
	GetIconInfo(icon, &info);

	HBITMAP copy = (HBITMAP)CopyImage(info.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	BITMAP bmp;
	GetObject(copy, sizeof(BITMAP), &bmp);

	if (bmp.bmBitsPixel != 32) {
		DeleteObject(copy);
		DestroyIcon(icon);
		return nullptr;
	}

	int w = bmp.bmWidth, h = bmp.bmHeight;
	u8 *buf = new u8[w*h*4];

	for (int i = 0; i < h; i++) {
		int in_idx  = (h-i-1) * w * 4;
		int out_idx = i * w * 4;
		for (int j = 0; j < w; j++) {
			u8 *p = &((u8*)bmp.bmBits)[in_idx];
			buf[out_idx++] = p[3];
			buf[out_idx++] = p[0];
			buf[out_idx++] = p[1];
			buf[out_idx++] = p[2];
			in_idx += 4;
		}
	}

	DeleteObject(copy);
	DestroyIcon(icon);

	Texture tex = sdl_create_texture((u32*)buf, w, h);
	delete[] buf;
	return tex;
}

void enumerate_files(char *path, std::vector<File_Entry*>& files, Arena& arena) {
	int len = strlen(path);
	auto path_buf = std::make_unique<char[]>(len + 4);
	snprintf(path_buf.get(), len + 4, "%s\\*", path);

	WIN32_FIND_DATAA info = {0};
	HANDLE h = FindFirstFileA(path_buf.get(), &info);

	auto it = files.begin();
	bool increase = false;
	int size = 0;

	while (true) {
		if (!strlen(info.cFileName) || !strcmp(info.cFileName, ".") || !strcmp(info.cFileName, "..")) {
			if (!FindNextFileA(h, &info))
				break;
			continue;
		}

		if (it == files.end())
			increase = true;

		if (increase) {
			files.push_back((File_Entry*)arena.allocate(sizeof(File_Entry)));
			it = files.begin() + (files.size() - 1);
		}

		File_Entry *&file = *it;
		if (!file)
			file = (File_Entry*)arena.allocate(sizeof(File_Entry));

		bool dir = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		bool pm_read = true;
		bool pm_write = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
		bool pm_exec = true;
		file->flags = (dir << 3) | (pm_read << 2) | (pm_write << 1) | pm_exec;

		file->path = path;
		strncpy(file->name, info.cFileName, MAX_FNAME-1);

		int end = MAX_FNAME-1;
		len = strlen(info.cFileName);
		file->name[end < len ? end : len] = 0;

		size++;
		++it;
		if (!FindNextFileA(h, &info))
			break;
	}

	FindClose(h);

	files.resize(size);

	if (size > 0) {
		std::sort(files.begin(), files.end(), [](File_Entry *&a, File_Entry *&b){
			if ((a->flags & 8) != 0 && (b->flags & 8) == 0)
				return true;
			if ((a->flags & 8) == 0 && (b->flags & 8) != 0)
				return false;
			return strcmp(a->name, b->name) < 0;
		});
	}
}

void refresh_file_region(Source& source) {
	if (source.regions.size() <= 0)
		source.regions.push_back({});

	Region& reg = source.regions[0];

	if (!source.handle)
		source.handle = open_file((LPCSTR)source.identifier);

	BY_HANDLE_FILE_INFORMATION info = {0};
	GetFileInformationByHandle(source.handle, &info);
	reg.size = ((u64)info.nFileSizeHigh << 32) | info.nFileSizeLow;

	if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		reg.size = 0;

	bool pm_read = true;
	bool pm_write = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0;
	bool pm_exec = true;
	reg.flags = (pm_read << REG_PM_READ) | (pm_write << REG_PM_WRITE) | (pm_exec << REG_PM_EXEC);

	reg.base = 0;
	reg.name = (char*)source.name.c_str();
}

void refresh_file_spans(Source& source, std::vector<Span>& input) {
	if (!source.handle)
		source.handle = open_file((LPCSTR)source.identifier);

	LARGE_INTEGER offset;
	for (auto& s : input) {
		if (s.size <= 0)
			continue;

		offset.QuadPart = (LONGLONG)s.address;
		SetFilePointerEx(source.handle, offset, nullptr, FILE_BEGIN);

		DWORD retrieved = 0;
		ReadFile(source.handle, &source.buffer[s.offset], s.size, &retrieved, nullptr);
		auto error = GetLastError();
		s.retrieved = retrieved;
	}
}

u8 *header = nullptr;
#define IMAGE_HEADER_PAGE_SIZE 0x1000
#define PROCESS_BASE_NAME_SIZE 1000
#define PROCESS_NAME_LEN 100

bool find_section_names(HANDLE proc, int idx, u64 base, u8 *buf, Source& source) {
	SIZE_T len = 0;
	ReadProcessMemory(proc, (LPCVOID)base, (LPVOID)buf, IMAGE_HEADER_PAGE_SIZE, &len);
	if (len != IMAGE_HEADER_PAGE_SIZE)
		return false;

	IMAGE_NT_HEADERS *headers = ImageNtHeader((LPVOID)buf);
	if (!headers)
		return false;

	int n_sections = headers->FileHeader.NumberOfSections;
	IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(headers);

	for (int i = 0; i < n_sections; i++) {
		u64 addr = base + section->VirtualAddress;
		Region& r = source.regions[idx + i + 1];
		if (r.base == addr)
			r.name = source.arena.alloc_string((char*)section->Name);

		section++;
	}

	return n_sections > 0;
}

void refresh_process_regions(Source& source) {
	auto& proc = (HANDLE&)source.handle;
	if (!proc)
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, source.pid);

	MEMORY_BASIC_INFORMATION info = {0};
	VirtualQueryEx(proc, (LPCVOID)0, &info, sizeof(MEMORY_BASIC_INFORMATION));
	u64 base = info.RegionSize;

	source.regions.resize(0);

	while (1) {
		if (!VirtualQueryEx(proc, (LPCVOID)base, &info, sizeof(MEMORY_BASIC_INFORMATION)))
			break;

		int protect = info.Protect & 0xff;
		bool acceptable = false;
		if (info.State == 0x1000 && (info.Protect & PAGE_GUARD) == 0)
			acceptable = (protect != PAGE_NOACCESS && protect != PAGE_EXECUTE);

		// simply unacceptable behaviour
		if (!acceptable) {
			base += info.RegionSize;
			continue;
		}

		u64 size = info.RegionSize;

		bool pm_read = true;
		bool pm_write = 
			protect != PAGE_EXECUTE_READ &&
			protect != PAGE_READONLY;
		bool pm_exec =
			protect == PAGE_EXECUTE_READ ||
			protect == PAGE_EXECUTE_READWRITE ||
			protect == PAGE_EXECUTE_WRITECOPY;

		u32 flags = (pm_read << REG_PM_READ) | (pm_write << REG_PM_WRITE) | (pm_exec << REG_PM_EXEC);

		source.regions.push_back({
			.name = nullptr,
			.base = base,
			.size = size,
			.flags = flags
		});

		base += info.RegionSize;
	}

	source.arena.rewind();
	source.arena.set_rewind_point();

	char name[PROCESS_BASE_NAME_SIZE];
	if (!header)
		header = new u8[IMAGE_HEADER_PAGE_SIZE];

	int idx = 0;
	u32 mask = FLAG_OCCUPIED | FLAG_NEW;
	for (int i = 0; i < source.regions.size(); i++) {
		u32 new_flags = source.regions[i].flags & mask;
		if (new_flags != mask)
			continue;

		auto& reg = source.regions[i];
		reg.flags &= ~FLAG_NEW;
		if (reg.size == IMAGE_HEADER_PAGE_SIZE)
			find_section_names(proc, i, reg.base, header, source);

		char *exe = nullptr;
		int len = GetMappedFileNameA(proc, (LPVOID)reg.base, name, PROCESS_BASE_NAME_SIZE);
		if (len > 0) {
			char *str = strrchr(name, '\\');
			exe = str ? str+1 : name;
		}

		if (exe || reg.name) {
			char *full_name = (char*)source.arena.allocate(PROCESS_NAME_LEN);

			snprintf(full_name, PROCESS_NAME_LEN-1, "%s%s%s",
				exe ? exe : "",
				reg.name ? " " : "",
				reg.name ? reg.name : ""
			);

			reg.name = full_name;
		}
	}
}

void refresh_process_spans(Source& source, std::vector<Span>& input) {
	auto& proc = (HANDLE&)source.handle;
	if (!proc)
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, source.pid);

	for (auto& s : input) {
		SIZE_T retrieved = 0;
		ReadProcessMemory(proc, (LPCVOID)s.address, (LPVOID)&source.buffer[s.offset], s.size, &retrieved);
		s.retrieved = retrieved;
	}
}

void close_source(Source& source) {
	if (source.handle) {
		CloseHandle(source.handle);
		source.handle = nullptr;
	}
}

SOURCE_HANDLE get_readonly_file_handle(void *identifier) {
	return open_file((LPCSTR)identifier);
}

SOURCE_HANDLE get_readonly_process_handle(int pid) {
	return OpenProcess(PROCESS_ALL_ACCESS, false, pid);
}

int read_page(SOURCE_HANDLE handle, SourceType type, u64 address, char *buf) {
	int retrieved = 0;
	if (type == SourceFile) {
		LARGE_INTEGER offset;
		offset.QuadPart = (LONGLONG)address;
		SetFilePointerEx(handle, offset, nullptr, FILE_BEGIN);

		DWORD r = 0;
		ReadFile(handle, buf, PAGE_SIZE, &r, nullptr);
		retrieved = (int)r;
	}
	else if (type == SourceProcess) {
		SIZE_T r = 0;
		ReadProcessMemory(handle, (LPCVOID)address, (LPVOID)buf, PAGE_SIZE, &r);
		retrieved = (int)r;
	}
	return retrieved;
}

void wait_ms(int ms) {
	Sleep(ms);
}

bool start_thread(void **thread_ptr, void *data, THREAD_RETURN_TYPE (*function)(void*)) {
	*thread_ptr = (HANDLE)CreateThread(nullptr, 0, function, data, 0, nullptr);
	return *thread_ptr == nullptr;
}

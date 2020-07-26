#include "muscles.h"
#include "io.h"

#include <algorithm>
#include <map>

#include <windows.h>
#include <Psapi.h>
#include <DbgHelp.h>

const char *get_folder_separator() {
	return "\\";
}
const char *get_root_folder() {
	return "C:\\";
}

Texture load_icon(const char *path);

void get_process_id_list(std::vector<int>& list) {
	const int max_pids = 1024;
	list.resize(max_pids);
	DWORD size = 0;
	EnumProcesses((DWORD*)list.data(), max_pids * sizeof(int), &size);
	list.resize(size / sizeof(int));
}

int get_process_names(std::vector<int>& list, std::vector<Texture>& icons, std::vector<char*>& names, int count_per_cell) {
	const int proc_path_len = 1024;
	auto proc_path = std::make_unique<char[]>(proc_path_len);
	int size = list.size() < names.size() ? list.size() : names.size();

	int idx = 0;
	for (int i = 0; i < size; i++) {
		// Many of the PIDs provided by EnumProcesses can't be opened, so we filter them out here
		int pid = list[i];
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

		icons[idx] = load_icon(path);

		// The name in 'proc_path' is currently represented as a path to the file,
		//  so we find the last backslash and use the string from that point
		char *p = strrchr(path, '\\');
		if (!p)
			p = path;
		else
			p++;

		snprintf(names[idx], count_per_cell, "%5d %s", pid, p);
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
			*it = (File_Entry*)arena.allocate(sizeof(File_Entry));

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

void refresh_file_region(Source& source, Arena& arena) {
	if (source.regions.size() <= 0)
		source.regions.push_back({});

	Region& reg = source.regions[0];

	DWORD attrs = 0;
	if (!source.handle) {
		WIN32_FIND_DATAA info = {0};
		source.handle = FindFirstFileA((LPCSTR)source.identifier, &info);
		attrs = info.dwFileAttributes;
		reg.size = ((u64)info.nFileSizeHigh << 32) | info.nFileSizeLow;
	}
	else {
		BY_HANDLE_FILE_INFORMATION info = {0};
		GetFileInformationByHandle(source.handle, &info);
		attrs = info.dwFileAttributes;
		reg.size = ((u64)info.nFileSizeHigh << 32) | info.nFileSizeLow;
	}

	if (attrs & FILE_ATTRIBUTE_DIRECTORY)
		reg.size = 0;

	bool pm_read = true;
	bool pm_write = (attrs & FILE_ATTRIBUTE_READONLY) == 0;
	bool pm_exec = true;
	reg.flags = (pm_read << 2) | (pm_write << 1) | pm_exec;

	reg.base = 0;
	reg.name = (char*)source.name.c_str();
}

void refresh_file_spans(Source& source, Arena& arena) {
	if (!source.handle) {
		WIN32_FIND_DATAA info;
		source.handle = FindFirstFileA((LPCSTR)source.identifier, &info);
	}

	LARGE_INTEGER offset;
	for (auto& s : source.spans) {
		if (s.size <= 0)
			continue;

		offset.QuadPart = (LONGLONG)s.offset;
		SetFilePointerEx(source.handle, offset, nullptr, FILE_BEGIN);

		if (!s.cache)
			s.cache = new u8[s.size];

		DWORD retrieved = 0;
		ReadFile(source.handle, s.cache, s.size, &retrieved, nullptr);
		s.retrieved = retrieved;
	}

	FindClose(source.handle);
	source.handle = nullptr;
}

u8 *header = nullptr;
#define IMAGE_HEADER_PAGE_SIZE 0x1000
#define PROCESS_BASE_NAME_SIZE 1000
#define PROCESS_NAME_LEN 100

void get_section_names(HANDLE proc, u64 base, u8 *buf, std::map<u64, char*>& sections, Arena& arena) {
	SIZE_T len = 0;
	ReadProcessMemory(proc, (LPCVOID)base, (LPVOID)buf, IMAGE_HEADER_PAGE_SIZE, &len);
	if (len != IMAGE_HEADER_PAGE_SIZE)
		return;

	IMAGE_NT_HEADERS *headers = ImageNtHeader((LPVOID)buf);
	if (!headers)
		return;

	int n_sections = headers->FileHeader.NumberOfSections;
	IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(headers);

	for (int i = 0; i < n_sections; i++) {
		sections[base + section->VirtualAddress] = arena.alloc_string((char*)section->Name);
		section++;
	}
}

void refresh_process_regions(Source& source, Arena& arena) {
	auto& proc = (HANDLE&)source.handle;
	if (!proc)
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, source.pid);

	MEMORY_BASIC_INFORMATION info = {0};
	VirtualQueryEx(proc, (LPCVOID)0, &info, sizeof(MEMORY_BASIC_INFORMATION));
	u64 base = info.RegionSize;

	char name[PROCESS_BASE_NAME_SIZE];
	if (!header)
		header = new u8[IMAGE_HEADER_PAGE_SIZE];

	std::map<u64, char*> sections;
	int idx = 0;

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

		if (idx >= source.regions.size()) {
			source.regions.push_back({});
			idx = source.regions.size() - 1;
		}

		Region& reg = source.regions[idx];
		reg.base = base;
		reg.size = info.RegionSize;

		bool pm_read = true;
		bool pm_write = 
			protect != PAGE_EXECUTE_READ &&
			protect != PAGE_READONLY;
		bool pm_exec =
			protect == PAGE_EXECUTE_READ ||
			protect == PAGE_EXECUTE_READWRITE ||
			protect == PAGE_EXECUTE_WRITECOPY;

		reg.flags = (pm_read << 2) | (pm_write << 1) | pm_exec;

		if (info.RegionSize == IMAGE_HEADER_PAGE_SIZE)
			get_section_names(proc, base, header, sections, arena);

		int len = GetMappedFileNameA(proc, (LPVOID)base, name, PROCESS_BASE_NAME_SIZE);
		char *exe = nullptr;
		if (len > 0) {
			char *str = strrchr(name, '\\');
			exe = str ? str+1 : name;
		}

		auto it = sections.find(base);
		char *s_name = (it != sections.end()) ? it->second : nullptr;

		if (exe && s_name) {
			if (!reg.name)
				reg.name = (char*)arena.allocate(PROCESS_NAME_LEN);

			snprintf(reg.name, PROCESS_NAME_LEN-1,
				"%s%s%s",
				exe ? exe : "",
				s_name ? " " : "",
				s_name ? s_name : ""
			);
		}
		else
			reg.name = nullptr;

		base += info.RegionSize;
		idx++;
	}
}

void refresh_process_spans(Source& source, Arena& arena) {
	auto& proc = (HANDLE&)source.handle;
	if (!proc)
		proc = OpenProcess(PROCESS_ALL_ACCESS, false, source.pid);

	for (auto& s : source.spans) {
		if (!s.cache)
			s.cache = new u8[s.size];

		SIZE_T retrieved = 0;
		ReadProcessMemory(proc, (LPCVOID)s.offset, (LPVOID)s.cache, s.size, &retrieved);
		s.retrieved = retrieved;
	}
}
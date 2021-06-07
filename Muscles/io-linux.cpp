#include "muscles.h"
#include <algorithm>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#define PROC_NAME_MAX 512

const char *get_folder_separator() {
	return "/";
}
const char *get_root_folder() {
	return "/";
}
const char *get_project_path() {
	return "./";
}

bool is_folder(const char *name) {
	struct stat s;
	if (stat(name, &s) != 0)
		return false;

	return (s.st_mode & S_IFMT) == S_IFDIR;
}

std::pair<int, std::unique_ptr<u8[]>> read_file(const char *path) {
	std::pair<int, std::unique_ptr<u8[]>> buf = {0, nullptr};

	FILE *f = fopen(path, "rb");
	if (!f)
		return buf;

	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	rewind(f);

	if (sz < 1)
		return buf;

	buf.first = sz;
	buf.second = std::make_unique<u8[]>(buf.first);

	fread(buf.second.get(), 1, buf.first, f);
	fclose(f);
	return buf;
}

std::pair<int, std::unique_ptr<u8[]>> read_file(std::string& path) {
	return read_file(path.c_str());
}

std::pair<int, std::unique_ptr<u8[]>> read_small_file(const char *path) {
	std::pair<int, std::unique_ptr<u8[]>> buf = {0, nullptr};

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return buf;

	struct stat s;
	int res = fstat(fd, &s);
	if (res != 0 || (s.st_mode & S_IFMT) == S_IFDIR) {
		close(fd);
		return buf;
	}

	buf.second = std::make_unique<u8[]>(PAGE_SIZE);
	buf.first = read(fd, buf.second.get(), PAGE_SIZE);
	close(fd);

	if (buf.first < PAGE_SIZE)
		buf.second[buf.first] = 0;

	return buf;
}

Texture load_icon(const char *path);

void get_process_id_list(std::vector<s64>& list) {
	list.reserve(1024);
	list.resize(0);

	DIR *d = opendir("/proc");
	if (!d) {
		std::string msg("Error: could not open /proc: ");
		msg += std::to_string(errno);
		sdl_log_string(msg.c_str());
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(d))) {
		s64 pid = 0;
		bool is_num = ent->d_name[0] != 0;

		for (int i = 0; ent->d_name[i] != 0; i++) {
			char c = ent->d_name[i];
			if (c < '0' || c > '9') {
				is_num = false;
				break;
			}
			pid *= 10l;
			pid += (long)(c - '0');
		}
		if (is_num)
			list.push_back(pid);
	}

	closedir(d);
}

int get_process_names(std::vector<s64> *full_list, Map& icon_map, std::vector<void*>& icons, std::vector<void*>& pids, std::vector<void*>& names, int count_per_cell) {
	char path[1024];

	std::vector<s64> *list = full_list ? full_list : (std::vector<s64>*)&pids;
	int size = list->size() < names.size() ? list->size() : names.size();

	int idx = 0;
	for (int i = 0; i < size; i++) {
		// Many of the PIDs provided by EnumProcesses can't be opened, so we filter them out here
		int pid = (int)(*list)[i];
		sprintf(path, "/proc/%d/comm", pid);

		auto buf = read_small_file((const char*)path);
		if (!buf.first)
			continue;

		icons[idx] = nullptr;

		s64 pid_long = (s64)pid;
		pids[idx] = (void*)pid_long;

		char *name = (char*)names[idx];
		strncpy(name, (char*)buf.second.get(), count_per_cell-1);
		if (buf.first < count_per_cell)
			name[buf.first] = 0;

		idx++;
	}

	return idx;
}

Texture load_icon(const char *path) {
	return nullptr;
}

void enumerate_files(char *path, std::vector<File_Entry*>& files, Arena& arena) {
	if (!is_folder((const char*)path))
		return;

	char full_name[1024];
	const int MAX_LEN = 512;

	int path_len = strlen(path);
	if (path_len > MAX_LEN)
		path_len = MAX_LEN;

	strncpy(full_name, path, MAX_LEN);
	full_name[path_len] = '/';
	int name_offset = path_len + 1;

	DIR *d = opendir(path);
	struct dirent *ent;

	auto it = files.begin();
	bool increase = false;
	int size = 0;

	while ((ent = readdir(d))) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		strcpy(&full_name[name_offset], ent->d_name);

		struct stat s;
		if (stat(full_name, &s) != 0)
			continue;

		if (it == files.end())
			increase = true;

		if (increase) {
			files.push_back((File_Entry*)arena.allocate(sizeof(File_Entry)));
			it = files.begin() + (files.size() - 1);
		}

		File_Entry *&file = *it;
		if (!file)
			file = (File_Entry*)arena.allocate(sizeof(File_Entry));

		int dir = (s.st_mode & S_IFDIR) != 0;
		int pms = (s.st_mode & S_IRWXU) / S_IXUSR;

		file->flags = (dir << 3) | pms;
		file->path = path;
		strncpy(file->name, ent->d_name, MAX_FNAME-1);
		file->name[MAX_FNAME-1] = 0;

		size++;
		++it;
	}

	closedir(d);

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

	if (source.fd <= 0)
		source.fd = open((char*)source.identifier, O_RDONLY);

	reg.flags = 0;
	reg.base = 0;
	reg.size = 0;
	reg.name = (char*)source.name.c_str();

	struct stat s;
	if (stat((const char*)source.identifier, &s) != 0)
		return;

	if ((s.st_mode & S_IFREG) != 0)
		reg.size = s.st_size;

	reg.flags = (s.st_mode & S_IRWXU) / S_IXUSR;
}

void refresh_file_spans(Source& source, std::vector<Span>& input) {
	if (source.fd <= 0)
		source.fd = open((char*)source.identifier, O_RDONLY);

	s64 offset;
	for (auto& s : input) {
		if (s.size <= 0)
			continue;

		offset = s.address;
		lseek64(source.fd, offset, SEEK_SET);

		s.retrieved = read(source.fd, &source.buffer[s.offset], s.size);
	}
}

void refresh_process_regions(Source& source) {
	char maps_path[32];
	char err_msg[128];

	snprintf(maps_path, 32, "/proc/%d/maps", source.pid);
	int maps_fd = open(maps_path, O_RDONLY);
	if (maps_fd < 0) {
		snprintf(err_msg, 128, "Error: could not open %s", maps_path);
		sdl_log_string(err_msg);
		return;
	}

	int idx = 0;
	int size = 0;
	int retrieved = 0;
	do {
		if (idx >= source.proc_map_pages.size())
			source.proc_map_pages.resize(idx + 1);

		char*& buf = source.proc_map_pages[idx];
		if (!buf)
			buf = new char[PAGE_SIZE];

		retrieved = read(maps_fd, buf, PAGE_SIZE);
		if (retrieved < 0) {
			snprintf(err_msg, 100, "Error: failed to read from offset %d in %s", size, maps_path);
			sdl_log_string(err_msg);
		}

		size += retrieved;
		idx++;
	}
	while (retrieved == PAGE_SIZE);

	close(maps_fd);

	source.regions.resize(0);

	source.arena.rewind();
	source.arena.set_rewind_point();

	int mode = 0;
	u64 start = 0;
	u64 end = 0;
	int pms = 0;
	int flag_count = 0;
	char name_buf[PROC_NAME_MAX];
	int name_size = 0;

	for (int i = 0; i < size; i++) {
		char *page = source.proc_map_pages[i / PAGE_SIZE];
		int off = i % PAGE_SIZE;
		char c = page[off];

		switch (mode) {
			case 0:
			{
				if (c == '-')
					mode = 1;
				else if (c >= '0' && c <= '9')
					start = (start << 4LL) | (u64)(c - '0');
				else if (c >= 'a' && c <= 'f')
					start = (start << 4LL) | (u64)(c - 'a' + 10);
				break;
			}
			case 1:
			{
				if (c == ' ')
					mode = 2;
				else if (c >= '0' && c <= '9')
					end = (end << 4LL) | (u64)(c - '0');
				else if (c >= 'a' && c <= 'f')
					end = (end << 4LL) | (u64)(c - 'a' + 10);
				break;
			}
			case 2:
			{
				if (flag_count >= 3)
					mode = 3;
				else {
					pms |= (c == "rwx"[flag_count]) << (2 - flag_count);
					flag_count++;
				}
				break;
			}
			case 3:
			{
				if (c == ' ' || c == '\t')
					name_size = 0;
				else if (name_size < PROC_NAME_MAX && c != '\n')
					name_buf[name_size++] = c;

				if (c != '\n' && i < size-1)
					break;

				char *name = nullptr;
				if (name_size > 0) {
					name = (char*)source.arena.allocate(name_size + 1);
					memcpy(name, name_buf, name_size);
				}

				source.regions.push_back((Region){
					.name = name,
					.base = start,
					.size = end - start,
					.flags = (u32)pms
				});

				mode = 0;
				start = 0;
				end = 0;
				pms = 0;
				flag_count = 0;
				name_size = 0;
				break;
			}
		}
	}
}

void refresh_process_spans(Source& source, std::vector<Span>& input) {
	char mem_path[32];
	snprintf(mem_path, 32, "/proc/%d/mem", source.pid);

	if (source.fd <= 0) {
		source.fd = open(mem_path, O_RDONLY);
		if (source.fd < 0) {
			char msg[128];
			snprintf(msg, 128, "Error: could not open %s", mem_path);
			sdl_log_string(msg);

			for (auto& s : input)
				s.retrieved = 0;
			return;
		}
	}

	for (auto& s : input) {
		lseek64(source.fd, s.address, SEEK_SET);
		s.retrieved = read(source.fd, (void*)&source.buffer[s.offset], s.size);
	}
}

void close_source(Source& source) {
	if (source.fd > 0) {
		close(source.fd);
		source.fd = 0;
	}
}

// please don't pass in "stdout" lol
SOURCE_HANDLE get_readonly_file_handle(void *identifier) {
	int fd = open((char*)identifier, O_RDONLY);
	return fd > 0 ? fd : 0;
}

SOURCE_HANDLE get_readonly_process_handle(int pid) {
	char mem_path[32];
	snprintf(mem_path, 32, "/proc/%d/mem", pid);
	int fd = open(mem_path, O_RDONLY);
	return fd > 0 ? fd : 0;
}

int read_page(SOURCE_HANDLE handle, u64 address, char *buf) {
	lseek64(handle, address, SEEK_SET);
	return read(handle, buf, PAGE_SIZE);
}

void wait_ms(int ms) {
	usleep(ms * 1000);
}

bool start_thread(void **thread_ptr, void *data, THREAD_RETURN_TYPE (*function)(void*)) {
	int res = pthread_create((pthread_t*)thread_ptr, nullptr, function, data);
	return res == 0;
}

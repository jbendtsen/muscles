#ifndef IO_H
#define IO_H

enum SourceType {
	TypeNoSource = 0,
	TypeFile,
	TypeProcess
};

struct Span {
	u64 offset = 0;
	int size = 0;
	int retrieved = 0;
	u8 *cache = nullptr;
};

struct Region {
	u64 base = 0;
	u64 size = 0;
	int flags = 0;
	char *name = nullptr;
};

struct Source {
	SourceType type = TypeNoSource;
	std::string name;

	int pid = 0;
	void *identifier = nullptr;
	void *handle = nullptr;

	std::vector<Region> regions;
	std::vector<Span> spans;

	int refresh_region_rate = 60;
	int refresh_span_rate = 60;
	int timer = 0;
};

void get_process_id_list(std::vector<int>& list);
int get_process_names(std::vector<int>& list, std::vector<Texture>& icons, std::vector<char*>& names, int count_per_cell);

const char *get_folder_separator();
const char *get_root_folder();
void enumerate_files(char *path, std::vector<File_Entry*>& files, Arena& arena);

void refresh_file_region(Source& source, Arena& arena);
void refresh_process_regions(Source& source, Arena& arena);

void refresh_file_spans(Source& source, Arena& arena);
void refresh_process_spans(Source& source, Arena& arena);

#endif
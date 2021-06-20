#pragma once

#define MAX_SEARCH_RESULTS 10000
#define MAX_SEARCH_PARAMS 100

#define METHOD_EQUALS 0
#define METHOD_RANGE  1

struct Search_Parameter {
	u32 flags;
	int method;
	int offset;
	int size;
	s64 value1;
	s64 value2;
};

struct Search {
	Search_Parameter single_value = {0};
	Search_Parameter *params = nullptr;
	int n_params = 0;

	int byte_align = 0;
	u64 start_addr = 0;
	u64 end_addr = 0;
	Struct *record = nullptr;

	SourceType source_type = SourceNone;
	int pid = 0;
	void *identifier = nullptr;
};

void start_search(Search& s, std::vector<Region> const& regions);
bool check_search_running();
bool check_search_finished();
void get_search_results(std::vector<u64>& results_vec);
void reset_search();
void exit_search();

#include "muscles.h"

// TODO merge clear_word() and advance_word() into String_Vector

void clear_word(char* word) {
	int len = strlen(word);
	memset(word, 0xff, len);
}

char *advance_word(char *p) {
	do {
		p += strlen(p + 1);
	} while (*p == 0xff);
	return p;
}

void parse_definitions(Map& definitions, String_Vector& tokens) {
	char *p = tokens.pool;
	char *prev = nullptr;
	bool define = false;
	bool was_hashtag = false;

	while (*p) {
		if (define) {
			
		}

		if (!strcmp(p, "define") && prev && prev[0] == '#') {
			define = true;
		}

		was_hashtag = *p == '#';
		prev = p;
		p = advance_word(p);
	}
}
#include "muscles.h"

#define PADDING(offset, align) ((align) - ((offset) % (align))) % (align)

Field_Vector::Field_Vector() {
	data = new Field[pool_size];
}
Field_Vector::~Field_Vector() {
	delete[] data;
}

Field& Field_Vector::back() {
	return data[n_fields-1];
}

// increase pool size by a power of two each time a pool expansion is necessary
void Field_Vector::expand() {
	int old_size = pool_size;
	pool_size *= 2;
	Field *next = new Field[pool_size];
	memcpy(next, data, old_size * sizeof(Field));
	delete[] data;
	data = next;
}

Field& Field_Vector::add(Field&& f) {
	if (n_fields >= pool_size)
		expand();
	data[n_fields++] = f;
	return back();
}
Field& Field_Vector::add(Field& f) {
	if (n_fields >= pool_size)
		expand();
	data[n_fields++] = f;
	return back();
}

void Field_Vector::cancel_latest() {
	if (n_fields <= 0)
		return;

	data[n_fields] = {0};
	n_fields--;
}

void Field_Vector::zero_out() {
	memset(data, 0, pool_size * sizeof(Field));
	n_fields = 0;
}

// A list of primitive types in the form of a vector. This lets us potentially add new primitives at run-time.
std::vector<Primitive> primitive_types = {
	{"char", 8, FLAG_SIGNED},
	{"int8_t", 8, FLAG_SIGNED},
	{"uint8_t", 8},
	{"short", 16, FLAG_SIGNED},
	{"int16_t", 16, FLAG_SIGNED},
	{"uint16_t", 16},
	{"int", 32, FLAG_SIGNED},
	{"int32_t", 32, FLAG_SIGNED},
	{"uint32_t", 32},
	{"long", 32, FLAG_SIGNED},
	{"long long", 64, FLAG_SIGNED},
	{"float", 32, FLAG_FLOAT},
	{"double", 64, FLAG_FLOAT}
};

int pointer_size = 64; // bits

bool is_symbol(char c) {
	return !(c >= '0' && c <= '9') &&
		!(c >= 'A' && c <= 'Z') &&
		!(c >= 'a' && c <= 'z') &&
		c != '_';
}

void store_word(char **pool, char *word, char **ptr) {
	if (word) {
		int len = strlen(word);
		if (len) {
			strcpy(*pool, word);
			*ptr = *pool;
			*pool += len + 1;
		}
	}
}

void add_token(std::vector<char>& tokens, const char *word, int len) {
	if (len <= 0)
		return;

	int size = tokens.size();
	tokens.resize(size + len + 1);

	char *buf = tokens.data() + size;
	memcpy(buf, word, len);
	buf[len] = 0;
}

void add_chars(std::vector<char>& tokens, char c, int count) {
	if (count <= 0)
		return;

	int size = tokens.size();
	tokens.resize(size + count + 1);

	char *p = tokens.data() + size;
	for (int i = 0; i < count; i++)
		*p++ = c;
	*p = 0;
}

void tokenize(std::vector<char>& tokens, const char *text, int sz) {
	const char *in = text;
	const char *word = text;
	int word_len = 0;

	bool multi_comment = false;
	bool line_comment = false;
	bool was_symbol = true;
	bool was_token_end = false;

	char prev = 0;
	for (int i = 0; i < sz; i++, in++) {
		char c = *in;

		if (multi_comment && prev == '*' && c == '/') {
			multi_comment = false;
			word = in+1;
			prev = c;
			continue;
		}
		if (line_comment && c == '\n') {
			line_comment = false;
			word = in;
		}

		if (c == '/' && i < sz-1) {
			char next = in[1];
			if (next == '*')
				multi_comment = true;
			if (next == '/')
				line_comment = true;
		}

		if (multi_comment || line_comment) {
			prev = c;
			continue;
		}

		// We make an exception for the '<', '>' and ':' symbols from being split into their own tokens
		//  to allow for templated and/or namespaced types.
		bool sandwichable = (c == '<' || c == '>' || c == ':');

		if (c <= ' ' || c > '~') {
			add_token(tokens, word, word_len);
			word_len = 0;
			was_token_end = true;
		}
		else if (
			(is_symbol(c) && !sandwichable) ||
			(was_symbol && sandwichable) ||
			(c == ':' && i < sz-1 && i > 0 && (in[1] != ':' && in[-1] != ':'))
		) {
			add_token(tokens, word, word_len);

			if (c == '*') {
				int n = 1;
				while (i < sz-1 && in[1] == '*') {
					n++;
					in++;
					i++;
				}
				add_chars(tokens, '*', n);
			}
			else
				add_chars(tokens, c, 1);

			word_len = 0;
			was_symbol = true;
			was_token_end = false;
		}
		else {
			if (was_token_end) {
				add_token(tokens, word, word_len);
				word = in;
				word_len = 1;
			}
			else
				word_len++;

			was_symbol = false;
			was_token_end = false;
		}

		prev = c;
	}

	if (was_token_end)
		add_token(tokens, word, word_len);

	tokens.push_back(0);
}

// TODO: Expand upon this function to allow for constant variable lookup
u64 evaluate_number(char *token) {
	return strtoull(token, nullptr, 0);
}

void embed_struct(Struct *master, Struct *embed) {
	Field& group = master->fields.back();

	if (group.flags & FLAG_POINTER) {
		master->offset += PADDING(master->offset, pointer_size);

		group.bit_offset = master->offset;
		group.bit_size = pointer_size;

		master->offset += group.bit_size;
		return;
	}

	group.bit_offset = master->offset;
	group.bit_size = embed->total_size;

	int align = embed->longest_primitive;
	master->offset += PADDING(master->offset, align);

	for (int i = 0; i < embed->fields.n_fields; i++) {
		Field *f = &master->fields.add(embed->fields.data[i]);
		f->bit_offset += master->offset;
	}

	master->offset += embed->total_size;
}

// When either a struct/union is defined or referenced inside another struct, a declaration list may follow.
// This function finds each element in the declaration list and embeds it into the current struct.
void add_struct_instances(Struct *current_st, Struct *embed, char **tokens, char **name_head) {
	Field *f = &current_st->fields.back();
	char *t = *tokens;
	bool first = true;

	while (*t) {
		if (!first && *t == ';')
			break;
		first = false;

		if (is_symbol(*t) && *t != ';') {
			if (*t == '*')
				f->flags |= FLAG_POINTER;

			t += strlen(t) + 1;
			continue;
		}

		f->flags |= FLAG_COMPOSITE;
		f->st = nullptr;

		f->type_name = embed->name;
		if (!f->type_name)
			f->st = embed;

		if (*t == ';') {
			embed_struct(current_st, embed);
			current_st->fields.add({0});
			break;
		}

		store_word(name_head, t, &f->field_name);
		embed_struct(current_st, embed);

		f = &current_st->fields.add({0});
		t += strlen(t) + 1;
	}

	*tokens = t;
}

Struct *lookup_struct(std::vector<Struct*>& structs, Struct *st, Field *f, char *str) {
	if (!strcmp(str, "void") || !strcmp(str, "struct") || !strcmp(str, "union"))
		return nullptr;

	Struct *record = nullptr;
	for (auto& c : structs) {
		if (!c->name || strcmp(c->name, str))
			continue;

		record = c;
		break;
	}

	if (!record) {
		f->flags |= FLAG_UNRECOGNISED;
		return nullptr;
	}

	// If the requested struct/union is the same as the current one, that would make it grow infinitely, so we do not allow that.
	if (!strcmp(record->name, st->name)) {
		f->flags |= FLAG_UNUSABLE;
		st->flags |= FLAG_UNUSABLE;
	}

	if (record->flags & FLAG_UNUSABLE)
		f->flags |= FLAG_UNUSABLE;

	if (f->flags & FLAG_UNUSABLE)
		return nullptr;

	return record;
}

Struct *find_new_struct(std::vector<Struct*>& structs) {
	Struct *new_st = nullptr;
	if (structs.size() > 0) {
		for (auto& s : structs) {
			if (s->flags & FLAG_AVAILABLE) {
				s->name = nullptr;
				s->offset = 0;
				s->total_size = 0;
				s->longest_primitive = 0;
				s->flags = 0;
				s->fields.zero_out();

				new_st = s;
				break;
			}
		}
	}
	if (!new_st) {
		new_st = new Struct();
		structs.push_back(new_st);
	}

	return new_st;
}

void parse_c_struct(std::vector<Struct*>& structs, char **tokens, char **name_buf, Struct *st) {
	char *name_head = *name_buf;
	std::string type;

	char *t = *tokens;
	char *prev = nullptr;

	bool top_level = false;
	if (!st) {
		st = find_new_struct(structs);
		top_level = true;
	}

	Field *f = nullptr;
	int field_flags = 0;

	bool was_symbol = false;
	bool was_array_open = false;
	bool was_zero_width = false;
	bool named_field = false;
	char *held_type = nullptr;

	// The longest primitive in a struct/union is used to determine its byte alignment.
	// The longest field in a union is the size of the union.
	st->longest_primitive = 8;
	int longest_field = 8;

	while (*t) {
		char *next = t + strlen(t) + 1;
		bool type_over = is_symbol(*next) && *next != '*';

		if (!f) {
			f = &st->fields.add({0});
			f->flags = field_flags;
			named_field = false;
		}

		// Inline struct/union declaration
		if (*t == '{' && type.size() > 0) {
			char *name = named_field ? prev : nullptr;
			if (!top_level) {
				bool is_union = !strcmp(type.c_str(), "union");

				Struct *new_st = find_new_struct(structs);
				new_st->flags |= is_union ? FLAG_UNION : 0;
				store_word(&name_head, name, &new_st->name);

				t += strlen(t) + 1;
				parse_c_struct(structs, &t, &name_head, new_st);

				add_struct_instances(st, new_st, &t, &name_head);
				next = t + strlen(t) + 1;

				f = &st->fields.back();
			}
			else {
				store_word(&name_head, name, &st->name);
				top_level = false;
			}

			held_type = nullptr;
			was_symbol = true;
			type = "";
		}
		else if (is_symbol(*t) && *t != '*') {
			// Bitfield
			if (!strcmp(t, ":")) { // the bitfield symbol ':' must have a length of 1
				int n = (int)evaluate_number(next);
				if (n >= 0) {
					if (n == 0) {
						f->type_name = f->field_name = nullptr;
						was_zero_width = true;
					}
					f->bit_size = n;
					f->flags |= FLAG_BITFIELD;

					if (!named_field || n == 0)
						f->flags |= FLAG_UNNAMED;
				}
			}

			// Set field name and type name
			if (!was_symbol) {
				if (!f->type_name)
					store_word(&name_head, (char*)type.c_str(), &f->type_name);

				if (!f->field_name && (f->flags & FLAG_UNNAMED) == 0)
					store_word(&name_head, prev, &f->field_name);
			}

			// Calculate array length. Each successive array dimension is simply multiplied with the current array length.
			if (*t == '[') {
				int n = (int)evaluate_number(next);
				if (n > 0) {
					if (f->array_len > 0)
						f->array_len *= n;
					else
						f->array_len = n;
				}
			}

			// End of the current field
			if (*t == ';' || *t == ',') {
				if (f->flags & FLAG_POINTER) {
					f->bit_size = pointer_size;
					f->default_bit_size = pointer_size;
				}

				// Determine padding before the current field starts
				int align = f->default_bit_size;
				if (align > 0 && (was_zero_width || (f->flags & FLAG_BITFIELD) == 0)) {
					if ((st->flags & FLAG_UNION) == 0)
						st->offset += PADDING(st->offset, align);
					if (align > st->longest_primitive)
						st->longest_primitive = align;

					was_zero_width = false;
				}

				if (f->array_len > 0)
					f->bit_size *= f->array_len;

				if (f->bit_size > longest_field)
					longest_field = f->bit_size;

				// A handy definition of the C union
				f->bit_offset = st->offset;
				if ((st->flags & FLAG_UNION) == 0)
					st->offset += f->bit_size;

				// Most zero-width fields are later overwritten.
				// By setting the current field to NULL, we know that a new field must be created later.
				if (!was_zero_width && (f->type_name || f->field_name)) {
					field_flags = *t == ',' ? f->flags : 0;
					held_type = f->type_name;
					f = nullptr;
				}
			}

			if (*t == ';') {
				held_type = nullptr;
				type = "";
			}

			was_symbol = true;
		}
		else {
			if (*t == '*') {
				f->flags |= FLAG_POINTER;
				f->pointer_levels = strlen(t);
			}
			else if (held_type) {
				named_field = true;
			}
			else {
				// Determine if this field has a name (unnamed fields can be legal, eg. zero-width bitfields)
				if (type_over) {
					named_field = true;
					for (auto& p : primitive_types) {
						if (!strcmp(t, p.name)) {
							named_field = false;
							break;
						}
					}

					if (named_field)
						named_field = strcmp(t, "struct") && strcmp(t, "union");
				}

				// Ignore "const" when determining the field type
				if ((!type_over && strcmp(t, "const")) || (type_over && !named_field)) {
					if (type.size() > 0)
						type += ' ';
					type += t;
				}
			}

			was_symbol = false;
		}

		if (held_type)
			type = held_type;

		// Determine the type of the current field.
		// This code may run multiple times per field,
		//  as there some primitive types that use more than one keyword (eg. "long long")
		if (f && type_over && type.size() > 0 && (f->flags & FLAG_COMPOSITE) == 0) {
			const char *str = type.c_str();

			bool found_primitive = false;
			for (auto& p : primitive_types) {
				if (!strcmp(str, p.name)) {
					f->flags |= p.flags;
					f->default_bit_size = p.bit_size;
					if ((f->flags & FLAG_BITFIELD) == 0 || f->bit_size > f->default_bit_size)
						f->bit_size = f->default_bit_size;

					found_primitive = true;
					break;
				}
			}

			// Check if the type refers to an existing struct/union
			if (!found_primitive) {
				size_t last_space = type.find_last_of(' ');
				std::string type_name = last_space == std::string::npos ? type : type.substr(last_space + 1);

				// If so, then add its following declaration list to this struct
				Struct *record = lookup_struct(structs, st, f, (char*)type_name.c_str());
				if (record) {
					add_struct_instances(st, record, &t, &name_head);
					next = t + strlen(t) + 1;
					f = &st->fields.back();

					held_type = nullptr;
					was_symbol = true;
					type = "";
				}
			}
		}

		prev = t;
		t = next;

		if (*prev == '}')
			break;
	}

	// Pad the struct to the next appropriate boundary
	if (st->flags & FLAG_UNION)
		st->total_size = longest_field + PADDING(longest_field, st->longest_primitive);
	else
		st->total_size = st->offset + PADDING(st->offset, st->longest_primitive);

	// Cancel any remaining half-processed field
	if (f)
		st->fields.cancel_latest();

	*name_buf = name_head;
	*tokens = t;
}

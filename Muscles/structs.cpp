#include "muscles.h"

#define PADDING(offset, align) ((align) - ((offset) % (align))) % (align)

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

/*
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
*/

void tokenize(String_Vector& tokens, const char *text, int sz) {
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
			tokens.add_buffer(word, word_len);
			word_len = 0;
			was_token_end = true;
		}
		else if (
			(is_symbol(c) && !sandwichable) ||
			(was_symbol && sandwichable) ||
			(c == ':' && i < sz-1 && i > 0 && (in[1] != ':' && in[-1] != ':'))
		) {
			tokens.add_buffer(word, word_len);

			if (c == '*') {
				int n = 1;
				while (i < sz-1 && in[1] == '*') {
					n++;
					in++;
					i++;
				}

				char *p = tokens.allocate(n);
				for (int i = 0; i < n; i++)
					*p++ = c;
				*p = 0;
			}
			else {
				char *p = tokens.allocate(1);
				*p++ = c;
				*p++ = 0;
			}

			word_len = 0;
			was_symbol = true;
			was_token_end = false;
		}
		else {
			if (was_symbol || was_token_end) {
				if (was_token_end)
					tokens.add_buffer(word, word_len);

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
		tokens.add_buffer(word, word_len);

	tokens.append_extra_zero();
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
void add_struct_instances(Struct *current_st, Struct *embed, char **tokens, String_Vector& name_vector) {
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

		f->type_name_idx = embed->name_idx;
		if (f->type_name_idx < 0)
			f->st = embed;

		if (*t == ';') {
			embed_struct(current_st, embed);
			current_st->fields.add_blank();
			break;
		}

		f->field_name_idx = name_vector.add_string(t);
		embed_struct(current_st, embed);

		f = &current_st->fields.add_blank();
		t += strlen(t) + 1;
	}

	*tokens = t;
}

Struct *lookup_struct(std::vector<Struct*>& structs, Struct *st, Field *f, char *name_pool, char *str) {
	if (!strcmp(str, "void") || !strcmp(str, "struct") || !strcmp(str, "union"))
		return nullptr;

	Struct *record = nullptr;
	for (auto& c : structs) {
		if (c->name_idx < 0 || strcmp(&name_pool[c->name_idx], str))
			continue;

		record = c;
		break;
	}

	if (!record) {
		f->flags |= FLAG_UNRECOGNISED;
		return nullptr;
	}

	// If the requested struct/union is the same as the current one, that would make it grow infinitely, so we do not allow that.
	if (!strcmp(&name_pool[record->name_idx], &name_pool[st->name_idx])) {
		f->flags |= FLAG_UNUSABLE;
		st->flags |= FLAG_UNUSABLE;
	}

	if (record->flags & FLAG_UNUSABLE)
		f->flags |= FLAG_UNUSABLE;

	if (f->flags & FLAG_UNUSABLE)
		return nullptr;

	return record;
}

void finalize_struct(Struct *st, Field *f, int longest_field) {
	// Pad the struct to the next appropriate boundary
	if (st->flags & FLAG_UNION)
		st->total_size = longest_field + PADDING(longest_field, st->longest_primitive);
	else
		st->total_size = st->offset + PADDING(st->offset, st->longest_primitive);

	// Cancel any remaining half-processed field
	if (f)
		st->fields.cancel_latest();
}

Struct *find_new_struct(std::vector<Struct*>& structs) {
	Struct *new_st = nullptr;
	if (structs.size() > 0) {
		for (auto& s : structs) {
			if (s->flags & FLAG_AVAILABLE) {
				s->name_idx = -1;
				s->offset = 0;
				s->total_size = 0;
				s->longest_primitive = 8;
				s->flags = 0;
				s->fields.zero_out();

				new_st = s;
				break;
			}
		}
	}
	if (!new_st) {
		new_st = new Struct();
		new_st->name_idx = -1;
		new_st->longest_primitive = 8;
		structs.push_back(new_st);
	}

	return new_st;
}

void parse_c_struct(std::vector<Struct*>& structs, char **tokens, String_Vector& name_vector, Struct *st) {
	std::string type;

	char *t = *tokens;
	char *prev = nullptr;

	bool top_level = false;
	if (!st) {
		st = find_new_struct(structs);
		top_level = true;
	}
	bool outside_struct = top_level;

	Field *f = nullptr;
	int field_flags = 0;

	bool was_symbol = false;
	bool was_array_open = false;
	bool was_zero_width = false;
	bool named_field = false;
	int held_type = -1;

	// The longest primitive in a struct/union is used to determine its byte alignment.
	// The longest field in a union is the size of the union.
	int longest_field = 8;

	while (*t) {
		char *next = t + strlen(t) + 1;
		bool type_over = is_symbol(*next) && *next != '*';

		if (!f) {
			f = &st->fields.add_blank();
			f->flags = field_flags;
			named_field = false;
		}

		// Inline struct/union declaration
		if (*t == '{' && type.size() > 0) {
			char *name = named_field ? prev : nullptr;
			if (!outside_struct) {
				bool is_union = !strcmp(type.c_str(), "union");

				Struct *new_st = find_new_struct(structs);
				new_st->flags |= is_union ? FLAG_UNION : 0;
				new_st->name_idx = name_vector.add_string(name);

				t += strlen(t) + 1;
				parse_c_struct(structs, &t, name_vector, new_st);

				add_struct_instances(st, new_st, &t, name_vector);
				next = t + strlen(t) + 1;

				f = &st->fields.back();
			}
			else {
				st->name_idx = name_vector.add_string(name);
				outside_struct = false;
			}

			held_type = -1;
			was_symbol = true;
			type = "";
		}
		else if (is_symbol(*t) && *t != '*') {
			// Bitfield
			if (!strcmp(t, ":")) { // the bitfield symbol ':' must have a length of 1
				int n = (int)evaluate_number(next);
				if (n >= 0) {
					if (n == 0) {
						f->type_name_idx = f->field_name_idx = -1;
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
				if (f->type_name_idx < 0)
					f->type_name_idx = name_vector.add_string((char*)type.c_str());

				if (f->field_name_idx < 0 && (f->flags & FLAG_UNNAMED) == 0)
					f->field_name_idx = name_vector.add_string(prev);
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
				if (!was_zero_width && (f->type_name_idx >= 0 || f->field_name_idx >= 0)) {
					field_flags = *t == ',' ? f->flags : 0;
					held_type = f->type_name_idx;
					f = nullptr;
				}
			}

			if (*t == ';') {
				held_type = -1;
				type = "";
			}

			was_symbol = true;
		}
		else {
			if (*t == '*') {
				f->flags |= FLAG_POINTER;
				f->pointer_levels = strlen(t);
			}
			else if (held_type >= 0) {
				named_field = true;
			}
			else {
				// Determine if this field has a name (unnamed fields can be legal, eg. zero-width bitfields)
				if (type_over) {
					named_field = true;
					for (auto& p : primitive_types) {
						if (!strcmp(t, p.name.c_str())) {
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

		if (held_type >= 0)
			type = &name_vector.pool[held_type];

		// Determine the type of the current field.
		// This code may run multiple times per field,
		//  as there some primitive types that use more than one keyword (eg. "long long")
		if (f && type_over && type.size() > 0 && (f->flags & FLAG_COMPOSITE) == 0) {
			bool found_primitive = false;
			for (auto& p : primitive_types) {
				if (p.name == type) {
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
				Struct *record = lookup_struct(structs, st, f, name_vector.pool, (char*)type_name.c_str());
				if (record) {
					add_struct_instances(st, record, &t, name_vector);
					next = t + strlen(t) + 1;
					f = &st->fields.back();

					held_type = -1;
					was_symbol = true;
					type = "";
				}
			}
		}

		prev = t;
		t = next;

		if (*prev == '}') {
			if (top_level) {
				outside_struct = true;
				finalize_struct(st, f, longest_field);
				f = nullptr;
				st = find_new_struct(structs);
			}
			else
				break;
		}
	}

	finalize_struct(st, f, longest_field);
	*tokens = t;
}

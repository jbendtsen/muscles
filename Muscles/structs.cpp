#include "muscles.h"

#define PADDING(offset, align) ((align) - ((offset) % (align))) % (align)

const int pointer_size = 64; // bits

bool is_symbol(char c) {
	return !(c >= '0' && c <= '9') &&
		!(c >= 'A' && c <= 'Z') &&
		!(c >= 'a' && c <= 'z') &&
		c != '_';
}

bool is_struct_usable(Struct *s) {
	return s && (s->flags & (FLAG_UNUSABLE | FLAG_AVAILABLE)) == 0 && s->fields.n_fields > 0;
}

void set_primitives(Map& definitions) {
	u32 flags = FLAG_OCCUPIED | FLAG_PRIMITIVE;
	auto set = [flags](Bucket& buck, u32 type, u64 value) {
		buck.flags |= flags | type;
		buck.value = value;
	};

	set(definitions.insert("char"), FLAG_SIGNED, 8);
	set(definitions.insert("int8_t"), FLAG_SIGNED, 8);
	set(definitions.insert("uint8_t"), 0, 8);
	set(definitions.insert("short"), FLAG_SIGNED, 16);
	set(definitions.insert("int16_t"), FLAG_SIGNED, 16);
	set(definitions.insert("uint16_t"), 0, 16);
	set(definitions.insert("int"), FLAG_SIGNED, 32);
	set(definitions.insert("int32_t"), FLAG_SIGNED, 32);
	set(definitions.insert("uint32_t"), 0, 32);
	set(definitions.insert("long"), FLAG_SIGNED, 32);
	set(definitions.insert("long long"), FLAG_SIGNED, 64);
	set(definitions.insert("int64_t"), FLAG_SIGNED, 64);
	set(definitions.insert("uint64_t"), 0, 64);
	set(definitions.insert("float"), FLAG_FLOAT, 32);
	set(definitions.insert("double"), FLAG_FLOAT, 64);
}

// TODO: Expand upon this function to allow for constant variable lookup
Value64 evaluate_number(const char *token, bool as_float) {
	Value64 value;
	if (as_float)
		value.d = atof(token);
	else
		value.i = strtoll(token, nullptr, 0);
	return value;
}

/*
   This method gets the full name of a field in a struct instance, eg. player.position.x.
   It retrieves names starting with the target field and working back up the hierarchy (eg. x, position, player)
   By reversing each field name, then reversing the result at the end, the string is arranged in the desired order.
*/
int get_full_field_name(Field& field, String_Vector& name_vector, String_Vector& out_vec) {
	Field *f = &field;
	int start = out_vec.head;
	char *name = nullptr;
	bool first = true;
	bool last = false;

	while (true) {
		if (!first)
			out_vec.pool[out_vec.head-1] = '.';

		if (!name) {
			name = name_vector.at(f->field_name_idx);
			if (!name || *name == 0)
				name = (char*)"???";
		}
		int len = strlen(name);

		char *out = out_vec.allocate(len);
		char *in = &name[len-1];
		while (in >= name) {
			*out++ = *in;
			in--;
		}

		if (last || !f->paste_st)
			break;

		first = false;
		last = f->paste_field < 0;
		if (last) {
			name = name_vector.at(f->paste_st->name_idx);
			if (!name || *name == 0)
				name = (char*)"???";
		}
		else {
			name = nullptr;
			f = &f->paste_st->fields.data[f->paste_field];
		}
	}

	int len = out_vec.head - start - 1;
	char *full_name = out_vec.allocate(len + 1);

	{
		char *begin = &out_vec.pool[start];
		char *in = &begin[len-1];
		char *out = full_name;

		while (in >= begin) {
			*out++ = *in;
			in--;
		}
		*out++ = 0;
	}

	return full_name - out_vec.pool;
}

// TODO: don't split up floating point literals into separate tokens, eg. "3", ".", "141"
void tokenize(String_Vector& tokens, const char *text, int sz) {
	const char *in = text;
	const char *word = text;
	int word_len = 0;

	bool multi_comment = false;
	bool line_comment = false;
	bool macro = false;
	bool single_quotes = false;
	bool double_quotes = false;
	bool was_symbol = true;
	bool was_token_end = false;
	char last_non_ws = 0;

	char prev = 0;
	for (int i = 0; i < sz; i++, in++) {
		char c = *in;

		if (multi_comment && prev == '*' && c == '/') {
			multi_comment = false;
			word = in+1;
			prev = c;
			last_non_ws = c;
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

		if (prev != '\\') {
			if (c == '\'' && !double_quotes)
				single_quotes = !single_quotes;
			if (c == '"' && !single_quotes)
				double_quotes = !double_quotes;
			if (c == '#' && !single_quotes && !double_quotes)
				macro = true;
		}

		if (c == '\n' && last_non_ws != '\\')
			macro = false;

		if (c > ' ' && c <= '~')
			last_non_ws = c;

		if (multi_comment || line_comment || macro) {
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

void parse_typedefs_and_enums(Map& definitions, String_Vector& tokens) {
	char *p = tokens.pool;
	char *prev = nullptr;
	char *next = nullptr;

	Arena& arena = get_default_arena();

	std::vector<int> cur_enum;
	bool is_enum = false;
	bool enum_mode = false;
	bool td_enum = false;
	bool inside_enum = false;
	bool active_enum_elem = false;
	bool start_enum_elem = false;
	char *enum_name = nullptr;
	s64 enum_value = 0;

	std::string type = "";
	bool td_mode = false;

	while (*p) {
		next = advance_word(p);
		is_enum = strcmp(p, "enum") == 0;
		bool was_typedef = prev && strcmp(prev, "typedef") == 0;
		bool was_symbol = prev && is_symbol(*prev);

		if (!td_mode && was_typedef) {
			if (!is_enum && strcmp(p, "struct") && strcmp(p, "union")) {
				clear_word(prev);
				td_mode = true;
			}
		}

		if (is_enum) {
			enum_mode = true;
			td_enum = was_typedef;

			enum_name = td_enum ? nullptr : arena.alloc_string(next);

			clear_word(p);
			if (was_typedef)
				clear_word(prev);
		}
		else {
			bool end_expr = next[0] == ';';

			if (enum_mode) {
				if (is_symbol(*p)) {
					if (*p == '{' && !inside_enum) {
						inside_enum = true;
						start_enum_elem = true;
					}
					if (*p == ',') {
						start_enum_elem = true;
					}
					if (*p == '}') {
						enum_mode = false;

						if (inside_enum) {
							char *name = td_enum ? next : enum_name;
							Bucket& buck = definitions.insert(name);
							buck.pointer = arena.store_buffer((void*)cur_enum.data(), cur_enum.size() * sizeof(int));
							buck.flags |= FLAG_EXTERNAL | FLAG_ENUM;
							if (td_enum)
								buck.flags |= FLAG_TYPEDEF;

							for (auto& e : cur_enum) {
								Bucket& elem = definitions[definitions.sv->at(e)];
								elem.parent_name_idx = buck.name_idx;
							}
						}

						td_enum = false;
						inside_enum = false;
						active_enum_elem = false;
						start_enum_elem = false;
						enum_value = 0;
						cur_enum.clear();
					}
				}
				else {
					if (start_enum_elem) {
						Bucket& buck = definitions.insert(p);
						buck.value = enum_value++;
						buck.flags |= FLAG_ENUM_ELEMENT;

						cur_enum.push_back(buck.name_idx);
						start_enum_elem = false;
					}
				}

				clear_word(p);
			}

			if (td_mode) {
				if (end_expr) {
					Bucket& buck = definitions.insert(p);
					buck.value = definitions.sv->add_buffer(type.c_str(), type.size());
					buck.flags |= FLAG_EXTERNAL | FLAG_PRIMITIVE;
					type = "";
					td_mode = false;
				}
				else {
					if (type.size())
						type += " ";

					type += p;
				}

				clear_word(p);
			}
		}

		prev = p;
		p = next;
	}
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

void embed_struct(Struct *master, int field_idx, Struct *embed) {
	Field& group = master->fields.back();
	int count = group.array_len > 0 ? group.array_len : 1;

	if (group.flags & FLAG_POINTER) {
		master->offset += PADDING(master->offset, pointer_size);

		group.bit_offset = master->offset;
		group.bit_size = count * pointer_size;

		master->offset += group.bit_size;
		return;
	}

	group.bit_offset = master->offset;
	group.bit_size = count * embed->total_size;

	int align = embed->longest_primitive;
	master->offset += PADDING(master->offset, align);

	int idx = 0;
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < embed->fields.n_fields; j++) {
			Field *f = &master->fields.add(embed->fields.data[j]);

			// These variables are empty-checked since we don't want their values to change in the case that they are embedded more than once
			if (!f->this_st)
				f->this_st = embed;
			if (f->paste_array_idx < 0)
				f->paste_array_idx = i;
			if (f->index < 0)
				f->index = idx;

			// These variables DO change every time, as we want them to be relative to the struct that 'embed' is being pasted into
			f->paste_st = master;
			f->paste_field = field_idx + idx - f->index;
			f->bit_offset += master->offset;

			idx++;
		}
		master->offset += embed->total_size;
	}
}

// When either a struct/union is defined or referenced inside another struct, a declaration list may follow.
// This function finds each element in the declaration list and embeds it into the current struct.
void add_struct_instances(Struct *current_st, Struct *embed, char **tokens, String_Vector& name_vector) {
	Field *f = &current_st->fields.back();
	char *t = *tokens;
	char *name = nullptr;
	bool first = true;

	while (*t) {
		char *next = advance_word(t);

		if (!is_symbol(*t))
			name = t;

		else if (*t == '*') {
			f->flags |= FLAG_POINTER;
			f->pointer_levels = strlen(t);
		}
		else if (*t == '[') {
			t = next;
			next = advance_word(next);

			int n = (int)evaluate_number(t).i;
			if (n > 0) {
				if (f->array_len > 0)
					f->array_len *= n;
				else
					f->array_len = n;
			}
		}
		else if (*t == ',' || *t == ';') {
			f->flags |= FLAG_COMPOSITE;
			f->st = embed;
			f->type_name_idx = embed->name_idx;

			if (name)
				f->field_name_idx = name_vector.add_string(name);

			f->paste_st = current_st;

			int parent_field = current_st->fields.n_fields - 1;
			embed_struct(current_st, parent_field, embed);

			f = &current_st->fields.add_blank();
			name = nullptr;
		}

		if (*t == ';')
			break;

		t = next;
	}

	*tokens = t;
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

void parse_c_struct(std::vector<Struct*>& structs, char **tokens, String_Vector& name_vector, Map& definitions, Struct *st) {
	std::string type;

	char *t = *tokens;
	if (*(u8*)t == 0xff)
		t = advance_word(t);

	char *prev = nullptr;

	bool top_level = false;
	if (!st) {
		st = find_new_struct(structs);
		top_level = true;
	}
	bool outside_struct = top_level;

	Field *f = nullptr;
	int field_flags = 0;

	bool typedef_decl = false;
	bool typedef_st = false;
	bool was_symbol = false;
	bool was_array_open = false;
	bool was_zero_width = false;
	bool named_field = false;
	int held_type = -1;

	// The longest primitive in a struct/union is used to determine its byte alignment.
	// The longest field in a union is the size of the union.
	int longest_field = 8;

	while (*t) {
		char *next = advance_word(t);
		bool type_over = is_symbol(*next) && *next != '*';
		if (strcmp(t, "typedef") == 0) {
			typedef_decl = true;
			typedef_st = true;
		}

		if (!f) {
			f = &st->fields.add_blank();
			f->flags = field_flags;
			named_field = false;
		}

		// Inline struct/union declaration
		if (*t == '{' && (type.size() > 0 || typedef_decl)) {
			char *name = nullptr;
			if (!typedef_decl && named_field)
				name = prev;

			if (!outside_struct) {
				bool is_union = false;
				if (typedef_decl)
					is_union = !strcmp(prev, "union");
				else if (type.size() > 0)
					is_union = !strcmp(type.c_str(), "union");

				Struct *new_st = find_new_struct(structs);
				new_st->flags |= is_union ? FLAG_UNION : 0;

				if (name)
					new_st->name_idx = name_vector.add_string(name);

				t = advance_word(t);
				parse_c_struct(structs, &t, name_vector, definitions, new_st);

				if (!typedef_decl)
					add_struct_instances(st, new_st, &t, name_vector);

				next = advance_word(t);
				f = &st->fields.back();
			}
			else {
				if (name)
					st->name_idx = name_vector.add_string(name);

				outside_struct = false;
			}

			held_type = -1;
			was_symbol = true;
			type = "";
		}
		else if (!outside_struct && is_symbol(*t)) {
			// Bitfield
			if (!strcmp(t, ":")) { // the bitfield symbol ':' must have a length of 1
				int n = (int)evaluate_number(next).i;
				if (n >= 0) {
					if (n == 0) {
						f->reset();
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
				int n = (int)evaluate_number(next).i;
				if (n > 0) {
					if (f->array_len > 0)
						f->array_len *= n;
					else
						f->array_len = n;
				}
			}

			if (*t == '*') {
				f->flags |= FLAG_POINTER;
				f->pointer_levels = strlen(t);
			}

			if (*t == '=') {
				f->flags |= FLAG_VALUE_INITED;
				f->value = evaluate_number(next, f->flags & FLAG_FLOAT);
/*
				std::string msg;
				if (f->field_name_idx >= 0)
					msg = name_vector.at(f->field_name_idx);
				else
					msg = "<unnamed>";

				msg += " = ";
				msg += (f->flags & FLAG_FLOAT) ? std::to_string(f->value.d) : std::to_string(f->value.i);
				sdl_log_string(msg.c_str());
*/
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

				f->bit_offset = st->offset;
				if ((st->flags & FLAG_UNION) == 0)
					st->offset += f->bit_size;

				f->this_st = f->paste_st = st;

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
				typedef_decl = false;
			}

			was_symbol = *t != '*';
		}
		else {
			if (is_symbol(*t)) {
				held_type = -1;
				type = "";
				was_symbol = *t != '*';

				if (*t == ';') {
					typedef_st = false;
					typedef_decl = false;
				}
			}
			else {
				if (held_type < 0) {
					// Determine if this field has a name (unnamed fields can be legal, eg. zero-width bitfields)
					if (type_over) {
						//named_field = true;
						u32 prim_mask = FLAG_OCCUPIED | FLAG_PRIMITIVE;
						named_field = (definitions[t].flags & prim_mask) != prim_mask;

						if (named_field)
							named_field = strcmp(t, "struct") != 0 && strcmp(t, "union") != 0;
					}

					// Ignore "const" when determining the field type
					if ((!type_over && strcmp(t, "const")) || (type_over && !named_field)) {
						if (type.size() > 0)
							type += ' ';
						type += t;
					}
				}
				else
					named_field = true;

				was_symbol = false;
			}
		}

		if (held_type >= 0)
			type = name_vector.at(held_type);

		// Determine the type of the current field.
		// This code may run multiple times per field,
		//  as there some primitive types that use more than one keyword (eg. "long long")
		if (f && type_over && type.size() > 0 && (f->flags & FLAG_COMPOSITE) == 0) {
			Bucket& p = definitions[type.c_str()];

			// If the type string matches a known primitive type
			const u32 prim_mask = FLAG_OCCUPIED | FLAG_PRIMITIVE;
			if ((p.flags & prim_mask) == prim_mask) {

				const u32 ext_mask = prim_mask | FLAG_EXTERNAL;
				int old_value = p.value;
				while ((p.flags & ext_mask) == ext_mask) {
					p = definitions[definitions.sv->at(p.value)];
					if (p.value == old_value)
						break;
				}

				if ((p.flags & prim_mask) == prim_mask) {
					f->flags |= p.flags & PRIMITIVE_FLAGS;
					f->default_bit_size = p.value;
					if ((f->flags & FLAG_BITFIELD) == 0 || f->bit_size > f->default_bit_size)
						f->bit_size = f->default_bit_size;
				}
			}

			// Check if the type refers to an existing struct/union
			else {
				size_t last_space = type.find_last_of(' ');
				std::string type_name = last_space == std::string::npos ? type : type.substr(last_space + 1);

				// TODO: Enum lookup goes here!!!

				// If so, then add its following declaration list to this struct
				Struct *record = lookup_struct(structs, st, f, name_vector.pool, (char*)type_name.c_str());
				if (record) {
					add_struct_instances(st, record, &t, name_vector);
					next = advance_word(t);
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
			if (typedef_st && !is_symbol(*t))
				st->name_idx = name_vector.add_string(t);

			typedef_st = false;

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

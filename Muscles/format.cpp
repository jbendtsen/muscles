#include "muscles.h"

char *format_field_name(Arena& arena, String_Vector& in_vec, Field& field) {
	char *fname = in_vec.at(field.field_name_idx);
	if (!fname) fname = (char*)"???";

	if (field.paste_field < 0) {
		field.parent_tag = nullptr;
		return fname;
	}

	Field *current = &field.paste_st->fields.data[field.paste_field];
	int len = strlen(fname);

	int ptag_len = 0;
	if (current->parent_tag) {
		ptag_len = strlen(current->parent_tag);
		len += ptag_len + 1;
	}

	int array_idx = -1;
	if (current->array_len > 1) {
		len += 12;
		array_idx = field.paste_array_idx;
	}

	char *pname = in_vec.at(current->field_name_idx);
	if (!pname) pname = (char*)"???";

	int pn_len = strlen(pname);
	len += pn_len + 1;

	char *name = (char*)arena.allocate(len + 1);
	char *p = name;

	if (ptag_len) {
		strcpy(p, current->parent_tag);
		p += ptag_len;
		*p++ = '.';
	}

	strcpy(p, pname);
	p += pn_len;

	if (array_idx >= 0)
		p += snprintf(p, 12, "[%d]", array_idx);

	field.parent_tag = arena.alloc_string(name);

	*p++ = '.';
	strcpy(p, fname);

	return name;
}

char *format_type_name(Arena& arena, String_Vector& in_vec, Field& field) {
	char *type_name = in_vec.at(field.type_name_idx);
	if (!type_name) type_name = (char*)"???";

	int name_len = strlen(type_name);
	int extra_len = field.pointer_levels + 12;
	char *name = (char*)arena.allocate(name_len + extra_len);

	char *p = name;
	for (int j = 0; j < field.pointer_levels; j++)
		*p++ = '*';

	strcpy(p, type_name);

	if (field.array_len > 0) {
		p += name_len;
		snprintf(p, extra_len - 1, "[%d]", field.array_len);
	}

	return name;
}
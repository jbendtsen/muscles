#include "muscles.h"

char *format_field_name(Arena& arena, String_Vector& in_vec, Struct& st, Field& field) {
	char *fname = in_vec.at(field.field_name_idx);
	if (!fname) fname = (char*)"???";

	if (field.parent_field < 0)
		return fname;

	Field& parent = st.fields.data[field.parent_field];
	int len = strlen(fname);

	char *opening = parent.tag;
	int opening_len = 0;
	if (opening) {
		opening_len = strlen(opening);
		len += opening_len + 1;
	}

	int array_idx = -1;
	if (parent.array_len > 1) {
		len += 12;
		array_idx = field.parent_idx;
	}

	char *pname = in_vec.at(parent.field_name_idx);
	if (!pname) pname = (char*)"???";

	int pn_len = strlen(pname);
	len += pn_len + 2;

	char *name = (char*)arena.allocate(len + 1);
	char *p = name;

	if (opening) {
		strcpy(p, opening);
		p += opening_len;
		*p++ = '.';
	}

	strcpy(p, pname);
	p += pn_len;

	if (array_idx >= 0)
		p += snprintf(p, 12, "[%d]", array_idx);

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
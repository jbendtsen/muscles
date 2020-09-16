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

void format_field_value(Field& field, Value_Format& format, Span& span, char*& cell) {
	if (field.array_len > 0 || field.flags & FLAG_COMPOSITE || field.bit_size > 64 || field.bit_size <= 0)
		return;

	int offset = (field.bit_offset + 7) / 8;
	if (offset >= span.retrieved) {
		strcpy(cell, "???");
		return;
	}

	if (field.array_len == 0 && field.bit_size <= 64) {
		u64 n = 0;
		bool byte_aligned = (field.bit_offset % 8 == 0) && (field.bit_size % 8 == 0);
		if (byte_aligned) {
			int bytes = field.bit_size / 8;
			if (field.flags & FLAG_BIG_ENDIAN) {
				for (int i = 0; i < bytes; i++)
					n = (n << 8) | span.data[offset + i];
			}
			else
				memcpy(&n, &span.data[offset], bytes);
		}
		else { // Endian-ness is ignored when reading bit fields (ie. bit fields are big-endian only)
			int off = field.bit_offset;
			int out_pos = field.bit_size - 1;
			for (int j = 0; j < field.bit_size; j++, off++, out_pos--) {
				u8 byte = span.data[off / 8];
				int bit = 7 - (off % 8);
				n |= (u64)((byte >> bit) & 1) << out_pos;
			}
		}

		strcpy(cell, "0x");
		int digits = count_hex_digits(n);
		write_hex(&cell[2], n, digits);
	}
}
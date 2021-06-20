#pragma once

enum StringStyle {
	StringAuto = 0,
	StringNone,
	StringBA,
	StringAscii
};

enum BracketsStyle {
	BracketsAuto = 0,
	BracketsNone,
	BracketsSquare,
	BracketsParen,
	BracketsCurly
};

enum PrecisionStyle {
	PrecisionAuto = -2,
	PrecisionField
};

enum FloatStyle {
	FloatNone = 0,
	FloatAuto,
	FloatFixed,
	FloatScientific
};

enum SignStyle {
	SignAuto = 0,
	SignSigned,
	SignUnsigned
};

#define SEPARATOR_LEN 4
#define PREFIX_LEN    4

struct Value_Format {
	enum StringStyle string = StringAuto;
	enum BracketsStyle brackets = BracketsAuto;
	char separator[4] = {0};
	char prefix[4] = {0};
	int base = 10;
	union {
		enum PrecisionStyle precision = PrecisionAuto;
		int digits;
	};
	enum FloatStyle floatfmt = FloatNone;
	bool uppercase = false;
	enum SignStyle sign = SignAuto;
	bool big_endian = false;
};

char *format_field_name(Arena& arena, String_Vector& in_vec, Field& field);
char *format_type_name(Arena& arena, String_Vector& in_vec, Field& field);

void format_field_value(Field& field, Value_Format& format, Span& span, char*& cell, int cell_len);


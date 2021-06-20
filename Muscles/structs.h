#pragma once

#define FLAG_POINTER       0x0001
#define FLAG_BITFIELD      0x0002
#define FLAG_UNNAMED       0x0004
#define FLAG_COMPOSITE     0x0008
#define FLAG_UNION         0x0010
#define FLAG_SIGNED        0x0020
#define FLAG_FLOAT         0x0040
#define FLAG_BIG_ENDIAN    0x0080
#define FLAG_PRIMITIVE     0x0100
#define FLAG_ENUM          0x0200
#define FLAG_ENUM_ELEMENT  0x0400
#define FLAG_TYPEDEF       0x0800
#define FLAG_VALUE_INITED  0x1000

#define FIELD_FLAGS        0x1fff
#define PRIMITIVE_FLAGS    0x00ff

#define FLAG_UNUSABLE      0x4000
#define FLAG_UNRECOGNISED  0x8000

#define FLAG_AVAILABLE  0x10000

#define FLAG_OCCUPIED  0x80000000
#define FLAG_NEW       0x40000000
#define FLAG_EXTERNAL  0x20000000

struct Struct;

struct Field {
	int field_name_idx;  // These would be strings, except that they point into a String_Vector
	int type_name_idx;
	Struct *st;          // The struct/union that the field refers to, if it's a composite field
	Struct *this_st;     // The struct that the field originally belongs to
	Struct *paste_st;    // The struct that this field currently lives in
	char *parent_tag;    // Used for the accumulated name of the struct that owns this field
	int paste_array_idx; // In case this field is embedded inside an array of structs, this keeps track of the struct index
	int paste_field;     // Refers to the field where the this field's original struct has been embedded/instantiated/pasted
	int index;           // The index of this field within its original struct
	int bit_offset;
	int bit_size;
	int default_bit_size;
	int array_len;
	int pointer_levels;
	u32 flags;

	Value64 value;

	void reset() {
		memset(this, 0, sizeof(Field));
		field_name_idx = type_name_idx = paste_field = paste_array_idx = index = -1;
	}
};

struct Field_Vector {
	Field *data = nullptr;
	int pool_size = 8;
	int n_fields = 0;

	Field_Vector();
	~Field_Vector();

	void expand();

	Field& back();
	Field& add(Field& f);
	Field& add_blank();

	void cancel_latest();
	void zero_out();
};

struct Struct {
	int name_idx;
	int offset;
	int total_size;
	int longest_primitive;
	u32 flags;
	Field_Vector fields;
};

bool is_struct_usable(Struct *s);
void set_primitives(Map& definitions);
Value64 evaluate_number(const char *token, bool as_float = false);
int get_full_field_name(Field& field, String_Vector& name_vector, String_Vector& out_vec);

void tokenize(String_Vector& tokens, const char *text, int sz);
void parse_typedefs_and_enums(Map& definitions, String_Vector& tokens);
void parse_c_struct(std::vector<Struct*>& structs, char **tokens, String_Vector& name_vector, Map& definitions, Struct *st = nullptr);


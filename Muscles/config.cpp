#include "muscles.h"
#include "ui.h"

void load_config(Workspace& ws, std::string& path) {
	Map colors;
	colors.insert("background").value        = 0x000000ff;
	colors.insert("text").value              = 0xffffffff;
	colors.insert("ph_text").value           = 0xffffffb2;
	colors.insert("outline").value           = 0x0c3380ff; //0x6699ccff;
	colors.insert("back").value              = 0x1933b2ff;
	colors.insert("dark").value              = 0x0c3380ff;
	colors.insert("light").value             = 0x2659d8ff;
	colors.insert("light_hl").value          = 0x3380ffff;
	colors.insert("table_cb_back").value     = 0x000c33ff;
	colors.insert("hl").value                = 0x3366b2ff;
	colors.insert("active").value            = 0x1940ccff;
	colors.insert("inactive").value          = 0x334c80ff;
	colors.insert("inactive_text").value     = 0xb2b2b2ff;
	colors.insert("inactive_outline").value  = 0x8c99a5ff;
	colors.insert("scroll_back").value       = 0x002666ff;
	colors.insert("scroll").value            = 0x66728cff;
	colors.insert("scroll_hl").value         = 0x99a0b2ff;
	colors.insert("scroll_sel").value        = 0xccccccff;
	colors.insert("div").value               = 0x8099ccff;
	colors.insert("caret").value             = 0xe5e5e5ff;
	colors.insert("cb").value                = 0x8cb2e5ff;
	colors.insert("sel").value               = 0x728099ff;
	colors.insert("editor").value            = 0x122060ff;
	colors.insert("folder_dark").value       = 0xa5a326ff;
	colors.insert("folder_light").value      = 0xe0d642ff;
	colors.insert("process_back").value      = 0xccccccff;
	colors.insert("process_outline").value   = 0x999999ff;
	colors.insert("file_back").value         = 0xccccccff;
	colors.insert("file_fold").value         = 0xe5e5e5ff;
	colors.insert("file_line").value         = 0x808080ff;
	colors.insert("cancel").value            = 0xffffffff;

	set_primitives(ws.definitions);

	auto cfg_file = read_file(path);
	if (cfg_file.second) {
		String_Vector tokens;
		tokenize(tokens, (const char*)cfg_file.second.get(), cfg_file.first);

		std::vector<Struct*> structs;
		char *tokens_alias = tokens.pool;
		String_Vector name_vector;
		parse_c_struct(structs, &tokens_alias, name_vector, ws.definitions);

		for (auto& s : structs) {
			if (s->name_idx < 0)
				continue;

			char *name = name_vector.at(s->name_idx);
			if (!strcmp(name, "Colors")) {
				for (int i = 0; i < s->fields.n_fields; i++) {
					Field& f = s->fields.data[i];
					if (f.field_name_idx < 0 || (f.flags & FIELD_FLAGS) != FLAG_VALUE_INITED)
						continue;

					char *attr = name_vector.at(f.field_name_idx);
					Bucket& buck = colors[attr];
					if (buck.flags & FLAG_OCCUPIED)
						buck.value = f.value.i;
				}
			}
		}
	}

	auto set_color = [](RGBA& color, u64 value) {
		color.r = (float)((value >> 24) & 0xff) / 255.0f;
		color.g = (float)((value >> 16) & 0xff) / 255.0f;
		color.b = (float)((value >> 8) & 0xff) / 255.0f;
		color.a = (float)(value & 0xff) / 255.0f;
	};

	#define SET_COLOR(attr) set_color(ws.colors.attr, colors[#attr].value);

	SET_COLOR(background)
	SET_COLOR(text)
	SET_COLOR(ph_text)
	SET_COLOR(outline)
	SET_COLOR(back)
	SET_COLOR(dark)
	SET_COLOR(light)
	SET_COLOR(light_hl)
	SET_COLOR(table_cb_back)
	SET_COLOR(hl)
	SET_COLOR(active)
	SET_COLOR(inactive)
	SET_COLOR(inactive_text)
	SET_COLOR(inactive_outline)
	SET_COLOR(scroll_back)
	SET_COLOR(scroll)
	SET_COLOR(scroll_hl)
	SET_COLOR(scroll_sel)
	SET_COLOR(div)
	SET_COLOR(caret)
	SET_COLOR(cb)
	SET_COLOR(sel)
	SET_COLOR(editor)
	SET_COLOR(folder_dark)
	SET_COLOR(folder_light)
	SET_COLOR(process_back)
	SET_COLOR(process_outline)
	SET_COLOR(file_back)
	SET_COLOR(file_fold)
	SET_COLOR(file_line)
	SET_COLOR(cancel)
}

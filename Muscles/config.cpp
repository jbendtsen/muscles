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
	colors.insert("folder_dark").value       = 0xa5a326ff;
	colors.insert("folder_light").value      = 0xe0d642ff;
	colors.insert("process_back").value      = 0xccccccff;
	colors.insert("process_outline").value   = 0x999999ff;
	colors.insert("file_back").value         = 0xccccccff;
	colors.insert("file_fold").value         = 0xe5e5e5ff;
	colors.insert("file_line").value         = 0x808080ff;
	colors.insert("cancel").value            = 0xffffffff;

	auto cfg_file = read_file(path);
	if (cfg_file.second) {
		Map defs;
		set_primitives(defs);

		String_Vector tokens;
		tokenize(tokens, (const char*)cfg_file.second.get(), cfg_file.first);

		std::vector<Struct*> structs;
		char *tokens_alias = tokens.pool;
		String_Vector name_vector;
		parse_c_struct(structs, &tokens_alias, name_vector, defs);

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

	set_color(ws.colors.background, colors["background"].value);
	set_color(ws.colors.text, colors["text"].value);
	set_color(ws.colors.ph_text, colors["ph_text"].value);
	set_color(ws.colors.outline, colors["outline"].value);
	set_color(ws.colors.back, colors["back"].value);
	set_color(ws.colors.dark, colors["dark"].value);
	set_color(ws.colors.light, colors["light"].value);
	set_color(ws.colors.light_hl, colors["light_hl"].value);
	set_color(ws.colors.table_cb_back, colors["table_cb_back"].value);
	set_color(ws.colors.hl, colors["hl"].value);
	set_color(ws.colors.active, colors["active"].value);
	set_color(ws.colors.inactive, colors["inactive"].value);
	set_color(ws.colors.inactive_text, colors["inactive_text"].value);
	set_color(ws.colors.inactive_outline, colors["inactive_outline"].value);
	set_color(ws.colors.scroll_back, colors["scroll_back"].value);
	set_color(ws.colors.scroll, colors["scroll"].value);
	set_color(ws.colors.scroll_hl, colors["scroll_hl"].value);
	set_color(ws.colors.scroll_sel, colors["scroll_sel"].value);
	set_color(ws.colors.div, colors["div"].value);
	set_color(ws.colors.caret, colors["caret"].value);
	set_color(ws.colors.cb, colors["cb"].value);
	set_color(ws.colors.sel, colors["sel"].value);
	set_color(ws.colors.folder_dark, colors["folder_dark"].value);
	set_color(ws.colors.folder_light, colors["folder_light"].value);
	set_color(ws.colors.process_back, colors["process_back"].value);
	set_color(ws.colors.process_outline, colors["process_outline"].value);
	set_color(ws.colors.file_back, colors["file_back"].value);
	set_color(ws.colors.file_fold, colors["file_fold"].value);
	set_color(ws.colors.file_line, colors["file_line"].value);
	set_color(ws.colors.cancel, colors["cancel"].value);
}

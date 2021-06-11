#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

#define LABEL_HEIGHT_FACTOR 1.2f
#define EDIT_HEIGHT_FACTOR  1.4f

#define MIN_OBJECT_VIEW_HEIGHT 100
#define MAX_DIVIDER_GAP_BOTTOM 150

void Search_Menu::update_ui(Camera& view) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

	float title_h = title.font->render.text_height() / view.scale;
	title.outer_box = {
		.x = border,
		.y = 0,
		.w = box.w - 2*border,
		.h = title_h + 2*border
	};

	float label_h = label_font->render.text_height() * LABEL_HEIGHT_FACTOR / view.scale;
	float edit_h  = source_edit.font->render.text_height() * EDIT_HEIGHT_FACTOR / view.scale;

	float start_x = 2*border;
	float y = title.outer_box.h;

	reveal_btn.pos = {
		.x = box.w - reveal_btn_length - start_x,
		.y = y + 3*border,
		.w = reveal_btn_length,
		.h = reveal_btn_length
	};

	const bool is_obj = menu_type == MenuObject;

	if (params_revealed) {
		float beneath_top_label = label_h + border;
		float source_x = start_x;

		if (is_obj) {
			float struct_w = box.w / 3;

			struct_lbl.pos = {
				.x = start_x,
				.y = y,
				.w = struct_w,
				.h = label_h
			};

			struct_edit.pos = {
				.x = start_x,
				.y = y + beneath_top_label,
				.w = struct_w,
				.h = edit_h
			};

			source_x += struct_w + start_x;
		}

		source_lbl.pos = {
			.x = source_x,
			.y = y,
			.w = 80,
			.h = label_h
		};
		y += beneath_top_label;

		source_edit.pos = {
			.x = source_x,
			.y = y,
			.w = reveal_btn.pos.x - start_x - source_x,
			.h = edit_h
		};
		y += source_edit.pos.h + 2*border;

		addr_lbl.pos = {
			.x = start_x,
			.y = y,
			.w = 200,
			.h = label_h
		};
		y += addr_lbl.pos.h + border;

		float addr_w = (box.w - 4*start_x) / 2;

		start_addr_edit.pos = {
			.x = start_x,
			.y = y,
			.w = addr_w,
			.h = edit_h
		};

		end_addr_edit.pos = {
			.x = start_x + box.w / 2,
			.y = y,
			.w = addr_w,
			.h = edit_h
		};
		y += end_addr_edit.pos.h + 2*border;

		if (is_obj) {
			object_lbl.pos = {
				.x = start_x,
				.y = y,
				.w = 140,
				.h = label_h
			};
			y += object_lbl.pos.h + border;

			const float table_div_gap = start_x;

			if (!object_div.minimum) {
				object_div.minimum = y + MIN_OBJECT_VIEW_HEIGHT + table_div_gap;
				object_div.position = object_div.minimum;
			}

			object_div.maximum = box.h - MAX_DIVIDER_GAP_BOTTOM;
			if (object_div.maximum < object_div.minimum)
				object_div.maximum = object_div.minimum;

			object.pos = {
				.x = start_x,
				.y = y,
				.w = box.w - 2*start_x,
				.h = object_div.position - table_div_gap - y
			};

			y = object_div.position;

			object_div.pos = {
				.x = start_x,
				.y = object_div.position,
				.w = box.w - 2*start_x,
				.h = object_div.breadth
			};
		}
		else {
			type_lbl.pos = {
				.x = start_x,
				.y = y,
				.w = 140,
				.h = label_h
			};

			method_lbl.pos = {
				.x = 3*start_x + type_lbl.pos.w,
				.y = y,
				.w = 120,
				.h = label_h
			};
			y += method_lbl.pos.h + border;

			type_edit.pos = {
				.x = start_x,
				.y = y,
				.w = type_lbl.pos.w,
				.h = edit_h
			};

			method_dd.pos = {
				.x = method_lbl.pos.x,
				.y = y,
				.w = method_lbl.pos.w,
				.h = edit_h
			};
			y += method_dd.pos.h + 2*border;

			value_lbl.pos = {
				.x = start_x,
				.y = y,
				.w = 100,
				.h = label_h
			};
			y += value_lbl.pos.h + border;

			value1_edit.pos = {
				.x = start_x,
				.y = y,
				.w = start_addr_edit.pos.w,
				.h = edit_h,
			};
			value2_edit.pos = {
				.x = end_addr_edit.pos.x,
				.y = y,
				.w = end_addr_edit.pos.w,
				.h = edit_h
			};
			y += value2_edit.pos.h;
		}
	}

	y += 5*border;

	// progress_bar.pos = {...

	float total_btn_w = search_btn.width + start_x + cancel_btn.width + start_x + reset_btn.width;

	search_btn.pos = {
		.x = (box.w - total_btn_w) / 2,
		.y = y,
		.w = search_btn.width,
		.h = search_btn.height
	};
	cancel_btn.pos = {
		.x = search_btn.pos.x + search_btn.pos.w + start_x,
		.y = y,
		.w = cancel_btn.width,
		.h = cancel_btn.height
	};
	reset_btn.pos = {
		.x = cancel_btn.pos.x + cancel_btn.pos.w + start_x,
		.y = y,
		.w = reset_btn.width,
		.h = reset_btn.height
	};
	y += search_btn.pos.h + 2*border;

	float tall_label_h = results_lbl.font->render.text_height() * LABEL_HEIGHT_FACTOR / view.scale;
	float rc_width = results_count_lbl.font->render.text_width(results_count_lbl.text.c_str()) * 1.1f / view.scale;

	results_lbl.pos = {
		.x = start_x,
		.y = y,
		.w = 150,
		.h = tall_label_h
	};
	results_count_lbl.pos = {
		.x = box.w - start_x - rc_width,
		.y = y,
		.w = rc_width,
		.h = tall_label_h
	};
	y += tall_label_h + border;

	results.pos = {
		.x = start_x,
		.y = y,
		.w = box.w - 2*start_x,
		.h = box.h - start_x - y
	};
}

void search_struct_edit_handler(Edit_Box *edit, Input& input) {
	auto sm = dynamic_cast<Search_Menu*>(edit->parent);
	Workspace& ws = *edit->parent->parent;
	populate_object_table<Search_Menu>(sm, ws.structs, ws.name_vector);
}

void search_source_edit_handler(Edit_Box *edit, Input& input) {
	auto sm = dynamic_cast<Search_Menu*>(edit->parent);

	Source *src = nullptr;

	if (edit->editor.text.size() > 0) {
		Workspace& ws = *edit->parent->parent;
		for (auto& s : ws.sources) {
			if (s->name == edit->editor.text) {
				src = s;
				break;
			}
		}
	}

	sm->source = src;
}

void search_address_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_type_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_method_dd_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);
	sm->value2_edit.visible = sm->method_dd.sel == 1;
	sm->value2_edit.needs_redraw = true;
}

void search_value_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_object_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	
}

void search_search_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	if (!sm->source || sm->method_dd.sel < 0)
		return;

	std::string& type = sm->type_edit.editor.text;
	if (type.size() == 0)
		return;

	Workspace& ws = *elem->parent->parent;
	Bucket& buck = ws.definitions[type.c_str()];

	const u32 min_flags = FLAG_OCCUPIED | FLAG_PRIMITIVE;
	if ((buck.flags & min_flags) != min_flags)
		return;

	sm->cancel_btn.set_active(true);

	sm->search.params = nullptr;
	sm->search.single_value = {
		.flags = buck.flags & FIELD_FLAGS,
		.method = sm->method_dd.sel,
		.offset = 0,
		.size = (int)buck.value,
		.value1 = evaluate_number(sm->value1_edit.editor.text.c_str(), buck.flags & FLAG_FLOAT).i,
		.value2 = evaluate_number(sm->value2_edit.editor.text.c_str(), buck.flags & FLAG_FLOAT).i
	};

	sm->search.start_addr = evaluate_number(sm->start_addr_edit.editor.text.c_str()).i;
	sm->search.end_addr = evaluate_number(sm->end_addr_edit.editor.text.c_str()).i;

	sm->search.source_type = sm->source->type;
	sm->search.pid = sm->source->pid;
	sm->search.identifier = sm->source->identifier;

	start_search(sm->search, sm->source->regions);
}

void search_cancel_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	
}

void search_reset_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	reset_search();
	sm->results_table.resize(0);
	sm->results_count_lbl.text = "";
	sm->require_redraw();
}

void search_reveal_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	sm->params_revealed = !sm->params_revealed;

	sm->source_lbl.visible = sm->params_revealed;
	sm->source_edit.visible = sm->params_revealed;
	sm->addr_lbl.visible = sm->params_revealed;
	sm->start_addr_edit.visible = sm->params_revealed;
	sm->end_addr_edit.visible = sm->params_revealed;
	sm->type_lbl.visible = sm->params_revealed;
	sm->type_edit.visible = sm->params_revealed;
	sm->method_lbl.visible = sm->params_revealed;
	sm->method_dd.visible = sm->params_revealed;
	sm->value_lbl.visible = sm->params_revealed;
	sm->value1_edit.visible = sm->params_revealed;
	sm->value2_edit.visible = sm->params_revealed;

	sm->update_reveal_button(view.scale);
}

void search_results_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	if (dbl_click) {
		if (!sm->source || sm->results.sel_row < 0)
			return;

		u64 address = (u64)sm->results_table.columns[0][sm->results.sel_row];
		sm->parent->view_source_at(sm->source, address);
	}
}

void Search_Menu::update_reveal_button(float scale) {
	int size = 0.5 + reveal_btn_length * scale;

	// we call sdl_destroy_texture() here since this function is also called when the icon needs to flip between up and down
	sdl_destroy_texture(&reveal_btn.icon);
	reveal_btn.icon = make_triangle(reveal_btn.default_color, size, size, params_revealed);
}

void Search_Menu::refresh(Point *cursor) {
	Workspace& ws = *parent;

	struct_dd.external = &ws.struct_names;

	int n_sources = ws.sources.size();
	source_dd.content.resize(n_sources);
	for (int i = 0; i < n_sources; i++)
		source_dd.content[i] = (char*)ws.sources[i]->name.c_str();

	type_dd.content.resize(0);
	const u32 flags = FLAG_OCCUPIED | FLAG_PRIMITIVE;

	for (int i = 0; i < ws.definitions.size(); i++) {
		if ((ws.definitions.data[i].flags & flags) == flags) {
			char *key = ws.definitions.key_from_index(i);
			type_dd.content.push_back(key);
		}
	}

	//results_count_lbl.text = std::to_string(search.results.size());

	if (check_search_finished()) {
		cancel_btn.set_active(false);

		get_search_results((std::vector<u64>&)results_table.columns[0]);

		int n_results = results_table.columns[0].size();
		results_table.columns[1].resize(n_results, nullptr);

		results_count_lbl.text = std::to_string(n_results);

		results_count_lbl.needs_redraw = true;
		results.needs_redraw = true;
	}
}

void Search_Menu::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	float edit_h = source_edit.font->render.text_height() * EDIT_HEIGHT_FACTOR / new_scale;
	source_edit.update_icon(IconTriangle, edit_h, new_scale);

	if (menu_type == MenuObject) {
		struct_edit.update_icon(IconTriangle, edit_h, new_scale);
		object_div.make_icon(new_scale);
	}
	else
		type_edit.update_icon(IconTriangle, edit_h, new_scale);

	sdl_destroy_texture(&method_dd.icon);
	method_dd.icon_length = dd_font->render.text_height() * EDIT_HEIGHT_FACTOR;
	method_dd.icon = make_triangle(method_dd.icon_color, method_dd.icon_length, method_dd.icon_length);

	sdl_destroy_texture(&search_btn.icon);
	float search_h = search_btn.get_icon_length(new_scale);
	search_btn.icon = make_glass_icon(search_btn.default_color, search_h);
	search_btn.update_size(new_scale);

	update_reveal_button(new_scale);
}

Search_Menu::Search_Menu(Workspace& ws, MenuType mtype) {
	float scale = get_default_camera().scale;

	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;
	ui.push_back(&maxm);

	title.font = ws.default_font;
	title.text = mtype == MenuValue ? "Value Search" : "Object Search";
	ui.push_back(&title);

	label_font = ws.make_font(11, ws.colors.text, scale);
	dd_font = label_font;

	RGBA icon_color = ws.colors.text;
	icon_color.a = 0.7;

	const bool is_obj = mtype == MenuObject;
	if (is_obj) {
		struct_lbl.font = label_font;
		struct_lbl.text = "Struct";
		ui.push_back(&struct_lbl);

		struct_edit.font = label_font;
		struct_edit.caret = ws.colors.caret;
		struct_edit.default_color = ws.colors.dark;
		struct_edit.icon_color = icon_color;
		struct_edit.icon_right = true;
		struct_edit.dropdown = &struct_dd;
		struct_edit.key_action = search_struct_edit_handler;
		ui.push_back(&struct_edit);

		struct_dd.visible = false;
		struct_dd.font = label_font;
		struct_dd.default_color = ws.colors.back;
		struct_dd.hl_color = ws.colors.hl;
		struct_dd.sel_color = ws.colors.active;
		ui.push_back(&struct_dd);
	}

	source_lbl.font = label_font;
	source_lbl.text = "Source";
	ui.push_back(&source_lbl);

	source_edit.font = label_font;
	source_edit.caret = ws.colors.caret;
	source_edit.default_color = ws.colors.dark;
	source_edit.icon_color = icon_color;
	source_edit.icon_right = true;
	source_edit.dropdown = &source_dd;
	source_edit.key_action = search_source_edit_handler;
	ui.push_back(&source_edit);

	source_dd.visible = false;
	source_dd.font = dd_font;
	source_dd.default_color = ws.colors.back;
	source_dd.hl_color = ws.colors.hl;
	source_dd.sel_color = ws.colors.active;
	ui.push_back(&source_dd);

	addr_lbl.font = label_font;
	addr_lbl.text = "Address Space";
	ui.push_back(&addr_lbl);

	start_addr_edit.font = label_font;
	start_addr_edit.editor.text = "0x" + std::string(16, '0');
	start_addr_edit.caret = ws.colors.caret;
	start_addr_edit.default_color = ws.colors.dark;
	start_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&start_addr_edit);

	end_addr_edit.font = label_font;
	end_addr_edit.editor.text = "0x" + std::string(16, 'f');
	end_addr_edit.caret = ws.colors.caret;
	end_addr_edit.default_color = ws.colors.dark;
	end_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&end_addr_edit);

	if (is_obj) {
		object_lbl.font = label_font;
		object_lbl.text = "Object";
		ui.push_back(&object_lbl);

		Column obj_cols[] = {
			{ColumnCheckbox, 0, 0.05, 1.0, 1.0, ""},
			{ColumnString,   0, 0.15, 0, 0, "Type"},
			{ColumnString,   0, 0.25, 0, 0, "Field"},
			{ColumnElement,  0, 0.15, 0, 0, "Method"},
			{ColumnElement,  0, 0.20, 0, 0, "Value 1"},
			{ColumnElement,  0, 0.20, 0, 0, "Value 2"}
		};
		object_table.init(obj_cols, nullptr, 6, 0);

		object.font = label_font;
		object.data = &object_table;
		object.action = search_object_handler;
		object.default_color = ws.colors.dark;
		object.back_color = ws.colors.dark;
		object.hl_color = ws.colors.hl;
		object.sel_color = ws.colors.hl;
		object.font = ws.default_font;
		object.vscroll = &object_scroll;
		//object.show_column_names = true;
		ui.push_back(&object);

		object_scroll.content = &object;
		ui.push_back(&object_scroll);

		object_div.default_color = ws.colors.div;
		object_div.breadth = 2;
		object_div.moveable = true;
		object_div.minimum = 0;
		object_div.position = 0;
		object_div.cursor_type = CursorResizeNorthSouth;
		ui.push_back(&object_div);
	}
	else {
		type_lbl.font = label_font;
		type_lbl.text = "Type";
		ui.push_back(&type_lbl);

		type_edit.font = label_font;
		type_edit.editor.text = "int32_t";
		type_edit.caret = ws.colors.caret;
		type_edit.default_color = ws.colors.dark;
		type_edit.icon_color = icon_color;
		type_edit.icon_right = true;
		type_edit.dropdown = &type_dd;
		type_edit.key_action = search_type_edit_handler;
		ui.push_back(&type_edit);

		type_dd.visible = false;
		type_dd.font = dd_font;
		type_dd.default_color = ws.colors.back;
		type_dd.hl_color = ws.colors.hl;
		type_dd.sel_color = ws.colors.active;
		ui.push_back(&type_dd);

		method_lbl.font = label_font;
		method_lbl.text = "Method";
		ui.push_back(&method_lbl);

		method_dd.font = dd_font;
		method_dd.action = search_method_dd_handler;
		method_dd.default_color = ws.colors.dark;
		method_dd.hl_color = ws.colors.hl;
		method_dd.sel_color = ws.colors.active;
		method_dd.icon_color = icon_color;
		method_dd.content = {
			(char*)"Equals",
			(char*)"Range"
		};
		method_dd.keep_selected = true;
		method_dd.sel = 0;
		ui.push_back(&method_dd);

		value_lbl.font = label_font;
		value_lbl.text = "Value";
		ui.push_back(&value_lbl);

		value1_edit.font = label_font;
		value1_edit.caret = ws.colors.caret;
		value1_edit.default_color = ws.colors.dark;
		value1_edit.key_action = search_value_edit_handler;
		ui.push_back(&value1_edit);

		value2_edit.visible = false;
		value2_edit.font = label_font;
		value2_edit.caret = ws.colors.caret;
		value2_edit.default_color = ws.colors.dark;
		value2_edit.key_action = search_value_edit_handler;
		ui.push_back(&value2_edit);
	}

	//Progress_Bar progress_bar;

	search_btn.text = "Search";
	search_btn.action = search_search_btn_handler;
	search_btn.default_color = ws.colors.text;
	search_btn.default_color.a = 0.7;
	search_btn.active_theme = {
		ws.colors.light,
		ws.colors.hl,
		ws.colors.hl,
		ws.default_font
	};
	search_btn.inactive_theme = {
		ws.colors.inactive,
		ws.colors.inactive,
		ws.colors.inactive,
		ws.default_font
	};
	search_btn.update_size(scale);
	ui.push_back(&search_btn);

	cancel_btn.text = "Cancel";
	cancel_btn.active = false;
	cancel_btn.action = search_cancel_btn_handler;
	cancel_btn.default_color = ws.colors.cancel;
	cancel_btn.default_color.a = 0.7;
	cancel_btn.active_theme = search_btn.active_theme;
	cancel_btn.inactive_theme = search_btn.inactive_theme;
	cancel_btn.update_size(scale);
	ui.push_back(&cancel_btn);

	reset_btn.text = "Reset";
	reset_btn.action = search_reset_btn_handler;
	reset_btn.active_theme = search_btn.active_theme;
	reset_btn.inactive_theme = search_btn.inactive_theme;
	reset_btn.update_size(scale);
	ui.push_back(&reset_btn);

	reveal_btn.padding = 0;
	reveal_btn.action = search_reveal_btn_handler;
	reveal_btn.active_theme = {
		ws.colors.light,
		ws.colors.light_hl,
		ws.colors.light_hl,
		nullptr
	};
	reveal_btn.inactive_theme = reveal_btn.active_theme;
	reveal_btn.default_color = ws.colors.text;
	reveal_btn.default_color.a = 0.8;
	ui.push_back(&reveal_btn);

	results_lbl.font = ws.default_font;
	results_lbl.text = "Results";
	ui.push_back(&results_lbl);

	results_count_lbl.font = results_lbl.font;
	results_count_lbl.text = "";
	ui.push_back(&results_count_lbl);

	Column cols[] = {
		{ColumnHex, 16, 0.5, 0, 0, "Address"},
		{ColumnString, 0, 0.5, 0, 0, "Value"},
	};
	results_table.init(cols, nullptr, 2, 0);

	results.font = label_font;
	results.data = &results_table;
	results.action = search_results_handler;
	results.default_color = ws.colors.dark;
	results.back_color = ws.colors.dark;
	results.hl_color = ws.colors.hl;
	results.sel_color = ws.colors.hl;
	results.font = ws.default_font;
	results.vscroll = &results_scroll;
	results.show_column_names = true;
	ui.push_back(&results);

	results_scroll.content = &results;
	ui.push_back(&results_scroll);

	initial_width = 400;
	initial_height = 450;
	min_width = 300;
	min_height = 450;
	refresh_every = 1;
}

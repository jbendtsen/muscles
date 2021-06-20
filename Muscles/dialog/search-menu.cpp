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
	const float scroll_w = 14;

	float start_x = 2*border;
	float total_w = box.w - 2*start_x;

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

		float addr_and_align_w = box.w - 4*start_x;
		float addr_w  = 0.6 * addr_and_align_w;
		float align_w = 0.4 * addr_and_align_w;

		addr_lbl.pos = {
			.x = start_x,
			.y = y,
			.w = addr_w,
			.h = label_h
		};

		align_lbl.pos = {
			.x = start_x + addr_w + 2*start_x,
			.y = 0,
			.w = align_w,
			.h = label_h
		};

		y += addr_lbl.pos.h + border;

		float addr_edit_h = start_addr_edit.font->render.text_height() * EDIT_HEIGHT_FACTOR / view.scale;
		float total_addr_edit_h = 2*addr_edit_h + border;

		start_addr_edit.pos = {
			.x = start_x,
			.y = y,
			.w = addr_w,
			.h = addr_edit_h
		};

		end_addr_edit.pos = {
			.x = start_x,
			.y = y + border + addr_edit_h,
			.w = addr_w,
			.h = addr_edit_h
		};

		align_edit.pos = {
			.x = align_lbl.pos.x,
			.y = y + (total_addr_edit_h - edit_h) / 2,
			.w = align_w,
			.h = edit_h
		};

		align_lbl.pos.y = align_edit.pos.y - border - edit_h;

		y += total_addr_edit_h + 2*border;

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
				.w = total_w - scroll_w - border,
				.h = object_div.position - table_div_gap - y
			};

			object_scroll.pos = {
				.x = start_x + total_w - scroll_w,
				.y = y,
				.w = scroll_w,
				.h = object.pos.h
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

			float value_w = (box.w - 3*start_x) / 2;

			value1_edit.pos = {
				.x = start_x,
				.y = y,
				.w = value_w,
				.h = edit_h,
			};
			value2_edit.pos = {
				.x = 2*start_x + value_w,
				.y = y,
				.w = value_w,
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
		.w = total_w - scroll_w - border,
		.h = box.h - start_x - y
	};

	results_scroll.pos = {
		.x = start_x + total_w - scroll_w,
		.y = y,
		.w = scroll_w,
		.h = results.pos.h
	};
}

void Search_Menu::set_object_row_visibility(int col, int row) {
	auto method_dd = dynamic_cast<Drop_Down*>((UI_Element*)object.data->columns[3][row]);
	auto value1_edit = dynamic_cast<Edit_Box*>((UI_Element*)object.data->columns[4][row]);
	auto value2_edit = dynamic_cast<Edit_Box*>((UI_Element*)object.data->columns[5][row]);

	method_dd->visible = true;

	if (method_dd->sel == 0) { // Equals
		value1_edit->visible = true;
		value2_edit->visible = false;
	}
	else if (method_dd->sel == 1) { // Range
		value1_edit->visible = true;
		value2_edit->visible = true;
	}
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

void search_method_dd_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);
	sm->value2_edit.visible = sm->method_dd.sel == 1;
	sm->value2_edit.needs_redraw = true;
}

void search_object_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	
}

void search_object_table_checkbox_handler(Data_View *object, int col, int row) {
	bool enabled = object->data->checkbox_checked(col, row);

	if (!enabled) {
		((UI_Element*)object->data->columns[3][row])->visible = false;
		((UI_Element*)object->data->columns[4][row])->visible = false;
		((UI_Element*)object->data->columns[5][row])->visible = false;
	}
	else {
		auto sm = dynamic_cast<Search_Menu*>(object->parent);
		sm->set_object_row_visibility(col, row);
	}
}

void search_object_table_method_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);
	sm->set_object_row_visibility(sm->object.active_col, sm->object.active_row);
}

void search_object_table_cell_init(Table *table, int col, int row) {
	auto sm = dynamic_cast<Search_Menu*>((Box*)table->element_tag);
	Workspace *ws = sm->parent;
	auto elem = (UI_Element*)table->columns[col][row];

	elem->parent = sm;
	elem->font = sm->table_font;
	elem->default_color = ws->colors.back;
	elem->hl_color = ws->colors.light;
	elem->sel_color = ws->colors.active;
	elem->visible = false;

	if (col == 3) {
		auto dd = dynamic_cast<Drop_Down*>(elem);
		dd->action = search_object_table_method_handler;
		dd->external = &sm->method_options;
		dd->keep_selected = true;
		dd->sel = 0;
		dd->title_off_y = -0.2;
		dd->leaning = 0.0;
	}
	else {
		auto edit = dynamic_cast<Edit_Box*>(elem);
		edit->text_off_y = -0.2;
	}
}

bool Search_Menu::prepare_object_params() {
	if (!is_struct_usable(record))
		return false;

	if (!params_pool)
		params_pool = new Search_Parameter[MAX_SEARCH_PARAMS];

	int n_params = 0;

	for (int i = 0; i < record->fields.n_fields && n_params < MAX_SEARCH_PARAMS; i++) {
		if (!object_table.checkbox_checked(0, i))
			continue;

		Field *f = &record->fields.data[i];

		int method = dynamic_cast<Drop_Down*>((UI_Element*)object_table.columns[3][i])->sel;

		params_pool[n_params] = {
			.flags = f->flags & FIELD_FLAGS,
			.method = method,
			.offset = f->bit_offset,
			.size = f->bit_size,
			.value1 = 0,
			.value2 = 0
		};

		if (method == METHOD_EQUALS || method == METHOD_RANGE) {
			const char *value_str = dynamic_cast<Edit_Box*>((UI_Element*)object_table.columns[4][i])->editor.text.c_str();
			params_pool[n_params].value1 = evaluate_number(value_str, f->flags & FLAG_FLOAT).i;
		}
		if (method == METHOD_RANGE) {
			const char *value_str = dynamic_cast<Edit_Box*>((UI_Element*)object_table.columns[5][i])->editor.text.c_str();
			params_pool[n_params].value2 = evaluate_number(value_str, f->flags & FLAG_FLOAT).i;
		}

		n_params++;
	}

	if (!n_params)
		return false;

	search.record = record;
	search.params = params_pool;
	search.n_params = n_params;

	search.byte_align = (int)evaluate_number(align_edit.editor.text.c_str()).i;

	search.start_addr = evaluate_number(start_addr_edit.editor.text.c_str()).i;
	search.end_addr = evaluate_number(end_addr_edit.editor.text.c_str()).i;

	search.source_type = source->type;
	search.pid = source->pid;
	search.identifier = source->identifier;

	return true;
}

bool Search_Menu::prepare_value_param() {
	if (method_dd.sel < 0)
		return false;

	std::string& type = type_edit.editor.text;
	if (type.size() == 0)
		return false;

	Bucket& buck = parent->definitions[type.c_str()];

	const u32 min_flags = FLAG_OCCUPIED | FLAG_PRIMITIVE;
	if ((buck.flags & min_flags) != min_flags)
		return false;

	search.record = nullptr;
	search.params = nullptr;
	search.n_params = 0;

	search.single_value = {
		.flags = buck.flags & FIELD_FLAGS,
		.method = method_dd.sel,
		.offset = 0,
		.size = (int)buck.value,
		.value1 = evaluate_number(value1_edit.editor.text.c_str(), buck.flags & FLAG_FLOAT).i,
		.value2 = evaluate_number(value2_edit.editor.text.c_str(), buck.flags & FLAG_FLOAT).i
	};

	search.byte_align = (int)evaluate_number(align_edit.editor.text.c_str()).i;

	search.start_addr = evaluate_number(start_addr_edit.editor.text.c_str()).i;
	search.end_addr = evaluate_number(end_addr_edit.editor.text.c_str()).i;

	search.source_type = source->type;
	search.pid = source->pid;
	search.identifier = source->identifier;

	return true;
}

void search_search_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	if (!sm->source || check_search_running())
		return;

	bool ok = false;
	if (sm->menu_type == MenuObject)
		ok = sm->prepare_object_params();
	else
		ok = sm->prepare_value_param();

	if (ok) {
		sm->cancel_btn.set_active(true);
		start_search(sm->search, sm->source->regions);
	}
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

	sm->struct_lbl.visible = sm->params_revealed;
	sm->struct_edit.visible = sm->params_revealed;
	sm->source_lbl.visible = sm->params_revealed;
	sm->source_edit.visible = sm->params_revealed;
	sm->addr_lbl.visible = sm->params_revealed;
	sm->start_addr_edit.visible = sm->params_revealed;
	sm->end_addr_edit.visible = sm->params_revealed;
	sm->align_lbl.visible = sm->params_revealed;
	sm->align_edit.visible = sm->params_revealed;
	sm->object_lbl.visible = sm->params_revealed;
	sm->object.visible = sm->params_revealed;
	sm->object_scroll.visible = sm->params_revealed;
	sm->object_div.visible = sm->params_revealed;
	sm->type_lbl.visible = sm->params_revealed;
	sm->type_edit.visible = sm->params_revealed;
	sm->method_lbl.visible = sm->params_revealed;
	sm->method_dd.visible = sm->params_revealed;
	sm->value_lbl.visible = sm->params_revealed;
	sm->value1_edit.visible = sm->params_revealed;
	sm->value2_edit.visible = sm->params_revealed;

	sm->update_reveal_button(view.scale);

	sm->min_height = sm->params_revealed ? sm->min_revealed_height : sm->min_hidden_height;
	if (sm->box.h < sm->min_height)
		sm->box.h = sm->min_height;
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

void Search_Menu::on_close() {
	if (params_pool)
		delete[] params_pool;
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
	table_font = ws.make_font(10, ws.colors.text, scale);

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

		struct_dd.edit_elem = &struct_edit;
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

	source_dd.edit_elem = &source_edit;
	source_dd.font = dd_font;
	source_dd.default_color = ws.colors.back;
	source_dd.hl_color = ws.colors.hl;
	source_dd.sel_color = ws.colors.active;
	ui.push_back(&source_dd);

	addr_lbl.font = label_font;
	addr_lbl.text = "Address Space";
	ui.push_back(&addr_lbl);

	start_addr_edit.font = table_font;
	start_addr_edit.editor.text = "0x" + std::string(16, '0');
	start_addr_edit.caret = ws.colors.caret;
	start_addr_edit.default_color = ws.colors.dark;
	ui.push_back(&start_addr_edit);

	end_addr_edit.font = table_font;
	end_addr_edit.editor.text = "0x" + std::string(16, 'f');
	end_addr_edit.caret = ws.colors.caret;
	end_addr_edit.default_color = ws.colors.dark;
	ui.push_back(&end_addr_edit);

	align_lbl.font = label_font;
	align_lbl.text = "Byte Alignment";
	ui.push_back(&align_lbl);

	align_edit.font = label_font;
	align_edit.ph_font = ws.make_font(label_font->size, icon_color, scale);
	align_edit.placeholder = "default";
	align_edit.caret = ws.colors.caret;
	align_edit.default_color = ws.colors.dark;
	ui.push_back(&align_edit);

	if (is_obj) {
		object_lbl.font = label_font;
		object_lbl.text = "Object";
		ui.push_back(&object_lbl);

		Column obj_cols[] = {
			{ColumnCheckbox, 0, 0.05, 1.0, 1.0, ""},
			{ColumnString,   0, 0.25, 0, 0, "Field"},
			{ColumnString,   0, 0.15, 0, 0, "Type"},
			{ColumnElement,  static_cast<int>(Elem_Drop_Down), 0.15, 0, 0, "Method"},
			{ColumnElement,  static_cast<int>(Elem_Edit_Box), 0.20, 0, 0, "Value"},
			{ColumnElement,  static_cast<int>(Elem_Edit_Box), 0.20, 0, 0, ""}
		};
		object_table.init(obj_cols, nullptr, this, search_object_table_cell_init, 6, 0);

		object.font = table_font;
		object.data = &object_table;
		object.action = search_object_handler;
		object.checkbox_toggle_handler = search_object_table_checkbox_handler;
		object.default_color = ws.colors.dark;
		object.back_color = ws.colors.dark;
		object.hl_color = ws.colors.hl;
		object.sel_color = ws.colors.hl;
		object.vscroll = &object_scroll;
		object.show_column_names = true;
		ui.push_back(&object);

		object_scroll.back_color = ws.colors.scroll_back;
		object_scroll.default_color = ws.colors.scroll;
		object_scroll.hl_color = ws.colors.scroll_hl;
		object_scroll.sel_color = ws.colors.scroll_sel;
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
		ui.push_back(&type_edit);

		type_dd.edit_elem = &type_edit;
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
		method_dd.external = &method_options;
		method_dd.leaning = 0.0;
		method_dd.keep_selected = true;
		method_dd.sel = 0;
		ui.push_back(&method_dd);

		value_lbl.font = label_font;
		value_lbl.text = "Value";
		ui.push_back(&value_lbl);

		value1_edit.font = label_font;
		value1_edit.caret = ws.colors.caret;
		value1_edit.default_color = ws.colors.dark;
		ui.push_back(&value1_edit);

		value2_edit.visible = false;
		value2_edit.font = label_font;
		value2_edit.caret = ws.colors.caret;
		value2_edit.default_color = ws.colors.dark;
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
	results_table.init(cols, nullptr, nullptr, nullptr, 2, 0);

	results.font = table_font;
	results.data = &results_table;
	results.action = search_results_handler;
	results.default_color = ws.colors.dark;
	results.back_color = ws.colors.dark;
	results.hl_color = ws.colors.hl;
	results.sel_color = ws.colors.hl;
	results.vscroll = &results_scroll;
	results.show_column_names = true;
	ui.push_back(&results);

	results_scroll.back_color = ws.colors.scroll_back;
	results_scroll.default_color = ws.colors.scroll;
	results_scroll.hl_color = ws.colors.scroll_hl;
	results_scroll.sel_color = ws.colors.scroll_sel;
	results_scroll.content = &results;
	ui.push_back(&results_scroll);

	initial_width = 400;
	initial_height = min_revealed_height;
	min_width = 300;
	min_height = min_revealed_height;
	refresh_every = 1;
}

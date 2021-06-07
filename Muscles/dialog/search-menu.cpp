#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

#define MAX_RESULTS 10000

#define LABEL_HEIGHT_FACTOR 1.2f
#define EDIT_HEIGHT_FACTOR  1.4f

void Search_Menu::update_ui(Camera& view) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

	float title_h = title.font->render.text_height() / view.scale;
	title.outer_box = {
		.x = border,
		.y = 0,
		.w = box.w - 2*border,
		.h = title_h + 2*border
	};

	float label_h = source_lbl.font->render.text_height() * LABEL_HEIGHT_FACTOR / view.scale;
	float edit_h  = source_edit.font->render.text_height() * EDIT_HEIGHT_FACTOR / view.scale;

	float start_x = 2*border;
	float y = title.outer_box.h;

	reveal_btn.pos = {
		.x = box.w - reveal_btn_length - start_x,
		.y = y + 3*border,
		.w = reveal_btn_length,
		.h = reveal_btn_length
	};

	if (params_revealed) {
		source_lbl.pos = {
			.x = start_x,
			.y = y,
			.w = 80,
			.h = label_h
		};
		y += source_lbl.pos.h + border;

		source_edit.pos = {
			.x = start_x,
			.y = y,
			.w = reveal_btn.pos.x - 2*start_x,
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

	y += 5*border;

	// progress_bar.pos = {...

	float total_btn_w = search_btn.width + start_x + cancel_btn.width;

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

	sm->search.source = src;
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

void search_search_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);

	if (sm->method_dd.sel < 0)
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

	sm->search.start();
}

void search_cancel_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	
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

void Search_Menu::update_reveal_button(float scale) {
	int size = 0.5 + reveal_btn_length * scale;

	// we call sdl_destroy_texture() here since this function is also called when the icon needs to flip between up and down
	sdl_destroy_texture(&reveal_btn.icon);
	reveal_btn.icon = make_triangle(reveal_btn.default_color, size, size, params_revealed);
}

void Search_Menu::refresh(Point *cursor) {
	Workspace& ws = *parent;

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

	results_count_lbl.text = std::to_string(search.results.size());

	if (search.started && !search.running) {
		search.started = false;
		search.finalize();

		cancel_btn.set_active(false);

		int n_results = search.results.size();
		if (n_results > MAX_RESULTS)
			n_results = MAX_RESULTS;

		table.resize(n_results);
		memcpy(table.columns[0].data(), search.results.data(), n_results * sizeof(u64));

		results.needs_redraw = true;
	}
}

void Search_Menu::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	float edit_h = source_edit.font->render.text_height() * EDIT_HEIGHT_FACTOR / new_scale;
	source_edit.update_icon(IconTriangle, edit_h, new_scale);
	type_edit.update_icon(IconTriangle, edit_h, new_scale);

	sdl_destroy_texture(&method_dd.icon);
	method_dd.icon_length = method_dd.font->render.text_height() * EDIT_HEIGHT_FACTOR;
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

	source_lbl.font = ws.make_font(11, ws.colors.text, scale);
	source_lbl.text = "Source";
	ui.push_back(&source_lbl);

	RGBA icon_color = ws.colors.text;
	icon_color.a = 0.7;

	source_edit.font = source_lbl.font;
	source_edit.caret = ws.colors.caret;
	source_edit.default_color = ws.colors.dark;
	source_edit.icon_color = icon_color;
	source_edit.icon_right = true;
	source_edit.dropdown = &source_dd;
	source_edit.key_action = search_source_edit_handler;
	ui.push_back(&source_edit);

	source_dd.visible = false;
	source_dd.font = source_lbl.font;
	source_dd.default_color = ws.colors.back;
	source_dd.hl_color = ws.colors.hl;
	source_dd.sel_color = ws.colors.active;
	ui.push_back(&source_dd);

	addr_lbl.font = source_lbl.font;
	addr_lbl.text = "Address Space";
	ui.push_back(&addr_lbl);

	start_addr_edit.font = source_lbl.font;
	start_addr_edit.editor.text = "0x" + std::string(16, '0');
	start_addr_edit.caret = ws.colors.caret;
	start_addr_edit.default_color = ws.colors.dark;
	start_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&start_addr_edit);

	end_addr_edit.font = source_lbl.font;
	end_addr_edit.editor.text = "0x" + std::string(16, 'f');
	end_addr_edit.caret = ws.colors.caret;
	end_addr_edit.default_color = ws.colors.dark;
	end_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&end_addr_edit);

	type_lbl.font = source_lbl.font;
	type_lbl.text = "Type";
	ui.push_back(&type_lbl);

	type_edit.font = source_lbl.font;
	type_edit.editor.text = "int32_t";
	type_edit.caret = ws.colors.caret;
	type_edit.default_color = ws.colors.dark;
	type_edit.icon_color = icon_color;
	type_edit.icon_right = true;
	type_edit.dropdown = &type_dd;
	type_edit.key_action = search_type_edit_handler;
	ui.push_back(&type_edit);

	type_dd.visible = false;
	type_dd.font = source_lbl.font;
	type_dd.default_color = ws.colors.back;
	type_dd.hl_color = ws.colors.hl;
	type_dd.sel_color = ws.colors.active;
	ui.push_back(&type_dd);

	method_lbl.font = source_lbl.font;
	method_lbl.text = "Method";
	ui.push_back(&method_lbl);

	method_dd.font = source_lbl.font;
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

	value_lbl.font = source_lbl.font;
	value_lbl.text = "Value";
	ui.push_back(&value_lbl);

	value1_edit.font = source_lbl.font;
	value1_edit.caret = ws.colors.caret;
	value1_edit.default_color = ws.colors.dark;
	value1_edit.key_action = search_value_edit_handler;
	ui.push_back(&value1_edit);

	value2_edit.visible = false;
	value2_edit.font = source_lbl.font;
	value2_edit.caret = ws.colors.caret;
	value2_edit.default_color = ws.colors.dark;
	value2_edit.key_action = search_value_edit_handler;
	ui.push_back(&value2_edit);

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
		{ColumnHex, 0, 0.5, 0, 0, "Address"},
		{ColumnString, 0, 0.5, 0, 0, "Value"},
	};
	table.init(cols, nullptr, 2, 0);

	results.font = source_lbl.font;
	results.data = &table;
	results.default_color = ws.colors.dark;
	results.hl_color = ws.colors.hl;
	results.sel_color = ws.colors.hl;
	results.font = ws.default_font;
	results.vscroll = &vscroll;
	ui.push_back(&results);

	vscroll.content = &results;
	ui.push_back(&vscroll);

	initial_width = 400;
	initial_height = 450;
	min_width = 300;
	min_height = 450;
	refresh_every = 1;
}

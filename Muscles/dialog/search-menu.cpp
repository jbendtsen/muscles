#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void Search_Menu::update_ui(Camera& view) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

	float title_h = title.font->render.text_height() / view.scale;
	title.outer_box = {
		.x = border,
		.y = 0,
		.w = box.w - 2*border,
		.h = title_h + 2*border
	};

	
}

void search_source_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_address_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_type_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_method_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_value_edit_handler(Edit_Box *edit, Input& input) {
	
}

void search_search_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	
}

void search_reveal_btn_handler(UI_Element *elem, Camera& view, bool dbl_click) {
	auto sm = dynamic_cast<Search_Menu*>(elem->parent);
	sm->params_revealed = !sm->params_revealed;
	sm->update_reveal_button(view.scale);
}

void Search_Menu::update_reveal_button(float scale) {
	int size = 0.5 + reveal_btn_length * scale;

	// we call sdl_destroy_texture() here since this function is also called when the icon needs to flip between up and down
	sdl_destroy_texture(&reveal_btn.icon);
	reveal_btn.icon = make_triangle(reveal_btn.default_color, size, size, params_revealed);
}

void Search_Menu::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	float search_h = search_btn.active_theme.font->render.text_height() * 1.4 / new_scale;
	search_btn.icon = make_glass_icon(search_btn.default_color, search_h);

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
	start_addr_edit.caret = ws.colors.caret;
	start_addr_edit.default_color = ws.colors.dark;
	start_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&start_addr_edit);

	end_addr_edit.font = source_lbl.font;
	end_addr_edit.caret = ws.colors.caret;
	end_addr_edit.default_color = ws.colors.dark;
	end_addr_edit.key_action = search_address_edit_handler;
	ui.push_back(&end_addr_edit);

	type_lbl.font = source_lbl.font;
	type_lbl.text = "Type";
	ui.push_back(&type_lbl);

	type_edit.font = source_lbl.font;
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

	method_edit.font = source_lbl.font;
	method_edit.caret = ws.colors.caret;
	method_edit.default_color = ws.colors.dark;
	method_edit.icon_color = icon_color;
	method_edit.icon_right = true;
	method_edit.dropdown = &method_dd;
	method_edit.key_action = search_method_edit_handler;
	ui.push_back(&method_edit);

	method_dd.visible = false;
	method_dd.font = source_lbl.font;
	method_dd.default_color = ws.colors.back;
	method_dd.hl_color = ws.colors.hl;
	method_dd.sel_color = ws.colors.active;
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

	search_btn.text = "Search";
	search_btn.action = search_search_btn_handler;
	search_btn.default_color = ws.colors.text;
	search_btn.default_color.a = 0.7;
	search_btn.active_theme = {
		ws.colors.light,
		ws.colors.light,
		ws.colors.light,
		ws.default_font
	};
	search_btn.inactive_theme = search_btn.active_theme;
	search_btn.update_size(scale);
	ui.push_back(&search_btn);

	cancel_btn.text = "Cancel";
	cancel_btn.action = search_search_btn_handler;
	cancel_btn.default_color = ws.colors.cancel;
	cancel_btn.default_color.a = 0.7;
	cancel_btn.active_theme = {
		ws.colors.light,
		ws.colors.light,
		ws.colors.light,
		ws.default_font
	};
	cancel_btn.inactive_theme = search_btn.active_theme;
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

	//Progress_Bar progress_bar;

	results_lbl.font = ws.default_font;
	results_lbl.text = "Results";
	ui.push_back(&results_lbl);

	result_count_lbl.font = source_lbl.font;
	result_count_lbl.text = "";
	ui.push_back(&result_count_lbl);

	//Data_View results;
	//Scroll vscroll;
}

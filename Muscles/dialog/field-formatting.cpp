#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

static float get_edit_height(Edit_Box& edit, float scale) {
	return edit.font->render.text_height() * 1.4f / scale;
}

void Field_Formatting::update_ui(Camera& view) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);

	float w = title.font->render.text_width(title.text.c_str()) / view.scale;
	float max_title_w = box.w - 5 * cross_size;
	w = w < max_title_w ? w : max_title_w;

	title.pos = {
		(box.w - w) / 2,
		2,
		w,
		title.font->render.text_height() * 1.1f / view.scale
	};

	float y = 2*cross.pos.y + cross.pos.h;

	field_edit.pos = {
		border,
		y,
		box.w - 2*border,
		get_edit_height(field_edit, view.scale)
	};
	y += field_edit.pos.h + border;

	float scroll_w = 14;

	options.pos = {
		field_edit.pos.x,
		y,
		field_edit.pos.w - scroll_w - border,
		box.h - border - y
	};

	scroll.pos = {
		box.w - border - scroll_w,
		y,
		scroll_w,
		options.pos.h
	};
}

void field_edit_handler(Edit_Box *edit, Input& input) {

}

void Field_Formatting::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;

	float height = get_edit_height(field_edit, new_scale);
	field_edit.update_icon(IconTriangle, height, new_scale);
}

Field_Formatting::Field_Formatting(Workspace& ws) {
	float scale = get_default_camera().scale;

	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;
	ui.push_back(&maxm);

	title.font = ws.default_font;
	title.text = "Field Formatting";
	title.padding = 0;
	ui.push_back(&title);

	field_dd.visible = false;
	field_dd.font = ws.make_font(11, ws.text_color, scale);
	field_dd.default_color = ws.back_color;
	field_dd.hl_color = ws.hl_color;
	field_dd.sel_color = ws.active_color;
	field_dd.width = 250;
	ui.push_back(&field_dd);

	field_edit.font = field_dd.font;
	field_edit.caret = ws.caret_color;
	field_edit.default_color = ws.dark_color;
	field_edit.icon_color = ws.text_color;
	field_edit.icon_color.a = 0.7;
	field_edit.icon_right = true;
	field_edit.dropdown = &field_dd;
	field_edit.key_action = field_edit_handler;
	ui.push_back(&field_edit);

	scroll.back_color = ws.scroll_back;
	scroll.default_color = ws.scroll_color;
	scroll.hl_color = ws.scroll_hl_color;
	scroll.sel_color = ws.scroll_sel_color;
	ui.push_back(&scroll);

	Column cols[] = {
		{ColumnString, 0, 0.5, 0, 0, ""},
		{ColumnElement, 0, 0.5, 0, 0, ""},
	};
	table.init(cols, nullptr, 2, 10);

	table.columns[0] = {
		(void*)"String",
		(void*)"Array Brackets",
		(void*)"Array Separator",
		(void*)"Base",
		(void*)"Prefix",
		(void*)"Precision",
		(void*)"Floating-Point",
		(void*)"Uppercase",
		(void*)"Signedness",
		(void*)"Big Endian"
	};
	table.columns[1] = {
		(void*)&string_dd,
		(void*)&brackets_dd,
		(void*)&separator_edit,
		(void*)&prefix_edit,
		(void*)&base_edit,
		(void*)&precision_edit,
		(void*)&floatfmt_dd,
		(void*)&uppercase_cb,
		(void*)&sign_dd,
		(void*)&endian_cb
	};

	string_dd.title = "auto";
	string_dd.default_color = ws.dark_color;

	brackets_dd.title = "[]";
	brackets_dd.default_color = ws.dark_color;

	separator_edit.default_color = ws.dark_color;

	prefix_edit.default_color = ws.dark_color;

	base_edit.default_color = ws.dark_color;

	precision_edit.default_color = ws.dark_color;
	precision_edit.dropdown = &precision_dd;

	precision_dd.title = "auto";

	floatfmt_dd.title = "auto";
	floatfmt_dd.default_color = ws.dark_color;

	uppercase_cb.default_color = ws.scroll_back;
	uppercase_cb.sel_color = ws.cb_color;
	uppercase_cb.leaning = 0.9;

	sign_dd.title = "auto";
	sign_dd.default_color = ws.dark_color;

	endian_cb.default_color = ws.scroll_back;
	endian_cb.sel_color = ws.cb_color;
	endian_cb.leaning = 0.9;

	options.data = &table;
	options.font = ws.make_font(10, ws.text_color, scale);
	options.default_color = ws.dark_color;
	options.hl_color = ws.dark_color;
	options.sel_color = ws.dark_color;
	options.vscroll = &scroll;
	options.vscroll->content = &options;
	ui.push_back(&options);

	back = ws.back_color;
	edge_color = ws.dark_color;

	initial_width = 350;
	initial_height = 280;
}
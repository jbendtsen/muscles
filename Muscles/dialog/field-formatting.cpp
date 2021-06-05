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

	precision_dd.font = options.font;
	base_edit.font = options.font;

	base_edit.make_icon(new_scale);

	float height = get_edit_height(field_edit, new_scale);
	field_edit.update_icon(IconTriangle, height, new_scale);

	float row_h = options.font->render.text_height();
	sdl_destroy_texture(&option_arrow);
	option_arrow = make_triangle(arrow_color, row_h, row_h);

	auto set_icon =
	[this, row_h]<typename T>(T& edit) {
		edit.icon = option_arrow;
		edit.icon_length = row_h;
		edit.needs_redraw = true;
	};

	set_icon(string_dd);
	set_icon(brackets_dd);
	set_icon(separator_edit);
	set_icon(prefix_edit);
	set_icon(precision_edit);
	set_icon(floatfmt_dd);
	set_icon(sign_dd);
}

void Field_Formatting::on_close() {
	sdl_destroy_texture(&option_arrow);
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
	field_dd.font = ws.make_font(11, ws.colors.text, scale);
	field_dd.default_color = ws.colors.back;
	field_dd.hl_color = ws.colors.hl;
	field_dd.sel_color = ws.colors.active;
	field_dd.width = 250;
	ui.push_back(&field_dd);

	arrow_color = ws.colors.text;
	arrow_color.a = 0.7;

	field_edit.font = field_dd.font;
	field_edit.caret = ws.colors.caret;
	field_edit.default_color = ws.colors.dark;
	field_edit.icon_color = arrow_color;
	field_edit.icon_right = true;
	field_edit.dropdown = &field_dd;
	field_edit.key_action = field_edit_handler;
	ui.push_back(&field_edit);

	scroll.back_color = ws.colors.scroll_back;
	scroll.default_color = ws.colors.scroll;
	scroll.hl_color = ws.colors.scroll_hl;
	scroll.sel_color = ws.colors.scroll_sel;
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
		(void*)"Prefix",
		(void*)"Base",
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
	string_dd.default_color = ws.colors.scroll_back;
	string_dd.hl_color = ws.colors.hl;
	string_dd.leaning = 1.0;
	string_dd.title_off_y = -0.1;
	string_dd.parent = this;

	brackets_dd.title = "[]";
	brackets_dd.default_color = ws.colors.scroll_back;
	brackets_dd.hl_color = ws.colors.hl;
	brackets_dd.leaning = 1.0;
	brackets_dd.title_off_y = -0.1;
	brackets_dd.parent = this;

	separator_edit.default_color = ws.colors.scroll_back;
	separator_edit.caret = ws.colors.text;
	separator_edit.icon_color = arrow_color;
	separator_edit.icon_right = true;
	separator_edit.manage_icon = false;
	separator_edit.text_off_y = -0.1;
	separator_edit.parent = this;

	prefix_edit.default_color = ws.colors.scroll_back;
	prefix_edit.caret = ws.colors.text;
	prefix_edit.icon_color = arrow_color;
	prefix_edit.icon_right = true;
	prefix_edit.manage_icon = false;
	prefix_edit.text_off_y = -0.1;
	prefix_edit.parent = this;

	base_edit.default_color = ws.colors.scroll_back;
	base_edit.sel_color = ws.colors.scroll_back;
	base_edit.text_off_y = -0.25;
	base_edit.parent = this;

	precision_edit.default_color = ws.colors.scroll_back;
	precision_edit.caret = ws.colors.text;
	precision_edit.text_off_y = -0.1;
	precision_edit.icon_color = arrow_color;
	precision_edit.icon_right = true;
	precision_edit.manage_icon = false;
	precision_edit.dropdown = &precision_dd;
	precision_edit.parent = this;

	precision_dd.title = "auto";
	precision_dd.parent = this;

	floatfmt_dd.title = "auto";
	floatfmt_dd.default_color = ws.colors.scroll_back;
	floatfmt_dd.hl_color = ws.colors.hl;
	floatfmt_dd.title_off_y = -0.1;
	floatfmt_dd.leaning = 1.0;
	floatfmt_dd.parent = this;

	uppercase_cb.default_color = ws.colors.scroll_back;
	uppercase_cb.sel_color = ws.colors.cb;
	uppercase_cb.leaning = 1.0;
	uppercase_cb.parent = this;

	sign_dd.title = "auto";
	sign_dd.default_color = ws.colors.scroll_back;
	sign_dd.hl_color = ws.colors.hl;
	sign_dd.title_off_y = -0.1;
	sign_dd.leaning = 1.0;
	sign_dd.parent = this;

	endian_cb.default_color = ws.colors.scroll_back;
	endian_cb.sel_color = ws.colors.cb;
	endian_cb.leaning = 1.0;
	endian_cb.parent = this;

	options.data = &table;
	options.font = ws.make_font(10, ws.colors.text, scale);
	options.extra_line_spacing = 0.1;
	options.default_color = ws.colors.dark;
	options.hl_color = ws.colors.dark;
	options.sel_color = ws.colors.dark;
	options.vscroll = &scroll;
	options.vscroll->content = &options;
	ui.push_back(&options);

	initial_width = 350;
	initial_height = 280;
}

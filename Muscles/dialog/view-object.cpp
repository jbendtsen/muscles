#include "muscles.h"
#include "ui.h"
#include "dialog.h"

float get_edit_height(View_Object *ui, float scale) {
	return ui->struct_edit->font->render.text_height() * 1.4f / scale;
}

void update_view_object(Box& b, Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	b.update_elements(view, input, inside, hover, focussed);

	auto ui = (View_Object*)b.markup;

	ui->cross->pos.y = b.cross_size * 0.5;
	ui->cross->pos.x = b.box.w - b.cross_size * 1.5;
	ui->cross->pos.w = b.cross_size;
	ui->cross->pos.h = b.cross_size;

	float title_w = 200;
	float title_x = (ui->cross->pos.x - title_w) / 2;
	if (title_x < b.border) {
		title_x = b.border;
		title_w = ui->cross->pos.x - title_x - 2*b.border;
	}

	ui->title_edit->pos = {
		title_x,
		2,
		title_w,
		ui->title_edit->font->render.text_height() * 1.4f / view.scale
	};

	float meta_y = ui->title_edit->pos.h + 4*b.border;
	ui->hide_meta->pos = {
		ui->cross->pos.x + (ui->cross->pos.w - ui->meta_btn_width),
		meta_y,
		ui->meta_btn_width,
		ui->meta_btn_height
	};

	float label_x = 2*b.border;
	float y = meta_y;

	if (!ui->meta_hidden) {
		float label_h = ui->struct_label->font->render.text_height() * 1.2f / view.scale;

		float edit_h = get_edit_height(ui, view.scale);
		float edit_gap = edit_h + b.border;
		float edit_y = meta_y - (edit_h - label_h) / 2;

		ui->struct_label->pos = {
			label_x,
			y,
			80,
			label_h
		};
		y += edit_gap;

		ui->source_label->pos = {
			label_x,
			y,
			80,
			label_h
		};
		y += edit_gap;

		ui->addr_label->pos = {
			label_x,
			y,
			100,
			label_h
		};

		ui->struct_label->update_position(view);
		ui->source_label->update_position(view);
		ui->addr_label->update_position(view);

		y = edit_y;
		ui->struct_edit->pos = {
			label_x + ui->struct_label->pos.w + b.border,
			y,
			150,
			edit_h
		};
		y += edit_gap;

		ui->source_edit->pos = {
			label_x + ui->source_label->pos.w + b.border,
			y,
			150,
			edit_h
		};
		y += edit_gap;

		ui->addr_edit->pos = {
			label_x + ui->addr_label->pos.w + b.border,
			y,
			130,
			edit_h
		};

		y += edit_gap + b.border;
	}
	else
		y += ui->meta_btn_height + 2*b.border;

	float scroll_w = 14;
	float view_w = b.box.w - 2*label_x - scroll_w - b.border;
	float view_h = b.box.h - y - scroll_w - ui->all_btn->height - 4*b.border;

	ui->vscroll->pos = {
		label_x + view_w + b.border,
		y,
		scroll_w,
		view_h
	};

	ui->hscroll->pos = {
		label_x,
		y + view_h + b.border,
		view_w,
		scroll_w
	};

	ui->view->pos = {
		label_x,
		y,
		view_w,
		view_h
	};

	ui->all_btn->pos = {
		label_x,
		b.box.h - 2*b.border - ui->all_btn->height,
		ui->all_btn->width,
		ui->all_btn->height
	};

	ui->sel_btn->pos = {
		ui->all_btn->pos.x + ui->all_btn->pos.w,
		ui->all_btn->pos.y,
		ui->sel_btn->width,
		ui->sel_btn->height
	};

	b.post_update_elements(view, input, inside, hover, focussed);
}

void update_hide_meta_button(View_Object *ui, float scale) {
	float pad = ui->hide_meta->padding;
	int meta_w = 0.5 + (ui->meta_btn_width - 2*pad) * scale;
	int meta_h = 0.5 + (ui->meta_btn_height - 2*pad) * scale;

	sdl_destroy_texture(&ui->hide_meta->icon);
	ui->hide_meta->icon = make_triangle(ui->hide_meta->default_color, meta_w, meta_h, !ui->meta_hidden);
}

void select_view_type(View_Object *ui, bool all) {
	ui->all = all;

	auto theme_all = ui->all ? &ui->theme_on : &ui->theme_off;
	auto theme_sel = ui->all ? &ui->theme_off : &ui->theme_on;

	ui->all_btn->active_theme = *theme_all;
	ui->all_btn->inactive_theme = *theme_all;
	ui->sel_btn->active_theme = *theme_sel;
	ui->sel_btn->inactive_theme = *theme_sel;
}

static void scale_change_handler(Workspace& ws, Box& b, float new_scale) {
	auto ui = (View_Object*)b.markup;
	ui->cross->img = ws.cross;

	update_hide_meta_button(ui, new_scale);

	float height = get_edit_height(ui, new_scale);
	ui->struct_edit->update_icon(IconTriangle, height, new_scale);
	ui->source_edit->update_icon(IconTriangle, height, new_scale);
}

void make_view_object(Workspace& ws, Box& b) {
	auto ui = new View_Object();

	ui->cross = new Image();
	ui->cross->action = get_delete_box();
	ui->cross->img = ws.cross;
	b.ui.push_back(ui->cross);

	ui->title_edit = new Edit_Box();
	ui->title_edit->placeholder = "<instance>";
	ui->title_edit->ph_font = ws.make_font(12, ws.ph_text_color);
	ui->title_edit->font = ws.default_font;
	ui->title_edit->caret = ws.caret_color;
	ui->title_edit->caret.a = 0.6;
	ui->title_edit->default_color = ws.dark_color;
	b.ui.push_back(ui->title_edit);

	ui->hide_meta = new Button();
	ui->hide_meta->padding = 0;
	ui->hide_meta->action = [](UI_Element *elem, bool dbl_click) {
		auto ui = (View_Object*)elem->parent->markup;
		ui->meta_hidden = !ui->meta_hidden;
		ui->struct_label->visible = ui->struct_edit->visible = !ui->meta_hidden;
		ui->source_label->visible = ui->source_edit->visible = !ui->meta_hidden;
		ui->addr_label->visible = ui->addr_edit->visible = !ui->meta_hidden;
		update_hide_meta_button(ui, elem->parent->parent->temp_scale);
	};
	ui->hide_meta->active_theme = {
		ws.light_color,
		ws.light_hl_color,
		ws.light_hl_color,
		nullptr
	};
	ui->hide_meta->inactive_theme = ui->hide_meta->active_theme;
	ui->hide_meta->default_color = ws.text_color;
	ui->hide_meta->default_color.a = 0.8;
	b.ui.push_back(ui->hide_meta);

	ui->struct_label = new Label();
	ui->struct_label->font = ws.make_font(11, ws.text_color);
	ui->struct_label->text = "Struct:";
	ui->struct_label->padding = 0;
	b.ui.push_back(ui->struct_label);

	ui->source_label = new Label();
	ui->source_label->font = ui->struct_label->font;
	ui->source_label->text = "Source:";
	ui->source_label->padding = 0;
	b.ui.push_back(ui->source_label);

	ui->addr_label = new Label();
	ui->addr_label->font = ui->struct_label->font;
	ui->addr_label->text = "Address:";
	ui->addr_label->padding = 0;
	b.ui.push_back(ui->addr_label);

	auto mm = (Main_Menu*)ws.first_box_of_type(TypeMain)->markup;
	RGBA icon_color = ws.text_color;
	icon_color.a = 0.7;

	ui->addr_edit = new Edit_Box();
	ui->addr_edit->font = ui->addr_label->font;
	ui->addr_edit->caret = ws.caret_color;
	ui->addr_edit->default_color = ws.dark_color;
	b.ui.push_back(ui->addr_edit);

	ui->source_dd = new Drop_Down();
	ui->source_dd->visible = false;
	ui->source_dd->font = ui->source_label->font;
	ui->source_dd->default_color = ws.back_color;
	ui->source_dd->hl_color = ws.hl_color;
	ui->source_dd->sel_color = ws.active_color;
	ui->source_dd->width = 250;
	ui->source_dd->external = (std::vector<char*>*)&mm->table->data.columns[2];
	b.ui.push_back(ui->source_dd);

	ui->source_edit = new Edit_Box();
	ui->source_edit->font = ui->source_label->font;
	ui->source_edit->caret = ws.caret_color;
	ui->source_edit->default_color = ws.dark_color;
	ui->source_edit->icon_color = icon_color;
	ui->source_edit->icon_right = true;
	ui->source_edit->dropdown = ui->source_dd;
	b.ui.push_back(ui->source_edit);

	ui->struct_dd = new Drop_Down();
	ui->struct_dd->visible = false;
	ui->struct_dd->font = ui->struct_label->font;
	ui->struct_dd->default_color = ws.back_color;
	ui->struct_dd->hl_color = ws.hl_color;
	ui->struct_dd->sel_color = ws.active_color;
	ui->struct_dd->width = 250;
	ui->struct_dd->content = {
		(char*)"Some Stuff",
		(char*)"Wait these aren't structs"
	};
	b.ui.push_back(ui->struct_dd);

	ui->struct_edit = new Edit_Box();
	ui->struct_edit->font = ui->struct_label->font;
	ui->struct_edit->caret = ws.caret_color;
	ui->struct_edit->default_color = ws.dark_color;
	ui->struct_edit->icon_color = icon_color;
	ui->struct_edit->icon_right = true;
	ui->struct_edit->dropdown = ui->struct_dd;
	b.ui.push_back(ui->struct_edit);

	ui->hscroll = new Scroll();
	ui->hscroll->back = ws.scroll_back;
	ui->hscroll->default_color = ws.scroll_color;
	ui->hscroll->hl_color = ws.scroll_hl_color;
	ui->hscroll->sel_color = ws.scroll_sel_color;
	ui->hscroll->vertical = false;
	b.ui.push_back(ui->hscroll);

	ui->vscroll = new Scroll();
	ui->vscroll->back = ws.scroll_back;
	ui->vscroll->default_color = ws.scroll_color;
	ui->vscroll->hl_color = ws.scroll_hl_color;
	ui->vscroll->sel_color = ws.scroll_sel_color;
	b.ui.push_back(ui->vscroll);

	ui->view = new Data_View();
	ui->view->font = ws.default_font;
	ui->view->default_color = ws.dark_color;
	ui->view->hl_color = ws.hl_color;
	ui->view->consume_box_scroll = true;
	b.ui.push_back(ui->view);

	ui->theme_on = {
		ws.dark_color,
		ws.light_color,
		ws.light_color,
		ws.make_font(10, ws.text_color)
	};
	ui->theme_off = {
		ws.back_color,
		ws.light_color,
		ws.light_color,
		ui->theme_on.font
	};

	ui->all_btn = new Button();
	ui->all_btn->padding = 0;
	ui->all_btn->text = "All";
	ui->all_btn->active_theme = ui->theme_on;
	ui->all_btn->inactive_theme = ui->theme_on;
	ui->all_btn->update_size(ws.temp_scale);
	ui->all_btn->action = [](UI_Element *elem, bool dbl_click) { select_view_type((View_Object*)elem->parent->markup, true); };
	b.ui.push_back(ui->all_btn);

	ui->sel_btn = new Button();
	ui->sel_btn->padding = 0;
	ui->sel_btn->text = "Selected";
	ui->sel_btn->active_theme = ui->theme_off;
	ui->sel_btn->inactive_theme = ui->theme_off;
	ui->sel_btn->update_size(ws.temp_scale);
	ui->sel_btn->action = [](UI_Element *elem, bool dbl_click) { select_view_type((View_Object*)elem->parent->markup, false); };
	b.ui.push_back(ui->sel_btn);

	b.markup = ui;
	b.update_handler = update_view_object;
	b.scale_change_handler = scale_change_handler;
	//b.refresh_handler = refresh_handler;
	b.back = ws.back_color;
	b.edge_color = ws.dark_color;
	b.box = {-200, -225, 400, 450};
	b.min_width = 300;
	b.min_height = 200;
	b.visible = true;
}
#include "muscles.h"
#include "ui.h"
#include "dialog/dialog.h"

void Workspace::init(Font_Face face) {
	this->face = face;

	sdl_get_dpi(dpi_w, dpi_h);

	adjust_scale(1.0, 1.0);

	default_font = new Font(face, 12, colors.text, dpi_w, dpi_h);
	fonts.push_back(default_font);

	inactive_font = new Font(face, 12, colors.inactive_text, dpi_w, dpi_h);
	fonts.push_back(inactive_font);

	// Fonts that are added to the 'fonts' vector are resized upon zooming in/out.
	// This is why we create separate fonts for 'rclick_menu', even though they use the same parameters.
	rclick_menu.font = new Font(face, 12, colors.text, dpi_w, dpi_h);
	rclick_menu.inactive_font = new Font(face, 12, colors.inactive_text, dpi_w, dpi_h);

	Menu_Item test_item = {0, (char*)"Item A", nullptr};
	default_rclick_menu.push_back(test_item);

	test_item.name = (char*)"Item B";
	default_rclick_menu.push_back(test_item);

	std::string text = "struct Test {\n\tint a;\n\tint b\n;};";
	update_structs(text);

	make_box<Opening_Menu>();
}

void Workspace::close() {
	sdl_destroy_texture(&cross);

	for (auto& b : boxes) {
		for (auto& e : b->ui)
			e->release();

		delete b;
	}

	for (auto& f : fonts)
		delete f;

	delete rclick_menu.font;
	delete rclick_menu.inactive_font;

	for (auto& s : sources) {
		close_source(*s);
		delete s;
	}
}

void Workspace::adjust_scale(float old_scale, float new_scale) {
	for (auto& f : fonts)
		f->adjust_scale(new_scale, dpi_w, dpi_h);

	sdl_destroy_texture(&cross);
	cross = make_cross_icon(colors.text, cross_size * new_scale, 1.0/24.0, 1.0/16.0);

	sdl_destroy_texture(&maxm);
	float length = cross_size * new_scale;
	maxm = make_rectangle(colors.text, length, length, 0, 0, 0.08);

	for (auto& b : boxes)
		b->handle_zoom(*this, new_scale);
}

void Workspace::add_box(Box *b) {
	boxes.push_back(b);
	new_box = b;
}

void Workspace::delete_box(int idx) {
	if (idx < 0)
		return;

	boxes[idx]->on_close();
	boxes[idx]->visible = false;

	if (!boxes[idx]->expungeable)
		return;

	if (boxes[idx] == selected)
		selected = nullptr;
	if (boxes[idx] == new_box)
		new_box = nullptr;

	delete boxes[idx];
	boxes.erase(boxes.begin()+idx);
	box_expunged = true;
}

void Workspace::delete_box(Box *b) {
	for (int i = 0; i < boxes.size(); i++) {
		if (b == boxes[i]) {
			delete_box(i);
			return;
		}
	}
}

Box *Workspace::box_under_cursor(Camera& view, Point& cur, Point& inside) {
	int n_boxes = boxes.size();
	for (int i = n_boxes-1; i >= 0; i--) {
		if (!boxes[i]->visible)
			continue;

		Rect& r = boxes[i]->box;
		float e = i == n_boxes-1 ? boxes[i]->border / view.scale : 0;

		if (cur.x >= r.x - e && cur.x < r.x+r.w + e && cur.y >= r.y - e && cur.y < r.y+r.h + e) {
			inside.x = cur.x - r.x;
			inside.y = cur.y - r.y;
			return boxes[i];
		}
	}

	return nullptr;
}

void Workspace::bring_to_front(Box *b) {
	for (int i = 0; i < boxes.size(); i++) {
		if (boxes[i] == b) {
			boxes.erase(boxes.begin() + i);
			boxes.push_back(b);
			return;
		}
	}
}

Font *Workspace::make_font(float size, RGBA& color, float scale) {
	auto to_points = [](float f) -> int { return f * 100.0; };

	int s = to_points(size);
	int r = to_points(color.r);
	int g = to_points(color.g);
	int b = to_points(color.b);
	int a = to_points(color.a);

	for (auto& f : fonts) {
		if (
			to_points(f->size) == s &&
			to_points(f->color.r) == r &&
			to_points(f->color.g) == g &&
			to_points(f->color.b) == b &&
			to_points(f->color.a) == a
		)
			return f;
	}

	Font *f = new Font(face, size, color, dpi_w, dpi_h);
	fonts.push_back(f);
	f->adjust_scale(scale, dpi_w, dpi_h);
	return f;
}

void Workspace::refresh_sources() {
	auto& sources = (std::vector<Source*>&)this->sources;
	for (auto& s : sources) {
		s->region_refreshed = false;
		if (!s->block_region_refresh && s->timer % s->refresh_region_rate == 0) {
			if (s->type == SourceFile)
				refresh_file_region(*s);
			else if (s->type == SourceProcess)
				refresh_process_regions(*s);

			s->region_refreshed = true;
		}
		if (s->timer % s->refresh_span_rate == 0) {
			s->gather_data();
		}
		s->timer++;
	}
}

void Workspace::view_source_at(Source *source, u64 address) {
	if (!source)
		return;

	View_Source *vs = nullptr;
	for (auto& box : boxes) {
		if (box->box_type == View_Source::box_type_meta) {
			auto cur = dynamic_cast<View_Source*>(box);
			if (cur->get_source() == source) {
				vs = cur;
				break;
			}
		}
	}

	if (!vs) {
		MenuType type = source->type == SourceFile ? MenuFile : MenuProcess;
		vs = make_box<View_Source>(type);
	}
	else {
		vs->visible = true;
		bring_to_front(vs);
	}

	vs->open_source(source);
	if (address != -1)
		vs->goto_address(address);
}

void Workspace::prepare_rclick_menu(Camera& view, Point& cursor) {
	return;
}

void reposition_rclick_menu(Context_Menu& menu, Camera& view, int x, int y) {
	auto& items = *menu.list;
	int n_visible = 0;
	int width = 0;

	for (auto& it : items) {
		if (it.flags & FLAG_INVISIBLE)
			continue;

		n_visible++;
		int w = menu.font->render.text_width(it.name);
		if (w > width)
			width = w;
	}

	menu.rect = {
		x,
		y,
		width + 50,
		n_visible * menu.get_item_height()
	};

	int sc_w = view.center_x * 2;
	int sc_h = view.center_y * 2;
	if (menu.rect.x + menu.rect.w >= sc_w)
		menu.rect.x -= menu.rect.w;
	if (menu.rect.y + menu.rect.h >= sc_h)
		menu.rect.y -= menu.rect.h;
}

void Workspace::update(Camera& view, Input& input, Point& cursor) {
	Point inside = {0};
	Box *hover = box_under_cursor(view, cursor, inside);

	// There are three main ways for a box to be distinguished a any time: if it's hovered, focussed or held with the mouse.
	if (!view.moving) {
		if (hover && (input.lclick || input.rclick)) {
			sdl_acquire_mouse();

			// This function makes it so the given box is the last to be rendered, making it the box at the front.
			// The box at the front is the focussed box.
			bring_to_front(hover);
			if (!input.rclick)
				selected = hover;
		}

		if (input.rclick) {
			show_rclick_menu = true;
			Point size = {0};
			if (hover) {
				rclick_box = hover;
				rclick_box->prepare_rclick_menu(rclick_menu, view, cursor);
				rclick_menu.list = &rclick_box->rclick_menu_items;
			}
			else {
				rclick_box = nullptr;
				prepare_rclick_menu(view, cursor);
				rclick_menu.list = &default_rclick_menu;
			}

			reposition_rclick_menu(rclick_menu, view, input.mouse_x, input.mouse_y);
		}
	}

	if (!input.lmouse) {
		if (selected)
			selected->ui_held = false;

		selected = nullptr;
	}

	bool disable_mouse_click = input.lctrl || input.rctrl;
	rclick_menu.hl = -1;

	if (show_rclick_menu) {
		if (rclick_menu.rect.contains(input.mouse_x, input.mouse_y)) {
			rclick_menu.hl = (input.mouse_y - rclick_menu.rect.y) / rclick_menu.get_item_height();
			auto& hl_item = (*rclick_menu.list)[rclick_menu.hl];

			if (input.action && (hl_item.flags & FLAG_INACTIVE) == 0) {
				auto action = hl_item.action;
				if (action)
					action(*this, rclick_box);

				show_rclick_menu = false;
			}

			disable_mouse_click = true;
		}
		else if (input.lclick) {
			show_rclick_menu = false;
		}
	}

	Input box_input = input;
	if (disable_mouse_click) {
		box_input.lmouse = box_input.lclick = false;
		//box_input.rmouse = box_input.rclick = false;
		box_input.scroll_x = box_input.scroll_y = 0;
		box_input.action = false;
	}

	cursor_set = false;
	for (int i = boxes.size() - 1; i >= 0; i--)
		boxes[i]->update(*this, view, box_input, hover, i == boxes.size() - 1);

	if (!cursor_set)
		sdl_set_cursor(CursorDefault);

	if (new_box) {
		new_box->parent = this;
		new_box->refresh(&inside);
		new_box->handle_zoom(*this, view.scale);
		new_box->update_ui(view);
		new_box = nullptr;
	}

	refresh_sources();

	sdl_clear();

	for (int i = 0; i < boxes.size(); i++) {
		if (boxes[i]->visible) {
			bool hovered = hover == boxes[i];
			boxes[i]->draw(*this, view, box_input.lmouse, hovered ? &inside : nullptr, hovered, i == boxes.size() - 1);
		}
	}

	if (show_rclick_menu) {
		sdl_draw_rect(rclick_menu.rect, colors.back, nullptr);

		int item_h = rclick_menu.get_item_height();
		if (rclick_menu.hl >= 0) {
			Rect_Int r = rclick_menu.rect;
			r.y += rclick_menu.hl * item_h;
			r.h = item_h;
			sdl_draw_rect(r, colors.hl, nullptr);
		}

		auto& list = *rclick_menu.list;
		int x = rclick_menu.rect.x + 5;
		int y = rclick_menu.rect.y - 3;

		for (auto& it : list) {
			if (it.flags & FLAG_INVISIBLE)
				continue;

			Font *font = (it.flags & FLAG_INACTIVE) ? rclick_menu.inactive_font : rclick_menu.font;
			font->render.draw_text_simple(nullptr, it.name, x, y);
			y += item_h;
		}
	}

	sdl_render();
}

void Workspace::update_structs(std::string& text) {
	if (first_struct_run) {
		int reserve = text.size() + 2;
		tokens.try_expand(reserve);
		name_vector.try_expand(reserve);
		first_struct_run = false;
	}
	else {
		tokens.head = 0;
		name_vector.head = 0;
		struct_names.clear();

		for (auto& s : structs) {
			s->flags |= FLAG_AVAILABLE;
			s->fields.zero_out();
		}

		definitions.erase_all_similar_to(FLAG_PRIMITIVE | FLAG_ENUM | FLAG_ENUM_ELEMENT);
		set_primitives(definitions);
	}

	tokenize(tokens, text.c_str(), text.size());

	parse_typedefs_and_enums(definitions, tokens);

	char *tokens_alias = tokens.pool;
	parse_c_struct(structs, &tokens_alias, name_vector, definitions);

	auto view_defs = first_box_of_type<View_Definitions>();
	if (view_defs)
		view_defs->update_tables();

	for (auto& s : structs) {
		char *name = name_vector.at(s->name_idx);
		if (name)
			struct_names.push_back(name);
	}

	for (auto& b : boxes) {
		if (b->box_type == BoxViewObject)
			populate_object_table<View_Object>((View_Object*)b, structs, name_vector);
		else if (b->box_type == BoxSearch && b->menu_type == MenuObject)
			populate_object_table<Search_Menu>((Search_Menu*)b, structs, name_vector);
	}
}

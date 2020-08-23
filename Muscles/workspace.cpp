#include "muscles.h"
#include "ui.h"

Workspace::Workspace(Font_Face face) {
	this->face = face;

	RGBA cross_color = { 1.0, 1.0, 1.0, 1.0 };
	cross = make_cross_icon(cross_color, 48);

	sdl_get_dpi(dpi_w, dpi_h);

	default_font = new Font(face, 12, text_color, dpi_w, dpi_h);
	fonts.push_back(default_font);

	inactive_font = new Font(face, 12, inactive_text_color, dpi_w, dpi_h);
	fonts.push_back(inactive_font);

	Box *opening = new Box();
	make_opening_menu(*this, *opening);
	add_box(opening);
}

Workspace::~Workspace() {
	sdl_destroy_texture(&cross);

	for (auto& b : boxes) {
		for (auto& e : b->ui)
			e->release();

		if (b->markup)
			delete b->markup;

		delete b;
	}

	for (auto& f : fonts)
		delete f;

	for (auto& s : sources) {
		close_source(*s);
		delete s;
	}
}

Box *Workspace::box_under_cursor(Camera& view, Point& cur, Point& inside) {
	for (int i = boxes.size()-1; i >= 0; i--) {
		if (!boxes[i]->visible)
			continue;

		Rect& r = boxes[i]->box;
		float e = boxes[i] == focus ? boxes[i]->border / view.scale : 0;

		if (cur.x >= r.x - e && cur.x < r.x+r.w + e && cur.y >= r.y - e && cur.y < r.y+r.h + e) {
			inside.x = cur.x - r.x;
			inside.y = cur.y - r.y;
			return boxes[i];
		}
	}

	return nullptr;
}

void Workspace::add_box(Box *b) {
	boxes.push_back(b);
	b->edge_color = dark_color;
	new_box = b;
}

void Workspace::delete_box(int idx) {
	if (idx < 0)
		return;

	boxes[idx]->visible = false;
	/*
	boxes.erase(boxes.begin()+idx);
	for (int i = 0; i < order.size(); i++) {
		if (order[i] == idx) {
			order.erase(order.begin()+i);
			break;
		}
	}

	for (auto& o : order) {
		if (o > idx)
			o--;
	}
	*/
}

void Workspace::delete_box(Box *b) {
	for (int i = 0; i < boxes.size(); i++) {
		if (b == boxes[i]) {
			delete_box(i);
			return;
		}
	}
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

Font *Workspace::make_font(float size, RGBA& color) {
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
	f->adjust_scale(temp_scale, dpi_w, dpi_h);
	return f;
}

Box *Workspace::first_box_of_type(BoxType type) {
	for (auto& b : boxes) {
		if (b->type == type)
			return b;
	}
	return nullptr;
}

void Workspace::adjust_scale(float old_scale, float new_scale) {
	for (auto& f : fonts)
		f->adjust_scale(new_scale, dpi_w, dpi_h);

	sdl_destroy_texture(&cross);
	cross = make_cross_icon(text_color, cross_size * new_scale);

	for (auto& b : boxes) {
		if (b->scale_change_handler)
			b->scale_change_handler(*this, *b, new_scale);
	}
}

void Workspace::refresh_sources() {
	Arena* arena = get_default_arena();
	auto& sources = (std::vector<Source*>&)this->sources;
	for (auto& s : sources) {
		s->region_refreshed = false;
		if (!s->block_region_refresh && s->timer % s->refresh_region_rate == 0) {
			if (s->type == SourceFile)
				refresh_file_region(*s, *arena);
			else if (s->type == SourceProcess)
				refresh_process_regions(*s, *arena);

			s->region_refreshed = true;
		}
		if (s->timer % s->refresh_span_rate == 0) {
			s->gather_data(*arena);
		}
		s->timer++;
	}
}

void Workspace::update(Camera& view, Input& input, Point& cursor) {
	temp_scale = view.scale;

	Point inside = {0};
	Box *hover = box_under_cursor(view, cursor, inside);

	if (input.lclick || input.rclick) {
		if (!view.moving) {
			sdl_acquire_mouse();
			focus = hover;
			if (hover)
				bring_to_front(focus);
		}

		if (hover && !input.rclick)
			selected = hover;
		/*
		if (!hover || !hover->current_dd || hover->current_dd->sel < 0) {
			for (auto& b : boxes) {
				if (b->current_dd)
					b->current_dd->dropped = false;

				b->current_dd = nullptr;
			}
		}
		*/
	}

	if (!input.lmouse) {
		if (selected)
			selected->ui_held = false;

		selected = nullptr;
	}

	Input box_input = input;
	if (input.lctrl || input.rctrl) {
		box_input.lmouse = box_input.lclick = false;
		box_input.rmouse = box_input.rclick = false;
		box_input.scroll_x = box_input.scroll_y = 0;
		box_input.action = false;
	}

	cursor_set = false;
	for (int i = boxes.size() - 1; i >= 0; i--)
		boxes[i]->update(*this, view, box_input, hover, focus == boxes[i]);

	if (!cursor_set)
		sdl_set_cursor(CursorDefault);

	if (new_box) {
		new_box->parent = this;

		if (new_box->refresh_handler)
			new_box->refresh_handler(*new_box, inside);

		if (new_box->scale_change_handler)
			new_box->scale_change_handler(*this, *new_box, view.scale);

		if (new_box->update_handler)
			new_box->update(*this, view, box_input, nullptr, false);

		new_box = nullptr;
	}

	refresh_sources();

	sdl_clear();

	for (int i = 0; i < boxes.size(); i++) {
		if (boxes[i]->visible) {
			bool hovered = hover == boxes[i];
			boxes[i]->draw(*this, view, box_input.lmouse, hovered ? &inside : nullptr, hovered, focus == boxes[i]);
		}
	}

	sdl_render();
}

void Box::draw(Workspace& ws, Camera& view, bool held, Point *inside, bool hovered, bool focussed) {
	if (!visible)
		return;

	Point p = view.to_screen(box.x, box.y);
	float w = box.w * view.scale;
	float h = box.h * view.scale;

	float brd = edge_size * view.scale;
	Rect edge_rect = {
		p.x - brd, p.y - brd, w + 2*brd, h + 2*brd
	};
	sdl_draw_rect(edge_rect, edge_color, nullptr);

	Rect_Int rect = {
		p.x, p.y, w, h
	};
	sdl_draw_rect(rect, back, nullptr);

	for (auto& e : ui) {
		if (e->visible)
			e->draw(view, rect, inside ? e->pos.contains(*inside) : false, hovered, focussed);
	}

	if (current_dd)
		current_dd->draw_menu(nullptr, view, rect);
}

void Box::select_edge(Camera& view, Point& p) {
	edge = 0;
	float gap = 6 / view.scale;
	if (p.x >= -gap && p.x < box.w + gap && p.y >= -gap && p.y < box.h + gap) {
		if (p.x < 0)
			edge |= 1;
		if (p.x >= box.w)
			edge |= 2;
		if (p.y < 0)
			edge |= 4;
		if (p.y >= box.h)
			edge |= 8;
	}
}

void Box::move(float dx, float dy, Camera& view, Input& input) {
	if (!edge) {
		box.x += dx;
		box.y += dy;
		return;
	}

	Point p = view.to_screen(box.x, box.y);
	float w = box.w * view.scale;
	float h = box.h * view.scale;

	float min_w = min_width * view.scale;
	float min_h = min_height * view.scale;

	if (edge & 2) {
		if (input.mouse_x - min_w >= (int)p.x)
			box.w += dx;
		else
			box.w = min_w / view.scale;
	}
	else if (edge & 1) {
		if (input.mouse_x + min_w < p.x + w) {
			box.x += dx;
			box.w -= dx;
		}
		else {
			box.x = view.to_world_x(p.x + w - min_w);
			box.w = min_w / view.scale;
		}
	}

	if (edge & 8) {
		if (input.mouse_y - min_h >= (int)p.y)
			box.h += dy;
		else
			box.h = min_h / view.scale;
	}
	else if (edge & 4) {
		if (input.mouse_y + min_h < p.y + h) {
			box.y += dy;
			box.h -= dy;
		}
		else {
			box.y = view.to_world_y(p.y + h - min_h);
			box.h = min_h / view.scale;
		}
	}
}

void Box::set_dropdown(Drop_Down *dd) {
	if (current_dd)
		current_dd->dropped = false;

	current_dd = dd;
	if (current_dd)
		current_dd->dropped = true;

	dropdown_set = true;
}

void Box::update_elements(Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	if (input.lclick) {
		if (active_edit) {
			if (active_edit->disengage(input, true))
				input.lclick = false;
		}
		active_edit = nullptr;
	}

	for (auto& elem : ui) {
		elem->parent = this;
		if (!elem->visible)
			continue;

		elem->mouse_handler(view, input, inside, this == hover);

		if (focussed)
			elem->key_handler(view, input);
	}

	if (!hover || this == hover) {
		if (!input.lmouse)
			select_edge(view, inside);

		Cursor_Type ct = CursorDefault;
		if (edge == 5 || edge == 10)
			ct = CursorResizeNWSE;
		else if (edge == 6 || edge == 9)
			ct = CursorResizeNESW;
		else if (edge & 3)
			ct = CursorResizeWestEast;
		else if (edge & 12)
			ct = CursorResizeNorthSouth;

		if (ct != CursorDefault) {
			sdl_set_cursor(ct);
			parent->cursor_set = true;
		}

		if (!ui_held && (edge > 0 || (!edge && parent->selected == this))) {
			if (input.lmouse && input.held == move_start) {
				moving = true;
				parent->box_moving = true;
			}
		}
	}
}

void Box::post_update_elements(Camera& view, Input& input, Point& inside, Box *hover, bool focussed) {
	bool hovered = this == hover;
	if (hovered && input.action && current_dd && current_dd->action) {
		current_dd->action(current_dd, input.double_click);
		input.action = false;
	}

	bool hl = false;
	for (auto& elem : ui) {
		if (!elem->visible)
			continue;

		bool within = elem->pos.contains(inside);
		if (!hl && hovered && !moving) {
			hl = elem->highlight(view, inside);
			if (within && !elem->use_default_cursor && !parent->cursor_set) {
				sdl_set_cursor(elem->cursor_type);
				parent->cursor_set = true;
			}
		}
		else
			elem->deselect();

		if (focussed && input.action && elem->active && elem->action && within) {
			elem->action(elem, input.double_click);
			input.action = false;
		}
	}
}

void Box::update(Workspace& ws, Camera& view, Input& input, Box *hover, bool focussed) {
	this->parent = &ws;

	if (!visible)
		return;

	if (moving) {
		float dx = (input.mouse_x - input.prev_x) / view.scale;
		float dy = (input.mouse_y - input.prev_y) / view.scale;
		move(dx, dy, view, input);
	}

	Point cursor = view.to_world(input.mouse_x, input.mouse_y);
	Point p = {cursor.x - box.x, cursor.y - box.y};

	if (current_dd)
		current_dd->highlight(view, p);

	if (refresh_handler && ticks % refresh_every == 0)
		refresh_handler(*this, p);
	ticks++;

	dropdown_set = false;

	if (update_handler)
		update_handler(*this, view, input, p, hover, focussed);

	if (input.lclick && !dropdown_set) {
		if (current_dd)
			current_dd->dropped = false;
		current_dd = nullptr;
	}
}
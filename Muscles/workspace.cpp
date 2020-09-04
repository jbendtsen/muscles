#include "muscles.h"
#include "ui.h"
#include "dialog/dialog.h"

Workspace::Workspace(Font_Face face) {
	this->face = face;

	RGBA cross_color = { 1.0, 1.0, 1.0, 1.0 };
	cross = make_cross_icon(cross_color, 48);

	sdl_get_dpi(dpi_w, dpi_h);

	default_font = new Font(face, 12, text_color, dpi_w, dpi_h);
	fonts.push_back(default_font);

	inactive_font = new Font(face, 12, inactive_text_color, dpi_w, dpi_h);
	fonts.push_back(inactive_font);

	u32 flags = FLAG_OCCUPIED | FLAG_PRIMITIVE;
	auto set = [flags](Bucket& buck, u32 type, u64 value) {
		buck.flags |= flags | type;
		buck.value = value;
	};

	set(definitions.insert("char"), FLAG_SIGNED, 8);
	set(definitions.insert("int8_t"), FLAG_SIGNED, 8);
	set(definitions.insert("uint8_t"), 0, 8);
	set(definitions.insert("short"), FLAG_SIGNED, 16);
	set(definitions.insert("int16_t"), FLAG_SIGNED, 16);
	set(definitions.insert("uint16_t"), 0, 16);
	set(definitions.insert("int"), FLAG_SIGNED, 32);
	set(definitions.insert("int32_t"), FLAG_SIGNED, 32);
	set(definitions.insert("uint32_t"), 0, 32);
	set(definitions.insert("long"), FLAG_SIGNED, 32);
	set(definitions.insert("s64"), FLAG_SIGNED, 64);
	set(definitions.insert("float"), FLAG_FLOAT, 32);
	set(definitions.insert("double"), FLAG_FLOAT, 64);

	make_box(BoxOpening);
}

Workspace::~Workspace() {
	sdl_destroy_texture(&cross);

	for (auto& b : boxes) {
		for (auto& e : b->ui)
			e->release();

		delete b;
	}

	for (auto& f : fonts)
		delete f;

	for (auto& s : sources) {
		close_source(*s);
		delete s;
	}
}

Box *Workspace::make_box(BoxType btype, MenuType mtype) {
	// 1
	Box *box = first_box_of_type(btype);
	if (box && !box->expungable) {
		box->visible = true;
		bring_to_front(box);
	}
	else {
		Box *b = nullptr;
		switch (btype) {
			case BoxOpening:
				b = new Opening_Menu(*this);
				break;
			case BoxMain:
				b = new Main_Menu(*this);
				break;
			case BoxOpenSource:
				b = new Source_Menu(*this, mtype);
				break;
			case BoxViewSource:
				b = new View_Source(*this, mtype);
				break;
			case BoxStructs:
				b = new Edit_Structs(*this);
				break;
			case BoxObject:
				b = new View_Object(*this);
				break;
			case BoxDefinitions:
				b = new View_Definitions(*this);
				break;
		}

		if (b) {
			b->box_type = btype;
			b->parent = this;
			b->visible = true;
			add_box(b);
		}

		box = b;
	}

	if (box)
		box->wake_up();

	return box;
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

	if (!boxes[idx]->expungable)
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
		if (b->box_type == type)
			return b;
	}
	return nullptr;
}

void Workspace::adjust_scale(float old_scale, float new_scale) {
	for (auto& f : fonts)
		f->adjust_scale(new_scale, dpi_w, dpi_h);

	sdl_destroy_texture(&cross);
	cross = make_cross_icon(text_color, cross_size * new_scale);

	for (auto& b : boxes)
		b->handle_zoom(*this, new_scale);
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

	// There are three main ways for a box to be distinguished a any time: if it's hovered, focussed or held with the mouse.
	if (hover && (input.lclick || input.rclick) && !view.moving) {
		sdl_acquire_mouse();

		// This function makes it so the given box is the last to be rendered, making it the box at the front.
		// The box at the front is the focussed box.
		bring_to_front(hover);
		if (!input.rclick)
			selected = hover;
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
		boxes[i]->update(*this, view, box_input, hover, i == boxes.size() - 1);

	if (!cursor_set)
		sdl_set_cursor(CursorDefault);

	if (new_box) {
		new_box->parent = this;
		new_box->refresh(inside);
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
		e->parent = this;
		if (e->visible)
			e->draw(view, rect, inside ? e->pos.contains(*inside) : false, hovered, focussed);
	}

	if (current_dd)
		current_dd->draw_menu(nullptr, view, rect);
}

void Box::require_redraw() {
	for (auto& elem : ui)
		elem->needs_redraw = true;
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
	if (current_dd && dd != current_dd)
		current_dd->cancel();

	current_dd = dd;
	if (current_dd)
		current_dd->dropped = true;

	dropdown_set = true;
}

void Box::update_hovered(Camera& view, Input& input, Point& inside) {
	if (!moving)
		select_edge(view, inside);

	CursorType ct = CursorDefault;
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

void Box::update_focussed(Camera& view, Input& input, Point& inside, Box *hover) {
	if (input.lclick) {
		if (active_edit) {
			if (active_edit->parent->disengage(input, true))
				input.lclick = false;
		}
		active_edit = nullptr;
	}

	bool hovered = this == hover;
	if (hovered && input.action && current_dd && current_dd->action) {
		current_dd->action(current_dd, input.double_click);
		input.action = false;
	}

	for (auto& elem : ui) {
		if (!elem->visible)
			continue;

		elem->mouse_handler(view, input, inside, this == hover);
		elem->key_handler(view, input);

		if (input.action && elem->active && elem->action && elem->pos.contains(inside)) {
			Workspace *ws = parent;
			ws->box_expunged = false;
			elem->action(elem, input.double_click);
			input.action = false;

			if (ws->box_expunged)
				return;
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
	else
		edge = 0;

	Point cursor = view.to_world(input.mouse_x, input.mouse_y);
	Point inside = {cursor.x - box.x, cursor.y - box.y};

	if (current_dd)
		current_dd->highlight(view, inside);

	if (ticks % refresh_every == 0)
		refresh(inside);
	ticks++;

	dropdown_set = false;

	bool hl = false;
	for (auto& elem : ui) {
		elem->parent = this;
		if (!elem->visible)
			continue;

		if (!hl && this == hover && !moving) {
			hl = elem->highlight(view, inside);

			if (elem->pos.contains(inside)) {
				Point p = {inside.x - elem->pos.x, inside.x - elem->pos.y};
				elem->scroll_handler(view, input, p);

				if (!elem->use_default_cursor && !parent->cursor_set) {
					sdl_set_cursor(elem->cursor_type);
					parent->cursor_set = true;
				}
			}
		}
		else
			elem->deselect();
	}

	update_ui(view);

	if (!parent->box_moving && (!hover || this == hover))
		update_hovered(view, input, inside);

	if (focussed)
		update_focussed(view, input, inside, hover);

	if (ws.box_expunged) {
		ws.box_expunged = false;
		return;
	}

	if (input.lclick && !dropdown_set) {
		if (current_dd)
			current_dd->dropped = false;
		current_dd = nullptr;
	}
}
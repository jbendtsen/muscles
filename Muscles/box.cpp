#include "muscles.h"
#include "ui.h"

void Box::draw(Workspace& ws, Camera& view, bool held, Point *inside, bool hovered, bool focussed) {
	if (!visible)
		return;

	Rect_Int r = view.to_screen_rect(box);

	int brd = edge_size * view.scale + 0.5;
	Rect_Int edge_rect = {
		r.x - brd, r.y - brd, r.w + 2*brd, r.h + 2*brd
	};
	sdl_draw_rect(edge_rect, edge_color, nullptr);

	sdl_draw_rect(r, back_color, nullptr);

	for (auto& e : ui) {
		if (e->visible) {
			bool elem_hovered = !dd_menu_hovered && inside && e->pos.contains(*inside);
			e->draw(view, r, elem_hovered, hovered, focussed);
		}
	}

	for (auto& e : ui) {
		if (e->visible && e->use_post_draw) {
			Rect_Int back = make_int_ui_box(r, e->pos, view.scale);
			bool elem_hovered = !dd_menu_hovered && inside && e->pos.contains(*inside);
			e->post_draw(view, back, elem_hovered, hovered, focussed);
		}
	}

	if (current_dd)
		current_dd->draw_menu(nullptr, view);
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

void Box::update_active_elements(Camera& view, Input& input, Point& inside, Box *hover) {
	if (input.lclick) {
		active_edit = nullptr;
	}

	if (current_dd && input.action && (!hover || hover == this)) {
		current_dd->base_action(view, inside, input);
		if (current_dd->action)
			current_dd->action(current_dd, view, input.double_click);

		if (current_dd->hl >= 0) {
			set_dropdown(nullptr);
			input.action = false;
		}
	}
}

void Box::update_element_actions(Camera& view, Input& input, Point& inside, Box *hover) {
	for (auto& elem : ui) {
		if (!elem->visible)
			continue;

		elem->mouse_handler(view, input, inside, this == hover);
		elem->key_handler(view, input);

		if (input.action && elem->active && elem->pos.contains(inside)) {
			Workspace *ws = parent;
			ws->box_expunged = false;

			elem->base_action(view, inside, input);
			if (elem->action) {
				elem->action(elem, view, input.double_click);
				input.action = false;
			}

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

	if (ticks % refresh_every == 0)
		refresh(&inside);
	ticks++;

	dd_menu_hovered = false;
	if (current_dd) {
		current_dd->highlight(view, inside, input);
		dd_menu_hovered = current_dd->hl >= 0;
	}

	bool was_dd = current_dd != nullptr;

	if (!parent->box_moving && !dd_menu_hovered && (!hover || this == hover))
		update_hovered(view, input, inside);

	if (focussed) {
		update_active_elements(view, input, inside, hover);
		if (!dd_menu_hovered)
			update_element_actions(view, input, inside, hover);
	}

	if (ws.box_expunged) {
		ws.box_expunged = false;
		return;
	}

	if (!dd_menu_hovered) {
		for (auto& elem : ui) {
			elem->parent = this;
			if (!elem->visible)
				continue;

			if ((!hover || this == hover) && !moving) {
				if (elem->pos.contains(inside)) {
					Point p = {inside.x - elem->pos.x, inside.y - elem->pos.y};
					elem->scroll_handler(view, input, p);

					if (!elem->use_default_cursor && !parent->cursor_set) {
						sdl_set_cursor(elem->cursor_type);
						parent->cursor_set = true;
					}
				}

				elem->highlight(view, inside, input);
			}

			if (this != hover)
				elem->deselect();
		}

		if (input.lclick && was_dd && current_dd) {
			current_dd->dropped = false;
			current_dd = nullptr;
		}
	}

	update_ui(view);
}

#include "muscles.h"
#include "ui.h"

int single_line_filterer(char *str, int len) {
	char *in = str;
	char *out = str;
	for (int i = 0; i < len; i++) {
		if (*in == '\n')
			break;

		if (*in != '\r')
			*out++ = *in;
		in++;
	}

	return out - str;
}

int multiline_ascii_filterer(char *str, int len) {
	char *in = str;
	char *out = str;

	for (int i = 0; i < len; i++) {
		if (*in == '\t' || *in == '\n' || (*in >= ' ' && *in <= '~'))
			*out++ = *in;

		in++;
	}

	return out - str;
}

void Editor::clear() {
	text.clear();
	set_cursor(primary, 0);
	set_cursor(secondary, 0);
	selected = false;
	x_offset = 0;
	y_offset = 0;
	lines = vis_lines = 0;
	columns = vis_columns = 0;
}

void Editor::set_text_from_buffer(std::pair<int, std::unique_ptr<u8[]>>&& buffer) {
	char *str = (char*)buffer.second.get();
	int len = multiline_ascii_filterer(str, buffer.first);
	text = std::string(str, len);

	set_cursor(primary, 0);
	set_cursor(secondary, 0);
	selected = false;
	x_offset = 0;
	y_offset = 0;

	measure_text();
}

void Editor::refresh(Render_Clip& clip, Font *font, float scroll_x, float scroll_y) {
	this->clip = clip;
	this->font = font;

	font_h = font->render.text_height();
	digit_w = font->render.digit_width();

	if (scroll_x >= 0)
		x_offset = scroll_x;
	if (scroll_y >= 0)
		y_offset = scroll_y;

	vis_columns = (clip.x_upper - clip.x_lower) / digit_w;
	vis_lines = (clip.y_upper - clip.y_lower) / font_h;
}

void Editor::erase(Cursor& cursor, bool is_back) {
	if (is_back) {
		if (cursor.cursor <= 0)
			return;

		set_cursor(cursor, cursor.cursor - 1);
	}
	else if (cursor.cursor >= text.size())
		return;

	text.erase(cursor.cursor, 1);
}

// Used for left/right arrow movement and when adding/removing characters
void Editor::set_cursor(Cursor& cursor, int cur) {
	int c = 0;
	int l = 0;
	int i, len = text.size();

	for (i = 0; i < cur && i < len; i++) {
		if (text[i] == '\t')
			c += tab_width;
		else
			c++;
			
		if (text[i] == '\n') {
			c = 0;
			l++;
		}
	}

	cursor.line = l;
	cursor.column = c;
	cursor.target_column = c;
	cursor.cursor = i;
}

// Used for up/down arrow movement and mouse selection on y-axis
void Editor::set_line(Cursor& cursor, int line_idx) {
	if (cursor.line == line_idx)
		return;

	int l = 0;
	int c = 0;
	int len = text.size();
	int prev_col_end = 0;
	int last_nl = 0;

	bool reached_target = false;
	int idx = 0;
	for (int i = 0; i < len; i++) {
		if (l == line_idx && c >= cursor.target_column) {
			cursor.column = c;
			reached_target = true;
			idx = i;
			break;
		}
		if (l > line_idx && line_idx >= 0) {
			cursor.column = prev_col_end;
			l--;
			idx = i-1;
			reached_target = true;
			break;
		}

		if (text[i] == '\t')
			c += tab_width;
		else
			c++;

		if (text[i] == '\n') {
			l++;
			last_nl = i+1;
			prev_col_end = c-1;
			c = 0;
		}
	}

	if (!reached_target) {
		int end = 0;
		int chars = 0;

		for (int i = last_nl; i < len; i++) {
			if (end < cursor.column)
				chars++;

			end += text[i] == '\t' ? tab_width : 1;
		}

		if (cursor.column > end || cursor.target_column >= end)
			cursor.column = end;

		cursor.cursor = last_nl + chars;
	}
	else
		cursor.cursor = idx;

	cursor.line = l;
}

// Used for home/end movement and mouse selection on x-axis
void Editor::set_column(Cursor& cursor, int col_idx) {
	int len = text.size();
	int start = 0, end = 0;
	bool past_end = true;
	int l = 0;

	for (int i = 0; i < len; i++) {
		if (text[i] != '\n')
			continue;

		if (end) start = end+1;
		end = i;

		if (l >= cursor.line) {
			past_end = false;
			break;
		}

		l++;
	}
	if (past_end) {
		if (end) start = end+1;
		end = len;
	}

	int max_cols = 0;
	int cols = 0;
	int chars = 0;
	int line_len = end - start;

	bool found_column = false;
	for (int i = 0; i < line_len; i++) {
		if (!found_column && max_cols >= col_idx) {
			cols = max_cols;
			chars = i;
			found_column = true;
		}

		max_cols += text[start+i] == '\t' ? tab_width : 1;
	}
	if (!found_column) {
		cols = max_cols;
		chars = line_len;
	}

	if (col_idx < 0 || col_idx > max_cols) {
		col_idx = max_cols;
		chars = line_len;
	}
	else
		col_idx = cols;

	cursor.column = cursor.target_column = col_idx;
	cursor.line = l;
	cursor.cursor = start + chars;
}

bool is_word_char(char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool is_space(char c) {
	return c == ' ' || c == '\t';
}

int previous_word(std::string& text, int cur) {
	if (cur > 0) cur--;
	int old = cur;

	while (cur > 0 && is_space(text[cur]))
		cur--;
	while (cur > 0 && is_word_char(text[cur]))
		cur--;
	if (old > cur && cur < text.size() && (text[cur] == ' ' || text[cur] == '\t'))
		cur++;

	return cur;
}

int next_word(std::string& text, int cur, bool selecting) {
	bool skip_word = true;
	if (is_space(text[cur])) {
		skip_word = selecting;
		while (cur < text.size() && is_space(text[cur]))
			cur++;
	}
	if (skip_word) {
		cur++;
		while (cur < text.size() && is_word_char(text[cur]))
			cur++;
	}

	if (!selecting) {
		while (cur < text.size() && (text[cur] == ' ' || text[cur] == '\t'))
			cur++;
	}

	return cur;
}

std::pair<int, int> get_text_span(int a, int b) {
	std::pair<int, int> span;
	if (a < b) {
		span.first = a;
		span.second = b - a;
	}
	else {
		span.first = b;
		span.second = a - b;
	}
	return span;
}

void Editor::measure_text() {
	lines = columns = 0;
	int cols = 0;
	for (auto& c : text) {
		cols += c == '\t' ? tab_width : 1;
		if (c == '\n') {
			lines++;
			if (cols > columns)
				columns = cols;
			cols = 0;
		}
	}
	if (cols > columns)
		columns = cols;

	lines++;
	columns++;
}

int Editor::handle_input(Input& input) {
	char ch = 0;
	bool ctrl = input.lctrl || input.rctrl;
	bool shift = input.lshift || input.rshift;
	bool pgup = input.strike(input.pgup);
	bool pgdown = input.strike(input.pgdown);

	bool esc = false;
	bool enter = false;
	bool page_move = false;
	bool key_press = true;
	bool end_selection = true;
	bool erased = false;
	bool paste = false;
	bool update = false;
	bool was_selected = selected;

	if (input.strike(input.back)) {
		if (selected && primary.cursor == secondary.cursor)
			selected = false;

		if (!selected) {
			if (ctrl) {
				int end = primary.cursor;
				int cur = primary.cursor - 1;
				if (shift) {
					shift = false;
					while (cur > 0 && text[cur] != '\n')
						cur--;
					if (text[cur] == '\n')
						cur++;
				}
				else
					cur = previous_word(text, primary.cursor);

				if (cur < 0) cur = 0;
				text.erase(cur, end - cur);
				set_cursor(primary, cur);
			}
			else
				erase(primary, true);
		}
		erased = true;
		update = true;
	}
	else if (input.strike(input.del)) {
		if (selected && primary.cursor == secondary.cursor)
			selected = false;

		if (!selected) {
			if (ctrl) {
				int start = primary.cursor;
				int cur = primary.cursor + 1;
				if (shift) {
					shift = false;
					while (cur <= text.size() && text[cur] != '\n')
						cur++;
				}
				else
					cur = next_word(text, primary.cursor, false);

				text.erase(start, cur - start);
				set_cursor(primary, start);
			}
			else
				erase(primary, false);
		}
		erased = true;
		update = true;
	}
	else if (input.strike(input.esc))
		esc = true;

	else if (input.strike(input.enter)) {
		enter = true;
		if (multiline)
			ch = '\n';
	}

	else if (multiline && input.strike(input.tab)) {
		if (secondary.cursor != primary.cursor) {
			Cursor *first  = primary.cursor < secondary.cursor ? &primary : &secondary;
			Cursor *second = primary.cursor < secondary.cursor ? &secondary : &primary;
			Cursor cur = *first;

			for (int i = first->line; i <= second->line; i++) {
				set_column(cur, 0);

				if (shift) {
					if (text[cur.cursor] == '\t')
						text.erase(cur.cursor, 1);
				}
				else
					text.insert(text.begin() + cur.cursor, '\t');

				set_line(cur, i+1);
			}

			end_selection = false;
			update = true;
		}
		else
			ch = '\t';
	}
	else if (input.strike(input.home)) {
		if (ctrl)
			set_line(primary, 0);
		set_column(primary, 0);
	}
	else if (input.strike(input.end)) {
		if (ctrl)
			set_line(primary, -1);
		set_column(primary, -1);
	}
	else if (multiline && (pgup || pgdown)) {
		float h = clip.y_upper - clip.y_lower;
		int visible = h / font_h;
		int delta = pgup ? -visible : visible;

		int req_line = primary.line + delta;
		if (req_line < 0)
			req_line = 0;

		set_line(primary, req_line);

		y_scroll_delta = (float)delta * font_h;
		page_move = true;
	}
	else if (input.strike(input.left)) {
		int cur = primary.cursor - 1;
		if (ctrl)
			cur = previous_word(text, primary.cursor);

		set_cursor(primary, cur);
	}
	else if (input.strike(input.right)) {
		int cur = primary.cursor + 1;
		if (ctrl)
			cur = next_word(text, primary.cursor, shift);

		set_cursor(primary, cur);
	}
	else if (input.strike(input.up) && !ctrl) {
		int l = primary.line - 1;
		if (l < 0) l = 0;
		set_line(primary, l);
	}
	else if (input.strike(input.down) && !ctrl) {
		set_line(primary, primary.line + 1);
	}
	else if (input.ch || input.last_key) {
		if (ctrl && use_clipboard) {
			if (selected && secondary.cursor != primary.cursor && (input.last_key == 'x' || input.last_key == 'c')) {
				auto span = get_text_span(primary.cursor, secondary.cursor);
				std::string clip_str(text, span.first, span.second);
				sdl_copy(clip_str);
				erased = input.last_key == 'x';
				end_selection = erased;
			}
			else if (input.last_key == 'v') {
				paste = true;
			}
			else if (input.last_key == 'a') {
				set_cursor(primary, 0);
				set_cursor(secondary, text.size());
				selected = true;
				end_selection = false;
			}
			else
				end_selection = false;
		}
		else if (input.ch) {
			if (numeric_only) {
				if (input.ch >= '0' && input.ch <= '9')
					ch = input.ch;
			}
			else
				ch = input.ch;
		}
	}
	else {
		end_selection = false;
		key_press = false;
	}

	if (end_selection) {
		bool expunge = selected && (erased || paste || ch);
		if (expunge && secondary.cursor != primary.cursor) {
			auto span = get_text_span(primary.cursor, secondary.cursor);
			if (primary.cursor > secondary.cursor)
				primary = secondary;

			text.erase(span.first, span.second);
			update = true;
		}
		selected = false;
	}

	if (paste) {
		auto filterer = multiline ? multiline_ascii_filterer : single_line_filterer;
		int advance = sdl_paste_into(text, primary.cursor, filterer);
		set_cursor(primary, primary.cursor + advance);
		update = true;
	}

	if (ch) {
		text.insert(text.begin() + primary.cursor, ch);
		set_cursor(primary, primary.cursor + 1);
		update = true;
	}
	else if (input.lmouse || shift)
		selected = true;

	if (!selected)
		secondary = primary;

	if (update) {
		measure_text();
	}

	if (key_press) {
		int first_vis_col  = x_offset / digit_w;
		int first_vis_line = y_offset / font_h;
		int last_vis_col   = vis_columns + x_offset / digit_w;
		int last_vis_line  = vis_lines + y_offset / font_h;

		if (primary.column < first_vis_col)
			x_scroll_delta = -digit_w * (float)(first_vis_col - primary.column);
		else if (secondary.column < first_vis_col)
			x_scroll_delta = -digit_w * (float)(first_vis_col - secondary.column);
		else if (primary.column > last_vis_col)
			x_scroll_delta = digit_w * (float)(primary.column - last_vis_col);
		else if (secondary.column > last_vis_col)
			x_scroll_delta = digit_w * (float)(secondary.column - last_vis_col);

		if (primary.line < first_vis_line)
			y_scroll_delta = -font_h * (float)(first_vis_line - primary.line);
		else if (secondary.line < first_vis_line)
			y_scroll_delta = -font_h * (float)(first_vis_line - secondary.line);
		else if (primary.line > last_vis_line)
			y_scroll_delta = font_h * (float)(primary.line - last_vis_line);
		else if (secondary.line > last_vis_line)
			y_scroll_delta = font_h * (float)(secondary.line - last_vis_line);
	}

	int flags = (update || enter) ? 1 : 0;
	flags |= (key_press || update || selected != was_selected) ? 2 : 0;
	flags |= end_selection ? 4 : 0;
	flags |= esc ? 8 : 0;
	flags |= page_move ? 16 : 0;
	return flags;
}

void Editor::update_cursor(float x, float y, Font *font, float scale, int ticks, bool click) {
	float edge = border * scale;
	float gl_w = font->render.digit_width();
	float gl_h = font->render.text_height();

	x += x_offset - edge;
	y += y_offset - edge;

	int line = y / gl_h;
	int col = x / gl_w + 0.5;
	if (line < 0) line = 0;
	if (col < 0) col = 0;

	if (ticks - last_tick < click_window) {
		if (click)
			click_run++;
	}
	else
		click_run = 1;

	if (click) {
		if (click_run == 1) {
			set_line(primary, line);
			set_column(primary, col);
			secondary = primary;
		}
		else if (click_run == 2) {
			set_cursor(primary, previous_word(text, primary.cursor + 1));
			set_cursor(secondary, next_word(text, primary.cursor, true));
			selected = true;
		}
		else if (click_run == 3) {
			set_column(primary, 0);
			secondary = primary;
			set_column(secondary, -1);
			selected = true;
		}

		last_tick = ticks;
	}
	else if (click_run <= 1) {
		set_line(primary, line);
		set_column(primary, col);
	}
}

void Editor::apply_scroll(Scroll *hscroll, Scroll *vscroll) {
	if (hscroll)
		hscroll->scroll(x_scroll_delta);
	else {
		x_offset += x_scroll_delta;
		x_offset = x_offset < 0 ? 0 : x_offset;
	}

	if (vscroll)
		vscroll->scroll(y_scroll_delta);
	else {
		y_offset += y_scroll_delta;
		y_offset = y_offset < 0 ? 0 : y_offset;
	}

	x_scroll_delta = y_scroll_delta = 0;
}

void Editor::draw_selection_box(Renderer renderer, RGBA& color) {
	float line_pad = font_h * 0.15f;
	Editor::Cursor *first  = primary.cursor < secondary.cursor ? &primary : &secondary;
	Editor::Cursor *second = primary.cursor < secondary.cursor ? &secondary : &primary;

	auto col_to_x = [this](int column) {
		return (column * digit_w) - x_offset;
	};
	auto line_to_y = [this](int line) {
		return (line * font_h) - y_offset;
	};

	float left = clip.x_lower;
	float top = clip.y_lower + line_pad;

	Rect_Int r = {0};
	if (first->line == second->line) {
		r.x = left + col_to_x(first->column);
		r.y = top + line_to_y(first->line);
		r.w = (second->column - first->column) * digit_w + 1;
		r.h = font_h + 1;

		sdl_draw_rect_clipped(r, clip, color, renderer);
	}
	else {
		float line_w = clip.x_upper - clip.x_lower;
		float x = col_to_x(first->column);

		r.x = left + x;
		r.y = top + line_to_y(first->line);
		r.w = line_w - x;
		r.h = font_h + 1;
		sdl_draw_rect_clipped(r, clip, color, renderer);

		int gulf = second->line - first->line - 1;
		if (gulf > 0) {
			r.x = left;
			r.y += font_h;
			r.w = line_w;
			r.h = gulf * font_h + 1;
			sdl_draw_rect_clipped(r, clip, color, renderer);
		}

		if (second->column > 0) {
			r.x = left;
			r.y = top + line_to_y(second->line);
			r.w = col_to_x(second->column) + 1;
			r.h = font_h + 1;
			sdl_draw_rect_clipped(r, clip, color, renderer);
		}
	}
}

void Editor::draw_cursor(Renderer renderer, RGBA& color, Editor::Cursor& cursor, Point& pos, Render_Clip& cur_clip, float scale) {
	float line_pad = font_h * 0.15f;
	float caret_x = pos.x - x_offset + ((float)cursor.column * digit_w);
	float caret_y = pos.y - y_offset + (line_pad * 1.5f) + ((float)cursor.line * font_h);

	float caret_w = cursor_width * scale;
	if (caret_w < 1)
		caret_w = 1;

	Rect caret = { caret_x, caret_y, caret_w, font_h - line_pad };
	sdl_draw_rect_clipped(caret, cur_clip, color, renderer);
}
#include "muscles.h"
#include "ui.h"

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

int Editor::handle_input(Input& input) {
	char ch = 0;
	bool ctrl = input.lctrl || input.rctrl;
	bool shift = input.lshift || input.rshift;
	bool esc = false;
	bool enter = false;
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
		if (ctrl) {
			if (selected && secondary.cursor != primary.cursor && (input.last_key == 'x' || input.last_key == 'c')) {
				auto span = get_text_span(primary.cursor, secondary.cursor);
				std::string clip(text, span.first, span.second);
				sdl_copy(clip);
				erased = input.last_key == 'x';
			}
			else if (input.last_key == 'v') {
				paste = true;
			}
			else
				end_selection = false;
		}
		else if (input.ch)
			ch = input.ch;
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
		int advance = sdl_paste_into(text, primary.cursor);
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

	int flags = (update || enter) ? 1 : 0;
	flags |= (key_press || update || selected != was_selected) ? 2 : 0;
	flags |= end_selection ? 4 : 0;
	flags |= esc ? 8 : 0;
	return flags;
}
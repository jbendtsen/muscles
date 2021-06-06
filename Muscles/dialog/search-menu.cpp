#include "../muscles.h"
#include "../ui.h"
#include "dialog.h"

void Search_Menu::update_ui(Camera& view) {
	reposition_box_buttons(cross, maxm, box.w, cross_size);
}

void Search_Menu::handle_zoom(Workspace& ws, float new_scale) {
	cross.img = ws.cross;
	maxm.img = ws.maxm;
}

Search_Menu::Search_Menu(Workspace& ws, MenuType mtype) {
	cross.action = get_delete_box();
	cross.img = ws.cross;
	ui.push_back(&cross);

	maxm.action = get_maximize_box();
	maxm.img = ws.maxm;
	ui.push_back(&maxm);
}

#include "Render.h"

void TransformToScreen(Point base, Point* target) {
	target->x = (target->x - base.x) + ((WINDOW_WIDTH-2)/2) + 1;
	target->y = (target->y - base.y) + ((WINDOW_HEIGHT-2)/2) + 1;
}

void DrawLine(WINDOW* win, Point start, Point end) {
	
}

void DrawStatusBar(WINDOW* win, Point pos, unsigned int score) {
}

void DrawRankingBar(WINDOW* win, char* ids) {
}

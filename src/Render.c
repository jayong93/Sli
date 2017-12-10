#include <malloc.h>
#include <pthread.h>
#include "Render.h"

void TransformToScreen(Point base, Point* target) {
	target->x = (target->x - base.x) + ((WINDOW_WIDTH-2)/2) + 1;
	target->y = (target->y - base.y) + ((WINDOW_HEIGHT-2)/2) + 1;
}

void DrawLine(WINDOW* win, Point start, Point end) {
	int xoffset = (end.x > start.x)?1:((end.x < start.x)?-1:0);
	int yoffset = (end.y > start.y)?1:((end.y < start.y)?-1:0);

	if (xoffset == 0 && yoffset == 0) return;
	int cx = start.x + xoffset, cy = start.y + yoffset;

	for (; cx != end.x || cy != end.y; cx += xoffset, cy += yoffset) {
		if (cx < 1 || cx > WINDOW_WIDTH-2) break;
		if (cy < 1 || cy > WINDOW_HEIGHT-2) break;
		mvwaddch(win, cy, cx, 'O');
	}
}

void DrawStatusBar(WINDOW* win, Point pos, unsigned int score) {
	mvwprintw(win, WINDOW_HEIGHT-1, 2, "X, Y: %d, %d", pos.x, pos.y);
	mvwprintw(win, WINDOW_HEIGHT-1, WINDOW_WIDTH-22, "Score: %d", score);
}

void DrawRankingBar(WINDOW* win, char* ids) {
	int i;
	char* data = ids;

	wmove(win, 0, 2);
	waddstr(win, "Ranking: ");
	for (i=0; i<3; ++i) {
		int len = *(int*)data;
		int j;
		data += sizeof(int);

		wprintw(win, "%d. ", i);
		for (j=0; j<len; ++j) {
			waddch(win, data[j]);
		}
		waddstr(win, ", ");

		data += len;
	}
}

void* Render() {
}

#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "Render.h"

extern pthread_mutex_t rDataLock;

void TransformToScreen(Point base, Point* target) {
	target->x = (target->x - base.x) + ((WINDOW_WIDTH-2)/2) + 1;
	target->y = (target->y - base.y) + ((WINDOW_HEIGHT-2)/2) + 1;
}

void DrawLine(WINDOW* win, Point start, Point end) {
	int xoffset = (end.x > start.x)?1:((end.x < start.x)?-1:0);
	int yoffset = (end.y > start.y)?1:((end.y < start.y)?-1:0);

	if (xoffset == 0 && yoffset == 0) return;
	int cx = start.x + xoffset, cy = start.y + yoffset;

	for (; cx-xoffset != end.x || cy-yoffset != end.y; cx += xoffset, cy += yoffset) {
		if (cx < 1 || cx > WINDOW_WIDTH-2) break;
		if (cy < 1 || cy > WINDOW_HEIGHT-2) break;
		mvwaddch(win, cy, cx, 'O');
	}
}

void DrawStatusBar(WINDOW* win, Point pos, unsigned int score) {
	char scoreBuf[20];
	mvwprintw(win, WINDOW_HEIGHT-1, 2, "X, Y: %d, %d", pos.x, pos.y);
	sprintf(scoreBuf, "Score: %d", score);
	mvwaddstr(win, WINDOW_HEIGHT-1, WINDOW_WIDTH-2-strlen(scoreBuf), scoreBuf);
}

void DrawRankingBar(WINDOW* win, const char* ids) {
	int i;
	const char* data = ids;

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
		if (i<2)
			waddstr(win, ", ");

		data += len;
	}
}

void* Render() {
}

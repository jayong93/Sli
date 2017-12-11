#ifndef RENDER_H
#define RENDER_H

#include <ncurses.h>

#define WINDOW_WIDTH 113
#define WINDOW_HEIGHT 35

typedef struct Point_ {
	int x, y;
} Point;

void TransformToScreen(Point base, Point* target);

void DrawLine(WINDOW* win, Point start, Point end);

void DrawStatusBar(WINDOW* win, Point pos, unsigned int score);

void DrawRankingBar(WINDOW* win, const int* idxList, const size_t* idList, int playerNum);

void* Render();

#endif

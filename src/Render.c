#define NCURSES_WIDECHAR 1

#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "Render.h"
#include "Util.h"

#define BODY 'O'
#define STAR '*'

extern pthread_mutex_t rDataLock;
extern pthread_cond_t dataCopyCond;
WINDOW* mainWin;
extern char* renderData;
extern int nRenderData;

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
		mvwaddch(win, cy, cx, BODY);
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
	int isColor;

	setlocale(LC_CTYPE, "ko_KR.utf-8");

	initscr();
	isColor = has_colors();
	if (isColor)
	{
		start_color();
		int ignore[] = {0, 16, 8};
		// COLOR_PAIR 번호는 1~228
		for (int i=0, j=0; i<227; ++i, ++j) {
			for (int t=0; t < (sizeof(ignore)/sizeof(*ignore)); ++t) {
				if (ignore[t] == j) {
					++j;
					t = -1;
				}
			}
			init_pair(i+1, j, COLOR_BLACK);
		}
	}
	else {
		endwin();
		printf("this terminal has no colors.\n");
		exit(6);
	}
	cbreak();
	curs_set(0);

	mainWin = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
	noecho();

	box(mainWin, 0, 0);

	int localDataLen = 0;
	int maxDataLen = 0;
	char* localData = NULL;
	while(1) {
		// 렌더링 데이터 복사
		pthread_mutex_lock(&rDataLock);
		pthread_cond_wait(&dataCopyCond, &rDataLock);
		localDataLen = nRenderData;
		if (maxDataLen < localDataLen) {
			if (localData) free(localData);
			maxDataLen = (int)(localDataLen*1.5);
			localData = (char*)malloc(maxDataLen);
		}
		memcpy(localData, renderData, localDataLen);
		pthread_mutex_unlock(&rDataLock);

		char* pData = localData;

		int myScore = *(int*)MovePointer(&pData, sizeof(int));
		int myColor = *(int*)MovePointer(&pData, sizeof(int));
		int myPointsNum = *(int*)MovePointer(&pData, sizeof(int));
		Point p1, p2, myHead;
		myHead = *(Point*)MovePointer(&pData, sizeof(Point));

		DrawStatusBar(mainWin, myHead, myScore);

		// Draw my Data
		TransformToScreen(myHead, &p1);
		wattron(mainWin, COLOR_PAIR(1));
		mvwaddch(mainWin, p1.y, p1.x, BODY);
		wattroff(mainWin, COLOR_PAIR(1));

		wattron(mainWin, COLOR_PAIR(myColor));
		for (int i=1; i<myPointsNum; ++i) {
			p2 = *(Point*)MovePointer(&pData, sizeof(Point));
			TransformToScreen(myHead, &p2);
			DrawLine(mainWin, p1, p2);

			p1 = p2;
		}
		wattroff(mainWin, COLOR_PAIR(myColor));

		// Draw other players
		int nPlayer = *(int*)MovePointer(&pData, sizeof(int));
		for (int i=0; i<nPlayer; ++i) {
			int color = *(int*)MovePointer(&pData, sizeof(int));
			int idLen = *(int*)MovePointer(&pData, sizeof(int));
			char* id = (char*)MovePointer(&pData, idLen);

			int nPoints = *(int*)MovePointer(&pData, sizeof(int));
			p1 = *(Point*)MovePointer(&pData, sizeof(Point));
			TransformToScreen(myHead, &p1);
			wattron(mainWin, COLOR_PAIR(1));
			mvwaddch(mainWin, p1.y, p1.x, BODY);
			wattroff(mainWin, COLOR_PAIR(1));

			wattron(mainWin, COLOR_PAIR(color));
			for (int j=1; j<nPoints; ++j) {
				p2 = *(Point*)MovePointer(&pData, sizeof(Point));
				TransformToScreen(myHead, &p2);
				DrawLine(mainWin, p1, p2);
				p1 = p2;
			}
			wattroff(mainWin, COLOR_PAIR(color));
		}

		int nStar = *(int*)MovePointer(&pData, sizeof(int));
		Point star;
		for (int i=0; i<nStar; ++i) {
			star = *(Point*)MovePointer(&pData, sizeof(Point));
			mvwaddch(mainWin, star.y, star.x, STAR);
		}

		DrawRankingBar(mainWin, pData);
	}

	if (localData) free(localData);
	endwin();
}
